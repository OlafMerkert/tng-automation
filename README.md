# Prerequisite

## nodejs
Download nodejs [here](https://nodejs.org/en/download/)
or install via command line

***Ubuntu***
```
sudo apt-get install nodejs
sudo apt-get install npm
```
***MacOS X***
```
brew install nodejs
```
***Windows***
```
choco install nodejs
```

## python
Download python [here](https://www.python.org/downloads/)
or install via command line
***Ubuntu***
```
sudo apt-get install python3.x
```
***MacOS X***
```
brew install python
```
***Windows***
```
choco install python3 --pre 
```
## MQTT broker

### Mosquitto
***Ubuntu***
```
sudo apt-get install mosquitto
sudo apt-get install mosquitto-clients
```
***MacOS X***
```
brew install mosquitto
```
***Windows***

Download mosquitto from [here](https://mosquitto.org/files/binary/win64/mosquitto-1.6.12a-install-windows-x64.exe)

Add mosquitto to Environment Path

---
To prevent automatic startup use

***Ubuntu***
```
sudo systemctl disable mosquitto.service
```

***Windows***
```
# net stop mosquitto
```

***MacOS X***
```
tba
```
---
Then you need to start the broker manually with
```
mosquitto -v
```
Use the following command to see published messages to the topic: /some/topic:
```
mosquitto_sub -v -t '/some/topic'
```

### Remote Broker (optional)
- Go to https://myqtthub.com/en/ and create an account
- Log in at https://pas.aspl.es/ using your your myqtthub credentials 
- Under ***Last Messages Received*** open following message `Service activation for your MyQttHub ...`
- Under `--== MQTT SERVER, PANEL AND API ==--` save following credentials for later use: ***MQTT Server***,
 ***User***, ***Password*** 

## Node RED
- Install
```
# npm install -g node-red
```
- Start
```
node-red
```
Open a browser at http://127.0.0.1:1880

## ESPHome
- Install
```
# pip3 install esphome
```
- Go to `<tng-automation>/esp-smarthome`
- Create a `secrets.yaml` file and add following information
```
wifi_ssid: "YOUR_WIFI_SSID"
wifi_password: "YOUR_WIFI_PASSWORD"
```
- In `smarthome.yaml` change following information
```
mqtt:
  broker: MQTT_BROKER_IP_ADDRESS    ip adress in local network if using Mosquitto
  client_id: "CLIENT_ID"            for remote mqtt broker only
  username: "MQTT_USERNAME"         for remote mqtt broker only
  password: "MQTT_PASSWORD"         for remote mqtt broker only
```
***Remote MQTT Broker***
- Go to https://node02.myqtthub.com
- log in with your saved ***User*** and ***Password***
- Click ***Add device*** and define ***Client Id***, ***Username*** and ***Passsword*** for the above configuration

## Radio Frequency
- Wire up the C11011 receiver as in the picture C1011_Schaltplan.png

# Processing RF Data with Node Red

## Locate the node-red user directory

On startup, node-red should print out a line like

`7 Nov 20:48:30 - [info] User directory : home/user/.node-red`

This is where node-red will keep user specific data like the flows you created, and where we can also install plugins.

## Copy example flows
Copy the `<tng-automation>/node-red/lib` folder into the node-red user directory

## Build and install ook en/decoder

Open terminal in `<tng-automation>/node-red/on-off-keying` and run:
```
npm install
npm run build
```
Go to the **node-red user directory** (e.g. `home/user/.node-red`) and run
```
npm install <tng-automation>/node-red/on-off-keying
```
- Restart node-red and reload the node-red web interface
- There should now be four new nodes: "ook_decode", "ook_encode", "ook_split", "ook_concat"

## Start RF Decoding
- Start mqtt broker mosquitto with `mosquitto -v` (only necessary if auto start is disabled)
- Start node-red with `node-red` and open http://127.0.0.1:1880 in your browser
- Click the three bars in the top right corner and import flows with `Import` > `Library` > `Wetter.json` and `Steckdose_schalten.json`
- Plug the Node-MCU into your PC and go to `<tng-automation>/esp-home`
- Run `esphome smarthome.yaml run`
- Your NodeMCU should connect to the WIFI and MQTT Broker.
- Click the `an` and `aus` button in node-red to send turn off and turn on commands to your wireless socket
- Open the backplate of your weather sensor and press the `TX` button. 
- You should see weather information in the debug view of node-red

***Remote MQTT Broker***
- Add a new device on MyQtt like in the ***ESPHome*** step
- Double Click the ***/esphome/433toMQTT*** node
- Select `Server` > `Add new mqtt-broker...`  and click the pencil symbol
- Define a ***Name*** for your remote broker in node-red
- Select ***Enable secure (SSL/TLS) connection***
- Enter following information: ***Port***: 8883 ***Server***: node02.myqtthub.com
- Under `Security` insert ***Username*** and ***Password*** as defined while adding a new device on MyQtt

## Smartphone Apps

Apps to try out:
- [MQTT Dash(IoT, Smart Home)](https://play.google.com/store/apps/details?id=net.routix.mqttdash&hl=de) (for Android)
- [iHomeTouch]([http://1j2.com/ihometouch/) (for iOS, please evaluate)

# Common Issues
## USB Permission
```
ERROR Running command failed: [Errno 13] could not open port /dev/ttyUSB0: [Errno 13] Permission denied: '/dev/ttyUSB0'
```
Fix 1:
If esphome is installed for all users, start esphome as admin
```
# esphome some_name.yaml run
```
Fix 2:
If esphome is installed only for one user, create /etc/udev/rules.d/01-ttyusb.rules with content:
```
SUBSYSTEMS=="usb-serial", TAG+="uaccess"
```

## WIFI Configuration
To connect the NodeMCU with your WIFI make sure **802.11b** is enabled