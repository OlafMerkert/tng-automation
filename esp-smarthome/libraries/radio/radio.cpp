#include "radio.h"

long unsigned lastSend = 0;
int sendDelay = 5000;


void Radio::sendTimings(String timings){
    bool failed = false;
    //TODO: Send timings via RF
    if(failed){
        Serial.println("Failed sending timings");
    }
    else{
        Serial.print("Timings \"");
        Serial.print(timings);
        Serial.println("\" transmitted successfully");
    }
}
String Radio::getTimings(){
    unsigned long now = millis();
    bool bufferReady = (now > lastSend+sendDelay);
    //TODO: Receive timings
    if(bufferReady){
        String timings = String("400 10000 300 900 300 900");
        Serial.print("Timings \"");
        Serial.print(timings);
        Serial.println("\" read successfully");
        lastSend = now;
        return timings;
    }
    else{
        return String("");
    }
}
