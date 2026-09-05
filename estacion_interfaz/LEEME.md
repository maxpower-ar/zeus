# Nodo interfaz

Firmware del ESP32 conectado por USB a la computadora. Es un **puente**: traduce comandos del
puerto serie a paquetes ESP-NOW hacia el nodo de campo, y las respuestas de vuelta.

No guarda datos ni los interpreta.

**Todavía vacío.** El sketch se crea en la **fase 0** del plan (`docs/plan.md`).

## Archivos previstos

| Archivo | Fase | Responsabilidad |
|---|---|---|
| `estacion_interfaz.ino` | 0 | Arranque y bucle principal |
| `config.h` | 0 | Pines de botones y OLED, MAC del nodo de campo |
| `comandos.h/.cpp` | 7-9 | Comandos de texto del puerto serie |
| `pantalla.h/.cpp` | opcional A | OLED y menú de botones |
