#include "LineaServidor.h"
#include <stdio.h>
#include <time.h>

const size_t TAMANO_VALOR = 16;

void formatearDato(char* destino, size_t tamano, float valor, EstadoDato estado, int decimales) {
  if (estado == DATO_SIN_SENSOR) {
    snprintf(destino, tamano, "%s", TEXTO_SIN_SENSOR);
    return;
  }
  // Una falla, o un "ok" que igual no es un numero (no deberia pasar): se avisa como falla,
  // para que alguien lo mire. Nunca se disfraza de cero ni de "sin sensor".
  if (estado != DATO_OK || isnan(valor) || isinf(valor)) {
    snprintf(destino, tamano, "%s", TEXTO_FALLA);
    return;
  }
  snprintf(destino, tamano, "%.*f", decimales, valor);
  for (char* c = destino; *c != '\0'; c++) {
    if (*c == '.') *c = SEPARADOR_DECIMAL;
  }
}

bool armarLinea(char* destino, size_t tamano, const char* codigo, const Medicion& medicion) {
  time_t segundos = (time_t)medicion.marcaTiempo;
  tm fechaHora;
  localtime_r(&segundos, &fechaHora);

  char temperatura[TAMANO_VALOR];
  char humedad[TAMANO_VALOR];
  char velocidad[TAMANO_VALOR];
  char gas[TAMANO_VALOR];
  char presion[TAMANO_VALOR];
  char luminosidad[TAMANO_VALOR];

  formatearDato(temperatura, sizeof(temperatura), medicion.temperatura,
                medicion.estadoTemperatura, DECIMALES_TEMPERATURA);
  formatearDato(humedad, sizeof(humedad), medicion.humedad, medicion.estadoHumedad,
                DECIMALES_HUMEDAD);
  formatearDato(velocidad, sizeof(velocidad), medicion.velocidad, medicion.estadoVelocidad,
                DECIMALES_VELOCIDAD);
  formatearDato(gas, sizeof(gas), medicion.gas, medicion.estadoGas, DECIMALES_GAS);
  formatearDato(presion, sizeof(presion), medicion.presion, medicion.estadoPresion,
                DECIMALES_PRESION);
  formatearDato(luminosidad, sizeof(luminosidad), medicion.luminosidad,
                medicion.estadoLuminosidad, DECIMALES_LUMINOSIDAD);

  // La direccion es texto ("NE"), no numero: sigue las mismas reglas a mano.
  const char* direccion = TEXTO_SIN_SENSOR;
  if (medicion.estadoDireccion == DATO_FALLA) {
    direccion = TEXTO_FALLA;
  } else if (medicion.estadoDireccion == DATO_OK) {
    direccion = (medicion.direccion[0] != '\0') ? medicion.direccion : TEXTO_FALLA;
  }

  int escritos = snprintf(destino, tamano,
                          "[%02d/%02d/%04d;%02d:%02d;%s;%s;%s;%s;%s;%s;%s;%s;EOF]",
                          fechaHora.tm_mday, fechaHora.tm_mon + 1, fechaHora.tm_year + 1900,
                          fechaHora.tm_hour, fechaHora.tm_min,
                          codigo, temperatura, humedad, velocidad, gas, direccion,
                          presion, luminosidad);

  return escritos > 0 && (size_t)escritos < tamano;
}
