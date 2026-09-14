#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "bsp_display.h"
#include "bsp_button.h"
#include "demo_eva_stopwatch.h"

static const char *TAG = "main";

static void on_button_event(bsp_button_id_t btn, bsp_button_event_t evt, void *user_data) {
    app_eva_stopwatch_handle_button(btn, evt);
}

void app_main(void) {
    ESP_LOGI(TAG, "Initializing EVA-02 Tactical Chrono...");

    // 1. 初始化屏幕与 LVGL 核心
    bsp_display_init();

    // 2. 初始化按键并注册事件回调
    bsp_button_init();
    bsp_button_register_callback(BSP_BUTTON_UP, BSP_BUTTON_CLICK | BSP_BUTTON_LONG_PRESS, on_button_event, NULL);
    bsp_button_register_callback(BSP_BUTTON_DOWN, BSP_BUTTON_CLICK, on_button_event, NULL);
    bsp_button_register_callback(BSP_BUTTON_OK, BSP_BUTTON_CLICK, on_button_event, NULL);

    // 3. 线程安全地创建 UI
    bsp_lvgl_lock(-1);
    app_eva_stopwatch_init();
    bsp_lvgl_unlock();

    ESP_LOGI(TAG, "EVA-02 System Online.");
}
