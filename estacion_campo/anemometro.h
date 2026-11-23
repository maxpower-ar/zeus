#pragma once

// Anemometro: encoder optico FC-03 (LM393) en GPIO18, disco de 30 ranuras.
// Cada ranura que corta el haz es un pulso; se cuentan con una interrupcion.
//
// Informa la velocidad PROMEDIO desde la medicion anterior: es lo que da una estacion
// meteorologica (el viento medio del periodo), no el valor de un instante.
//
// Limitacion: desde el codigo, "rueda quieta" y "sensor desconectado" se ven iguales (no
// llegan pulsos en los dos casos). Por eso el anemometro nunca sale como ERR: a lo sumo
// marca 0 km/h. Para revisarlo esta la tecla m y el nivel del pin que muestra.
//
// METROS_POR_PULSO esta SIN CALIBRAR (config.h): los km/h son aproximados.

#include <Medicion.h>

void iniciarAnemometro();

// Completa la velocidad de la medicion. Con 'cerrarPeriodo' = true el promedio vuelve a
// empezar (se usa al guardar una medicion); con false solo se mira (tecla m).
void anotarViento(Medicion& medicion, bool cerrarPeriodo);

// Tira lo contado hasta ahora y empieza un periodo nuevo (por ejemplo, al tener hora).
void descartarPeriodoViento();

// Nivel actual del pin del encoder, para distinguir a mano "quieta" de "desconectada".
bool nivelPinEncoder();
