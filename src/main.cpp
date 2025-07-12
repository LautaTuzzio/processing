#include "BluetoothSerial.h"
#include <Arduino.h>

// Verificar que el Bluetooth esté habilitado
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

// Pines para los LEDs
const int ledPin1 = 19;  // Pin para el LED 1 (Arriba-Derecha)
const int ledPin2 = 21;  // Pin para el LED 2 (Arriba-Izquierda)
const int ledPin3 = 22;  // Pin para el LED 3 (Abajo-Izquierda)
const int ledPin4 = 23;  // Pin para el LED 4 (Abajo-Derecha)

// Pin para LED de estado (opcional, si tienes un LED extra para mostrar conexión)
const int statusLedPin = 2; // LED integrado del ESP32

// Objeto para la comunicación Bluetooth
BluetoothSerial SerialBT;

// Variables para almacenar las coordenadas
int16_t x = 0;
int16_t y = 0;

// Variables para manejo de timeout
unsigned long lastDataTime = 0;
const unsigned long dataTimeout = 3000; // 3 segundos sin datos = timeout

// Variables para el LED de estado
unsigned long lastBlinkTime = 0;
bool statusLedState = false;

// Variables para debugging
unsigned long lastDebugTime = 0;
const unsigned long debugInterval = 2000; // Debug cada 2 segundos
int totalPacketsReceived = 0;
int validPacketsReceived = 0;
int errorPacketsReceived = 0;

// Variables para limpiar buffer
unsigned long lastBufferClear = 0;
const unsigned long bufferClearInterval = 10000; // Limpiar buffer cada 10 segundos

void setup() {
  // Inicializar pines de los LEDs como salidas
  pinMode(ledPin1, OUTPUT);
  pinMode(ledPin2, OUTPUT);
  pinMode(ledPin3, OUTPUT);
  pinMode(ledPin4, OUTPUT);
  pinMode(statusLedPin, OUTPUT);
  
  // Apagar todos los LEDs al inicio
  digitalWrite(ledPin1, LOW);
  digitalWrite(ledPin2, LOW);
  digitalWrite(ledPin3, LOW);
  digitalWrite(ledPin4, LOW);
  digitalWrite(statusLedPin, LOW);
  
  // Inicializar comunicación serial para depuración
  Serial.begin(115200);
  Serial.println("=== ESP32 Hand Tracking con Debugging ===");
  Serial.println("Versión: 2.0 - Debugging mejorado");
  Serial.println("Compilado: " + String(__DATE__) + " " + String(__TIME__));
  
  // Inicializar Bluetooth
  SerialBT.begin("ESP32_Hand_Tracking"); // Nombre del dispositivo Bluetooth
  Serial.println("Bluetooth iniciado. Nombre del dispositivo: ESP32_Hand_Tracking");
  Serial.println("Esperando conexión...");
  
  // Parpadear LED de estado para indicar que está listo
  for (int i = 0; i < 6; i++) {
    digitalWrite(statusLedPin, HIGH);
    delay(200);
    digitalWrite(statusLedPin, LOW);
    delay(200);
  }
  
  Serial.println("=== SISTEMA LISTO ===");
}

void controlarLEDs(int16_t x, int16_t y) {
  // Apagar todos los LEDs primero
  digitalWrite(ledPin1, LOW);
  digitalWrite(ledPin2, LOW);
  digitalWrite(ledPin3, LOW);
  digitalWrite(ledPin4, LOW);
  
  // Definir zona muerta más pequeña para mejor sensibilidad
  const int deadZone = 15;
  
  // Controlar LED 1 (Arriba a la derecha) - X > 0, Y > 0
  if (x > deadZone && y > deadZone) {
    digitalWrite(ledPin1, HIGH);
    Serial.println("LED 1 activado (Arriba-Derecha)");
  }
  // Controlar LED 2 (Arriba a la izquierda) - X < 0, Y > 0
  else if (x < -deadZone && y > deadZone) {
    digitalWrite(ledPin2, HIGH);
    Serial.println("LED 2 activado (Arriba-Izquierda)");
  }
  // Controlar LED 3 (Abajo a la izquierda) - X < 0, Y < 0
  else if (x < -deadZone && y < -deadZone) {
    digitalWrite(ledPin3, HIGH);
    Serial.println("LED 3 activado (Abajo-Izquierda)");
  }
  // Controlar LED 4 (Abajo a la derecha) - X > 0, Y < 0
  else if (x > deadZone && y < -deadZone) {
    digitalWrite(ledPin4, HIGH);
    Serial.println("LED 4 activado (Abajo-Derecha)");
  }
  // Si la mano está cerca del centro, encender todos los LEDs
  else if (abs(x) <= deadZone && abs(y) <= deadZone) {
    digitalWrite(ledPin1, HIGH);
    digitalWrite(ledPin2, HIGH);
    digitalWrite(ledPin3, HIGH);
    digitalWrite(ledPin4, HIGH);
    Serial.println("Todos los LEDs activados (Centro)");
  }
}

void manejarStatusLed() {
  unsigned long currentTime = millis();
  
  // Verificar si hay timeout de datos
  if (currentTime - lastDataTime > dataTimeout) {
    // Parpadear LED de estado si no hay datos
    if (currentTime - lastBlinkTime > 500) {
      statusLedState = !statusLedState;
      digitalWrite(statusLedPin, statusLedState);
      lastBlinkTime = currentTime;
    }
  } else {
    // LED de estado fijo si hay datos
    digitalWrite(statusLedPin, HIGH);
  }
}

void printDebugInfo() {
  unsigned long currentTime = millis();
  
  if (currentTime - lastDebugTime > debugInterval) {
    Serial.println("=== DEBUG INFO ===");
    Serial.println("Uptime: " + String(currentTime / 1000) + " segundos");
    Serial.println("Cliente conectado: " + String(SerialBT.hasClient() ? "SI" : "NO"));
    Serial.println("Bytes disponibles: " + String(SerialBT.available()));
    Serial.println("Paquetes totales: " + String(totalPacketsReceived));
    Serial.println("Paquetes válidos: " + String(validPacketsReceived));
    Serial.println("Paquetes con error: " + String(errorPacketsReceived));
    
    if (totalPacketsReceived > 0) {
      float successRate = (float)validPacketsReceived / totalPacketsReceived * 100;
      Serial.println("Tasa de éxito: " + String(successRate, 1) + "%");
    }
    
    Serial.println("Última coord válida: X=" + String(x) + ", Y=" + String(y));
    Serial.println("Tiempo sin datos: " + String(currentTime - lastDataTime) + " ms");
    Serial.println("=== FIN DEBUG ===");
    
    lastDebugTime = currentTime;
  }
}

void clearBluetoothBuffer() {
  unsigned long currentTime = millis();
  
  if (currentTime - lastBufferClear > bufferClearInterval) {
    int bytesCleared = 0;
    
    // Limpiar buffer si hay demasiados bytes acumulados
    if (SerialBT.available() > 50) {
      Serial.println("ADVERTENCIA: Buffer con muchos bytes (" + String(SerialBT.available()) + "), limpiando...");
      
      while (SerialBT.available()) {
        SerialBT.read();
        bytesCleared++;
        if (bytesCleared > 100) break; // Evitar bucle infinito
      }
      
      Serial.println("Buffer limpiado, " + String(bytesCleared) + " bytes eliminados");
    }
    
    lastBufferClear = currentTime;
  }
}

bool leerDatosBluetooth() {
  static int estado = 0; // 0: buscando 'X', 1: leyendo X, 2: buscando 'Y', 3: leyendo Y
  static int16_t tempX = 0, tempY = 0;
  static int byteCount = 0;
  static unsigned long lastByteTime = 0;
  
  // Timeout para reset del estado si pasa mucho tiempo
  if (millis() - lastByteTime > 1000 && estado != 0) {
    Serial.println("TIMEOUT: Reseteando estado de lectura");
    estado = 0;
    byteCount = 0;
    errorPacketsReceived++;
  }
  
  while (SerialBT.available()) {
    uint8_t byte_recibido = SerialBT.read();
    lastByteTime = millis();
    
    // Debug: mostrar bytes recibidos en formato legible
    if (byte_recibido >= 32 && byte_recibido <= 126) {
      Serial.print("Byte recibido: '" + String((char)byte_recibido) + "' (");
    } else {
      Serial.print("Byte recibido: [" + String(byte_recibido) + "] (");
    }
    Serial.print(byte_recibido, HEX);
    Serial.println(") - Estado: " + String(estado));
    
    switch (estado) {
      case 0: // Buscando 'X'
        if (byte_recibido == 'X') {
          Serial.println("-> Encontré 'X', cambiando a estado 1");
          estado = 1;
          byteCount = 0;
          tempX = 0;
        } else {
          Serial.println("-> Byte ignorado, buscando 'X'");
        }
        break;
        
      case 1: // Leyendo coordenada X (2 bytes)
        if (byteCount == 0) {
          tempX = (int16_t)(byte_recibido << 8); // Byte alto
          byteCount++;
          Serial.println("-> X byte alto: " + String(byte_recibido) + ", tempX=" + String(tempX));
        } else if (byteCount == 1) {
          tempX |= byte_recibido; // Byte bajo
          Serial.println("-> X byte bajo: " + String(byte_recibido) + ", tempX final=" + String(tempX));
          estado = 2;
          byteCount = 0;
        }
        break;
        
      case 2: // Buscando 'Y'
        if (byte_recibido == 'Y') {
          Serial.println("-> Encontré 'Y', cambiando a estado 3");
          estado = 3;
          byteCount = 0;
          tempY = 0;
        } else {
          Serial.println("-> Error: esperaba 'Y', pero recibí " + String(byte_recibido) + ". Reseteando.");
          estado = 0;
          errorPacketsReceived++;
        }
        break;
        
      case 3: // Leyendo coordenada Y (2 bytes)
        if (byteCount == 0) {
          tempY = (int16_t)(byte_recibido << 8); // Byte alto
          byteCount++;
          Serial.println("-> Y byte alto: " + String(byte_recibido) + ", tempY=" + String(tempY));
        } else if (byteCount == 1) {
          tempY |= byte_recibido; // Byte bajo
          Serial.println("-> Y byte bajo: " + String(byte_recibido) + ", tempY final=" + String(tempY));
          
          // Datos completos recibidos
          x = tempX;
          y = tempY;
          estado = 0; // Volver al estado inicial
          totalPacketsReceived++;
          validPacketsReceived++;
          
          Serial.println("*** PAQUETE VÁLIDO RECIBIDO: X=" + String(x) + ", Y=" + String(y) + " ***");
          return true; // Datos válidos recibidos
        }
        break;
    }
  }
  
  return false; // No se recibieron datos completos
}

void loop() {
  // Manejar LED de estado
  manejarStatusLed();
  
  // Mostrar información de debug periódicamente
  printDebugInfo();
  
  // Limpiar buffer periódicamente
  clearBluetoothBuffer();
  
  // Verificar conexión Bluetooth
  if (!SerialBT.hasClient()) {
    // No hay cliente conectado, apagar todos los LEDs
    digitalWrite(ledPin1, LOW);
    digitalWrite(ledPin2, LOW);
    digitalWrite(ledPin3, LOW);
    digitalWrite(ledPin4, LOW);
    delay(100);
    return;
  }
  
  // Leer datos del Bluetooth
  if (leerDatosBluetooth()) {
    // Actualizar tiempo de última recepción de datos
    lastDataTime = millis();
    
    // Imprimir las coordenadas recibidas
    Serial.print(">>> COORDENADAS PROCESADAS - X: ");
    Serial.print(x);
    Serial.print(", Y: ");
    Serial.println(y);
    
    // Controlar los LEDs según la posición de la mano
    controlarLEDs(x, y);
  }
  
  // Verificar timeout de datos
  if (millis() - lastDataTime > dataTimeout) {
    // Si no hay datos por mucho tiempo, apagar todos los LEDs
    digitalWrite(ledPin1, LOW);
    digitalWrite(ledPin2, LOW);
    digitalWrite(ledPin3, LOW);
    digitalWrite(ledPin4, LOW);
  }
  
  // Pequeña pausa para evitar saturar el bucle
  delay(20); // Aumenté un poco el delay para mejor debugging
}