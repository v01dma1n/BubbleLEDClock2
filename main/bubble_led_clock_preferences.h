// bubble_led_clock_preferences.h — application-specific NVS preferences for BubbleLEDClock2.
//
// Extends BaseConfig with weather and display preferences.
// Keeps separate city / state / country fields to match the original
// BubbleLEDClock UI; the app composes "city,state,country" when calling
// the weather client.

#pragma once

#include "ESP32NTPClock.h"

struct BubbleLedClockConfig : public BaseConfig {
    bool showStartupAnimation;
    char owm_api_key[MAX_PREF_STRING_LEN];
    char owm_city[MAX_PREF_STRING_LEN];
    char owm_state_code[MAX_PREF_STRING_LEN];
    char owm_country_code[MAX_PREF_STRING_LEN];
    // "F" for Fahrenheit (default), "C" for Celsius.
    // The IDF weather client always returns Celsius; F is derived on display.
    char tempUnit[MAX_PREF_STRING_LEN];
};

class BubbleLedClockPreferences : public BasePreferences {
public:
    BubbleLedClockPreferences();

    void getPreferences() override;
    void putPreferences() override;
    void dumpPreferences() override;

    BubbleLedClockConfig config;
};
