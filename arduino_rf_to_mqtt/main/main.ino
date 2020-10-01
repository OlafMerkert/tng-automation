/*
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation
   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker
   Send and receiving command by MQTT
  This program enables to:
 - receive MQTT data from a topic and send signal (RF, IR, BLE, GSM)  corresponding to the received MQTT data
 - publish MQTT data to a different topic related to received signals (RF, IR, BLE, GSM)
  Copyright: (c)Florian ROBERT
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

#include "User_config.h"
#include "config_RFCC1101.h"

//Time used to wait for an interval before checking system measures
unsigned long timer_sys_measures = 0;
#define ARDUINOJSON_USE_LONG_LONG 1
#include <ArduinoJson.h>
#include <ArduinoLog.h>
#include <PubSubClient.h>
#include <Arduino.h>
#include <ArduinoOTA.h>

/*------------------------------------------------------------------------*/

//adding this to bypass the problem of the arduino builder issue 50
void callback(char* topic, byte* payload, unsigned int length);

char mqtt_user[parameters_size] = MQTT_USER; // not compulsory only if your broker needs authentication
char mqtt_pass[parameters_size * 2] = MQTT_PASS; // not compulsory only if your broker needs authentication
char mqtt_server[parameters_size] = MQTT_SERVER;
char mqtt_port[6] = MQTT_PORT;
char mqtt_topic[mqtt_topic_max_size] = Base_Topic;
char gateway_name[parameters_size * 2] = Gateway_Name;

bool connectedOnce = false; //indicate if we have been connected once to MQTT
int failure_number_ntwk = 0; // number of failure connecting to network
int failure_number_mqtt = 0; // number of failure connecting to MQTT

uint8_t wifiProtocol = 0; // default mode, automatic selection

unsigned long timer_led_measures = 0;


WiFiClient eClient;

// client link to pubsub mqtt
PubSubClient client(eClient);



void setup() {
  //Launch serial for debugging purposes
  Serial.begin(SERIAL_BAUD);
  Log.begin(LOG_LEVEL, &Serial);
  Serial.print(F(CR "************* WELCOME TO OpenMQTTGateway **************" CR));

  setup_wifi();
  
  Log.notice(F("OpenMQTTGateway Wifi protocol used: %d" CR), wifiProtocol);
  Log.notice(F("OpenMQTTGateway mac: %s" CR), WiFi.macAddress().c_str());
  Log.notice(F("OpenMQTTGateway ip: %s" CR), WiFi.localIP().toString().c_str());
  setOTA();

  long port;
  port = strtol(mqtt_port, NULL, 10);
  Log.trace(F("Port: %l" CR), port);
  Log.trace(F("Mqtt server: %s" CR), mqtt_server);
  client.setServer(mqtt_server, port);

  setup_parameters();

  client.setCallback(callback);

  delay(1500);
  
  setupRFCC1101();

  Log.trace(F("mqtt_max_packet_size: %d" CR), mqtt_max_packet_size);
  Log.notice(F("Setup OpenMQTTGateway end" CR));
}


void setup_wifi() {
  char manual_wifi_ssid[] = wifi_ssid;
  char manual_wifi_password[] = wifi_password;

  delay(10);
  WiFi.mode(WIFI_STA);
  if (wifiProtocol) forceWifiProtocol();

  // We start by connecting to a WiFi network
  Log.trace(F("Connecting to %s" CR), manual_wifi_ssid);
#ifdef ESPWifiAdvancedSetup
  IPAddress ip_adress(ip);
  IPAddress gateway_adress(gateway);
  IPAddress subnet_adress(subnet);
  IPAddress dns_adress(Dns);
  if (!WiFi.config(ip_adress, gateway_adress, subnet_adress, dns_adress)) {
    Log.error(F("Wifi STA Failed to configure" CR));
  }
  WiFi.begin(manual_wifi_ssid, manual_wifi_password);
#else
  WiFi.begin(manual_wifi_ssid, manual_wifi_password);
#endif

  if (wifi_reconnect_bypass())
    Log.notice(F("Connected with saved credentials" CR));

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Log.trace(F("." CR));
    failure_number_ntwk++;
    disconnection_handling(failure_number_ntwk);
  }
  Log.notice(F("WiFi ok with manual config credentials" CR));
}


void loop() {

  unsigned long now = millis();

  if (WiFi.status() == WL_CONNECTED) {
    ArduinoOTA.handle();
    failure_number_ntwk = 0;
    if (client.connected()) {
      connectedOnce = true;
      failure_number_ntwk = 0;

      client.loop();
      if(RFCC1101toMQTT()){
        //Serial.println(F("RFCC1101toMQTT OK" CR));
      }
      if (now > (timer_sys_measures + (TimeBetweenReadingSYS * 1000)) || !timer_sys_measures) {
        timer_sys_measures = millis();
        stateMeasures();
      }
    } else {
      connectMQTT();
    }
  } else { // disconnected from network
    Log.warning(F("Network disconnected:" CR));
    Log.warning(F("wifi" CR));
    failure_number_ntwk++;
    disconnection_handling(failure_number_ntwk);
  }
}

void stateMeasures() {
  String state = "State meassurements:\n";
  state = state + "uptime: " + String(millis() / 1000) + "\n";
  state = state + "version: " + String(OMG_VERSION) + "\n";
  state = state + "freemem: " + String(ESP.getFreeHeap()) + "\n";
  state = state + "rssi: " + String(WiFi.RSSI()) + "\n";
  state = state + "SSID: " + WiFi.SSID() + "\n";
  state = state + "ip: " + String(ip2CharArray(WiFi.localIP())) + "\n";
  state = state + "mac: " + WiFi.macAddress() + "\n";
  String b = "fff";
  state = state+b;
  client.publish(subjectSYStoMQTT, (char *)state.c_str());
  Serial.println(state);
}

void receivingMQTT(char* topicOri, char* datacallback) {
  StaticJsonBuffer<JSON_MSG_BUFFER> jsonBuffer;
  JsonObject& jsondata = jsonBuffer.parseObject(datacallback);

  if (jsondata.success()) { // json object ok -> json decoding
    // log the received json
    //logJson(jsondata);
    MQTTtoSYS(topicOri, jsondata);
  } else { // not a json object --> simple decoding
      MQTTtoRFCC1101(topicOri, datacallback);
  }
}

void MQTTtoSYS(char* topicOri, JsonObject& SYSdata) { // json object decoding
  if (cmpToMainTopic(topicOri, subjectMQTTtoSYSset)) {
    Log.trace(F("MQTTtoSYS json" CR));
    if (SYSdata.containsKey("cmd")) {
      const char* cmd = SYSdata["cmd"];
      Log.notice(F("Command: %s" CR), cmd);
      if (strstr(cmd, restartCmd) != NULL) { //restart
        ESP.reset();
      } else {
        Log.warning(F("wrong command" CR));
      }
    }
  }
}


void connectMQTT() {
  Log.warning(F("MQTT connection..." CR));
  char topic[mqtt_topic_max_size];
  strcpy(topic, mqtt_topic);
  strcat(topic, will_Topic);
  client.setBufferSize(mqtt_max_packet_size);
  if (client.connect(gateway_name, mqtt_user, mqtt_pass, topic, will_QoS, will_Retain, will_Message)) {
    Log.notice(F("Connected to broker" CR));
    failure_number_mqtt = 0;
    //Subscribing to topic
    char topic2[mqtt_topic_max_size];
    strcpy(topic2, mqtt_topic);
    strcat(topic2, subjectMQTTtoX);
    if (client.subscribe(topic2)) {
      Log.trace(F("Subscription OK to the subjects" CR));
    }
  } else {
    failure_number_mqtt++; // we count the failure
    Log.warning(F("failure_number_mqtt: %d" CR), failure_number_mqtt);
    Log.warning(F("failed, rc=%d" CR), client.state());
    disconnection_handling(failure_number_mqtt);
  }
}

// Callback function, when the gateway receive an MQTT value on the topics subscribed this function is called
void callback(char* topic, byte* payload, unsigned int length) {
  // In order to republish this payload, a copy must be made
  // as the orignal payload buffer will be overwritten whilst
  // constructing the PUBLISH packet.
  Log.trace(F("Hey I got a callback " CR));
  // Allocate the correct amount of memory for the payload copy
  byte* p = (byte*)malloc(length + 1);
  // Copy the payload to the new buffer
  memcpy(p, payload, length);
  // Conversion to a printable string
  p[length] = '\0';
  //launch the function to treat received data if this data concern OpenMQTTGateway
  if ((strstr(topic, subjectMultiGTWKey) != NULL) || (strstr(topic, subjectGTWSendKey) != NULL))
    receivingMQTT(topic, (char*)p);
  // Free the memory
  free(p);
}

void setup_parameters() {
  strcat(mqtt_topic, gateway_name);
}

// Bypass for ESP not reconnecting automaticaly the second time https://github.com/espressif/arduino-esp32/issues/2501
bool wifi_reconnect_bypass() {
  uint8_t wifi_autoreconnect_cnt = 0;
  while (WiFi.waitForConnectResult() != WL_CONNECTED && wifi_autoreconnect_cnt < maxConnectionRetryWifi) {
    Log.notice(F("Attempting Wifi connection with saved AP: %d" CR), wifi_autoreconnect_cnt);
    WiFi.begin();
    delay(500);
    wifi_autoreconnect_cnt++;
  }
  if (wifi_autoreconnect_cnt < maxConnectionRetryWifi) {
    return true;
  } else {
    return false;
  }
}

// the 2 methods below are used to recover wifi connection by changing the protocols
void forceWifiProtocol() {
  Log.warning(F("ESP8266: Forcing to wifi %d" CR), wifiProtocol);
  WiFi.setPhyMode((WiFiPhyMode_t)wifiProtocol);
}

void reinit_wifi() {
  delay(10);
  WiFi.mode(WIFI_STA);
  if (wifiProtocol) forceWifiProtocol();
  WiFi.begin();
}

void disconnection_handling(int failure_number) {
  Log.warning(F("disconnection_handling, failed %d times" CR), failure_number);
  if ((failure_number > maxConnectionRetry) && !connectedOnce) {
  }
  if (failure_number <= (maxConnectionRetry + ATTEMPTS_BEFORE_BG)) {
    Log.warning(F("Attempt to reinit wifi: %d" CR), wifiProtocol);
    reinit_wifi();
  } else if ((failure_number > (maxConnectionRetry + ATTEMPTS_BEFORE_BG)) && (failure_number <= (maxConnectionRetry + ATTEMPTS_BEFORE_B))) // After maxConnectionRetry + ATTEMPTS_BEFORE_BG try to connect with BG protocol
  {
    wifiProtocol = WIFI_PHY_MODE_11G;
    Log.warning(F("Wifi Protocol changed to WIFI_11G: %d" CR), wifiProtocol);
    reinit_wifi();
  } else if ((failure_number > (maxConnectionRetry + ATTEMPTS_BEFORE_B)) && (failure_number <= (maxConnectionRetry + ATTEMPTS_BEFORE_B + ATTEMPTS_BEFORE_BG))) // After maxConnectionRetry + ATTEMPTS_BEFORE_B try to connect with B protocol
  {
    wifiProtocol = WIFI_PHY_MODE_11B;
    Log.warning(F("Wifi Protocol changed to WIFI_11B: %d" CR), wifiProtocol);
    reinit_wifi();
  } else if (failure_number > (maxConnectionRetry + ATTEMPTS_BEFORE_B + ATTEMPTS_BEFORE_BG)) // After maxConnectionRetry + ATTEMPTS_BEFORE_B try to connect with B protocol
  {
    wifiProtocol = 0;
    Log.warning(F("Wifi Protocol reverted to normal mode: %d" CR), wifiProtocol);
    reinit_wifi();
  }
}

void setOTA() {
  // Port defaults to 8266
  ArduinoOTA.setPort(ota_port);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(ota_hostname);

  // No authentication by default
  ArduinoOTA.setPassword(ota_password);

  ArduinoOTA.onStart([]() {
    Log.trace(F("Start OTA, lock other functions" CR));
  });
  ArduinoOTA.onEnd([]() {
    Log.trace(F("\nOTA done" CR));
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Log.trace(F("Progress: %u%%\r" CR), (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR)
      Log.error(F("Auth Failed" CR));
    else if (error == OTA_BEGIN_ERROR)
      Log.error(F("Begin Failed" CR));
    else if (error == OTA_CONNECT_ERROR)
      Log.error(F("Connect Failed" CR));
    else if (error == OTA_RECEIVE_ERROR)
      Log.error(F("Receive Failed" CR));
    else if (error == OTA_END_ERROR)
      Log.error(F("End Failed" CR));
  });
  ArduinoOTA.begin();
}

void revert_hex_data(const char* in, char* out, int l) {
  //reverting array 2 by 2 to get the data in good order
  int i = l - 2, j = 0;
  while (i != -2) {
    if (i % 2 == 0)
      out[j] = in[i + 1];
    else
      out[j] = in[i - 1];
    j++;
    i--;
  }
  out[l - 1] = '\0';
}

void extract_char(const char* token_char, char* subset, int start, int l, bool reverse, bool isNumber) {
  if (isNumber) {
    if (reverse)
      revert_hex_data(token_char + start, subset, l + 1);
    long long_value = strtoul(subset, NULL, 16);
    sprintf(subset, "%ld", long_value);
  } else {
    if (reverse)
      revert_hex_data(token_char + start, subset, l + 1);
    else
      strncpy(subset, token_char + start, l + 1);
  }
  subset[l] = '\0';
}

char* ip2CharArray(IPAddress ip) { //from Nick Lee https://stackoverflow.com/questions/28119653/arduino-display-ethernet-localip
  static char a[16];
  sprintf(a, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
  return a;
}

bool to_bool(String const& s) { // thanks Chris Jester-Young from stackoverflow
  return s != "0";
}

bool cmpToMainTopic(char* topicOri, char* toAdd) {
  char topic[mqtt_topic_max_size];
  strcpy(topic, mqtt_topic);
  strcat(topic, toAdd);
  if (strstr(topicOri, topic) != NULL) {
    return true;
  } else {
    return false;
  }
}
