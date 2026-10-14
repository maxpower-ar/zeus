/*
  PRUEBA DE BANCO - VELOCIDAD DE GIRO DE LA RUEDA, encoder optico en GPIO18

  Una sola linea por vez con el dato concreto: a cuantas vueltas por minuto gira.

  CONEXIONADO: no cambia nada respecto de pruebas/encoder_optico.

    VCC -> 3V3    (NO a 5V: el DO del LM393 saldria a 5V y daña la entrada del ESP32)
    GND -> GND
    DO  -> GPIO18
    AO  -> no se conecta

  COMO SE MIDE LA VELOCIDAD. La interrupcion cuenta ranuras y nada mas. Cada VENTANA_MS
  el bucle mira cuantas ranuras pasaron en ese rato y lo convierte a vueltas por minuto:

      vueltas/min = pulsos / RANURAS_POR_VUELTA / (tiempo en minutos)

  POR QUE VA PROMEDIADO. El viento no sopla parejo y la rueda acelera y frena todo el
  tiempo, asi que la medicion de media ventana salta de un numero a otro y no se puede
  leer. Lo que se imprime es el promedio de las ultimas VENTANAS_PROMEDIO ventanas,
  o sea de los ultimos 2 segundos. Eso alcanza para que el numero se quede quieto
  cuando la velocidad es estable, y siga reaccionando cuando cambia de verdad.

  El promedio es de dos segundos a proposito, no mas: si se promediara mucho mas, una
  rafaga corta quedaria borrada y justamente las rafagas son un dato meteorologico.

  RESOLUCION. Con 30 ranuras y ventanas de medio segundo, un solo pulso de diferencia
  son 4 vueltas/min. Girando lento el numero salta de a escalones de ese tamaño: no es
  ruido, es el limite de lo que se puede saber contando ranuras en ese tiempo.

  QUE HAY QUE VER EN EL MONITOR SERIE (115200):

    - Con la rueda quieta: DETENIDO, y el nivel del pin (alto con la ranura libre, bajo
      con la ranura tapada), para distinguir "quieta" de "desconectada".
    - Girando con el dedo parejo: un numero que se mantiene mas o menos estable.
    - Empujando fuerte la rueda y soltandola: el numero sube y despues baja de a poco
      mientras frena sola. Cuanto tarda en llegar a cero es la medida de friccion del
      eje, que es lo que decide entre el cooler Jamicon y el eje del kit TT.
    - "descartados" solo aparece si hubo flancos demasiado juntos para ser ranuras
      distintas. Si aparece seguido, hay rebote del comparador o ruido en el cable.

  Mandar 'r' + Enter pone el contador total en cero.

  El disco es de 30 ranuras, confirmado contra el modelo OpenSCAD que genera la pieza.

  Y vueltas por minuto NO es velocidad del viento: para pasar de una cosa a la otra
  hace falta calibrar contra una referencia conocida. Eso lo hace pruebas/anemometro,
  que imprime m/s y km/h y trae el modo de calibracion. Ese factor no se puede
  inventar, y queda anotado en hardware/calibraciones/anemometro.md
*/

const int PIN_ENCODER = 18;

// Ranuras del disco = pulsos por vuelta. CONFIRMADO contra el modelo OpenSCAD que
// genera la pieza (enc_ranuras = 30). El disco es de 69 mm, 2 mm de espesor, con las
// ranuras entre los radios 23.5 y 33.5 mm.
//
// Si alguna vez se reimprime el disco con otra cantidad de ranuras, HAY QUE CAMBIAR
// este numero: una diferencia aca desplaza toda la escala de velocidad por un factor
// constante, y las lecturas igual siguen pareciendo razonables. Es el error mas
// dificil de detectar de esta fase.
const int RANURAS_POR_VUELTA = 30;

// Dos flancos mas juntos que esto no pueden ser dos ranuras distintas: es ruido o un
// rebote del comparador. Con 30 ranuras, 300 us entre pulsos serian unas 6600 vueltas por
// minuto, muy por encima de lo que puede girar un anemometro de cazoletas.
const unsigned long ANTIRREBOTE_US = 300;

// Cada cuanto se mide, y cuantas mediciones entran en el promedio que se imprime.
const unsigned long VENTANA_MS = 500;
const int VENTANAS_PROMEDIO = 4;  // 4 x 500 ms = 2 segundos de promedio

// Si pasa mas tiempo que esto sin un solo pulso, se considera que la rueda esta quieta.
const unsigned long TIEMPO_DETENIDO_MS = 2000;

// Compartidas entre la interrupcion y el bucle: van volatile, si no el compilador
// podria cachearlas en un registro y el bucle nunca veria el cambio.
volatile unsigned long pulsosTotales = 0;
volatile unsigned long descartadosTotales = 0;
volatile unsigned long ultimoPulsoUs = 0;

unsigned long pulsosVentanaAnterior = 0;
unsigned long descartadosVentanaAnterior = 0;
unsigned long momentoVentanaAnterior = 0;
unsigned long momentoUltimoPulsoMs = 0;

float vueltasPorVentana[VENTANAS_PROMEDIO];
int indicePromedio = 0;
int ventanasCargadas = 0;

// La rutina de interrupcion tiene que ser lo mas corta posible y vivir en la RAM
// interna (IRAM_ATTR): nada de Serial.print ni delay aca adentro.
void IRAM_ATTR contarPulso() {
  unsigned long ahora = micros();

  if (ahora - ultimoPulsoUs < ANTIRREBOTE_US) {
    descartadosTotales++;  // rebote o ruido: no puede ser una ranura distinta
    return;
  }

  ultimoPulsoUs = ahora;
  pulsosTotales++;
}

void ponerEnCero() {
  noInterrupts();
  pulsosTotales = 0;
  descartadosTotales = 0;
  interrupts();

  pulsosVentanaAnterior = 0;
  descartadosVentanaAnterior = 0;
  indicePromedio = 0;
  ventanasCargadas = 0;
}

void setup() {
  Serial.begin(115200);
  delay(500);

  // INPUT_PULLUP porque la salida del LM393 suele ser de colector abierto: sin pull-up
  // el pin quedaria flotando y contaria pulsos fantasma.
  pinMode(PIN_ENCODER, INPUT_PULLUP);

  // FLANCO DE BAJADA, no CHANGE: cada ranura produce una bajada y una subida, asi que
  // con CHANGE contariamos el doble de ranuras de las que tiene el disco.
  attachInterrupt(digitalPinToInterrupt(PIN_ENCODER), contarPulso, FALLING);

  ponerEnCero();

  Serial.println();
  Serial.println("=== Velocidad de giro - encoder optico en GPIO18 ===");
  Serial.print("Ranuras por vuelta asumidas: ");
  Serial.print(RANURAS_POR_VUELTA);
  Serial.println("  (SIN VERIFICAR - la escala es provisional)");
  Serial.println("Promedio de los ultimos 2 segundos. 'r' + Enter pone en cero.");
  Serial.println();

  momentoVentanaAnterior = millis();
}

void loop() {
  if (Serial.available() > 0) {
    char tecla = Serial.read();
    if (tecla == 'r' || tecla == 'R') {
      ponerEnCero();
      Serial.println(">>> contador en cero.");
    }
  }

  unsigned long ahora = millis();
  if (ahora - momentoVentanaAnterior < VENTANA_MS) {
    return;  // todavia no se cumplio la ventana; el bucle sigue girando sin delay()
  }

  // Se copian las variables compartidas con las interrupciones desactivadas, para que
  // no cambien en el medio de la lectura y quede una mezcla de dos instantes distintos.
  noInterrupts();
  unsigned long total = pulsosTotales;
  unsigned long descartados = descartadosTotales;
  interrupts();

  unsigned long transcurridoMs = ahora - momentoVentanaAnterior;
  unsigned long pulsosVentana = total - pulsosVentanaAnterior;
  unsigned long descartadosVentana = descartados - descartadosVentanaAnterior;

  momentoVentanaAnterior = ahora;
  pulsosVentanaAnterior = total;
  descartadosVentanaAnterior = descartados;

  // Cuanto hace que no llega un pulso, medido siempre en millis(). No se compara contra
  // ultimoPulsoUs: micros() se desborda cada 71 minutos y millis() no, asi que mezclar
  // los dos relojes daria un "DETENIDO" falso una vez por hora.
  if (pulsosVentana > 0) {
    momentoUltimoPulsoMs = ahora;
  }

  if (ahora - momentoUltimoPulsoMs > TIEMPO_DETENIDO_MS) {
    // Quieta: se vacia el promedio para que al arrancar de nuevo no arrastre el valor
    // viejo, y se imprime el nivel del pin, porque "quieta" y "desconectada" se ven
    // iguales desde el codigo.
    ventanasCargadas = 0;
    indicePromedio = 0;

    Serial.print("     0 vueltas/min   DETENIDO   nivel del pin = ");
    Serial.println(digitalRead(PIN_ENCODER) == HIGH ? "ALTO" : "BAJO");
    return;
  }

  // Vueltas por minuto de esta ventana: pulsos -> vueltas -> por minuto.
  float vueltasPorMinuto = (pulsosVentana * 60000.0) /
                           (RANURAS_POR_VUELTA * (float) transcurridoMs);

  vueltasPorVentana[indicePromedio] = vueltasPorMinuto;
  indicePromedio = (indicePromedio + 1) % VENTANAS_PROMEDIO;
  if (ventanasCargadas < VENTANAS_PROMEDIO) {
    ventanasCargadas++;  // al arrancar se promedia solo lo que ya se midio
  }

  float suma = 0;
  for (int i = 0; i < ventanasCargadas; i++) {
    suma += vueltasPorVentana[i];
  }
  float promedio = suma / ventanasCargadas;

  // Ancho fijo para que la columna de numeros quede alineada y se pueda leer de un
  // vistazo como sube y baja.
  if (promedio < 10) {
    Serial.print("   ");
  } else if (promedio < 100) {
    Serial.print("  ");
  } else if (promedio < 1000) {
    Serial.print(" ");
  }

  Serial.print(promedio, 0);
  Serial.print(" vueltas/min   (");
  Serial.print(promedio / 60.0, 1);
  Serial.print(" por segundo)");

  // Solo molesta con esto cuando hay algo que mirar.
  if (descartadosVentana > 0) {
    Serial.print("   descartados = ");
    Serial.print(descartadosVentana);
  }

  Serial.println();
}
