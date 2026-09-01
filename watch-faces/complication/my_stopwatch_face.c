#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include "my_stopwatch_face.h"
#include "watch.h"
#include "watch_common_display.h"
#include "watch_rtc.h"

#define MY_STOPWATCH_RUNNING_RATE 32

static void my_stopwatch_beep(void) {
    if (movement_button_should_sound()) {
        watch_buzzer_play_note_with_volume(BUZZER_NOTE_C7, 50, movement_button_volume());
    }
}

static uint32_t my_stopwatch_elapsed(const my_stopwatch_state_t *state, rtc_counter_t counter) {
    switch (state->status) {
        case MY_SW_RUNNING:
            return counter - state->start_counter;
        case MY_SW_STOPPED:
            return state->stop_counter - state->start_counter;
        case MY_SW_IDLE:
        default:
            return 0;
    }
}

static void my_stopwatch_force_redraw(my_stopwatch_state_t *state) {
    state->old_display.seconds = UINT_MAX;
    state->old_display.minutes = UINT_MAX;
    state->old_display.hours = UINT_MAX;
}

static void my_stopwatch_display(my_stopwatch_state_t *state, uint32_t ticks) {
    char buf[4];
    uint8_t hundredths = (ticks & 0x7F) * 100 / 128;
    watch_display_character_lp_seconds('0' + hundredths / 10, 8);
    watch_display_character_lp_seconds('0' + hundredths % 10, 9);

    uint32_t seconds = ticks >> 7;
    if (seconds == state->old_display.seconds) return;
    state->old_display.seconds = seconds;
    snprintf(buf, sizeof(buf), "%02lu", seconds % 60);
    watch_display_text(WATCH_POSITION_MINUTES, buf);

    uint32_t minutes = seconds / 60;
    if (minutes == state->old_display.minutes) return;
    state->old_display.minutes = minutes;
    snprintf(buf, sizeof(buf), "%02lu", minutes % 60);
    watch_display_text(WATCH_POSITION_HOURS, buf);

    uint32_t hours = (minutes / 60) % 24;
    if (hours == state->old_display.hours) return;
    state->old_display.hours = hours;
    if (hours) {
        snprintf(buf, sizeof(buf), "%2lu", hours);
        watch_display_text(WATCH_POSITION_TOP_RIGHT, buf);
    } else {
        watch_display_text(WATCH_POSITION_TOP_RIGHT, "  ");
    }
}

static void my_stopwatch_draw_indicators(const my_stopwatch_state_t *state, uint32_t elapsed) {
    watch_clear_indicator(WATCH_INDICATOR_LAP);
    if (state->status == MY_SW_RUNNING && (elapsed & 127) >= 64) watch_clear_colon();
    else watch_set_colon();
}

static void my_stopwatch_toggle(my_stopwatch_state_t *state, rtc_counter_t counter) {
    if (state->status == MY_SW_RUNNING) {
        state->status = MY_SW_STOPPED;
        state->stop_counter = counter;
        movement_request_tick_frequency(1);
    } else if (state->status == MY_SW_STOPPED) {
        state->start_counter = counter - state->stop_counter + state->start_counter;
        state->status = MY_SW_RUNNING;
        movement_request_tick_frequency(MY_STOPWATCH_RUNNING_RATE);
    } else {
        state->start_counter = counter;
        state->status = MY_SW_RUNNING;
        movement_request_tick_frequency(MY_STOPWATCH_RUNNING_RATE);
    }
}

static void my_stopwatch_reset(my_stopwatch_state_t *state) {
    state->status = MY_SW_IDLE;
    state->start_counter = 0;
    state->stop_counter = 0;
    movement_request_tick_frequency(1);
    my_stopwatch_force_redraw(state);
}

void my_stopwatch_face_setup(uint8_t watch_face_index, void **context_ptr) {
    (void)watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(my_stopwatch_state_t));
        memset(*context_ptr, 0, sizeof(my_stopwatch_state_t));
    }
}

void my_stopwatch_face_activate(void *context) {
    my_stopwatch_state_t *state = (my_stopwatch_state_t *)context;
    my_stopwatch_force_redraw(state);
    movement_request_tick_frequency(state->status == MY_SW_RUNNING ? MY_STOPWATCH_RUNNING_RATE : 1);
}

bool my_stopwatch_face_loop(movement_event_t event, void *context) {
    my_stopwatch_state_t *state = (my_stopwatch_state_t *)context;
    rtc_counter_t counter = watch_rtc_get_counter();

    switch (event.event_type) {
        case EVENT_ACTIVATE:
            watch_display_text_with_fallback(WATCH_POSITION_TOP_LEFT, "STW", "ST");
            // fall through
        case EVENT_TICK: {
            uint32_t elapsed = my_stopwatch_elapsed(state, counter);
            my_stopwatch_draw_indicators(state, elapsed);
            my_stopwatch_display(state, elapsed);
            break;
        }
        case EVENT_ALARM_BUTTON_UP:
            my_stopwatch_toggle(state, counter);
            my_stopwatch_beep();
            my_stopwatch_force_redraw(state);
            my_stopwatch_draw_indicators(state, my_stopwatch_elapsed(state, counter));
            my_stopwatch_display(state, my_stopwatch_elapsed(state, counter));
            break;
        case EVENT_ALARM_LONG_PRESS:
            my_stopwatch_reset(state);
            my_stopwatch_beep();
            my_stopwatch_draw_indicators(state, 0);
            my_stopwatch_display(state, 0);
            break;
        default:
            return movement_default_loop_handler(event);
    }

    return true;
}

void my_stopwatch_face_resign(void *context) {
    (void)context;
    movement_request_tick_frequency(1);
}
