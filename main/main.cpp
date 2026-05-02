// main.cpp — ESP-IDF entry point for BubbleLEDClock2.
//
// Task layout:
//   Core 0 "AppTask"     — setup() + loop(): WiFi, NTP, portal, scene FSM.
//   Core 1 "DisplayTask" — dm.update() at 50 Hz (animation ticks).
//
// The HT16K33 handles digit multiplexing internally over I2C, so no
// separate refresh task is required (unlike MAX6921-based VFD clocks).

#include "bubble_led_clock_app.h"
#include "logging.h"

#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static constexpr int APP_TASK_STACK = 12288;
static constexpr int APP_TASK_PRIO  = 5;
static constexpr int APP_TASK_CORE  = 0;

static void logTaskWatermarks() {
    static char buf[800];
    vTaskList(buf);
    LOGINF("--- task watermarks (name / state / prio / free_stack_words / num) ---");
    char* line = buf;
    for (char* p = buf; ; ++p) {
        if (*p == '\n' || *p == '\0') {
            bool done = (*p == '\0');
            *p = '\0';
            if (*line) LOGINF("  %s", line);
            if (done) break;
            line = p + 1;
        }
    }
    LOGINF("--- end watermarks ---");
}

// Drives animation ticks at 50 Hz on Core 1 so AppTask can block in
// WiFi / NTP without stalling the display.
static void displayTask(void* /*pvParameters*/) {
    DisplayManager& dm = BubbleLedClockApp::getInstance().getClock();
    for (;;) {
        dm.update();
        vTaskDelay(pdMS_TO_TICKS(20));  // 50 Hz
    }
}

static void appTask(void* /*pvParameters*/) {
    auto& app = BubbleLedClockApp::getInstance();
    app.setup();

    xTaskCreatePinnedToCore(
        displayTask, "DisplayTask",
        4096, nullptr, 6, nullptr, /*core=*/1);

    logTaskWatermarks();

    static TickType_t s_lastDump = 0;
    for (;;) {
        app.loop();

        TickType_t now = xTaskGetTickCount();
        if (now - s_lastDump >= pdMS_TO_TICKS(30000)) {
            s_lastDump = now;
            logTaskWatermarks();
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

extern "C" void app_main(void) {
    LOGINF(">>> BubbleLEDClock2 booting");

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
        err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    } else {
        ESP_ERROR_CHECK(err);
    }

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    xTaskCreatePinnedToCore(
        appTask, "AppTask",
        APP_TASK_STACK, nullptr, APP_TASK_PRIO, nullptr, APP_TASK_CORE);

    LOGINF(">>> tasks running; app_main returning to idle");
}
