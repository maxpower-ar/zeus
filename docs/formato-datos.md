# Formato de datos

Este documento describe **la propuesta** de formato. Se congela en la **fase 5**, cuando se
escriba `libraries/EstacionComun/Medicion.h`. Ese archivo es la fuente de verdad: si alguna vez
discrepa con este documento, manda el código y hay que corregir el documento.

---

## El registro: `Medicion`

Un registro de **tamaño fijo**. Que sea fijo no es un detalle: permite calcular la posición de
cualquier registro en el archivo con una multiplicación, sin recorrerlo, y hace trivial el log
circular de la fase 6.

| Campo | Tipo | Bytes | Unidad | Nota |
|---|---|---|---|---|
| `timestamp` | `uint32_t` | 4 | segundos | Época Unix, del DS3231 |
| `tempDht` | `int16_t` | 2 | 0.1 °C | 235 = 23.5 °C |
| `humDht` | `uint16_t` | 2 | 0.1 % | 604 = 60.4 % |
| `tempLm35A` | `int16_t` | 2 | 0.1 °C | |
| `tempLm35B` | `int16_t` | 2 | 0.1 °C | |
| `tempBme` | `int16_t` | 2 | 0.1 °C | |
| `presion` | `uint32_t` | 4 | Pa | 101325 Pa = 1013.25 hPa |
| `vientoVel` | `uint16_t` | 2 | 0.1 km/h | |
| `vientoDir` | `uint8_t` | 1 | rumbo | 0=N, 1=NE … 7=NO, 255=inválido |
| `gas` | `uint16_t` | 2 | crudo | Lectura del ADC o ppm según sensor |
| `vbat` | `uint16_t` | 2 | mV | Fase opcional B; 0 si no se mide |
| `validez` | `uint8_t` | 1 | banderas | Ver abajo |
| `crc` | `uint8_t` | 1 | — | CRC-8 de los bytes anteriores |
| **Total** | | **27** | | Puede quedar en 28 por alineación |

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
| 1 | LM35 #1 |
| 2 | LM35 #2 |
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

Un CRC-8 sobre los 26 bytes anteriores del registro, calculado al guardar y verificado al leer.
Detecta corrupción en la flash y errores de transmisión por radio. Un registro con CRC
incorrecto se descarta y se reporta; no se entrega como si fuera bueno.

---

## El archivo de log (nodo de campo)

Archivo binario en LittleFS, formado por registros consecutivos de tamaño fijo:

```
┌──────────┬───────────┬───────────┬─────┬───────────┐
│ Cabecera │ Registro0 │ Registro1 │ ... │ RegistroN │
│  16 B    │   27 B    │   27 B    │     │   27 B    │
└──────────┴───────────┴───────────┴─────┴───────────┘
```

La cabecera guarda una **firma** (para reconocer el archivo), la **versión del formato** (para
que un cambio futuro no haga ilegible lo viejo sin avisar), la **cantidad de registros** y el
**índice de escritura** del log circular.

### Log circular

Cuando el archivo llega al máximo, el registro nuevo pisa al **más viejo**. La estación nunca
se queda sin espacio ni deja de medir: simplemente pierde el pasado más lejano.

Es la decisión correcta para una estación que puede pasar semanas sin que la descarguen —
preferimos perder lo de hace tres meses antes que dejar de registrar lo de hoy.

### Capacidad estimada

Con una partición LittleFS de unos 1.5 MB y registros de 27 bytes:

| Intervalo de muestreo | Registros que entran | Autonomía |
|---|---|---|
| 1 minuto | ~55 000 | ~38 días |
| 5 minutos | ~55 000 | ~190 días |
| 15 minutos | ~55 000 | ~570 días |

**Cinco minutos es el intervalo propuesto:** más de seis meses de historia, y resolución de
sobra para el clima, que no cambia significativamente en menos de eso.

Este cálculo debe **confirmarse en la fase 6** con el tamaño real de la partición elegida.

---

## Salida a la PC (fase 9)

Por el puerto serie del nodo interfaz, a **115200 baudios**. Formato **CSV** con encabezado,
una línea por medición:

```
fecha_hora,temp_dht,hum_dht,temp_lm35a,temp_lm35b,temp_bme,presion_hpa,viento_kmh,viento_dir,gas,validez
2026-09-05T14:30:00,23.5,60.4,23.8,23.1,23.6,1013.2,12.4,NE,320,0xFF
2026-09-05T14:35:00,23.7,60.1,,23.3,23.8,1013.1,14.0,NE,318,0xFD
```

Reglas del formato:

- **Fecha y hora en ISO 8601** (`AAAA-MM-DDTHH:MM:SS`): se ordena alfabéticamente igual que
  cronológicamente, y toda planilla de cálculo lo entiende
- **Los valores ya vienen convertidos** a unidades legibles (grados, hPa, km/h): la división
  por 10 se hace acá, no la tiene que hacer quien lea el archivo
- **La dirección del viento va como texto** (`NE`), no como número
- **Un campo inválido queda vacío**, no en cero. En el ejemplo, la segunda línea tiene el
  DHT22 caído: su temperatura y humedad están vacías y el bit 0 de `validez` está en 0
- **La columna `validez` se conserva** en hexadecimal, para poder diagnosticar después qué pasó

Que un campo vacío sea distinto de un cero es la misma decisión del byte `validez`, sostenida
hasta el final de la cadena: una planilla de cálculo dibuja una interrupción en la línea del
gráfico donde el dato falta, y un cero dibujaría una caída de temperatura que nunca ocurrió.
