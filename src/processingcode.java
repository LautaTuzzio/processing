import gab.opencv.*;
import processing.video.*;
import java.util.*;
import java.awt.Rectangle;

Capture cam;
OpenCV opencv;
PImage objectMask;
PVector smoothedHandCenter = new PVector();
float smoothingFactor = 0.3;

void setup() {
  size(640, 480);
String[] cameras = Capture.list();
println("Cámaras disponibles: " + cameras.length);

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


  opencv = new OpenCV(this, 640, 480);
  objectMask = createImage(640, 480, RGB);
}

void draw() {
  background(0);

  if (cam.available()) {
    cam.read();
  }

  // Mostrar imagen de la cámara (espejada)
  pushMatrix();
  translate(width, 0);
  scale(-1, 1);
  image(cam, 0, 0);
  popMatrix();

  processHandDetection();

  // Dibuja el punto en el centro de la pantalla
  fill(0, 255, 255);
  noStroke();
  ellipse(width / 2, height / 2, 8, 8);
}

void processHandDetection() {
  objectMask.loadPixels();
  cam.loadPixels();
  
  for (int i = 0; i < cam.pixels.length; i++) {
    color c = cam.pixels[i];
    float r = red(c);
    float g = green(c);
    float b = blue(c);

    if (r > 0 && r < 50 && 
        g > 0 && g < 50 && 
        b > 0 && b < 50 && 
        abs(r - g) < 15 && abs(g - b) < 15) {
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
    drawHandDetection(hand);
  } else {
    smoothedHandCenter.set(0, 0, 0);
  }
}

void drawHandDetection(Contour hand) {
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
  int mirroredRectX = width - (bound.x + bound.width);
  int mirroredRectY = bound.y;

  stroke(255, 0, 0);
  noFill();
  rect(mirroredRectX, mirroredRectY, bound.width, bound.height);

  PVector currentHandCenter = new PVector(mirroredRectX + bound.width / 2, mirroredRectY + bound.height / 2);
  if (smoothedHandCenter.x == 0 && smoothedHandCenter.y == 0) {
    smoothedHandCenter.set(currentHandCenter);
  } else {
    smoothedHandCenter.lerp(currentHandCenter, 1.0 - smoothingFactor);
  }

  int centerX = width / 2;
  int centerY = height / 2;
  int offsetX = (int)(smoothedHandCenter.x - centerX);
  int offsetY = (int)(centerY - smoothedHandCenter.y);

  fill(255);
  textSize(14);
  text("X = " + offsetX + ", Y = " + offsetY, 20, 20);

  fill(255, 0, 0);
  noStroke();
  ellipse(smoothedHandCenter.x, smoothedHandCenter.y, 10, 10);

  stroke(255, 255, 0);
  strokeWeight(1);
  line(centerX, centerY, smoothedHandCenter.x, smoothedHandCenter.y);
}
