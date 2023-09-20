#include "DHT.h" // DHT sensor library

#define DHTTYPE DHT11

const uint8_t PIN_BUZZER  = 8;
const uint8_t PIN_RED   = 11;
const uint8_t PIN_GREEN = 12;
const uint8_t PIN_BLUE  = 13;
const uint8_t PIN_DHT = 2;

unsigned long time_now = 0;

bool statusActive = 0;
bool statusConnecting = 0;
bool statusAlarm = 0;

float temperature;
float humidity;
int updateDelay = 1000;

DHT dht(PIN_DHT, DHTTYPE);

void setup() {
  Serial.begin(9600);

  pinMode(PIN_RED,   OUTPUT);
  pinMode(PIN_GREEN, OUTPUT);
  pinMode(PIN_BLUE,  OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_DHT, INPUT);

  dht.begin();
}

void loop() {
  humidity = dht.readHumidity();
  temperature = dht.readTemperature(); 
  time_now = millis();

  if (isnan(humidity) || isnan(temperature)) {
    statusAlarm = 1;
    Serial.println(F("Failed to read from DHT sensor!"));
    Serial.println(humidity);
    return;
  } else {
    statusActive = 1;
    Serial.println("Temp: " + String(temperature, 2) + "°C");
    Serial.println("Humidity: " + String(humidity, 1) + "%");
    Serial.println("---");
  }

  if(temperature < 10 || temperature > 27  || humidity < 10 || humidity > 70) { 
    statusAlarm = 1;
    statusActive = 0;
  } else {
    statusAlarm = 0;
    statusActive = 1;
  }

  if (statusAlarm) {
    Serial.println("It's panic time");
    statusActive = 0;
    statusConnecting = 0;
    ledSignal(1, 1, 0, 0, 500);
    tone(PIN_BUZZER, 2093, 1000);
    millisDelay(1000);
    tone(PIN_BUZZER, 4186, 1000);
    millisDelay(1000);
    noTone(PIN_BUZZER);
  }  

  if (statusActive) {
    Serial.println("Normal operating");
    statusConnecting = 0;
    statusAlarm = 0;
    ledSignal(1, 0, 1, 0, 0);
    millisDelay(1000);
  }
  
  if (statusConnecting) {
    Serial.println("Attempting to connect to server");
    statusActive = 0;
    statusAlarm = 0;
    ledSignal(1, 0, 0, 1, 1000);
    tone(PIN_BUZZER, 523, 250);
    millisDelay(250);
    tone(PIN_BUZZER, 1047, 250);
    millisDelay(250);
    tone(PIN_BUZZER, 2093, 250);
    millisDelay(500);
    noTone(PIN_BUZZER);
  }

  millisDelay(updateDelay);
}

void ledSignal(bool active, bool r, bool g, bool b, int delayMs) {
  turnOffAllLed();
  digitalWrite(11, r);
  digitalWrite(12, g);
  digitalWrite(13, b);
  if (delayMs != 0) {
    millisDelay(1000);
    turnOffAllLed();
    millisDelay(1000);
  }
}

void turnOffAllLed() {
  digitalWrite(11, 0);
  digitalWrite(12, 0);
  digitalWrite(13, 0);
}

void setUpdateDelay(int ms) {
  updateDelay = ms;
}

void millisDelay(long int delayTime){
  long int start_time = millis();
  while ( millis() - start_time < delayTime) ;
}