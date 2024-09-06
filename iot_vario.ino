#include <Wire.h>              // Wire library (required for I2C devices)
#include <Adafruit_BMP280.h>   // Adafruit BMP280 sensor library
#include <ESP8266WiFi.h>       // ESP8266 WiFi library
#include <ESP8266mDNS.h>       // ESP8266 mDNS library
#include <WebSocketsServer.h>  // WebSocket Server library
#include <ArduinoJson.h>       // ArduinoJson library

// BMP280 settings
#define BMP280_I2C_ADDRESS 0x76  // Define device I2C address: 0x76 or 0x77
Adafruit_BMP280 bmp280;          // Initialize Adafruit BMP280 library

// Buzzer pin configuration
#define BUZZER_PIN D0  // Define the pin connected to the buzzer (e.g., D0 -> GPIO16)

// Variables to calculate vertical speed
float previousAltitude = 0;  // Variable to store previous altitude
unsigned long previousTime = 0;  // Variable to store previous time
const int interval = 800;  // Time interval in milliseconds

// WiFi credentials
const char* apSSID = "NodeMCU_Vario";  // AP SSID
const char* apPassword = "123456789";  // AP Password

// Create WebSocket instance
WebSocketsServer webSocket = WebSocketsServer(81);

bool buzzerEnabled = true; // Default state

void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);
  delay(1000);

  // Set up the WiFi Access Point
  WiFi.softAP(apSSID, apPassword);
  Serial.println("Access Point started");

  // Initialize mDNS
  if (!MDNS.begin("vario")) {
    Serial.println("Error starting mDNS");
    while (1);
  }
  Serial.println("mDNS started");

  // Add a service to mDNS
  MDNS.addService("http", "tcp", 80);
  Serial.println("mDNS service added");

  // Initialize WebSocket
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  // Initialize BMP280 sensor
  Wire.begin(D2, D1);
  if (!bmp280.begin(BMP280_I2C_ADDRESS)) {
    Serial.println("BMP280 Sensor Connection Error!");
    while (1);
  }

  Serial.println("BMP280 sensor initialized successfully.");

  // Initialize buzzer pin
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  // Initialize variables for altitude calculation
  previousAltitude = bmp280.readAltitude(1013.25);
  previousTime = millis();
}

void loop() {
  // Handle WebSocket communication
  webSocket.loop();

  // Handle mDNS
  MDNS.update();

  // Read current altitude from BMP280 sensor
  float currentAltitude = bmp280.readAltitude(1013.25);
  unsigned long currentTime = millis();
  float timeDifference = (currentTime - previousTime) / 1000.0;
  float verticalSpeed = (currentAltitude - previousAltitude) / timeDifference;

  // Output the current altitude and vertical speed to the Serial Monitor
  Serial.print("Current Altitude: ");
  Serial.print(currentAltitude);
  Serial.println(" m");

  Serial.print("Vertical Speed: ");
  Serial.print(verticalSpeed);
  Serial.println(" m/s");

  // Send the data to all connected WebSocket clients
  DynamicJsonDocument doc(1024);
  doc["altitude"] = currentAltitude;
  doc["verticalSpeed"] = verticalSpeed;
  doc["buzzerEnabled"] = buzzerEnabled;
  String json;
  serializeJson(doc, json);
  webSocket.broadcastTXT(json);

  // Control the buzzer based on vertical speed if enabled
  if (buzzerEnabled) {
    if (verticalSpeed > 0.5) {
      tone(BUZZER_PIN, 2000);
      delay(100);
      noTone(BUZZER_PIN);
      delay(200);
    } else if (verticalSpeed < -0.5) {
      tone(BUZZER_PIN, 1000);
      delay(300);
      noTone(BUZZER_PIN);
      delay(700);
    } else {
      noTone(BUZZER_PIN);
    }
  } else {
    noTone(BUZZER_PIN);
  }

  // Update previous altitude and time for the next calculation
  previousAltitude = currentAltitude;
  previousTime = currentTime;

  delay(interval);
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
  if (type == WStype_TEXT) {
    String message = String((char*)payload);
    if (message.startsWith("{")) {
      DynamicJsonDocument doc(1024);
      DeserializationError error = deserializeJson(doc, message);
      if (!error) {
        String action = doc["action"];
        if (action == "disable") {
          buzzerEnabled = false;
        } else if (action == "enable") {
          buzzerEnabled = true;
        }
      }
    }
  }
}