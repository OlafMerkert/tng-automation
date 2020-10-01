#include "esphome.h"
#include "CC1101.h"

#define RFCC1101_RECEIVER_PIN 4 // D2
#define RFCC1101_EMITTER_PIN 4
#ifndef ESP8266
# define ESP8266
#endif

enum CFREQ
{
    CFREQ_868 = 0,
    CFREQ_915,
    CFREQ_433,
    CFREQ_918,
    CFREQ_LAST
};
enum RFSTATE
{
    RFSTATE_IDLE = 0,
    RFSTATE_RX,
    RFSTATE_TX
};


class CC1101Radio : public CC1101{
public:
    CC1101Radio() : CC1101(){
        this->ssPin = SS;
        this->gd0pin = RFCC1101_RECEIVER_PIN;
    }
    CC1101 radio;

    void setup(){
        Serial.println(this->initialize());
    }

private:
    uint8_t ssPin;
    uint8_t gd0pin;
    uint8_t workMode;
    uint8_t carrierFreq;
    uint8_t rfState;

    bool initialize(){
        this->init(CFREQ_433, 0);
        this->setRxState();

        this->writeRegister(CC1101_FSCTRL1, 0x06);
        this->writeRegister(CC1101_FSCTRL0, 0x00);

        this->setReceiveFreq();

        this->writeRegister(CC1101_FREND0, 0x11);

        this->writeRegister(CC1101_MDMCFG4,     0x87);
        this->writeRegister(CC1101_MDMCFG3,     0x32);
        this->writeRegister(CC1101_MDMCFG2,     0x30);
        this->writeRegister(CC1101_MDMCFG1, 0x22);
        this->writeRegister(CC1101_MDMCFG0, 0xF8);

        this->writeRegister(CC1101_AGCCTRL0, 0x91);
        this->writeRegister(CC1101_AGCCTRL1, 0x00);
        this->writeRegister(CC1101_AGCCTRL2, 0x07);

        this->writeRegister(CC1101_MCSM2, 0x07);
        this->writeRegister(CC1101_MCSM1, 0x00);
        this->writeRegister(CC1101_MCSM0, 0x18);

        this->writeRegister(CC1101_PKTCTRL0, 0x32); // asynchronous serial mode
        this->writeRegister(CC1101_IOCFG0, 0x0d);   // GD0 output

        Serial.println(this->readRegister(CC1101_TEST1, CC1101_CONFIG_REGISTER));
        return this->readRegister(CC1101_TEST1, CC1101_CONFIG_REGISTER) == 0x35;
    }

    void init(uint8_t freq, uint8_t mode){
        this->carrierFreq = freq;
        this->workMode = mode;

        SPI.begin();
        SPI.setFrequency(100000);
        pinMode(this->gd0pin, INPUT);

        this->reset();
        this->setTxPowerAmp(CC1101_PA_LowPower);
    }

    void setTxPowerAmp(uint8_t paLevel){
        this->writeRegister(CC1101_PATABLE, paLevel);
    }

    void setRxState(){
        this->cmdStrobe(CC1101_SRX);
        this->rfState = RFSTATE_RX;
    }

    void cmdStrobe(byte cmd){
        this->cc1101_Select();
        this->waitMiso();
        SPI.transfer(cmd);
        this->cc1101_Deselect();
    }

    void cc1101_Select(){
        digitalWrite(this->ssPin, LOW);
    }

    void cc1101_Deselect(){
        digitalWrite(this->ssPin, HIGH);
    }

    void waitMiso(){
        while (digitalRead(MISO)>0);
    }

    void setReceiveFreq() {
        this->writeRegister(CC1101_FREQ2,       0x10);
        this->writeRegister(CC1101_FREQ1,       0xB0);
        this->writeRegister(CC1101_FREQ0,       0x71);
    }

};

class CC1101Component : public PollingComponent, public Sensor {
public:
    // For now poll every 15 seconds
    CC1101Component() : PollingComponent(15000) {    }
    CC1101Radio radio;
    void setup() {
    }

    void update() override {
        Serial.println("Called update");
        radio.setup();
        //fanspeed->publish_state(State.c_str());
        //fantimer->publish_state(String(Timer).c_str());
    }

};