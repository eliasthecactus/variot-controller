#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Adafruit_BMP280.h>
#include <Wire.h>
#include <QMC5883LCompass.h>  // Include the QMC5883L compass library
 
// Define the device name
const char* deviceName = "Vario-78318";
 
// Create BLE service UUID
#define SERVICE_UUID               "19B10000-E8F2-537E-4F6C-D104768A1214"
#define CHAR_UUID_ALTITUDE         "19B10001-E8F2-537E-4F6C-D104768A1214"
#define CHAR_UUID_TEMPERATURE      "19B10002-E8F2-537E-4F6C-D104768A1214"
#define CHAR_UUID_ANGLE            "19B10003-E8F2-537E-4F6C-D104768A1214"
#define CHAR_UUID_BUZZER_CONTROL   "19B10004-E8F2-537E-4F6C-D104768A1214"
 
BLEServer *pServer = NULL;
BLECharacteristic *altitudeCharacteristic = NULL;
BLECharacteristic *temperatureCharacteristic = NULL;
BLECharacteristic *angleCharacteristic = NULL;
BLECharacteristic *buzzerControlCharacteristic = NULL;
 
bool deviceConnected = false;
bool buzzerEnabled = true;
 
// BMP280 and QMC5883L objects
Adafruit_BMP280 bmp;
QMC5883LCompass compass;
 
// Define buzzer pin
const int buzzerPin = 25;
float previousAltitude = 0;
const float climbThreshold = 0.5;
const float descendThreshold = -1;
const float noiseThreshold = 0.1;
unsigned long lastBeepTime = 0;
const unsigned long beepInterval = 200;
 
// Function to read average altitude
float getAverageAltitude() {
  float firstAltitude = bmp.readAltitude(1013.25);
  delay(50);
  float secondAltitude = bmp.readAltitude(1013.25);
  return (firstAltitude + secondAltitude) / 2.0;
}
 
// Custom callback for write events on the buzzer control characteristic
class BuzzerControlCallback : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    uint8_t* data = pCharacteristic->getData();
    buzzerEnabled = (data[0] != 0);
    Serial.print("Buzzer enabled: ");
    Serial.println(buzzerEnabled ? "Yes" : "No");
  }
};
 
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) {
    deviceConnected = true;
    Serial.println("Device connected");
  }
 
  void onDisconnect(BLEServer *pServer) {
    deviceConnected = false;
    Serial.println("Device disconnected");
 
    // Restart advertising when disconnected
    BLEDevice::startAdvertising();
    Serial.println("Restarting advertising...");
  }
};
 
void setup() {
  Serial.begin(115200);
 
  // Initialize BMP280
  if (!bmp.begin(0x77)) {
    Serial.println("BMP280 initialization failed!");  // Troubleshooting: check BMP280 initialization
    while (1);  // Stop the program if the sensor fails to initialize
  } else {
    Serial.println("BMP280 initialized successfully");  // Success message for BMP280
  }
 
  // Initialize QMC5883L compass
  Wire.begin();
  compass.init();  // Ensure the compass initializes correctly
  Serial.println("Compass initialized");
 
  // Initialize BLE
  BLEDevice::init(deviceName);
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
 
  // Create BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);
 
  // Create BLE Characteristics
  altitudeCharacteristic = pService->createCharacteristic(
    CHAR_UUID_ALTITUDE,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
  );
 
  temperatureCharacteristic = pService->createCharacteristic(
    CHAR_UUID_TEMPERATURE,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
  );
 
  angleCharacteristic = pService->createCharacteristic(
    CHAR_UUID_ANGLE,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
  );
 
  buzzerControlCharacteristic = pService->createCharacteristic(
    CHAR_UUID_BUZZER_CONTROL,
    BLECharacteristic::PROPERTY_WRITE
  );
  buzzerControlCharacteristic->setCallbacks(new BuzzerControlCallback()); // Set callback for buzzer control
 
  // Start the service
  pService->start();
 
  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMaxPreferred(0x12);
  BLEDevice::startAdvertising();
  Serial.println("Waiting for a client connection...");
 
  // Beep once on startup
  tone(buzzerPin, 1000, 200);
}
 
void loop() {
  if (deviceConnected) {
    // Troubleshooting: Print sensor values to check if they are being read correctly
    float altitude = getAverageAltitude();
    float temperature = bmp.readTemperature();
 
    Serial.print("Altitude: ");
    Serial.println(altitude);  // Check if altitude readings are correct
    Serial.print("Temperature: ");
    Serial.println(temperature);  // Check if temperature readings are correct
 
    compass.read();
    byte azimuth = compass.getAzimuth();
    char direction[3];
    compass.getDirection(direction, azimuth);
    String angleStr = String(direction);
    Serial.print("Compass direction: ");
    Serial.println(angleStr);  // Check compass data
 
    // Convert sensor data to strings
    String altitudeStr = String(altitude);
    String temperatureStr = String(temperature);
 
    // Set values in BLE characteristics
    altitudeCharacteristic->setValue(altitudeStr.c_str());
    temperatureCharacteristic->setValue(temperatureStr.c_str());
    angleCharacteristic->setValue(angleStr.c_str());
 
    // Notify the client of characteristic changes
    altitudeCharacteristic->notify();
    temperatureCharacteristic->notify();
    angleCharacteristic->notify();
 
    // Determine altitude change rate
    float altitudeChange = altitude - previousAltitude;
    if (abs(altitudeChange) > noiseThreshold) {
      previousAltitude = altitude;
    } else {
      altitudeChange = 0;
    }
 
    // Troubleshooting: Print altitude change rate
    Serial.print("Altitude change rate: ");
    Serial.println(altitudeChange);
 
    // Buzzer behavior based on altitude change
    if (buzzerEnabled) {
      if (altitudeChange > climbThreshold) {
        if (millis() - lastBeepTime > beepInterval) {
          tone(buzzerPin, 2000, 100);  // Beep for climbing
          lastBeepTime = millis();
        }
      } else if (altitudeChange < descendThreshold) {
        if (millis() - lastBeepTime > beepInterval) {
          tone(buzzerPin, 500, 100);   // Beep for descending
          lastBeepTime = millis();
        }
      }
    }
 
    // Delay between updates
    delay(200);
  } else {
    // Troubleshooting: Check for disconnection
    Serial.println("Waiting for connection...");
    delay(1000);
  }
}