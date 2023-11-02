# WIICMATH
Western Industrial Internet-Connected Monitor &amp; Alarm for Temperature &amp; Humidity (The WIICMATH) is an Arduino IoT device intended to use together with the WIICMATH server and frontend projects. The project is *mostly* resuable without any changes to code.

## Function
First, the device needs to be connected to the internet. If it has no WiFi credentials, the Arduino will set up an AP for the user to connect to and input new credentials.
The device then measures the air temp and humidity.
If 5 seconds or more have passed since last server connection, the device alternates between requesting device settings, or post current sensor readings to server.

## Components
- Arduino R4 WiFi
- DHT-11 air temperature and humidity sensor module
- RGB LED module
- Passive buzzer module
- Push button module

### Schematics
Schematics for module and sensor setup will be uploaded soon.
