/*
  PRUEBA DE BANCO - ANEMOMETRO: velocidad del viento en m/s y km/h
  Encoder optico ranurado (FC-03 / LM393) en GPIO18, disco de 30 ranuras.

  Este sketch es el paso siguiente a pruebas/encoder_rueda: aquel imprime vueltas por
  minuto, que es un dato del aparato. Este imprime velocidad del viento, que es el dato
  meteorologico. La diferencia entre los dos es una constante que HAY QUE MEDIR.

  No necesita ninguna libreria.

  CONEXIONADO  (igual que en las otras pruebas del encoder):

    VCC -> 3V3    del ESP32   <-- NO a 5V
    GND -> GND    del ESP32
    DO  -> GPIO18
    AO  -> no se conecta

  Alimentado a 5V el DO del LM393 saldria a ~5V y daña la entrada del ESP32, que es de
  3.3V. El LM393 funciona desde 2V, asi que a 3V3 el modulo anda igual y la salida queda
  en el nivel correcto. GPIO18 y no GPIO36/39 porque esos dos generan pulsos espurios
  cuando el ADC convierte, y aca estamos contando pulsos.


  ====================================================================================
  LO QUE SE MIDE SOLO Y LO QUE HAY QUE CALIBRAR
  ====================================================================================

  Las vueltas por minuto salen EXACTAS de contar ranuras: no hay nada que calibrar ahi,
  siempre que RANURAS_POR_VUELTA sea el numero real de ranuras del disco.

  La velocidad del viento NO sale de ahi sola. Hacen falta dos cosas mas:

    - el RADIO del rotor (del eje al centro de una cazoleta), y
    - el FACTOR del anemometro: las cazoletas giran MAS LENTO que el viento, entre 2.5
      y 3 veces mas lento segun la forma del rotor. Ese numero es empirico, no se
      calcula en el escritorio.

  Este sketch no usa el radio y el factor por separado, porque no se pueden medir por
  separado con precision. Usa el producto de los dos, que es lo que SI se puede medir
  de una sola vez: cuantos metros de viento equivale cada pulso.

      velocidad [m/s] = metrosPorPulso x pulsos_por_segundo

  Una sola constante, un solo experimento. Ver MODO CALIBRACION mas abajo.

  Y una aclaracion sobre las unidades: m/s y km/h son EL MISMO numero multiplicado por
  3.6. Esa conversion es exacta y no depende de ninguna calibracion. Si la velocidad en
  km/h esta mal, la de m/s esta igual de mal: el error estaria en metrosPorPulso, no en
  la conversion. Las dos unidades se imprimen juntas solo por comodidad.


  ====================================================================================
  MODO CALIBRACION  (tecla 'c')
  ====================================================================================

  Hay que exponer el anemometro a una velocidad de viento CONOCIDA y decirle al sketch
  cual es. El metodo practico sin tunel de viento es el auto:

    1. Anemometro firme fuera de la ventanilla, lo mas lejos posible de la carroceria
       (el aire pegado al auto va mas lento que el auto).
    2. Camino recto, dia sin viento, velocidad constante. 30 o 40 km/h esta bien.
    3. Esperar a que el numero de pulsos/s se estabilice (unos 5 segundos).
    4. Escribir en el monitor serie:   c 40      (siendo 40 los km/h del velocimetro)
    5. El sketch calcula metrosPorPulso y lo imprime. Copiarlo a la constante de aca
       abajo, volver a compilar, y anotar el valor y las condiciones en
       hardware/calibraciones/anemometro.md

  Repetirlo a 20, 30, 40 y 50 km/h. Los cuatro valores de metrosPorPulso tienen que dar
  parecidos: si dan parecidos, la relacion es lineal como se espera y se promedia. Si
  dan muy distintos, algo anda mal en el montaje y hay que revisarlo antes de seguir.

  IDA Y VUELTA: hacer cada velocidad en los dos sentidos del camino y promediar. Si hay
  aunque sea poco viento, de ida se suma y de vuelta se resta; promediando se cancela.

  Obvio pero hay que decirlo: maneja otra persona.


  ====================================================================================
  QUE HAY QUE VER EN EL MONITOR SERIE (115200)
  ====================================================================================

    - Rueda quieta: DETENIDO, y el nivel del pin, para distinguir "quieta" de
      "desconectada" (desde el codigo se ven iguales).
    - Soplando o girando con el dedo: km/h y m/s suben juntos, siempre en relacion 3.6.
    - "descartados" solo aparece si hubo flancos demasiado juntos para ser dos ranuras
      distintas. Si aparece seguido hay ruido en el cable o rebote del comparador.

  Teclas:  c <km/h>  calibrar contra una referencia     r  poner contadores en cero

  MIENTRAS metrosPorPulso SIGA SIN CALIBRAR, los km/h y los m/s que imprime este sketch
  son PROVISIONALES y no sirven como medicion. Las vueltas por minuto si sirven.
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

// ---------------------------------------------------------------------------------
// TODO: CALIBRAR - este valor es PROVISIONAL, no esta medido.
//
// Cuantos metros de viento equivale cada pulso del encoder:
//     metrosPorPulso = factor_anemometro x 2 x PI x radio_rotor / RANURAS_POR_VUELTA
//
// El provisional de abajo sale de suponer radio = 0.090 m y factor = 2.5, que son
// valores tipicos pero NO son los de este rotor. Reemplazar por el que devuelva el
// modo calibracion y anotar el experimento en hardware/calibraciones/anemometro.md
// ---------------------------------------------------------------------------------
float metrosPorPulso = 0.0471;

// Dos flancos mas juntos que esto no pueden ser dos ranuras distintas: es ruido o un
// rebote del comparador. Con 30 ranuras, 300 us entre pulsos serian unas 6600 vueltas
// por minuto, muy por encima de lo que puede girar un anemometro de cazoletas: con
// viento de 150 km/h este rotor giraria del orden de 1800 vueltas por minuto.
const unsigned long ANTIRREBOTE_US = 300;

// Cada cuanto se mide, y cuantas mediciones entran en el promedio que se imprime.
// El promedio es corto a proposito: si se promediara mucho mas, una rafaga corta
// quedaria borrada, y justamente las rafagas son un dato meteorologico.
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

float pulsosPorSegundoVentana[VENTANAS_PROMEDIO];
int indicePromedio = 0;
int ventanasCargadas = 0;

// Promedio vigente de pulsos/s. Lo usa el modo calibracion, que necesita el valor
// estable del momento en que se tecleo la referencia.
float pulsosPorSegundoPromedio = 0;

float maximaKmhDesdeCero = 0;

// La rutina de interrupcion tiene que ser lo mas corta posible y vivir en la RAM
// interna (IRAM_ATTR): nada de Serial.print ni delay aca adentro.
void IRAM_ATTR contarPulso() {
  unsigned long ahora = micros();

  if (ahora - ultimoPulsoUs < ANTIRREBOTE_US) {
    descartadosTotales = descartadosTotales + 1;  // rebote o ruido: no puede ser una ranura distinta
    return;
  }

  ultimoPulsoUs = ahora;
  pulsosTotales = pulsosTotales + 1;
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
  pulsosPorSegundoPromedio = 0;
  maximaKmhDesdeCero = 0;
}

// Calcula y muestra la constante que habria que poner para que la velocidad actual
// coincida con la referencia que el usuario dice tener.
void calibrarContra(float referenciaKmh) {
  if (referenciaKmh <= 0) {
    Serial.println(">>> la referencia tiene que ser mayor que cero. Ejemplo:  c 40");
    return;
  }
  if (pulsosPorSegundoPromedio < 1.0) {
    Serial.println(">>> la rueda casi no gira: no hay nada que calibrar todavia.");
    Serial.println("    Esperar a tener pulsos/s estables y repetir.");
    return;
  }

  float referenciaMs = referenciaKmh / 3.6;
  float nuevo = referenciaMs / pulsosPorSegundoPromedio;

  Serial.println();
  Serial.println(">>> CALIBRACION");
  Serial.print("    referencia       = ");
  Serial.print(referenciaKmh, 1);
  Serial.print(" km/h  (");
  Serial.print(referenciaMs, 2);
  Serial.println(" m/s)");
  Serial.print("    pulsos/s medidos = ");
  Serial.println(pulsosPorSegundoPromedio, 2);
  Serial.print("    metrosPorPulso   = ");
  Serial.println(nuevo, 5);
  Serial.println();
  Serial.println("    Copiar ese valor a la constante metrosPorPulso del sketch y");
  Serial.println("    anotar la prueba en hardware/calibraciones/anemometro.md");
  Serial.println("    Repetir a varias velocidades: tienen que dar parecidos.");
  Serial.println();

  // Se aplica en caliente para poder seguir probando sin recompilar. Esto NO queda
  // guardado: al reiniciar la placa vuelve al valor del codigo fuente.
  metrosPorPulso = nuevo;
  Serial.println(">>> aplicado en memoria (se pierde al reiniciar la placa).");
  Serial.println();
}

void leerTeclas() {
  if (Serial.available() <= 0) {
    return;
  }

  String linea = Serial.readStringUntil('\n');
  linea.trim();
  if (linea.length() == 0) {
    return;
  }

  char comando = linea.charAt(0);

  if (comando == 'r' || comando == 'R') {
    ponerEnCero();
    Serial.println(">>> contadores en cero.");
    return;
  }

  if (comando == 'c' || comando == 'C') {
    String resto = linea.substring(1);
    resto.trim();
    calibrarContra(resto.toFloat());
    return;
  }

  Serial.println(">>> teclas:  c <km/h> para calibrar,  r para poner en cero");
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
  Serial.println("=== Anemometro - encoder optico en GPIO18 ===");
  Serial.print("Ranuras por vuelta: ");
  Serial.println(RANURAS_POR_VUELTA);
  Serial.print("metrosPorPulso    : ");
  Serial.print(metrosPorPulso, 5);
  Serial.println("   <-- PROVISIONAL, SIN CALIBRAR");
  Serial.println();
  Serial.println("Las vueltas/min son exactas. Los km/h y m/s NO sirven como medicion");
  Serial.println("hasta calibrar contra una referencia conocida.");
  Serial.println("Teclas:  c <km/h> calibrar    r poner en cero");
  Serial.println();

  momentoVentanaAnterior = millis();
}

void loop() {
  leerTeclas();

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
    pulsosPorSegundoPromedio = 0;

    Serial.print("    0.0 km/h     0.00 m/s   DETENIDO   nivel del pin = ");
    Serial.println(digitalRead(PIN_ENCODER) == HIGH ? "ALTO" : "BAJO");
    return;
  }

  float pulsosPorSegundo = (pulsosVentana * 1000.0) / (float) transcurridoMs;

  pulsosPorSegundoVentana[indicePromedio] = pulsosPorSegundo;
  indicePromedio = (indicePromedio + 1) % VENTANAS_PROMEDIO;
  if (ventanasCargadas < VENTANAS_PROMEDIO) {
    ventanasCargadas++;  // al arrancar se promedia solo lo que ya se midio
  }

  float suma = 0;
  for (int i = 0; i < ventanasCargadas; i++) {
    suma += pulsosPorSegundoVentana[i];
  }
  pulsosPorSegundoPromedio = suma / ventanasCargadas;

  // De pulsos a magnitudes. Las vueltas por minuto son exactas; la velocidad depende
  // de metrosPorPulso, que es lo que hay que calibrar.
  float vueltasPorMinuto = (pulsosPorSegundoPromedio * 60.0) / RANURAS_POR_VUELTA;
  float metrosPorSegundo = pulsosPorSegundoPromedio * metrosPorPulso;
  float kilometrosPorHora = metrosPorSegundo * 3.6;  // exacto, no es una calibracion

  if (kilometrosPorHora > maximaKmhDesdeCero) {
    maximaKmhDesdeCero = kilometrosPorHora;
  }

  // Ancho fijo para que las columnas queden alineadas y se pueda leer de un vistazo
  // como suben y bajan.
  if (kilometrosPorHora < 10) {
    Serial.print("  ");
  } else if (kilometrosPorHora < 100) {
    Serial.print(" ");
  }
  Serial.print(kilometrosPorHora, 1);
  Serial.print(" km/h   ");

  if (metrosPorSegundo < 10) {
    Serial.print(" ");
  }
  Serial.print(metrosPorSegundo, 2);
  Serial.print(" m/s   |  ");

  Serial.print(vueltasPorMinuto, 0);
  Serial.print(" vueltas/min   ");
  Serial.print(pulsosPorSegundoPromedio, 1);
  Serial.print(" pulsos/s   max ");
  Serial.print(maximaKmhDesdeCero, 1);
  Serial.print(" km/h");

  // Solo molesta con esto cuando hay algo que mirar.
  if (descartadosVentana > 0) {
    Serial.print("   descartados = ");
    Serial.print(descartadosVentana);
  }

  Serial.println();
}
