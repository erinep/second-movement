#pragma once

#include "movement.h"

void title_face_set_transition(const char *title, const char *fallback, uint8_t destination);
void title_face_setup(uint8_t watch_face_index, void **context_ptr);
void title_face_activate(void *context);
bool title_face_loop(movement_event_t event, void *context);
void title_face_resign(void *context);

#define title_face ((const watch_face_t){ \
    title_face_setup, \
    title_face_activate, \
    title_face_loop, \
    title_face_resign, \
    NULL, \
})
