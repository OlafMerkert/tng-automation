#include "esphome.h"
#include "CC1101.h"

CC1101 radio;

class CC1101Component : public PollingComponent, public Sensor {
  public:
    // For now poll every 15 seconds
    CC1101Component() : PollingComponent(15000) { }

    void setup() {
    	radio.init();
 
      radio.writeRegister(CC1101_FSCTRL1, 0x06);
      radio.writeRegister(CC1101_FSCTRL0, 0x00);

      radio.spi_waitMiso();

      radio.writeRegister(CC1101_FREND0, 0x11);


      radio.writeRegister(CC1101_MDMCFG4,     0x87);
      radio.writeRegister(CC1101_MDMCFG3,     0x32);
      radio.writeRegister(CC1101_MDMCFG2,     0x30);
      radio.writeRegister(CC1101_MDMCFG1, 0x22);
      radio.writeRegister(CC1101_MDMCFG0, 0xF8);

      radio.writeRegister(CC1101_AGCCTRL0, 0x91);
      radio.writeRegister(CC1101_AGCCTRL1, 0x00);
      radio.writeRegister(CC1101_AGCCTRL2, 0x07);

      radio.writeRegister(CC1101_MCSM2, 0x07);
      radio.writeRegister(CC1101_MCSM1, 0x00);
      radio.writeRegister(CC1101_MCSM0, 0x18);

      radio.writeRegister(CC1101_PKTCTRL0, 0x32); // asynchronous serial mode
      radio.writeRegister(CC1101_IOCFG0, 0x0d);   // GD0 output
    }

    void update() override {
    	Serial.println("Called update");
    	Serial.println(radio.readRegister(CC1101_TEST1, CC1101_CONFIG_REGISTER));
        //fanspeed->publish_state(State.c_str());
        //fantimer->publish_state(String(Timer).c_str());
    }

};
