// bubble_led_clock_app.cpp — BubbleLEDClock2 application implementation.

#include "bubble_led_clock_app.h"
#include "version.h"
#include "bubble_led_clock_hardware_map.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "sntp_client.h"
#include "weather_client.h"
#include "logging.h"

#include <cstdio>
#include <cstring>
#include <ctime>

// --- WiFi activity LED -------------------------------------------------------

static esp_timer_handle_t s_ledOffTimer = nullptr;

static void ledOffCallback(void*) {
    gpio_set_level(static_cast<gpio_num_t>(LED_GPIO), 0);
}

static void ledWifiEventHandler(void*, esp_event_base_t, int32_t, void*) {
    if (LED_GPIO < 0 || !s_ledOffTimer) return;
    gpio_set_level(static_cast<gpio_num_t>(LED_GPIO), 1);
    esp_timer_stop(s_ledOffTimer);
    esp_timer_start_once(s_ledOffTimer, 80 * 1000ULL);
}

// --- Scene playlist data getters --------------------------------------------

static float blc_getTimeData()     { return UNSET_VALUE; }
static float blc_getTempF()        { return BubbleLedClockApp::getInstance().getTempF(); }
static float blc_getTempC()        { return BubbleLedClockApp::getInstance().getTempC(); }
static float blc_getHumidityData() { return BubbleLedClockApp::getInstance().getHumidityData(); }

// Scene playlist for the 8-digit bubble LED display.
// Matches the original BubbleLEDClock sequence: time / date / temp / humidity.
// Both °F and °C temperature scenes are included so both units are always shown.
static const DisplayScene s_scenePlaylist[] = {
    { "Time",        "%H.%M.%S",  SLOT_MACHINE, true,  true,  10000, 200, 50, &blc_getTimeData     },
    { "Date",        "%b %d %Y",  SCROLLING,    false, false,  7000, 300,  0, &blc_getTimeData     },
    { "Time",        "%H.%M.%S",  SLOT_MACHINE, true,  true,  10000, 200, 50, &blc_getTimeData     },
    { "Temp F",      " %3.0f F",  MATRIX,       false, false,  5000, 250, 40, &blc_getTempF        },
    { "Temp C",      " %3.0f C",  MATRIX,       false, false,  5000, 250, 40, &blc_getTempC        },
    { "Time",        "%H.%M.%S",  SLOT_MACHINE, true,  true,  10000, 200, 50, &blc_getTimeData     },
    { "Humidity",    " %3.0fPCT", MATRIX,       false, false,  5000, 250, 40, &blc_getHumidityData },
};
static const int s_numScenes = static_cast<int>(sizeof(s_scenePlaylist) / sizeof(s_scenePlaylist[0]));

// --- Singleton ---------------------------------------------------------------

BubbleLedClockApp& BubbleLedClockApp::getInstance() {
    static BubbleLedClockApp instance;
    return instance;
}

BubbleLedClockApp::~BubbleLedClockApp() = default;

BubbleLedClockApp::BubbleLedClockApp()
    : _display(BLC_I2C_PORT, BLC_I2C_SDA, BLC_I2C_SCL,
               HT16K33_I2C_ADDR, HT16K33_DISPLAY_SIZE),
      _appPrefs(),
      _apManagerConcrete(_appPrefs),
      _rtc(BLC_I2C_PORT, BLC_I2C_SDA, BLC_I2C_SCL, RTC_I2C_ADDR)
{
    _displayManager = std::make_unique<DisplayManager>(_display);
    _prefs          = &_appPrefs;
    _apManager      = &_apManagerConcrete;
}

// --- Hardware & lifecycle ----------------------------------------------------

void BubbleLedClockApp::setupHardware() {
    // WiFi activity LED.
    if constexpr (LED_GPIO >= 0) {
        gpio_config_t io = {};
        io.pin_bit_mask = 1ULL << LED_GPIO;
        io.mode         = GPIO_MODE_OUTPUT;
        io.pull_up_en   = GPIO_PULLUP_DISABLE;
        io.pull_down_en = GPIO_PULLDOWN_DISABLE;
        io.intr_type    = GPIO_INTR_DISABLE;
        gpio_config(&io);
        gpio_set_level(static_cast<gpio_num_t>(LED_GPIO), 0);

        esp_timer_create_args_t ta = {};
        ta.callback = ledOffCallback;
        ta.name     = "led_off";
        esp_timer_create(&ta, &s_ledOffTimer);

        esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, ledWifiEventHandler, nullptr);
        esp_event_handler_register(IP_EVENT,   ESP_EVENT_ANY_ID, ledWifiEventHandler, nullptr);
        LOGINF("WiFi activity LED on GPIO %d", LED_GPIO);
    }

    // DS1307 RTC — seed system clock from battery-backed time if available.
    // The RTC shares the I2C bus with the display; DispDriverHT16K33::begin()
    // tolerates ESP_ERR_INVALID_STATE if the port is already installed.
    _rtcPresent = _rtc.init();
    if (_rtcPresent && _rtc.isRunning()) {
        time_t t = _rtc.readTime();
        if (t != (time_t)-1) {
            struct timeval tv = { .tv_sec = t, .tv_usec = 0 };
            settimeofday(&tv, nullptr);
            _rtcAvailable = true;
            LOGINF("System clock seeded from DS1307");
        }
    }

    // HT16K33 display init + optional startup animation.
    _displayManager->begin();

    if (_appPrefs.config.showStartupAnimation) {
        _displayManager->setAnimation(
            std::make_unique<ScrollingTextAnimation>(APP_GREETING, 150, false));
    } else {
        _displayManager->setAnimation(
            std::make_unique<StaticTextAnimation>(APP_GREETING));
    }
}

void BubbleLedClockApp::setup() {
    BaseNtpClockApp::setup();

    _weatherManager = std::make_unique<WeatherManager>(*this);

    if (_sceneManager) _sceneManager->setup(s_scenePlaylist, s_numScenes);

    LOGINF("BubbleLEDClock2 ready");
}

void BubbleLedClockApp::loop() {
    BaseNtpClockApp::loop();

    // Weather polling (15 min, only when WiFi is up and OWM is configured).
    if (_weatherManager) _weatherManager->update();

    // Write NTP time back to DS1307 once on first sync.
    if (_rtcPresent && !_rtcSynced && timeAvail) {
        _rtc.writeTime(time(nullptr));
        _rtcSynced = true;
        LOGINF("DS1307 synchronized with NTP time");
    }
}

// --- IBaseClock implementations ----------------------------------------------

bool BubbleLedClockApp::isOkToRunScenes() const {
    return _fsmManager && _fsmManager->isInState("RUNNING_NORMAL");
}

void BubbleLedClockApp::formatTime(char* txt, unsigned txt_size,
                                   const char* format, time_t now) {
    struct tm ti;
    localtime_r(&now, &ti);
    strftime(txt, txt_size, format, &ti);
}

void BubbleLedClockApp::activateAccessPoint() {
    _apManagerConcrete.setup(BLC_HOST_NAME);

    char waiting[64];
    snprintf(waiting, sizeof(waiting), "AP MODE - JOIN %s", BLC_HOST_NAME);
    char connected[] = "GOTO 192.168.4.1";

    _displayManager->setAnimation(
        std::make_unique<ScrollingTextAnimation>(waiting, 180, false));
    _apManagerConcrete.runBlockingLoop(*_displayManager, waiting, connected);
}

// --- IWeatherClock implementations -------------------------------------------

const char* BubbleLedClockApp::getOwmCity() const {
    snprintf(_owmCityBuf, sizeof(_owmCityBuf), "%s,%s,%s",
             _appPrefs.config.owm_city,
             _appPrefs.config.owm_state_code,
             _appPrefs.config.owm_country_code);
    return _owmCityBuf;
}

// --- Weather data accessors --------------------------------------------------

float BubbleLedClockApp::getTempF() const {
    if (!_weatherManager) return UNSET_VALUE;
    const WeatherData& w = _weatherManager->getWeatherData();
    if (!w.valid) return UNSET_VALUE;
    return (w.tempC * 9.0f / 5.0f) + 32.0f;
}

float BubbleLedClockApp::getTempC() const {
    if (!_weatherManager) return UNSET_VALUE;
    const WeatherData& w = _weatherManager->getWeatherData();
    return w.valid ? w.tempC : UNSET_VALUE;
}

float BubbleLedClockApp::getHumidityData() const {
    if (!_weatherManager) return UNSET_VALUE;
    const WeatherData& w = _weatherManager->getWeatherData();
    return w.valid ? static_cast<float>(w.humidity) : UNSET_VALUE;
}
