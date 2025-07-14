#include <Arduino.h>
#include "BluetoothSerial.h"

BluetoothSerial SerialBT;

// Pines de los LEDs
const int LED1_PIN = 19;
const int LED2_PIN = 21;
const int LED3_PIN = 22;
const int LED4_PIN = 23;

int currentLED = 0;
unsigned long lastReceived = 0;
const unsigned long TIMEOUT = 5000;

void turnOffAllLEDs() {
  digitalWrite(LED1_PIN, LOW);
  digitalWrite(LED2_PIN, LOW);
  digitalWrite(LED3_PIN, LOW);
  digitalWrite(LED4_PIN, LOW);
}

void processCommand(int cmd) {
  turnOffAllLEDs();
  switch (cmd) {
    case 1: digitalWrite(LED1_PIN, HIGH); break;
    case 2: digitalWrite(LED2_PIN, HIGH); break;
    case 3: digitalWrite(LED3_PIN, HIGH); break;
    case 4: digitalWrite(LED4_PIN, HIGH); break;
  }
  currentLED = cmd;
}

void setup() {
  Serial.begin(115200);
  pinMode(LED1_PIN, OUTPUT);
  pinMode(LED2_PIN, OUTPUT);
  pinMode(LED3_PIN, OUTPUT);
  pinMode(LED4_PIN, OUTPUT);
  turnOffAllLEDs();

  SerialBT.begin("ESP32_LED_Control");
  Serial.println("Bluetooth listo como 'ESP32_LED_Control'");
}

void loop() {
  if (SerialBT.available() >= 2) {
    int8_t x = SerialBT.read();
    int8_t y = SerialBT.read();
    lastReceived = millis();

    int cmd = 0;
    if (y < -20 && x < -20) cmd = 1;
    else if (y < -20 && x > 20) cmd = 2;
    else if (y > 20 && x < -20) cmd = 3;
    else if (y > 20 && x > 20) cmd = 4;

    processCommand(cmd);
    Serial.printf("X: %d Y: %d → LED %d\n", x, y, cmd);
  }

  if (millis() - lastReceived > TIMEOUT && currentLED != 0) {
    turnOffAllLEDs();
    currentLED = 0;
    Serial.println("Timeout, apagando LEDs.");
  }

  delay(20);
}
