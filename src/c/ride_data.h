#pragma once

#include <pebble.h>
#include <time.h>

#define MAX_RIDES_STORED 10
#define MAX_PARK_NAME_LEN 48
#define MAX_COASTER_NAME_LEN 48

typedef struct {
  char park[MAX_PARK_NAME_LEN];
  char coaster[MAX_COASTER_NAME_LEN];
  uint32_t duration_s;
  uint16_t max_g;      // milli-g
  uint16_t min_g;      // milli-g
  time_t timestamp;
} RideRecord;

// Initialize ride storage
void ride_data_init(void);

// Add a new ride record
bool ride_data_add(const RideRecord *ride);

// Get ride by index
RideRecord* ride_data_get(int index);

// Get total recorded rides
int ride_data_count(void);

// Clear all rides
void ride_data_clear(void);

// Export rides as CSV (to buffer)
void ride_data_export_csv(char *buffer, size_t buffer_size);

// Cleanup
void ride_data_deinit(void);
