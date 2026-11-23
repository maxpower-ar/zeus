#pragma once

// La cola de envio: decide QUE mandar y CUANDO. Lo que pasa por el cable lo hacen
// envio_sheets y envio_itr.
//
// - Solo se intenta con WiFi conectado
// - Primero las pendientes mas viejas, en orden
// - Una medicion se da por enviada solo cuando el servidor confirma
// - Si un envio falla, ese servidor espera INTERVALO_REINTENTO_MS; mientras tanto la
//   estacion sigue midiendo y guardando
// - Una tanda por vuelta del loop, alternando servidores: ninguno traba al otro

#include <Arduino.h>
#include <time.h>
#include "destinos.h"

// Llamar en cada vuelta del loop.
void atenderEnvios();

// Para la pantalla y la tecla e. ultimoEnvioOk = 0 si todavia no hubo ninguno.
time_t ultimoEnvioOk(Destino destino);
bool ultimoEnvioFallo(Destino destino);
uint32_t rechazadasItr();  // mediciones que la API del ITR no acepto (se saltearon)
