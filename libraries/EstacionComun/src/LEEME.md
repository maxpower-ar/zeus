# EstacionComun — código compartido

Librería local con todo lo que define **cómo es un dato**. La usan el firmware de la estación
y los sketches de `pruebas/` que necesiten armar mediciones.

## Archivos

| Archivo | Contenido |
|---|---|
| `Medicion.h` | El struct de una medición, con el **estado de cada dato** (ok / sin sensor / falla) y `VERSION_MEDICION`. Fuente única de verdad del formato |
| `LineaServidor.h/.cpp` | Traduce un `Medicion` a la línea `[fecha;hora;codigo;...;EOF]` de `docs/servidor.md`: `NaN` si no hay sensor, `ERR` si falló |
| `JsonItr.h/.cpp` | Traduce un `Medicion` al JSON de la API del ITR (`null` + lista `"fallas"`) |

Si se cambia el struct `Medicion`, **subir `VERSION_MEDICION`** (un `static_assert` del tamaño
lo recuerda al compilar): el registro en flash de la estación descarta lo guardado con otra
versión en vez de leerlo mal.

## Por qué existe esta carpeta

Antes compartía el formato entre dos nodos (se retiró el nodo interfaz). Ahora el motivo es
otro: la línea que se manda al servidor es un **contrato con el equipo de software del ITR**.
Si se arma en un solo lugar, cambiarla es un cambio visible y deliberado, no un detalle
escondido en el medio del firmware.

## Cómo la encuentra el Arduino IDE

El repositorio es el sketchbook: con `Archivo → Preferencias → Ubicación del sketchbook`
apuntando a la raíz del proyecto, el IDE descubre `libraries/EstacionComun` solo.
