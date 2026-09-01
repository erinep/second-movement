#ifndef MY_LOCATION_H_
#define MY_LOCATION_H_

#include "movement.h"

#define MY_LOCATION_DEFAULT_LATITUDE 4550
#define MY_LOCATION_DEFAULT_LONGITUDE (-7357)

movement_location_t my_location_load(void);
void my_location_store(movement_location_t location);

#endif
