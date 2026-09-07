// Configuracion del nodo de campo.
// Todo numero que dependa del hardware o del armado va aca, nunca dentro de la logica.
// Asignacion de pines: ver docs/hardware/pinout.md

#ifndef CONFIG_H
#define CONFIG_H

// ---------------- General ----------------

const unsigned long VELOCIDAD_SERIE = 115200;

// LED integrado de la placa DevKit V1. Es GPIO2, un pin de strapping:
// como salida hacia el LED no molesta, pero no conectar sensores ahi.
const int PIN_LED = 2;

const unsigned long LED_ENCENDIDO_MS = 200;
const unsigned long LED_APAGADO_MS   = 1300;

// ---------------- Sensores ----------------

// DHT22: dato en GPIO4, con pull-up de 10k a 3.3 V.
const int PIN_DHT = 4;

// LM35Z: salida analogica directa a ADC1 (nunca ADC2: no funciona con la radio encendida).
const int PIN_LM35_1 = 34;
const int PIN_LM35_2 = 35;

// El LM35 entrega 10 mV por grado. En el rango util del proyecto (0 a 100 C) eso son
// 0 a 1000 mV, asi que conviene el ADC en su escala mas chica (0 db, ~0 a 1100 mV):
// se usa todo el conversor para la zona que interesa en vez de desperdiciarlo hasta 3.3 V.
const float LM35_MV_POR_GRADO = 10.0;

// El ADC del ESP32 es ruidoso: se promedian muchas lecturas por medicion.
const int LM35_MUESTRAS = 32;

// Limites de plausibilidad. Fuera de esta ventana la lectura se marca invalida
// (sensor desconectado, al reves, o mal alimentado).
const float LM35_MV_MINIMO = 20.0;    // ~2 C
const float LM35_MV_MAXIMO = 1050.0;  // ~105 C

// TODO: calibrar - desvio de cada LM35 contra el multimetro UT890C.
// Provisional en 0.0 hasta hacer la medicion de la fase 1 y anotarla
// en hardware/calibraciones/. NO son valores medidos.
const float LM35_1_OFFSET_C = 0.0;
const float LM35_2_OFFSET_C = 0.0;

// El DHT22 no admite mas de una lectura cada 2 segundos.
const unsigned long INTERVALO_MUESTREO_MS = 2500;

#endif
