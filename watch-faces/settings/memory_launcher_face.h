#pragma once

#include "movement.h"

void memory_launcher_face_setup(uint8_t watch_face_index, void **context_ptr);
void memory_launcher_face_activate(void *context);
bool memory_launcher_face_loop(movement_event_t event, void *context);
void memory_launcher_face_resign(void *context);

#define memory_launcher_face ((const watch_face_t){ \
    memory_launcher_face_setup, \
    memory_launcher_face_activate, \
    memory_launcher_face_loop, \
    memory_launcher_face_resign, \
    NULL, \
})
