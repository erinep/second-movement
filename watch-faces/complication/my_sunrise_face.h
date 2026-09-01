#ifndef MY_SUNRISE_FACE_H_
#define MY_SUNRISE_FACE_H_

#include "movement.h"

typedef struct {
    bool show_sunset;
    watch_date_time_t expires;
} my_sunrise_state_t;

void my_sunrise_face_setup(uint8_t watch_face_index, void **context_ptr);
void my_sunrise_face_activate(void *context);
bool my_sunrise_face_loop(movement_event_t event, void *context);
void my_sunrise_face_resign(void *context);

#define my_sunrise_face ((const watch_face_t){ \
    my_sunrise_face_setup, \
    my_sunrise_face_activate, \
    my_sunrise_face_loop, \
    my_sunrise_face_resign, \
    NULL, \
})

#endif
