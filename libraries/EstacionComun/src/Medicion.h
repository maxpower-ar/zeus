#pragma once

// Una medicion completa de la estacion. Es lo que se guarda en la memoria flash y lo que
// despues se traduce a la linea del servidor (LineaServidor.h).
//
// Version simple, con float: un dato faltante se guarda como NAN, que es exactamente lo
// que viaja al servidor ("NaN") y nunca se confunde con un cero. La version compacta con
// enteros de docs/formato-datos.md queda para cuando el espacio sea un problema.

#include <Arduino.h>
#include <math.h>

const size_t TAMANO_DIRECCION = 4;  // "NO" + terminador, con margen

struct Medicion {
  uint32_t marcaTiempo;              // segundos desde el 1/1/1970 UTC (epoca Unix)
  float temperatura;                 // °C
  float humedad;                     // %
  float velocidad;                   // km/h, promedio desde la medicion anterior
  float gas;                         // unidad a definir con el sensor
  float presion;                     // hPa
  float luminosidad;                 // lux
  char direccion[TAMANO_DIRECCION];  // "N", "NE" ... "NO"; vacio = sin dato
};

// Una medicion con la hora puesta y todos los datos en "sin dato". Se parte de aca y se
// completan solo los sensores que respondieron.
inline Medicion medicionVacia(uint32_t marcaTiempo) {
  Medicion m;
  m.marcaTiempo = marcaTiempo;
  m.temperatura = NAN;
  m.humedad = NAN;
  m.velocidad = NAN;
  m.gas = NAN;
  m.presion = NAN;
  m.luminosidad = NAN;
  m.direccion[0] = '\0';
  return m;
}
