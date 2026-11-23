#pragma once

// Configuracion de la estacion (placa Delta). Pines, intervalos, sensores instalados y
// calibraciones: ningun numero de estos va escrito dentro de la logica.
// Asignacion de pines: docs/hardware/pinout.md. Lo secreto (WiFi, claves) va en secretos.h.

#include <Arduino.h>

const char* const CODIGO_ESTACION = "EM_ITR_001";
const unsigned long VELOCIDAD_SERIE = 115200;

// ================================================================ que hay instalado
// true  = el sensor ESTA conectado. Si no responde, el dato sale como ERR (falla): hay que
//         ir a revisar el cable o el sensor.
// false = el sensor NO esta. El dato sale como NaN, que es lo esperable.
//
// Gas, direccion del viento y luminosidad no tienen sensor ni codigo todavia: siempre NaN.
const bool DHT22_INSTALADO = true;
const bool ANEMOMETRO_INSTALADO = true;
const bool BMP180_INSTALADO = false;  // EN CAMINO: cuando llegue, poner true y recompilar
const bool PANTALLA_INSTALADA = true;
const bool RTC_INSTALADO = false;     // DS3231 todavia no comprado (ver rtc.h)

// ================================================================ a donde se manda
const bool ENVIAR_A_SHEETS = true;
// DESACTIVADO: su API todavia no acepta null en co_concentracion ni recibe la fecha de la
// medicion (bitacora 2026-11-22). Activarlo cuando lo resuelvan. La clave va en secretos.h.
const bool ENVIAR_A_ITR = false;
const char* const URL_ITR = "https://estacionmeteorologica.pythonanywhere.com/api/lecturas";

// ================================================================ pines (docs/hardware/pinout.md)
const int PIN_DHT = 4;
const int PIN_ENCODER = 18;  // nunca 36 ni 39: generan pulsos espurios cuando el ADC convierte
const int PIN_I2C_SDA = 21;  // bus I2C compartido: BMP180, pantalla y DS3231
const int PIN_I2C_SCL = 22;

// 400 kHz: la pantalla se redibuja en ~25 ms. Si con cables largos aparecen fallas de I2C,
// bajar a 100000 (la pantalla tarda ~100 ms por dibujo, sigue andando).
const uint32_t FRECUENCIA_I2C_HZ = 400000;

const uint8_t DIRECCION_BMP180 = 0x77;  // fija, no se puede cambiar
const uint8_t DIRECCION_PANTALLA = 0x3C;
const uint8_t DIRECCION_DS3231 = 0x68;

// ================================================================ tiempos
// Cada cuanto se mide. 5 minutos en la estacion; para probar, 1 * 60.
// Las mediciones caen alineadas al reloj: con 5 minutos, a las 10:00, 10:05, 10:10...
// NUNCA menos de 60 s: el servidor identifica cada medicion por codigo + fecha + hora:minuto,
// y dos mediciones en el mismo minuto se tomarian como duplicadas.
const uint32_t INTERVALO_MEDICION_S = 5 * 60;
static_assert(INTERVALO_MEDICION_S >= 60, "El intervalo de medicion no puede ser menor a 60 s");

// Si un envio falla, cuanto esperar antes de reintentar. Mientras tanto se sigue midiendo.
const unsigned long INTERVALO_REINTENTO_MS = 30000;

const unsigned long INTERVALO_AVISO_WIFI_MS = 30000;
const uint16_t TIEMPO_ESPERA_HTTP_MS = 15000;
const unsigned long INTERVALO_PANTALLA_MS = 1000;

// ================================================================ registro en memoria
// Cuantas mediciones entran en la memoria flash. Cada una ocupa 44 bytes en el archivo (40 de
// la medicion, 1 de CRC y 3 de relleno que agrega el compilador), asi que 20000 son ~880 KB,
// algo mas de la mitad de la particion de datos. A 1 cada 5 minutos son ~69 dias
// sin red (a 1 por minuto, ~14). Si se llena, la mas vieja cede su lugar.
const uint32_t CAPACIDAD_REGISTROS = 20000;

// Cuantas pendientes se mandan juntas en un solo envio a la planilla, despues de un corte.
const int LOTE_MAXIMO_SHEETS = 20;

// ================================================================ hora
// Argentina: UTC-3 todo el ano. En la notacion POSIX el signo va al reves.
const char* const ZONA_HORARIA = "<-03>3";
const char* const SERVIDOR_NTP_1 = "pool.ntp.org";
const char* const SERVIDOR_NTP_2 = "time.google.com";
const int ANIO_MINIMO_VALIDO = 2025;  // un reloj que marca antes de esto todavia no sincronizo

// ================================================================ anemometro
const int RANURAS_POR_VUELTA = 30;  // confirmado contra el modelo OpenSCAD del disco

// TODO: calibrar - PROVISIONAL, no esta medido (ver pruebas/anemometro y
// hardware/calibraciones/anemometro.md). Mientras no se calibre, los km/h son aproximados.
const float METROS_POR_PULSO = 0.0471;

// Dos flancos mas juntos que esto no pueden ser dos ranuras distintas: es ruido o rebote.
const unsigned long ANTIRREBOTE_US = 300;

const float MS_A_KMH = 3.6;  // conversion exacta, no es una calibracion

// ================================================================ limites de plausibilidad
// Rango que el fabricante garantiza para cada sensor. Una lectura fuera de esto no puede ser
// real: se anota como FALLA (ERR), nunca se recorta ni se corrige.
const float DHT22_TEMPERATURA_MINIMA = -40.0;
const float DHT22_TEMPERATURA_MAXIMA = 80.0;
const float DHT22_HUMEDAD_MINIMA = 0.0;
const float DHT22_HUMEDAD_MAXIMA = 100.0;
const float BMP180_PRESION_MINIMA_HPA = 300.0;
const float BMP180_PRESION_MAXIMA_HPA = 1100.0;

const float PA_A_HPA = 0.01;  // 1 hPa = 100 Pa, conversion exacta
