# SmartED-to-MQTT-with-ESP32
SmartED CAN-BUS Data Sniffer and Sender to MQTT Server over ESP32 WIFI

Pin Usage

 * MCP2515  = WEMOS ESP32
 * VCC      =       5V
 * GND      =      GND
 * CS       =       P5
 * SO       =      P19
 * SI       =      P23
 * SCK      =      P18
 * INT      =      P22


The MQTT-Server recieves something like this:
data/SmartED *43043*81*66*-411.20*374.60*-1423*88.50*84.50*40.50*32.50*24.50*16.50*

They are sent every 10sec over the ESP32 WIFI to the MQTT-Server:

The sniffed values are seperated by "*".

1. Odometer (km)
2. Range (km)
3. Poverlevel as in the middle instrument (33%, 66%, 99%)
4. Amperes coming/going to the traction battery (A) 
5. Volts of the traction battery (V) 
6. Calculated power coming/going to the traction battery (W)
7. Battery state of charge as in the middle gauge (%)
8. Real internal state of charge of the battery (%)
9. ECO Value over all (%)
10. ECO Value breaking (%)
11. ECO Value rolling (%)
12. ECO Value accelerating (%)


