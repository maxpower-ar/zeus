#include "rtc.h"
#include "config.h"
#include <Wire.h>

// Registros del DS3231 (hoja de datos, "Timekeeping Registers").
static const uint8_t REGISTRO_SEGUNDOS = 0x00;  // de aca salen 7 seguidos: seg, min, hora,
                                                // dia de la semana, dia, mes, anio
static const uint8_t REGISTRO_ESTADO = 0x0F;
static const uint8_t BIT_OSF = 0x80;            // "Oscillator Stop Flag": perdio la hora
static const uint8_t BIT_SIGLO = 0x80;          // en el registro del mes
static const uint8_t MASCARA_HORA_24 = 0x3F;    // bit 6 en 0 = modo 24 h
static const int CANTIDAD_REGISTROS_HORA = 7;
static const int ANIO_BASE = 2000;               // el DS3231 guarda el anio como 00..99
static const long SEGUNDOS_POR_DIA = 86400L;

// El DS3231 guarda cada numero en BCD: 0x59 significa 59.
static uint8_t bcdADecimal(uint8_t bcd) { return (bcd >> 4) * 10 + (bcd & 0x0F); }
static uint8_t decimalABcd(uint8_t numero) { return ((numero / 10) << 4) | (numero % 10); }

// Dias desde el 1/1/1970 hasta una fecha (algoritmo de Howard Hinnant, "days_from_civil").
// Hace falta porque la libreria de C del ESP32 no trae timegm().
static long diasDesde1970(int anio, int mes, int dia) {
  anio -= (mes <= 2) ? 1 : 0;
  long era = (anio >= 0 ? anio : anio - 399) / 400;
  long anioDeEra = anio - era * 400;
  long diaDelAnio = (153L * (mes + (mes > 2 ? -3 : 9)) + 2) / 5 + dia - 1;
  long diaDeEra = anioDeEra * 365 + anioDeEra / 4 - anioDeEra / 100 + diaDelAnio;
  return era * 146097L + diaDeEra - 719468L;
}

static bool leerRegistros(uint8_t primero, uint8_t* destino, int cantidad) {
  Wire.beginTransmission(DIRECCION_DS3231);
  Wire.write(primero);
  if (Wire.endTransmission() != 0) return false;
  if (Wire.requestFrom((int)DIRECCION_DS3231, cantidad) != cantidad) return false;
  for (int i = 0; i < cantidad; i++) destino[i] = Wire.read();
  return true;
}

bool rtcResponde() {
  Wire.beginTransmission(DIRECCION_DS3231);
  return Wire.endTransmission() == 0;
}

bool leerRtc(time_t& utc) {
  uint8_t estado;
  if (!leerRegistros(REGISTRO_ESTADO, &estado, 1)) return false;
  if (estado & BIT_OSF) return false;  // se paro el oscilador: la hora no sirve

  uint8_t r[CANTIDAD_REGISTROS_HORA];
  if (!leerRegistros(REGISTRO_SEGUNDOS, r, CANTIDAD_REGISTROS_HORA)) return false;

  int segundos = bcdADecimal(r[0] & 0x7F);
  int minutos = bcdADecimal(r[1] & 0x7F);
  int horas = bcdADecimal(r[2] & MASCARA_HORA_24);
  int dia = bcdADecimal(r[4] & 0x3F);
  int mes = bcdADecimal(r[5] & 0x1F);
  int anio = ANIO_BASE + bcdADecimal(r[6]);

  utc = (time_t)(diasDesde1970(anio, mes, dia) * SEGUNDOS_POR_DIA + horas * 3600L +
                 minutos * 60L + segundos);
  return true;
}

bool escribirRtc(time_t utc) {
  tm f;
  gmtime_r(&utc, &f);

  Wire.beginTransmission(DIRECCION_DS3231);
  Wire.write(REGISTRO_SEGUNDOS);
  Wire.write(decimalABcd(f.tm_sec));
  Wire.write(decimalABcd(f.tm_min));
  Wire.write(decimalABcd(f.tm_hour));     // modo 24 h
  Wire.write(decimalABcd(f.tm_wday + 1));  // 1..7, el DS3231 no le da significado
  Wire.write(decimalABcd(f.tm_mday));
  Wire.write(decimalABcd(f.tm_mon + 1) & ~BIT_SIGLO);
  Wire.write(decimalABcd(f.tm_year + 1900 - ANIO_BASE));
  if (Wire.endTransmission() != 0) return false;

  // Borrar la bandera OSF: desde ahora la hora es buena.
  uint8_t estado;
  if (!leerRegistros(REGISTRO_ESTADO, &estado, 1)) return false;
  Wire.beginTransmission(DIRECCION_DS3231);
  Wire.write(REGISTRO_ESTADO);
  Wire.write(estado & ~BIT_OSF);
  return Wire.endTransmission() == 0;
}
