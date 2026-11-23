#pragma once

// Registro circular de mediciones en la memoria flash (LittleFS).
//
// Cada medicion se guarda ANTES de intentar mandarla. Los contadores dicen donde estamos:
//
//   escritas         : cuantas mediciones se guardaron en total, desde que existe el registro
//   enviadas[destino]: cuantas de esas confirmo cada servidor (planilla e ITR, ver destinos.h)
//
//   pendientes[destino] = escritas - enviadas[destino]
//
// El archivo tiene lugar para CAPACIDAD_REGISTROS mediciones. La numero N va en la casilla
// N % CAPACIDAD: cuando se llena, la nueva pisa a la mas vieja. Si esa todavia no le habia
// llegado a un servidor, se cuenta como perdida para ese servidor. La estacion nunca deja de
// medir por falta de lugar.
//
// Un destino DESACTIVADO en config.h no acumula pendientes: al activarlo, empieza a recibir
// desde la medicion siguiente (no le cae de golpe todo el historial).
//
// Los contadores se guardan en la cabecera del archivo: sobreviven a un corte de luz.

#include <Medicion.h>
#include "destinos.h"

bool iniciarRegistro();

// Guarda una medicion al final del registro.
bool guardarMedicion(const Medicion& medicion);

// Copia en destino hasta 'maximo' pendientes de ese servidor, empezando por la mas vieja.
// Devuelve cuantas. No las marca como enviadas: eso lo hace marcarEnviadas() cuando el
// servidor confirma.
int leerPendientes(Destino servidor, Medicion* destino, int maximo);

// Las 'cantidad' pendientes mas viejas ya llegaron a ese servidor.
bool marcarEnviadas(Destino servidor, int cantidad);

uint32_t cantidadPendientes(Destino servidor);
uint32_t cantidadPerdidas(Destino servidor);
uint32_t cantidadEscritas();

// Borra todo el registro y empieza de cero. Solo para pruebas (tecla B).
bool borrarRegistro();
