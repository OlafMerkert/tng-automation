# Smarthome Hackathon 11.12.2020

# Prerequisites
If you are on **Windows** you can either install the package manager ***Chocolatey***
first, or install ***NodeJS*** and ***Python3*** by hand.

https://chocolatey.org/docs/installation
## NodeJS
- Go to https://nodejs.org/en/download
- Select your OS version installer

Alternatively use package manager:

**Ubuntu**
- `sudo apt-get install nodejs`
- `sudo apt-get install npm`

**MacOS X**
- `brew install nodejs`

**Windows**
- `choco install nodejs`

## Python 3
Any python3 version will work
- Visit the python3.9 download website at  https://www.python.org/downloads/release/python-390/
- Scroll down and select your OS version installer
- Run installation and select `Add Python 3.9 to PATH`

Alternatively use package manager:

**Ubuntu**
- `sudo apt-get install python3.9`

**MacOS X**
- `brew install python`

**Windows**
- `choco install python3 --pre`

## Mosquitto
- Visit the mosquitto download website at https://mosquitto.org/download
- Select your OS version installer
- Add mosquitto folder to environment path if necessary

Alternatively use package manager:

**Ubuntu**
- `sudo apt-get install mosquitto`
- `sudo apt-get install mosquitto-clients`

**MacOS X**
- `brew install mosquitto`

#### Disable auto start:

**Ubuntu**
- //TODO: Add command

**Windows**
- `net stop mosquitto`


## ESPHome
- Install using python package manager pip
- Open a terminal as administrator and enter 
   - `# pip3 install esphome`

## Node-Red
- Install using node package manager npm
- Open a terminal as and enter 
   - `$ npm install -g node-red`

# Cable Switch
#### 1. ESPHome firmware
- Make sure you installed ***ESPHome*** successfully, for reference check the prerequisites section

- Create a new folder and open a terminal to start the ESPHome setup wizard
   - `$ esphome cable_switch.yaml wizard`

- Enter the following information, when prompted:
    
    ***STEP 1 CORE***: cable_switch
    
    ***STEP 2 ESP (platform)***: ESP8266
    
    ***STEP 2 ESP (board)***: nodemcuv2
    
    ***STEP 3 WIFI (ssid)***: YOUR_WIFI_SSID
    
    ***STEP 3 WIFI (psk)***: YOUR_WIFI_PASSWORD
    
    ***STEP 4 OTA (password)***: press enter (no password)
 
- Inspect the ***cable_switch.yaml*** file
 
- Use a micro usb cable to connect your NodeMCU to your computer
 
- Flash the above created firmware onto your NodeMCU with the following command

   - `$ esphome cable_switch.yaml run`

- After compilation enter `1` to select ***USB Serial*** to upload the firmware

- You should now see that your NodeMCU connects to your wifi

Output:
```
[12:12:44][C][wifi:303]:   SSID: 'WIFI_NAME'
[12:12:44][C][wifi:304]:   IP Address: IP_ADDRESS
...
```

#### 2. Binary Sensor Component

- Open your ***cable_switch.yaml*** file and add following information

```
binary_sensor:
  - platform: gpio
    pin:
      number: D1
      mode: INPUT_PULLUP
      inverted: True
    name: "My first Binary Sensor"
```

- Flash the firmware onto your NodeMCU again with the command 
   - `$ esphome cable_switch.yaml run`

- Use a cable to connect the ***D1*** bin to a ***G*** pin on your NodeMCU

- You should see output similar to the following when connecting and disconnecting the two pins

Output:
```
[10:51:42][D][binary_sensor:036]: 'My first Binary Sensor': Sending state ON
[10:51:43][D][binary_sensor:036]: 'My first Binary Sensor': Sending state OFF
```

#### 3. MQTT

- Make sure you installed ***node-red*** and ***mosquitto***  successfully, for reference check the prerequisites section

- Open a new terminal and start the mqtt broker ***mosquitto*** with the command `$ mosquitto -v` (only necessary if auto start is disabled)

Output:
```
1603706412: mosquitto version 1.6.9 starting
1603706412: Using default config.
1603706412: Opening ipv4 listen socket on port 1883.
1603706412: Opening ipv6 listen socket on port 1883.
```

- Find out your computers local ip address and configure your mqtt broker in the ***cable_switch.yaml*** file

IP Address
```
ip add //Linux
ipconfig getifaddr en1 // MacOS X
ipconfig // Windows
```
Add following information to your ***cable_switch.yaml*** file
```yaml
mqtt:
  broker: YOUR_LOCAL_IPv4_ADDRESS
```

- Flash the firmware onto your NodeMCU again with the command 
   - `$ esphome cable_switch.yaml run`

- Open a new terminal and start ***node-red*** with the command `node-red`

Output:
```
26 Oct 11:02:29 - [info] Server now running at http://127.0.0.1:1880/
26 Oct 11:02:29 - [info] Starting flows
26 Oct 11:02:29 - [info] Started flows
```

- Open a browser and go to http://127.0.0.1:1880

- Drag and drop a ***mqtt in*** node and a ***debug*** node onto the main frame

- Connect the two gray dots between the nodes

- Double click the ***mqtt in*** node and click the pencil symbol

- Enter a name like `local_broker` for your local mqtt broker mosquitto, enter `localhost` in the server field and click `Add`

- Copy `cable_switch/binary_sensor/my_first_binary_sensor/state` into the topic field and click `Done`

- Click `Deploy` in the top right corner and click the ***bug symbol*** to see the debug output

- When connecting and disconnecting the D1 and G pins you should see messages sent to the mqtt topic

# LED
#### 1. ESPHome firmware
- Copy the information from your ***binar_sensor.yaml*** file to a new file named ***led.yaml***

- Change the name to ***led***
```yaml
esphome:
  name: binary_sensor  //change to led
  platform: ESP8266
  board: nodemcuv2
```

- Add the following information to your ***led.yaml*** file

```yaml
switch:
  - platform: gpio
    pin: D3
    name: "LED Switch"
```

- Connect the ***D3*** pin and a ***3V*** pin to your LED //TODO: Check if this works

- Flash the firmware onto your NodeMCU with 
    - `$ esphome led.yaml run`

- Look for output starting with

```
[08:45:59][C][mqtt.switch:038]: ...
```

- Find out which mqtt topic will turn the LED on and off

#### 2. Configure node-red

- Start mqtt broker mosquitto with `$ mosquitto` (only necessary if auto start is disabled)

- Start node-red with `$ node-red` and open http://127.0.0.1:1880 in your browser

- Add a ***mqtt out*** node and use your `local_broker` as mqtt broker

- Configure the topic you found in the terminal output above

- Add two ***inject*** nodes and send the strings "ON" and "OFF" to your ***mqtt out*** node

- Click `Deploy` in the top right corner

- Turn your LED on and off by injecting the messages

- Optional: Use your Cable Switch from the last step to turn your LED on and off

# Wireless Socket
#### 1. Setup Transceiver
- Wire up the CC1101 antenna as shown in the picture CC1101_wiring.png

- Go to `<tng-automation>/esp-smarthome/radio.yaml` and change following information
```yaml
wifi:
  ssid: "YOUR_WIFI_SSID"
  password: "YOUR_WIFI_PASSWORD"

mqtt:
  broker: YOUR_LOCAL_IP_ADDRESS
```
- Start mqtt broker mosquitto with `$ mosquitto` (only necessary if auto start is disabled)

- Flash the firmware onto your NodeMCU with
   - `$ esphome radio.yaml run`

- Received RF timings are sent to the ***esphome/433toMQTT*** topic

- MQTT messages with timings to ***esphome/MQTTto433*** are sent via RF

#### 2. Control wireless socket
- Go to `<tng-automation>/node-red` and inspect the ***wireless_socket_on*** and ***wireless_socket_off*** files

- Start node-red with `$ node-red` and open http://127.0.0.1:1880 in your browser

- Add a ***file in*** node and put the path to the `<tng-automation>/node-red/wireless_socket_on` file into the `Filename` field

- Add an ***inject*** node before the ***file in*** node to trigger reading the input file

- Connect the output of the ***file in*** node with an ***mqtt out*** node and use your `local_broker` as mqtt broker

- Set ***esphome/MQTTto433*** as MQTT Topic

- Repeat this process with the ***wireless_socket_off*** file

- Click `Deploy` in the top right corner

- Now you can turn your wireless socket on and off by triggering the inject node

#### 3. De/Encode Signals
- Instead of sending the recorded timings from the ***wireless_socket_on/off*** files we try to decode the timings and send a binary code

- On startup, node-red should print out a line like
  `7 Nov 20:48:30 - [info] User directory : home/<user>/.node-red`

- This is where node-red will keep user specific data like the flows you created, and where we can also install plugins

- Copy the `<tng-automation>/node-red/lib` folder into your node-red user directory

- Open terminal in `<tng-automation>/node-red/on-off-keying` and run:
  - `$ npm install`
  - `$ npm run build`

- Go to the **node-red user directory** (e.g. `home/<user>/.node-red`) and run
  - `$ npm install <tng-automation>/node-red/on-off-keying`

- Restart node-red and reload the node-red web interface

- There should now be four new nodes: ***ook_decode***, ***ook_encode***, ***ook_split***, ***ook_concat***

- Add a ***ook_decode*** node and double click it to see the configuration options

- Inspect the `<tng-automation>/node-red/wireless_socket_on` file and find the correct patterns for zero, one, and start to configure the ***ook_decode*** node

- Use the ***file in*** node as input for the ***ook_decode*** node and a ***debug*** node as output

- When triggering the inject node you should see the 24 bit binary code necessary to turn on the wireless socket

- Use a ***inject*** node to send this binary code as a string to an ***ook_encode*** node

- Configure the ***ook_encode*** node with the correct patterns for zero, one, and start

- Connect the output of the ***ook_encode*** with the ***mqtt out*** node

- Repeat this process with the ***wireless_socket_off*** file

- Now you should be able to turn your socket on and off using the correct binary code

- Optional: Use your Cable Switch to turn your wireless socket on and off and indicate the current status with your LED

# Weather Station
#### 1. Decode Timings
- Add a ***mqtt in*** node with `esphome/433toMQTT` as topic and `local_broker` as Server

- Add a ***debug*** node to output the timings to your debug window

- Your NodeMCU should still be sending 433 MHz signals to the specified topic, otherwise flash the radio.yaml file onto your device again

- Remove the backplate of your weather station and press the ***TX*** button to send 433 MHz signals

- You should see 433 MHz timings in the debug window

- Try to find the correct patterns for zero, one, and start to configure the ***ook_decode*** node

- Connect the output of the ***mqtt in*** node with the ***ook_decode*** node

- You should now see a binary array of length 40 in your debug window when pressing the ***TX*** button

#### 2. Decode Binary
- Add a ***function*** node and use the 40 bit binary code as input

- Add following information into the function node and configure the `Outputs` in the bottom of the screen to 2
```javascript
    function binaryToNumber(input) {
        return input.reduceRight(
            ((previousValue, currentValue, currentIndex, array) => Math.pow(2, (array.length - currentIndex - 1)) * currentValue + previousValue), 0);
    }

    let temperatureBits = msg.payload.slice(16, 28);
    let temperature = null; //TODO: Calculate temperature
    let temperatureMsg = {payload: temperature};

    let humidityBits = msg.payload.slice(28, 36);
    let humidity = null; //TODO: Calculate humidity
    let humidityMsg = {payload: humidity};

    return [temperatureMsg, humidityMsg];
```
- Hints for decoding:
    - The 12 temperatureBits represent the temperature +90°F in Fahrenheit as a binary number with one decimal places
    - The 8 humidityBits are divided in 2 groups of 4, each one representing one digit of the decimal value in binary

- Output temperature and humidity to the debug window

# Additional

#### 1. MQTT on smartphone
- Download an MQTT App for your smartphone
    - [MQTT Dash(IoT, Smart Home)](https://play.google.com/store/apps/details?id=net.routix.mqttdash&hl=de) (for Android)
    - [iHomeTouch]([http://1j2.com/ihometouch/) (for iOS)
- Control your wireless socket with your smartphone
- View temperature and humidity information on your smartphone

#### 2. Remote Broker
To replace the mosquitto broker on your pc you can use this remote broker service
- Go to https://myqtthub.com/en/ and create an account
- Log in at https://pas.aspl.es/ using your your myqtthub credentials
- Under `Last Messages Received` open following message `Service activation for your MyQttHub ...`
- Under `--== MQTT SERVER, PANEL AND API ==--` save following credentials for later use: ***MQTT Server***,
 ***User***, ***Password*** 
- Go to https://node02.myqtthub.com
- log in with your saved ***User*** and ***Password***
- Click `Add device` and define ***Client Id***, ***Username*** and ***Passsword*** for the above configuration
- In your ESPHome yaml files configure following information
```yaml
mqtt:
  broker: node02.myqtthub.com
  client_id: "CLIENT_ID"
  username: "MQTT_USERNAME"
  password: "MQTT_PASSWORD"
```
- Add another device on https://node02.myqtthub.com and configure a new broker in node-red
- Enter following information
    - `Connection > Server` node02.myqtthub.com
    - `Connection > Client ID` ***Client Id***
    - `Security > Username` ***Username***
    - `Security > Password` ***Password***