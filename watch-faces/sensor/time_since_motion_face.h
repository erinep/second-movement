#pragma once

#include "movement.h"

/* Rest-analysis tuning. These affect reports, never the raw history. */
#define TIME_SINCE_MOTION_REST_WINDOW_MINUTES 20
#define TIME_SINCE_MOTION_REST_PERCENT 80
#define TIME_SINCE_MOTION_REST_GAP_MINUTES 10
#define TIME_SINCE_MOTION_MIN_REST_MINUTES 60
#define TIME_SINCE_MOTION_WAKE_MINUTES 5

void time_since_motion_face_setup(uint8_t watch_face_index, void **context_ptr);
void time_since_motion_face_activate(void *context);
bool time_since_motion_face_loop(movement_event_t event, void *context);
void time_since_motion_face_resign(void *context);
movement_watch_face_advisory_t time_since_motion_face_advise(void *context);

#define time_since_motion_face ((const watch_face_t){ \
    time_since_motion_face_setup, \
    time_since_motion_face_activate, \
    time_since_motion_face_loop, \
    time_since_motion_face_resign, \
    time_since_motion_face_advise, \
})
