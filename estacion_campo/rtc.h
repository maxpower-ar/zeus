#pragma once

// Reloj DS3231 (I2C, 0x68), respaldo de la hora. TODAVIA NO COMPRADO: se usa solo con
// RTC_INSTALADO = true en config.h. Escrito pero NO PROBADO en hardware.
//
// Para que sirve: la hora sale de internet (NTP). Si la placa se reinicia durante un corte
// de red, no tiene de donde sacarla y no puede registrar nada. El DS3231 tiene pila y sigue
// en hora con la placa apagada.
//
//   - al arrancar: si el DS3231 tiene una hora valida, el ESP32 la toma (red.cpp)
//   - cada vez que llega la hora por NTP: se la copia al DS3231 para mantenerlo en hora
//
// El DS3231 guarda la hora en UTC (no en hora de Argentina): asi no hay que pensar en
// zonas horarias al leerlo; la conversion a hora local la hace el ESP32 como con NTP.
//
// Se habla con el chip directamente por I2C (son 7 registros), sin libreria extra.

#include <Arduino.h>
#include <time.h>

// true si el DS3231 responde en el bus.
bool rtcResponde();

// Lee la hora (UTC, segundos desde 1970). Devuelve false si no responde o si el chip avisa
// que perdio la hora (se quedo sin pila: bandera OSF).
bool leerRtc(time_t& utc);

// Pone el DS3231 en hora (UTC) y borra la bandera OSF.
bool escribirRtc(time_t utc);
