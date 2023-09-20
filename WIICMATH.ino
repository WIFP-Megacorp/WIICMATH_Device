// Mye tatt fra https://circuitdigest.com/microcontroller-projects/interface-dht22-sensor-module-with-arduino

#include "DHT.h" // DHT sensor library

#define DHTTYPE DHT11
unsigned long delayTime;
const uint8_t DHTPin = 2; // DHT-sensor på pinne 2
const uint8_t redLedPin = 4; // Rød Led for å visuelt advare om høy temperatur.
DHT dht(DHTPin, DHTTYPE);
uint32_t delayMS;
float Temperature;
float Humidity;

//void setup is for configurations on start up
void setup() { 
  Serial.begin(9600); // 9600 bps
  pinMode(DHTPin, INPUT); //define DHTPin as an input
  pinMode(redLedPin, OUTPUT); //define ledPin as an output
  dht.begin();
    
  delayMS = 1000;
}

void loop() {
  Humidity = dht.readHumidity();
  Temperature = dht.readTemperature(); 


  if (isnan(Humidity) || isnan(Temperature)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    delay(delayMS);
    return;
  }

  Serial.println("Temp: " + String(Temperature, 2) + "°C");
  Serial.println("Humidity: " + String(Humidity, 1) + "%");
  Serial.println("---");

  //check if temperature is above 10
  if(Temperature > 26) { 
    digitalWrite(redLedPin, HIGH); //turn on the LED.
  }
  else{ 
    digitalWrite(redLedPin, LOW);
  }

  delay(delayMS); // Wait for a new reading from the sensor (DHT22 has ~0.5Hz sample rate)
}