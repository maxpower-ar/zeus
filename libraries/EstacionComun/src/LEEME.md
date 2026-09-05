# EstacionComun — código compartido

Librería local usada por **los dos** sketches. Acá vive todo lo que ambos nodos tienen que
entender igual.

**Todavía vacía.** Se escribe en las fases 5, 7 y 8 del plan.

## Archivos previstos

| Archivo | Fase | Contenido |
|---|---|---|
| `Medicion.h` | 5 | El struct del registro. Fuente única de verdad del formato |
| `Protocolo.h/.cpp` | 7 | Tipos de paquete, CRC, serialización |
| `Transporte.h/.cpp` | 7 | Abstracción de ESP-NOW (`enviar` / `recibir`) |

## Por qué existe esta carpeta

Si el formato del dato o el del paquete se copiaran en los dos sketches, tarde o temprano
alguien cambiaría uno y no el otro, y los nodos dejarían de entenderse — con un bug que se
manifestaría como datos corruptos por radio, que es de los más difíciles de diagnosticar.

Estando acá, un cambio obliga a recompilar ambos firmwares con la misma definición.

`Transporte` está separado del resto para que cambiar ESP-NOW por LoRa (fase opcional E) no
obligue a tocar el protocolo ni el almacenamiento.

## Cómo la encuentra el Arduino IDE

El repositorio es el sketchbook: con `Archivo → Preferencias → Ubicación del sketchbook`
apuntando a la raíz del proyecto, el IDE descubre `libraries/EstacionComun` solo.
