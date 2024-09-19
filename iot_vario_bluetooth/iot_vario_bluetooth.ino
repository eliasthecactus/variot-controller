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
BLECharacteristic buzzerControlCharacteristic("19B10004-E8F2-537E-4F6C-D104768A1214", BLEWrite, 1); // 1-byte write characteristic

// BMP280 and QMC5883L objects
Adafruit_BMP280 bmp;
QMC5883LCompass compass;

// Define buzzer pin
const int buzzerPin = 9; // Change to your buzzer pin
bool buzzerEnabled = true; // Track whether the buzzer is enabled or not

// Variables to manage altitude changes
float previousAltitude = 0;
const float climbThreshold = 0.5; // Climb threshold (in meters per second)
const float descendThreshold = -1; // Descend threshold (in meters per second)
const float noiseThreshold = 0.1;  // Ignore small changes within this range (noise)
unsigned long lastBeepTime = 0;
const unsigned long beepInterval = 200; // Time between beeps in milliseconds

float getAverageAltitude() {
    float firstAltitude = bmp.readAltitude(1013.25); // First altitude reading
    delay(50);  // Short delay between readings
    float secondAltitude = bmp.readAltitude(1013.25); // Second altitude reading
    return (firstAltitude + secondAltitude) / 2.0;  // Return the average of the two readings
}

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
  varioService.addCharacteristic(buzzerControlCharacteristic);
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
            // Check for buzzer control characteristic updates
            if (buzzerControlCharacteristic.written()) {
                buzzerEnabled = buzzerControlCharacteristic.value()[0] != 0;
                Serial.print("Buzzer enabled: ");
                Serial.println(buzzerEnabled ? "Yes" : "No");
            }

            // Read and average altitude measurements
            float altitude = getAverageAltitude();
            float temperature = bmp.readTemperature();

            // Read angle from QMC5883L (compass)
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

            // Determine altitude change rate
            float altitudeChange = altitude - previousAltitude;

            // Update previous altitude if the change is significant
            if (abs(altitudeChange) > noiseThreshold) {
                previousAltitude = altitude;
            } else {
                altitudeChange = 0;  // Ignore changes smaller than the noise threshold
            }

            // Beep based on altitude change if buzzer is enabled
            if (buzzerEnabled) {
                if (altitudeChange > climbThreshold) {
                    // Beep for climbing
                    if (millis() - lastBeepTime > beepInterval) {
                        tone(buzzerPin, 2000, 100); // 2kHz tone for 100ms for climbing
                        lastBeepTime = millis();
                    }
                } else if (altitudeChange < descendThreshold) {
                    // Beep for descending
                    if (millis() - lastBeepTime > beepInterval) {
                        tone(buzzerPin, 500, 100); // 500Hz tone for 100ms for descending
                        lastBeepTime = millis();
                    }
                }
            }

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