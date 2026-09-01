#include <stdlib.h>
#include "set_timezone_face.h"
#include "watch.h"
#include "watch_utility.h"
#include "zones.h"

static bool quick_ticks_running;

static void increment_timezone(void) {
    movement_set_timezone_index(movement_get_timezone_index() + 1);
    if (movement_get_timezone_index() >= NUM_ZONE_NAMES) movement_set_timezone_index(0);
}

static void abort_quick_ticks(void) {
    if (quick_ticks_running) {
        quick_ticks_running = false;
        movement_request_tick_frequency(1);
    }
}

static void draw_timezone(void) {
    int32_t offset = movement_get_current_timezone_offset_for_zone(movement_get_timezone_index());
    uint8_t hours = abs(offset) / 3600;
    uint8_t minutes = (abs(offset) % 3600) / 60;
    char offset_buf[6];

    // The custom LCD has five usable title characters, so ±HHMM fits exactly.
    // `*` is its established plus-sign glyph in the first position.
    snprintf(offset_buf, sizeof(offset_buf), "%c%02d%02d", offset < 0 ? '-' : '*', hours % 100, minutes % 100);
    watch_display_text_with_fallback(WATCH_POSITION_TOP, offset_buf, offset < 0 ? "-Z" : "+Z");
    watch_display_text(WATCH_POSITION_BOTTOM, watch_utility_time_zone_name_at_index(movement_get_timezone_index()));
    watch_clear_colon();
    watch_clear_indicator(WATCH_INDICATOR_24H);
    watch_clear_indicator(WATCH_INDICATOR_PM);
}

void set_timezone_face_setup(uint8_t watch_face_index, void **context_ptr) {
    (void)watch_face_index;
    (void)context_ptr;
}

void set_timezone_face_activate(void *context) {
    (void)context;
    quick_ticks_running = false;
    movement_request_tick_frequency(1);
}

bool set_timezone_face_loop(movement_event_t event, void *context) {
    (void)context;
    switch (event.event_type) {
        case EVENT_ACTIVATE:
            break;
        case EVENT_TICK:
            if (quick_ticks_running) {
                if (HAL_GPIO_BTN_ALARM_read()) increment_timezone();
                else abort_quick_ticks();
            }
            break;
        case EVENT_ALARM_BUTTON_UP:
            abort_quick_ticks();
            increment_timezone();
            break;
        case EVENT_ALARM_LONG_PRESS:
            quick_ticks_running = true;
            movement_request_tick_frequency(8);
            break;
        case EVENT_ALARM_LONG_UP:
            abort_quick_ticks();
            break;
        case EVENT_TIMEOUT:
            abort_quick_ticks();
            movement_move_to_face(0);
            break;
        default:
            return movement_default_loop_handler(event);
    }
    draw_timezone();
    return true;
}

void set_timezone_face_resign(void *context) {
    (void)context;
    abort_quick_ticks();
    movement_store_settings();
    movement_request_tick_frequency(1);
}
