// Configuracion del nodo interfaz.
// Todo numero que dependa del hardware o del armado va aca, nunca dentro de la logica.

#ifndef CONFIG_H
#define CONFIG_H

// LED integrado de la placa DevKit V1 (ver docs/hardware/pinout.md).
const int PIN_LED = 2;

// Parpadeo invertido respecto del nodo de campo: encendido largo, apagado corto.
// Con las dos placas sobre la mesa se distingue cual es cual de un vistazo.
const unsigned long LED_ENCENDIDO_MS = 1300;
const unsigned long LED_APAGADO_MS   = 200;

const unsigned long INTERVALO_MAC_MS = 5000;

const unsigned long VELOCIDAD_SERIE = 115200;

#endif
