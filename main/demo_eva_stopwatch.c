#include "demo_eva_stopwatch.h"
#include "stopwatch_core.h"
#include "bsp_display.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "lvgl.h"
#include <stdio.h>

// EVA 贰号机专属配色
#define COLOR_EVA_BG        lv_color_hex(0x101014) // 驾驶舱深黑
#define COLOR_EVA_RED       lv_color_hex(0xC8102E) // 贰号机机体红
#define COLOR_EVA_ORANGE    lv_color_hex(0xFF5400) // 粒子警示橙
#define COLOR_EVA_YELLOW    lv_color_hex(0xFFB703) // 状态指示黄
#define COLOR_EVA_WHITE     lv_color_hex(0xEDF2F4) // 读数白
#define COLOR_EVA_PANEL     lv_color_hex(0x221215) // 内舱暗红底色

static stopwatch_engine_t s_engine;
static lv_obj_t *s_main_time_label = NULL;
static lv_obj_t *s_status_badge = NULL;
static lv_obj_t *s_lap_info_label = NULL;
static lv_obj_t *s_history_list = NULL;
static lv_obj_t *s_footer_label = NULL;
static lv_timer_t *s_update_timer = NULL;

static uint32_t get_time_ms(void) {
    return (uint32_t)(esp_timer_get_time() / 1000ULL);
}

static void update_footer_legend(void) {
    if (!s_footer_label) return;
    switch (s_engine.state) {
        case STOPWATCH_STATE_IDLE:
            lv_label_set_text(s_footer_label, "[UP] --   [OK] START   [DN] VIEW");
            break;
        case STOPWATCH_STATE_RUNNING:
            lv_label_set_text(s_footer_label, "[UP] --   [OK] PAUSE   [DN] LAP+");
            break;
        case STOPWATCH_STATE_PAUSED:
            lv_label_set_text(s_footer_label, "[UP] RST  [OK] RESUME  [DN] VIEW");
            break;
        case STOPWATCH_STATE_REVIEW:
            lv_label_set_text(s_footer_label, "[UP] PREV [OK] EXIT    [DN] NEXT");
            break;
    }
}

static void refresh_history_view(void) {
    if (!s_history_list) return;
    lv_obj_clean(s_history_list);

    char line_buf[64];
    char time_str[16];

    if (s_engine.lap_count == 0) {
        lv_obj_t *empty_lbl = lv_label_create(s_history_list);
        lv_label_set_text(empty_lbl, "NO TELEMETRY LOGGED");
        lv_obj_set_style_text_color(empty_lbl, lv_color_hex(0x666666), 0);
        return;
    }

    for (int i = s_engine.lap_count - 1; i >= 0; i--) {
        stopwatch_format_time(s_engine.laps[i].split_time_ms, time_str, sizeof(time_str));
        snprintf(line_buf, sizeof(line_buf), "%sP%02d | +%02u.%02us | %s",
                 (s_engine.state == STOPWATCH_STATE_REVIEW && s_engine.review_cursor == i) ? ">" : " ",
                 s_engine.laps[i].lap_index,
                 (unsigned int)(s_engine.laps[i].lap_time_ms / 1000),
                 (unsigned int)((s_engine.laps[i].lap_time_ms % 1000) / 10),
                 time_str);

        lv_obj_t *item = lv_label_create(s_history_list);
        lv_label_set_text(item, line_buf);
        if (s_engine.state == STOPWATCH_STATE_REVIEW && s_engine.review_cursor == i) {
            lv_obj_set_style_text_color(item, COLOR_EVA_ORANGE, 0);
        } else {
            lv_obj_set_style_text_color(item, COLOR_EVA_WHITE, 0);
        }
    }
}

static void stopwatch_timer_cb(lv_timer_t *timer) {
    char buf[32];
    uint32_t now = get_time_ms();
    uint32_t elapsed = stopwatch_get_current_elapsed(&s_engine, now);

    stopwatch_format_time(elapsed, buf, sizeof(buf));
    lv_label_set_text(s_main_time_label, buf);

    switch (s_engine.state) {
        case STOPWATCH_STATE_IDLE:
            lv_label_set_text(s_status_badge, "[ STANDBY ]");
            lv_obj_set_style_text_color(s_status_badge, COLOR_EVA_YELLOW, 0);
            break;
        case STOPWATCH_STATE_RUNNING:
            lv_label_set_text(s_status_badge, "[ SYNC RUNNING ]");
            lv_obj_set_style_text_color(s_status_badge, COLOR_EVA_ORANGE, 0);
            break;
        case STOPWATCH_STATE_PAUSED:
            lv_label_set_text(s_status_badge, "[ HOLD / SUSPEND ]");
            lv_obj_set_style_text_color(s_status_badge, COLOR_EVA_RED, 0);
            break;
        case STOPWATCH_STATE_REVIEW:
            lv_label_set_text(s_status_badge, "[ TACTICAL REVIEW ]");
            lv_obj_set_style_text_color(s_status_badge, COLOR_EVA_WHITE, 0);
            break;
    }

    snprintf(buf, sizeof(buf), "LAP COUNT: %02d / %02d", s_engine.lap_count, STOPWATCH_MAX_LAPS);
    lv_label_set_text(s_lap_info_label, buf);
}

void app_eva_stopwatch_init(void) {
    stopwatch_init(&s_engine);

    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, COLOR_EVA_BG, 0);

    // 顶部 HUD (240 x 32)
    lv_obj_t *header = lv_obj_create(scr);
    lv_obj_set_size(header, 240, 32);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(header, COLOR_EVA_RED, 0);
    lv_obj_set_style_radius(header, 0, 0);
    lv_obj_set_style_border_width(header, 0, 0);

    lv_obj_t *title = lv_label_create(header);
    lv_label_set_text(title, "EVA-02 // CHRONO-SYNC");
    lv_obj_set_style_text_color(title, COLOR_EVA_WHITE, 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);

    // 主表盘区
    lv_obj_t *gauge_panel = lv_obj_create(scr);
    lv_obj_set_size(gauge_panel, 224, 90);
    lv_obj_align(gauge_panel, LV_ALIGN_TOP_MID, 0, 36);
    lv_obj_set_style_bg_color(gauge_panel, COLOR_EVA_PANEL, 0);
    lv_obj_set_style_border_color(gauge_panel, COLOR_EVA_RED, 0);
    lv_obj_set_style_border_width(gauge_panel, 2, 0);
    lv_obj_set_style_radius(gauge_panel, 4, 0);

    s_status_badge = lv_label_create(gauge_panel);
    lv_obj_align(s_status_badge, LV_ALIGN_TOP_MID, 0, 4);

    s_main_time_label = lv_label_create(gauge_panel);
    lv_obj_set_style_text_font(s_main_time_label, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(s_main_time_label, COLOR_EVA_WHITE, 0);
    lv_label_set_text(s_main_time_label, "00:00.00");
    lv_obj_align(s_main_time_label, LV_ALIGN_CENTER, 0, 10);

    // 道次信息
    s_lap_info_label = lv_label_create(scr);
    lv_obj_align(s_lap_info_label, LV_ALIGN_TOP_LEFT, 12, 134);
    lv_obj_set_style_text_color(s_lap_info_label, COLOR_EVA_YELLOW, 0);

    // 历史道次滚动列表
    s_history_list = lv_obj_create(scr);
    lv_obj_set_size(s_history_list, 224, 115);
    lv_obj_align(s_history_list, LV_ALIGN_TOP_MID, 0, 158);
    lv_obj_set_style_bg_color(s_history_list, lv_color_hex(0x18181C), 0);
    lv_obj_set_style_border_color(s_history_list, COLOR_EVA_ORANGE, 0);
    lv_obj_set_style_border_width(s_history_list, 1, 0);
    lv_obj_set_style_radius(s_history_list, 2, 0);

    // 底部按键提示
    s_footer_label = lv_label_create(scr);
    lv_obj_align(s_footer_label, LV_ALIGN_BOTTOM_MID, 0, -6);
    lv_obj_set_style_text_color(s_footer_label, COLOR_EVA_YELLOW, 0);

    update_footer_legend();
    refresh_history_view();

    // 启动 30ms 刷新定时器
    s_update_timer = lv_timer_create(stopwatch_timer_cb, 30, NULL);
}

void app_eva_stopwatch_handle_button(bsp_button_id_t btn, bsp_button_event_t evt) {
    bsp_lvgl_lock(-1);
    uint32_t now = get_time_ms();

    if (btn == BSP_BUTTON_OK && evt == BSP_BUTTON_CLICK) {
        if (s_engine.state == STOPWATCH_STATE_IDLE || s_engine.state == STOPWATCH_STATE_PAUSED) {
            stopwatch_start(&s_engine, now);
        } else if (s_engine.state == STOPWATCH_STATE_RUNNING) {
            stopwatch_pause(&s_engine, now);
        } else if (s_engine.state == STOPWATCH_STATE_REVIEW) {
            stopwatch_exit_review(&s_engine);
        }
        refresh_history_view();
        update_footer_legend();
    } else if (btn == BSP_BUTTON_DOWN && evt == BSP_BUTTON_CLICK) {
        if (s_engine.state == STOPWATCH_STATE_RUNNING) {
            stopwatch_record_lap(&s_engine, now);
            refresh_history_view();
        } else if (s_engine.state == STOPWATCH_STATE_PAUSED || s_engine.state == STOPWATCH_STATE_IDLE) {
            stopwatch_enter_review(&s_engine);
            refresh_history_view();
            update_footer_legend();
        } else if (s_engine.state == STOPWATCH_STATE_REVIEW) {
            stopwatch_review_next(&s_engine);
            refresh_history_view();
        }
    } else if (btn == BSP_BUTTON_UP) {
        if (s_engine.state == STOPWATCH_STATE_PAUSED && evt == BSP_BUTTON_LONG_PRESS) {
            stopwatch_reset(&s_engine);
            refresh_history_view();
            update_footer_legend();
        } else if (s_engine.state == STOPWATCH_STATE_REVIEW && evt == BSP_BUTTON_CLICK) {
            stopwatch_review_prev(&s_engine);
            refresh_history_view();
        }
    }
    bsp_lvgl_unlock();
}
