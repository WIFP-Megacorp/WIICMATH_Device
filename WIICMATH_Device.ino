#include "DHT.h" // Include the DHT sensor library
#include "WiFiS3.h" // Include the WiFi module library
//#include "EEPROM.h" // Uncomment this line if you plan to use EEPROM
#include "webpage.h" // Include the webpage header file

const uint8_t PIN_BUZZER  = 8; // Pin for the buzzer module voltage
const uint8_t PIN_RED   = 11; // Pin for the "red" LED voltage
const uint8_t PIN_GREEN = 12; // Pin for the "green"LED voltage
const uint8_t PIN_BLUE  = 13; // Pin for the "blue" LED voltage
const uint8_t PIN_DHT = 2;    // Pin for the DHT sensor voltage

unsigned long time_now = 0;

//DHT sensor variables and setup
float dht_temperature;
float dht_humidity;
DHT dht(PIN_DHT, DHT11);

// Default module and settings flags
bool MODULE_BUZZER = true; // Indicates if the Buzzer module is present
bool MODULE_RGBLED = true; // Indicates if the RGB LED module is present
bool setting_sound = false; // If true, the buzzer makes sound
bool setting_light = true; // If true, the RGB LED is used for status
int setting_minTemp = 10; // Minimum acceptable temperature
int setting_maxTemp = 27; // Maximum acceptable temperature
int setting_minHum = 30; // Minimum acceptable humidity
int setting_maxHum = 70; // Maximum acceptable humidity
int setting_updateDelay = 1000; // Delay between main loop executions

//Access point setup
const char* apSSID = "WIICMATH";    // SSID of the access point
const char* apPassword = "password";  // Password for the access point
int status = WL_IDLE_STATUS;

WiFiServer server(80);

//EEPROM & WiFi connection variables (EEPROM not currently used)
char ssid[32];      // WiFi SSID can be up to 32 characters
char password[64];  // WiFi password can be up to 64 characters

void setup() {
  // Configure pin modes
  pinMode(PIN_RED,   OUTPUT);
  pinMode(PIN_GREEN, OUTPUT);
  pinMode(PIN_BLUE,  OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_DHT, INPUT);

  // Initialize serial communication for debugging
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  // Check if WiFi module works
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // Don't continue
    while (true);
  }

  // Check WiFi module firmware version
  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }

  // Start the setup procedure if the device isn't connected to the internet
  // (Potential future expansion for saving WiFi credentials to EEPROM)
  if(WiFi.status() != WL_CONNECTED) {
    modeSetup(); // if no connected to mr internet!
  }

  // Start reading measurements from the DHT sensor
  dht.begin();
}

void loop() {
  // Read DHT sensor values & set current time
  dht_humidity = dht.readHumidity();
  dht_temperature = dht.readTemperature(); 
  time_now = millis();

  // Process sensor data and device modes
  readSensor();

  if(WiFi.status() != WL_CONNECTED) {
    modeConnecting(); // If not connected to the internet, attempt to connect
  } else if(dht_temperature < setting_minTemp || dht_temperature > setting_maxTemp  || dht_humidity < setting_minHum || dht_humidity > setting_maxHum) {
    modeAlarm(); // If sensor readings are out of spec, activate alarm mode
  } else {
    modeNormal(); // Device operates normally when all conditions are met
  }

  millisDelay(setting_updateDelay); // Delay before next loop execution
}

/**
 * Device mode when everything is normal.
 */
void modeNormal() {
  Serial.println("Normal operating");
  if(MODULE_RGBLED && setting_light) {
    ledSignal(0, 1, 0, 0);
    millisDelay(1000);
  }
}

/**
 * Currently unused
 */
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

/**
 * Gateway to WiFi setup mode.
 */
void modeSetup() {
  ledSignal(1, 0, 1, 0);

  while(WiFi.status() != WL_CONNECTED) {
    handleConfigure();
  }
}

/**
 * Device panic mode when something isn't right.
 */
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

/**
 * Turns on, or blinks, the RGB LED.
 *
 * @param r - Color value of red
 * @param g - Color value of green
 * @param b - Color value of blue
 * @param delayMs - How long LED should stay on and off
 */
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

/**
 * Sets all LED pins to LOW (off).
 */
void turnOffAllLed() {
  digitalWrite(11, 0);
  digitalWrite(12, 0);
  digitalWrite(13, 0);
}

/**
 * Changes the delay of each main loop execution.
 *
 * @param ms - Delay in milliseconds
 */
void setUpdateDelay(int ms) {
  setting_updateDelay = ms;
}

/**
 * Read sensor data from the DHT sensor and handle errors.
 */
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

/**
 * Sets upper and lower thresholds for acceptable temperature and humidity readings.
 *
 * @param minTemperature - lower temperature threshold in degrees Celsius
 * @param maxTemperature - upper temperature threshold in degrees Celsius
 * @param minHumidity - lower relative humidity threshold in percent
 * @param maxHumidity - upper relative humidity threshold in percent
 */
void setLimits(int minTemperature, int maxTemperature, int minHumidity, int maxHumidity) {
  setting_minTemp = minTemperature;
  setting_maxTemp = maxTemperature;
  setting_minHum = minHumidity;
  setting_maxHum = maxHumidity;
}

/**
 * Delay for a specified number of milliseconds using millis().
 *
 * @param delayTime - Delay time in milliseconds
 */
void millisDelay(long int delayTime){
  long int start_time = millis();
  while ( millis() - start_time < delayTime) ;
}

/**
 * Print WiFi connection status and IP address.
 */
void printWiFiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.print(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print(" | IP Address: ");
  Serial.print(ip);

  Serial.println("");
}

/**
 * Handle the configuration of WiFi credentials via a web interface.
 */
void handleConfigure() {
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

/**
 * Connect to a 2G network using an SSID and password
 *
 *
 * @param receivedSSID - SSID to connect to
 * @param receivedPassword - Password of SSID to connect to
 */
void connectToUserSSID(const String& receivedSSID, const String& receivedPassword) {
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
  } else {
    Serial.println("Failed to connect to user-provided SSID.");
    // You can add error handling here, such as retrying or other actions.
    WiFi.end();
    handleConfigure(); // Restart the configuration process
  }
}

 /*
 * Prints the current SSID the device is connected to
 */
void printCurrentNet() {
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
}

/**
 * Used to remove ASCII encoding in URLs or data received from HTML forms
 *
 * @param input - URL to decode
 * @return - decoded URL
 */
String urlDecode(const String& input) {
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