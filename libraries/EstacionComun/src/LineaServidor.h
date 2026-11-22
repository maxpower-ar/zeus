#pragma once

// Arma la linea que la estacion manda al servidor. Formato acordado en docs/servidor.md:
//
//   [DD/MM/AAAA;HH:MM;CODIGO;temperatura;humedad;velocidad;gas;direccion;presion;luminosidad;EOF]
//   [26/10/2026;09:56;EM_ITR_001;26,1;24;12,4;NaN;NaN;1013,2;NaN;EOF]
//
// Este es el UNICO lugar donde se arma la linea. Cambiarla rompe al servidor: cualquier
// cambio se acuerda antes con el equipo de software.

#include <Arduino.h>
#include "Medicion.h"

// PENDIENTE de acordar con el equipo de software del ITR. Nuestro servidor de Google
// Sheets acepta coma o punto.
const char SEPARADOR_DECIMAL = ',';

const int DECIMALES_TEMPERATURA = 1;
const int DECIMALES_HUMEDAD = 0;     // el DHT22 tiene +-2 %: un decimal seria falsa precision
const int DECIMALES_VELOCIDAD = 1;
const int DECIMALES_GAS = 0;
const int DECIMALES_PRESION = 1;
const int DECIMALES_LUMINOSIDAD = 0;

// Alcanza de sobra: una linea completa ronda los 75 caracteres.
const size_t TAMANO_LINEA = 128;

// Escribe la linea en destino, con la fecha y hora LOCAL de la medicion (la zona horaria
// la fija configTzTime). Los datos en NAN salen como "NaN", nunca como cero.
// Devuelve false si no entro en el tamano dado.
bool armarLinea(char* destino, size_t tamano, const char* codigo, const Medicion& medicion);
