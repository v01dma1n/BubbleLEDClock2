// bubble_led_clock_access_point_manager.h — adds app-specific rows to the captive portal.

#pragma once

#include "ESP32NTPClock.h"
#include "bubble_led_clock_preferences.h"

class BubbleLedClockAccessPointManager : public BaseAccessPointManager {
public:
    explicit BubbleLedClockAccessPointManager(BubbleLedClockPreferences& prefs)
        : BaseAccessPointManager(prefs) {}

protected:
    void initializeFormFields() override;
};
