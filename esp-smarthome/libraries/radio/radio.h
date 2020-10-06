#ifndef TNG_AUTOMATION_RADIO_H
#define TNG_AUTOMATION_RADIO_H

#include "esphome.h"

class Radio{
public:
    void sendTimings(String timings);
    String getTimings();
};

#endif
