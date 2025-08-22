#include <Arduino.h>
#include "BluetoothSerial.h"

BluetoothSerial SerialBT;

// Pines de las 4 ruedas (motores)
#define pin_adelante_r1 25    // Rueda 1 - adelante
#define pin_atras_r1 26       // Rueda 1 - atrás
#define pin_adelante_r2 33    // Rueda 2 - adelante  
#define pin_atras_r2 32       // Rueda 2 - atrás
#define pin_adelante_r3 19    // Rueda 3 - adelante
#define pin_atras_r3 21       // Rueda 3 - atrás
#define pin_adelante_r4 22    // Rueda 4 - adelante
#define pin_atras_r4 23       // Rueda 4 - atrás

int currentCommand = 0;
unsigned long lastReceived = 0;
const unsigned long TIMEOUT = 5000;

void stopAllMotors() {
  // Detener todas las ruedas
  digitalWrite(pin_adelante_r1, LOW);
  digitalWrite(pin_atras_r1, LOW);
  digitalWrite(pin_adelante_r2, LOW);
  digitalWrite(pin_atras_r2, LOW);
  digitalWrite(pin_adelante_r3, LOW);
  digitalWrite(pin_atras_r3, LOW);
  digitalWrite(pin_adelante_r4, LOW);
  digitalWrite(pin_atras_r4, LOW);
}

void moveForward() {
  // Todas las ruedas hacia adelante
  digitalWrite(pin_adelante_r1, HIGH);
  digitalWrite(pin_atras_r1, LOW);
  digitalWrite(pin_adelante_r2, HIGH);
  digitalWrite(pin_atras_r2, LOW);
  digitalWrite(pin_adelante_r3, HIGH);
  digitalWrite(pin_atras_r3, LOW);
  digitalWrite(pin_adelante_r4, HIGH);
  digitalWrite(pin_atras_r4, LOW);
}

void moveBackward() {
  // Todas las ruedas hacia atrás
  digitalWrite(pin_adelante_r1, LOW);
  digitalWrite(pin_atras_r1, HIGH);
  digitalWrite(pin_adelante_r2, LOW);
  digitalWrite(pin_atras_r2, HIGH);
  digitalWrite(pin_adelante_r3, LOW);
  digitalWrite(pin_atras_r3, HIGH);
  digitalWrite(pin_adelante_r4, LOW);
  digitalWrite(pin_atras_r4, HIGH);
}

void turnLeft() {
  // Ruedas izquierdas (r1, r3) hacia atrás, derechas (r2, r4) hacia adelante
  digitalWrite(pin_adelante_r1, LOW);
  digitalWrite(pin_atras_r1, HIGH);
  digitalWrite(pin_adelante_r2, HIGH);
  digitalWrite(pin_atras_r2, LOW);
  digitalWrite(pin_adelante_r3, LOW);
  digitalWrite(pin_atras_r3, HIGH);
  digitalWrite(pin_adelante_r4, HIGH);
  digitalWrite(pin_atras_r4, LOW);
}

void turnRight() {
  // Ruedas derechas (r2, r4) hacia atrás, izquierdas (r1, r3) hacia adelante
  digitalWrite(pin_adelante_r1, HIGH);
  digitalWrite(pin_atras_r1, LOW);
  digitalWrite(pin_adelante_r2, LOW);
  digitalWrite(pin_atras_r2, HIGH);
  digitalWrite(pin_adelante_r3, HIGH);
  digitalWrite(pin_atras_r3, LOW);
  digitalWrite(pin_adelante_r4, LOW);
  digitalWrite(pin_atras_r4, HIGH);
}

void processCommand(int cmd) {
  stopAllMotors();
  
  // Procesar comandos de dirección
  if (cmd == 1) { // Top-Left (Adelante + Izquierda)
    moveForward();
    // Podrías implementar un giro diagonal aquí si es necesario
  } else if (cmd == 2) { // Top-Right (Adelante + Derecha)
    moveForward();
    // Podrías implementar un giro diagonal aquí si es necesario
  } else if (cmd == 3) { // Bottom-Left (Atrás + Izquierda)
    moveBackward();
  } else if (cmd == 4) { // Bottom-Right (Atrás + Derecha)
    moveBackward();
  } else if (cmd == 5) { // Top (Adelante)
    moveForward();
  } else if (cmd == 6) { // Right (Girar Derecha)
    turnRight();
  } else if (cmd == 7) { // Bottom (Atrás)
    moveBackward();
  } else if (cmd == 8) { // Left (Girar Izquierda)
    turnLeft();
  }
  
  currentCommand = cmd;
}

void setup() {
  Serial.begin(115200);
  
  // Configurar todos los pines como salida
  pinMode(pin_adelante_r1, OUTPUT);
  pinMode(pin_atras_r1, OUTPUT);
  pinMode(pin_adelante_r2, OUTPUT);
  pinMode(pin_atras_r2, OUTPUT);
  pinMode(pin_adelante_r3, OUTPUT);
  pinMode(pin_atras_r3, OUTPUT);
  pinMode(pin_adelante_r4, OUTPUT);
  pinMode(pin_atras_r4, OUTPUT);
  
  // Pines de entrada (del código original)
  pinMode(34, INPUT);
  pinMode(35, INPUT);
  
  // Inicializar motores apagados
  stopAllMotors();
  
  // Inicializar Bluetooth
  SerialBT.begin("ESP32_Car_Control");
  Serial.println("Bluetooth listo como 'ESP32_Car_Control'");
}

void loop() {
  if (SerialBT.available() >= 2) {
    int8_t x = SerialBT.read();
    int8_t y = SerialBT.read();
    lastReceived = millis();
    
    int cmd = 0;
    
    // Detección de dirección basada en umbrales
    const int threshold = 20;  // Ajusta este valor para cambiar la sensibilidad
    
    bool top = y < -threshold;
    bool bottom = y > threshold;
    bool left = x < -threshold;
    bool right = x > threshold;
    
    // Direcciones diagonales (combinan dos direcciones)
    if (top && left) cmd = 1;        // Top-Left
    else if (top && right) cmd = 2;  // Top-Right
    else if (bottom && left) cmd = 3; // Bottom-Left
    else if (bottom && right) cmd = 4; // Bottom-Right
    // Direcciones rectas
    else if (top) cmd = 5;    // Top (Adelante)
    else if (right) cmd = 6;  // Right (Girar Derecha)
    else if (bottom) cmd = 7; // Bottom (Atrás)
    else if (left) cmd = 8;   // Left (Girar Izquierda)
    
    processCommand(cmd);
    
    // Debug: mostrar información
    const char* direction = "";
    switch(cmd) {
      case 1: direction = "Adelante-Izquierda"; break;
      case 2: direction = "Adelante-Derecha"; break;
      case 3: direction = "Atrás-Izquierda"; break;
      case 4: direction = "Atrás-Derecha"; break;
      case 5: direction = "Adelante"; break;
      case 6: direction = "Girar Derecha"; break;
      case 7: direction = "Atrás"; break;
      case 8: direction = "Girar Izquierda"; break;
      default: direction = "Parado";
    }
    Serial.printf("X: %d Y: %d → %s (Comando %d)\n", x, y, direction, cmd);
  }
  
  // Timeout: si no se reciben comandos, detener el auto
  if (millis() - lastReceived > TIMEOUT && currentCommand != 0) {
    stopAllMotors();
    currentCommand = 0;
    Serial.println("Timeout, deteniendo auto.");
  }
  
  delay(20);
}
