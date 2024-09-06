#include <ArduinoBLE.h>

const char* peripheralName = "Vario-78317";
BLEService varioService("19B10000-E8F2-537E-4F6C-D104768A1214");
BLECharacteristic sensorDataCharacteristic("19B10001-E8F2-537E-4F6C-D104768A1214", BLERead | BLENotify, 20);

void setup() {
  Serial.begin(9600);
  delay(1000); 

  if (!BLE.begin()) {
    Serial.println("BLE initialization failed!");
    while (1);
  }

  BLE.setLocalName(peripheralName);
  BLE.setAdvertisedService(varioService);
  varioService.addCharacteristic(sensorDataCharacteristic);
  BLE.addService(varioService);

   temperatureCharacteristic.setEventHandler(BLERead, sensorDataCharacteristic);

  BLE.advertise();

  Serial.println("BLE advertising...");
}

void loop() {
  BLEDevice central = BLE.central();
  if (central) {
    Serial.print("Connected to: ");
    Serial.println(central.address());

    while (central.connected()) {
      String sensorData = "Altitude: " + String(random(1000, 3000) / 10.0) + " m";
      sensorDataCharacteristic.setValue(sensorData.c_str());
      Serial.print("Sending data: ");
      Serial.println(sensorData);
      
      delay(1000); // Send data every second
    }

    Serial.print("Disconnected from: ");
    Serial.println(central.address());
  }
}