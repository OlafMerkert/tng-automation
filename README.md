# Prerequisite

## nodejs
Download [nodejs](https://nodejs.org/en/download/)
or install via apt-get
```
sudo apt-get install nodejs --version 10.19.0
sudo apt-get install npm --version 6.14.4
```
## python
Install [python](https://www.python.org/downloads/)

## MQTT broker
```
remove for version without warnings
```
Install
```
sudo npm install -g mosca
```
Run with
```
mosca -v
```

## Node RED
```
remove for version without warnings
```
Install
```
sudo npm install -g node-red
```
Start with
```
node-red
```
Open a browser at this [URL](http://127.0.0.1:1880/).

## Arduino
**Do not use!!!**
```
sudo apt-get install arduino //outdated!!
```
Instead install Arduino from [here](https://www.arduino.cc/en/Main/Software).

Move folder to desired install location, move to the downloaded folder and run
```
sudo ./install.sh
```
Start Arduino and open preferences

Enter `http://arduino.esp8266.com/stable/package_esp8266com_index.json` into “Additional Board Manager URLs” field. 

Open `Tools` > `Board: "Arduino Uno"` > `Boards Manager...` and install the “esp8266” package.

Under `Tools` > `Board: "Arduino Uno"` > `ESP8266 Boards (2.7.4)` select `NodeMCU 1.0 (ESP-12E Module)` as target platform.

Under `Tools` > `Port`, select the correct usb device (e.g. `/dev/ttyUSB0`) after connecting the board via USB.


## Using OpenMQTT

- Copy the `<tng-automation>/arduino_rf_to_mqtt/libraries` folder into your Arduino user folder (e.g. `~/Arduino`).

- Open main sketch inside the repo with the Arduino IDE

```
arduino <tng-automation>/arduino\_rf\_to\_mqtt/main/main.ino
```
- Edit User_config.h with mqtt_server IP and WiFi settings Variables which need to be adjusted
	
	
	-- wifi_ssid
	
	-- wifi_password
	   
	-- MQTT_SERVER

optional
    
    -- MQTT_PORT
    
    -- MQTT_USER (cloud only)
    
    -- MQTT_PASS (cloud only)
	
- Flash Arduino Program onto NodeMCU with `Sketch` > `Upload`
- Open Arduino console (ctrl+shift+m) and select `11520 baud`
- You should see your NodeMCU connecting to WIFI and MQTT Broker

## Using ESPHome
- Install
```
pip3 install esphome
```
- Add Install location to path
```
PATH=$PATH:/usr/lib/python3/dist-packages
```
- Go to `<tng-automation>/esp_rf_to_mqtt`
- Change `ssid` and `password` according for your WIFI connection
- Run `esphome smarthome.yaml run`
- You should see your NodeMCU connecting to WIFI
## RF
- Wire up the C11011 receiver as in the picture C1011_Schaltplan.png

# Processing RF Data with Node Red

## Locate the node-red user directory

On startup, node-red sould print out a line like

`7 Nov 20:48:30 - [info] User directory : home/olaf/.node-red`

This is were node-red will keep user specific data like the flows you created, and where we can also install plugins.

## Copy example flows
Copy the `<tng-automation>/node-red/lib` folder into the node-red user directory

## Build and install ook en/decoder

Open terminal in `<tng-automation>/node-red/on-off-keying` and run:
```
npm install
npm run build
```
Go to the **node-red user directory** (e.g. `home/olaf/.node-red`) and run
```
npm install <tng-automation>/node-red/on-off-keying
```
- Restart node-red and reload the node-red web interface
- There should now be four new nodes: "ook_decode", "ook_encode", "ook_split", "ook_concat"

- Open node-red in the browser and click the three bars in the top left corner `Import` > `Library` > `Wetter_Steckdose.json` > `Import`

- Click the bug symbol in the top right corner and see Temperature and Humidity Data sent to your Sensor
