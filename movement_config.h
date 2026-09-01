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

#ifndef MOVEMENT_CONFIG_H_
#define MOVEMENT_CONFIG_H_

#include "movement_faces.h"

const watch_face_t movement_clock_faces[] = {
    clock_face,
    my_sunrise_face,
    title_face, // transient; excluded from short-Mode rotation
};

const watch_face_t movement_alarm_faces[] = {
    my_countdown_face,
    my_stopwatch_face,
    time_since_motion_face,
    tomato_face,
    alarm_face,
    title_face, // transient; excluded from short-Mode rotation
};

const watch_face_t movement_settings_faces[] = {
    set_timezone_face,
    set_date_face,
    set_time_face,
    set_location_face,
    settings_face,
    voltage_face,
    memory_launcher_face,
    title_face, // transient; excluded from short-Mode rotation
};

/* Nested under Config; these faces are excluded from top-level rotation. */
const watch_face_t movement_memory_faces[] = {
    my_databank_face,
    title_face,
};

#define MOVEMENT_CLOCK_FACE_COUNT (sizeof(movement_clock_faces) / sizeof(watch_face_t))
#define MOVEMENT_ALARM_FACE_COUNT (sizeof(movement_alarm_faces) / sizeof(watch_face_t))
#define MOVEMENT_SETTINGS_FACE_COUNT (sizeof(movement_settings_faces) / sizeof(watch_face_t))
#define MOVEMENT_MEMORY_FACE_COUNT (sizeof(movement_memory_faces) / sizeof(watch_face_t))

#define MOVEMENT_ALARM_FACE_INDEX MOVEMENT_CLOCK_FACE_COUNT
#define MOVEMENT_SETTINGS_FACE_INDEX (MOVEMENT_ALARM_FACE_INDEX + MOVEMENT_ALARM_FACE_COUNT)
#define MOVEMENT_MEMORY_FACE_INDEX (MOVEMENT_SETTINGS_FACE_INDEX + MOVEMENT_SETTINGS_FACE_COUNT)
#define MOVEMENT_NUM_FACES (MOVEMENT_CLOCK_FACE_COUNT + MOVEMENT_ALARM_FACE_COUNT + MOVEMENT_MEMORY_FACE_COUNT + MOVEMENT_SETTINGS_FACE_COUNT)

#define MOVEMENT_CLOCK_TITLE_FACE_INDEX (MOVEMENT_ALARM_FACE_INDEX - 1)
#define MOVEMENT_ALARM_TITLE_FACE_INDEX (MOVEMENT_SETTINGS_FACE_INDEX - 1)
#define MOVEMENT_SETTINGS_TITLE_FACE_INDEX (MOVEMENT_MEMORY_FACE_INDEX - 1)
#define MOVEMENT_MEMORY_LAUNCHER_FACE_INDEX (MOVEMENT_SETTINGS_TITLE_FACE_INDEX - 1)
#define MOVEMENT_MEMORY_TITLE_FACE_INDEX (MOVEMENT_NUM_FACES - 1)

/* These indexes divide the face list into three top-level menus. Short presses of Mode
 * cycle within the current menu; long presses advance to the first face of the
 * next menu: clock/info -> tools -> settings -> clock/info. Memory is a nested
 * menu opened from memory_launcher_face in Settings.
 */

/* Custom hourly chime tune. Check movement_custom_signal_tunes.h for options. */
#define SIGNAL_TUNE_DEFAULT

/* Determines the intensity of the led colors
 * Set a hex value 0-15 with 0x0 being off and 0xF being max intensity
 */
#define MOVEMENT_DEFAULT_RED_COLOR 0x0
#define MOVEMENT_DEFAULT_GREEN_COLOR 0xF
#define MOVEMENT_DEFAULT_BLUE_COLOR 0x0

/* Set to true for 24h mode or false for 12h mode */
#define MOVEMENT_DEFAULT_24H_MODE false

/* Enable or disable the sound on mode button press */
#define MOVEMENT_DEFAULT_BUTTON_SOUND true

#define MOVEMENT_DEFAULT_BUTTON_VOLUME WATCH_BUZZER_VOLUME_SOFT
#define MOVEMENT_DEFAULT_SIGNAL_VOLUME WATCH_BUZZER_VOLUME_LOUD
#define MOVEMENT_DEFAULT_ALARM_VOLUME WATCH_BUZZER_VOLUME_LOUD

/* Set the timeout before switching back to the main watch face
 * Valid values are:
 * 0: 60 seconds
 * 1: 2 minutes
 * 2: 5 minutes
 * 3: 30 minutes
 */
#define MOVEMENT_DEFAULT_TIMEOUT_INTERVAL 0

/* Set the timeout before switching to low energy mode
 * Valid values are:
 * 0: Never
 * 1: 10 minutes
 * 2: 1 hour
 * 3: 2 hours
 * 4: 6 hours
 * 5: 12 hours
 * 6: 1 day
 * 7: 7 days
 */
#define MOVEMENT_DEFAULT_LOW_ENERGY_INTERVAL 2

/* Enable short development-only timeouts. Keep this disabled for daily use.
 * Debug overrides do not change saved Settings values or on-screen labels.
 */
#define MOVEMENT_DEBUG_MODE 0

#if MOVEMENT_DEBUG_MODE
#define MOVEMENT_LOW_ENERGY_TEST_TIMEOUT_SECONDS 60
#endif

/* Set the led duration
 * Valid values are:
 * 0: No LED
 * 1: 1 second
 * 2: 3 seconds
 * 3: 5 seconds
 */
#define MOVEMENT_DEFAULT_LED_DURATION 1

/* Menu title duration in half-second units: 1=.5s, 2=1s, 3=1.5s, 4=2s. */
#define MOVEMENT_DEFAULT_TITLE_LENGTH 1

/* LIS2DW motion threshold, in 0.03125 g steps (32 = 1.0 g). */
#define MOVEMENT_DEFAULT_MOTION_THRESHOLD 32

/* Optionally debounce button presses (disable by default).
 * A value of 4 is a good starting point if you have issues
 * with multiple button presses firing.
*/
#define MOVEMENT_DEBOUNCE_TICKS 0

#endif // MOVEMENT_CONFIG_H_
