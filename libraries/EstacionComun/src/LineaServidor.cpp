#include "LineaServidor.h"
#include <stdio.h>
#include <time.h>

const size_t TAMANO_VALOR = 16;

// Escribe un numero con el separador decimal acordado, o "NaN" si el dato es invalido.
static void formatearValor(char* destino, size_t tamano, float valor, int decimales) {
  if (isnan(valor) || isinf(valor)) {
    snprintf(destino, tamano, "NaN");
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

  formatearValor(temperatura, sizeof(temperatura), medicion.temperatura, DECIMALES_TEMPERATURA);
  formatearValor(humedad, sizeof(humedad), medicion.humedad, DECIMALES_HUMEDAD);
  formatearValor(velocidad, sizeof(velocidad), medicion.velocidad, DECIMALES_VELOCIDAD);
  formatearValor(gas, sizeof(gas), medicion.gas, DECIMALES_GAS);
  formatearValor(presion, sizeof(presion), medicion.presion, DECIMALES_PRESION);
  formatearValor(luminosidad, sizeof(luminosidad), medicion.luminosidad, DECIMALES_LUMINOSIDAD);

  const char* direccion = (medicion.direccion[0] != '\0') ? medicion.direccion : "NaN";

  int escritos = snprintf(destino, tamano,
                          "[%02d/%02d/%04d;%02d:%02d;%s;%s;%s;%s;%s;%s;%s;%s;EOF]",
                          fechaHora.tm_mday, fechaHora.tm_mon + 1, fechaHora.tm_year + 1900,
                          fechaHora.tm_hour, fechaHora.tm_min,
                          codigo, temperatura, humedad, velocidad, gas, direccion,
                          presion, luminosidad);

  return escritos > 0 && (size_t)escritos < tamano;
}
