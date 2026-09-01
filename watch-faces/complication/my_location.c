#include "my_location.h"
#include "filesystem.h"

#define MY_LOCATION_FILE "location.u32"

movement_location_t my_location_load(void) {
    movement_location_t location = {0};
    filesystem_read_file(MY_LOCATION_FILE, (char *)&location.reg, sizeof(location));

    if (location.reg == 0) {
        location.bit.latitude = MY_LOCATION_DEFAULT_LATITUDE;
        location.bit.longitude = MY_LOCATION_DEFAULT_LONGITUDE;
        filesystem_write_file(MY_LOCATION_FILE, (char *)&location.reg, sizeof(location));
    }

    return location;
}

void my_location_store(movement_location_t location) {
    movement_location_t saved = {0};
    filesystem_read_file(MY_LOCATION_FILE, (char *)&saved.reg, sizeof(saved));
    if (saved.reg != location.reg) {
        filesystem_write_file(MY_LOCATION_FILE, (char *)&location.reg, sizeof(location));
    }
}
