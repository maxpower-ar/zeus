#pragma once

// Registro circular de mediciones en la memoria flash (LittleFS).
//
// Cada medicion se guarda ANTES de intentar mandarla. Dos contadores dicen donde estamos:
//
//   escritas: cuantas mediciones se guardaron en total, desde que existe el registro
//   enviadas : cuantas de esas confirmo el servidor
//
//   pendientes = escritas - enviadas
//
// El archivo tiene lugar para CAPACIDAD_REGISTROS mediciones. La numero N va en la casilla
// N % CAPACIDAD: cuando se llena, la nueva pisa a la mas vieja. Si esa todavia no se habia
// enviado, se cuenta como perdida. La estacion nunca deja de medir por falta de lugar.
//
// Los contadores se guardan en la cabecera del archivo: sobreviven a un corte de luz.

#include <Medicion.h>

bool iniciarRegistro();

// Guarda una medicion al final del registro.
bool guardarMedicion(const Medicion& medicion);

// Copia en destino hasta 'maximo' pendientes, empezando por la mas vieja. Devuelve cuantas.
// No las marca como enviadas: eso se hace con marcarEnviadas() cuando el servidor confirma.
int leerPendientes(Medicion* destino, int maximo);

// Las 'cantidad' pendientes mas viejas ya llegaron al servidor.
bool marcarEnviadas(int cantidad);

uint32_t cantidadPendientes();
uint32_t cantidadEscritas();
uint32_t cantidadPerdidas();

// Borra todo el registro y empieza de cero. Solo para pruebas.
bool borrarRegistro();
