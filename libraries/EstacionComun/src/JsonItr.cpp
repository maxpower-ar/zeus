#include "JsonItr.h"
#include "LineaServidor.h"  // mismos decimales que la linea
#include <stdio.h>
#include <time.h>

static const size_t TAMANO_VALOR_JSON = 16;

// Un numero con punto decimal, o null si no hay dato valido.
static void valorJson(char* destino, size_t tamano, float valor, EstadoDato estado,
                      int decimales) {
  if (estado != DATO_OK || isnan(valor) || isinf(valor)) {
    snprintf(destino, tamano, "null");
  } else {
    snprintf(destino, tamano, "%.*f", decimales, valor);
  }
}

// Agrega "nombre" a la lista de fallas si el dato fallo. Devuelve false si no entro.
static bool anotarSiFallo(char* lista, size_t tamano, const char* nombre, EstadoDato estado) {
  if (estado != DATO_FALLA) return true;
  size_t usado = strlen(lista);
  int escritos = snprintf(lista + usado, tamano - usado, "%s\"%s\"", usado > 0 ? "," : "",
                          nombre);
  return escritos > 0 && (size_t)escritos < tamano - usado;
}

bool armarJsonItr(char* destino, size_t tamano, const char* codigo, const Medicion& medicion) {
  time_t segundos = (time_t)medicion.marcaTiempo;
  tm utc;
  gmtime_r(&segundos, &utc);

  char temperatura[TAMANO_VALOR_JSON];
  char humedad[TAMANO_VALOR_JSON];
  char velocidad[TAMANO_VALOR_JSON];
  char gas[TAMANO_VALOR_JSON];
  char presion[TAMANO_VALOR_JSON];
  char luminosidad[TAMANO_VALOR_JSON];
  valorJson(temperatura, sizeof(temperatura), medicion.temperatura, medicion.estadoTemperatura,
            DECIMALES_TEMPERATURA);
  valorJson(humedad, sizeof(humedad), medicion.humedad, medicion.estadoHumedad,
            DECIMALES_HUMEDAD);
  valorJson(velocidad, sizeof(velocidad), medicion.velocidad, medicion.estadoVelocidad,
            DECIMALES_VELOCIDAD);
  valorJson(gas, sizeof(gas), medicion.gas, medicion.estadoGas, DECIMALES_GAS);
  valorJson(presion, sizeof(presion), medicion.presion, medicion.estadoPresion,
            DECIMALES_PRESION);
  valorJson(luminosidad, sizeof(luminosidad), medicion.luminosidad,
            medicion.estadoLuminosidad, DECIMALES_LUMINOSIDAD);

  // Nombres de los campos tal como los llama su API.
  char fallas[96] = "";
  bool ok = anotarSiFallo(fallas, sizeof(fallas), "temperatura", medicion.estadoTemperatura) &&
            anotarSiFallo(fallas, sizeof(fallas), "humedad", medicion.estadoHumedad) &&
            anotarSiFallo(fallas, sizeof(fallas), "velocidad_viento", medicion.estadoVelocidad) &&
            anotarSiFallo(fallas, sizeof(fallas), "co_concentracion", medicion.estadoGas) &&
            anotarSiFallo(fallas, sizeof(fallas), "presion", medicion.estadoPresion) &&
            anotarSiFallo(fallas, sizeof(fallas), "luminosidad", medicion.estadoLuminosidad);
  if (!ok) return false;

  int escritos = snprintf(
      destino, tamano,
      "{\"codigo\":\"%s\",\"fecha\":\"%04d-%02d-%02dT%02d:%02d:%02dZ\","
      "\"temperatura\":%s,\"humedad\":%s,\"velocidad_viento\":%s,\"co_concentracion\":%s,"
      "\"presion\":%s,\"luminosidad\":%s,\"fallas\":[%s]}",
      codigo, utc.tm_year + 1900, utc.tm_mon + 1, utc.tm_mday, utc.tm_hour, utc.tm_min,
      utc.tm_sec, temperatura, humedad, velocidad, gas, presion, luminosidad, fallas);

  return escritos > 0 && (size_t)escritos < tamano;
}
