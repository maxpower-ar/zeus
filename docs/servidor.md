# Envío de datos al servidor

**Propuesta en negociación.** El contrato con el servidor lo define el equipo de software del
ITR. Este documento junta lo que ya está acordado y lo que falta acordar. Se congela en la
**fase 10**, cuando el envío funcione contra el servidor real.

---

## Arquitectura

Hay un solo ESP32 (**Delta**), a la intemperie. Mide, guarda cada medición en su memoria y la
manda por WiFi a una API del ITR.

```
   ┌─────────────────────┐        WiFi         ┌───────────┐     HTTPS      ┌──────────────┐
   │   ESP32 "Delta"     │ ──────────────────► │  Router   │ ─────────────► │ Servidor ITR │
   │  Sensores + log     │      2.4 GHz        │           │    internet    │  (API)       │
   └─────────────────────┘                     └───────────┘                └──────────────┘
```

El ESP32 "Beta" (nodo puente USB con ESP-NOW) **se retiró** del proyecto el 2026-10-27. Ver la
bitácora.

### Dos servidores, la misma línea

| Servidor | Quién lo hace | Para qué |
|---|---|---|
| **Google Sheets + Apps Script** ([`servidor/`](../servidor/LEEME.md)) | Nosotros | Desarrollo y pruebas, y base de la página web. Gratuito |
| **API del ITR** | Equipo de software | Destino final acordado con la escuela |

La estación manda **exactamente la misma línea** a los dos. Cambiar de servidor es cambiar la
URL en `secretos.h`, no el firmware. Al final se comparan los dos.

---

## Formato de la línea (acordado)

```
[DD/MM/AAAA;HH:MM;CODIGO;temperatura;humedad;velocidad;gas;direccion;presion;luminosidad;EOF]
```

Ejemplos:

```
[26/10/2026;09:56;EM_ITR_001;26,1;24;12,4;NaN;NaN;1013,2;NaN;EOF]
[22/11/2026;10:05;EM_ITR_001;ERR;ERR;3,2;NaN;NaN;NaN;NaN;EOF]     <- el DHT22 falló
```

| # | Campo | Ejemplo | Estado |
|---|---|---|---|
| 1 | Fecha `DD/MM/AAAA` | `26/10/2026` | Acordado |
| 2 | Hora `HH:MM` | `09:56` | Acordado |
| 3 | Código de la estación (EM = Estación Meteorológica) | `EM_ITR_001` | Acordado. Va en `config.h` |
| 4 | Temperatura | `26,1` | Acordado. Unidad: °C |
| 5 | Humedad relativa | `24` | Acordado. Unidad: % |
| 6 | Velocidad del viento | `12,4` | Acordado. **Unidad a confirmar** (km/h o m/s) |
| 7 | Gas | `NaN` | **ppm de CO** (MQ-7, desde 2026-11-24), sin decimales. Sin calibrar todavía |
| 8 | Dirección del viento | `NaN` | Acordado el lugar. Sensor postergado: va `NaN` hasta que exista |
| 9 | Presión atmosférica | `1013,2` | Agregado 2026-11-17. Unidad: hPa. **Presión absoluta de la estación** (BMP180), sin corregir a nivel del mar |
| 10 | Luminosidad | `NaN` | Agregado 2026-11-17 porque la API del ITR lo pide. Unidad: lux. **Sin sensor todavía** |
| — | Fin de línea | `EOF` | Acordado |

Reglas:

- **Separador de campos: `;`**
- **Sensor no instalado: `NaN`.** Es esperable (por ejemplo, el gas mientras no haya sensor)
- **Sensor instalado que falló: `ERR`** (desde el 2026-11-22). No respondió, llegó con el CRC
  mal o dio un valor imposible. No es esperable: hay que ir a revisar el sensor
- **Nunca `0` ni `-1` ni ningún número como marca.** Un cero es un dato (0 °C, viento en
  calma) y −1 °C también existe. Cualquier número puede ser real; `NaN` y `ERR` no
- **Separador decimal: a definir** (`26,1` o `26.1`). El separador `;` permite usar coma sin
  romper los campos, pero el servidor tiene que saber cuál esperar. El firmware usa coma
  (`SEPARADOR_DECIMAL` en `LineaServidor.h`); nuestro servidor acepta las dos
- **Se pueden mandar varias líneas en un envío**, una por renglón (en nuestro servidor; con el
  del ITR está por acordar)

La línea se arma en un solo lugar: `libraries/EstacionComun/src/LineaServidor.cpp`.

### Cómo se ven `NaN` y `ERR` en nuestro servidor (Google Sheets)

| En la línea | En la planilla | En el JSON de `doGet` (para la página) |
|---|---|---|
| `26,1` | `26.1` | `26.1` |
| `NaN` | celda vacía | `null` |
| `ERR` | texto `ERR`, **en rojo** | `"ERR"` (texto) |

La página tiene que preguntar si el valor es un número (`typeof v === 'number'`) antes de
graficarlo; un `"ERR"` se puede mostrar como alerta ("revisar sensor"), un `null` como "sin
sensor".

### JSON para la API del ITR

Su API pide JSON, no la línea. El firmware lo arma en
`libraries/EstacionComun/src/JsonItr.cpp` (envío **activado** el 2026-11-24; ver
limitaciones abajo):

```json
{"codigo":"EM_ITR_001","fecha":"2026-11-22T13:35:00Z","temperatura":22.4,"humedad":48,
 "velocidad_viento":3.2,"co_concentracion":null,"presion":null,"luminosidad":null,
 "fallas":["presion"]}
```

- En JSON un número no puede valer `ERR`: **sin sensor y falla van como `null`**, y los campos
  con falla se listan en `"fallas"`
- `fecha`: hora de la **medición** en UTC (la `Z`). `codigo`, `fecha` y `fallas` son
  **propuestas**: su API todavía no los guarda
- Una medición por pedido. Si responden 400 o 422 (no aceptan esa medición), el firmware la
  saltea para no trabar a las demás; queda igual en la planilla

**Formato que pasó el equipo de software** (2026-11-24, para `/api/lecturas`):

```
{ temperatura: 24, humedad: 50, co_concentracion: 4, velocidad_viento: 15, presion: 1013 }
```

El JSON de la estación ya trae esos cinco campos con esos nombres; los demás (`codigo`,
`fecha`, `luminosidad`, `fallas`) no molestan. `co_concentracion` va en **ppm** (MQ-7).

**Limitaciones con el envío activado (2026-11-24):**

- Mientras un sensor obligatorio para ellos (temperatura, humedad, viento, CO) dé ERR, su API
  rechaza la medición: se saltea y queda solo en la planilla. En el monitor serie aparece
  `el ITR RECHAZO esta medicion`
- Su API no guarda la hora de la medición: lo atrasado aparece con la hora de llegada
- Sin la clave real en `CLAVE_API_ITR` el firmware no intenta y acumula pendientes

---

## Lo que implementó el ITR (actualizado el 2026-11-22)

Desde el 22/11 los obligatorios son `temperatura`, `humedad`, `velocidad_viento` y
`co_concentracion`; `presion` y `luminosidad` aceptan `null`. Un envío de prueba fue aceptado
(`201 CREATED`) y apareció en su página. Detalle en la bitácora del 2026-11-22.

### Versión del 2026-11-17

`POST https://estacionmeteorologica.pythonanywhere.com/api/lecturas`, clave en el encabezado
`X-API-Key`, cuerpo **JSON** con `temperatura`, `humedad`, `presion` y `luminosidad`, todos
numéricos y obligatorios. **No coincide con la línea acordada.** Detalle y captura en la
bitácora del 2026-11-17.

## Pendiente de acordar con el equipo de software

Ver la lista completa de preguntas en la entrada de bitácora del 2026-10-27. Resumen:

1. **URL exacta.** El puerto va pegado al dominio: `https://itr.edu.ar:9090/estacion001`, no
   `itr.edu.ar/estacion001:9090`. Confirmar dominio, puerto y ruta
2. **Método HTTP y cuerpo:** POST o GET; si el cuerpo es la línea sola o lleva el prefijo
   `DATOS:`; tipo de contenido (`text/plain`, JSON, formulario)
3. **Respuestas:** qué código HTTP significa "recibido bien" y cuál "formato inválido"
4. **Duplicados:** si la estación reenvía una línea que ya había llegado (pasa después de un
   corte), ¿el servidor la descarta o la guarda dos veces?
5. **Lotes:** después de un corte se acumulan mediciones. ¿Se pueden mandar varias líneas en
   un solo pedido?
6. **Autenticación:** clave o token, o abierto
7. **Zona horaria** de la fecha y hora (hora de Argentina, UTC−3, o UTC)
8. **Unidades** de velocidad y gas, y separador decimal
10. **Frecuencia** de envío esperada
11. Si hay un **entorno de prueba** o si se prueba directo contra producción

---

## Lo que el firmware tiene que garantizar

- **La medición no depende de la red.** Sin WiFi la estación sigue midiendo y guardando en
  LittleFS
- **Nada se pierde por un corte.** Cada medición queda marcada como pendiente hasta que el
  servidor confirma que la recibió; al volver la red se mandan las pendientes en orden
- **La hora viene de NTP**, con el DS3231 como respaldo para cuando la placa arranca sin red
- **HTTPS se valida contra el certificado raíz** del servidor. Desactivar la validación
  (`setInsecure()`) solo se acepta en pruebas de banco, nunca en la versión final
