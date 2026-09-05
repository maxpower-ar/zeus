# Calibraciones

Mediciones reales tomadas con el multímetro **UT890C**. Un archivo por sensor.

**Ninguna constante de calibración del firmware puede salir de otro lado que no sea esta
carpeta.** Si un valor todavía no se midió, en el código va marcado con `TODO: calibrar` y se
dice explícitamente que es provisional.

## Archivos esperados

| Archivo | Fase | Contenido |
|---|---|---|
| `lm35.md` | 1 | Desvío de cada LM35 contra el UT890C, a varias temperaturas |
| `dht22.md` | 1 | Contraste del DHT22 contra el multímetro |
| `anemometro.md` | 3 | Tabla tensión → km/h y velocidad de arranque del motor |
| `veleta.md` | 2 | Qué pin corresponde a qué rumbo, y polaridad del imán |
| `bme280.md` | 4 | Presión leída contra la del servicio meteorológico, y altitud del lugar |
| `gas.md` | 4 | Línea de base en aire limpio, tras el precalentamiento |

## Plantilla

```
# Calibración de <sensor>

Fecha: AAAA-MM-DD
Instrumento de referencia: multímetro UT890C
Condiciones: (temperatura ambiente, si es relevante)

| Referencia (UT890C) | Lectura del sensor | Error |
|---|---|---|
|  |  |  |

**Constante resultante:**
**Rango en el que es válida:**
**Observaciones:**
```
