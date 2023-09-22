#include "DHT.h" // DHT sensor library

#define DHTTYPE DHT11

const uint8_t PIN_BUZZER  = 8;
const uint8_t PIN_RED   = 11;
const uint8_t PIN_GREEN = 12;
const uint8_t PIN_BLUE  = 13;
const uint8_t PIN_DHT = 2;

unsigned long time_now = 0;

bool firstSetup = false; //First time launching device?
bool connected = true; //TBD: if connected to mr internet

float dht_temperature;
float dht_humidity;

//some settings
bool MODULE_BUZZER = true; //Is the Buzzer module present?
bool MODULE_RGBLED = true; //Is the RGB LED module present?
bool setting_sound = false; //If true, buzzer makes sound
bool setting_light = true; //If true, RGB LED is used for status
int setting_minTemp = 10;
int setting_maxTemp = 27;
int setting_minHum = 30;
int setting_maxHum = 70;
int setting_updateDelay = 1000;

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
  dht_humidity = dht.readHumidity();
  dht_temperature = dht.readTemperature(); 
  time_now = millis();

  //If first time launching device, run setup
  if(firstSetup) {
    modeSetup();
  }
  
  readSensor();

  if(!connected) { //If not connected to internet
    modeConnecting();
  } else if(dht_temperature < setting_minTemp || dht_temperature > setting_maxTemp  || dht_humidity < setting_minHum || dht_humidity > setting_maxHum) { //If temp & humidity not within spec
    modeAlarm();
  } else {
    modeNormal();
  }

  millisDelay(setting_updateDelay);
}

//Modes
void modeNormal() {
  Serial.println("Normal operating");
  if(MODULE_RGBLED && setting_light) {
    ledSignal(1, 0, 1, 0, 0);
    millisDelay(1000);
  }
}

void modeConnecting() {
  Serial.println("Attempting to connect to server");
  if(MODULE_RGBLED && setting_light) {
    ledSignal(1, 0, 0, 1, 1000);
  }
  if(MODULE_BUZZER && setting_sound) {
    tone(PIN_BUZZER, 523, 250);
    millisDelay(250);
    tone(PIN_BUZZER, 1047, 250);
    millisDelay(250);
    tone(PIN_BUZZER, 2093, 250);
    millisDelay(500);
    noTone(PIN_BUZZER);
  }
}

void modeSetup() {
  //do some things if first launch
}

void modeAlarm() {
  Serial.println("It's panic time");
  if(MODULE_RGBLED && setting_light) {
    ledSignal(1, 1, 0, 0, 500);
  }
  if(MODULE_BUZZER && setting_sound) {
    tone(PIN_BUZZER, 2093, 1000);
    millisDelay(1000);
    tone(PIN_BUZZER, 4186, 1000);
    millisDelay(1000);
    noTone(PIN_BUZZER);
  }
}

//LED Handling
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

//Other
void setUpdateDelay(int ms) {
  setting_updateDelay = ms;
}

void readSensor() {
  if (isnan(dht_humidity) || isnan(dht_temperature)) {
    modeAlarm();
    Serial.println("---");
    Serial.println(F("Failed to read from DHT sensor!"));
    
    return;
  } else {
    Serial.println("---");
    Serial.println("Temp: " + String(dht_temperature, 2) + "°C");
    Serial.println("Humidity: " + String(dht_humidity, 1) + "%");
  }
}

void setLimits(int minTemperature, int maxTemperature, int minHumidity, int maxHumidity) {
  setting_minTemp = minTemperature;
  setting_maxTemp = maxTemperature;
  setting_minHum = minHumidity;
  setting_maxHum = maxHumidity;
}

void millisDelay(long int delayTime){
  long int start_time = millis();
  while ( millis() - start_time < delayTime) ;
}