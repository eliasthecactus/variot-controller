#include <ArduinoBLE.h>
#include <Adafruit_BMP280.h>
#include <Wire.h>
#include <QMC5883LCompass.h>  // Include the QMC5883L compass library

// Define the device name
const char* deviceName = "Vario-78317";

// Create BLE service UUID
BLEService varioService("19B10000-E8F2-537E-4F6C-D104768A1214");

// Create BLE characteristics for altitude, pressure, angle, and temperature
BLECharacteristic altitudeCharacteristic("19B10001-E8F2-537E-4F6C-D104768A1214", BLERead | BLENotify, 20);
BLECharacteristic temperatureCharacteristic("19B10002-E8F2-537E-4F6C-D104768A1214", BLERead | BLENotify, 20);
BLECharacteristic angleCharacteristic("19B10003-E8F2-537E-4F6C-D104768A1214", BLERead | BLENotify, 20);

// BMP280 and QMC5883L objects
Adafruit_BMP280 bmp;
QMC5883LCompass compass;

// Define buzzer pin
const int buzzerPin = 9; // Change to your buzzer pin

void setup() {
  Serial.begin(9600);
  delay(1000);

  // Initialize BMP280
  if (!bmp.begin(0x77)) {
    Serial.println("BMP280 initialization failed!");
    while (1);
  }

  // Initialize QMC5883L (GY-273)
  Wire.begin();
  compass.init();

  // Initialize BLE
  if (!BLE.begin()) {
    Serial.println("BLE initialization failed!");
    while (1);
  }

  // Set the device name and add the service and characteristics
  BLE.setDeviceName(deviceName);
  BLE.setLocalName(deviceName);

  BLE.setAdvertisedService(varioService);
  varioService.addCharacteristic(altitudeCharacteristic);
  varioService.addCharacteristic(angleCharacteristic);
  varioService.addCharacteristic(temperatureCharacteristic);
  BLE.addService(varioService);

  // Start advertising
  BLE.advertise();

  Serial.println("BLE advertising...");

  // Beep once on startup
  tone(buzzerPin, 1000, 200); // 1kHz tone for 200ms
}

void loop() {
  BLEDevice central = BLE.central();
  if (central) {


    Serial.print("Connected to: ");
    Serial.println(central.address());

    while (central.connected()) {
      // Read sensor data from BMP280
      float altitude = bmp.readAltitude(1013.25); // Pressure at sea level in hPa
      float temperature = bmp.readTemperature();

      // Read angle from QMC5883L
      compass.read();
      byte azimuth = compass.getAzimuth();
      char direction[3];
      compass.getDirection(direction, azimuth);
      String angleStr = String(direction);

      // Convert values to strings for BLE characteristics
      String altitudeStr = String(altitude);
      String temperatureStr = String(temperature);

      // Update BLE characteristics with sensor data
      altitudeCharacteristic.setValue(altitudeStr.c_str());
      temperatureCharacteristic.setValue(temperatureStr.c_str());
      angleCharacteristic.setValue(angleStr.c_str());

      // Beep once when data is read
      tone(buzzerPin, 1000, 200); // 1kHz tone for 200ms

      // Debugging output
      Serial.println("Sending data:");
      Serial.println("Altitude: " + altitudeStr);
      Serial.println("Angle: " + angleStr);
      Serial.println("Temperature: " + temperatureStr);

      delay(200);  // Send data every 200 ms
    }

    Serial.print("Disconnected from: ");
    Serial.println(central.address());
  }
}