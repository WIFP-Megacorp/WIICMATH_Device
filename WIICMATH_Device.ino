#include "DHT.h" // DHT sensor library
#include "WiFiS3.h" //WiFi module library
//#include "EEPROM.h"

const uint8_t PIN_BUZZER  = 8;
const uint8_t PIN_RED   = 11;
const uint8_t PIN_GREEN = 12;
const uint8_t PIN_BLUE  = 13;
const uint8_t PIN_DHT = 2;

unsigned long time_now = 0;

//bool firstSetup = true; //First time launching device?
bool connected = false; //TBD: if connected to mr internet

//DHT sensor variables and setup
float dht_temperature;
float dht_humidity;
DHT dht(PIN_DHT, DHT11);

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

WiFiServer server(80);
const char* index_html = R"(
<!DOCTYPE html>
<html>
<head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body {
            background-color: aqua;
            font-family: sans-serif;
        }
    
        form {
            width:50%;
            margin: auto;
        }
        
        input {
            width:100%;
            margin:auto;
            font-size:20px;
            min-height:5vh;
        }
        
        h1 {
            text-align: center;
        }

        @media screen and (max-width: 480px) {
            form {
                width:90%;
            }
            input {
                width:100%;
            }
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>WIICMATH<br>WiFi Configuration</h1>
        <form action="/configure" method="POST">
            <label for="ssid">WiFi SSID:</label><br>
            <input type="text" id="ssid" name="ssid"><br><br>
            <label for="password">WiFi Password:</label><br>
            <input type="password" id="password" name="password"><br><br>
            <input type="submit" value="Submit">
        </form>
    </div>
</body>
</html>
)";

//EEPROM & WiFi connection variables (EEPROM not currently used)
char ssid[32];      // WiFi SSID can be up to 32 characters
char password[64];  // WiFi password can be up to 64 characters

void setup() {
  //Deciding what pins are output and input
  pinMode(PIN_RED,   OUTPUT);
  pinMode(PIN_GREEN, OUTPUT);
  pinMode(PIN_BLUE,  OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_DHT, INPUT);

  //Starting to read serial through USB, for debugging purposes
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  //Starts the setup procedure if device isn't connected to internet
  //Is here for potential future expansion of saving WiFi credentials to EEPROM
  if(!connected) {
    modeSetup();
  }

  //Starts to read measurements from the DHT-sensor
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
  /*
  * Device mode when everything is just fine
  */
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
  /*
  * Gateway to WiFi-setup
  */
  ledSignal(1, 0, 1, 0);

  while(!connected) {
    handleConfigure();
  }
}

void modeAlarm() {
  /*
  * Device panic mode, when something isn't right
  */
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

void ledSignal(bool r, bool g, bool b, int delayMs) {
  /*
  * Turns on, or blinks, the RGB LED
  *
  * @param r - Color value of red
  * @param g - Color value of green
  * @param b - Color value of blue
  * @param delayMs - How long LED should stay on and off
  */
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
  /*
  * Sets all LED pins to LOW (off)
  */
  digitalWrite(11, 0);
  digitalWrite(12, 0);
  digitalWrite(13, 0);
}

void setUpdateDelay(int ms) {
  /*
  * Changes the delay of each main loop execution
  *
  * @param ms - Delay in milliseconds
  */
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
  /*
  * Sets upper and lower thresholds for acceptable temperature and humidity readings
  *
  * @param minTemperature - lower temperature threshold in degrees celsius 
  * @param maxTemperature - upper temperature threshold in degrees celsius 
  * @param minHumidity - lower relative humidity threshold in percent
  * @param maxHumidity - upper relative humidity threshold in percent
  */
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

void handleConfigure() {
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // Don't continue
    while (true);
  }

  // Create an access point
  WiFi.beginAP(apSSID, apPassword);

  Serial.println("Waiting for a client to configure...");
  server.begin();

  // You're connected now, so print out the status
  printWiFiStatus();

  WiFiClient client = server.available();
  if (!client) {
    return;
  }

  if (client.connected()) {
    Serial.println("New client connected");

    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println();
    client.println(index_html);

    String request = client.readStringUntil('\r');
    if (request.indexOf("POST /configure") != -1) {
      // Read the POST data from the client
      String body = "";
      while (client.available()) {
        char c = client.read();
        body += c;
      }

      // Extract SSID and password from the POST data
      String receivedSSID = "";
      String receivedPassword = "";

      int ssidIndex = body.indexOf("ssid=");
      int passwordIndex = body.indexOf("password=");
      
      if (ssidIndex != -1 && passwordIndex != -1) {
        receivedSSID = body.substring(ssidIndex + 5, body.indexOf('&', ssidIndex));
        receivedPassword = body.substring(passwordIndex + 9);
        receivedSSID.trim();
        receivedPassword.trim();
      }

      // Save data to Preferences
      //saveCredentialsToPreferences(receivedSSID, receivedPassword);
      Serial.println("Received SSID: " + receivedSSID);
      Serial.println("Received password: " + receivedPassword);

      // Send a response back to the client
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: text/plain");
      client.println();
      client.println("WiFi credentials saved.");

      // Disable the access point and connect to the user-provided SSID
      WiFi.end();

      String ssidDecoded = urlDecode(receivedSSID);
      String passwordDecoded = urlDecode(receivedPassword);

      connectToUserSSID(ssidDecoded, passwordDecoded);
    }

    client.stop(); // Close the client connection
    Serial.println("Client disconnected");
  }
}

void connectToUserSSID(const String& receivedSSID, const String& receivedPassword) {
  /*
  * Connect to a 2G network using an SSID and password
  *
  *
  * @param receivedSSID - SSID to connect to
  * @param receivedPassword - Password of SSID to connect to
  */

  Serial.print("Attempting to connect to user-provided SSID: ");
  Serial.println(receivedSSID);

  int attempts = 0;
  WiFi.begin(receivedSSID.c_str(), receivedPassword.c_str());

  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(1000);
    Serial.println("Connecting...");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected to user-provided SSID");
    printCurrentNet();
    connected = true;
  } else {
    Serial.println("Failed to connect to user-provided SSID.");
    // You can add error handling here, such as retrying or other actions.
    WiFi.end();
    handleConfigure(); // Restart the configuration process
  }
}

void printCurrentNet() {
  /*
  * Prints the current SSID the device is connected to
  */
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
}

String urlDecode(const String& input) {
  /*
  * Used to remove ASCII encoding in URLs or data received from HTML forms
  *
  * @param input - URL to decode
  * @return - decoded URL
  */
  String output = input;
  output.replace("+", " ");
  output.replace("%20", " ");
  output.replace("%21", "!");
  output.replace("%23", "#");
  output.replace("%24", "$");
  output.replace("%26", "&");
  output.replace("%27", "'");
  output.replace("%28", "(");
  output.replace("%29", ")");
  output.replace("%2A", "*");
  output.replace("%2B", "+");
  output.replace("%2C", ",");
  output.replace("%2F", "/");
  output.replace("%3A", ":");
  output.replace("%3B", ";");
  output.replace("%3D", "=");
  output.replace("%3F", "?");
  output.replace("%40", "@");
  output.replace("%5B", "[");
  output.replace("%5D", "]");
  return output;
}