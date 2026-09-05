# Asignación de pines

**Antes de asignar cualquier pin nuevo, leer este archivo.** Las asignaciones no son
arbitrarias: cada una respeta restricciones físicas del ESP32 que se explican al final.

Placa: **ESP32 DevKit V1, 30 pines, ESP32-WROOM-32**.

---

## Nodo de campo

| Función | Pin | Tipo | Nota |
|---|---|---|---|
| DHT22 (dato) | GPIO4 | Digital E/S | Resistencia de pull-up de 10 kΩ a 3.3 V |
| LM35 #1 | GPIO34 (D34) | **ADC1**, solo entrada | Salida directa, no supera 3.3 V |
| LM35 #2 | GPIO35 (D35) | **ADC1**, solo entrada | |
| Anemómetro (motor) | GPIO36 (VP) | **ADC1**, solo entrada | **Con divisor resistivo** |
| Sensor de gas | GPIO39 (VN) | **ADC1**, solo entrada | Con divisor si la salida es de 5 V |
| Tensión de batería | GPIO33 | **ADC1** | Con divisor. Fase opcional B |
| I²C SDA | GPIO21 | Bus compartido | BME280 + DS3231 |
| I²C SCL | GPIO22 | Bus compartido | |
| Hall N | GPIO13 | Digital entrada | Vía conversor de nivel |
| Hall NE | GPIO14 | Digital entrada | |
| Hall E | GPIO16 | Digital entrada | UART2 RX, libre (no se usa UART2) |
| Hall SE | GPIO17 | Digital entrada | UART2 TX, libre |
| Hall S | GPIO25 | Digital entrada | |
| Hall SO | GPIO26 | Digital entrada | |
| Hall O | GPIO27 | Digital entrada | |
| Hall NO | GPIO32 | Digital entrada | Es ADC1 pero se usa como digital |
| *(reservado)* | GPIO5, 18, 19, 23 | SPI | Para microSD, fase opcional C |

Pines deliberadamente **libres**: GPIO 0, 2, 12, 15 (strapping) y GPIO 1, 3 (puerto serie USB).

## Nodo interfaz

Tiene muy poco hardware: es un puente.

| Función | Pin | Tipo | Nota |
|---|---|---|---|
| OLED SDA | GPIO21 | I²C | Dirección 0x3C |
| OLED SCL | GPIO22 | I²C | |
| Botón ARRIBA | GPIO13 | Digital entrada | `INPUT_PULLUP`, a GND |
| Botón ABAJO | GPIO14 | Digital entrada | `INPUT_PULLUP`, a GND |
| Botón OK | GPIO27 | Digital entrada | `INPUT_PULLUP`, a GND |
| LED de estado | GPIO2 | Digital salida | LED integrado de la placa |
| Serie a la PC | GPIO1/3 | UART0 | Vía el USB de la placa |

Los botones se conectan **entre el pin y GND**, sin resistencias externas: se usa el pull-up
interno del ESP32 y la lógica queda invertida (presionado = LOW).

---

## Bus I²C — direcciones

Todos estos dispositivos comparten los mismos dos cables (SDA y SCL). No hay conflicto porque
cada uno tiene una dirección distinta:

| Dispositivo | Dirección | Nodo |
|---|---|---|
| DS3231 (reloj) | 0x68 | Campo |
| BME280 (presión) | 0x76 (o 0x77) | Campo |
| OLED SSD1306 | 0x3C (o 0x3D) | Interfaz |

Ante cualquier duda de conexionado, correr un **escáner I²C**: si el dispositivo no aparece en
la lista, el problema es de cableado o alimentación, no de código.

---

## Por qué estas asignaciones y no otras

### ADC2 es inutilizable con la radio encendida

El ESP32 tiene dos conversores analógico-digitales. El **ADC2 comparte hardware con el módulo
de radio**: mientras WiFi o ESP-NOW están activos, sus lecturas fallan o devuelven basura.
Como este proyecto usa ESP-NOW de forma permanente, **toda entrada analógica va a ADC1**:

> GPIO **32, 33, 34, 35, 36 (VP), 39 (VN)** — no hay otros.

Son seis pines y el proyecto necesita cuatro analógicas (2 × LM35, anemómetro, gas) más la
batería en el futuro. Entra, pero sin margen: por eso ninguno se desperdicia.

### Pines de strapping

GPIO **0, 2, 12 y 15** son leídos por el ESP32 durante el arranque para decidir cómo iniciar.
Si tienen una señal conectada que los fuerza a un nivel equivocado, **la placa no arranca**.

Esto es especialmente peligroso con los módulos Hall: su salida es de colector abierto con una
resistencia de pull-up interna, así que en reposo están en nivel alto. Un módulo Hall en el
GPIO12 puede impedir el arranque. Por eso ninguno de los ocho está en un pin de strapping.

### Pines de solo entrada

GPIO **34, 35, 36 y 39** no pueden ser salida y **no tienen pull-up interno**. Son perfectos
para sensores analógicos (que es exactamente para lo que los usamos) e inservibles para
botones sin resistencia externa.

### El conversor de nivel y los sensores Hall

Los módulos Hall A3144 necesitan **4.5 V como mínimo**: no funcionan a 3.3 V. Van alimentados a
5 V, y en consecuencia su salida digital también es de 5 V — que dañaría las entradas del
ESP32, que son de 3.3 V.

Para eso están los dos conversores de nivel de 8 canales: los ocho Hall pasan por uno de ellos
y llegan al ESP32 convertidos a 3.3 V.

**El conversor solo sirve para señales digitales.** Para el anemómetro y el sensor de gas, que
son analógicos, hay que usar un **divisor resistivo**: un conversor de nivel deformaría el
valor de la tensión, que es justamente el dato que queremos medir.

### Zona útil del ADC

El ADC del ESP32 no es lineal cerca de 0 V ni cerca de 3.3 V. Los divisores resistivos se
calculan para que la señal caiga en la zona central del rango, no para aprovechar hasta el
último milivoltio.
