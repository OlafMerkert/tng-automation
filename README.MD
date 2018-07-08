# Setup #

## node-red and mosquitto broker ##

      cd docker
      mkdir node-red-data
      chmod 777 node-red-data
      docker-compose up

## Install node-red nodes for on-off-keying

* Go to node-red-data
* Run npm install "../../node-red/on-off-keying"
* Go to node-red/on-off-keying and run: "npm install" and "npm run build"
* Restart docker containers if needed

### Test mosquitto mqtt broker ###

* Install mosquitto client tools (e.g. ```apt install mosquitto-client```)
* Subscribe to the test topic

      mosquitto_sub -t test -h <infrastructure-host>

* Send a message

      mosquitto_pub -t test -m "Hello wold" -h <infrastructure-host>

## Microcontroller ##

### Install arduino IDE with ESP8266 support ###

* Download Arduino from [here](https://www.arduino.cc/en/Main/Software?)
* Start Arduino IDE and go to preferences
* Enter http://arduino.esp8266.com/stable/package_esp8266com_index.json into Additional Board Manager URLs field. 
* Restart IDE and select ESP8266 as target platform
* Set flash mode to "DIO", reset method to "nodemcu", debug port to serial and select the correct usb device (e.g. ```/dev/ttyUSB0```)

### Installing OpenMQTTGateway ###

* Clone source code from here

      https://github.com/fgather/OpenMQTTGateway.git
      
* Open main sketch with Arduino "IDE"

      arduino  OpenMQTTGateway.ino
      
* Copy lib folder from OpenMQTTGateway.git to <sketchbook location>/libraries to install all required libraries
* Edit User_config.h with mqtt_server ip and wifi settings
* Flash OpenMQTTGateway.ino via upload sketch and check result on arduino console (ctrl+shift+m)
* You should get an ```Connected to broker message```


