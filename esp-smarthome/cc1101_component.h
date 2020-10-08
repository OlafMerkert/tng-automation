#include "esphome.h"
#include <radio.h>
#include <queue>

#define RECEIVE_STATE_TOPIC "/esphome/MQTTto433"
#define TRANSMIT_STATE_TOPIC "/esphome/433toMQTT"
#define MAXSENDBUFFERLENGTH 200
#define MAXMQTTLENGTH 2000
#define SEND_REPETITIONS 5
#define DELAY_BETWEEN_SEND 300
#define INVERT_SIGNALS true

Radio radio = Radio();

ICACHE_RAM_ATTR void handlePinChange() {
    radio.handlePinChange();
}

class RadioGateway{
public:
    void setupGateway(){
        radio.setup();
        enterReceiveMode();
        Serial.println("Start Receiving");
    }

    String RFCC1101toMQTT(){
        radio.doWork();
        if(radio.isBufferReady()){
            String msg = "";

            uint16_t *timeBuffer = radio.getBuffer();
            uint16_t bufferLength = radio.getBufferLength();

            for (int i = 0; i < bufferLength; i++) {
                String newStr = String(timeBuffer[i]) + ' ';
                msg += newStr;
            }
            radio.resetBuffer();
            return msg.substring(0, msg.length()-1);
        }
        return String("");
    }

    void MQTTtoRFCC1101(String msg){
        Serial.println("Starting sending");
        String nextToken = "";
        Serial.println(msg);

        for (int i = 0; i < msg.length(); i++) {
            if (msg[i] != ' ' && msg[i] != '+') {
                nextToken += msg[i];
            }

            if (msg[i] == '+') {
                Serial.println("wait for next part of timing pattern");
                return;
            }

            if (msg[i] == ' ' || i == msg.length() - 1) {
                sendBuffer[sendBufferLength] = nextToken.toInt();
                sendBufferLength++;

                if (sendBufferLength >= MAXSENDBUFFERLENGTH) {
                    sendBufferLength = 0;
                    Serial.println("buffer overflow");
                    return;
                }
                nextToken = "";
            }
        }

        Serial.println("starting rf transmission");
        enterSendMode();

        for (int j = 0; j < 5; j++) {
            bool transmitting = INVERT_SIGNALS;

            for (int i = 0; i < sendBufferLength; i++ ) {
                transmitting = !transmitting;
                digitalWrite(RFCC1101_EMITTER_PIN, !transmitting);
                delayMicroseconds(sendBuffer[i]);
            }
            digitalWrite(RFCC1101_EMITTER_PIN, transmitting);
            delayMicroseconds(DELAY_BETWEEN_SEND);
        }
        sendBufferLength = 0;

        enterReceiveMode();
    }

private:
    uint16_t sendBuffer[MAXSENDBUFFERLENGTH];
    uint16_t sendBufferLength = 0;
    void enterReceiveMode() {
        pinMode(RFCC1101_RECEIVER_PIN, INPUT);
        attachInterrupt(digitalPinToInterrupt(RFCC1101_RECEIVER_PIN), handlePinChange, CHANGE );
        radio.enterRxMode();
    }
    void enterSendMode() {
        pinMode(RFCC1101_EMITTER_PIN, OUTPUT);
        digitalWrite(RFCC1101_EMITTER_PIN, 1);
        detachInterrupt(digitalPinToInterrupt(RFCC1101_RECEIVER_PIN));
        while (!radio.sendReady()) {
            radio.enterTxMode();
            delay(1);
        }
    }
};

class CC1101Component : public Component, public CustomMQTTDevice {
public:
    RadioGateway gateway = RadioGateway();

    void setup() {
        gateway.setupGateway();
        subscribe(RECEIVE_STATE_TOPIC, &CC1101Component::on_message);
    }

    void on_message(const std::string &payload) {
        Serial.println("Received timings from MQTT");
        gateway.MQTTtoRFCC1101(String(payload.c_str()));
    }

    void loop() {
        String msg = gateway.RFCC1101toMQTT();
        if(!msg.equals("")){
            int i = max_mqtt_length;
            int length = msg.length();
            while(i<length){
                String toSend = msg.substring(i-max_mqtt_length, i);
                toSend = toSend+"+";
                publish(TRANSMIT_STATE_TOPIC, toSend.c_str());
                i = i+max_mqtt_length;
            }
            String toSend = msg.substring(i-max_mqtt_length, length);
            publish(TRANSMIT_STATE_TOPIC, toSend.c_str());
        }
    }

    // setup after mqtt and wifi is connected
    float get_setup_priority() const override { return -100.0; }

private:
    int max_mqtt_length = MAXMQTTLENGTH;
};
