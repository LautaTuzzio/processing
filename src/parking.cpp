#include <Arduino.h>

// --- Motores delanteros ---
#define IN1_DEL 15
#define IN4_DEL 16
#define IN2_DEL 2
#define IN3_DEL 4

// --- Motores traseros ---
#define IN1_TRA 17
#define IN2_TRA 5
#define IN3_TRA 18
#define IN4_TRA 19

#define ECHO_LTRAS 13
#define TRIG_LTRAS 12
#define ECHO_LDEL 14
#define TRIG_LDEL 27

#define ECHO_TRAS 22
#define TRIG_TRAS 23

int deteccionesLT = 0;
int deteccionesLD = 0;

unsigned long lastMotorChange = 0;
bool motoresEncendidos = false;
const unsigned long motorOnTime  = 100; 
const unsigned long motorOffTime = 150; 

unsigned long lastDeteccionLT = 0;
unsigned long lastDeteccionLD = 0;
const unsigned long debounceTime = 2000; 

bool avanceAdicional = false;
int pulsosAvanceAdicional = 0;
const int maxPulsosAvanceAdicional = 3; 

int girosCompletados = 0; 
const int girosObjetivo = 2;
bool girando = false;


bool retrocediendo = false;


long medirDistancia(int trig, int echo) {
  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);
  long duracion = pulseIn(echo, HIGH, 30000);
  return duracion * 0.034 / 2; 
}

void motoresStop() {
  digitalWrite(IN1_DEL, LOW);
  digitalWrite(IN4_DEL, LOW);
  digitalWrite(IN2_DEL, LOW);
  digitalWrite(IN3_DEL, LOW);
  digitalWrite(IN1_TRA, LOW);
  digitalWrite(IN2_TRA, LOW);
  digitalWrite(IN3_TRA, LOW);
  digitalWrite(IN4_TRA, LOW);
}

void motoresAdelante() {
  // delanteras
  digitalWrite(IN1_DEL, HIGH);
  digitalWrite(IN4_DEL, LOW);
  digitalWrite(IN2_DEL, HIGH);
  digitalWrite(IN3_DEL, LOW);
  // traseras
  digitalWrite(IN1_TRA, HIGH);
  digitalWrite(IN2_TRA, LOW);
  digitalWrite(IN3_TRA, HIGH);
  digitalWrite(IN4_TRA, LOW);
}

// Giro sobre su propio eje a la derecha
void giroDerecha() {

  digitalWrite(IN1_DEL, HIGH);
  digitalWrite(IN4_DEL, LOW);
  digitalWrite(IN2_DEL, LOW);
  digitalWrite(IN3_DEL, HIGH);

  digitalWrite(IN1_TRA, HIGH);
  digitalWrite(IN2_TRA, LOW);
  digitalWrite(IN3_TRA, LOW);
  digitalWrite(IN4_TRA, HIGH);
}

// Giro sobre su propio eje a la izquierda
void giroIzquierda() {

  digitalWrite(IN1_DEL, LOW);
  digitalWrite(IN4_DEL, HIGH);
  digitalWrite(IN2_DEL, HIGH);
  digitalWrite(IN3_DEL, LOW);

  digitalWrite(IN1_TRA, LOW);
  digitalWrite(IN2_TRA, HIGH);
  digitalWrite(IN3_TRA, HIGH);
  digitalWrite(IN4_TRA, LOW);
}

// Retroceso
void motoresAtras() {

  digitalWrite(IN1_DEL, LOW);
  digitalWrite(IN4_DEL, HIGH);
  digitalWrite(IN2_DEL, LOW);
  digitalWrite(IN3_DEL, HIGH);

  digitalWrite(IN1_TRA, LOW);
  digitalWrite(IN2_TRA, HIGH);
  digitalWrite(IN3_TRA, LOW);
  digitalWrite(IN4_TRA, HIGH);
}

void setup() {
  Serial.begin(115200);

  pinMode(IN1_DEL, OUTPUT);
  pinMode(IN4_DEL, OUTPUT);
  pinMode(IN2_DEL, OUTPUT);
  pinMode(IN3_DEL, OUTPUT);
  pinMode(IN1_TRA, OUTPUT);
  pinMode(IN2_TRA, OUTPUT);
  pinMode(IN3_TRA, OUTPUT);
  pinMode(IN4_TRA, OUTPUT);

  pinMode(TRIG_LTRAS, OUTPUT); pinMode(ECHO_LTRAS, INPUT);
  pinMode(TRIG_LDEL, OUTPUT);  pinMode(ECHO_LDEL, INPUT);
  pinMode(TRIG_TRAS, OUTPUT);  pinMode(ECHO_TRAS, INPUT);

  motoresStop();
  lastMotorChange = millis();
}

void loop() {
  unsigned long now = millis();

  long distLT = medirDistancia(TRIG_LTRAS, ECHO_LTRAS);
  long distLD = medirDistancia(TRIG_LDEL, ECHO_LDEL);
  long distTras = medirDistancia(TRIG_TRAS, ECHO_TRAS);

  Serial.print("LT: "); Serial.print(distLT);
  Serial.print(" cm | LD: "); Serial.print(distLD);
  Serial.print(" cm | Tras: "); Serial.print(distTras);
  Serial.print(" cm | detecciones LT: "); Serial.print(deteccionesLT);
  Serial.print(" | detecciones LD: "); Serial.print(deteccionesLD);
  Serial.print(" | giros: "); Serial.print(girosCompletados);
  Serial.print(" | avance adicional: "); Serial.println(pulsosAvanceAdicional);

  if (distLT > 0 && distLT < 25 && now - lastDeteccionLT > debounceTime && deteccionesLT < 2) {
    deteccionesLT++;
    lastDeteccionLT = now;
    Serial.println("Detección LT");
  }

  if (distLD > 0 && distLD < 25 && now - lastDeteccionLD > debounceTime && deteccionesLD < 2) {
    deteccionesLD++;
    lastDeteccionLD = now;
    Serial.println("Detección LD");
    
    if (deteccionesLD == 2) {
      avanceAdicional = true;
      pulsosAvanceAdicional = 0;
      Serial.println("Iniciando avance adicional...");
    }
  }

  if (girosCompletados < girosObjetivo) {
    bool ambosCompletados = (deteccionesLT >= 2 && deteccionesLD >= 2);
    

    if (ambosCompletados && avanceAdicional && pulsosAvanceAdicional < maxPulsosAvanceAdicional) {
      if (motoresEncendidos && now - lastMotorChange >= motorOnTime) {
        motoresStop();
        motoresEncendidos = false;
        lastMotorChange = now;
        pulsosAvanceAdicional++;
      } else if (!motoresEncendidos && now - lastMotorChange >= motorOffTime) {
        motoresAdelante();
        motoresEncendidos = true;
        lastMotorChange = now;
      }
    }
    else if ((ambosCompletados && pulsosAvanceAdicional >= maxPulsosAvanceAdicional) || girando) {
      if (avanceAdicional && pulsosAvanceAdicional >= maxPulsosAvanceAdicional) {
        avanceAdicional = false;
        Serial.println("Avance adicional completado. Iniciando giro...");
      }
      
      girando = true;
      if (motoresEncendidos && now - lastMotorChange >= motorOnTime) {
        motoresStop();
        motoresEncendidos = false;
        lastMotorChange = now;
        girosCompletados++;
      } else if (!motoresEncendidos && now - lastMotorChange >= motorOffTime) {
        giroDerecha();
        motoresEncendidos = true;
        lastMotorChange = now;
      }
    } else {
      if (motoresEncendidos && now - lastMotorChange >= motorOnTime) {
        motoresStop();
        motoresEncendidos = false;
        lastMotorChange = now;
      } else if (!motoresEncendidos && now - lastMotorChange >= motorOffTime) {
        motoresAdelante();
        motoresEncendidos = true;
        lastMotorChange = now;
      }
    }
  } else {
    retrocediendo = true;
    if (distTras > 0 && distTras > 10) { 
      if (motoresEncendidos && now - lastMotorChange >= motorOnTime) {
        motoresStop();
        motoresEncendidos = false;
        lastMotorChange = now;
      } else if (!motoresEncendidos && now - lastMotorChange >= motorOffTime) {
        motoresAtras();
        motoresEncendidos = true;
        lastMotorChange = now;
      }
    } else {
      motoresStop();
      Serial.println("RETROCESO COMPLETO");

      bool paralelo = false;
      while (!paralelo) {
        long distLT = medirDistancia(TRIG_LTRAS, ECHO_LTRAS);
        long distLD = medirDistancia(TRIG_LDEL, ECHO_LDEL);

        Serial.print("GIRANDO IZQ | LT: ");
        Serial.print(distLT);
        Serial.print(" cm | LD: ");
        Serial.println(distLD);

        if (distLT > 0 && distLD > 0 && distLT == distLD) {
          motoresStop();
          paralelo = true;
          Serial.println("AUTO PARALELO (lecturas iguales)");
        } else {
          giroIzquierda();
        }
      }

      while (true);
    }
  }
}
