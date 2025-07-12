import gab.opencv.*; // Importa la libreria OpenCV para vision por computadora
import processing.video.*; // Importa la libreria de video para capturar imagenes de la camara
import processing.serial.*; // Importa la libreria Serial para comunicacion Bluetooth
import java.util.*; // Importa utilidades de Java, como ArrayList
import java.awt.Rectangle; // Importa la clase Rectangle de AWT para representar rectangulos

// Variables globales importantes
Capture cam; // Objeto para capturar video de la camara
OpenCV opencv; // Objeto OpenCV para procesamiento de imagenes
PImage objectMask; // Imagen que actuara como mascara para el objeto detectado (el guante)
Serial bt; // Objeto Serial para comunicacion Bluetooth

// Variable para el factor de suavizado del centro de la mano
PVector smoothedHandCenter = new PVector(); // Almacena la posicion suavizada del centro de la mano
float smoothingFactor = 0.3; // Controla que tan rapido el punto rojo sigue el movimiento real de la mano

// Variables para control de envío de datos - OPTIMIZADAS
long lastSendTime = 0; // Tiempo del último envío
int sendInterval = 100; // Intervalo entre envíos AUMENTADO a 100ms (10 fps) para evitar saturación
int maxRetriesPerFrame = 3; // Máximo de reintentos de envío por frame

// Variables para manejo de errores y estabilidad
boolean cameraReady = false;
boolean bluetoothConnected = false;
int frameCount = 0;
long lastFrameTime = 0;

// Variables para timeout de cámara
long lastCameraRead = 0;
final long CAMERA_TIMEOUT = 3000; // 3 segundos timeout

/**
 * Funcion setup()
 * Se ejecuta una vez al inicio del programa.
 * Configura el tamaño de la ventana, inicializa la camara, OpenCV.
 */
void setup() {
  size(640, 480); // Establece el tamaño de la ventana de visualizacion en 640x480 pixeles.

  // Inicializar cámara con manejo de errores mejorado
  println("Inicializando cámara...");
  initializeCamera();
  
  // Inicializar OpenCV
  opencv = new OpenCV(this, 640, 480);
  objectMask = createImage(640, 480, RGB);
  
  // Instrucciones para el usuario
  println("=== INSTRUCCIONES ===");
  println("1. Espera a que la cámara esté funcionando");
  println("2. Presiona 'l' para listar puertos disponibles");
  println("3. Presiona 'b' para inicializar Bluetooth");
  println("4. Presiona 'r' para reiniciar conexión Bluetooth");
  println("5. Presiona 'c' para reiniciar cámara");
  println("====================");
}

/**
 * Función para inicializar la cámara de forma segura
 */
void initializeCamera() {
  try {
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
      return;
    }
    
    cam.start();
    cameraReady = false; // Se marcará como true cuando reciba el primer frame
    println("Cámara inicializada, esperando primer frame...");
    
  } catch (Exception e) {
    println("ERROR al inicializar cámara: " + e.getMessage());
    cam = null;
  }
}

/**
 * Función para inicializar la conexión Bluetooth
 */
void initBluetooth() {
  if (!cameraReady) {
    println("ERROR: Inicializa la cámara primero. Presiona 'c' si es necesario.");
    return;
  }
  
  // Cerrar conexión anterior si existe
  if (bt != null) {
    try {
      bt.stop();
      bt = null;
    } catch (Exception e) {
      println("Error al cerrar conexión anterior: " + e.getMessage());
    }
    delay(500); // Pausa para asegurar que se cierre completamente
  }
  
  try {
    // CAMBIAR ESTE PUERTO POR EL TUYO (ya sabes que es COM4)
    String puertoESP32 = "COM4"; // PUERTO CORRECTO PARA TU ESP32
    
    println("Intentando conectar al puerto: " + puertoESP32);
    bt = new Serial(this, puertoESP32, 115200);
    
    // Limpiar buffer inicial
    bt.clear();
    
    // Pausa para estabilizar la conexión
    delay(1000);
    
    bluetoothConnected = true;
    println("✓ Bluetooth conectado exitosamente en " + puertoESP32);
    
  } catch (Exception e) {
    println("ERROR al conectar Bluetooth: " + e.getMessage());
    println("Verifica que:");
    println("1. El ESP32 esté encendido y emparejado");
    println("2. El puerto sea el correcto (COM4)");
    println("3. No haya otras aplicaciones usando el puerto");
    println("4. El ESP32 esté alimentado independientemente (no por USB)");
    bt = null;
    bluetoothConnected = false;
  }
}

/**
 * Función para enviar coordenadas al ESP32 vía Bluetooth de forma segura
 */
void sendCoordinates(int x, int y) {
  if (!bluetoothConnected || bt == null) {
    return;
  }
  
  // Control de frecuencia de envío
  long currentTime = millis();
  if (currentTime - lastSendTime < sendInterval) {
    return;
  }
  
  int retries = 0;
  boolean sent = false;
  
  while (!sent && retries < maxRetriesPerFrame) {
    try {
      // Verificar que la conexión esté activa
      if (!bt.active()) {
        println("Conexión Bluetooth perdida, reintentando...");
        bluetoothConnected = false;
        return;
      }
      
      // Protocolo: X + 2 bytes para X + Y + 2 bytes para Y
      bt.write('X');
      bt.write((byte)((x >> 8) & 0xFF)); // Byte alto de X
      bt.write((byte)(x & 0xFF));        // Byte bajo de X
      bt.write('Y');
      bt.write((byte)((y >> 8) & 0xFF)); // Byte alto de Y
      bt.write((byte)(y & 0xFF));        // Byte bajo de Y
      
      sent = true;
      lastSendTime = currentTime;
      
      // Mostrar datos enviados (con menos frecuencia para evitar saturar consola)
      if (frameCount % 10 == 0) {
        println("Enviado - X: " + x + ", Y: " + y);
      }
      
    } catch (Exception e) {
      retries++;
      println("Error al enviar datos (intento " + retries + "): " + e.getMessage());
      
      if (retries >= maxRetriesPerFrame) {
        println("Demasiados errores, reiniciando conexión...");
        bluetoothConnected = false;
        try {
          bt.stop();
          bt = null;
        } catch (Exception ex) {
          // Ignorar errores al cerrar
        }
        return;
      }
      
      // Pausa breve antes de reintentar
      delay(10);
    }
  }
}

/**
 * Función principal de dibujo - OPTIMIZADA
 */
void draw() {
  background(0);
  frameCount++;
  
  // Verificar si la cámara existe
  if (cam == null) {
    showError("Cámara no inicializada. Presiona 'c' para reiniciar.");
    return;
  }

  // Verificar disponibilidad de la cámara con timeout
  long currentTime = millis();
  if (!cam.available()) {
    if (currentTime - lastCameraRead > CAMERA_TIMEOUT) {
      showError("Cámara no responde. Presiona 'c' para reiniciar.");
      return;
    }
    
    fill(255, 255, 0);
    textSize(20);
    text("Esperando cámara...", 20, height/2);
    return;
  }
  
  // Leer frame de la cámara
  try {
    cam.read();
    lastCameraRead = currentTime;
    
    if (!cameraReady) {
      cameraReady = true;
      println("✓ Cámara lista y funcionando");
    }
    
  } catch (Exception e) {
    println("ERROR al leer cámara: " + e.getMessage());
    showError("Error de cámara. Presiona 'c' para reiniciar.");
    return;
  }

  // Mostrar imagen de la cámara (espejada)
  pushMatrix();
  translate(width, 0);
  scale(-1, 1);
  image(cam, 0, 0);
  popMatrix();

  // Procesar detección de mano (cada 2 frames para mejorar rendimiento)
  if (frameCount % 2 == 0) {
    processHandDetection();
  }

  // Mostrar información de estado
  showStatus();
  
  // Mostrar FPS
  if (frameCount % 30 == 0) {
    float fps = 1000.0 / (currentTime - lastFrameTime);
    println("FPS: " + nf(fps, 1, 1));
  }
  lastFrameTime = currentTime;
}

/**
 * Función para procesar la detección de mano
 */
void processHandDetection() {
  if (cam == null) return;
  
  try {
    // Crear mascara del objeto (guante)
    objectMask.loadPixels();
    cam.loadPixels();
    
    for (int i = 0; i < cam.pixels.length; i++) {
      color c = cam.pixels[i];
      float r = red(c);
      float g = green(c);
      float b = blue(c);

      // Condiciones para detectar un color especifico (negro/gris oscuro)
      if (r > 0 && r < 50 && 
          g > 0 && g < 50 && 
          b > 0 && b < 50 && 
          r > (g - 15) && r < (g + 15) && 
          g > (b - 15) && g < (b + 15)) {
        objectMask.pixels[i] = color(255);
      } else {
        objectMask.pixels[i] = color(0);
      }
    }
    objectMask.updatePixels();

    // Procesamiento OpenCV
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

    // Filtrar contornos para encontrar el guante
    for (Contour c : contours) {
      if (c.area() > 4000 && c.area() < 120000) {
        Rectangle bound = c.getBoundingBox();
        float aspectRatio = (float)bound.width / bound.height;

        if (aspectRatio > 0.3 && aspectRatio < 3.0) {
          if (c.area() > maxArea) {
            maxArea = c.area();
            hand = c;
          }
        }
      }
    }

    if (hand != null && maxArea > 6000) {
      drawHandDetection(hand);
    } else {
      // Reiniciar el centro suavizado si no hay mano detectada
      smoothedHandCenter.set(0, 0, 0);
    }
    
  } catch (Exception e) {
    println("ERROR en procesamiento de mano: " + e.getMessage());
  }
}

/**
 * Función para dibujar la detección de mano y enviar datos
 */
void drawHandDetection(Contour hand) {
  stroke(0, 255, 0);
  strokeWeight(2);
  noFill();

  // Obtener la aproximacion del poligono para el contorno de la mano
  hand.setPolygonApproximationFactor(2);
  ArrayList<PVector> points = hand.getPolygonApproximation().getPoints();

  // Dibujar el contorno espejado
  beginShape();
  for (PVector pt : points) {
    vertex(width - pt.x, pt.y);
  }
  endShape(CLOSE);

  // Calcular el rectangulo envolvente de la mano
  Rectangle bound = hand.getBoundingBox();
  int mirroredRectX = width - (bound.x + bound.width);
  int mirroredRectY = bound.y;
  int mirroredRectWidth = bound.width;
  int mirroredRectHeight = bound.height;

  // Dibujar el rectangulo espejado alrededor de la mano
  stroke(255, 0, 0);
  noFill();
  rect(mirroredRectX, mirroredRectY, mirroredRectWidth, mirroredRectHeight);

  // Calcular el centro de la mano espejada
  PVector currentHandCenter = new PVector(mirroredRectX + mirroredRectWidth / 2, mirroredRectY + mirroredRectHeight / 2);

  // Suavizar la posicion del centro de la mano
  if (smoothedHandCenter.x == 0 && smoothedHandCenter.y == 0) {
    smoothedHandCenter.set(currentHandCenter);
  } else {
    smoothedHandCenter.lerp(currentHandCenter, 1.0 - smoothingFactor);
  }

  // Centro de la pantalla
  int centerX = width / 2;
  int centerY = height / 2;

  // Calcular desplazamiento desde el centro
  int offsetX = (int) (smoothedHandCenter.x - centerX);
  int offsetY = (int) (centerY - smoothedHandCenter.y);

  // Enviar coordenadas al ESP32
  sendCoordinates(offsetX, offsetY);

  // Dibujar punto en el centro de la mano
  fill(255, 0, 0);
  noStroke();
  ellipse(smoothedHandCenter.x, smoothedHandCenter.y, 10, 10);

  // Linea al centro de la pantalla
  stroke(255, 255, 0);
  strokeWeight(1);
  line(centerX, centerY, smoothedHandCenter.x, smoothedHandCenter.y);
}

/**
 * Función para mostrar información de estado
 */
void showStatus() {
  fill(255);
  textSize(12);
  
  // Estado de la cámara
  if (cameraReady) {
    fill(0, 255, 0);
    text("Cámara: OK", 20, 30);
  } else {
    fill(255, 255, 0);
    text("Cámara: INICIANDO", 20, 30);
  }
  
  // Estado del Bluetooth
  if (bluetoothConnected && bt != null && bt.active()) {
    fill(0, 255, 0);
    text("Bluetooth: CONECTADO (COM4)", 20, 50);
  } else if (!cameraReady) {
    fill(255, 255, 0);
    text("Bluetooth: Espera cámara, luego presiona 'b'", 20, 50);
  } else {
    fill(255, 0, 0);
    text("Bluetooth: DESCONECTADO - Presiona 'b'", 20, 50);
  }
  
  // Mostrar desplazamiento si hay mano detectada
  if (smoothedHandCenter.x != 0 || smoothedHandCenter.y != 0) {
    int centerX = width / 2;
    int centerY = height / 2;
    int offsetX = (int) (smoothedHandCenter.x - centerX);
    int offsetY = (int) (centerY - smoothedHandCenter.y);
    
    fill(255);
    text("Desplazamiento: X = " + offsetX + ", Y = " + offsetY, 20, 70);
  }
  
  // Instrucciones
  fill(150);
  textSize(10);
  text("Teclas: 'l'=listar puertos, 'b'=conectar BT, 'r'=reiniciar BT, 'c'=reiniciar cámara", 20, height - 20);
  
  // Punto centro pantalla
  fill(0, 255, 255);
  noStroke();
  ellipse(width / 2, height / 2, 8, 8);
}

/**
 * Función para mostrar errores
 */
void showError(String message) {
  fill(255, 0, 0);
  textSize(16);
  text(message, 20, height/2);
  
  fill(255, 255, 0);
  textSize(12);
  text("Teclas disponibles: 'l', 'b', 'r', 'c'", 20, height/2 + 30);
}

/**
 * Función para manejar eventos de teclado
 */
void keyPressed() {
  if (key == 'b' || key == 'B') {
    println("Inicializando Bluetooth...");
    initBluetooth();
  }
  else if (key == 'r' || key == 'R') {
    println("Reiniciando conexión Bluetooth...");
    bluetoothConnected = false;
    if (bt != null) {
      try {
        bt.stop();
        bt = null;
      } catch (Exception e) {
        println("Error al cerrar BT: " + e.getMessage());
      }
    }
    delay(500);
    initBluetooth();
  }
  else if (key == 'c' || key == 'C') {
    println("Reiniciando cámara...");
    cameraReady = false;
    if (cam != null) {
      try {
        cam.stop();
        cam = null;
      } catch (Exception e) {
        println("Error al cerrar cámara: " + e.getMessage());
      }
    }
    delay(1000);
    initializeCamera();
  }
  else if (key == 'l' || key == 'L') {
    println("=== LISTANDO PUERTOS SERIE ===");
    try {
      String[] ports = Serial.list();
      if (ports.length > 0) {
        for (int i = 0; i < ports.length; i++) {
          println(i + ": " + ports[i]);
        }
      } else {
        println("No se encontraron puertos serie.");
      }
    } catch (Exception e) {
      println("Error al listar puertos: " + e.getMessage());
    }
    println("=== FIN LISTADO ===");
  }
}

/**
 * Función que se ejecuta al cerrar el programa
 */
void exit() {
  println("Cerrando programa...");
  
  if (bt != null) {
    try {
      bt.stop();
    } catch (Exception e) {
      println("Error al cerrar Bluetooth: " + e.getMessage());
    }
  }
  
  if (cam != null) {
    try {
      cam.stop();
    } catch (Exception e) {
      println("Error al cerrar cámara: " + e.getMessage());
    }
  }
  
  super.exit();
}