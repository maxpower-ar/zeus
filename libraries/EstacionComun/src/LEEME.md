# EstacionComun — código compartido

Librería local con todo lo que define **cómo es un dato**. La usan el firmware de la estación
y los sketches de `pruebas/` que necesiten armar mediciones.

**Todavía vacía.** Se escribe en las fases 5 y 8 del plan.

## Archivos previstos

| Archivo | Fase | Contenido |
|---|---|---|
| `Medicion.h` | 5 | El struct del registro. Fuente única de verdad del formato |
| `LineaServidor.h/.cpp` | 8 | Traduce un `Medicion` a la línea `[fecha;hora;codigo;...;EOF]` de `docs/servidor.md` |

## Por qué existe esta carpeta

Antes compartía el formato entre dos nodos (se retiró el nodo interfaz). Ahora el motivo es
otro: la línea que se manda al servidor es un **contrato con el equipo de software del ITR**.
Si se arma en un solo lugar, cambiarla es un cambio visible y deliberado, no un detalle
escondido en el medio del firmware.

## Cómo la encuentra el Arduino IDE

El repositorio es el sketchbook: con `Archivo → Preferencias → Ubicación del sketchbook`
apuntando a la raíz del proyecto, el IDE descubre `libraries/EstacionComun` solo.
