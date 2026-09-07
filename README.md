# Estación Meteorológica ESP32

Estación meteorológica autónoma de dos nodos, construida con dos placas ESP32 DevKit V1.
Proyecto de escuela secundaria.

## La idea

Un ESP32 se queda **en el campo** midiendo el clima y guardando cada medición en su propia
memoria. Otro ESP32 vive **enchufado a una computadora** y sirve de puente: cuando se lo pide,
se conecta por radio con el de campo, le descarga el histórico y lo entrega a la PC por el
puerto serie.

```
   ┌─────────────────────┐                      ┌──────────────────────┐
   │   NODO DE CAMPO     │                      │   NODO INTERFAZ      │
   │                     │                      │                      │
   │  Sensores           │      ESP-NOW         │   OLED + botones     │      USB
   │  Reloj (RTC)        │ ◄──────────────────► │   (puente)           │ ◄────────► PC
   │  Log en flash       │      2.4 GHz         │                      │   serie
   └─────────────────────┘                      └──────────────────────┘
```

La PC corre un software de terceros que **no forma parte de este proyecto**. Lo que sí es
parte del proyecto es entregarle los datos en un formato estable y documentado.

## Qué mide

| Magnitud | Sensor | Estado |
|---|---|---|
| Temperatura y humedad | DHT22 | Disponible |
| Temperatura (redundante ×2) | LM35Z | Disponible |
| Dirección del viento | 8 × Hall A3144 (veleta de 8 rumbos) | Disponible |
| Velocidad del viento | Encoder óptico ranurado + paletas | Disponible |
| Presión atmosférica | BME280 | **A comprar** |
| Gas / calidad de aire | MQ-135 o ENS160 | **A comprar** |
| Hora | DS3231 (RTC) | **A comprar** |

Detalle completo en [`hardware/lista-componentes.md`](hardware/lista-componentes.md).

### Por qué tres sensores de temperatura

No es redundancia por capricho. Comparar tres lecturas de la misma magnitud permite detectar
cuándo un sensor se está degradando o quedó mal ventilado: si dos coinciden y uno se aparta,
el que miente es el tercero. Es una técnica real de estaciones meteorológicas.

### Cómo funciona la veleta

Un imán montado en el eje de la veleta pasa frente a ocho sensores Hall dispuestos en círculo,
uno por cada rumbo (N, NE, E, SE, S, SO, O, NO). El sensor que queda enfrentado al imán se
activa. Sin contactos que se desgasten, sin partes que se ensucien y sin necesidad de
calibración: la dirección es la posición física del sensor.

### Cómo funciona el anemómetro

Una rueda ranurada acoplada al eje de las paletas gira dentro de un fotointerruptor en U: un LED
infrarrojo de un lado, un fototransistor del otro. Cada ranura corta el haz y produce un pulso,
y la frecuencia de pulsos da la velocidad de giro.

Con unas veinte ranuras por vuelta, el instrumento tiene resolución suficiente incluso con
viento muy suave. Y al ser óptico y sin contacto, no agrega fricción al eje: el único límite es
el rodamiento.

Se probó y se descartó un anemómetro basado en un motor de corriente continua usado como dinamo.
La física es correcta —la tensión generada es proporcional a la velocidad de giro— pero las
escobillas del motor imponen tanta fricción que no arranca con brisa suave, y el instrumento
marca cero justo en el rango de viento más frecuente. El motor se conserva para medir esa
diferencia y documentarla.

## Cómo abrir el proyecto

Este repositorio **es el sketchbook de Arduino**. Configurá el IDE una sola vez:

> Arduino IDE → **Archivo → Preferencias → Ubicación del sketchbook** →
> apuntar a la carpeta de este repositorio.

Después de eso, `Archivo → Sketchbook` muestra los dos sketches y la librería compartida se
encuentra sola. Placa a seleccionar: **ESP32 Dev Module**. Monitor serie a **115200**.

### Librerías que hay que instalar

No viajan en el repositorio: son código de terceros. Se instalan una vez por máquina desde
**Herramientas → Administrar bibliotecas**:

| Librería | Autor | Para qué |
|---|---|---|
| DHT sensor library | Adafruit | DHT22 (temperatura y humedad) |
| Adafruit Unified Sensor | Adafruit | Dependencia de la anterior; el IDE la ofrece solo |

El Arduino IDE las descarga dentro de `libraries/` de este repositorio, porque el repositorio
es el sketchbook. Están excluidas en el `.gitignore`.

## Organización

```
estacion_campo/          Firmware del nodo registrador
estacion_interfaz/       Firmware del nodo puente USB
libraries/EstacionComun/ Formato de datos y protocolo, compartidos por ambos nodos
docs/                    Plan de trabajo, bitácora y documentación técnica
hardware/                Componentes y calibraciones medidas
tools/                   Utilidades de PC para probar el enlace serie
img/                     Fotos de los componentes reales
```

El código que define **cómo se ve un dato** y **cómo viaja por el aire** está en un solo lugar
(`libraries/EstacionComun/`) y lo usan los dos firmwares. Duplicarlo garantizaría que algún día
los dos nodos dejen de entenderse.

## Estado y próximos pasos

El trabajo está dividido en fases, de lo más simple a lo más complejo, en
[`docs/plan.md`](docs/plan.md). Cada fase termina con algo que se puede probar y ver funcionar.

Las fases 0 a 3 no requieren comprar ningún componente.

## Documentación

- [`docs/plan.md`](docs/plan.md) — plan de fases y estado de avance
- [`docs/bitacora.md`](docs/bitacora.md) — registro de lo que se probó y midió
- [`docs/hardware/pinout.md`](docs/hardware/pinout.md) — asignación de pines y sus motivos
- [`docs/hardware/sensores.md`](docs/hardware/sensores.md) — cómo funciona y se conecta cada sensor
- [`docs/formato-datos.md`](docs/formato-datos.md) — estructura del registro y del archivo de log
- [`docs/protocolo.md`](docs/protocolo.md) — comandos y paquetes entre los dos nodos

## Herramientas

- Multímetro **UT890C** — es el instrumento de referencia del proyecto: toda calibración se
  contrasta contra él.
