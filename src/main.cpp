#include <Arduino.h>
#include "BluetoothSerial.h" // Add this line

BluetoothSerial SerialBT;    // Create a BluetoothSerial object

const int ledPin = 4;
int myFunction(int, int);

void setup() {
  pinMode(ledPin, OUTPUT);
  SerialBT.begin("ESP32_BT"); // Bluetooth device name
}

void loop() {
  if (SerialBT.available()) {
    char c = SerialBT.read();
    if (c == '1') {
      digitalWrite(ledPin, HIGH);
    } else if (c == '0') {
      digitalWrite(ledPin, LOW);
    }
  }
  delay(20);
}