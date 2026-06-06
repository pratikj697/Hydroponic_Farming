# Wiring Diagram Reference

## ESP8266 NodeMCU Pin Connections

### Digital Pins

|ESP8266 Pin|Connected To|Purpose|
|-|-|-|
|D0 (GPIO16)|74HC4067 Pin S0|MUX channel select bit 0|
|D1 (GPIO5)|74HC4067 Pin S1|MUX channel select bit 1|
|D2 (GPIO4)|74HC4067 Pin S2|MUX channel select bit 2|
|D3 (GPIO0)|74HC4067 Pin S3|MUX channel select bit 3|
|D4 (GPIO2)|DS18B20 Data pin|Water temperature sensor|
|D5 (GPIO14)|DHT11 Data pin|Air temp \& humidity|
|D7 (GPIO13)|Relay IN1|Grow light control|
|D8 (GPIO15)|Relay IN2|Water pump control|

### Analog Pin

|ESP8266 Pin|Connected To|Purpose|
|-|-|-|
|A0|74HC4067 Pin SIG|Analog MUX output|

\---

## 74HC4067 MUX Channel Wiring

|MUX Channel|Sensor|
|-|-|
|CH0|TDS sensor analog output|
|CH1|pH sensor analog output|
|CH2|LDR sensor analog output|
|CH3|Water level sensor analog output|

\---

## Power Connections

|Component|VCC|GND|
|-|-|-|
|DS18B20|3.3V|GND|
|DHT11|3.3V|GND|
|74HC4067|3.3V|GND|
|pH module|5V|GND|
|TDS module|3.3V–5V|GND|
|LDR module|3.3V|GND|
|Water level sensor|3.3V|GND|
|4-channel relay|5V|GND|

> **Note**: DS18B20 requires a 4.7kΩ pull-up resistor between Data and VCC.  
	DHT11 requires a 10kΩ pull-up resistor between Data and VCC.
---

