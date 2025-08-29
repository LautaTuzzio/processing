import gab.opencv.*;
import processing.video.*;
import processing.serial.*;
import java.util.*;
import java.awt.Rectangle;

Capture cam;
OpenCV opencv;
PImage objectMask;
PVector smoothedHandCenter = new PVector();
float smoothingFactor = 0.3;

Serial bt; // Bluetooth serial
boolean btConnected = false;
int lastX = -9999, lastY = -9999;
int sendInterval = 100; // ms
long lastSendTime = 0;

// CAMBIA ESTA VARIABLE POR EL PUERTO DE TU BLUETOOTH ↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓
String puertoBluetooth = "COM7"; // COMX ← CAMBIAR POR TU PUERTO BLUETOOTH (Ej: "COM4")

void setup() {
  size(640, 480);
  println("Puertos serie disponibles:");

// opción 1 → más clara
String[] puertos = Serial.list();
for (int i = 0; i < puertos.length; i++) {
  println("[" + i + "] " + puertos[i]);
}

  // Inicializar cámara
  String[] cameras = Capture.list();
  if (cameras.length > 1) {
    println("Usando cámara: " + cameras[1]);
    cam = new Capture(this, 640, 480, cameras[1]);
  } else if (cameras.length > 0) {
    println("Usando cámara: " + cameras[0]);
    cam = new Capture(this, 640, 480, cameras[0]);
  } else {
    println("ERROR: No se encontraron cámaras.");
    exit();
  }
  cam.start();

  // Inicializar OpenCV
  opencv = new OpenCV(this, 640, 480);
  objectMask = createImage(640, 480, RGB);

  // Intentar conexión al puerto Bluetooth indicado
  connectBluetooth();
}

void draw() {
  background(0);
  if (cam.available()) cam.read();

  // Mostrar imagen espejada
  pushMatrix();
  translate(width, 0);
  scale(-1, 1);
  image(cam, 0, 0);
  popMatrix();

  processHandDetection();

  // Punto centro pantalla
  fill(0, 255, 255);
  noStroke();
  ellipse(width/2, height/2, 8, 8);
}

void connectBluetooth() {
  println("Conectando a " + puertoBluetooth + "...");
  try {
    bt = new Serial(this, puertoBluetooth, 115200);
    bt.clear();
    delay(1000);
    btConnected = true;
    println("✓ Bluetooth conectado en " + puertoBluetooth);
  } catch (Exception e) {
    println("❌ Error al conectar Bluetooth: " + e.getMessage());
    btConnected = false;
  }
}

void processHandDetection() {
  objectMask.loadPixels();
  cam.loadPixels();

  for (int i = 0; i < cam.pixels.length; i++) {
    color c = cam.pixels[i];
    float r = red(c), g = green(c), b = blue(c);

    if (r > 0 && r < 50 && g > 0 && g < 50 && b > 0 && b < 50 && abs(r - g) < 15 && abs(g - b) < 15) {
      objectMask.pixels[i] = color(255);
    } else {
      objectMask.pixels[i] = color(0);
    }
  }
  objectMask.updatePixels();

  opencv.loadImage(objectMask);
  opencv.dilate();
  opencv.dilate();
  opencv.erode();
  opencv.erode();
  opencv.dilate();
  opencv.blur(5);

  ArrayList<Contour> contours = opencv.findContours(true, true);
  Contour hand = null;
  float maxArea = 0;

  for (Contour c : contours) {
    if (c.area() > 4000 && c.area() < 120000) {
      Rectangle bound = c.getBoundingBox();
      float aspectRatio = (float)bound.width / bound.height;
      if (aspectRatio > 0.3 && aspectRatio < 3.0 && c.area() > maxArea) {
        maxArea = c.area();
        hand = c;
      }
    }
  }

  if (hand != null) {
    drawHand(hand);
  } else {
    smoothedHandCenter.set(0, 0, 0);
  }
}

void drawHand(Contour hand) {
  stroke(0, 255, 0);
  strokeWeight(2);
  noFill();
  hand.setPolygonApproximationFactor(2);
  ArrayList<PVector> points = hand.getPolygonApproximation().getPoints();

  beginShape();
  for (PVector pt : points) {
    vertex(width - pt.x, pt.y);
  }
  endShape(CLOSE);

  Rectangle bound = hand.getBoundingBox();
  int mirroredX = width - (bound.x + bound.width);
  int mirroredY = bound.y;

  stroke(255, 0, 0);
  noFill();
  rect(mirroredX, mirroredY, bound.width, bound.height);

  PVector currentCenter = new PVector(mirroredX + bound.width/2, mirroredY + bound.height/2);
  if (smoothedHandCenter.x == 0 && smoothedHandCenter.y == 0) {
    smoothedHandCenter.set(currentCenter);
  } else {
    smoothedHandCenter.lerp(currentCenter, 1.0 - smoothingFactor);
  }

  int centerX = width / 2;
  int centerY = height / 2;
  int offsetX = (int)(smoothedHandCenter.x - centerX);
  int offsetY = (int)(centerY - smoothedHandCenter.y);

  fill(255);
  text("X = " + offsetX + ", Y = " + offsetY, 20, 20);

  fill(255, 0, 0);
  noStroke();
  ellipse(smoothedHandCenter.x, smoothedHandCenter.y, 10, 10);

  stroke(255, 255, 0);
  line(centerX, centerY, smoothedHandCenter.x, smoothedHandCenter.y);

  sendToESP(offsetX, offsetY);
}

void sendToESP(int x, int y) {
  if (!btConnected || bt == null) return;

  long now = millis();
  if (now - lastSendTime < sendInterval) return;

  try {
    bt.write((byte) constrain(x, -127, 127));
    bt.write((byte) constrain(y, -127, 127));
    lastSendTime = now;
  } catch (Exception e) {
    println("Error al enviar datos: " + e.getMessage());
    btConnected = false;
    bt = null;
  }
}
