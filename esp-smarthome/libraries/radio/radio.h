/*
  radio.cpp - CC1101 as 433 MHz Transceiver
  Copyright (c) 2020 Florian Gather.
  Copyright (c) 2020 Georg Kreuzmayr
    Author: Florian, <https://www.tngtech.com/>
    Version: October, 2020
  This library is designed to use CC1101 with ESP8266 on esphome
  For the details, please refer to the datasheet of CC1100/CC1101.
----------------------------------------------------------------------------------------------------------------
SPDX-License-Identifier: Apache-2.0
----------------------------------------------------------------------------------------------------------------
*/

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
