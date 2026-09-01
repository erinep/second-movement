#pragma once

#include "movement.h"

void my_databank_face_setup(uint8_t watch_face_index, void **context_ptr);
void my_databank_face_activate(void *context);
bool my_databank_face_loop(movement_event_t event, void *context);
void my_databank_face_resign(void *context);

#define my_databank_face ((const watch_face_t){ \
    my_databank_face_setup, \
    my_databank_face_activate, \
    my_databank_face_loop, \
    my_databank_face_resign, \
    NULL, \
})
