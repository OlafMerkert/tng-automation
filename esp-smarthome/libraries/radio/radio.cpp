#include "radio.h"

long unsigned lastSend = 0;
int sendDelay = 5000;

void Radio::setup() {
    while(!init()){
        Serial.println("Cannot initialize receiver!");
        delay(1000);
    }
}

void Radio::doWork() {
    if (time != -1L) {
        long newTime = micros();
        long diff = newTime - time;
        if (diff > MAXTIME) {
            time = -1L;
            if(getBufferLength()>20){
                packetBufferReady=true;
            } else {
                resetBuffer();
            }
        }
    }
}

bool Radio::isBufferReady(){
    return Radio::packetBufferReady;
}

uint16_t * Radio::getBuffer(){
    return Radio::packetBuffer;
}

uint16_t Radio::getBufferLength(){
    return Radio::packetBufferPos+1;
}

void Radio::resetBuffer(){
    Radio::packetBufferPos=0;
    Radio::packetBufferReady=false;
}

bool Radio::init() {
    ELECHOUSE_cc1101.Init();               // must be set to initialize the cc1101!
    ELECHOUSE_cc1101.setGDO(gdo0,0);      // set lib internal gdo pins (gdo0,gdo2). Gdo2 not use for this example.
    ELECHOUSE_cc1101.setCCMode(1);       // set config for internal transmission mode.
    ELECHOUSE_cc1101.setModulation(0);  // set modulation mode. 0 = 2-FSK, 1 = GFSK, 2 = ASK/OOK, 3 = 4-FSK, 4 = MSK.
    ELECHOUSE_cc1101.setMHZ(433.92);   // Here you can set your basic frequency. The lib calculates the frequency automatically (default = 433.92).The cc1101 can: 300-348 MHZ, 387-464MHZ and 779-928MHZ. Read More info from datasheet.
    ELECHOUSE_cc1101.setSyncMode(2);  // Combined sync-word qualifier mode. 0 = No preamble/sync. 1 = 16 sync word bits detected. 2 = 16/16 sync word bits detected. 3 = 30/32 sync word bits detected. 4 = No preamble/sync, carrier-sense above threshold. 5 = 15/16 + carrier-sense above threshold. 6 = 16/16 + carrier-sense above threshold. 7 = 30/32 + carrier-sense above threshold.
    ELECHOUSE_cc1101.setCrc(1);      // 1 = CRC calculation in TX and CRC check in RX enabled. 0 = CRC disabled for TX and RX.

    ELECHOUSE_cc1101.SetRx();

    ELECHOUSE_cc1101.SpiWriteReg(CC1101_FSCTRL1, 0x06);
    ELECHOUSE_cc1101.SpiWriteReg(CC1101_FSCTRL0, 0x00);

    setReceiveFreq();

    ELECHOUSE_cc1101.SpiWriteReg(CC1101_FREND0, 0x11);


    ELECHOUSE_cc1101.SpiWriteReg(CC1101_MDMCFG4, 0x87);
    ELECHOUSE_cc1101.SpiWriteReg(CC1101_MDMCFG3, 0x32);
    ELECHOUSE_cc1101.SpiWriteReg(CC1101_MDMCFG2, 0x30);
    ELECHOUSE_cc1101.SpiWriteReg(CC1101_MDMCFG1, 0x22);
    ELECHOUSE_cc1101.SpiWriteReg(CC1101_MDMCFG0, 0xF8);

    ELECHOUSE_cc1101.SpiWriteReg(CC1101_AGCCTRL0, 0x91);
    ELECHOUSE_cc1101.SpiWriteReg(CC1101_AGCCTRL1, 0x00);
    ELECHOUSE_cc1101.SpiWriteReg(CC1101_AGCCTRL2, 0x07);

    ELECHOUSE_cc1101.SpiWriteReg(CC1101_MCSM2, 0x07);
    ELECHOUSE_cc1101.SpiWriteReg(CC1101_MCSM1, 0x00);
    ELECHOUSE_cc1101.SpiWriteReg(CC1101_MCSM0, 0x18);

    ELECHOUSE_cc1101.SpiWriteReg(CC1101_PKTCTRL0, 0x32); // asynchronous serial mode
    ELECHOUSE_cc1101.SpiWriteReg(CC1101_IOCFG0, 0x0d);   // GD0 output

    return ELECHOUSE_cc1101.SpiReadReg(CC1101_TEST1) == 0x35;
}

void Radio::sendTimings(String msg){

}

void Radio::setReceiveFreq() {
    ELECHOUSE_cc1101.SpiWriteReg(CC1101_FREQ2,       0x10);
    ELECHOUSE_cc1101.SpiWriteReg(CC1101_FREQ1,       0xB0);
    ELECHOUSE_cc1101.SpiWriteReg(CC1101_FREQ0,       0x71);
}

ICACHE_RAM_ATTR void Radio::handlePinChange() {
    if(!packetBufferReady && packetBufferPos<BUFFERSIZE){
        long newTime = micros();
        long diff = newTime - time;
        if((diff < MINTIME || diff > MAXTIME)&&(time!=-1L)){
            time=-1L;
            return;
        }
        if (time != -1L) {
            packetBuffer[packetBufferPos]=diff;
            packetBufferPos++;
        }
        time = newTime;
    }
}

void Radio::enterTxMode(){
    ELECHOUSE_cc1101.SpiWriteReg(CC1101_IOCFG0, 0x2e);
    ELECHOUSE_cc1101.SetTx(433.92);
}

void Radio::enterRxMode(){
    ELECHOUSE_cc1101.SpiWriteReg(CC1101_IOCFG0, 0x0d);
    setReceiveFreq();
    ELECHOUSE_cc1101.SetRx();
    if(!isBufferReady()){
        Radio::resetBuffer();
    }
}

bool Radio::sendReady() {
    byte b [1] = {0};
    ELECHOUSE_cc1101.SpiReadBurstReg(CC1101_MARCSTATE, b, 1);
    return b[0] == 0x13;
}
