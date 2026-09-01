#include "set_location_face.h"
#include "my_location.h"
#include "sunrise_sunset_face.h"

void set_location_face_setup(uint8_t watch_face_index, void **context_ptr) {
    sunrise_sunset_face_setup(watch_face_index, context_ptr);
}

void set_location_face_activate(void *context) {
    // Ensure a fresh installation has the Montreal default before the original
    // editor loads location.u32 into its working fields.
    my_location_load();
    sunrise_sunset_face_activate(context);
    sunrise_sunset_state_t *state = (sunrise_sunset_state_t *)context;
    state->page = 1;
    state->active_digit = 0;
    movement_request_tick_frequency(4);
}

bool set_location_face_loop(movement_event_t event, void *context) {
    // The original face draws its settings UI on ticks, whereas a normal
    // activation would draw the rise/set result. Substitute a tick so this
    // dedicated face opens directly on the identical latitude screen.
    if (event.event_type == EVENT_ACTIVATE) event.event_type = EVENT_TICK;

    switch (event.event_type) {
        case EVENT_MODE_BUTTON_UP:
        case EVENT_ALARM_LONG_PRESS:
            sunrise_sunset_face_resign(context);
            movement_move_to_next_face();
            return true;
        case EVENT_TIMEOUT:
            sunrise_sunset_face_resign(context);
            movement_move_to_face(0);
            return true;
        default:
            return sunrise_sunset_face_loop(event, context);
    }
}

void set_location_face_resign(void *context) {
    sunrise_sunset_face_resign(context);
    movement_request_tick_frequency(1);
}
