# Hydroponic_Farming

Final Year Project — Electronics & Telecommunication Engineering  
Vidyalankar Institute of Technology, Wadala, Mumbai

Project Overview:
An automated hydroponics monitoring and control system using ESP8266 NodeMCU, multiple sensors, and AWS IoT Core. Sensor data is published to the cloud every 5 seconds via MQTT over TLS, and actuators (water pump, grow light) are controlled remotely through incoming MQTT commands.

How It Works:
Sensors continuously check the water's health, room temperature, and nutrient levels.The ESP8266 chip acts as the brain to collect this data.
The data is saved online in an AWS DynamoDB database.
The system automatically turns the Water Pump ON or OFF when the plants need water or nutrients.

Hardware Components:
1)ESP8266 NodeMCU: The main brain chip.
2)pH Sensor: Checks water acidity.
3)TDS Sensor: Measures nutrient food levels.
4)DHT11 Sensor: Measures room temperature and humidity.
5)LDR Sensor: Tracks light intensity.
6)DS18B20 Sensor: Tracks water temperature.
7)Water Pump & 4-Channel Relay: Automatically circulates water.

Software Tools:
Arduino IDE & AWS IoT Core (DynamoDB)

Project Results:
1)High Water Efficiency: Saves up to 80% more water than normal soil farming by recycling the water loop.
Proven Performance: Tested successfully by growing healthy Fenugreek plants over a 15-day cycle.

This is the WorkFlow of the Project:

<img width="1540" height="854" alt="Screenshot 2026-06-03 210343" src="https://github.com/user-attachments/assets/f7ae4a47-13d5-499d-98ed-93f75ade444e" />

Schematic of desing:

<img width="1400" height="785" alt="Screenshot 2026-06-03 211404" src="https://github.com/user-attachments/assets/ec8276d7-ab76-43eb-9579-6ea89030b4ee" />

This is the Layout design:

<img width="741" height="743" alt="Layout" src="https://github.com/user-attachments/assets/deea4db7-ef31-488a-bb2a-4c8d393ddab2" />

<img width="1200" height="1600" alt="PCB_Design" src="https://github.com/user-attachments/assets/2134a93f-d1e6-4db8-b4e8-63857fb053f4" />


After Implementation:

<img width="719" height="1600" alt="Project" src="https://github.com/user-attachments/assets/086d0e0b-c4c3-4a3d-a30a-adb15f93fc78" />
