#include <Wire.h>              // Wire library (required for I2C devices)
#include <Adafruit_BMP280.h>   // Adafruit BMP280 sensor library
#include <ESP8266WiFi.h>       // ESP8266 WiFi library
#include <ESP8266WebServer.h>  // ESP8266 Web Server library
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

// Create instances
ESP8266WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

bool buzzerEnabled = true; // Default state

void handleRoot() {
  // Serve the HTML page with a loading indicator and a toggle button
  String html = "<html><head><title>IoT Vario</title>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; margin: 0; padding: 20px; display: flex; justify-content: center; align-items: center; flex-direction: column; height: 100vh; }";
  html += "#loading { font-size: 20px; color: #555; display: flex; flex-direction: column; align-items: center; }";
  html += "#data { display: none; display: flex; flex-direction: column; align-items: center; }";
  html += "#spinner { border: 16px solid #f3f3f3; /* Light grey */";
  html += "border-top: 16px solid #3498db; /* Blue */";
  html += "border-radius: 50%;";
  html += "width: 120px; height: 120px;";
  html += "animation: spin 2s linear infinite; }";
  html += "#toggle { margin-top: 20px; }";
  html += "@keyframes spin { 0% { transform: rotate(0deg); } 100% { transform: rotate(360deg); } }";
  html += "</style>";
  html += "<script>";
  html += "window.onload = function() {";
  html += "  var ws = new WebSocket('ws://' + location.hostname + ':81/');";
  html += "  var loading = document.getElementById('loading');";
  html += "  var dataDiv = document.getElementById('data');";
  html += "  var spinner = document.getElementById('spinner');";
  html += "  var toggleButton = document.getElementById('toggleButton');";
  html += "  var buzzerEnabled = true;";
  html += "  ws.onopen = function() {";
  html += "    console.log('WebSocket connection established');";
  html += "    loading.style.display = 'none';";
  html += "    dataDiv.style.display = 'flex';";
  html += "  };";
  html += "  ws.onmessage = function(event) {";
  html += "    var data = JSON.parse(event.data);";
  html += "    document.getElementById('altitude').innerText = 'Current Altitude: ' + data.altitude + ' m';";
  html += "    document.getElementById('verticalSpeed').innerText = 'Vertical Speed: ' + data.verticalSpeed + ' m/s';";
  html += "    buzzerEnabled = data.buzzerEnabled;";
  html += "    toggleButton.innerText = buzzerEnabled ? 'Disable Sound' : 'Enable Sound';";
  html += "  };";
  html += "  ws.onerror = function() {";
  html += "    console.log('WebSocket error');";
  html += "  };";
  html += "  toggleButton.onclick = function() {";
  html += "    var action = buzzerEnabled ? 'disable' : 'enable';";
  html += "    ws.send(JSON.stringify({ action: action }));";
  html += "  };";
  html += "};";
  html += "</script></head><body>";
  html += "<h1>Vario Data</h1>";
  html += "<div id='loading'>";
  html += "  <div id='spinner'></div>";
  html += "  Loading data...";
  html += "</div>";
  html += "<div id='data'>";
  html += "  <p id='altitude'>Current Altitude: </p>";
  html += "  <p id='verticalSpeed'>Vertical Speed: </p>";
  html += "  <button id='toggleButton'>Disable Sound</button>";
  html += "</div>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);
  delay(1000);

  // Set up the WiFi Access Point
  WiFi.softAP(apSSID, apPassword);
  Serial.println("Access Point started");

  // Initialize WebSocket and HTTP Server
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  server.on("/", HTTP_GET, handleRoot);
  server.begin();

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
  // Handle WebSocket and HTTP requests
  webSocket.loop();
  server.handleClient();

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
