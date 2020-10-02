/*
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker
   Send and receiving command by MQTT

  This gateway enables to:
  - receive MQTT data from a topic and send RF 433Mhz signal corresponding to the received MQTT data
  - publish MQTT data to a different topic related to received 433Mhz signal
  - Uses the CC1101 RF Module for 433Mhz
  - Allows for receiving and sending of generic On-Off-Keying Patterns

    Copyright: (c)Florian ROBERT
    Copyright: (c)Florian GATHER

    This file is part of OpenMQTTGateway.

    OpenMQTTGateway is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenMQTTGateway is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifdef ZgatewayRFCC1101

#include <radio.h>

C1101Radio radio = C1101Radio(SS, RFCC1101_RECEIVER_PIN);

#define MAXSENDBUFFERLENGTH 200
uint16_t sendBuffer[MAXSENDBUFFERLENGTH];
uint16_t sendBufferLength = 0;

void setupRFCC1101() {
  radio.setup();
  enterReceiveMode();
  Log.warning(F("ZgatewayCC1101 setup done "));
}

boolean RFCC1101toMQTT() {
  radio.doWork();
  if (radio.isBufferReady()) {
    //callbackHandler(radio.getBuffer(), radio.getBufferLength());

    //Log.warning(F("CC1101 received 433 blocks" CR));
    String msg = "";

    uint16_t *timeBuffer = radio.getBuffer();
    uint16_t bufferLength = radio.getBufferLength();

    int strLen = 0;
    for (int i = 0; i < bufferLength; i++) {
      String newStr = String(timeBuffer[i]) + ' ';

      if (strLen + newStr.length() >= (200)) {
        msg += '+';
        client.publish(subjectRFCC1101toMQTT, (char *)msg.c_str());
        yield();
        msg = "";
        strLen = 0;
      }

      strLen += newStr.length();
      msg += newStr;
    }

    client.publish(subjectRFCC1101toMQTT, (char *)msg.c_str());
    yield();

    radio.resetBuffer();
    return true;
  }

  return false;
}


void MQTTtoRFCC1101(char * topicOri, char * datacallback) {
  String topic = String(topicOri);


  if (topic.equals(subjectMQTTtoRFCC1101)) {
    Log.warning("received pattern for cc1101");

    String msg = String(datacallback);
    String nextToken = "";

    for (int i = 0; i < msg.length(); i++) {
      if (msg[i] != ' ' && msg[i] != '+') {
        nextToken += msg[i];
      }

      if (msg[i] == '+') {
        Log.warning("wait for next part of timing pattern\n");
        return;
      }

      if (msg[i] == ' ' || i == msg.length() - 1) {
        sendBuffer[sendBufferLength] = nextToken.toInt();
        sendBufferLength++;

        if (sendBufferLength >= MAXSENDBUFFERLENGTH) {
          sendBufferLength = 0;
          Log.warning("buffer overflow");
          return;
        }
        nextToken = "";
      }
    }

    Log.warning("starting rf transmission\n");
    enterSendMode(434000000, 0x60);

    for (int j = 0; j < 5; j++) {
      bool transmitting = false;

      for (int i = 0; i < sendBufferLength; i++ ) {
        transmitting = !transmitting;
        digitalWrite(RFCC1101_EMITTER_PIN, !transmitting);
        delayMicroseconds(sendBuffer[i]);
      }

      digitalWrite(RFCC1101_EMITTER_PIN, 1);
      delayMicroseconds(37000);
      yield();
    }
    sendBufferLength = 0;

    enterReceiveMode();
  }
}

#ifdef jsonPublishing

#endif

void enterReceiveMode() {
  pinMode(RFCC1101_RECEIVER_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(RFCC1101_RECEIVER_PIN), handlePinChange, CHANGE );
  radio.enterRxMode();
}

void enterSendMode(uint32_t freq, uint8_t power) {
  pinMode(RFCC1101_EMITTER_PIN, OUTPUT);
  digitalWrite(RFCC1101_EMITTER_PIN, 1);
  detachInterrupt(digitalPinToInterrupt(RFCC1101_RECEIVER_PIN));

  while (radio.readReg(CC1101_MARCSTATE, CC1101_STATUS_REGISTER) != 0x13) {
    radio.enterTxMode(freq, power);
    delay(1);
  }
}

ICACHE_RAM_ATTR void handlePinChange() {
  radio.handlePinChange();
}

#endif
