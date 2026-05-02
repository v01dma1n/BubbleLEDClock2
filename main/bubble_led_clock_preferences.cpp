#include "bubble_led_clock_preferences.h"

#include <cstring>

static constexpr const char* KEY_SHOW_STARTUP = "startup_anim";
static constexpr const char* KEY_OWM_API_KEY  = "owm_api_key";
static constexpr const char* KEY_OWM_CITY     = "owm_city";
static constexpr const char* KEY_OWM_STATE    = "owm_state";
static constexpr const char* KEY_OWM_COUNTRY  = "owm_country";
static constexpr const char* KEY_TEMP_UNIT    = "temp_unit";

BubbleLedClockPreferences::BubbleLedClockPreferences()
    : BasePreferences(config)
{
    std::memset(&config, 0, sizeof(config));
    config.showStartupAnimation = true;
    std::strncpy(config.owm_city,         "New York",  sizeof(config.owm_city) - 1);
    std::strncpy(config.owm_state_code,   "NY",        sizeof(config.owm_state_code) - 1);
    std::strncpy(config.owm_country_code, "US",        sizeof(config.owm_country_code) - 1);
    std::strncpy(config.tempUnit,         "F",         sizeof(config.tempUnit) - 1);
}

void BubbleLedClockPreferences::getPreferences() {
    BasePreferences::getPreferences();

    if (!openNvs(false)) return;

    config.showStartupAnimation = readBool(KEY_SHOW_STARTUP, true);
    readString(KEY_OWM_API_KEY, config.owm_api_key,     sizeof(config.owm_api_key));
    readString(KEY_OWM_CITY,    config.owm_city,         sizeof(config.owm_city));
    readString(KEY_OWM_STATE,   config.owm_state_code,   sizeof(config.owm_state_code));
    readString(KEY_OWM_COUNTRY, config.owm_country_code, sizeof(config.owm_country_code));
    readString(KEY_TEMP_UNIT,   config.tempUnit,         sizeof(config.tempUnit));

    if (config.owm_city[0]         == '\0') std::strncpy(config.owm_city,         "New York", sizeof(config.owm_city) - 1);
    if (config.owm_state_code[0]   == '\0') std::strncpy(config.owm_state_code,   "NY",       sizeof(config.owm_state_code) - 1);
    if (config.owm_country_code[0] == '\0') std::strncpy(config.owm_country_code, "US",       sizeof(config.owm_country_code) - 1);
    if (config.tempUnit[0]         == '\0') std::strncpy(config.tempUnit,         "F",        sizeof(config.tempUnit) - 1);

    closeNvs();
}

void BubbleLedClockPreferences::putPreferences() {
    BasePreferences::putPreferences();

    if (!openNvs(true)) return;

    writeBool  (KEY_SHOW_STARTUP, config.showStartupAnimation);
    writeString(KEY_OWM_API_KEY,  config.owm_api_key);
    writeString(KEY_OWM_CITY,     config.owm_city);
    writeString(KEY_OWM_STATE,    config.owm_state_code);
    writeString(KEY_OWM_COUNTRY,  config.owm_country_code);
    writeString(KEY_TEMP_UNIT,    config.tempUnit);

    closeNvs();
}

void BubbleLedClockPreferences::dumpPreferences() {
    BasePreferences::dumpPreferences();
    LOGDBG("Pref=%s: %s", KEY_SHOW_STARTUP, config.showStartupAnimation ? "yes" : "no");
    LOGDBG("Pref=%s: %s", KEY_OWM_API_KEY,  config.owm_api_key[0] ? "(set)" : "(empty)");
    LOGDBG("Pref=%s: %s", KEY_OWM_CITY,     config.owm_city);
    LOGDBG("Pref=%s: %s", KEY_OWM_STATE,    config.owm_state_code);
    LOGDBG("Pref=%s: %s", KEY_OWM_COUNTRY,  config.owm_country_code);
    LOGDBG("Pref=%s: %s", KEY_TEMP_UNIT,    config.tempUnit);
}
