#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "time_since_motion_face.h"
#include "watch_utility.h"

#define HISTORY_MINUTES 1440
#define HISTORY_BYTES ((HISTORY_MINUTES + 7) / 8)
#define REPORT_PAGES 6

typedef enum {
    REPORT_LIVE,
    REPORT_REST,
    REPORT_START,
    REPORT_END,
    REPORT_STILL,
    REPORT_WAKE,
} report_page_t;

typedef struct {
    uint32_t start_timestamp;
    uint32_t end_timestamp;
    uint16_t duration_minutes;
    uint16_t still_minutes;
    uint16_t wake_count;
    uint8_t coverage_hours;
    bool valid;
} motion_report_t;

typedef struct {
    uint32_t last_motion_timestamp;
    uint32_t last_sample_minute;
    uint8_t history[HISTORY_BYTES];
    uint16_t write_index;
    uint16_t sample_count;
    uint8_t report_page;
    bool accelerometer_available;
    motion_report_t report;
} time_since_motion_state_t;

static bool _sensor_is_active(void) {
    return !HAL_GPIO_A4_read();
}

static void _record_minute(time_since_motion_state_t *state, bool active) {
    uint8_t mask = 1 << (state->write_index & 7);
    uint8_t *byte = &state->history[state->write_index >> 3];
    if (active) *byte |= mask;
    else *byte &= ~mask;

    state->write_index = (state->write_index + 1) % HISTORY_MINUTES;
    if (state->sample_count < HISTORY_MINUTES) state->sample_count++;
}

static void _reset_history(time_since_motion_state_t *state) {
    memset(state->history, 0, sizeof(state->history));
    state->write_index = 0;
    state->sample_count = 0;
}

static void _sample_motion(time_since_motion_state_t *state) {
    if (!state->accelerometer_available) return;

    uint32_t now = movement_get_utc_timestamp();
    uint32_t minute = now / 60;
    bool active = _sensor_is_active();
    if (active) state->last_motion_timestamp = now;

    if (state->last_sample_minute == UINT32_MAX) {
        _record_minute(state, active);
    } else if (minute < state->last_sample_minute) {
        // The clock moved backward; old indexes no longer describe a rolling timeline.
        _reset_history(state);
        _record_minute(state, active);
    } else if (minute > state->last_sample_minute) {
        uint32_t elapsed = minute - state->last_sample_minute;
        if (elapsed > HISTORY_MINUTES) {
            // Do not invent a day's samples after a clock jump or long power gap.
            _reset_history(state);
            elapsed = 1;
        }
        while (elapsed--) _record_minute(state, active);
    }
    state->last_sample_minute = minute;
}

static bool _history_is_active(const time_since_motion_state_t *state, uint16_t offset) {
    uint16_t oldest = state->sample_count < HISTORY_MINUTES ? 0 : state->write_index;
    uint16_t index = (oldest + offset) % HISTORY_MINUTES;
    return state->history[index >> 3] & (1 << (index & 7));
}

static uint8_t _coverage_hours(const time_since_motion_state_t *state);

static bool _minute_is_rest_like(const time_since_motion_state_t *state,
                                 uint16_t minute) {
    uint16_t half_window = TIME_SINCE_MOTION_REST_WINDOW_MINUTES / 2;
    uint16_t start = minute > half_window ? minute - half_window : 0;
    uint16_t end = minute + half_window;
    if (end > state->sample_count) end = state->sample_count;

    uint16_t samples = end - start;
    if (samples < half_window) return false;

    uint16_t still = 0;
    for (uint16_t i = start; i < end; i++) {
        if (!_history_is_active(state, i)) still++;
    }
    return (uint32_t)still * 100 >=
        (uint32_t)samples * TIME_SINCE_MOTION_REST_PERCENT;
}

static uint16_t _count_still(const time_since_motion_state_t *state,
                             uint16_t start, uint16_t end) {
    uint16_t still = 0;
    for (uint16_t i = start; i < end; i++) {
        if (!_history_is_active(state, i)) still++;
    }
    return still;
}

static uint16_t _count_wakes(const time_since_motion_state_t *state,
                             uint16_t start, uint16_t end) {
    uint16_t wakes = 0;
    uint16_t i = start;
    while (i < end) {
        if (!_history_is_active(state, i)) {
            i++;
            continue;
        }
        uint16_t run = 1;
        while (i + run < end && _history_is_active(state, i + run)) run++;
        if (run >= TIME_SINCE_MOTION_WAKE_MINUTES) wakes++;
        i += run;
    }
    return wakes;
}

static motion_report_t _analyze_history(const time_since_motion_state_t *state) {
    motion_report_t report = {0};
    report.coverage_hours = _coverage_hours(state);
    if (state->sample_count < TIME_SINCE_MOTION_MIN_REST_MINUTES) return report;

    uint32_t best_score = 0;
    uint16_t best_start = 0;
    uint16_t best_end = 0;
    uint16_t best_still = 0;
    uint16_t i = 0;

    while (i < state->sample_count) {
        while (i < state->sample_count && !_minute_is_rest_like(state, i)) i++;
        if (i == state->sample_count) break;

        uint16_t start = i;
        uint16_t last_rest = i;
        i++;
        while (i < state->sample_count) {
            if (_minute_is_rest_like(state, i)) {
                last_rest = i;
            } else if (i - last_rest > TIME_SINCE_MOTION_REST_GAP_MINUTES) {
                break;
            }
            i++;
        }

        uint16_t end = last_rest + 1;
        uint16_t duration = end - start;
        if (duration >= TIME_SINCE_MOTION_MIN_REST_MINUTES) {
            uint16_t still = _count_still(state, start, end);
            // Duration rewards a coherent interval; stillness rewards continuity.
            uint32_t score = (uint32_t)duration + still;
            if (score > best_score) {
                best_score = score;
                best_start = start;
                best_end = end;
                best_still = still;
            }
        }
    }

    if (!best_score) return report;

    uint32_t oldest_minute = state->last_sample_minute - state->sample_count + 1;
    report.start_timestamp = (oldest_minute + best_start) * 60;
    report.end_timestamp = (oldest_minute + best_end) * 60;
    report.duration_minutes = best_end - best_start;
    report.still_minutes = best_still;
    report.wake_count = _count_wakes(state, best_start, best_end);
    report.valid = true;
    return report;
}

static uint8_t _coverage_hours(const time_since_motion_state_t *state) {
    return (state->sample_count + 59) / 60;
}

static void _display_minutes(const char *label, const char *fallback,
                             uint16_t minutes, uint8_t coverage_hours) {
    char buf[7];
    uint8_t hours = (minutes / 60) % 100;
    uint8_t minute_part = minutes % 60;
    uint8_t coverage = coverage_hours % 100;
    watch_display_text_with_fallback(WATCH_POSITION_TOP, label, fallback);
    snprintf(buf, sizeof(buf), "%2u%02u%02u", hours, minute_part, coverage);
    watch_display_text(WATCH_POSITION_BOTTOM, buf);
    watch_set_colon();
}

static void _display_timestamp(const char *label, const char *fallback,
                               uint32_t timestamp) {
    char buf[7];
    watch_date_time_t local = watch_utility_date_time_from_unix_time(
        timestamp, movement_get_current_timezone_offset());
    watch_display_text_with_fallback(WATCH_POSITION_TOP, label, fallback);
    snprintf(buf, sizeof(buf), "%2u%02u%02u", local.unit.hour,
             local.unit.minute, local.unit.day);
    watch_display_text(WATCH_POSITION_BOTTOM, buf);
    watch_set_colon();
}

static void _draw(time_since_motion_state_t *state) {
    char buf[10];
    watch_clear_colon();

    if (!state->accelerometer_available) {
        watch_display_text_with_fallback(WATCH_POSITION_TOP, "INACT", "IA");
        watch_display_text(WATCH_POSITION_BOTTOM, "no ACC");
        return;
    }

    uint8_t coverage = _coverage_hours(state);
    if (state->report_page == REPORT_LIVE) {
        watch_display_text_with_fallback(WATCH_POSITION_TOP, "INACT", "IA");
        if (_sensor_is_active()) {
            watch_display_text(WATCH_POSITION_BOTTOM, "Active");
        } else {
            uint32_t elapsed = movement_get_utc_timestamp() - state->last_motion_timestamp;
            uint32_t minutes = elapsed / 60;
            if (minutes > 5999) minutes = 5999;
            uint8_t hours = (minutes / 60) % 100;
            uint8_t minute_part = minutes % 60;
            snprintf(buf, sizeof(buf), "%2u%02u%02u", hours, minute_part,
                     coverage % 100);
            watch_display_text(WATCH_POSITION_BOTTOM, buf);
            watch_set_colon();
        }
        return;
    }

    motion_report_t *report = &state->report;
    if (!report->valid) {
        watch_display_text_with_fallback(WATCH_POSITION_TOP, "REST ", "RE");
        watch_display_text(WATCH_POSITION_BOTTOM, "no dat");
    } else if (state->report_page == REPORT_REST) {
        _display_minutes("REST ", "RE", report->duration_minutes,
                         report->coverage_hours);
    } else if (state->report_page == REPORT_START) {
        _display_timestamp("START", "ST", report->start_timestamp);
    } else if (state->report_page == REPORT_END) {
        _display_timestamp("END  ", "EN", report->end_timestamp);
    } else if (state->report_page == REPORT_STILL) {
        uint16_t percent = (uint32_t)report->still_minutes * 100 /
            report->duration_minutes;
        watch_display_text_with_fallback(WATCH_POSITION_TOP, "STILL", "SL");
        snprintf(buf, sizeof(buf), " %3uPC", percent);
        watch_display_text(WATCH_POSITION_BOTTOM, buf);
    } else {
        watch_display_text_with_fallback(WATCH_POSITION_TOP, "WAKE ", "WA");
        snprintf(buf, sizeof(buf), "   %3u", report->wake_count % 1000);
        watch_display_text(WATCH_POSITION_BOTTOM, buf);
    }
}

void time_since_motion_face_setup(uint8_t watch_face_index, void **context_ptr) {
    (void)watch_face_index;
    if (*context_ptr == NULL) {
        time_since_motion_state_t *state = calloc(1, sizeof(*state));
        state->last_sample_minute = UINT32_MAX;
        movement_set_accelerometer_background_rate(LIS2DW_DATA_RATE_LOWEST);
        state->accelerometer_available =
            movement_get_accelerometer_background_rate() != LIS2DW_DATA_RATE_POWERDOWN;
        state->last_motion_timestamp = movement_get_utc_timestamp();
        *context_ptr = state;
    }
}

void time_since_motion_face_activate(void *context) {
    time_since_motion_state_t *state = context;
    state->report_page = REPORT_LIVE;
    _sample_motion(state);
    movement_request_tick_frequency(1);
}

bool time_since_motion_face_loop(movement_event_t event, void *context) {
    time_since_motion_state_t *state = context;
    switch (event.event_type) {
        case EVENT_ACTIVATE:
        case EVENT_TICK:
            _sample_motion(state);
            _draw(state);
            break;
        case EVENT_ALARM_BUTTON_UP:
            state->report_page = (state->report_page + 1) % REPORT_PAGES;
            if (state->report_page == REPORT_REST) {
                _sample_motion(state);
                state->report = _analyze_history(state);
            }
            _draw(state);
            break;
        case EVENT_ALARM_LONG_PRESS:
            state->report_page = REPORT_LIVE;
            _draw(state);
            break;
        case EVENT_LOW_ENERGY_UPDATE:
            _draw(state);
            break;
        case EVENT_TIMEOUT:
            movement_move_to_face(0);
            break;
        default:
            movement_default_loop_handler(event);
            break;
    }
    return true;
}

void time_since_motion_face_resign(void *context) {
    (void)context;
}

movement_watch_face_advisory_t time_since_motion_face_advise(void *context) {
    time_since_motion_state_t *state = context;
    _sample_motion(state);
    return (movement_watch_face_advisory_t){0};
}
