# Hydroponic_Farming


Project Overview:
This project is a Smart Hydroponic System. It allows plants to grow in water without any soil. It uses smart sensors to check plant health and sends all the data 
to the cloud (AWS) so you can monitor your plants from anywhere.

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

<img width="1540" height="854" alt="Screenshot 2026-06-03 210343" src="https://github.com/user-attachments/assets/f7ae4a47-13d5-499d-98ed-93f75ade444e" />

