#include <stdlib.h>
#include "title_face.h"
#include "watch.h"

typedef struct {
    uint8_t elapsed_ticks;
} title_face_state_t;

static const char *transition_title = "CLOCK";
static const char *transition_fallback = "CL";
static uint8_t transition_destination;

void title_face_set_transition(const char *title, const char *fallback, uint8_t destination) {
    transition_title = title;
    transition_fallback = fallback;
    transition_destination = destination;
}

void title_face_setup(uint8_t watch_face_index, void **context_ptr) {
    (void)watch_face_index;
    if (*context_ptr == NULL) *context_ptr = calloc(1, sizeof(title_face_state_t));
}

void title_face_activate(void *context) {
    title_face_state_t *state = context;
    state->elapsed_ticks = 0;
    movement_request_tick_frequency(8);
}

bool title_face_loop(movement_event_t event, void *context) {
    title_face_state_t *state = context;

    switch (event.event_type) {
        case EVENT_ACTIVATE:
            watch_display_text_with_fallback(WATCH_POSITION_TOP, transition_title, transition_fallback);
            watch_display_text(WATCH_POSITION_BOTTOM, "      ");
            break;
        case EVENT_TICK:
            if (++state->elapsed_ticks >= movement_get_title_length() * 4) {
                movement_move_to_face(transition_destination);
            }
            break;
        case EVENT_MODE_BUTTON_UP:
            movement_move_to_face(transition_destination);
            break;
        default:
            return movement_default_loop_handler(event);
    }

    return true;
}

void title_face_resign(void *context) {
    (void)context;
}
