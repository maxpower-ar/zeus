# Formato de datos

Este documento describe **la propuesta** de formato. Se congela en la **fase 5**, cuando se
escriba `libraries/EstacionComun/Medicion.h`. Ese archivo es la fuente de verdad: si alguna vez
discrepa con este documento, manda el código y hay que corregir el documento.

---

## El registro: `Medicion`

## Versión en uso: `Medicion` v2 (2026-11-22)

La fuente de verdad es `libraries/EstacionComun/src/Medicion.h`. Usa `float` en vez de los
enteros de la propuesta original (más abajo), y **cada dato lleva su estado**:

| Estado | Significa | Valor | Viaja como |
|---|---|---|---|
| `DATO_OK` | El sensor respondió | el número | `26,1` |
| `DATO_SIN_SENSOR` | El sensor no está instalado (`config.h`). Esperable | `NAN` | `NaN` |
| `DATO_FALLA` | Está instalado pero falló. **Hay que revisarlo** | `NAN` | `ERR` |

Un `NAN` solo no alcanza para distinguir "no hay sensor" de "el sensor se rompió": por eso
el estado se guarda aparte. Nunca se usa un número (`-1`, `0`) como marca.

| Campo | Tipo | Bytes | Unidad |
|---|---|---|---|
| `marcaTiempo` | `uint32_t` | 4 | segundos desde 1970 (UTC) |
| `temperatura`, `humedad`, `velocidad`, `gas`, `presion`, `luminosidad` | `float` ×6 | 24 | °C, %, km/h, —, hPa, lux |
| `direccion` | `char[4]` | 4 | `"N"` … `"NO"` |
| `estadoTemperatura` … `estadoDireccion` | `uint8_t` ×7 | 7 | `EstadoDato` |
| `reservado` | `uint8_t` | 1 | relleno, siempre 0 |
| **Total** | | **40** | (más 1 byte de CRC al guardarlo) |

**Versión del formato:** `VERSION_MEDICION = 2` (la 1 era sin estados). Si cambia el struct
hay que subirla; un `static_assert` del tamaño frena la compilación si alguien se olvida. El
registro en flash guardado con otra versión se descarta en vez de leerse mal.

**Presión:** absoluta de la estación (lo que mide el BMP180), en hPa. No se corrige a nivel
del mar porque no tenemos la altitud medida.

---

## Propuesta original (no implementada)

Queda como referencia por si algún día el espacio no alcanza.


Un registro de **tamaño fijo**. Que sea fijo no es un detalle: permite calcular la posición de
cualquier registro en el archivo con una multiplicación, sin recorrerlo, y hace trivial el log
circular de la fase 6.

| Campo | Tipo | Bytes | Unidad | Nota |
|---|---|---|---|---|
| `timestamp` | `uint32_t` | 4 | segundos | Época Unix, de NTP (DS3231 de respaldo) |
| `tempDht` | `int16_t` | 2 | 0.1 °C | 235 = 23.5 °C |
| `humDht` | `uint16_t` | 2 | 0.1 % | 604 = 60.4 % |
| `tempBme` | `int16_t` | 2 | 0.1 °C | |
| `presion` | `uint32_t` | 4 | Pa | 101325 Pa = 1013.25 hPa |
| `vientoVel` | `uint16_t` | 2 | 0.1 km/h | |
| `vientoDir` | `uint8_t` | 1 | rumbo | 0=N, 1=NE … 7=NO, 255=inválido. Sensor postergado |
| `gas` | `uint16_t` | 2 | crudo | Lectura del ADC o ppm según sensor |
| `luminosidad` | `uint32_t` | 4 | lux | Sin sensor todavía. 32 bits: al sol pleno supera los 65 535 lux |
| `vbat` | `uint16_t` | 2 | mV | Fase opcional B; 0 si no se mide |
| `validez` | `uint8_t` | 1 | banderas | Ver abajo |
| `crc` | `uint8_t` | 1 | — | CRC-8 de los bytes anteriores |
| **Total** | | **23** | | Puede quedar en 24 por alineación |

> Los dos LM35 se descartaron (bitácora 2026-09-06) y sus campos se quitaron.

### Por qué enteros y no `float`

Guardar décimas en un entero en vez de usar `float` tiene tres ventajas concretas:

1. **Ocupa la mitad** en la flash, que es el recurso escaso del nodo de campo
2. **No hay error de redondeo** al escribir y leer: el valor que se guardó es el que vuelve
3. **Los `float` no son portables byte a byte** entre plataformas; los enteros sí

El precio es recordar dividir por 10 al mostrar. Es un precio barato.

### El campo `validez`

Un bit por sensor. **Si el bit está en 0, el valor de ese campo no significa nada** y no debe
graficarse ni promediarse.

| Bit | Sensor |
|---|---|
| 0 | DHT22 |
| 1 | *(libre, era LM35 #1)* |
| 2 | *(libre, era LM35 #2)* |
| 3 | BME280 |
| 4 | Anemómetro |
| 5 | Veleta |
| 6 | Gas |
| 7 | RTC |

**Por qué existe este campo:** un sensor desconectado no debe producir un cero silencioso. Un
cero es un dato: significa 0 °C, o viento en calma. "No sé" es algo completamente distinto, y
confundirlos arruina cualquier análisis posterior. Este byte es la diferencia entre las dos
cosas.

### El campo `crc`

Un CRC-8 sobre los bytes anteriores del registro, calculado al guardar y verificado al leer.
Detecta corrupción en la flash. Un registro con CRC
incorrecto se descarta y se reporta; no se entrega como si fuera bueno.

---

## El archivo de log

> **Implementado (2026-11-22)** en `estacion_campo/registro.cpp`, con dos diferencias con lo
> de abajo: la cabecera guarda **un índice de envío por servidor** (planilla e ITR), y la
> capacidad es de **20 000 mediciones de 41 bytes** (~820 KB): a una cada 5 minutos, unos
> **69 días** sin red. Firma `EMES`, versión 2.

Archivo binario en LittleFS, formado por registros consecutivos de tamaño fijo:

```
┌──────────┬───────────┬───────────┬─────┬───────────┐
│ Cabecera │ Registro0 │ Registro1 │ ... │ RegistroN │
│  16 B    │   23 B    │   23 B    │     │   23 B    │
└──────────┴───────────┴───────────┴─────┴───────────┘
```

La cabecera guarda una **firma** (para reconocer el archivo), la **versión del formato** (para
que un cambio futuro no haga ilegible lo viejo sin avisar), la **cantidad de registros**, el
**índice de escritura** del log circular y el **índice de envío**: el último registro que el
servidor confirmó haber recibido. Todo lo que está entre el índice de envío y el de escritura
está **pendiente** y se manda cuando vuelve la red.

### Log circular

Cuando el archivo llega al máximo, el registro nuevo pisa al **más viejo**. La estación nunca
se queda sin espacio ni deja de medir: simplemente pierde el pasado más lejano.

Es la decisión correcta para una estación que puede pasar semanas sin red — preferimos perder
lo de hace tres meses antes que dejar de registrar lo de hoy.

### Capacidad estimada

Con una partición LittleFS de unos 1.5 MB y registros de 23 bytes:

| Intervalo de muestreo | Registros que entran | Autonomía sin red |
|---|---|---|
| 1 minuto | ~65 000 | ~45 días |
| 5 minutos | ~65 000 | ~225 días |
| 15 minutos | ~65 000 | ~680 días |

**Cinco minutos es el intervalo propuesto:** más de seis meses de historia, y resolución de
sobra para el clima, que no cambia significativamente en menos de eso.

Este cálculo debe **confirmarse en la fase 6** con el tamaño real de la partición elegida.

---

## Envío al servidor (fases 8 a 10)

Cada registro se traduce a una línea de texto y se manda por WiFi a la API del ITR. El formato
de esa línea, lo acordado y lo pendiente están en [`servidor.md`](servidor.md).

La traducción respeta el estado de cada campo: **sin sensor se manda como `NaN` y falla como
`ERR`**, nunca como cero. Un cero dibujaría en un gráfico una caída de temperatura que nunca
ocurrió.
