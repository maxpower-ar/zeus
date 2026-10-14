/*
  PRUEBA DE BANCO - ENCODER OPTICO RANURADO EN U (FC-03 / LM393) en GPIO18

  Es el sensor del anemometro: un LED infrarrojo de un lado de la U, un fototransistor
  del otro, y un comparador LM393 que limpia la señal. Cada ranura de la rueda corta el
  haz y produce un pulso. La frecuencia de pulsos es proporcional a la velocidad de giro.

  No necesita ninguna libreria.

  CONEXIONADO  (leer el encabezado entero antes de enchufar nada):

    VCC -> 3V3    del ESP32   <-- NO a 5V, ver la advertencia de abajo
    GND -> GND    del ESP32
    DO  -> GPIO18
    AO  -> no se conecta (no aporta nada util en este modulo)

  ADVERTENCIA DE TENSION. El LM393 entrega en DO casi la misma tension con la que se lo
  alimenta. Alimentado a 5V, DO entrega ~5V y eso daña la entrada del ESP32, que es de
  3.3V. Por eso se alimenta a 3V3: el LM393 funciona desde 2V, asi que el modulo anda
  igual y la salida queda en 3.3V, que es lo que el ESP32 espera.

  ANTES de conectar DO al GPIO18, medir con el UT890C entre DO y GND, con el modulo ya
  alimentado a 3V3: tapando y destapando la ranura con un papel, la tension tiene que
  saltar entre ~0V y ~3.3V. Si en alto midiera mas de 3.3V, NO conectarlo: haria falta
  el conversor de nivel.

  POR QUE GPIO18 y no cualquier otro: contamos pulsos, y GPIO36 (VP) y GPIO39 (VN)
  generan pulsos espurios de nanosegundos cuando el ADC convierte. Cada glitch se
  contaria como una ranura que nunca paso. GPIO18 es un pin digital comun y limpio.

  QUE HAY QUE VER EN EL MONITOR SERIE (115200):

    1. Con la ranura libre, el sketch imprime "DETENIDO". Metiendo y sacando un papel
       de la ranura, el contador total tiene que subir de a uno por vez.
    2. Girando la rueda con la mano aparecen pulsos/s y las vueltas por minuto.
    3. MODO CONTAR RANURAS: el sketch arranca en este modo. El total de pulsos se
       reinicia solo cuando lo pedis con la tecla 'r' + Enter. Poner una marca en la
       rueda, mandar 'r', dar EXACTAMENTE una vuelta despacio con el dedo, y leer el
       total: ese es el numero de ranuras del disco. Anotarlo en RANURAS_POR_VUELTA
       aca abajo y en hardware/calibraciones/anemometro.md.

  El disco es de 30 ranuras, confirmado contra el modelo OpenSCAD que genera la pieza.
  El modo de conteo sirve igual como verificacion fisica, y para cualquier disco nuevo.

  Para medir velocidad de viento en m/s y km/h, y no solo vueltas por minuto, usar
  pruebas/anemometro.
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

// Cada cuanto se informa por el monitor serie.
const unsigned long INTERVALO_INFORME_MS = 1000;

// Si pasa mas tiempo que esto sin un solo pulso, se considera que la rueda esta quieta.
const unsigned long TIEMPO_DETENIDO_MS = 2000;

// Estas tres las escribe la interrupcion y las lee el bucle principal: van volatile,
// si no el compilador podria cachearlas en un registro y el bucle nunca veria el cambio.
volatile unsigned long pulsosTotales = 0;
volatile unsigned long ultimoPulsoUs = 0;
volatile unsigned long periodoUs = 0;  // tiempo entre los dos ultimos pulsos validos

unsigned long pulsosDelInformeAnterior = 0;
unsigned long momentoInformeAnterior = 0;
unsigned long momentoUltimoPulsoMs = 0;

// La rutina de interrupcion tiene que ser lo mas corta posible y vivir en la RAM
// interna (IRAM_ATTR): nada de Serial.print ni delay aca adentro.
void IRAM_ATTR contarPulso() {
  unsigned long ahora = micros();
  unsigned long transcurrido = ahora - ultimoPulsoUs;

  if (transcurrido < ANTIRREBOTE_US) {
    return;  // demasiado junto al anterior: es ruido, no una ranura
  }

  periodoUs = transcurrido;
  ultimoPulsoUs = ahora;
  pulsosTotales++;
}

void setup() {
  Serial.begin(115200);
  delay(500);

  // INPUT_PULLUP porque la salida del LM393 suele ser de colector abierto: sin pull-up
  // el pin quedaria flotando y contaria pulsos fantasma. Si el modulo ya trae su propia
  // resistencia, el pull-up interno no molesta.
  pinMode(PIN_ENCODER, INPUT_PULLUP);

  // FLANCO DE BAJADA, no CHANGE: cada ranura produce una bajada y una subida, asi que
  // con CHANGE contariamos el doble de ranuras de las que tiene el disco.
  attachInterrupt(digitalPinToInterrupt(PIN_ENCODER), contarPulso, FALLING);

  Serial.println();
  Serial.println("=== Prueba encoder optico en GPIO18 ===");
  Serial.print("Ranuras por vuelta asumidas: ");
  Serial.print(RANURAS_POR_VUELTA);
  Serial.println("  (SIN VERIFICAR - las vueltas/min son provisionales)");
  Serial.println("Meter y sacar un papel de la ranura: el total sube de a uno.");
  Serial.println("Mandar 'r' + Enter para poner el total en cero y contar las ranuras");
  Serial.println("de una vuelta completa.");
  Serial.println();

  momentoInformeAnterior = millis();
}

void loop() {
  // Reinicio del contador a pedido, para el modo "contar ranuras".
  if (Serial.available() > 0) {
    char tecla = Serial.read();
    if (tecla == 'r' || tecla == 'R') {
      noInterrupts();
      pulsosTotales = 0;
      interrupts();
      pulsosDelInformeAnterior = 0;
      Serial.println(">>> total en cero. Dar UNA vuelta completa y leer el total.");
    }
  }

  unsigned long ahora = millis();
  if (ahora - momentoInformeAnterior < INTERVALO_INFORME_MS) {
    return;  // todavia no toca informar; el bucle sigue girando sin delay()
  }

  // Se copian las variables compartidas con las interrupciones desactivadas, para que
  // no cambien en el medio de la lectura y quede una mezcla de dos instantes distintos.
  noInterrupts();
  unsigned long total = pulsosTotales;
  unsigned long periodo = periodoUs;
  interrupts();

  unsigned long transcurridoMs = ahora - momentoInformeAnterior;
  unsigned long pulsosVentana = total - pulsosDelInformeAnterior;

  momentoInformeAnterior = ahora;
  pulsosDelInformeAnterior = total;

  // Cuanto hace que no llega un pulso, medido siempre en millis(). No se compara contra
  // ultimoPulsoUs: micros() se desborda cada 71 minutos y millis() no, asi que mezclar
  // los dos relojes daria un "DETENIDO" falso una vez por hora.
  if (pulsosVentana > 0) {
    momentoUltimoPulsoMs = ahora;
  }
  bool detenido = (total == 0) || (ahora - momentoUltimoPulsoMs > TIEMPO_DETENIDO_MS);

  Serial.print("total = ");
  Serial.print(total);

  if (detenido) {
    // Ojo: "detenido" y "desconectado" se ven igual desde el codigo. Por eso se imprime
    // tambien el nivel del pin: con el modulo conectado y la ranura libre tiene que
    // leerse un nivel estable, no saltar solo.
    Serial.print("   DETENIDO (sin pulsos)   nivel del pin = ");
    Serial.println(digitalRead(PIN_ENCODER) == HIGH ? "ALTO" : "BAJO");
  } else {
    float pulsosPorSegundo = (pulsosVentana * 1000.0) / transcurridoMs;
    float vueltasPorMinuto = (pulsosPorSegundo * 60.0) / RANURAS_POR_VUELTA;

    Serial.print("   pulsos/s = ");
    Serial.print(pulsosPorSegundo, 1);
    Serial.print("   vueltas/min = ");
    Serial.print(vueltasPorMinuto, 1);
    Serial.print("   periodo entre pulsos = ");
    Serial.print(periodo);
    Serial.println(" us");
  }
}
