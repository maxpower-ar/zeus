#pragma once

// Arma la linea que la estacion manda al servidor. Formato acordado en docs/servidor.md:
//
//   [DD/MM/AAAA;HH:MM;CODIGO;temperatura;humedad;velocidad;gas;direccion;presion;luminosidad;EOF]
//   [26/10/2026;09:56;EM_ITR_001;26,1;24;12,4;NaN;NaN;1013,2;NaN;EOF]
//   [26/10/2026;10:01;EM_ITR_001;ERR;ERR;12,4;NaN;NaN;NaN;NaN;EOF]   <- DHT22 fallo
//
// Este es el UNICO lugar donde se arma la linea. Cambiarla rompe al servidor: cualquier
// cambio se acuerda antes con el equipo de software.

#include <Arduino.h>
#include "Medicion.h"

// PENDIENTE de acordar con el equipo de software del ITR. Nuestro servidor de Google
// Sheets acepta coma o punto.
const char SEPARADOR_DECIMAL = ',';

// Lo que se escribe en lugar del numero cuando no hay dato (ver EstadoDato en Medicion.h).
const char* const TEXTO_SIN_SENSOR = "NaN";  // el sensor no esta instalado: esperable
const char* const TEXTO_FALLA = "ERR";       // el sensor esta pero fallo: hay que revisarlo

const int DECIMALES_TEMPERATURA = 1;
const int DECIMALES_HUMEDAD = 0;     // el DHT22 tiene +-2 %: un decimal seria falsa precision
const int DECIMALES_VELOCIDAD = 1;
const int DECIMALES_GAS = 0;
const int DECIMALES_PRESION = 1;
const int DECIMALES_LUMINOSIDAD = 0;

// Alcanza de sobra: una linea completa ronda los 75 caracteres.
const size_t TAMANO_LINEA = 128;

// Escribe la linea en destino, con la fecha y hora LOCAL de la medicion (la zona horaria
// la fija configTzTime). Cada dato sale como numero, "NaN" (sin sensor) o "ERR" (falla),
// nunca como cero. Devuelve false si no entro en el tamano dado.
bool armarLinea(char* destino, size_t tamano, const char* codigo, const Medicion& medicion);

// Un dato suelto con las mismas reglas que en la linea ("26,1", "NaN" o "ERR"). Lo usan
// tambien los mensajes del monitor serie, para que se vea lo mismo que viaja al servidor.
void formatearDato(char* destino, size_t tamano, float valor, EstadoDato estado, int decimales);
