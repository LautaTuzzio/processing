#include <Arduino.h>
#include "BluetoothSerial.h"

BluetoothSerial SerialBT;

// Pines puente H delantero (ruedas delanteras)
#define IN1_DEL 15 // Frontal - Izquierda Adelante
#define IN4_DEL 16 // Frontal - Izquierda Atras
#define IN2_DEL 2  // Frontal - Derecha Adelante
#define IN3_DEL 4  // Frontal - Derecha Atras

// Pines puente H trasero (ruedas traseras)
#define IN1_TRA 17 // Trasero - Izquierda Adelante
#define IN2_TRA 5  // Trasero - Izquierda Atras
#define IN3_TRA 18 // Trasero - Derecha Adelante
#define IN4_TRA 19 // Trasero - Derecha Atras

int currentCmd = 0;
unsigned long lastReceived = 0;
const unsigned long TIMEOUT = 1000;

// FUNCIONES DE MOVIMIENTO
void adelante() {
  digitalWrite(IN1_DEL, HIGH);
  digitalWrite(IN2_DEL, LOW);
  digitalWrite(IN3_DEL, HIGH);
  digitalWrite(IN4_DEL, LOW);

  digitalWrite(IN1_TRA, HIGH);
  digitalWrite(IN2_TRA, LOW);
  digitalWrite(IN3_TRA, HIGH);
  digitalWrite(IN4_TRA, LOW);
}

void atras() {
  digitalWrite(IN1_DEL, LOW);
  digitalWrite(IN2_DEL, HIGH);
  digitalWrite(IN3_DEL, LOW);
  digitalWrite(IN4_DEL, HIGH);

  digitalWrite(IN1_TRA, LOW);
  digitalWrite(IN2_TRA, HIGH);
  digitalWrite(IN3_TRA, LOW);
  digitalWrite(IN4_TRA, HIGH);
}

void izquierda() {
  digitalWrite(IN1_DEL, LOW);
  digitalWrite(IN2_DEL, HIGH);
  digitalWrite(IN3_DEL, HIGH);
  digitalWrite(IN4_DEL, LOW);

  digitalWrite(IN1_TRA, LOW);
  digitalWrite(IN2_TRA, HIGH);
  digitalWrite(IN3_TRA, HIGH);
  digitalWrite(IN4_TRA, LOW);
}

void derecha() {
  digitalWrite(IN1_DEL, HIGH);
  digitalWrite(IN2_DEL, LOW);
  digitalWrite(IN3_DEL, LOW);
  digitalWrite(IN4_DEL, HIGH);

  digitalWrite(IN1_TRA, HIGH);
  digitalWrite(IN2_TRA, LOW);
  digitalWrite(IN3_TRA, LOW);
  digitalWrite(IN4_TRA, HIGH);
}

void frenar() {
  digitalWrite(IN1_DEL, LOW);
  digitalWrite(IN2_DEL, LOW);
  digitalWrite(IN3_DEL, LOW);
  digitalWrite(IN4_DEL, LOW);

  digitalWrite(IN1_TRA, LOW);
  digitalWrite(IN2_TRA, LOW);
  digitalWrite(IN3_TRA, LOW);
  digitalWrite(IN4_TRA, LOW);
}

// PROCESAR COMANDO
void processCommand(int cmd) {
  frenar();

  switch (cmd) {
    case 1: adelante(); izquierda(); break;
    case 2: adelante(); derecha(); break;
    case 3: atras(); izquierda(); break;
    case 4: atras(); derecha(); break;
    case 5: adelante(); break;
    case 6: derecha(); break;
    case 7: atras(); break;
    case 8: izquierda(); break;
    default: frenar();
  }

  currentCmd = cmd;
}

void setup() {
  Serial.begin(115200);

  pinMode(IN1_DEL, OUTPUT);
  pinMode(IN2_DEL, OUTPUT);
  pinMode(IN3_DEL, OUTPUT);
  pinMode(IN4_DEL, OUTPUT);

  pinMode(IN1_TRA, OUTPUT);
  pinMode(IN2_TRA, OUTPUT);
  pinMode(IN3_TRA, OUTPUT);
  pinMode(IN4_TRA, OUTPUT);

  frenar();

  SerialBT.begin("estoy dormido");
  Serial.println("Bluetooth listo como 'ESP32_CAR_Control'");
}

void loop() {
  if (SerialBT.available() >= 2) {
    int8_t x = SerialBT.read();
    int8_t y = SerialBT.read();
    lastReceived = millis();

    // Mostrar coordenadas crudas de la mano
    Serial.printf("Coords → X: %d | Y: %d\n", x, y);

        int cmd = 0;
    const int threshold = 50;


    bool top = y > threshold;     
    bool bottom = y < -threshold; 
    bool left = x < -threshold;
    bool right = x > threshold;

    if (top && left) cmd = 1;
    else if (top && right) cmd = 2;
    else if (bottom && left) cmd = 3;
    else if (bottom && right) cmd = 4;
    else if (top) cmd = 5;
    else if (right) cmd = 6;
    else if (bottom) cmd = 7;
    else if (left) cmd = 8;


    processCommand(cmd);

    const char* direction = "";
    switch (cmd) {
      case 1: direction = "Arriba-Izquierda"; break;
      case 2: direction = "Arriba-Derecha"; break;
      case 3: direction = "Abajo-Izquierda"; break;
      case 4: direction = "Abajo-Derecha"; break;
      case 5: direction = "Arriba"; break;
      case 6: direction = "Derecha"; break;
      case 7: direction = "Abajo"; break;
      case 8: direction = "Izquierda"; break;
      default: direction = "Detenido";
    }
    Serial.printf("Direccion: %s (CMD %d)\n", direction, cmd);
  }

  if (millis() - lastReceived > TIMEOUT && currentCmd != 0) {
    frenar();
    currentCmd = 0;
    Serial.println("Timeout");
  }

  delay(20);
}
