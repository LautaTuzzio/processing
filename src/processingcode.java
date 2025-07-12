import gab.opencv.*; // Importa la libreria OpenCV para vision por computadora
import processing.video.*; // Importa la libreria de video para capturar imagenes de la camara
import java.util.*; // Importa utilidades de Java, como ArrayList
import java.awt.Rectangle; // Importa la clase Rectangle de AWT para representar rectangulos

// Variables globales importantes

Capture cam; // Objeto para capturar video de la camara
OpenCV opencv; // Objeto OpenCV para procesamiento de imagenes
PImage objectMask; // Imagen que actuara como mascara para el objeto detectado (el guante)

// Variable para el factor de suavizado del centro de la mano
PVector smoothedHandCenter = new PVector(); // Almacena la posicion suavizada del centro de la mano
// Reducir smoothingFactor para acelerar el movimiento del punto rojo.
float smoothingFactor = 0.3; // Controla que tan rapido el punto rojo sigue el movimiento real de la mano.
                             // Valores mas bajos (ej. 0.1) significan mas suavizado y movimiento mas lento.
                             // Valores mas altos (ej. 0.9) significan menos suavizado y movimiento mas rapido.

/**
 * Funcion setup()
 * Se ejecuta una vez al inicio del programa.
 * Configura el tamaño de la ventana, inicializa la camara y OpenCV.
 */
void setup() {
  size(640, 480); // Establece el tamaño de la ventana de visualizacion en 640x480 pixeles.

  String[] cameras = Capture.list(); // Obtiene una lista de todas las camaras disponibles en el sistema.

  // Intenta usar la camara DroidCam (indice 1).
  // Si no esta disponible o es diferente, ajusta el indice.
  if (cameras.length > 1) { // Si hay mas de una camara disponible
    cam = new Capture(this, cameras[1]); // Intenta inicializar la camara con el indice 1 (DroidCam).
  } else if (cameras.length > 0) { // Si solo hay una camara disponible
    cam = new Capture(this, cameras[0]); // Usa la primera camara (indice 0).
  } else { // Si no se encontraron camaras
    println("No se encontraron camaras."); // Imprime un mensaje de error.
    exit(); // Termina la ejecucion del programa.
  }
  cam.start(); // Inicia la captura de video de la camara.

  opencv = new OpenCV(this, 640, 480); // Inicializa el objeto OpenCV con el mismo tamaño de la ventana.
  objectMask = createImage(640, 480, RGB); // Crea una imagen en blanco para la mascara del objeto.
}

void draw() {
  background(0); // Limpia el fondo de la ventana con negro en cada fotograma.

  if (cam.available()) cam.read(); // Si hay un nuevo fotograma disponible de la camara, lo lee.

  // Espejo horizontal de la camara
  pushMatrix(); // Guarda la configuracion de transformacion actual.
  translate(width, 0); // Mueve el origen de las coordenadas al borde derecho de la ventana.
  scale(-1, 1); // Invierte la escala horizontalmente, creando un efecto de espejo.
  image(cam, 0, 0); // Dibuja el fotograma de la camara en la posicion actual (espejada).
  popMatrix(); // Restaura la configuracion de transformacion anterior.

  // Crear mascara del objeto (guante)
  objectMask.loadPixels(); // Carga los pixeles de la imagen de la mascara para modificarlos.
  cam.loadPixels(); // Carga los pixeles del fotograma actual de la camara.
  for (int i = 0; i < cam.pixels.length; i++) { // Itera sobre cada pixel del fotograma de la camara.
    color c = cam.pixels[i]; // Obtiene el color del pixel actual.
    float r = red(c); // Extrae el componente rojo del color.
    float g = green(c); // Extrae el componente verde del color.
    float b = blue(c); // Extrae el componente azul del color.

    // *******************************************************************
    // CRITICO: AJUSTAR ESTOS VALORES PARA DETECTAR TU GUANTE ESPECIFICO
    // *******************************************************************
    // ** PROBLEMA: Si el guante no se detecta (mascara negra): **
    // 1. Aumenta los limites superiores (e.g., de 50 a 60, 70, etc.).
    // 2. Aumenta el margen de error para la proximidad de RGB (e.g., de 15 a 20, 25).
    // ** PROBLEMA: Si el fondo se detecta como parte del guante (mascara blanca): **
    // 1. Disminuye los limites superiores (e.g., de 50 a 40, 30).
    // 2. Disminuye el margen de error para la proximidad de RGB.
    // *******************************************************************
    // Condiciones para detectar un color especifico (negro/gris oscuro)
    if (r > 0 && r < 50 && // Rango de Rojo (muy bajo para negro)
        g > 0 && g < 50 && // Rango de Verde (muy bajo para negro)
        b > 0 && b < 50 && // Rango de Azul (muy bajo para negro)
        r > (g - 15) && r < (g + 15) && // RGB valores deben ser cercanos entre si para grises/negros
        g > (b - 15) && g < (b + 15)    // Aumentado ligeramente el margen de error para sombras
        ) {
      objectMask.pixels[i] = color(255); // Si el pixel coincide con el color del guante, lo marca como blanco en la mascara.
    } else {
      objectMask.pixels[i] = color(0);    // De lo contrario, lo marca como negro en la mascara.
    }
  }
  objectMask.updatePixels(); // Actualiza la imagen de la mascara con los cambios realizados.

  // *******************************************************************
  // PASO DE DEPURACION CRITICO: DESCOMENTA LA LINEA DE ABAJO PARA VER LA MASCARA
  // Asegurate de que tu guante sea un blob blanco solido y el fondo negro.
  // Una vez que este bien, vuelve a comentarla.
  // *******************************************************************
  // image(objectMask, 0, 0); // Descomentar para depurar la mascara de color. Muestra la mascara en la ventana.

  opencv.loadImage(objectMask); // Carga la mascara de la imagen en OpenCV para su procesamiento.

  // *******************************************************************
  // AJUSTES: OPERACIONES MORFOLOGICAS
  // *******************************************************************
  // ** PROBLEMA: Si el contorno del guante esta roto (agujeros, dedos desconectados): **
  // 1. Añade mas opencv.dilate(); o aumenta el radio del blur.
  // ** PROBLEMA: Si hay mucho ruido, la mascara es "peluda" o conecta cosas que no deberia: **
  // 1. Añade mas opencv.erode(); despues de las dilataciones iniciales.
  // 2. Aumenta el radio del blur.
  // *******************************************************************
  opencv.dilate(); // Dilata la mascara: expande las areas blancas, uniendo pequenos espacios y fortaleciendo la forma.
  opencv.dilate(); // Segunda dilatacion para asegurar la conexion en dedos finos o areas delgadas.
  opencv.erode();    // Erode la mascara: contrae las areas blancas, suavizando bordes y eliminando ruido pequeno.
  opencv.erode();    // Otra erosion, pero no tan agresiva como antes para evitar romper la forma de los dedos.
  opencv.dilate();   // Dilata nuevamente para restaurar el tamaño original de la forma y cerrar cualquier hueco remanente.
  opencv.blur(5);    // Aplica un desenfoque (filtro Gaussiano) con un radio de 5.
                     // Esto suaviza los bordes de la mascara antes de encontrar los contornos, lo que ayuda a obtener contornos mas limpios.
                     // Puedes experimentar con valores como 3, 7, etc., para diferentes niveles de suavizado.

  ArrayList<Contour> contours = opencv.findContours(true, true); // Encuentra los contornos (bordes de los objetos) en la mascara.
                                                                // Los argumentos true, true indican que se busquen contornos externos y se almacene su jerarquia.
  Contour hand = null; // Variable para almacenar el contorno que se identifique como la mano.
  float maxArea = 0; // Variable para encontrar el contorno mas grande.

  // Filtrar contornos para encontrar el guante
  for (Contour c : contours) { // Itera sobre cada contorno encontrado.
    // *******************************************************************
    // AJUSTES: RANGO DE AREA Y RELACION DE ASPECTO
    // *******************************************************************
    // ** PROBLEMA: Si aun dice "No se detecto la mano" a pesar de una buena mascara: **
    // 1. Disminuye el umbral minimo de area (e.g., de 4000 a 2000 o 1000).
    // 2. Aumenta el umbral maximo de area (e.g., de 120000 a 150000, 200000).
    // 3. Amplia el rango de aspectRatio (e.g., de 0.3-3.0 a 0.2-4.0).
    // ** PROBLEMA: Si detecta otros objetos grandes o partes del brazo: **
    // 1. Aumenta el umbral minimo de area.
    // 2. Estrecha el rango de aspectRatio.
    // *******************************************************************
    if (c.area() > 4000 && c.area() < 120000) { // Filtra contornos por area. Se buscan contornos dentro de un rango de tamaño tipico de una mano.
      Rectangle bound = c.getBoundingBox(); // Obtiene el rectangulo que encierra el contorno.
      float aspectRatio = (float)bound.width / bound.height; // Calcula la relacion de aspecto (ancho/alto) del contorno.

      if (aspectRatio > 0.3 && aspectRatio < 3.0) { // Filtra por relacion de aspecto. Se asume que una mano tendra una relacion de aspecto dentro de este rango.
        if (c.area() > maxArea) { // Si el area del contorno actual es mayor que la maxima encontrada hasta ahora
          maxArea = c.area(); // Actualiza la maxima area.
          hand = c; // Asigna este contorno como el candidato a ser la mano.
        }
      }
    }
  }

  if (hand != null && maxArea > 6000) { // Si se encontro un contorno que se considera la mano y su area es suficientemente grande
    stroke(0, 255, 0); // Establece el color del contorno en verde.
    strokeWeight(2); // Establece el grosor de la linea del contorno en 2 pixeles.
    noFill(); // No rellena la forma.

    // Obtener la aproximacion del poligono para el contorno de la mano
    hand.setPolygonApproximationFactor(2); // Simplifica el contorno de la mano a un poligono con un factor de aproximacion de 2.
                                          // Un valor mas bajo resultara en un poligono mas detallado.
    ArrayList<PVector> points = hand.getPolygonApproximation().getPoints(); // Obtiene los puntos del poligono aproximado.

    // Dibujar el contorno espejado
    beginShape(); // Inicia el dibujo de una forma personalizada.
    for (PVector pt : points) { // Itera sobre cada punto del poligono.
      vertex(width - pt.x, pt.y); // Dibuja un vertice, reflejando horizontalmente la coordenada X para que coincida con el espejo de la camara.
    }
    endShape(CLOSE); // Finaliza el dibujo de la forma, cerrandola.

    // Calcular el rectangulo envolvente de la mano *original*
    Rectangle bound = hand.getBoundingBox(); // Obtiene el rectangulo que encierra el contorno de la mano original.

    // Calcular las coordenadas del rectangulo envolvente *espejado*
    int mirroredRectX = width - (bound.x + bound.width); // Calcula la posicion X del rectangulo espejado.
    int mirroredRectY = bound.y; // La posicion Y no cambia al espejar horizontalmente.
    int mirroredRectWidth = bound.width; // El ancho del rectangulo no cambia.
    int mirroredRectHeight = bound.height; // La altura del rectangulo no cambia.

    // Dibujar el rectangulo espejado alrededor de la mano
    stroke(255, 0, 0); // Establece el color del rectangulo en rojo.
    noFill(); // No rellena el rectangulo.
    rect(mirroredRectX, mirroredRectY, mirroredRectWidth, mirroredRectHeight); // Dibuja el rectangulo espejado.

    // Calcular el centro de la mano espejada
    PVector currentHandCenter = new PVector(mirroredRectX + mirroredRectWidth / 2, mirroredRectY + mirroredRectHeight / 2); // Calcula el centro del rectangulo espejado.

    // Suavizar la posicion del centro de la mano
    if (smoothedHandCenter.x == 0 && smoothedHandCenter.y == 0) { // Si es la primera deteccion o el suavizado esta reiniciado
      smoothedHandCenter.set(currentHandCenter); // Inicializa el centro suavizado con la posicion actual.
    } else {
      // Interpola linealmente entre la posicion suavizada anterior y la posicion actual del centro de la mano.
      // 1.0 - smoothingFactor controla la cantidad de suavizado. Un valor mas bajo de smoothingFactor
      // (ej. 0.1) resulta en 1.0 - 0.1 = 0.9, lo que significa que el smoothedHandCenter se movera
      // en un 90% hacia el currentHandCenter en cada fotograma, haciendo el movimiento mas rapido y menos suavizado.
      // Un valor mas alto de smoothingFactor (ej. 0.7) resulta en 1.0 - 0.7 = 0.3, moviendo el
      // smoothedHandCenter solo un 30% hacia el currentHandCenter, resultando en un movimiento mas lento y mas suave.
      smoothedHandCenter.lerp(currentHandCenter, 1.0 - smoothingFactor);
    }

    // Centro de la pantalla
    int centerX = width / 2; // Calcula la coordenada X del centro de la pantalla.
    int centerY = height / 2; // Calcula la coordenada Y del centro de la pantalla.

    // Y invertida: positivo hacia abajo, negativo hacia arriba.
    int offsetX = (int) (smoothedHandCenter.x - centerX); // Calcula el desplazamiento en X desde el centro de la pantalla.
    int offsetY = (int) (centerY - smoothedHandCenter.y); // Calcula el desplazamiento en Y desde el centro de la pantalla, invertido para que Y positivo sea hacia arriba.

    fill(255); // Establece el color del texto en blanco.
    textSize(16); // Establece el tamaño de la fuente del texto en 16 pixeles.
    text("Desplazamiento: X = " + offsetX + ", Y = " + offsetY, 20, 40); // Muestra el desplazamiento X e Y.

    // Dibujar punto en el centro de la mano (suavizado y espejado)
    fill(255, 0, 0); // Establece el color del punto en rojo.
    ellipse(smoothedHandCenter.x, smoothedHandCenter.y, 10, 10); // Dibuja una elipse (circulo) en la posicion suavizada del centro de la mano.

    // Linea al centro de la pantalla
    stroke(255, 255, 0); // Establece el color de la linea en amarillo.
    line(centerX, centerY, smoothedHandCenter.x, smoothedHandCenter.y); // Dibuja una linea desde el centro de la pantalla hasta el centro de la mano suavizado.
  } else {
    fill(255, 0, 0); // Establece el color del texto en rojo.
    textSize(24); // Establece el tamaño de la fuente del texto en 24 pixeles.
    text("No se detecto la mano", 20, 40); // Muestra un mensaje si la mano no es detectada.
    // Reiniciar el centro suavizado si no hay mano detectada
    smoothedHandCenter.set(0, 0, 0); // Reinicia el vector del centro suavizado a cero.
  }

  // Punto centro pantalla
  fill(0, 255, 255); // Establece el color del punto central en cian.
  noStroke(); // No dibuja el borde del punto.
  ellipse(width / 2, height / 2, 8, 8); // Dibuja un circulo pequeño en el centro de la pantalla.
}