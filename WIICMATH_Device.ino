#include "DHT.h"     // Include the DHT sensor library
#include "WiFiS3.h"  // Include the WiFi module library
//#include "EEPROM.h" // Uncomment this line if you plan to use EEPROM
#include "webpage.h"  // Include the webpage header file

const uint8_t PIN_BUZZER = 8;  // Pin for the buzzer module voltage
const uint8_t PIN_RED = 10;    // Pin for the "red" LED voltage
const uint8_t PIN_GREEN = 11;  // Pin for the "green"LED voltage
const uint8_t PIN_BLUE = 12;   // Pin for the "blue" LED voltage
const uint8_t PIN_DHT = 2;     // Pin for the DHT sensor voltage

unsigned long time_now = 0;
unsigned long prev_blink = 0;
unsigned long previous_bip = 0;
int led_state = 0;
int buzzer_state = 0;

//DHT sensor variables and setup
float dht_temperature;
float dht_humidity;
DHT dht(PIN_DHT, DHT11);

// Default module and settings flags
bool MODULE_BUZZER = true;       // Indicates if the Buzzer module is present
bool MODULE_RGBLED = true;       // Indicates if the RGB LED module is present
bool setting_sound = false;      // If true, the buzzer makes sound
bool setting_light = true;       // If true, the RGB LED is used for status
int setting_minTemp = 10;        // Minimum acceptable temperature
int setting_maxTemp = 27;        // Maximum acceptable temperature
int setting_minHum = 30;         // Minimum acceptable humidity
int setting_maxHum = 70;         // Maximum acceptable humidity
int setting_updateDelay = 1000;  // Delay between main loop executions

//Access point setup
const char* apSSID = "WIICMATH";      // SSID of the access point
const char* apPassword = "password";  // Password for the access point
int status = WL_IDLE_STATUS;

WiFiServer server(80);

//EEPROM & WiFi connection variables (EEPROM not currently used)
char ssid[32];      // WiFi SSID can be up to 32 characters
char password[64];  // WiFi password can be up to 64 characters
String receivedSSID = "";
String receivedPassword = "";



void setup() {
  // Configure pin modes
  pinMode(PIN_RED, OUTPUT);
  pinMode(PIN_GREEN, OUTPUT);
  pinMode(PIN_BLUE, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_DHT, INPUT);

  // Initialize serial communication for debugging
  Serial.begin(9600);
  while (!Serial) {
    ;  // wait for serial port to connect. Needed for native USB port only
  }

  // Check if WiFi module works
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    while (true)
      ;  // Don't continue
  }

  // Check WiFi module firmware version
  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }

  // Start the setup procedure if the device isn't connected to the internet
  modeConnecting();

  // Start reading measurements from the DHT sensor
  dht.begin();
}


void loop() {
  // Read DHT sensor values & set current time
  dht_humidity = dht.readHumidity();
  dht_temperature = dht.readTemperature();
  time_now = millis();


  // Process sensor data and device modes
  Serial.println("---");
  if (isnan(dht_humidity) || isnan(dht_temperature)) {
    modeAlarm();
    Serial.println(F("Failed to read from DHT sensor!"));
    delay(500);
    return;
  }
  Serial.println("Temp: " + String(dht_temperature, 2) + "°C");
  Serial.println("Humidity: " + String(dht_humidity, 1) + "%");


  if (WiFi.status() != WL_CONNECTED) {
    // If lost connection to the internet, attempt to connect
    // Arduino takes a few seconds to recognice a disconnect
    modeConnecting();
  } else if (dht_temperature < setting_minTemp || dht_temperature > setting_maxTemp || dht_humidity < setting_minHum || dht_humidity > setting_maxHum) {
    modeAlarm();  // If sensor readings are out of spec, activate alarm mode
  } else {
    modeNormal();  // Device operates normally when all conditions are met
  }


  while (millis() < time_now + setting_updateDelay)
    ;  // Delay before next loop execution
}

/**
 * Device mode when everything is normal.
 */
void modeNormal() {
  Serial.println("Normal operating");
  ledSignal(0, 1, 0, 0);
}

/**
 * Handling logic flow when device isn't connected to the Internet
 */
void modeConnecting() {
  // if no connected to mr internet!
  // (Potential future expansion for saving WiFi credentials to EEPROM)

  status = WL_IDLE_STATUS;
  int attempt = 0;

  ledSignal(1, 0, 1, 0);  // Change signal light

  // TODO: Connect using stored SSID & password first, if found
  // eg func named eepromReadWifiSettings() then call connectToSSID()
  while (status != WL_CONNECTED && attempt < 5) {
    status = connectToSSID(receivedSSID, receivedPassword);
    attempt++;
  }

  /* Glenn - 28.09.2023
    Suggestion, add manual option/button to enter setup through access point.
    That way, in case of large blackout, the device will reconnect on its own using stored ssid.
    In the case that a new network is needed, the manual setup can be initiated.
  */

  while (status != WL_CONNECTED) {
    // If no SSID, or connection error, open web client to request SSID
    Serial.println("Requesting new SSID and password");
    Serial.println();

    openWebInterface();     // Setup access point
    bool got_ssid = false;  // Variable for if an ssid is found

    while (!got_ssid) {
      got_ssid = loopWebInterface();
      Serial.println(got_ssid);
      Serial.println("Loop");

      ledSignal(0, 0, 1, 2000);  // Blinking led

      if (MODULE_BUZZER && setting_sound) {
        //Todo: firstLaunch variable, if firstLaunch don't BUZZ!!!!!!!!!!

        if (time_now - previous_bip >= 250) {
          previous_bip = time_now;

          if (buzzer_state == 0) {
            buzzer_state = 1;
            tone(PIN_BUZZER, 523, 250);
          } else if (buzzer_state == 1) {
            buzzer_state == 2;
            tone(PIN_BUZZER, 1047, 250);
          } else if (buzzer_state == 2) {
            buzzer_state = 3;
            tone(PIN_BUZZER, 2093, 250);
          } else {
            buzzer_state = 0;
            noTone(PIN_BUZZER);
          }
        }
      }
      delay(1500);
    }
    Serial.println("Got SSID. Attemting to connect to it");
    WiFi.end();
    Serial.println("Wifi ended.");

    // Connect to given SSID
    attempt = 0;

    ledSignal(0, 0, 1, 0);  // Solid led

    while (status != WL_CONNECTED && attempt < 5) {
      status = connectToSSID(receivedSSID, receivedPassword);
      attempt++;
    }
  }
  //after out of loop, store newest credentials to eeprom
  //eg fun named eepromWriteWifiSettings()
}

/**
 * Device panic mode when something isn't right.
 */
void modeAlarm() {
  Serial.println("It's panic time");
  ledSignal(1, 0, 0, 500);

  if (MODULE_BUZZER && setting_sound) {
    if (time_now - previous_bip >= 1000) {
      previous_bip = time_now;

      if (buzzer_state == 0) {
        buzzer_state = 1;
        tone(PIN_BUZZER, 2093, 1000);
      } else if (buzzer_state == 1) {
        buzzer_state == 2;
        tone(PIN_BUZZER, 4186, 1000);
      } else {
        buzzer_state = 0;
        noTone(PIN_BUZZER);
      }
    }
  }
}

/**
 * Turns on, or blinks, the RGB LED.
 *
 * @param r - Color value of red
 * @param g - Color value of green
 * @param b - Color value of blue
 * @param intervalMS - How long LED should stay on and off
 */
void ledSignal(bool r, bool g, bool b, int intervalMS) {
  if (MODULE_RGBLED && setting_light) {
    if (intervalMS != 0) {
      if (time_now - prev_blink >= intervalMS) {
        prev_blink = time_now;

        if (led_state == 0) {
          led_state = 1;
          digitalWrite(PIN_RED, r);
          digitalWrite(PIN_GREEN, g);
          digitalWrite(PIN_BLUE, b);
        } else {
          led_state = 0;
          turnOffAllLed();
        }
      }
    } else {
      //turnOffAllLed();
      digitalWrite(PIN_RED, r);
      digitalWrite(PIN_GREEN, g);
      digitalWrite(PIN_BLUE, b);
    }
  }
}

/**
 * Sets all LED pins to LOW (off).
 */
void turnOffAllLed() {
  digitalWrite(PIN_RED, 0);
  digitalWrite(PIN_GREEN, 0);
  digitalWrite(PIN_BLUE, 0);
}

/**
 * Changes the delay of each main loop execution.
 *
 * @param ms - Delay in milliseconds
 */
void setUpdateDelay(int ms) {
  if (ms >= 1000) setting_updateDelay = ms;
  else setting_updateDelay = 1000;
}


/**
 * Sets upper and lower thresholds for acceptable temperature and humidity readings.
 *
 * @param minTemperature - lower temperature threshold in degrees Celsius
 * @param maxTemperature - upper temperature threshold in degrees Celsius
 * @param minHumidity - lower relative humidity threshold in percent
 * @param maxHumidity - upper relative humidity threshold in percent
 */
void setThresholds(int minTemperature, int maxTemperature, int minHumidity, int maxHumidity) {
  // If the given value is below or higher than the upper and lower limits of the dht11 sensor, the value is set to the sensor limit.

  if (minTemperature >= 0) setting_minTemp = minTemperature;
  else setting_minTemp = 0;

  if (maxTemperature <= 50) setting_maxTemp = maxTemperature;
  else setting_maxTemp = 50;

  if (minHumidity >= 20) setting_minHum = minHumidity;
  else setting_minHum = 20;

  if (maxHumidity <= 80) setting_maxHum = maxHumidity;
  else setting_maxHum = 80;
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
void openWebInterface() {
  // Create an access point
  WiFi.end();
  Serial.println("Wifi ended.");
  delay(2000);

  Serial.println("WifiAP begun.");
  status = WiFi.beginAP(apSSID, apPassword);

  if (status != WL_AP_LISTENING) {
    Serial.println("Creating access point failed");
    // don't continue
    while (true) ledSignal(1, 0, 1, 500);
  }
  Serial.println("Waiting for a client to configure...");
  server.begin();

  // You're connected now, so print out the status
  printWiFiStatus();
  // End of setup of access point
}

/**
 * Listens to access point and 
 *
 * @return - If an ssid was recieved
 */
bool loopWebInterface() {
  bool var = false;

  // compare the previous status to the current status
  if (status != WiFi.status()) {
    // it has changed update the variable
    status = WiFi.status();
    Serial.println(status);

    if (status == WL_AP_CONNECTED) {
      // a device has connected to the AP
      Serial.println("Device connected to AP");
    } else {
      // a device has disconnected from the AP, and we are back in listening mode
      Serial.println("Device disconnected from AP");
    }
  }
  Serial.println("I got this far.");

  WiFiClient client = server.available();
  // Glenn - 01.10.2023
  // Causes problems second time AP is created.
  // Unknown reason why.


  // Connect to client
  if (!client) {
    Serial.println("Found no clients. Returning");
    return var;
  }  //If no client was found

  Serial.println("New client connected");
  String request = "";
  while (client.connected()) {
    delayMicroseconds(10);     // To give time to connect
    if (client.available()) {  // if there's bytes to read from the client,
      char c = client.read();  // read a byte, then
      if (c == '\n') {         // if the byte is a newline character

        // if the current line is blank, you got two newline characters in a row.
        // that's the end of the client HTTP request, so send a response:
        if (request.length() == 0) {
          // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
          // and a content-type so the client knows what's coming, then a blank line:
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println();
          client.println(index_html);
          // The HTTP response ends with another blank line:
          client.println();

          break;  // break out of the while loop:
        } else {  // if you got a newline, then clear request:
          request = "";
        }
      } else if (c != '\r') {  // if you got anything else but a carriage return character,
        request += c;          // add it to the end of the request
      }

      if (request.endsWith("POST /configure")) {
        // Read the POST data from the client
        // Extract SSID and password from the POST data
        request = "";
        while (client.available()) {
          char c = client.read();
          request += c;
        }

        int ssidIndex = request.indexOf("ssid=");
        int passwordIndex = request.indexOf("password=");

        if (ssidIndex != -1 && passwordIndex != -1) {
          receivedSSID = request.substring(ssidIndex + 5, request.indexOf('&', ssidIndex));
          receivedPassword = request.substring(passwordIndex + 9);
          receivedSSID.trim();
          receivedPassword.trim();
          receivedSSID = urlDecode(receivedSSID);
          receivedPassword = urlDecode(receivedPassword);
        }
        Serial.println("Received SSID: " + receivedSSID);
        Serial.println("Received password: " + receivedPassword);

        // Send a response back to the client
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: text/plain");
        client.println();
        client.println("WiFi credentials saved.");


        var = true;  // SSID has been given, so true
        break;       // Break free from the loop
      }
    }
  }
  // close the connection:
  client.stop();
  Serial.println("client disconnected");
  return var;
}
// End of client connection


/**
 * Connect to a 2G network using an SSID and password
 *
 * @param receivedSSID - SSID to connect to
 * @param receivedPassword - Password of SSID to connect to
 * @return - WiFi - status
 */
int connectToSSID(String receivedSSID, String receivedPassword) {
  Serial.print("Attempting to connect to SSID: ");
  Serial.println(receivedSSID);

  Serial.println("Wifi begun.");
  status = WiFi.begin(receivedSSID.c_str(), receivedPassword.c_str());

  if (status == WL_CONNECTED) {
    Serial.println("Connected to SSID");
    printCurrentNet();
  } else {
    Serial.println("Failed to connect to SSID.");
  }
  return status;
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
  output.replace("%92", "'");
  return output;
}