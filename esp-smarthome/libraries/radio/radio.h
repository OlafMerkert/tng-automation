#ifndef TNG_AUTOMATION_RADIO_H
#define TNG_AUTOMATION_RADIO_H

#include "esphome.h"
#include <ELECHOUSE_CC1101_SRC_DRV.h>

#define RFCC1101_RECEIVER_PIN 4 //D2
#define RFCC1101_EMITTER_PIN 4

#define MAXTIME 20000
#define MINTIME 100
#define BUFFERSIZE 500


class Radio{
public:
    void setup();
    void doWork();
    void sendTimings(String timings);
    bool isBufferReady();
    uint16_t * getBuffer();
    uint16_t getBufferLength();
    void resetBuffer();
    ICACHE_RAM_ATTR void handlePinChange();
    void enterTxMode();
    void enterRxMode();
    bool sendReady();

private:
    int gdo0 = RFCC1101_RECEIVER_PIN;
    volatile long time = -1L;
    uint16_t packetBuffer[BUFFERSIZE];
    volatile uint16_t packetBufferPos=0;
    volatile bool packetBufferReady=false;
    bool init();
    void setReceiveFreq();
};

#endif
