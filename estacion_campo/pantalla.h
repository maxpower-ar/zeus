#pragma once

// Pantalla OLED SSD1306 de 128x64 (I2C, 0x3C), en el mismo bus que el BMP180.
// Muestra, sin necesidad de PC:
//
//   22/11/2026  10:35:12     fecha y hora
//   WiFi OK  -62 dBm         estado del WiFi
//   Medicion 10:35           hora de la ultima medicion
//   T 22,4 C   H 48 %        ultima medicion: numero, "--" si no hay sensor, "ERR" si fallo
//   Viento 3,2 km/h
//   Presion -- hPa
//   Pendientes: 0            mediciones guardadas que todavia no llegaron al servidor
//   Envio OK 10:35           resultado del ultimo envio a la planilla
//
// Si la pantalla no esta (PANTALLA_INSTALADA = false) o no responde, la estacion sigue
// funcionando igual: la pantalla es solo para mirar.

#include <Medicion.h>

// Llamar despues de iniciar el bus I2C (Wire.begin).
void iniciarPantalla();

// La medicion que se acaba de guardar, para mostrarla.
void pantallaMedicion(const Medicion& medicion);

// Llamar en cada vuelta del loop: redibuja cada INTERVALO_PANTALLA_MS.
void atenderPantalla();
