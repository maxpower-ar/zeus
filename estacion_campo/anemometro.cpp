#include "anemometro.h"
#include "config.h"

// Compartidas con la interrupcion: van volatile para que el bucle vea siempre el valor real.
static volatile unsigned long pulsosTotales = 0;
static volatile unsigned long ultimoPulsoUs = 0;

static unsigned long pulsosInicioPeriodo = 0;
static unsigned long momentoInicioPeriodo = 0;

// La rutina de interrupcion tiene que ser corta y vivir en la RAM interna (IRAM_ATTR).
static void IRAM_ATTR contarPulso() {
  unsigned long ahora = micros();
  if (ahora - ultimoPulsoUs < ANTIRREBOTE_US) return;  // rebote o ruido
  ultimoPulsoUs = ahora;
  pulsosTotales = pulsosTotales + 1;
}

static unsigned long leerPulsos() {
  noInterrupts();
  unsigned long total = pulsosTotales;
  interrupts();
  return total;
}

void iniciarAnemometro() {
  if (!ANEMOMETRO_INSTALADO) return;
  // INPUT_PULLUP: la salida del LM393 es de colector abierto. FALLING: un pulso por ranura
  // (con CHANGE se contaria el doble).
  pinMode(PIN_ENCODER, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_ENCODER), contarPulso, FALLING);
  descartarPeriodoViento();
}

void descartarPeriodoViento() {
  pulsosInicioPeriodo = leerPulsos();
  momentoInicioPeriodo = millis();
}

void anotarViento(Medicion& medicion, bool cerrarPeriodo) {
  if (!ANEMOMETRO_INSTALADO) return;  // queda "sin sensor"

  unsigned long ahora = millis();
  unsigned long total = leerPulsos();
  unsigned long pulsos = total - pulsosInicioPeriodo;
  float segundos = (ahora - momentoInicioPeriodo) / 1000.0;

  if (cerrarPeriodo) {
    pulsosInicioPeriodo = total;
    momentoInicioPeriodo = ahora;
  }

  // Un periodo de duracion cero no deberia pasar; si pasa, no hay promedio posible: falla.
  float kmh = (segundos > 0) ? (pulsos / segundos) * METROS_POR_PULSO * MS_A_KMH : NAN;
  anotarValor(medicion.velocidad, medicion.estadoVelocidad, kmh);
}

bool nivelPinEncoder() { return digitalRead(PIN_ENCODER) == HIGH; }
