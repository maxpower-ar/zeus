#pragma once

// WiFi y hora.
//
// WiFi: el ESP32 se reconecta solo si se corta. Este modulo solo avisa los cambios por el
// monitor serie y permite cortarlo a mano (tecla w) para probar sin tocar el router.
//
// Hora: viene de internet (NTP), en hora de Argentina (UTC-3). Si esta instalado el DS3231
// (RTC_INSTALADO en config.h), al arrancar se toma la hora de ahi y cada sincronizacion NTP
// lo vuelve a poner en hora (ver rtc.h).

#include <Arduino.h>
#include <time.h>

void iniciarRed();

// Llamar en cada vuelta del loop: avisa cambios del WiFi y de la hora.
void atenderRed();

bool wifiConectado();
void alternarWifi();  // tecla w: cortar / volver a conectar

// "conectado", "sin conexion" o "cortado a mano"
const char* textoEstadoWifi();

// Hora actual. Devuelve false mientras el reloj no este en hora (sin NTP ni DS3231).
bool obtenerHora(time_t& ahora);
