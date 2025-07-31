#pragma once

enum TVState {
  TV_OFF,          // 0 Confirmed OFF state  
  TV_ON,           // 1 ON state
  TV_UNKNOWN,      // 2 Confirmed Initial/unknown state
  TV_TRANSITIONING // 3 Currently changing state
};

const char* toString(TVState state);

