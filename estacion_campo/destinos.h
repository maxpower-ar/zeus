#pragma once

// Los servidores a los que la estacion puede mandar sus mediciones. Cada uno lleva su propia
// cuenta de cuales ya recibio (ver registro.h): si la planilla esta al dia pero el ITR no
// responde, las del ITR siguen pendientes sin trabar a la planilla.

#include "config.h"

enum Destino : uint8_t {
  DESTINO_SHEETS = 0,  // nuestro servidor de Google Sheets (servidor/Codigo.gs)
  DESTINO_ITR = 1,     // la API del equipo de software
  CANTIDAD_DESTINOS = 2,
};

inline bool destinoActivo(Destino destino) {
  return destino == DESTINO_SHEETS ? ENVIAR_A_SHEETS : ENVIAR_A_ITR;
}

inline const char* nombreDestino(Destino destino) {
  return destino == DESTINO_SHEETS ? "planilla" : "ITR";
}
