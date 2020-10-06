#include "esphome.h"
#include "radio.h"

#define RECEIVE_STATE_TOPIC "/esphome/MQTTto433"
#define TRANSMIT_STATE_TOPIC "/esphome/433toMQTT"


class CC1101Component : public Component, public CustomMQTTDevice {
public:
    Radio radio;

    void setup() {
        Serial.println("Setup success");
        subscribe(RECEIVE_STATE_TOPIC, &CC1101Component::on_message);
    }

    void on_message(const std::string &payload) {
        Serial.println("Received timings");
        radio.sendTimings(String(payload.c_str()));
    }

    void loop() {
        String timings = radio.getTimings();
        if(!timings.equals("")){
            Serial.println("Publishing timings");
            publish(TRANSMIT_STATE_TOPIC, timings.c_str());
        }
    }
};
