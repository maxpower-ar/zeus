# Lista de componentes

**Consultar este archivo antes de proponer cualquier solución que requiera hardware.** Si un
componente no está acá, no existe en el proyecto.

---

## Disponible

| Componente | Cant. | Uso | Alimentación | Nota |
|---|---|---|---|---|
| ESP32 DevKit V1 (30 pines, WROOM-32) | 2 | Nodo de campo y nodo interfaz | 5 V por USB | Ver `img/esp32.jpeg` |
| DHT22 | 1 | Temperatura y humedad | 3.3 V | Máximo una lectura cada 2 s |
| LM35Z | 2 | Temperatura (redundante) | 5 V | 10 mV/°C, salida analógica |
| Módulo Hall A3144 | 8 | Veleta de 8 rumbos | **5 V mínimo** | Salida digital. Ver `img/hall.jpeg` |
| Conversor de nivel 8 canales | 2 | Hall 5 V → ESP32 3.3 V | 5 V y 3.3 V | **Solo señales digitales** |
| Display OLED I²C | 1 | Interfaz local (fase opcional A) | 3.3 V | SSD1306, dirección 0x3C |
| Botones | varios | Navegación del menú | — | Con `INPUT_PULLUP`, a GND |
| Motor DC + paletas | 1 | Anemómetro generador | — | Requiere divisor resistivo |
| Multímetro **UT890C** | 1 | Instrumento de referencia | — | Toda calibración se contrasta con él |

---

## A comprar

Ordenados por prioridad.

### Imprescindibles

| Componente | Para qué | Aprox. | Por qué es imprescindible |
|---|---|---|---|
| **DS3231** (módulo RTC I²C con pila) | Hora | bajo | El ESP32 pierde la hora en cada reinicio. Sin esto las mediciones no tienen marca temporal confiable y el proyecto pierde sentido |
| **BME280** (módulo I²C) | Presión | medio | Es la magnitud que falta. De paso da una tercera temperatura para contrastar |

**Al comprar el BME280:** existe el **BMP280**, casi idéntico y más barato pero **sin humedad**.
Para este proyecto sirve igual (la humedad la da el DHT22), pero conviene saber cuál se compra.

**Al comprar el DS3231:** evitar el DS1307, que es más barato pero se atrasa mucho. El DS3231
compensa la temperatura y se desvía ~2 minutos por año.

### Sensor de gas — elegir uno

| Opción | Precio | Alimentación | Ventaja | Desventaja |
|---|---|---|---|---|
| **MQ-135** | muy bajo | 5 V | Barato; su principio de funcionamiento es fácil de explicar | Analógico (necesita divisor), consume ~150 mA continuos, 24-48 h de precalentamiento inicial, calibración difícil |
| **ENS160** | alto | 3.3 V | I²C, bajo consumo, entrega TVOC y eCO₂ ya procesados | Bastante más caro; el procesamiento está oculto adentro del chip |

**Recomendación: MQ-135.** Para un proyecto escolar alcanza, y poder explicar cómo funciona
(un calefactor y una resistencia que cambia de valor según el gas presente) vale más en la
defensa del proyecto que un número que sale hecho de un integrado. El consumo de 150 mA solo
sería un problema si se hace la fase opcional de batería.

### Consumibles del armado

| Componente | Para qué |
|---|---|
| Resistencias variadas (1 kΩ, 2.2 kΩ, 10 kΩ) | Divisores resistivos y pull-up del DHT22 |
| Imán de neodimio pequeño | Eje de la veleta |
| Cables Dupont y protoboard | Armado de banco |
| Fuente de 5 V / 2 A o powerbank | Alimentación |

---

## Solo si se hacen las fases opcionales

| Componente | Fase | Para qué |
|---|---|---|
| Módulo microSD SPI | C | Almacenamiento ilimitado; los pines 5, 18, 19, 23 ya están reservados |
| Abrigo meteorológico ventilado | D | Sin él, el sol directo sobre los sensores de temperatura arruina la medición |
| Caja estanca y pasacables | D | Intemperie |
| 2 × módulo LoRa SX1276 | E | Solo si la distancia supera los ~150 m |

---

## Lo que este proyecto NO necesita

Anotado para no gastar de más:

- **Router WiFi o conexión a internet** — ESP-NOW conecta los dos ESP32 directamente
- **Sensor de lluvia** — no está en el alcance del proyecto
- **Módulo relé** — no hay nada que conmutar
- **Fuente de laboratorio** — un powerbank y el USB alcanzan para las fases 0 a 9
