# Lista de componentes

**Consultar este archivo antes de proponer cualquier solución que requiera hardware.** Si un
componente no está acá, no existe en el proyecto.

---

## Disponible

| Componente | Cant. | Uso | Alimentación | Nota |
|---|---|---|---|---|
| ESP32 DevKit V1 (30 pines, WROOM-32) | 2 | **Delta**: la estación. **Beta**: retirada, queda de repuesto | 5 V por USB | Ver `img/esp32.jpeg` |
| DHT22 | 1 | Temperatura y humedad, GPIO4 | 3.3 V | Máximo una lectura cada 2 s. Verificado en banco |
| ~~LM35Z~~ | 2 | ~~Temperatura (redundante)~~ | 5 V | **Fallados**, descartados (bitácora 2026-09-06) |
| Módulo Hall A3144 | 8 | Dirección del viento (postergada) y pluviómetro (opcional) | **5 V mínimo** | Salida digital. Ver `img/hall.jpeg` |
| Conversor de nivel 8 canales | 2 | Hall 5 V → ESP32 3.3 V | 5 V y 3.3 V | **Solo señales digitales** |
| Display OLED I²C 128×64 | 1 | **Pantalla de la estación**: hora, WiFi, última medición, pendientes | 3.3 V | SSD1306, dirección 0x3C, bus I²C (GPIO21/22). Librerías *Adafruit SSD1306* + *GFX* |
| Botones | varios | Navegación del menú | — | Con `INPUT_PULLUP`, a GND |
| **Encoder óptico ranurado en U (FC-03)** + rueda | 1 | **Anemómetro** (sensor elegido), GPIO18, disco de 30 ranuras | 3.3 V | LM393, 4 pines (VCC/GND/DO/AO). Ver `img/lm393.jpeg`. Necesita carcasa opaca: el sol lo ciega. `METROS_POR_PULSO` **sin calibrar** |
| Cooler Jamicon JF0825B1xx, 80 mm | 1 | **Candidato a rotor y eje** del anemómetro | 12 V (no se alimenta) | Brushless, **doble rodamiento a bolillas**. Ver `img/cooler_JF0825B1HS-R.jpeg` |
| Motor DC + paletas | 1 | Anemómetro de comparación (fase 3b) | — | Descartado como instrumento. Requiere divisor resistivo |
| Multímetro **UT890C** | 1 | Instrumento de referencia | — | Toda calibración se contrasta con él |

---

## En camino

| Componente | Para qué | Nota |
|---|---|---|
| **BMP180** (módulo I²C) | **Presión** (y una segunda temperatura para comparar con el DHT22) | Reemplaza al BME280, que no se consiguió. **No mide humedad.** I²C 0x77, en el bus de GPIO21/22. Librería *Adafruit BMP085 Library*. El firmware ya lo lee (`BMP180_INSTALADO = true`): mientras no esté conectado manda ERR |

| **MQ-7** (módulo, CO) | **Monóxido de carbono** en ppm (campo `gas` / `co_concentracion`) | 5 V, ~150 mA. AO por divisor 10 kΩ / 20 kΩ a GPIO39. El firmware ya lo lee (`MQ7_INSTALADO = true`): sin conectar manda ERR. **Sin calibrar** (`hardware/calibraciones/mq7.md`) |
| Resistencias 10 kΩ ×3 | Divisor del MQ-7 (una arriba, dos en serie abajo) | Medirlas con el UT890C |

Cómo se conecta cada cosa: [`conexiones.md`](conexiones.md).

Se manda la **presión absoluta** de la estación, en hPa. No se corrige a nivel del mar: para
eso hace falta la altitud del lugar medida, y todavía no la tenemos.

## A comprar

Ordenados por prioridad.

### Imprescindibles

| Componente | Para qué | Aprox. | Por qué es imprescindible |
|---|---|---|---|
| **DS3231** (módulo RTC I²C con pila) | Respaldo de la hora | bajo | La hora principal viene de internet (NTP), pero si la placa se reinicia durante un corte de red no tiene de dónde sacarla. Sin el DS3231, la estación no mide hasta tener red. **El firmware ya lo soporta** (`RTC_INSTALADO` en `config.h`, desactivado) |

**Al comprar el DS3231:** evitar el DS1307, que es más barato pero se atrasa mucho. El DS3231
compensa la temperatura y se desvía ~2 minutos por año.

### Sensor de gas — elegir uno

| Opción | Precio | Alimentación | Ventaja | Desventaja |
|---|---|---|---|---|
| **MQ-135** | muy bajo | 5 V | Barato; su principio de funcionamiento es fácil de explicar | Analógico (necesita divisor), consume ~150 mA continuos, 24-48 h de precalentamiento inicial, calibración difícil |
| **ENS160** | alto | 3.3 V | I²C, bajo consumo, entrega TVOC y eCO₂ ya procesados | Bastante más caro; el procesamiento está oculto adentro del chip |

**Elegido: MQ-7** (específico para CO, ver "En camino"). La tabla queda como registro de lo evaluado.

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
