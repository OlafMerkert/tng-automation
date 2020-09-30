#include "esphome.h"


class MyCustomSensor : public PollingComponent, public Sensor {
 public:
  // constructor
  MyCustomSensor() : PollingComponent(15000) {}
  
  

  void setup() override {
  	Serial.begin(115200);
  }

  void update() override {
  }
  
};
