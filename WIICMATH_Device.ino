#include "DHT.h" // DHT sensor library
#include "WiFiS3.h" //WiFi module library
#include "EEPROM.h"

#define DHTTYPE DHT11

const uint8_t PIN_BUZZER  = 8;
const uint8_t PIN_RED   = 11;
const uint8_t PIN_GREEN = 12;
const uint8_t PIN_BLUE  = 13;
const uint8_t PIN_DHT = 2;

unsigned long time_now = 0;

//bool firstSetup = true; //First time launching device?
bool connected = false; //TBD: if connected to mr internet

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

//Access point setup
const char* apSSID = "WIICMATH";    // SSID of the access point
const char* apPassword = "password";  // Password for the access point
int status = WL_IDLE_STATUS;
//WiFiServer server(80);
WiFiServer server(80);
const char* index_html = R"(
<!DOCTYPE html>
<html>
<body>
  <h2>WiFi Configuration</h2>
  <form action="/configure" method="POST">
    <label for="ssid">WiFi SSID:</label>
    <input type="text" id="ssid" name="ssid"><br><br>
    <label for="password">WiFi Password:</label>
    <input type="password" id="password" name="password"><br><br>
    <input type="submit" value="Submit">
  </form>
</body>
</html>
)";

//EEPROM variables
char ssid[32];      // WiFi SSID can be up to 32 characters
char password[64];  // WiFi password can be up to 64 characters

DHT dht(PIN_DHT, DHTTYPE);

void setup() {
  pinMode(PIN_RED,   OUTPUT);
  pinMode(PIN_GREEN, OUTPUT);
  pinMode(PIN_BLUE,  OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_DHT, INPUT);

  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  if(!connected) {
    modeSetup();
  }

  dht.begin();
}

void loop() {
  dht_humidity = dht.readHumidity();
  dht_temperature = dht.readTemperature(); 
  time_now = millis();

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
    ledSignal(0, 1, 0, 0);
    millisDelay(1000);
  }
}

void modeConnecting() {
  Serial.println("Attempting to connect to server");
  if(MODULE_RGBLED && setting_light) {
    ledSignal(0, 0, 1, 1000);
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
  ledSignal(1, 0, 1, 0);

  if (WiFi.status() == WL_NO_MODULE) {
      Serial.println("Communication with WiFi module failed!");
      // don't continue
      while (true);
    }

  WiFi.beginAP(apSSID, apPassword);

  Serial.println("Waiting 5 seconds to set up Access Point");
  millisDelay(5000);

  server.begin();

  // you're connected now, so print out the status
  printWiFiStatus();

  while(!connected) {
    WiFiClient client = server.available();
    if (client) {
      Serial.println("New client connected");
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: text/html");
      client.println();
      client.println(index_html);
      client.stop();
      Serial.println("Client disconnected");
    }
  }
}

void modeAlarm() {
  Serial.println("It's panic time");
  if(MODULE_RGBLED && setting_light) {
    ledSignal(1, 0, 0, 500);
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
void ledSignal(bool r, bool g, bool b, int delayMs) {
  turnOffAllLed();
  digitalWrite(11, r);
  digitalWrite(12, g);
  digitalWrite(13, b);
  if (delayMs != 0) {
    millisDelay(delayMs);
    turnOffAllLed();
    millisDelay(delayMs);
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
  }
  //glenn var her
  Serial.println("---");
  Serial.println("Temp: " + String(dht_temperature, 2) + "°C");
  Serial.println("Humidity: " + String(dht_humidity, 1) + "%");
  
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

void printWiFiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print where to go in a browser:
  Serial.print("To see this page in action, open a browser to http://");
  Serial.println(ip);

}