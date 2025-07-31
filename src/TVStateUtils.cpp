#include "TVStateUtils.h"

const char* toString(TVState state) {
  switch (state) {
    case TV_OFF:
      return "OFF";
    case TV_ON:
      return "ON";
    case TV_UNKNOWN:
      return "UNKNOWN";
    case TV_TRANSITIONING:
      return "TRANSITIONING";
    default:
      return "INVALID_STATE";
  }
}
