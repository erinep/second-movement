#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "my_databank_face.h"
#include "watch.h"

typedef struct {
    const char *name;
    const char *number;
} my_databank_entry_t;

/* Build-time Databank entries. Keep names to five characters; numbers can be
 * any length and may include displayable punctuation.
 */
static const my_databank_entry_t my_databank_entries[] = {
#include "private/my_databank_entries.inc"
};

#define MY_DATABANK_SCROLL_GAP 3
#define MY_DATABANK_ENTRY_COUNT (sizeof(my_databank_entries) / sizeof(my_databank_entries[0]))

typedef struct {
    uint8_t entry_index;
    uint8_t scroll_offset;
} my_databank_state_t;

static void my_databank_draw(const my_databank_state_t *state) {
    const my_databank_entry_t *entry = &my_databank_entries[state->entry_index];
    char title[6];
    char fallback[3];
    char window[7];
    size_t number_length = strlen(entry->number);
    size_t cycle_length = number_length + MY_DATABANK_SCROLL_GAP;

    snprintf(title, sizeof(title), "%-5.5s", entry->name);
    snprintf(fallback, sizeof(fallback), "%-2.2s", entry->name);
    watch_display_text_with_fallback(WATCH_POSITION_TOP, title, fallback);

    for (uint8_t i = 0; i < 6; i++) {
        size_t position = (state->scroll_offset + i) % cycle_length;
        window[i] = position < number_length ? entry->number[position] : ' ';
    }
    window[6] = '\0';
    watch_display_text(WATCH_POSITION_BOTTOM, window);
}

void my_databank_face_setup(uint8_t watch_face_index, void **context_ptr) {
    (void)watch_face_index;
    if (*context_ptr == NULL) *context_ptr = calloc(1, sizeof(my_databank_state_t));
}

void my_databank_face_activate(void *context) {
    my_databank_state_t *state = context;
    state->scroll_offset = 0;
    movement_request_tick_frequency(2);
}

bool my_databank_face_loop(movement_event_t event, void *context) {
    my_databank_state_t *state = context;
    size_t cycle_length = strlen(my_databank_entries[state->entry_index].number) + MY_DATABANK_SCROLL_GAP;

    switch (event.event_type) {
        case EVENT_ACTIVATE:
            my_databank_draw(state);
            break;
        case EVENT_TICK:
            state->scroll_offset = (state->scroll_offset + 1) % cycle_length;
            my_databank_draw(state);
            break;
        case EVENT_ALARM_BUTTON_UP:
            state->entry_index = (state->entry_index + 1) % MY_DATABANK_ENTRY_COUNT;
            state->scroll_offset = 0;
            my_databank_draw(state);
            break;
        case EVENT_ALARM_LONG_PRESS:
            state->entry_index = (state->entry_index + MY_DATABANK_ENTRY_COUNT - 1) % MY_DATABANK_ENTRY_COUNT;
            state->scroll_offset = 0;
            my_databank_draw(state);
            break;
        case EVENT_MODE_BUTTON_UP:
            movement_close_memory_menu();
            break;
        case EVENT_LOW_ENERGY_UPDATE:
            my_databank_draw(state);
            break;
        case EVENT_TIMEOUT:
            movement_move_to_face(0);
            break;
        default:
            return movement_default_loop_handler(event);
    }

    return true;
}

void my_databank_face_resign(void *context) {
    (void)context;
}
