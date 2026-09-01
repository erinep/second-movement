#ifndef SET_LOCATION_FACE_H_
#define SET_LOCATION_FACE_H_

#include "movement.h"

void set_location_face_setup(uint8_t watch_face_index, void **context_ptr);
void set_location_face_activate(void *context);
bool set_location_face_loop(movement_event_t event, void *context);
void set_location_face_resign(void *context);

#define set_location_face ((const watch_face_t){ \
    set_location_face_setup, \
    set_location_face_activate, \
    set_location_face_loop, \
    set_location_face_resign, \
    NULL, \
})

#endif
