# Estación Meteorológica ESP32

Estación meteorológica autónoma con un ESP32 DevKit V1 que manda sus mediciones por WiFi a
un servidor del ITR. Proyecto de escuela secundaria.

## La idea

Un ESP32 se queda **a la intemperie** midiendo el clima. Cada medición se guarda en su propia
memoria y se manda por WiFi a una API del ITR. Si se corta la red, la estación sigue midiendo
y, cuando vuelve la conexión, manda todo lo que quedó pendiente.

```
   ┌─────────────────────┐        WiFi         ┌───────────┐     HTTPS      ┌──────────────┐
   │   ESTACIÓN (Delta)  │ ──────────────────► │  Router   │ ─────────────► │ Servidor ITR │
   │  Sensores           │      2.4 GHz        │           │    internet    │  (API)       │
   │  Log en flash       │                     └───────────┘                └──────────────┘
   │  Hora: NTP + RTC    │
   └─────────────────────┘
```

Cada medición viaja como una línea de texto:

```
[26/10/2026;09:56;EM_ITR_001;26,1;24;12,4;NaN;NaN;1013,2;NaN;EOF]
 fecha      hora  estación   temp hum vel  gas dir presión luz
```

Un dato que falta nunca va como cero: **`NaN`** si el sensor no está instalado, **`ERR`** si
está instalado pero falló (hay que ir a revisarlo).

El servidor lo hace el equipo de software del ITR y **no forma parte de este proyecto**. Lo que
sí es parte del proyecto es entregarle los datos en el formato acordado
([`docs/servidor.md`](docs/servidor.md)).

> Hasta el 2026-10-27 el proyecto tenía un segundo ESP32 ("Beta") que bajaba los datos por
> radio ESP-NOW y los pasaba a una PC por USB. Se retiró al pasar a enviar los datos a un
> servidor.

## Qué mide

| Magnitud | Sensor | Estado |
|---|---|---|
| Temperatura y humedad | DHT22 (GPIO4) | Funcionando en banco |
| Velocidad del viento | Encoder óptico FC-03 (GPIO18), disco de 30 ranuras | Funcionando en banco, falta calibrar |
| Presión atmosférica | BMP180 (I²C 0x77) | **En camino**. Firmware listo, desactivado. Reemplaza al BME280, que no se consiguió |
| Gas / calidad de aire | MQ-135 o ENS160 | **A comprar**. Mientras tanto `NaN` |
| Luminosidad | BH1750 (sugerido) | **A definir**. Mientras tanto `NaN` |
| Hora | NTP por internet, DS3231 de respaldo | DS3231 **a comprar**. Firmware listo, desactivado |
| Pantalla | OLED SSD1306 128×64 (I²C 0x3C) | Disponible, en el firmware |
| Dirección del viento | Sensores Hall A3144 | Postergada, se rediseña |
| Lluvia *(opcional)* | Pluviómetro de balancín con un A3144 | Idea a futuro |

Detalle completo en [`hardware/lista-componentes.md`](hardware/lista-componentes.md).

### Por qué dos sensores de temperatura

El DHT22 y el BMP180 miden temperatura. Comparar las dos lecturas permite detectar cuándo un
sensor se está degradando o quedó mal ventilado. Es una técnica real de estaciones
meteorológicas. El firmware muestra las dos en el monitor serie; al servidor va la del DHT22.
Los dos LM35 que había para esto llegaron fallados y se descartaron.

### Cómo funciona el anemómetro

Una rueda ranurada acoplada al eje de las paletas gira dentro de un fotointerruptor en U: un LED
infrarrojo de un lado, un fototransistor del otro. Cada ranura corta el haz y produce un pulso,
y la frecuencia de pulsos da la velocidad de giro.

Con treinta ranuras por vuelta, el instrumento tiene resolución suficiente incluso con viento
muy suave. Y al ser óptico y sin contacto, no agrega fricción al eje: el único límite es
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

Después de eso, `Archivo → Sketchbook` muestra el firmware y las pruebas, y la librería
compartida se encuentra sola. Placa a seleccionar: **ESP32 Dev Module**. Monitor serie a **115200**.

### Librerías que hay que instalar

No viajan en el repositorio: son código de terceros. Se instalan una vez por máquina desde
**Herramientas → Administrar bibliotecas**:

| Librería | Autor | Para qué |
|---|---|---|
| DHT sensor library | Adafruit | DHT22 (temperatura y humedad) |
| Adafruit Unified Sensor | Adafruit | Dependencia de la anterior; el IDE la ofrece solo |
| Adafruit BMP085 Library | Adafruit | BMP180 (presión). Sirve para los dos chips |
| Adafruit SSD1306 | Adafruit | Pantalla OLED |
| Adafruit GFX Library | Adafruit | Letras y dibujo en la pantalla (el IDE ofrece *Adafruit BusIO* sola) |

El Arduino IDE las descarga dentro de `libraries/` de este repositorio, porque el repositorio
es el sketchbook. Están excluidas en el `.gitignore`.

## Organización

```
estacion_campo/          Firmware de la estación (placa Delta)
estacion_interfaz/       Retirado: nodo puente USB (placa Beta)
libraries/EstacionComun/ Formato del dato y de la línea que se manda al servidor
pruebas/                 Sketches de prueba de concepto, uno por sensor (no son firmware)
docs/                    Plan de trabajo, bitácora y documentación técnica
hardware/                Componentes y calibraciones medidas
servidor/                Nuestro servidor: Google Sheets + Apps Script
tools/                   Utilidades de PC (prueba del formato del servidor con node)
img/                     Fotos de los componentes reales
```

En `pruebas/` va un sketch mínimo por sensor, para verificarlo aislado antes de integrarlo al
firmware. No forman parte del producto: son banco de pruebas. El Arduino IDE los muestra en
`Archivo → Sketchbook → pruebas`.

El código que define **cómo es un dato** y **cómo se le manda al servidor** está en un solo
lugar (`libraries/EstacionComun/`). Esa línea es un contrato con el equipo de software: si se
cambia, se avisa.

## Estado y próximos pasos

El trabajo está dividido en fases, de lo más simple a lo más complejo, en
[`docs/plan.md`](docs/plan.md). Cada fase termina con algo que se puede probar y ver funcionar.

El **firmware final** está en [`estacion_campo/`](estacion_campo/LEEME.md) (escrito y
compilado el 2026-11-22, falta probarlo en la placa). Junta lo probado en `pruebas/`: mide,
guarda en la flash, manda a la planilla y muestra el estado en la pantalla.

## Documentación

- [`docs/plan.md`](docs/plan.md) — plan de fases y estado de avance
- [`docs/bitacora.md`](docs/bitacora.md) — registro de lo que se probó y midió
- [`docs/hardware/pinout.md`](docs/hardware/pinout.md) — asignación de pines y sus motivos
- [`docs/hardware/sensores.md`](docs/hardware/sensores.md) — cómo funciona y se conecta cada sensor
- [`docs/formato-datos.md`](docs/formato-datos.md) — estructura del registro y del archivo de log
- [`docs/servidor.md`](docs/servidor.md) — formato de la línea y contrato con el servidor del ITR

## Herramientas

- Multímetro **UT890C** — es el instrumento de referencia del proyecto: toda calibración se
  contrasta contra él.
