#pragma once

// Arma el cuerpo JSON que pide la API del ITR (POST /api/lecturas, ver docs/servidor.md).
// Es el equivalente de LineaServidor.h para su servidor: el UNICO lugar donde se arma.
//
//   {"codigo":"EM_ITR_001","fecha":"2026-11-22T13:35:00Z",
//    "temperatura":22.4,"humedad":48,"velocidad_viento":3.2,"co_concentracion":null,
//    "presion":null,"luminosidad":null,"fallas":["presion"]}
//
// - Un dato sin sensor o con falla va como null: en JSON un numero no puede ser "ERR".
//   Para no perder la diferencia, los campos con FALLA se listan aparte en "fallas".
// - "fecha" es la hora de la MEDICION en UTC (la Z del final), no la de llegada.
// - "codigo", "fecha" y "fallas" son PROPUESTAS: su API todavia no los recibe (bitacora
//   2026-11-22). Los campos de mas no molestan: un envio de prueba con "codigo" fue aceptado.
// - Decimales con punto: JSON no admite coma.

#include <Arduino.h>
#include "Medicion.h"

const size_t TAMANO_JSON_ITR = 320;

// Devuelve false si no entro en el tamano dado.
bool armarJsonItr(char* destino, size_t tamano, const char* codigo, const Medicion& medicion);
