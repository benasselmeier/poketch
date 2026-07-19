#include "parks.h"
#include <stdlib.h>
#include <string.h>

static Park s_parks[MAX_PARKS];
static int s_park_count = 0;

// Hardcoded parks from coasters.txt
static void parks_init_hardcoded(void) {
  s_park_count = 0;

  // Six Flags St. Louis
  Park *park = &s_parks[s_park_count++];
  strcpy(park->name, "Six Flags St. Louis");
  park->coaster_count = 0;

  strcpy(park->coasters[park->coaster_count].name, "American Thunder");
  park->coasters[park->coaster_count].type = COASTER_TYPE_WOOD;
  park->coaster_count++;

  strcpy(park->coasters[park->coaster_count].name, "Batman the Ride");
  park->coasters[park->coaster_count].type = COASTER_TYPE_STEEL;
  park->coaster_count++;

  strcpy(park->coasters[park->coaster_count].name, "Boomerang");
  park->coasters[park->coaster_count].type = COASTER_TYPE_STEEL;
  park->coaster_count++;

  strcpy(park->coasters[park->coaster_count].name, "Boss");
  park->coasters[park->coaster_count].type = COASTER_TYPE_WOOD;
  park->coaster_count++;

  strcpy(park->coasters[park->coaster_count].name, "Mr. Freeze Reverse Blast");
  park->coasters[park->coaster_count].type = COASTER_TYPE_STEEL;
  park->coaster_count++;

  strcpy(park->coasters[park->coaster_count].name, "Ninja");
  park->coasters[park->coaster_count].type = COASTER_TYPE_STEEL;
  park->coaster_count++;

  strcpy(park->coasters[park->coaster_count].name, "Pandemonium");
  park->coasters[park->coaster_count].type = COASTER_TYPE_STEEL;
  park->coaster_count++;

  strcpy(park->coasters[park->coaster_count].name, "River King Mine Train");
  park->coasters[park->coaster_count].type = COASTER_TYPE_STEEL;
  park->coaster_count++;

  strcpy(park->coasters[park->coaster_count].name, "Rookie Racer");
  park->coasters[park->coaster_count].type = COASTER_TYPE_STEEL;
  park->coaster_count++;

  strcpy(park->coasters[park->coaster_count].name, "Screamin' Eagle");
  park->coasters[park->coaster_count].type = COASTER_TYPE_WOOD;
  park->coaster_count++;

  // St. Louis Union Station
  park = &s_parks[s_park_count++];
  strcpy(park->name, "St. Louis Union Station");
  park->coaster_count = 0;

  strcpy(park->coasters[park->coaster_count].name, "Loco Motion");
  park->coasters[park->coaster_count].type = COASTER_TYPE_STEEL;
  park->coaster_count++;

  // Six Flags Over Georgia
  park = &s_parks[s_park_count++];
  strcpy(park->name, "Six Flags Over Georgia");
  park->coaster_count = 0;

  strcpy(park->coasters[park->coaster_count].name, "Batman the Ride");
  park->coasters[park->coaster_count].type = COASTER_TYPE_STEEL;
  park->coaster_count++;

  strcpy(park->coasters[park->coaster_count].name, "Blue Hawk");
  park->coasters[park->coaster_count].type = COASTER_TYPE_STEEL;
  park->coaster_count++;

  strcpy(park->coasters[park->coaster_count].name, "Dahlonega Mine Train");
  park->coasters[park->coaster_count].type = COASTER_TYPE_STEEL;
  park->coaster_count++;

  strcpy(park->coasters[park->coaster_count].name, "Dare Devil Dive");
  park->coasters[park->coaster_count].type = COASTER_TYPE_STEEL;
  park->coaster_count++;

  strcpy(park->coasters[park->coaster_count].name, "Georgia Scorcher");
  park->coasters[park->coaster_count].type = COASTER_TYPE_STEEL;
  park->coaster_count++;

  strcpy(park->coasters[park->coaster_count].name, "Georgia Goldrusher");
  park->coasters[park->coaster_count].type = COASTER_TYPE_STEEL;
  park->coaster_count++;

  strcpy(park->coasters[park->coaster_count].name, "Goliath");
  park->coasters[park->coaster_count].type = COASTER_TYPE_STEEL;
  park->coaster_count++;

  strcpy(park->coasters[park->coaster_count].name, "Great American Scream Machine");
  park->coasters[park->coaster_count].type = COASTER_TYPE_WOOD;
  park->coaster_count++;

  strcpy(park->coasters[park->coaster_count].name, "Joker Funhouse Coaster");
  park->coasters[park->coaster_count].type = COASTER_TYPE_STEEL;
  park->coaster_count++;

  strcpy(park->coasters[park->coaster_count].name, "Riddler Mindbender");
  park->coasters[park->coaster_count].type = COASTER_TYPE_STEEL;
  park->coaster_count++;

  strcpy(park->coasters[park->coaster_count].name, "Superman - Ultimate Flight");
  park->coasters[park->coaster_count].type = COASTER_TYPE_STEEL;
  park->coaster_count++;

  strcpy(park->coasters[park->coaster_count].name, "Twisted Cyclone");
  park->coasters[park->coaster_count].type = COASTER_TYPE_STEEL;
  park->coaster_count++;
}

void parks_init(void) {
  parks_init_hardcoded();
}

Park* parks_get_all(void) {
  return s_parks;
}

int parks_get_count(void) {
  return s_park_count;
}

Park* parks_get(int index) {
  if (index < 0 || index >= s_park_count) return NULL;
  return &s_parks[index];
}

Coaster* parks_get_coaster(int park_idx, int coaster_idx) {
  Park *park = parks_get(park_idx);
  if (!park || coaster_idx < 0 || coaster_idx >= park->coaster_count) {
    return NULL;
  }
  return &park->coasters[coaster_idx];
}

void parks_deinit(void) {
  // Nothing to free in hardcoded version
}
