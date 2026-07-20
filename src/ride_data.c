#include "ride_data.h"
#include <stdlib.h>
#include <string.h>

static RideRecord s_rides[MAX_RIDES_STORED];
static int s_ride_count = 0;

void ride_data_init(void) {
  s_ride_count = 0;
  memset(s_rides, 0, sizeof(s_rides));
}

bool ride_data_add(const RideRecord *ride) {
  if (s_ride_count >= MAX_RIDES_STORED) return false;
  if (!ride) return false;

  memcpy(&s_rides[s_ride_count], ride, sizeof(RideRecord));
  s_ride_count++;
  return true;
}

RideRecord* ride_data_get(int index) {
  if (index < 0 || index >= s_ride_count) return NULL;
  return &s_rides[index];
}

int ride_data_count(void) {
  return s_ride_count;
}

void ride_data_clear(void) {
  s_ride_count = 0;
  memset(s_rides, 0, sizeof(s_rides));
}

void ride_data_export_csv(char *buffer, size_t buffer_size) {
  if (!buffer || buffer_size == 0) return;

  buffer[0] = '\0';
  size_t offset = 0;

  // Header
  snprintf(buffer + offset, buffer_size - offset,
           "Park,Coaster,Duration (s),Max G,Min G,Date\n");
  offset = strlen(buffer);

  for (int i = 0; i < s_ride_count && offset < buffer_size - 100; i++) {
    RideRecord *ride = &s_rides[i];
    struct tm *time_info = localtime(&ride->timestamp);

    char date_buf[32];
    strftime(date_buf, sizeof(date_buf), "%Y-%m-%d %H:%M", time_info);

    snprintf(buffer + offset, buffer_size - offset,
             "%s,%s,%lu,%.2f,%.2f,%s\n",
             ride->park,
             ride->coaster,
             (unsigned long)ride->duration_s,
             ride->max_g / 1000.0f,
             ride->min_g / 1000.0f,
             date_buf);
    offset = strlen(buffer);
  }
}

void ride_data_deinit(void) {
  ride_data_clear();
}
