#include "memory_launcher_face.h"
#include "watch.h"

static void memory_launcher_draw(void) {
    watch_display_text_with_fallback(WATCH_POSITION_TOP, "MEMRY", "ME");
    watch_display_text_with_fallback(WATCH_POSITION_BOTTOM, "OPEN  ", " OPEN ");
}

void memory_launcher_face_setup(uint8_t watch_face_index, void **context_ptr) {
    (void)watch_face_index;
    (void)context_ptr;
}

void memory_launcher_face_activate(void *context) {
    (void)context;
}

bool memory_launcher_face_loop(movement_event_t event, void *context) {
    (void)context;

    switch (event.event_type) {
        case EVENT_ACTIVATE:
            memory_launcher_draw();
            break;
        case EVENT_ALARM_BUTTON_UP:
            movement_open_memory_menu();
            break;
        case EVENT_TIMEOUT:
            movement_move_to_face(0);
            break;
        default:
            return movement_default_loop_handler(event);
    }

    return true;
}

void memory_launcher_face_resign(void *context) {
    (void)context;
}
