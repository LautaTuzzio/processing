#include "BluetoothSerial.h"
#include <Arduino.h>

// Pines para los LEDs
const int ledPin1 = 4;  // Pin para el LED 1
const int ledPin2 = 5;  // Pin para el LED 2
const int ledPin3 = 18; // Pin para el LED 3
const int ledPin4 = 19; // Pin para el LED 4

// Objeto para la comunicación Bluetooth
BluetoothSerial SerialBT;

// Variables para almacenar las coordenadas
int16_t x = 0;
int16_t y = 0;

void setup() {
  // Inicializar pines de los LEDs como salidas
  pinMode(ledPin1, OUTPUT);
  pinMode(ledPin2, OUTPUT);
  pinMode(ledPin3, OUTPUT);
  pinMode(ledPin4, OUTPUT);
  
  // Inicializar comunicación serial para depuración
  Serial.begin(115200);
  
  // Inicializar Bluetooth
  SerialBT.begin("ESP32_Hand_Tracking"); // Nombre del dispositivo Bluetooth
  Serial.println("Bluetooth iniciado. Conecta el dispositivo...");
}

void loop() {
  // Si hay datos disponibles a través de Bluetooth
  if (SerialBT.available() >= 6) { // Esperar al menos 6 bytes (X + 2 bytes + Y + 2 bytes)
    // Leer el primer byte (debe ser 'X')
    if (SerialBT.read() == 'X') {
      // Leer los 2 bytes de X (big endian)
      x = (int16_t)(SerialBT.read() << 8 | SerialBT.read());
      
      // Leer el siguiente byte (debe ser 'Y')
      if (SerialBT.read() == 'Y') {
        // Leer los 2 bytes de Y (big endian)
        y = (int16_t)(SerialBT.read() << 8 | SerialBT.read());
        
        // Imprimir las coordenadas recibidas
        Serial.print("X: ");
        Serial.print(x);
        Serial.print(", Y: ");
        Serial.println(y);
        
        // Controlar los LEDs según la posición de la mano
        controlarLEDs(x, y);
      }
    }
  }
  
  // Pequeña pausa para evitar saturar el bucle
  delay(10);
}

void controlarLEDs(int16_t x, int16_t y) {
  // Apagar todos los LEDs primero
  digitalWrite(ledPin1, LOW);
  digitalWrite(ledPin2, LOW);
  digitalWrite(ledPin3, LOW);
  digitalWrite(ledPin4, LOW);
  
  // Controlar LED 1 (Arriba a la derecha) - X > 0, Y > 0
  if (x > 20 && y > 20) {
    digitalWrite(ledPin1, HIGH);
  }
  // Controlar LED 2 (Arriba a la izquierda) - X < 0, Y > 0
  else if (x < -20 && y > 20) {
    digitalWrite(ledPin2, HIGH);
  }
  // Controlar LED 3 (Abajo a la izquierda) - X < 0, Y < 0
  else if (x < -20 && y < -20) {
    digitalWrite(ledPin3, HIGH);
  }
  // Controlar LED 4 (Abajo a la derecha) - X > 0, Y < 0
  else if (x > 20 && y < -20) {
    digitalWrite(ledPin4, HIGH);
  }
  // Si la mano está cerca del centro, encender todos los LEDs
  else if (abs(x) < 20 && abs(y) < 20) {
    digitalWrite(ledPin1, HIGH);
    digitalWrite(ledPin2, HIGH);
    digitalWrite(ledPin3, HIGH);
    digitalWrite(ledPin4, HIGH);
  }
}