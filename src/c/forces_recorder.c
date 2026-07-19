#include "forces_recorder.h"
#include <stdlib.h>
#include <stdint.h>

#define ACCEL_BUFFER_SIZE 32

static AccelVector s_accel_buffer[ACCEL_BUFFER_SIZE];
static int s_buffer_idx = 0;
static bool s_recording = false;
static uint32_t s_start_time_ms = 0;
static uint16_t s_max_g = 0;
static uint16_t s_min_g = 1000;  // Start high, will drop

// Accelerometer data handler
static void accel_data_handler(AccelData *data, uint32_t num_samples) {
  if (!s_recording) return;

  for (uint32_t i = 0; i < num_samples; i++) {
    // Read raw accel data
    int16_t x = data[i].x;
    int16_t y = data[i].y;
    int16_t z = data[i].z;

    // Calculate magnitude: sqrt(x^2 + y^2 + z^2)
    // Pebble returns values in milli-g
    // Use integer-only math to avoid needing libm
    int32_t x_sq = (int32_t)x * x;
    int32_t y_sq = (int32_t)y * y;
    int32_t z_sq = (int32_t)z * z;

    uint32_t magnitude_sq = x_sq + y_sq + z_sq;
    // Fast integer sqrt approximation (Babylonian method, 1 iteration)
    uint16_t magnitude = magnitude_sq / 1000;
    if (magnitude == 0) magnitude = 1;
    uint16_t guess = (magnitude + magnitude_sq / magnitude) / 2;
    magnitude = guess;

    // Track min/max
    if (magnitude > s_max_g) s_max_g = magnitude;
    if (magnitude < s_min_g) s_min_g = magnitude;

    // Store in buffer (circular)
    s_accel_buffer[s_buffer_idx].x = x;
    s_accel_buffer[s_buffer_idx].y = y;
    s_accel_buffer[s_buffer_idx].z = z;
    s_buffer_idx = (s_buffer_idx + 1) % ACCEL_BUFFER_SIZE;
  }
}

void forces_recorder_init(void) {
  s_recording = false;
  s_buffer_idx = 0;
  s_max_g = 0;
  s_min_g = 1000;
  s_start_time_ms = 0;

  // Subscribe to accelerometer data at 10 Hz
  accel_data_service_subscribe(10, accel_data_handler);
}

void forces_recorder_start(void) {
  s_recording = true;
  s_start_time_ms = 0;  // Will be set on first reading
  s_max_g = 0;
  s_min_g = 1000;
  s_buffer_idx = 0;

  time_t sec;
  uint16_t ms;
  time_ms(&sec, &ms);
  s_start_time_ms = (uint32_t)sec * 1000 + ms;
}

void forces_recorder_stop(void) {
  s_recording = false;
}

ForceReading forces_recorder_get_current(void) {
  ForceReading reading = {0, 0, s_max_g, s_min_g};

  if (s_recording && s_start_time_ms > 0) {
    time_t sec;
    uint16_t ms;
    time_ms(&sec, &ms);
    uint32_t now_ms = (uint32_t)sec * 1000 + ms;

    reading.elapsed_ms = now_ms - s_start_time_ms;

    // Calculate current magnitude from last sample
    if (s_buffer_idx > 0) {
      AccelVector *last = &s_accel_buffer[s_buffer_idx - 1];
      int32_t x_sq = (int32_t)last->x * last->x;
      int32_t y_sq = (int32_t)last->y * last->y;
      int32_t z_sq = (int32_t)last->z * last->z;
      uint32_t mag_sq = x_sq + y_sq + z_sq;
      // Fast integer sqrt
      uint16_t guess = mag_sq / 1000;
      if (guess == 0) guess = 1;
      uint16_t refined = (guess + mag_sq / guess) / 2;
      reading.magnitude = refined;
    }
  }

  return reading;
}

uint32_t forces_recorder_get_elapsed_s(void) {
  if (!s_recording || s_start_time_ms == 0) return 0;

  time_t sec;
  uint16_t ms;
  time_ms(&sec, &ms);
  uint32_t now_ms = (uint32_t)sec * 1000 + ms;

  return (now_ms - s_start_time_ms) / 1000;
}

uint16_t forces_recorder_get_max_g(void) {
  return s_max_g;
}

uint16_t forces_recorder_get_min_g(void) {
  return s_min_g == 1000 ? 0 : s_min_g;
}

bool forces_recorder_is_recording(void) {
  return s_recording;
}

void forces_recorder_deinit(void) {
  accel_data_service_unsubscribe();
  s_recording = false;
}
