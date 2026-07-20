#pragma once

#include <pebble.h>

#define MAX_PARKS 10
#define MAX_COASTERS_PER_PARK 25
#define MAX_PARK_NAME_LEN 48
#define MAX_COASTER_NAME_LEN 48

typedef enum {
  COASTER_TYPE_WOOD,
  COASTER_TYPE_STEEL,
} CoasterType;

typedef struct {
  char name[MAX_COASTER_NAME_LEN];
  CoasterType type;
} Coaster;

typedef struct {
  char name[MAX_PARK_NAME_LEN];
  Coaster coasters[MAX_COASTERS_PER_PARK];
  int coaster_count;
} Park;

// Initialize park/coaster data from coasters.txt
void parks_init(void);

// Get all parks
Park* parks_get_all(void);

// Get park count
int parks_get_count(void);

// Get park by index
Park* parks_get(int index);

// Get coaster by park and coaster index
Coaster* parks_get_coaster(int park_idx, int coaster_idx);

// Cleanup
void parks_deinit(void);
