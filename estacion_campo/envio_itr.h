#pragma once

// Envio a la API del equipo de software del ITR. PREPARADO pero DESACTIVADO
// (ENVIAR_A_ITR = false en config.h): su API todavia no acepta null en co_concentracion ni
// recibe la fecha de la medicion (bitacora 2026-11-22).
//
// POST URL_ITR con JSON (JsonItr.h en EstacionComun) y la clave en el encabezado X-API-Key
// (CLAVE_API_ITR en secretos.h). Una medicion por pedido: no sabemos si aceptan lotes.

#include <Medicion.h>

enum ResultadoItr {
  ITR_ACEPTADA,   // 200 o 201: la guardaron
  ITR_RECHAZADA,  // 400 o 422: no acepta ESA medicion (por ejemplo un null). Reintentarla no
                  // sirve: se saltea para no trabar a las demas. La planilla la tiene igual.
  ITR_FALLO,      // sin conexion, clave incorrecta (401), servidor caido...: reintentar despues
};

ResultadoItr enviarMedicionItr(const Medicion& medicion);
