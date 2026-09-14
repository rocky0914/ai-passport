#include "stopwatch_core.h"
#include <stdio.h>
#include <string.h>

void stopwatch_init(stopwatch_engine_t *engine) {
    if (!engine) return;
    memset(engine, 0, sizeof(stopwatch_engine_t));
    engine->state = STOPWATCH_STATE_IDLE;
    engine->previous_state = STOPWATCH_STATE_IDLE;
    engine->review_cursor = -1;
}

void stopwatch_start(stopwatch_engine_t *engine, uint32_t current_time_ms) {
    if (!engine) return;
    if (engine->state == STOPWATCH_STATE_IDLE || engine->state == STOPWATCH_STATE_PAUSED) {
        engine->start_timestamp_ms = current_time_ms;
        engine->state = STOPWATCH_STATE_RUNNING;
    }
}

void stopwatch_pause(stopwatch_engine_t *engine, uint32_t current_time_ms) {
    if (!engine || engine->state != STOPWATCH_STATE_RUNNING) return;
    engine->elapsed_ms += (current_time_ms - engine->start_timestamp_ms);
    engine->state = STOPWATCH_STATE_PAUSED;
}

void stopwatch_reset(stopwatch_engine_t *engine) {
    if (!engine) return;
    engine->state = STOPWATCH_STATE_IDLE;
    engine->previous_state = STOPWATCH_STATE_IDLE;
    engine->elapsed_ms = 0;
    engine->start_timestamp_ms = 0;
    engine->last_lap_split_ms = 0;
    engine->lap_count = 0;
    engine->review_cursor = -1;
}

bool stopwatch_record_lap(stopwatch_engine_t *engine, uint32_t current_time_ms) {
    if (!engine || engine->state != STOPWATCH_STATE_RUNNING) return false;
    if (engine->lap_count >= STOPWATCH_MAX_LAPS) return false;

    uint32_t total = stopwatch_get_current_elapsed(engine, current_time_ms);
    uint32_t lap_duration = total - engine->last_lap_split_ms;

    stopwatch_lap_record_t *rec = &engine->laps[engine->lap_count];
    rec->lap_index = engine->lap_count + 1;
    rec->split_time_ms = total;
    rec->lap_time_ms = lap_duration;

    engine->last_lap_split_ms = total;
    engine->lap_count++;
    return true;
}

uint32_t stopwatch_get_current_elapsed(const stopwatch_engine_t *engine, uint32_t current_time_ms) {
    if (!engine) return 0;
    if (engine->state == STOPWATCH_STATE_RUNNING) {
        return engine->elapsed_ms + (current_time_ms - engine->start_timestamp_ms);
    }
    return engine->elapsed_ms;
}

void stopwatch_enter_review(stopwatch_engine_t *engine) {
    if (!engine || engine->lap_count == 0) return;
    engine->previous_state = engine->state;
    engine->state = STOPWATCH_STATE_REVIEW;
    engine->review_cursor = engine->lap_count - 1;
}

void stopwatch_exit_review(stopwatch_engine_t *engine) {
    if (!engine || engine->state != STOPWATCH_STATE_REVIEW) return;
    engine->state = engine->previous_state;
}

void stopwatch_review_next(stopwatch_engine_t *engine) {
    if (!engine || engine->state != STOPWATCH_STATE_REVIEW) return;
    if (engine->review_cursor < (int8_t)(engine->lap_count - 1)) {
        engine->review_cursor++;
    }
}

void stopwatch_review_prev(stopwatch_engine_t *engine) {
    if (!engine || engine->state != STOPWATCH_STATE_REVIEW) return;
    if (engine->review_cursor > 0) {
        engine->review_cursor--;
    }
}

void stopwatch_format_time(uint32_t total_ms, char *buf, size_t buf_size) {
    uint32_t minutes = total_ms / (60 * 1000);
    uint32_t seconds = (total_ms / 1000) % 60;
    uint32_t centiseconds = (total_ms % 1000) / 10;
    snprintf(buf, buf_size, "%02u:%02u.%02u", (unsigned int)minutes, (unsigned int)seconds, (unsigned int)centiseconds);
}
