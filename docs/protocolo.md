# Protocolo entre los dos nodos

**Propuesta.** Se congela en las **fases 7 y 8**, al escribir
`libraries/EstacionComun/Protocolo.h`. Ese archivo es la fuente de verdad.

---

## Capa de transporte: ESP-NOW

Los dos ESP32 se hablan directamente por ESP-NOW: sin router, sin red WiFi, sin contraseñas.
Cada nodo conoce la **dirección MAC** del otro (anotadas en `docs/bitacora.md` durante la
fase 0).

- Alcance esperado: 50 a 150 m con línea de vista
- Tamaño máximo de paquete: **250 bytes** — es el límite que define el diseño de la descarga
- Sin garantía de entrega: ESP-NOW confirma que el paquete salió, no que se entendió. **Los
  reintentos y las confirmaciones son responsabilidad de este protocolo.**

El transporte vive detrás de una interfaz mínima (`enviar` / `recibir`) en
`libraries/EstacionComun/Transporte.h`. Si el día de mañana hay que cambiar ESP-NOW por LoRa
(fase opcional E), se reemplaza esa capa y el resto del protocolo no se entera.

---

## Estructura del paquete

```
┌──────┬──────┬──────────┬──────────┬─────────────┬─────┐
│ 0xEM │ tipo │ secuencia│ longitud │   carga     │ CRC │
│ 2 B  │ 1 B  │   2 B    │   1 B    │  0 a 240 B  │ 2 B │
└──────┴──────┴──────────┴──────────┴─────────────┴─────┘
```

- **Firma** — dos bytes fijos. Descarta de entrada cualquier paquete que no sea nuestro
- **Tipo** — qué mensaje es (tabla siguiente)
- **Secuencia** — número que vincula una respuesta con su pedido, y ordena los bloques de la
  descarga
- **Longitud** — bytes útiles de la carga
- **CRC** — CRC-16 de todo lo anterior. Un paquete con CRC malo **se descarta en silencio**,
  como si nunca hubiera llegado

---

## Tipos de mensaje

### Fase 7 — enlace básico

| Tipo | Nombre | Dirección | Carga |
|---|---|---|---|
| 0x01 | `PING` | interfaz → campo | vacía |
| 0x02 | `PONG` | campo → interfaz | versión de firmware, tiempo encendido |
| 0x03 | `PEDIR_ACTUAL` | interfaz → campo | vacía |
| 0x04 | `MEDICION_ACTUAL` | campo → interfaz | un registro `Medicion` |
| 0x05 | `PEDIR_ESTADO` | interfaz → campo | vacía |
| 0x06 | `ESTADO` | campo → interfaz | registros guardados, capacidad, hora, sensores fallados |

### Fase 8 — descarga del histórico

| Tipo | Nombre | Dirección | Carga |
|---|---|---|---|
| 0x10 | `PEDIR_BLOQUE` | interfaz → campo | índice del primer registro, cantidad |
| 0x11 | `BLOQUE` | campo → interfaz | N registros consecutivos |
| 0x12 | `ACK_BLOQUE` | interfaz → campo | número de secuencia recibido correctamente |
| 0x13 | `FIN_DESCARGA` | campo → interfaz | total enviado |
| 0x1F | `ERROR` | cualquiera | código de error |

### Utilidad

| Tipo | Nombre | Dirección | Carga |
|---|---|---|---|
| 0x20 | `PONER_HORA` | interfaz → campo | época Unix |
| 0x21 | `BORRAR_LOG` | interfaz → campo | palabra de confirmación |

`BORRAR_LOG` **exige una palabra de confirmación en la carga**. Es la única operación
destructiva del sistema y no debe poder dispararse por un paquete corrupto o un botón apretado
sin querer.

---

## La descarga del histórico (fase 8)

Es la parte difícil, y conviene entender por qué antes de escribirla.

Con paquetes de 240 bytes útiles y registros de 27 bytes, en cada `BLOQUE` entran **8
registros**. Descargar 50 000 registros son más de 6 000 paquetes: con esa cantidad, que
algunos se pierdan no es una posibilidad remota, es una certeza.

El esquema es **pedido y confirmación, de a un bloque por vez**:

```
interfaz                              campo
   │                                    │
   ├── PEDIR_BLOQUE(desde=0, n=8) ────► │
   │ ◄──────────── BLOQUE(seq=0) ───────┤
   ├── ACK_BLOQUE(seq=0) ─────────────► │
   │ ◄──────────── BLOQUE(seq=1) ───────┤
   ├── ACK_BLOQUE(seq=1) ─────────────► │
   │              ...                   │
   │ ◄──────────── FIN_DESCARGA ────────┤
```

Si un ACK no llega en el tiempo previsto, el nodo de campo **reenvía el mismo bloque**. Tras
varios intentos fallidos, la descarga se aborta con `ERROR`.

**Regla no negociable: la descarga falla ruidosamente antes que entregar datos incompletos.**
Un archivo al que le faltan registros en el medio, sin que nadie lo avise, es peor que ninguna
descarga: parece bueno y no lo es.

### Descarga incremental

El nodo interfaz recuerda la marca de tiempo del último registro que bajó, y pide solo lo
posterior. La primera descarga es larga; las siguientes, cortas. Sin esto, cada visita a la
estación significaría volver a bajar meses de datos.

---

## Comandos desde la PC

El nodo interfaz recibe por el puerto serie líneas de texto simples, terminadas en salto de
línea, y las traduce a paquetes. Texto plano para poder probar todo el sistema **a mano desde
el monitor serie del Arduino IDE**, sin escribir ningún software de PC.

| Comando | Efecto |
|---|---|
| `PING` | Verifica que el nodo de campo responda |
| `ESTADO` | Muestra registros almacenados, hora y sensores fallados |
| `ACTUAL` | Pide y muestra la medición actual |
| `DESCARGAR` | Descarga todo el histórico como CSV |
| `DESCARGAR <n>` | Descarga los últimos n registros |
| `HORA` | Sincroniza el reloj del nodo de campo |
| `AYUDA` | Lista los comandos |

Cada respuesta empieza con un prefijo que la clasifica (`OK`, `ERROR`, `DATO`), para que el
software de la PC pueda distinguir un dato de un mensaje sin tener que interpretar el texto.
