#include "esphome.h"
#include <radio.h>

#define MAXSENDBUFFERLENGTH 200
#define MAXMQTTLENGTH 2000
#define INVERT_SIGNALS false

char const *SEND_RF_TOPIC;
char const *RECEIVE_RF_TOPIC;
int SEND_REPETITIONS;
int DELAY_BETWEEN_SEND;

Radio radio = Radio();

ICACHE_RAM_ATTR void handlePinChange() {
    radio.handlePinChange();
}

class RadioGateway{
public:
    void setupGateway(){
        radio.setup();
        enterReceiveMode();
        ESP_LOGI(TAG, "CC1101 Gateway setup successfully");
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
        ESP_LOGD(TAG, "Received MQTT message with RF timings: %s", msg.c_str());
        String nextToken = "";

        for (int i = 0; i < msg.length(); i++) {
            if (msg[i] != ' ' && msg[i] != '+') {
                nextToken += msg[i];
            }

            if (msg[i] == '+') {
                ESP_LOGD(TAG, "Wait for next part of RF timings");
                return;
            }

            if (msg[i] == ' ' || i == msg.length() - 1) {
                sendBuffer[sendBufferLength] = nextToken.toInt();
                sendBufferLength++;

                if (sendBufferLength >= MAXSENDBUFFERLENGTH) {
                    sendBufferLength = 0;
                    ESP_LOGW(TAG, "RF timings buffer overflow");
                    return;
                }
                nextToken = "";
            }
        }

        ESP_LOGD(TAG, "Transmitting RF timings via CC1101");
        enterSendMode();

        for (int j = 0; j < 5; j++) {
            bool transmitting = !INVERT_SIGNALS;

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
    const char *TAG = "RadioGateway";
    void enterReceiveMode() {
        pinMode(RFCC1101_RECEIVER_PIN, INPUT);
        attachInterrupt(digitalPinToInterrupt(RFCC1101_RECEIVER_PIN), handlePinChange, CHANGE );
        radio.enterRxMode();
        ESP_LOGV(TAG, "CC1101 in receive mode");
    }
    void enterSendMode() {
        pinMode(RFCC1101_EMITTER_PIN, OUTPUT);
        digitalWrite(RFCC1101_EMITTER_PIN, 1);
        detachInterrupt(digitalPinToInterrupt(RFCC1101_RECEIVER_PIN));
        while (!radio.sendReady()) {
            radio.enterTxMode();
            delay(1);
        }
        ESP_LOGV(TAG, "CC1101 in send mode");
    }
};

class CC1101Component : public Component, public CustomMQTTDevice {
public:
    CC1101Component(char const *send_rf_topic, char const *receive_rf_topic, int send_repetitions, int delay_between_send){
        SEND_RF_TOPIC = send_rf_topic;
        RECEIVE_RF_TOPIC = receive_rf_topic;
        SEND_REPETITIONS = send_repetitions;
        DELAY_BETWEEN_SEND = delay_between_send;
    }

    void setup() {
        ESP_LOGI(TAG, "Setting up CC1101");
        gateway.setupGateway();
        subscribe(SEND_RF_TOPIC, &CC1101Component::on_message);
        log_config();
    }

    void on_message(const std::string &payload) {
        ESP_LOGV(TAG, "Received RF timings from MQTT");
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
                publish(RECEIVE_RF_TOPIC, toSend.c_str());
                ESP_LOGV(TAG, "Sent RF timings to MQTT");
                ESP_LOGVV(TAG, "Timings: %s", toSend.c_str());
                i = i+max_mqtt_length;
            }
            String toSend = msg.substring(i-max_mqtt_length, length);
            publish(RECEIVE_RF_TOPIC, toSend.c_str());
            ESP_LOGV(TAG, "Sent RF timings to MQTT");
            ESP_LOGVV(TAG, "Timings: %s", toSend.c_str());
        }
    }

    // setup after mqtt and wifi is connected
    float get_setup_priority() const override { return -100.0; }

private:
    RadioGateway gateway = RadioGateway();
    int max_mqtt_length = MAXMQTTLENGTH;
    const char *TAG = "CC1101Component";

    void log_config(){
        const char *invert_signals;
        if(INVERT_SIGNALS){
            invert_signals = "true";
        }
        else{
            invert_signals = "false";
        }
        ESP_LOGCONFIG(TAG, "CC1101Component:");
        ESP_LOGCONFIG(TAG, "  SEND_RF_TOPIC: %s", SEND_RF_TOPIC);
        ESP_LOGCONFIG(TAG, "  RECEIVE_RF_TOPIC: %s", RECEIVE_RF_TOPIC);
        ESP_LOGCONFIG(TAG, "  SEND_REPETITIONS: %i", SEND_REPETITIONS);
        ESP_LOGCONFIG(TAG, "  DELAY_BETWEEN_SEND: %i", DELAY_BETWEEN_SEND);
        ESP_LOGCONFIG(TAG, "  MAX_SEND_BUFFER_LENGTH: %i", MAXSENDBUFFERLENGTH);
        ESP_LOGCONFIG(TAG, "  MAX_MQTT_LENGTH: %i", MAXMQTTLENGTH);
        ESP_LOGCONFIG(TAG, "  INVERT_SIGNALS: %s", invert_signals);
    }
};
