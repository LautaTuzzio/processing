#include <Arduino.h>
#include "BluetoothSerial.h"

BluetoothSerial SerialBT;

// Pines de los LEDs
const int BOTTOM_PIN = 19;
const int LEFT_PIN = 21;
const int RIGHT_PIN = 22;
const int TOP_PIN = 23;

int currentLED = 0;
unsigned long lastReceived = 0;
const unsigned long TIMEOUT = 5000;

void turnOffAllLEDs() {
  digitalWrite(TOP_PIN, LOW);
  digitalWrite(RIGHT_PIN, LOW);
  digitalWrite(BOTTOM_PIN, LOW);
  digitalWrite(LEFT_PIN, LOW);
}

void processCommand(int cmd) {
  turnOffAllLEDs();
  
  // Handle directions
  if (cmd == 1) { // Top-Left
    digitalWrite(TOP_PIN, HIGH);
    digitalWrite(LEFT_PIN, HIGH);
  } else if (cmd == 2) { // Top-Right
    digitalWrite(TOP_PIN, HIGH);
    digitalWrite(RIGHT_PIN, HIGH);
  } else if (cmd == 3) { // Bottom-Left
    digitalWrite(BOTTOM_PIN, HIGH);
    digitalWrite(LEFT_PIN, HIGH);
  } else if (cmd == 4) { // Bottom-Right
    digitalWrite(BOTTOM_PIN, HIGH);
    digitalWrite(RIGHT_PIN, HIGH);
  } else if (cmd == 5) { // Top
    digitalWrite(TOP_PIN, HIGH);
  } else if (cmd == 6) { // Right
    digitalWrite(RIGHT_PIN, HIGH);
  } else if (cmd == 7) { // Bottom
    digitalWrite(BOTTOM_PIN, HIGH);
  } else if (cmd == 8) { // Left
    digitalWrite(LEFT_PIN, HIGH);
  }
  
  currentLED = cmd;
}

void setup() {
  Serial.begin(115200);
  pinMode(TOP_PIN, OUTPUT);
  pinMode(RIGHT_PIN, OUTPUT);
  pinMode(BOTTOM_PIN, OUTPUT);
  pinMode(LEFT_PIN, OUTPUT);
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
    
    // Simple threshold-based direction detection
    const int threshold = 20;  // Adjust this value to change sensitivity
    
    bool top = y < -threshold;
    bool bottom = y > threshold;
    bool left = x < -threshold;
    bool right = x > threshold;
    
    // Diagonal directions (combine two directions)
    if (top && left) cmd = 1;        // Top-Left
    else if (top && right) cmd = 2;  // Top-Right
    else if (bottom && left) cmd = 3; // Bottom-Left
    else if (bottom && right) cmd = 4; // Bottom-Right
    // Straight directions
    else if (top) cmd = 5;    // Top
    else if (right) cmd = 6;  // Right
    else if (bottom) cmd = 7; // Bottom
    else if (left) cmd = 8;   // Left

    processCommand(cmd);
    const char* direction = "";
  switch(cmd) {
    case 1: direction = "Top-Left"; break;
    case 2: direction = "Top-Right"; break;
    case 3: direction = "Bottom-Left"; break;
    case 4: direction = "Bottom-Right"; break;
    case 5: direction = "Top"; break;
    case 6: direction = "Right"; break;
    case 7: direction = "Bottom"; break;
    case 8: direction = "Left"; break;
    default: direction = "Center";
  }
  Serial.printf("X: %d Y: %d → %s (LED %d)\n", x, y, direction, cmd);
  }

  if (millis() - lastReceived > TIMEOUT && currentLED != 0) {
    turnOffAllLEDs();
    currentLED = 0;
    Serial.println("Timeout, apagando LEDs.");
  }

  delay(20);
}
