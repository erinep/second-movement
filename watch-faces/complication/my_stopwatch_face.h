#ifndef MY_STOPWATCH_FACE_H_
#define MY_STOPWATCH_FACE_H_

#include "movement.h"

typedef enum {
    MY_SW_IDLE = 0,
    MY_SW_RUNNING,
    MY_SW_STOPPED,
} my_stopwatch_status_t;

typedef struct {
    rtc_counter_t start_counter;
    rtc_counter_t stop_counter;
    my_stopwatch_status_t status;
    struct {
        rtc_counter_t seconds;
        rtc_counter_t minutes;
        rtc_counter_t hours;
    } old_display;
} my_stopwatch_state_t;

void my_stopwatch_face_setup(uint8_t watch_face_index, void **context_ptr);
void my_stopwatch_face_activate(void *context);
bool my_stopwatch_face_loop(movement_event_t event, void *context);
void my_stopwatch_face_resign(void *context);

#define my_stopwatch_face ((const watch_face_t){ \
    my_stopwatch_face_setup, \
    my_stopwatch_face_activate, \
    my_stopwatch_face_loop, \
    my_stopwatch_face_resign, \
    NULL, \
})

#endif
