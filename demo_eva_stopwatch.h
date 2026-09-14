#pragma once

#include "bsp_button.h"

// 初始化 UI
void app_eva_stopwatch_init(void);

// 按键事件转发分发器
void app_eva_stopwatch_handle_button(bsp_button_id_t btn, bsp_button_event_t evt);
