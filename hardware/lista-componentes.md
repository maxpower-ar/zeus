# Lista de componentes

**Consultar este archivo antes de proponer cualquier solución que requiera hardware.** Si un
componente no está acá, no existe en el proyecto.

---

## Disponible

| Componente | Cant. | Uso | Alimentación | Nota |
|---|---|---|---|---|
| ESP32 DevKit V1 (30 pines, WROOM-32) | 2 | **Delta**: la estación. **Beta**: retirada, queda de repuesto | 5 V por USB | Ver `img/esp32.jpeg` |
| DHT22 | 1 | Temperatura y humedad | 3.3 V | Máximo una lectura cada 2 s |
| ~~LM35Z~~ | 2 | ~~Temperatura (redundante)~~ | 5 V | **Fallados**, descartados (bitácora 2026-09-06) |
| Módulo Hall A3144 | 8 | Dirección del viento (postergada) y pluviómetro (opcional) | **5 V mínimo** | Salida digital. Ver `img/hall.jpeg` |
| Conversor de nivel 8 canales | 2 | Hall 5 V → ESP32 3.3 V | 5 V y 3.3 V | **Solo señales digitales** |
| Display OLED I²C | 1 | Pantalla de estado (fase opcional A) | 3.3 V | SSD1306, dirección 0x3C |
| Botones | varios | Navegación del menú | — | Con `INPUT_PULLUP`, a GND |
| **Encoder óptico ranurado en U (FC-03)** + rueda | 1 | **Anemómetro** (sensor elegido) | Probar a 3.3 V | LM393, 4 pines (VCC/GND/DO/AO). Ver `img/lm393.jpeg`. Necesita carcasa opaca: el sol lo ciega |
| Cooler Jamicon JF0825B1xx, 80 mm | 1 | **Candidato a rotor y eje** del anemómetro | 12 V (no se alimenta) | Brushless, **doble rodamiento a bolillas**. Ver `img/cooler_JF0825B1HS-R.jpeg` |
| Motor DC + paletas | 1 | Anemómetro de comparación (fase 3b) | — | Descartado como instrumento. Requiere divisor resistivo |
| Multímetro **UT890C** | 1 | Instrumento de referencia | — | Toda calibración se contrasta con él |

---

## A comprar

Ordenados por prioridad.

### Imprescindibles

| Componente | Para qué | Aprox. | Por qué es imprescindible |
|---|---|---|---|
| **DS3231** (módulo RTC I²C con pila) | Respaldo de la hora | bajo | La hora principal viene de internet (NTP), pero si la placa se reinicia durante un corte de red no tiene de dónde sacarla. Sin el DS3231, esas mediciones quedan sin hora y no sirven |
| **BME280** (módulo I²C) | Presión | medio | Es la magnitud que falta. De paso da una segunda temperatura y humedad para contrastar con el DHT22. Se pidió el 2026-10-27 pero **no se consiguió** (2026-11-22). Pendiente |

**Al comprar el BME280:** existe el **BMP280**, casi idéntico y más barato pero **sin humedad**.
Se venden mucho confundidos: el BME280 tiene el chip casi cuadrado, el BMP280 más rectangular.
Conviene verificarlo con el escáner I²C y la librería al llegar.

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

### Sensor de luz — a definir

La API del ITR pide **luminosidad** (lux). No hay sensor de luz en el proyecto. Opción
sugerida: **BH1750** (I²C, 3.3 V, da lux directamente, dirección 0x23, barato). Hasta que se
compre, el campo va como `NaN`.

### Consumibles del armado

| Componente | Para qué |
|---|---|
| Resistencias variadas (1 kΩ, 2.2 kΩ, 10 kΩ) | Divisores resistivos y pull-up del DHT22 |
| Imán de neodimio pequeño | Eje de la veleta |
| Rodamiento libre | Eje del anemómetro. **Solo si no sirve el del cooler**, que ya trae doble rodamiento a bolillas |
| Caja o tubo opaco | Carcasa del encoder óptico. **Imprescindible**: sin ella el sol lo deja ciego |
| Cables Dupont y protoboard | Armado de banco |
| Fuente de 5 V / 2 A o powerbank | Alimentación |

### Red

| Componente | Para qué | Nota |
|---|---|---|
| Acceso a una red WiFi de **2.4 GHz** | Enviar los datos al servidor | El ESP32 **no** se conecta a redes de 5 GHz. Depende de qué red ofrezca el ITR (ver fase 7) |
| Router propio *(solo si hace falta)* | Red aparte para la estación | Solo si la red del ITR tiene portal cautivo o WPA2-Enterprise. Uno básico de 2.4 GHz alcanza, conectado por cable a la red de la escuela, con permiso de sistemas |

---

## Solo si se hacen las fases opcionales

| Componente | Fase | Para qué |
|---|---|---|
| Módulo microSD SPI | C | Almacenamiento ilimitado; los pines 5, 18, 19, 23 ya están reservados |
| Abrigo meteorológico ventilado | D | Sin él, el sol directo sobre los sensores de temperatura arruina la medición |
| Caja estanca y pasacables | D | Intemperie |
| Imán de neodimio + balancín impreso | F | Pluviómetro, usando un A3144 de los disponibles |

---

## Lo que este proyecto NO necesita

Anotado para no gastar de más:

- **Sensor de lluvia de placa** (los de "gotas") — solo dicen si llueve, no cuánto. Si se
  mide lluvia, es con el pluviómetro de balancín (fase opcional F)
- **Módulos LoRa** — con WiFi ya no hace falta radio entre dos nodos
- **Módulo relé** — no hay nada que conmutar
- **Fuente de laboratorio** — un powerbank y el USB alcanzan para el núcleo
