# Conexiones: qué cable va a qué pata

Guía para armar la estación (placa **Delta**, ESP32 DevKit V1 de 30 pines) con el firmware de
`estacion_campo/`. Los motivos de cada elección de pin están en
[`docs/hardware/pinout.md`](../docs/hardware/pinout.md); acá solo está **cómo conectar**.

> **Antes de enchufar el USB:** revisar dos veces el 5 V. El único módulo que va a **5 V** es
> el **MQ-7**. Todo lo demás va a **3V3**. Y la salida AO del MQ-7 **nunca** va directo al
> ESP32: pasa por el divisor.

---

## Resumen

| Módulo | Pata del módulo | Pata del ESP32 | Rótulo en la placa |
|---|---|---|---|
| **DHT22** (temp. y humedad) | VCC (+) | 3.3 V | `3V3` |
| | DATA (out) | GPIO4 | `D4` |
| | GND (−) | GND | `GND` |
| **Encoder FC-03** (viento) | VCC | 3.3 V | `3V3` |
| | GND | GND | `GND` |
| | DO | GPIO18 | `D18` |
| | AO | *sin conectar* | — |
| **BMP180** (presión) | VIN (o VCC) | 3.3 V | `3V3` |
| | GND | GND | `GND` |
| | SDA | GPIO21 | `D21` |
| | SCL | GPIO22 | `D22` |
| **Pantalla OLED** SSD1306 | VCC | 3.3 V | `3V3` |
| | GND | GND | `GND` |
| | SDA | GPIO21 | `D21` |
| | SCL | GPIO22 | `D22` |
| **MQ-7** (CO) | VCC | **5 V** | `VIN` |
| | GND | GND | `GND` |
| | AO | GPIO39, **por el divisor** | `VN` |
| | DO | *sin conectar* | — |
| *DS3231 (reloj, cuando se compre)* | VCC / GND / SDA / SCL | 3V3 / GND / D21 / D22 | |

- **Todos los GND juntos.** Sin GND común, ningún sensor anda.
- **SDA y SCL son un bus:** BMP180, pantalla y DS3231 van a los **mismos** dos pines (D21 y
  D22), en paralelo. Conviene una tira de borneras o un punto en la protoboard para cada
  uno.
- El pin `VIN` entrega los 5 V del USB solo cuando la placa está enchufada por USB (o
  alimentada por `VIN`).

---

## Cada módulo en detalle

### DHT22 — temperatura y humedad → `D4`

- **Si es módulo de 3 patas** (placa chica con `+`, `out`, `−`): ya trae la resistencia de
  pull-up. Conectar `+` a `3V3`, `out` a `D4`, `−` a `GND`.
- **Si es el sensor suelto de 4 patas** (mirándolo de frente, rejilla hacia vos, patas
  abajo, de izquierda a derecha):

  | Pata | Va a |
  |---|---|
  | 1 VCC | `3V3` |
  | 2 DATA | `D4`, **y una resistencia de 10 kΩ entre DATA y `3V3`** |
  | 3 | sin conectar |
  | 4 GND | `GND` |

Desconectado, el firmware manda **ERR** en temperatura y humedad.

### Encoder óptico FC-03 — viento → `D18`

| Pata | Va a |
|---|---|
| VCC | `3V3` |
| GND | `GND` |
| DO | `D18` |
| AO | sin conectar |

**Nunca en `VP` (GPIO36) ni `VN` (GPIO39)**: esos pines inventan pulsos y el viento saldría
inflado. El disco tiene 30 ranuras. La horquilla tiene que quedar **tapada del sol**.

Ojo: si el encoder está desconectado el firmware **no puede darse cuenta** (sin pulsos = sin
viento) y manda **0 km/h**, no ERR. Para probarlo: tecla `m` en el monitor serie y girar la
rueda; el renglón "pin del anemómetro" tiene que cambiar entre ALTO y BAJO.

### BMP180 — presión → `D21` / `D22` (I²C, dirección 0x77)

| Pata | Va a |
|---|---|
| VIN (o VCC) | `3V3` |
| GND | `GND` |
| SCL | `D22` |
| SDA | `D21` |
| 3.3 (si el módulo tiene esta pata extra) | sin conectar: es una **salida** del regulador |

Desconectado, el firmware manda **ERR** en presión. Se manda la presión **absoluta** de la
estación en hPa (en Córdoba ronda los 950-970 hPa, no los 1013 del nivel del mar).

### Pantalla OLED SSD1306 128×64 → `D21` / `D22` (I²C, dirección 0x3C)

| Pata | Va a |
|---|---|
| VCC | `3V3` |
| GND | `GND` |
| SCL | `D22` |
| SDA | `D21` |

Ojo con el orden: en muchos módulos es `GND VCC SCL SDA` y en otros `VCC GND SCL SDA`.
**Leer el rótulo del módulo, no la posición.** Sin pantalla, la estación sigue andando igual.

### MQ-7 — monóxido de carbono → `VN` (GPIO39), con divisor

El módulo tiene un calefactor que necesita **5 V** (consume ~150 mA, el USB alcanza). Su
salida **AO** puede llegar a 5 V, y el ESP32 se daña con más de 3.3 V: por eso pasa por un
**divisor resistivo** de dos resistencias.

| Pata del módulo | Va a |
|---|---|
| VCC | `VIN` (5 V) |
| GND | `GND` |
| DO | sin conectar (es un comparador con umbral fijo, no sirve para medir) |
| AO | resistencia de **10 kΩ** → punto medio |

```
 MQ-7 AO ──[ 10 kΩ ]──┬──────────► VN (GPIO39) del ESP32
                      │
                   [ 20 kΩ ]   (dos de 10 kΩ en serie)
                      │
 GND ─────────────────┴──────────► GND del ESP32
```

- 5 V en AO → 3.33 V en el pin. Con aire normal da bastante menos.
- Con el sensor desconectado, la resistencia de 20 kΩ lleva el pin a 0 V y el firmware
  manda **ERR**.
- Si usás otras resistencias, **medirlas con el UT890C** y anotar los valores en
  `MQ7_DIVISOR_ARRIBA_KOHM` y `MQ7_DIVISOR_ABAJO_KOHM` (`estacion_campo/config.h`). La de
  abajo tiene que ser el doble de la de arriba, o más chica (nunca más grande: el pin
  pasaría de 3.3 V).
- **Precalentamiento:** la primera vez necesita 24-48 h encendido para estabilizarse, y cada
  vez que se enciende, unos minutos. Antes de eso marca CO alto.
- **Sin calibrar,** los ppm no son confiables. Sirven para ver si el CO sube o baja.
  Procedimiento: [`calibraciones/mq7.md`](calibraciones/mq7.md).

### DS3231 — reloj (todavía no comprado)

Al bus I²C como los demás: VCC a `3V3`, GND a `GND`, SDA a `D21`, SCL a `D22`. Después,
`RTC_INSTALADO = true` en `config.h`.

---

## Qué pasa con cada sensor sin conectar

En `estacion_campo/config.h` cada sensor tiene una marca de **instalado**. Hoy están todos en
`true` (DHT22, anemómetro, BMP180, MQ-7): mientras alguno no esté conectado, la planilla
muestra **ERR en rojo** en esa columna. Es a propósito: ERR quiere decir "debería estar y no
responde".

| Sensor | Sin conectar manda | En la planilla |
|---|---|---|
| DHT22 | ERR en temperatura y humedad | rojo |
| BMP180 | ERR en presión | rojo |
| MQ-7 | ERR en CO (columna Gas) | rojo |
| Encoder | **0 km/h** (no se puede detectar) | número |
| Dirección y luminosidad | NaN (no hay sensor ni código) | vacío |

A la API del ITR un ERR no se le puede mandar (pide números): va `null`, y si el campo es
obligatorio para ellos, rechazan la medición. Esa medición queda igual en la planilla.

---

## Primera prueba, paso a paso

1. Sin USB, conectar **solo** GND y 3V3 de todos los módulos. Revisar con el multímetro que
   no haya corto entre `3V3` y `GND`.
2. Conectar los cables de datos. Conectar el MQ-7 (VCC a `VIN`) y el divisor.
3. Enchufar el USB, abrir el monitor serie a **115200**.
4. Al arrancar tiene que aparecer:
   - `[pantalla] lista`
   - `[sensores] BMP180: detectado`
   - `[registro] ...` y después la conexión WiFi
5. Tecla **`m`**: lee todos los sensores en el momento. Cada uno conectado tiene que dar
   número; el que dé **ERR** trae al lado la causa probable (qué cable revisar).
6. Tecla **`e`**: estado completo (pendientes por servidor, último envío).
7. Con el tester, medir la tensión en `VN` con el MQ-7 conectado: tiene que estar entre
   0.05 V y 3.1 V. Si da 0, revisar el cable de AO y los 5 V del módulo.
