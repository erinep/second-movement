#include <stdlib.h>
#include "set_date_face.h"
#include "watch.h"
#include "watch_utility.h"

#define SET_DATE_FACE_NUM_SETTINGS 3

static const char titles[SET_DATE_FACE_NUM_SETTINGS][6] = {"Year ", "Month", "Day  "};
static const char fallback_titles[SET_DATE_FACE_NUM_SETTINGS][3] = {"YR", "MO", "DA"};
static bool quick_ticks_running;

static void increment_date(watch_date_time_t date_time, uint8_t page) {
    if (page == 0) {
        date_time.unit.year = (date_time.unit.year + 1) % 60;
        uint8_t days = watch_utility_days_in_month(date_time.unit.month, date_time.unit.year + WATCH_RTC_REFERENCE_YEAR);
        if (date_time.unit.day > days) date_time.unit.day = days;
    } else if (page == 1) {
        date_time.unit.month = (date_time.unit.month % 12) + 1;
        uint8_t days = watch_utility_days_in_month(date_time.unit.month, date_time.unit.year + WATCH_RTC_REFERENCE_YEAR);
        if (date_time.unit.day > days) date_time.unit.day = days;
    } else {
        uint8_t days = watch_utility_days_in_month(date_time.unit.month, date_time.unit.year + WATCH_RTC_REFERENCE_YEAR);
        date_time.unit.day = (date_time.unit.day % days) + 1;
    }
    movement_set_local_date_time(date_time);
}

static void abort_quick_ticks(void) {
    if (quick_ticks_running) {
        quick_ticks_running = false;
        movement_request_tick_frequency(4);
    }
}

void set_date_face_setup(uint8_t watch_face_index, void **context_ptr) {
    (void)watch_face_index;
    if (*context_ptr == NULL) *context_ptr = malloc(sizeof(uint8_t));
}

void set_date_face_activate(void *context) {
    *((uint8_t *)context) = 0;
    quick_ticks_running = false;
    movement_request_tick_frequency(4);
}

bool set_date_face_loop(movement_event_t event, void *context) {
    uint8_t page = *((uint8_t *)context);
    watch_date_time_t date_time = movement_get_local_date_time();

    switch (event.event_type) {
        case EVENT_TICK:
            if (quick_ticks_running) {
                if (HAL_GPIO_BTN_ALARM_read()) increment_date(date_time, page);
                else abort_quick_ticks();
                date_time = movement_get_local_date_time();
            }
            break;
        case EVENT_ALARM_LONG_PRESS:
            quick_ticks_running = true;
            movement_request_tick_frequency(8);
            break;
        case EVENT_ALARM_LONG_UP:
            abort_quick_ticks();
            break;
        case EVENT_LIGHT_BUTTON_DOWN:
            page = (page + 1) % SET_DATE_FACE_NUM_SETTINGS;
            *((uint8_t *)context) = page;
            break;
        case EVENT_ALARM_BUTTON_UP:
            abort_quick_ticks();
            increment_date(date_time, page);
            date_time = movement_get_local_date_time();
            break;
        case EVENT_TIMEOUT:
            abort_quick_ticks();
            movement_move_to_face(0);
            break;
        default:
            if (event.event_type != EVENT_ACTIVATE) return movement_default_loop_handler(event);
            break;
    }

    char buf[8];
    watch_clear_colon();
    watch_clear_indicator(WATCH_INDICATOR_24H);
    watch_clear_indicator(WATCH_INDICATOR_PM);
    watch_display_text(WATCH_POSITION_TOP_RIGHT, "  ");
    watch_display_text_with_fallback(WATCH_POSITION_TOP, (char *)titles[page], (char *)fallback_titles[page]);
    snprintf(buf, sizeof(buf), "%2d%02d%02d", date_time.unit.year + 20, date_time.unit.month, date_time.unit.day);
    watch_display_text(WATCH_POSITION_BOTTOM, buf);

    if (event.subsecond % 2 && !quick_ticks_running) {
        if (page == 0) watch_display_text(WATCH_POSITION_HOURS, "  ");
        else if (page == 1) watch_display_text(WATCH_POSITION_MINUTES, "  ");
        else watch_display_text(WATCH_POSITION_SECONDS, "  ");
    }
    return true;
}

void set_date_face_resign(void *context) {
    (void)context;
    abort_quick_ticks();
    movement_store_settings();
    movement_request_tick_frequency(1);
}
