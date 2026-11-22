# Nodo de campo

Firmware de la estación (placa **Delta**), a la intemperie: mide, guarda en su memoria y manda
cada medición por WiFi al servidor del ITR.

**Todavía vacío.** El sketch se crea en la **fase 0** del plan (`docs/plan.md`).

## Archivos previstos

| Archivo | Fase | Responsabilidad |
|---|---|---|
| `estacion_campo.ino` | 0 | Arranque y bucle principal |
| `config.h` | 0 | Pines, intervalos y constantes de calibración |
| `sensores.h/.cpp` | 1-5 | Lectura de cada sensor, con bandera de validez |
| `almacenamiento.h/.cpp` | 6 | Log circular en LittleFS, detrás de una interfaz común |
| `red.h/.cpp` | 7 | WiFi, reconexión y hora por NTP |
| `envio.h/.cpp` | 8-9 | Envío al servidor y reenvío de pendientes |
| `secretos.h` | 7 | Contraseña WiFi y clave de la API. **No se sube** (`.gitignore`) |
| `energia.h/.cpp` | opcional B | Deep-sleep y medición de batería |

El struct `Medicion` y el armado de la línea al servidor **no van acá**: viven en
`libraries/EstacionComun/`.
