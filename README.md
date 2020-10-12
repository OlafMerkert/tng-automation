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

###Mosca (don't use)
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

###Mosquitto
```
sudo apt-get install mosquitto
sudo apt-get install mosquitto-clients
```
Mosquitto starts automatically in the background. Use the following command to see all published messages:
```
mosquitto_sub -v -t '#'
```
Replace # with a topic name to subscribe to a specific topic

To prevent automatic startup use
```
sudo systemctl disable mosquitto.service
```
Then you need to start the broker manually with
```
mosquitto -v
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

## Wifi Configuration
To connect the NodeMCU with your WIFI make sure your Router supports **802.11b**

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
sudo pip3 install esphome
```
- Add Install location to path
```
PATH=$PATH:/usr/lib/python3/dist-packages
```
- Go to `<tng-automation>/esp-smarthome`
- Create a `secrets.yaml` file and add following information
```
wifi_password: "YOUR_WIFI_PASSWORD"
wifi_ssid: "YOUR_WIFI_SSID"
```
- In `smarthome.yaml` change following information
```
mqtt:
  broker: MQTT_BROKER_IP_ADDRESS    ip adress in local network if using Mosquitto
  username: "MQTT_USERNAME"         for remote mqtt broker only
  password: "MQTT_PASSWORD"         for remote mqtt broker only
```
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

## Start RF Decoding
- Start mqtt broker mosca with `mosca -v` (not necessary if using mosquitto)
- Start node-red with `node-red` and open http://127.0.0.1:1880 in your browser
- Click the three bars in the top right corner and import flows with `Import` > `Library` > `Wetter.json` and `Steckdose_schalten.json`
- Plug the Node-MCU into your PC and go to `<tng-automation>/esp-home`
- Run `esphome smarthome.yaml run`
- Your NodeMCU should connect to the WIFI and MQTT Broker.
- Click the `an` and `aus` button in node-red to send turn off and turn on commands to your wireless socket
- Open the backplate of your weather sensor and press the `TX` button. 
- You should see weather information in the debug view of node-red

##Known Problems
```
ERROR Running command failed: [Errno 13] could not open port /dev/ttyUSB0: [Errno 13] Permission denied: '/dev/ttyUSB0'
```
If you encounter this error after starting esphome, you need to create /etc/udev/rules.d/01-ttyusb.rules with content:
```
SUBSYSTEMS=="usb-serial", TAG+="uaccess"
```