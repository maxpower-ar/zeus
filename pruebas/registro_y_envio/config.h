#pragma once

// Configuracion de la prueba registro_y_envio. Pines, intervalos y calibraciones: ningun
// numero de estos va escrito dentro de la logica.

#include <Arduino.h>

const char* const CODIGO_ESTACION = "EM_ITR_001";

// ---------------------------------------------------------------- pines (docs/hardware/pinout.md)
const int PIN_DHT = 4;
const int PIN_ENCODER = 18;  // nunca 36 ni 39: generan pulsos espurios cuando el ADC convierte

// ---------------------------------------------------------------- tiempos
// 1 minuto para probar; la estacion real: 5 minutos. NO bajar de 60 s: el servidor
// identifica cada medicion por codigo + fecha + hora:minuto, y dos mediciones en el mismo
// minuto se tomarian como duplicadas.
const unsigned long INTERVALO_MEDICION_MS = 60000;

// Si un envio falla, cuanto esperar antes de reintentar. Mientras tanto se sigue midiendo.
const unsigned long INTERVALO_REINTENTO_MS = 30000;

const unsigned long INTERVALO_AVISO_WIFI_MS = 10000;
const uint16_t TIEMPO_ESPERA_HTTP_MS = 15000;

// ---------------------------------------------------------------- registro en memoria
// Cuantas mediciones entran en la memoria flash. Cada una ocupa 44 bytes (40 + CRC +
// relleno), asi que 20000 son ~880 KB, algo mas de la mitad de la particion de datos.
// A 1 por minuto son ~14 dias sin red; a 1 cada 5 minutos, ~69 dias. Si se llena, la mas
// vieja cede su lugar.
const uint32_t CAPACIDAD_REGISTROS = 20000;

// Cuantas mediciones pendientes se mandan juntas en un solo envio, despues de un corte.
const int LOTE_MAXIMO = 20;

// ---------------------------------------------------------------- hora
// Argentina: UTC-3 todo el ano. En la notacion POSIX el signo va al reves.
const char* const ZONA_HORARIA = "<-03>3";
const char* const SERVIDOR_NTP_1 = "pool.ntp.org";
const char* const SERVIDOR_NTP_2 = "time.google.com";
const int ANIO_MINIMO_VALIDO = 2025;  // un reloj que marca antes de esto todavia no sincronizo

// ---------------------------------------------------------------- anemometro
const int RANURAS_POR_VUELTA = 30;  // confirmado contra el modelo OpenSCAD del disco

// TODO: calibrar - PROVISIONAL, no esta medido (ver pruebas/anemometro y
// hardware/calibraciones/anemometro.md). Mientras no se calibre, los km/h son aproximados.
const float METROS_POR_PULSO = 0.0471;

// Dos flancos mas juntos que esto no pueden ser dos ranuras distintas: es ruido o rebote.
const unsigned long ANTIRREBOTE_US = 300;

const float MS_A_KMH = 3.6;  // conversion exacta, no es una calibracion
