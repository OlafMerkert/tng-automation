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

#include <ArduinoOTA.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <FS.h>
WiFiClient eClient;

#define convertTemp_CtoF(c) ((c * 1.8) + 32)
#define convertTemp_FtoC(f) ((f - 32) * 5 / 9)

// client link to pubsub mqtt
PubSubClient client(eClient);

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

void pub(char* topicori, char* payload, bool retainFlag) {
  String topic = String(mqtt_topic) + String(topicori);
  pubMQTT((char*)topic.c_str(), payload, retainFlag);
}

void pub(char* topicori, JsonObject& data) {
  Log.notice(F("Subject: %s" CR), topicori);
  digitalWrite(LED_RECEIVE, LED_RECEIVE_ON);
  logJson(data);
  if (client.connected()) {
    String topic = String(mqtt_topic) + String(topicori);
#ifdef valueAsASubject
#  ifdef ZgatewayPilight
    String value = data["value"];
    if (value != 0) {
      topic = topic + "/" + value;
    }
#  else
    SIGNAL_SIZE_UL_ULL value = data["value"];
    if (value != 0) {
      topic = topic + "/" + String(value);
    }
#  endif
#endif

#ifdef jsonPublishing
    Log.trace(F("jsonPublishing" CR));
#  if defined(ESP8266) || defined(ESP32) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__)
    char JSONmessageBuffer[data.measureLength() + 1];
#  else
    char JSONmessageBuffer[JSON_MSG_BUFFER];
#  endif
    data.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
    pubMQTT(topic, JSONmessageBuffer);
#endif

#ifdef simplePublishing
    Log.trace(F("simplePublishing" CR));
    // Loop through all the key-value pairs in obj
    for (JsonPair& p : data) {
#  if defined(ESP8266)
      yield();
#  endif
      if (p.value.is<SIGNAL_SIZE_UL_ULL>() && strcmp(p.key, "rssi") != 0) { //test rssi , bypass solution due to the fact that a int is considered as an SIGNAL_SIZE_UL_ULL
        if (strcmp(p.key, "value") == 0) { // if data is a value we don't integrate the name into the topic
          pubMQTT(topic, p.value.as<SIGNAL_SIZE_UL_ULL>());
        } else { // if data is not a value we integrate the name into the topic
          pubMQTT(topic + "/" + String(p.key), p.value.as<SIGNAL_SIZE_UL_ULL>());
        }
      } else if (p.value.is<int>()) {
        pubMQTT(topic + "/" + String(p.key), p.value.as<int>());
      } else if (p.value.is<float>()) {
        pubMQTT(topic + "/" + String(p.key), p.value.as<float>());
      } else if (p.value.is<char*>()) {
        pubMQTT(topic + "/" + String(p.key), p.value.as<const char*>());
      }
    }
#endif
  } else {
    Log.warning(F("client not connected can't pub" CR));
  }
}

void pub(char* topicori, char* payload) {
  if (client.connected()) {
    String topic = String(mqtt_topic) + String(topicori);
    Log.trace(F("Pub ack %s into: %s" CR), payload, topic.c_str());
    pubMQTT(topic, payload);
  } else {
    Log.warning(F("client not connected can't pub" CR));
  }
}

void pub_custom_topic(char* topicori, JsonObject& data, boolean retain) {
  if (client.connected()) {
#if defined(ESP8266) || defined(ESP32) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__)
    char JSONmessageBuffer[data.measureLength() + 1];
#else
    char JSONmessageBuffer[JSON_MSG_BUFFER];
#endif
    data.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
    Log.trace(F("Pub json :%s into custom topic: %s" CR), JSONmessageBuffer, topicori);
    pubMQTT(topicori, JSONmessageBuffer, retain);
  } else {
    Log.warning(F("client not connected can't pub" CR));
  }
}

// Low level MQTT functions
void pubMQTT(char* topic, char* payload) {
  client.publish(topic, payload);
}

void pubMQTT(char* topicori, char* payload, bool retainFlag) {
  client.publish(topicori, payload, retainFlag);
}

void pubMQTT(String topic, char* payload) {
  client.publish((char*)topic.c_str(), payload);
}

void pubMQTT(char* topic, unsigned long payload) {
  char val[11];
  sprintf(val, "%lu", payload);
  client.publish(topic, val);
}

void pubMQTT(char* topic, unsigned long long payload) {
  char val[21];
  sprintf(val, "%llu", payload);
  client.publish(topic, val);
}

void pubMQTT(char* topic, String payload) {
  client.publish(topic, (char*)payload.c_str());
}

void pubMQTT(String topic, String payload) {
  client.publish((char*)topic.c_str(), (char*)payload.c_str());
}

void pubMQTT(String topic, int payload) {
  char val[12];
  sprintf(val, "%d", payload);
  client.publish((char*)topic.c_str(), val);
}

void pubMQTT(String topic, unsigned long long payload) {
  char val[21];
  sprintf(val, "%llu", payload);
  client.publish((char*)topic.c_str(), val);
}

void pubMQTT(String topic, float payload) {
  char val[12];
  dtostrf(payload, 3, 1, val);
  client.publish((char*)topic.c_str(), val);
}

void pubMQTT(char* topic, float payload) {
  char val[12];
  dtostrf(payload, 3, 1, val);
  client.publish(topic, val);
}

void pubMQTT(char* topic, int payload) {
  char val[6];
  sprintf(val, "%d", payload);
  client.publish(topic, val);
}

void pubMQTT(char* topic, unsigned int payload) {
  char val[6];
  sprintf(val, "%u", payload);
  client.publish(topic, val);
}

void pubMQTT(char* topic, long payload) {
  char val[11];
  sprintf(val, "%l", payload);
  client.publish(topic, val);
}

void pubMQTT(char* topic, double payload) {
  char val[16];
  sprintf(val, "%d", payload);
  client.publish(topic, val);
}

void pubMQTT(String topic, unsigned long payload) {
  char val[11];
  sprintf(val, "%lu", payload);
  client.publish((char*)topic.c_str(), val);
}

void logJson(JsonObject& jsondata) {
#if defined(ESP8266) || defined(ESP32) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__)
  char JSONmessageBuffer[jsondata.measureLength() + 1];
#else
  char JSONmessageBuffer[JSON_MSG_BUFFER];
#endif
  jsondata.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
  Log.notice(F("Received json : %s" CR), JSONmessageBuffer);
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

void connectMQTT() {
#ifndef ESPWifiManualSetup
#  if defined(ESP8266) || defined(ESP32)
  checkButton(); // check if a reset of wifi/mqtt settings is asked
#  endif
#endif

  Log.warning(F("MQTT connection..." CR));
  char topic[mqtt_topic_max_size];
  strcpy(topic, mqtt_topic);
  strcat(topic, will_Topic);
  client.setBufferSize(mqtt_max_packet_size);
  if (client.connect(gateway_name, mqtt_user, mqtt_pass, topic, will_QoS, will_Retain, will_Message)) {
#if defined(ZboardM5STICKC) || defined(ZboardM5STACK)
    if (low_power_mode < 2)
      M5Display("MQTT connected", "", "");
#endif
    Log.notice(F("Connected to broker" CR));
    failure_number_mqtt = 0;
    // Once connected, publish an announcement...
    pub(will_Topic, Gateway_AnnouncementMsg, will_Retain);
    // publish version
    pub(version_Topic, OMG_VERSION, will_Retain);
    //Subscribing to topic
    char topic2[mqtt_topic_max_size];
    strcpy(topic2, mqtt_topic);
    strcat(topic2, subjectMQTTtoX);
    if (client.subscribe(topic2)) {
#ifdef ZgatewayRF
      client.subscribe(subjectMultiGTWRF); // subject on which other OMG will publish, this OMG will store these msg and by the way don't republish them if they have been already published
#endif
#ifdef ZgatewayIR
      client.subscribe(subjectMultiGTWIR); // subject on which other OMG will publish, this OMG will store these msg and by the way don't republish them if they have been already published
#endif
      Log.trace(F("Subscription OK to the subjects" CR));
    }
  } else {
    failure_number_mqtt++; // we count the failure
    Log.warning(F("failure_number_mqtt: %d" CR), failure_number_mqtt);
    Log.warning(F("failed, rc=%d" CR), client.state());
#if defined(SECURE_CONNECTION) && defined(ESP32)
    Log.warning(F("failed, ssl error code=%d" CR), eClient.lastError(nullptr, 0));
#elif defined(SECURE_CONNECTION) && defined(ESP8266)
    Log.warning(F("failed, ssl error code=%d" CR), eClient.getLastSSLError());
#endif
    digitalWrite(LED_INFO, LED_INFO_ON);
    delay(1000);
    digitalWrite(LED_INFO, !LED_INFO_ON);
    delay(4000);
#if defined(ESP8266) || defined(ESP32) && !defined(ESP32_ETHERNET)
    disconnection_handling(failure_number_mqtt);
#endif
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

void setup() {
  //Launch serial for debugging purposes
  Serial.begin(SERIAL_BAUD);
  Log.begin(LOG_LEVEL, &Serial);
  Log.notice(F(CR "************* WELCOME TO OpenMQTTGateway **************" CR));

#if defined(ESP8266) || defined(ESP32)
#  ifdef ESP8266
#    ifndef ZgatewaySRFB // if we are not in sonoff rf bridge case we apply the ESP8266 GPIO optimization
  Serial.end();
  Serial.begin(SERIAL_BAUD, SERIAL_8N1, SERIAL_TX_ONLY); // enable on ESP8266 to free some pin
#    endif
#  elif ESP32
  preferences.begin(Gateway_Short_Name, false);
  low_power_mode = preferences.getUInt("low_power_mode", DEFAULT_LOW_POWER_MODE);
  preferences.end();
#    if defined(ZboardM5STICKC) || defined(ZboardM5STACK)
  setupM5();
#    endif
#  endif

#  ifdef ESP32_ETHERNET
  setup_ethernet_esp32();
#  else // WIFI ESP
#    if defined(ESPWifiManualSetup)
  setup_wifi();
#    else
  setup_wifimanager(false);
#    endif
  Log.trace(F("OpenMQTTGateway Wifi protocol used: %d" CR), wifiProtocol);
  Log.trace(F("OpenMQTTGateway mac: %s" CR), WiFi.macAddress().c_str());
  Log.trace(F("OpenMQTTGateway ip: %s" CR), WiFi.localIP().toString().c_str());
#  endif

  setOTA();
#  ifdef SECURE_CONNECTION
  setupTLS();
#  endif
#else // In case of arduino platform

  //Launch serial for debugging purposes
  Serial.begin(SERIAL_BAUD);
  //Begining ethernet connection in case of Arduino + W5100
  setup_ethernet();
#endif

  //setup LED status
  pinMode(LED_RECEIVE, OUTPUT);
  pinMode(LED_SEND, OUTPUT);
  pinMode(LED_INFO, OUTPUT);
  digitalWrite(LED_RECEIVE, !LED_RECEIVE_ON);
  digitalWrite(LED_SEND, !LED_SEND_ON);
  digitalWrite(LED_INFO, !LED_INFO_ON);

#if defined(MDNS_SD) && defined(ESP8266)
  Log.trace(F("Connecting to MQTT by mDNS without mqtt hostname" CR));
  connectMQTTmdns();
#else
  long port;
  port = strtol(mqtt_port, NULL, 10);
  Log.trace(F("Port: %l" CR), port);
  Log.trace(F("Mqtt server: %s" CR), mqtt_server);
  client.setServer(mqtt_server, port);
#endif

  setup_parameters();

  client.setCallback(callback);

  delay(1500);
  setupRFCC1101();

  Log.trace(F("mqtt_max_packet_size: %d" CR), mqtt_max_packet_size);
  Log.notice(F("Setup OpenMQTTGateway end" CR));
}

#if defined(ESP8266) || defined(ESP32)
// Bypass for ESP not reconnecting automaticaly the second time https://github.com/espressif/arduino-esp32/issues/2501
bool wifi_reconnect_bypass() {
  uint8_t wifi_autoreconnect_cnt = 0;
#  ifdef ESP32
  while (WiFi.status() != WL_CONNECTED && wifi_autoreconnect_cnt < maxConnectionRetryWifi) {
#  else
  while (WiFi.waitForConnectResult() != WL_CONNECTED && wifi_autoreconnect_cnt < maxConnectionRetryWifi) {
#  endif
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
#  ifdef ESP32
  Log.warning(F("ESP32: Forcing to wifi %d" CR), wifiProtocol);
  esp_wifi_set_protocol(WIFI_IF_STA, wifiProtocol);
#  elif ESP8266
  Log.warning(F("ESP8266: Forcing to wifi %d" CR), wifiProtocol);
  WiFi.setPhyMode((WiFiPhyMode_t)wifiProtocol);
#  endif
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
#  ifndef ESPWifiManualSetup
    Log.error(F("Failed connecting 1st time to mqtt, you should put TRIGGER_GPIO to LOW or erase the flash" CR));
#  endif
  }
  if (failure_number <= (maxConnectionRetry + ATTEMPTS_BEFORE_BG)) {
    Log.warning(F("Attempt to reinit wifi: %d" CR), wifiProtocol);
    reinit_wifi();
  } else if ((failure_number > (maxConnectionRetry + ATTEMPTS_BEFORE_BG)) && (failure_number <= (maxConnectionRetry + ATTEMPTS_BEFORE_B))) // After maxConnectionRetry + ATTEMPTS_BEFORE_BG try to connect with BG protocol
  {
#  ifdef ESP32
    wifiProtocol = WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G;
#  elif ESP8266
    wifiProtocol = WIFI_PHY_MODE_11G;
#  endif
    Log.warning(F("Wifi Protocol changed to WIFI_11G: %d" CR), wifiProtocol);
    reinit_wifi();
  } else if ((failure_number > (maxConnectionRetry + ATTEMPTS_BEFORE_B)) && (failure_number <= (maxConnectionRetry + ATTEMPTS_BEFORE_B + ATTEMPTS_BEFORE_BG))) // After maxConnectionRetry + ATTEMPTS_BEFORE_B try to connect with B protocol
  {
#  ifdef ESP32
    wifiProtocol = WIFI_PROTOCOL_11B;
#  elif ESP8266
    wifiProtocol = WIFI_PHY_MODE_11B;
#  endif
    Log.warning(F("Wifi Protocol changed to WIFI_11B: %d" CR), wifiProtocol);
    reinit_wifi();
  } else if (failure_number > (maxConnectionRetry + ATTEMPTS_BEFORE_B + ATTEMPTS_BEFORE_BG)) // After maxConnectionRetry + ATTEMPTS_BEFORE_B try to connect with B protocol
  {
#  ifdef ESP32
    wifiProtocol = 0;
#  elif ESP8266
    wifiProtocol = 0;
#  endif
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
#  if defined(ZgatewayBT) && defined(ESP32)
    stopProcessing();
#  endif
#  if defined(ZboardM5STICKC) || defined(ZboardM5STACK)
    M5Display("OTA in progress", "", "");
#  endif
  });
  ArduinoOTA.onEnd([]() {
    Log.trace(F("\nOTA done" CR));
#  if defined(ZgatewayBT) && defined(ESP32)
    startProcessing();
#  endif
#  if defined(ZboardM5STICKC) || defined(ZboardM5STACK)
    M5Display("OTA done", "", "");
#  endif
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Log.trace(F("Progress: %u%%\r" CR), (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
#  if defined(ZgatewayBT) && defined(ESP32)
    startProcessing();
#  endif
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

#  ifdef SECURE_CONNECTION
void setupTLS() {
#    if defined(NTP_SERVER)
  configTime(0, 0, NTP_SERVER);
#    endif
#    if defined(ESP32)
  eClient.setCACert(certificate);
#    elif defined(ESP8266)
  eClient.setTrustAnchors(&caCert);
  eClient.setBufferSizes(512, 512);
#    endif
}
#  endif
#endif

#if defined(ESPWifiManualSetup)
void setup_wifi() {
  char manual_wifi_ssid[] = wifi_ssid;
  char manual_wifi_password[] = wifi_password;

  delay(10);
  WiFi.mode(WIFI_STA);
  if (wifiProtocol) forceWifiProtocol();

  // We start by connecting to a WiFi network
  Log.trace(F("Connecting to %s" CR), manual_wifi_ssid);
#  ifdef ESPWifiAdvancedSetup
  IPAddress ip_adress(ip);
  IPAddress gateway_adress(gateway);
  IPAddress subnet_adress(subnet);
  IPAddress dns_adress(Dns);
  if (!WiFi.config(ip_adress, gateway_adress, subnet_adress, dns_adress)) {
    Log.error(F("Wifi STA Failed to configure" CR));
  }
  WiFi.begin(manual_wifi_ssid, manual_wifi_password);
#  else
  WiFi.begin(manual_wifi_ssid, manual_wifi_password);
#  endif

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

#elif defined(ESP8266) || defined(ESP32)

WiFiManager wifiManager;

//flag for saving data
bool shouldSaveConfig = true;
//do we have been connected once to mqtt

//callback notifying us of the need to save config
void saveConfigCallback() {
  Log.trace(F("Should save config" CR));
  shouldSaveConfig = true;
}

#  if TRIGGER_GPIO
void checkButton() { // code from tzapu/wifimanager examples
  // check for button press
  if (digitalRead(TRIGGER_GPIO) == LOW) {
    // poor mans debounce/press-hold, code not ideal for production
    delay(50);
    if (digitalRead(TRIGGER_GPIO) == LOW) {
      Log.trace(F("Trigger button Pressed" CR));
      // still holding button for 3000 ms, reset settings, code not ideaa for production
      delay(3000); // reset delay hold
      if (digitalRead(TRIGGER_GPIO) == LOW) {
        Log.trace(F("Button Held" CR));
        Log.notice(F("Erasing ESP Config, restarting" CR));
        setup_wifimanager(true);
      }
    }
  }
}
#  else
void checkButton() {}
#  endif

void eraseAndRestart() {
#  if defined(ESP8266)
  WiFi.disconnect(true);
#  else
  WiFi.disconnect(true, true);
#  endif

  Log.trace(F("Formatting requested, result: %d" CR), SPIFFS.format());

#  if defined(ESP8266)
  ESP.eraseConfig();
  delay(5000);
  ESP.reset();
#  else
  ESP.restart();
#  endif
}

void setup_wifimanager(bool reset_settings) {
#  if TRIGGER_GPIO
  pinMode(TRIGGER_GPIO, INPUT_PULLUP);
#  endif
  delay(10);
  WiFi.mode(WIFI_STA);
  if (wifiProtocol) forceWifiProtocol();

  if (reset_settings)
    eraseAndRestart();

  //read configuration from FS json
  Log.trace(F("mounting FS..." CR));

  if (SPIFFS.begin()) {
    Log.trace(F("mounted file system" CR));
  } else {
    Log.warning(F("failed to mount FS -> formating" CR));
    SPIFFS.format();
    if (SPIFFS.begin())
      Log.trace(F("mounted file system after formating" CR));
  }
  if (SPIFFS.exists("/config.json")) {
    //file exists, reading and loading
    Log.trace(F("reading config file" CR));
    File configFile = SPIFFS.open("/config.json", "r");
    if (configFile) {
      Log.trace(F("opened config file" CR));
      size_t size = configFile.size();
      // Allocate a buffer to store contents of the file.
      std::unique_ptr<char[]> buf(new char[size]);
      configFile.readBytes(buf.get(), size);
      DynamicJsonBuffer jsonBuffer;
      JsonObject& json = jsonBuffer.parseObject(buf.get());
      json.printTo(Serial);
      if (json.success()) {
        Log.trace(F("\nparsed json" CR));
        if (json.containsKey("mqtt_server"))
          strcpy(mqtt_server, json["mqtt_server"]);
        if (json.containsKey("mqtt_port"))
          strcpy(mqtt_port, json["mqtt_port"]);
        if (json.containsKey("mqtt_user"))
          strcpy(mqtt_user, json["mqtt_user"]);
        if (json.containsKey("mqtt_pass"))
          strcpy(mqtt_pass, json["mqtt_pass"]);
        if (json.containsKey("mqtt_topic"))
          strcpy(mqtt_topic, json["mqtt_topic"]);
        if (json.containsKey("gateway_name"))
          strcpy(gateway_name, json["gateway_name"]);
      } else {
        Log.warning(F("failed to load json config" CR));
      }
    }
  }

  // The extra parameters to be configured (can be either global or just in the setup)
  // After connecting, parameter.getValue() will get you the configured value
  // id/name placeholder/prompt default length
  WiFiManagerParameter custom_mqtt_server("server", "mqtt server", mqtt_server, parameters_size);
  WiFiManagerParameter custom_mqtt_port("port", "mqtt port", mqtt_port, 6);
  WiFiManagerParameter custom_mqtt_user("user", "mqtt user", mqtt_user, parameters_size);
  WiFiManagerParameter custom_mqtt_pass("pass", "mqtt pass", mqtt_pass, parameters_size * 2);
  WiFiManagerParameter custom_mqtt_topic("topic", "mqtt base topic", mqtt_topic, mqtt_topic_max_size);
  WiFiManagerParameter custom_gateway_name("name", "gateway name", gateway_name, parameters_size * 2);

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around

  wifiManager.setConnectTimeout(WifiManager_TimeOut);
  //Set timeout before going to portal
  wifiManager.setConfigPortalTimeout(WifiManager_ConfigPortalTimeOut);

  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

//set static ip
#  ifdef NetworkAdvancedSetup
  Log.trace(F("Adv wifi cfg" CR));
  IPAddress gateway_adress(gateway);
  IPAddress subnet_adress(subnet);
  IPAddress ip_adress(ip);
  wifiManager.setSTAStaticIPConfig(ip_adress, gateway_adress, subnet_adress);
#  endif

  //add all your parameters here
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_mqtt_user);
  wifiManager.addParameter(&custom_mqtt_pass);
  wifiManager.addParameter(&custom_gateway_name);
  wifiManager.addParameter(&custom_mqtt_topic);

  //set minimum quality of signal so it ignores AP's under that quality
  wifiManager.setMinimumSignalQuality(MinimumWifiSignalQuality);

  if (!wifi_reconnect_bypass()) // if we didn't connect with saved credential we start Wifimanager web portal
  {s

    Log.notice(F("Connect your phone to WIFI AP: %s with PWD: %s" CR), WifiManager_ssid, WifiManager_password);
    //fetches ssid and pass and tries to connect
    //if it does not connect it starts an access point with the specified name
    //and goes into a blocking loop awaiting configuration
    if (!wifiManager.autoConnect(WifiManager_ssid, WifiManager_password)) {
      Log.warning(F("failed to connect and hit timeout" CR));
      delay(3000);
//reset and try again
      ESP.reset();
      delay(5000);
    }
  }

#  if defined(ZboardM5STICKC) || defined(ZboardM5STACK)
  if (low_power_mode < 2)
    M5Display("Wifi connected", "", "");
#  endif

  //read updated parameters
  strcpy(mqtt_server, custom_mqtt_server.getValue());
  strcpy(mqtt_port, custom_mqtt_port.getValue());
  strcpy(mqtt_user, custom_mqtt_user.getValue());
  strcpy(mqtt_pass, custom_mqtt_pass.getValue());
  strcpy(mqtt_topic, custom_mqtt_topic.getValue());
  strcpy(gateway_name, custom_gateway_name.getValue());

  //save the custom parameters to FS
  if (shouldSaveConfig) {
    Log.trace(F("saving config" CR));
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["mqtt_server"] = mqtt_server;
    json["mqtt_port"] = mqtt_port;
    json["mqtt_user"] = mqtt_user;
    json["mqtt_pass"] = mqtt_pass;
    json["mqtt_topic"] = mqtt_topic;
    json["gateway_name"] = gateway_name;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Log.error(F("failed to open config file for writing" CR));
    }

    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
    //end save
  }
}
#  ifdef ESP32_ETHERNET
void setup_ethernet_esp32() {
  bool ethBeginSuccess = false;
  WiFi.onEvent(WiFiEvent);
#    ifdef NetworkAdvancedSetup
  Log.trace(F("Adv eth cfg" CR));
  ETH.config(ip, gateway, subnet, Dns);
  ethBeginSuccess = ETH.begin();
#    else
  Log.trace(F("Spl eth cfg" CR));
  ethBeginSuccess = ETH.begin();
#    endif
  Log.trace(F("Connecting to Ethernet" CR));
  while (!esp32EthConnected) {
    delay(500);
    Log.trace(F("." CR));
  }
}

void WiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case SYSTEM_EVENT_ETH_START:
      Log.trace(F("Ethernet Started" CR));
      ETH.setHostname(gateway_name);
      break;
    case SYSTEM_EVENT_ETH_CONNECTED:
      Log.notice(F("Ethernet Connected" CR));
      break;
    case SYSTEM_EVENT_ETH_GOT_IP:
      Log.trace(F("OpenMQTTGateway mac: %s" CR), ETH.macAddress().c_str());
      Log.trace(F("OpenMQTTGateway ip: %s" CR), ETH.localIP().toString().c_str());
      Log.trace(F("OpenMQTTGateway link speed: %d Mbps" CR), ETH.linkSpeed());
      esp32EthConnected = true;
      break;
    case SYSTEM_EVENT_ETH_DISCONNECTED:
      Log.error(F("Ethernet Disconnected" CR));
      esp32EthConnected = false;
      break;
    case SYSTEM_EVENT_ETH_STOP:
      Log.error(F("Ethernet Stopped" CR));
      esp32EthConnected = false;
      break;
    default:
      break;
  }
}
#  endif
#else // Arduino case
void setup_ethernet() {
#  ifdef NetworkAdvancedSetup
  Log.trace(F("Adv eth cfg" CR));
  Ethernet.begin(mac, ip, Dns, gateway, subnet);
#  else
  Log.trace(F("Spl eth cfg" CR));
  Ethernet.begin(mac, ip);
#  endif
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Log.error(F("Ethernet shield was not found." CR));
  } else {
    Log.trace(F("ip: %s " CR), Ethernet.localIP());
  }
}
#endif


void loop() {

  unsigned long now = millis();

  if (WiFi.status() == WL_CONNECTED) {
    ArduinoOTA.handle();
    failure_number_ntwk = 0;
    if (client.connected()) {
#ifdef ZmqttDiscovery
      if (!connectedOnce) pubMqttDiscovery(); // at first connection we publish the discovery payloads
#endif
      connectedOnce = true;
      failure_number_ntwk = 0;

      client.loop();
      if(RFCC1101toMQTT()){
        Log.warning(F("RFCC1101toMQTT OK" CR));
        //digitalWrite(led_receive, LOW);
        //timer_led_receive = millis();
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
    digitalWrite(LED_INFO, LED_INFO_ON);
    delay(5000); // add a delay to avoid ESP32 crash and reset
    digitalWrite(LED_INFO, !LED_INFO_ON);
    delay(5000);
    Log.warning(F("wifi" CR));
    failure_number_ntwk++;
    disconnection_handling(failure_number_ntwk);
  }
}

void stateMeasures() {
  StaticJsonBuffer<JSON_MSG_BUFFER> jsonBuffer;
  JsonObject& SYSdata = jsonBuffer.createObject();
  unsigned long uptime = millis() / 1000;
  SYSdata["uptime"] = uptime;
  SYSdata["version"] = OMG_VERSION;
  Log.trace(F("retrieving value of system characteristics Uptime (s):%u" CR), uptime);
  uint32_t freeMem;
  freeMem = ESP.getFreeHeap();
  SYSdata["freemem"] = freeMem;
  long rssi = WiFi.RSSI();
  SYSdata["rssi"] = rssi;
  String SSID = WiFi.SSID();
  SYSdata["SSID"] = SSID;
  SYSdata["ip"] = ip2CharArray(WiFi.localIP());
  String mac = WiFi.macAddress();
  SYSdata["mac"] = (char*)mac.c_str();
  //SYSdata["wifiprt"] = (int)wifiProtocol;
  String modules = "";
  SYSdata["modules"] = modules;
  pub(subjectSYStoMQTT, SYSdata);
}

void receivingMQTT(char* topicOri, char* datacallback) {
  StaticJsonBuffer<JSON_MSG_BUFFER> jsonBuffer;
  JsonObject& jsondata = jsonBuffer.parseObject(datacallback);

  if (jsondata.success()) { // json object ok -> json decoding
    // log the received json
    logJson(jsondata);
    digitalWrite(LED_SEND, LED_SEND_ON);

    MQTTtoSYS(topicOri, jsondata);
  } else { // not a json object --> simple decoding
      MQTTtoRFCC1101(topicOri, datacallback);
  }
}

void MQTTtoSYS(char* topicOri, JsonObject& SYSdata) { // json object decoding
  if (cmpToMainTopic(topicOri, subjectMQTTtoSYSset)) {
    Log.trace(F("MQTTtoSYS json" CR));
#if defined(ESP8266) || defined(ESP32)
    if (SYSdata.containsKey("cmd")) {
      const char* cmd = SYSdata["cmd"];
      Log.notice(F("Command: %s" CR), cmd);
      if (strstr(cmd, restartCmd) != NULL) { //restart
#  if defined(ESP8266)
        ESP.reset();
#  else
        ESP.restart();
#  endif
      } else if (strstr(cmd, eraseCmd) != NULL) { //erase and restart
#  ifndef ESPWifiManualSetup
        setup_wifimanager(true);
#  endif
      } else {
        Log.warning(F("wrong command" CR));
      }
    }
#endif
  }
}
