// bubble_led_clock_app.h — BubbleLEDClock2 application.
//
// Ports the Arduino BubbleLEDClock to ESP-IDF, following the GustavClock2
// pattern. Inherits BaseNtpClockApp for the WiFi/NTP/FSM engine and
// IWeatherClock so the library WeatherManager can drive OWM polling.
//
// Hardware:
//   * 8-digit HT16K33 7-segment display over I2C.
//   * DS1307 RTC for time resilience across WiFi outages.
//   * OpenWeatherMap weather data displayed in a scene playlist.

#pragma once

#include "ESP32NTPClock.h"
#include "disp_driver_ht16k33.h"
#include "ds1307_driver.h"
#include "bubble_led_clock_preferences.h"
#include "bubble_led_clock_access_point_manager.h"

#include <memory>

static constexpr const char* BLC_HOST_NAME = "bubble-clock";

class BubbleLedClockApp : public BaseNtpClockApp, public IWeatherClock {
public:
    static BubbleLedClockApp& getInstance();
    ~BubbleLedClockApp() override;

    void setup()         override;
    void loop()          override;
    void setupHardware() override;

    BubbleLedClockPreferences& getPrefs() { return _appPrefs; }

    // Temperature in Fahrenheit and Celsius for the two scene playlist entries.
    float getTempF()        const;
    float getTempC()        const;
    float getHumidityData() const;

    // --- IBaseClock ----------------------------------------------------------
    const char* getAppName()  const override { return BLC_HOST_NAME; }
    const char* getSsid()     const override { return _appPrefs.config.ssid; }
    const char* getPassword() const override { return _appPrefs.config.password; }
    const char* getTimezone() const override { return _appPrefs.config.time_zone; }

    IDisplayDriver& getDisplay() override { return _display; }
    DisplayManager& getClock()   override { return *_displayManager; }

    bool isOkToRunScenes() const override;
    bool hasRtcTime()      const override { return _rtcAvailable; }
    void activateAccessPoint() override;
    void formatTime(char* txt, unsigned txt_size,
                    const char* format, time_t now) override;

    // --- IWeatherClock -------------------------------------------------------
    const char* getOwmApiKey() const override { return _appPrefs.config.owm_api_key; }
    // Composes "city,state,country" from the three separate preference fields.
    const char* getOwmCity()   const override;

private:
    BubbleLedClockApp();

    DispDriverHT16K33                        _display;
    std::unique_ptr<DisplayManager>          _displayManager;

    BubbleLedClockPreferences                _appPrefs;
    BubbleLedClockAccessPointManager         _apManagerConcrete;

    Ds1307Driver                             _rtc;
    bool                                     _rtcPresent   = false;
    bool                                     _rtcAvailable = false;
    bool                                     _rtcSynced    = false;

    std::unique_ptr<WeatherManager>          _weatherManager;

    // Buffer for the composed "city,state,country" OWM query string.
    mutable char _owmCityBuf[MAX_PREF_STRING_LEN * 3 + 2];
};
