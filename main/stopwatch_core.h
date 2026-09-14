#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define STOPWATCH_MAX_LAPS 50

typedef enum {
    STOPWATCH_STATE_IDLE = 0,
    STOPWATCH_STATE_RUNNING,
    STOPWATCH_STATE_PAUSED,
    STOPWATCH_STATE_REVIEW
} stopwatch_state_t;

typedef struct {
    uint8_t  lap_index;       // 1 ~ 50
    uint32_t lap_time_ms;     // 本道单独用时
    uint32_t split_time_ms;   // 到达本道时的累计总用时
} stopwatch_lap_record_t;

typedef struct {
    stopwatch_state_t state;
    stopwatch_state_t previous_state;
    uint32_t elapsed_ms;
    uint32_t start_timestamp_ms;
    uint32_t last_lap_split_ms;
    uint8_t  lap_count;
    int8_t   review_cursor;
    stopwatch_lap_record_t laps[STOPWATCH_MAX_LAPS];
} stopwatch_engine_t;

void stopwatch_init(stopwatch_engine_t *engine);
void stopwatch_start(stopwatch_engine_t *engine, uint32_t current_time_ms);
void stopwatch_pause(stopwatch_engine_t *engine, uint32_t current_time_ms);
void stopwatch_reset(stopwatch_engine_t *engine);
bool stopwatch_record_lap(stopwatch_engine_t *engine, uint32_t current_time_ms);

uint32_t stopwatch_get_current_elapsed(const stopwatch_engine_t *engine, uint32_t current_time_ms);
void stopwatch_enter_review(stopwatch_engine_t *engine);
void stopwatch_exit_review(stopwatch_engine_t *engine);
void stopwatch_review_next(stopwatch_engine_t *engine);
void stopwatch_review_prev(stopwatch_engine_t *engine);

void stopwatch_format_time(uint32_t total_ms, char *buf, size_t buf_size);
