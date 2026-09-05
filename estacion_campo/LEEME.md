# Nodo de campo

Firmware del ESP32 que vive a la intemperie: mide, guarda en su memoria y responde por radio.

**Todavía vacío.** El sketch se crea en la **fase 0** del plan (`docs/plan.md`).

## Archivos previstos

| Archivo | Fase | Responsabilidad |
|---|---|---|
| `estacion_campo.ino` | 0 | Arranque y bucle principal |
| `config.h` | 0 | Pines, intervalos y constantes de calibración |
| `sensores.h/.cpp` | 1-5 | Lectura de cada sensor, con bandera de validez |
| `almacenamiento.h/.cpp` | 6 | Log circular en LittleFS, detrás de una interfaz común |
| `energia.h/.cpp` | opcional B | Deep-sleep y medición de batería |

El struct `Medicion` y el protocolo **no van acá**: viven en `libraries/EstacionComun/`,
compartidos con el nodo interfaz.
