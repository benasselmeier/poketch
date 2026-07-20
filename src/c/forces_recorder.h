#pragma once

#include <pebble.h>
#include <stdint.h>

// G-force data is stored in milli-g units (1000 = 1.0g)
// Display as hundredths of g (e.g., 150 = 1.50g)

typedef struct {
  uint16_t x;  // milli-g
  uint16_t y;  // milli-g
  uint16_t z;  // milli-g
} AccelVector;

typedef struct {
  uint32_t elapsed_ms;
  uint16_t magnitude;  // milli-g
  uint16_t max_g;      // milli-g, hundredths display
  uint16_t min_g;      // milli-g, hundredths display
} ForceReading;

// Initialize accelerometer
void forces_recorder_init(void);

// Start recording
void forces_recorder_start(void);

// Stop recording
void forces_recorder_stop(void);

// Get current force reading (call during recording)
ForceReading forces_recorder_get_current(void);

// Get elapsed time in seconds
uint32_t forces_recorder_get_elapsed_s(void);

// Get max G encountered
uint16_t forces_recorder_get_max_g(void);

// Get min G encountered
uint16_t forces_recorder_get_min_g(void);

// Check if currently recording
bool forces_recorder_is_recording(void);

// Cleanup
void forces_recorder_deinit(void);
