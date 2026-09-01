#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "my_sunrise_face.h"
#include "my_location.h"
#include "sunriset.h"
#include "watch.h"
#include "watch_common_display.h"
#include "watch_utility.h"

static watch_date_time_t my_sunrise_event_for_date(
    watch_date_time_t local_date,
    movement_location_t location,
    bool sunset,
    bool *valid
) {
    double rise;
    double set;
    int16_t latitude = (int16_t)location.bit.latitude;
    int16_t longitude = (int16_t)location.bit.longitude;
    uint8_t result = sun_rise_set(
        local_date.unit.year + WATCH_RTC_REFERENCE_YEAR,
        local_date.unit.month,
        local_date.unit.day,
        (double)longitude / 100.0,
        (double)latitude / 100.0,
        &rise,
        &set
    );

    if (result != 0) {
        *valid = false;
        return local_date;
    }

    local_date.unit.hour = 0;
    local_date.unit.minute = 0;
    local_date.unit.second = 0;
    uint32_t utc_midnight = watch_utility_date_time_to_unix_time(local_date, 0);
    int32_t event_seconds = (int32_t)lround((sunset ? set : rise) * 3600.0);
    uint32_t event_timestamp = utc_midnight + event_seconds;
    int32_t offset = movement_get_timezone_offset_for_date(local_date);
    *valid = true;
    return watch_utility_date_time_from_unix_time(event_timestamp, offset);
}

static bool my_sunrise_find_next(
    movement_location_t location,
    watch_date_time_t now_local,
    uint32_t now,
    bool sunset,
    watch_date_time_t *candidate,
    uint32_t *candidate_timestamp
) {
    for (uint8_t day = 0; day < 2; day++) {
        watch_date_time_t date = watch_utility_date_time_from_unix_time(
            watch_utility_date_time_to_unix_time(now_local, 0) + (uint32_t)day * 86400,
            0
        );
        bool valid = false;
        *candidate = my_sunrise_event_for_date(date, location, sunset, &valid);
        if (!valid) return false;
        *candidate_timestamp = watch_utility_date_time_to_unix_time(
            *candidate,
            movement_get_timezone_offset_for_date(*candidate)
        );
        if (*candidate_timestamp > now) return true;
    }

    return false;
}

static void my_sunrise_select_next_event(my_sunrise_state_t *state) {
    movement_location_t location = my_location_load();
    watch_date_time_t now_local = movement_get_local_date_time();
    uint32_t now = watch_utility_date_time_to_unix_time(movement_get_utc_date_time(), 0);
    watch_date_time_t sunrise;
    watch_date_time_t sunset;
    uint32_t sunrise_timestamp = 0;
    uint32_t sunset_timestamp = 0;
    bool has_sunrise = my_sunrise_find_next(location, now_local, now, false, &sunrise, &sunrise_timestamp);
    bool has_sunset = my_sunrise_find_next(location, now_local, now, true, &sunset, &sunset_timestamp);

    if (has_sunset && (!has_sunrise || sunset_timestamp < sunrise_timestamp)) {
        state->show_sunset = true;
    } else {
        state->show_sunset = false;
    }
}

static void my_sunrise_display_event_name(bool sunset) {
    watch_display_text_with_fallback(
        WATCH_POSITION_TOP,
        sunset ? "SET  " : "RISE ",
        sunset ? "SE" : "rI"
    );
}

static void my_sunrise_update(my_sunrise_state_t *state) {
    movement_location_t location = my_location_load();
    watch_date_time_t now_local = movement_get_local_date_time();
    uint32_t now = watch_utility_date_time_to_unix_time(movement_get_utc_date_time(), 0);
    watch_date_time_t candidate = now_local;
    uint32_t candidate_timestamp = 0;
    bool valid = my_sunrise_find_next(
        location,
        now_local,
        now,
        state->show_sunset,
        &candidate,
        &candidate_timestamp
    );

    watch_clear_indicator(WATCH_INDICATOR_PM);
    watch_clear_indicator(WATCH_INDICATOR_24H);
    if (!valid) {
        watch_clear_colon();
        my_sunrise_display_event_name(state->show_sunset);
        watch_display_text(WATCH_POSITION_BOTTOM, "None  ");
        return;
    }

    state->expires = watch_utility_date_time_from_unix_time(now + 60, 0);
    watch_set_colon();
    if (movement_clock_mode_24h()) watch_set_indicator(WATCH_INDICATOR_24H);
    else if (watch_utility_convert_to_12_hour(&candidate)) watch_set_indicator(WATCH_INDICATOR_PM);

    char buf[9];
    my_sunrise_display_event_name(state->show_sunset);
    // The five-character event name uses the whole custom-LCD top row, so put
    // the event's day in the final two digits of the main display.
    snprintf(buf, sizeof(buf), "%2d%02d%02d", candidate.unit.hour, candidate.unit.minute, candidate.unit.day);
    watch_display_text(WATCH_POSITION_BOTTOM, buf);
}

void my_sunrise_face_setup(uint8_t watch_face_index, void **context_ptr) {
    (void)watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(my_sunrise_state_t));
        memset(*context_ptr, 0, sizeof(my_sunrise_state_t));
    }
}

void my_sunrise_face_activate(void *context) {
    my_sunrise_state_t *state = (my_sunrise_state_t *)context;
    if (watch_sleep_animation_is_running()) watch_stop_sleep_animation();
    my_sunrise_select_next_event(state);
}

bool my_sunrise_face_loop(movement_event_t event, void *context) {
    my_sunrise_state_t *state = (my_sunrise_state_t *)context;
    switch (event.event_type) {
        case EVENT_ACTIVATE:
            my_sunrise_update(state);
            break;
        case EVENT_TICK:
            if (movement_get_utc_date_time().reg >= state->expires.reg) my_sunrise_update(state);
            break;
        case EVENT_ALARM_LONG_PRESS:
            movement_set_clock_mode_24h(
                movement_clock_mode_24h() == MOVEMENT_CLOCK_MODE_12H
                    ? MOVEMENT_CLOCK_MODE_24H
                    : MOVEMENT_CLOCK_MODE_12H
            );
            movement_store_settings();
            my_sunrise_update(state);
            break;
        case EVENT_ALARM_BUTTON_UP:
            state->show_sunset = !state->show_sunset;
            my_sunrise_update(state);
            break;
        case EVENT_LOW_ENERGY_UPDATE:
            if (!watch_sleep_animation_is_running()) watch_start_sleep_animation(1000);
            my_sunrise_update(state);
            break;
        default:
            return movement_default_loop_handler(event);
    }
    return true;
}

void my_sunrise_face_resign(void *context) {
    (void)context;
}
