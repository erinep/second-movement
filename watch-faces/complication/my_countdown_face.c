#include <stdlib.h>
#include <string.h>
#include "my_countdown_face.h"
#include "watch.h"
#include "watch_utility.h"

#define MY_COUNTDOWN_TAP_DETECTION_SECONDS 5
#define MY_COUNTDOWN_DEFAULT_PRESET_INDEX 0

static const uint8_t my_countdown_preset_minutes[] = {1, 3, 5, 10, 15, 20, 30};
#define MY_COUNTDOWN_PRESET_COUNT (sizeof(my_countdown_preset_minutes) / sizeof(my_countdown_preset_minutes[0]))

static uint32_t my_countdown_selected_seconds(const my_countdown_state_t *state) {
    return (uint32_t)my_countdown_preset_minutes[state->preset_index] * 60;
}

static void my_countdown_snap_up_to_preset(my_countdown_state_t *state) {
    for (uint8_t i = 0; i < MY_COUNTDOWN_PRESET_COUNT; i++) {
        if ((uint32_t)my_countdown_preset_minutes[i] * 60 >= state->remaining_seconds) {
            state->preset_index = i;
            state->remaining_seconds = my_countdown_selected_seconds(state);
            return;
        }
    }

    // Remaining time should never exceed the largest selectable preset, but
    // clamp to it if state from an older build or timer ever does.
    state->preset_index = MY_COUNTDOWN_PRESET_COUNT - 1;
    state->remaining_seconds = my_countdown_selected_seconds(state);
}

static void my_countdown_beep(void) {
    if (movement_button_should_sound()) {
        watch_buzzer_play_note_with_volume(BUZZER_NOTE_C7, 50, movement_button_volume());
    }
}

static void my_countdown_stop_tap_detection(my_countdown_state_t *state) {
    state->tap_detection_ticks = 0;
    movement_disable_tap_detection_if_available();
}

static void my_countdown_start_tap_detection(my_countdown_state_t *state) {
    if (movement_enable_tap_detection_if_available(false)) {
        state->tap_detection_ticks = MY_COUNTDOWN_TAP_DETECTION_SECONDS;
        state->tap_cycle_started = false;
    }
}

static void my_countdown_reset(my_countdown_state_t *state) {
    state->mode = my_cd_stopped;
    state->remaining_seconds = my_countdown_selected_seconds(state);
    movement_cancel_background_task_for_face(state->watch_face_index);
    my_countdown_start_tap_detection(state);
}

static void my_countdown_schedule(my_countdown_state_t *state) {
    uint32_t now = watch_utility_date_time_to_unix_time(
        movement_get_utc_date_time(),
        movement_get_current_timezone_offset()
    );
    state->now_ts = now;
    state->target_ts = now + state->remaining_seconds;
    watch_date_time_t target = watch_utility_date_time_from_unix_time(
        state->target_ts,
        movement_get_current_timezone_offset()
    );
    movement_schedule_background_task_for_face(state->watch_face_index, target);
}

static void my_countdown_update_remaining(my_countdown_state_t *state) {
    if (state->target_ts <= state->now_ts) state->remaining_seconds = 0;
    else state->remaining_seconds = state->target_ts - state->now_ts;
}

static void my_countdown_draw(const my_countdown_state_t *state) {
    char buf[8];
    uint32_t seconds = state->remaining_seconds;
    uint8_t hours = seconds / 3600;
    uint8_t minutes = (seconds % 3600) / 60;
    uint8_t secs = seconds % 60;

    snprintf(buf, sizeof(buf), "%2d%02d%02d", hours, minutes, secs);
    watch_display_text(WATCH_POSITION_BOTTOM, buf);
    if (state->mode == my_cd_running) watch_set_indicator(WATCH_INDICATOR_SIGNAL);
    else watch_clear_indicator(WATCH_INDICATOR_SIGNAL);
}

void my_countdown_face_setup(uint8_t watch_face_index, void **context_ptr) {
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(my_countdown_state_t));
        my_countdown_state_t *state = (my_countdown_state_t *)*context_ptr;
        memset(state, 0, sizeof(my_countdown_state_t));
        state->watch_face_index = watch_face_index;
        state->preset_index = MY_COUNTDOWN_DEFAULT_PRESET_INDEX;
        state->remaining_seconds = my_countdown_selected_seconds(state);
    }
}

void my_countdown_face_activate(void *context) {
    my_countdown_state_t *state = (my_countdown_state_t *)context;
    watch_set_colon();
    movement_request_tick_frequency(1);

    if (state->mode == my_cd_running) {
        state->now_ts = watch_utility_date_time_to_unix_time(
            movement_get_utc_date_time(),
            movement_get_current_timezone_offset()
        );
        my_countdown_update_remaining(state);
    } else {
        my_countdown_start_tap_detection(state);
    }
}

bool my_countdown_face_loop(movement_event_t event, void *context) {
    my_countdown_state_t *state = (my_countdown_state_t *)context;

    switch (event.event_type) {
        case EVENT_ACTIVATE:
            if (watch_sleep_animation_is_running()) watch_stop_sleep_animation();
            watch_display_text_with_fallback(WATCH_POSITION_TOP, "TIMER", "CD");
            my_countdown_draw(state);
            break;
        case EVENT_TICK:
            if (state->mode == my_cd_running) {
                state->now_ts++;
                my_countdown_update_remaining(state);
            }
            if (state->tap_detection_ticks && --state->tap_detection_ticks == 0) {
                movement_disable_tap_detection_if_available();
            }
            my_countdown_draw(state);
            break;
        case EVENT_ALARM_BUTTON_UP:
            if (state->mode == my_cd_running) {
                state->now_ts = watch_utility_date_time_to_unix_time(
                    movement_get_utc_date_time(),
                    movement_get_current_timezone_offset()
                );
                my_countdown_update_remaining(state);
                state->mode = my_cd_stopped;
                movement_cancel_background_task_for_face(state->watch_face_index);
                my_countdown_start_tap_detection(state);
            } else if (state->remaining_seconds) {
                my_countdown_stop_tap_detection(state);
                state->mode = my_cd_running;
                my_countdown_schedule(state);
            }
            my_countdown_beep();
            my_countdown_draw(state);
            break;
        case EVENT_ALARM_LONG_PRESS:
            my_countdown_reset(state);
            my_countdown_beep();
            my_countdown_draw(state);
            break;
        case EVENT_SINGLE_TAP:
            if (state->mode == my_cd_stopped) {
                if (state->tap_cycle_started) {
                    state->preset_index = (state->preset_index + 1) % MY_COUNTDOWN_PRESET_COUNT;
                    state->remaining_seconds = my_countdown_selected_seconds(state);
                } else {
                    state->tap_cycle_started = true;
                    my_countdown_snap_up_to_preset(state);
                }
                state->tap_detection_ticks = MY_COUNTDOWN_TAP_DETECTION_SECONDS;
                my_countdown_beep();
                my_countdown_draw(state);
            }
            break;
        case EVENT_BACKGROUND_TASK:
            if (state->mode == my_cd_running) {
                movement_play_alarm();
                my_countdown_reset(state);
                my_countdown_draw(state);
            }
            break;
        case EVENT_TIMEOUT:
            if (state->mode != my_cd_running) movement_move_to_face(0);
            break;
        case EVENT_LOW_ENERGY_UPDATE:
            if (!watch_sleep_animation_is_running()) watch_start_sleep_animation(1000);
            break;
        default:
            return movement_default_loop_handler(event);
    }

    return true;
}

void my_countdown_face_resign(void *context) {
    my_countdown_state_t *state = (my_countdown_state_t *)context;
    my_countdown_stop_tap_detection(state);
}
