#ifndef MY_COUNTDOWN_FACE_H_
#define MY_COUNTDOWN_FACE_H_

#include "movement.h"

typedef enum {
    my_cd_stopped,
    my_cd_running,
} my_countdown_mode_t;

typedef struct {
    uint32_t target_ts;
    uint32_t now_ts;
    uint32_t remaining_seconds;
    uint8_t preset_index;
    uint8_t tap_detection_ticks;
    bool tap_cycle_started;
    my_countdown_mode_t mode;
    uint8_t watch_face_index;
} my_countdown_state_t;

void my_countdown_face_setup(uint8_t watch_face_index, void **context_ptr);
void my_countdown_face_activate(void *context);
bool my_countdown_face_loop(movement_event_t event, void *context);
void my_countdown_face_resign(void *context);

#define my_countdown_face ((const watch_face_t){ \
    my_countdown_face_setup, \
    my_countdown_face_activate, \
    my_countdown_face_loop, \
    my_countdown_face_resign, \
    NULL, \
})

#endif
