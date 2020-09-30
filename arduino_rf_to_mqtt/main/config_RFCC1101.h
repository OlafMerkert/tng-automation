/*  
  OpenMQTTGateway  - ESP8266 or Arduino program for home automation 

   Act as a wifi or ethernet gateway between your 433mhz/infrared IR signal  and a MQTT broker 
   Send and receiving command by MQTT   
  
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

/*-------------------RF topics & parameters----------------------*/
//433Mhz MQTT Subjects and keys
#define subjectMQTTtoRFCC1101  Base_Topic Gateway_Name "/commands/MQTTto433"
#define subjectRFCC1101toMQTT  Base_Topic Gateway_Name "/433toMQTT"

/*-------------------PIN DEFINITIONS----------------------*/

#define RFCC1101_RECEIVER_PIN 4 // D2
#define RFCC1101_EMITTER_PIN 4
