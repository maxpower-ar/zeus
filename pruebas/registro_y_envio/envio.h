#pragma once

// Envio de mediciones al servidor de Google Sheets (servidor/Codigo.gs).

#include <Medicion.h>

// Manda 'cantidad' mediciones juntas, una linea por renglon. Devuelve true solo si el
// servidor confirmo TODAS (guardadas o ya existentes); recien ahi se pueden dar por
// enviadas. Si devuelve false, quedan pendientes y se reintentan mas tarde.
bool enviarLote(const Medicion* mediciones, int cantidad);
