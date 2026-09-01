/*
 * MIT License
 *
 * Copyright (c) 2022 Joey Castillo
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdlib.h>
#include <string.h>
#include "set_time_face.h"
#include "watch.h"
#include "watch_utility.h"
#define SET_TIME_FACE_NUM_SETTINGS (3)
const char set_time_face_titles[SET_TIME_FACE_NUM_SETTINGS][6] = {"Hour ", "Minut", "Secnd"};
const char set_time_face_fallback_titles[SET_TIME_FACE_NUM_SETTINGS][3] = {"HR", "M1", "SE"};

static bool _quick_ticks_running;

static void _handle_alarm_button(watch_date_time_t date_time, uint8_t current_page) {
    // handles short or long pressing of the alarm button

    switch (current_page) {
        case 0: // hour
            date_time.unit.hour = (date_time.unit.hour + 1) % 24;
            break;
        case 1: // minute
            date_time.unit.minute = (date_time.unit.minute + 1) % 60;
            break;
        case 2: // second
            date_time.unit.second = 0;
            break;
    }
    movement_set_local_date_time(date_time);
}

static void _abort_quick_ticks() {
    if (_quick_ticks_running) {
        _quick_ticks_running = false;
        movement_request_tick_frequency(4);
    }
}

void set_time_face_setup(uint8_t watch_face_index, void ** context_ptr) {
    (void) watch_face_index;
    if (*context_ptr == NULL) *context_ptr = malloc(sizeof(uint8_t));
}

void set_time_face_activate(void *context) {
    *((uint8_t *)context) = 0;
    movement_request_tick_frequency(4);
    _quick_ticks_running = false;
}

bool set_time_face_loop(movement_event_t event, void *context) {
    uint8_t current_page = *((uint8_t *)context);
    watch_date_time_t date_time = movement_get_local_date_time();

    switch (event.event_type) {
        case EVENT_ACTIVATE:
            break;
        case EVENT_TICK:
            if (_quick_ticks_running) {
                if (HAL_GPIO_BTN_ALARM_read()) _handle_alarm_button(date_time, current_page);
                else _abort_quick_ticks();
            }
            break;
        case EVENT_ALARM_LONG_PRESS:
            if (current_page != 2) {
                _quick_ticks_running = true;
                movement_request_tick_frequency(8);
            }
            break;
        case EVENT_ALARM_LONG_UP:
            _abort_quick_ticks();
            break;
        case EVENT_LIGHT_BUTTON_DOWN:
            current_page = (current_page + 1) % SET_TIME_FACE_NUM_SETTINGS;
            *((uint8_t *)context) = current_page;
            break;
        case EVENT_ALARM_BUTTON_UP:
            _abort_quick_ticks();
            _handle_alarm_button(date_time, current_page);
            break;
        case EVENT_TIMEOUT:
            _abort_quick_ticks();
            movement_move_to_face(0);
            break;
        default:
            return movement_default_loop_handler(event);
    }

    char buf[11];
    watch_display_text(WATCH_POSITION_TOP_RIGHT, "  ");
    watch_display_text_with_fallback(WATCH_POSITION_TOP, (char *) set_time_face_titles[current_page], (char *) set_time_face_fallback_titles[current_page]);
    watch_set_colon();
    if (movement_clock_mode_24h()) {
        watch_set_indicator(WATCH_INDICATOR_24H);
        sprintf(buf, "%2d%02d%02d", date_time.unit.hour, date_time.unit.minute, date_time.unit.second);
    } else {
        sprintf(buf, "%2d%02d%02d", (date_time.unit.hour % 12) ? (date_time.unit.hour % 12) : 12, date_time.unit.minute, date_time.unit.second);
        if (date_time.unit.hour < 12) watch_clear_indicator(WATCH_INDICATOR_PM);
        else watch_set_indicator(WATCH_INDICATOR_PM);
    }

    watch_display_text(WATCH_POSITION_BOTTOM, buf);

    // blink up the parameter we're setting
    if (event.subsecond % 2 && !_quick_ticks_running) {
        switch (current_page) {
            case 0:
                watch_display_text(WATCH_POSITION_HOURS, "  ");
                break;
            case 1:
                watch_display_text(WATCH_POSITION_MINUTES, "  ");
                break;
            case 2:
                watch_display_text(WATCH_POSITION_SECONDS, "  ");
                break;
        }
    }

    return true;
}

void set_time_face_resign(void *context) {
    (void) context;
    movement_store_settings();
    movement_request_tick_frequency(1);
}
