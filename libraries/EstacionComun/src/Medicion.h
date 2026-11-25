#pragma once

// Una medicion completa de la estacion. Es lo que se guarda en la memoria flash y lo que
// despues se traduce a la linea del servidor (LineaServidor.h).
//
// Cada dato lleva su valor (float) Y su ESTADO. El estado distingue tres casos que un NaN
// solo no puede distinguir:
//
//   DATO_OK          el sensor respondio: el valor es un numero real
//   DATO_SIN_SENSOR  el sensor no esta instalado. Es esperable.         -> "NaN" al servidor
//   DATO_FALLA       el sensor SI esta instalado pero la lectura fallo  -> "ERR" al servidor
//                    (no responde, CRC del DHT22, BMP180 no detectado). Hay que ir a revisar.
//
// Nunca se usa -1 ni ningun otro numero como marca: cualquier numero puede ser un dato real
// (-1 C existe). Cuando el estado no es DATO_OK, el valor queda en NAN y no significa nada.

#include <Arduino.h>
#include <math.h>

// Version del formato binario de Medicion. SUBIRLA cada vez que cambie el struct: el
// registro en flash compara esta version y descarta lo guardado con otra, en vez de leerlo mal.
//   1 (2026-11-22): datos en float, NAN = sin dato
//   2 (2026-11-22): estado por campo (ok / sin sensor / falla)
const uint16_t VERSION_MEDICION = 2;

enum EstadoDato : uint8_t {
  DATO_OK = 0,
  DATO_SIN_SENSOR = 1,
  DATO_FALLA = 2,
};

const size_t TAMANO_DIRECCION = 4;  // "NO" + terminador, con margen

struct Medicion {
  uint32_t marcaTiempo;              // segundos desde el 1/1/1970 UTC (epoca Unix)
  float temperatura;                 // °C
  float humedad;                     // %
  float velocidad;                   // km/h, promedio desde la medicion anterior
  float gas;                         // ppm de CO (MQ-7)
  float presion;                     // hPa, presion absoluta de la estacion
  float luminosidad;                 // lux
  char direccion[TAMANO_DIRECCION];  // "N", "NE" ... "NO"

  EstadoDato estadoTemperatura;
  EstadoDato estadoHumedad;
  EstadoDato estadoVelocidad;
  EstadoDato estadoGas;
  EstadoDato estadoPresion;
  EstadoDato estadoLuminosidad;
  EstadoDato estadoDireccion;
  uint8_t reservado;  // relleno a proposito, siempre 0: el tamano queda en 40 bytes exactos
};

// Si alguien agrega un campo y se olvida de subir VERSION_MEDICION, esto lo frena al compilar.
static_assert(sizeof(Medicion) == 40, "Cambio el tamano de Medicion: subir VERSION_MEDICION "
                                      "y actualizar este control");

// Una medicion con la hora puesta y TODOS los datos en "sin sensor". Se parte de aca y se
// completan solo los sensores instalados.
inline Medicion medicionVacia(uint32_t marcaTiempo) {
  Medicion m;
  memset(&m, 0, sizeof(m));
  m.marcaTiempo = marcaTiempo;
  m.temperatura = NAN;
  m.humedad = NAN;
  m.velocidad = NAN;
  m.gas = NAN;
  m.presion = NAN;
  m.luminosidad = NAN;
  m.direccion[0] = '\0';
  m.estadoTemperatura = DATO_SIN_SENSOR;
  m.estadoHumedad = DATO_SIN_SENSOR;
  m.estadoVelocidad = DATO_SIN_SENSOR;
  m.estadoGas = DATO_SIN_SENSOR;
  m.estadoPresion = DATO_SIN_SENSOR;
  m.estadoLuminosidad = DATO_SIN_SENSOR;
  m.estadoDireccion = DATO_SIN_SENSOR;
  return m;
}

// Anota lo que devolvio un sensor INSTALADO. Si no es un numero (la libreria devolvio NaN),
// es una falla: el sensor esta pero no contesto bien.
inline void anotarValor(float& campo, EstadoDato& estado, float valor) {
  if (isnan(valor) || isinf(valor)) {
    campo = NAN;
    estado = DATO_FALLA;
  } else {
    campo = valor;
    estado = DATO_OK;
  }
}

// Un sensor instalado que no respondio.
inline void anotarFalla(float& campo, EstadoDato& estado) {
  campo = NAN;
  estado = DATO_FALLA;
}

// Para los mensajes del monitor serie.
inline const char* textoEstado(EstadoDato estado) {
  switch (estado) {
    case DATO_OK: return "ok";
    case DATO_SIN_SENSOR: return "sin sensor";
    case DATO_FALLA: return "FALLA";
  }
  return "?";
}
