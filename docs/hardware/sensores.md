# Sensores: cómo funcionan y cómo se conectan

Un archivo por consultar antes de cablear. Para la asignación de pines ver
[`pinout.md`](pinout.md).

---

## DHT22 — temperatura y humedad

Sensor digital de un solo cable, con protocolo propio.

- **Alimentación:** 3.3 V (funciona bien, no necesita 5 V)
- **Conexión:** dato a GPIO4, con resistencia de **pull-up de 10 kΩ** a 3.3 V
- **Rango:** −40 a 80 °C, 0 a 100 % HR
- **Precisión:** ±0.5 °C, ±2 % HR

**Lo que hay que saber:** no se puede leer más rápido que **una vez cada 2 segundos**. Si se lo
consulta más seguido devuelve error o el valor anterior. Como nuestro intervalo de muestreo es
de minutos, no es un problema — pero sí lo es al depurar en un bucle rápido.

Es el sensor **más confiable de los tres de temperatura** y sirve de referencia para los LM35.

---

## LM35Z — temperatura (×2)

Sensor analógico. Entrega **10 mV por grado centígrado**, referido a 0 °C.

- **Alimentación:** 4 a 30 V. Funciona a 5 V; a 3.3 V queda al límite de su especificación
- **Conexión:** salida directa al ADC1 (no supera 3.3 V en ningún rango realista de temperatura)
- **Conversión:** `°C = milivoltios / 10`

**Lo que hay que saber — este es el sensor incómodo del proyecto.** A 25 °C entrega apenas
250 mV. El ADC del ESP32 es poco lineal y ruidoso en esa zona baja del rango, así que:

- Hay que **promediar muchas lecturas** (16 o 32) para cada medición
- El error absoluto va a ser mayor que el del DHT22
- Conviene calibrar contra el UT890C y guardar el desvío en `hardware/calibraciones/`

Están dos, en pines distintos, precisamente para poder compararlos entre sí y con el DHT22.

---

## Hall A3144 (×8) — dirección del viento

Módulo con el chip **A3144** montado sobre una placa con comparador LM393. Pines: `VCC`, `GND`,
`DO` (salida digital), `AO`.

- **Alimentación: 4.5 V mínimo.** Van a **5 V**, no funcionan a 3.3 V
- **Salida:** usar **DO** únicamente. El `AO` de estos módulos no aporta nada útil con un A3144,
  porque el A3144 ya es un sensor digital: no entrega una señal proporcional al campo magnético
- **Conexión al ESP32:** DO → conversor de nivel (lado 5 V) → ESP32 (lado 3.3 V)
- **Lógica:** salida de colector abierto con pull-up. En reposo está en **alto**; con el imán
  presente baja a **bajo**

**Lo que hay que saber — el A3144 es unipolar.** Responde a **una sola cara del imán** (el polo
sur, según la hoja de datos). Si acercás el imán al revés no pasa nada. Lo primero que hay que
determinar en la fase 2 es qué cara del imán activa al sensor.

### El montaje de la veleta

Ocho sensores dispuestos en círculo, uno cada 45°, un rumbo cada uno:

```
              N (GPIO13)
       NO             NE
    (GPIO32)       (GPIO14)
  O                       E
(GPIO27)               (GPIO16)
       SO             SE
    (GPIO26)       (GPIO17)
              S (GPIO25)
```

Un imán montado en el eje de la veleta activa el sensor que tiene enfrente.

**Casos que el código debe contemplar:**

- **Ningún sensor activo** — el imán quedó entre dos posiciones o la veleta está mal montada.
  Se registra como dirección inválida, no como Norte.
- **Dos sensores adyacentes activos** — el imán está justo en el medio. Se puede aprovechar
  para reportar los 8 rumbos intermedios (NNE, ENE…) y llegar a 16 direcciones, que es la
  resolución de una veleta comercial. Mejora barata, si la geometría del imán la permite.
- **Dos sensores no adyacentes activos** — es imposible físicamente: hay un sensor fallado o un
  problema de cableado. Debe reportarse como error.

---

## Anemómetro: motor DC + paletas — velocidad del viento

Un motor de corriente continua girado por el viento funciona como generador: entrega una
tensión aproximadamente proporcional a la velocidad de giro.

- **Conexión:** bornes del motor → **divisor resistivo** → GPIO36 (ADC1)
- **El divisor es obligatorio:** un motor girando rápido puede superar los 3.3 V y dañar la
  entrada del ESP32
- **No usar el conversor de nivel:** es para señales digitales; deformaría la tensión, que es
  justamente el dato que queremos

**Lo que hay que saber — sus dos limitaciones:**

1. **Zona muerta a baja velocidad.** La fricción de las escobillas y el cogging magnético hacen
   que el motor no arranque con viento suave. Por debajo de cierta velocidad la lectura es
   cero, aunque haya viento. Hay que **medir esa velocidad de arranque** y documentarla como
   límite conocido del instrumento.
2. **La relación no es perfectamente lineal.** Por eso la calibración **no** puede ser una
   constante inventada del tipo "1 V = 3 km/h", sino una **tabla de puntos medidos** con el
   UT890C, interpolando entre ellos.

### Cómo calibrar (fase 3)

Sin túnel de viento, la técnica práctica es el auto: sacar el anemómetro por la ventanilla con
el vehículo a velocidad constante conocida, y anotar la tensión que entrega a 10, 20, 30, 40 y
50 km/h. Cinco puntos alcanzan para una interpolación decente. Los resultados van a
`hardware/calibraciones/`.

> **Alternativa si el motor resulta demasiado impreciso:** un anemómetro de pulsos, con un imán
> en el eje y un sensor Hall contando vueltas. Mide frecuencia en vez de tensión, no tiene zona
> muerta y no necesita calibración de tensión. Requeriría un noveno sensor Hall. Queda anotado
> como plan B para la fase 3.

---

## BME280 — presión atmosférica *(a comprar)*

Sensor I²C que entrega **presión, temperatura y humedad** en un solo integrado.

- **Alimentación:** 3.3 V (verificar el módulo; algunos traen regulador y aceptan 5 V)
- **Conexión:** SDA a GPIO21, SCL a GPIO22
- **Dirección I²C:** 0x76 (algunos módulos usan 0x77)

Da presión, que es lo que se busca, y de paso una tercera temperatura y una segunda humedad
para contrastar con el DHT22.

**Cuidado al comprar:** existe el **BMP280**, casi idéntico y más barato, pero **sin humedad**.
Para este proyecto sirve igual (la humedad ya la da el DHT22), pero conviene saber cuál se está
comprando.

**Verificación de la fase 4:** comparar la presión leída con la que informa el servicio
meteorológico local. No van a coincidir exactamente porque la estación oficial corrige el valor
al nivel del mar; la diferencia debería explicarse por la altitud del lugar.

---

## Sensor de gas *(a comprar)*

Dos alternativas, con un compromiso claro entre precio y comodidad.

### MQ-135 — el barato

- **Precio:** muy bajo
- **Alimentación:** 5 V obligatorio (tiene un calefactor interno)
- **Salida:** analógica, hasta 5 V → necesita **divisor resistivo** al ADC1
- **Consumo: ~150 mA de forma continua**, por el calefactor
- **Precalentamiento:** de 24 a 48 horas la primera vez para estabilizarse
- **Calibración:** difícil. Da una lectura relativa de "calidad de aire", no ppm confiables sin
  un equipo de referencia

### ENS160 — el cómodo

- **Precio:** bastante mayor
- **Alimentación:** 3.3 V, I²C — se suma al bus que ya existe
- **Salida:** valores digitales de TVOC y eCO₂, ya procesados
- **Consumo:** bajo
- **Sin divisor, sin calibración analógica, sin problema de consumo**

**Recomendación:** para un proyecto escolar el **MQ-135 alcanza** y su comportamiento (un
calefactor, una resistencia que cambia con el gas) es más fácil de explicar en la defensa del
proyecto que un integrado que ya entrega el número hecho. El consumo de 150 mA solo sería un
problema si se implementa la fase opcional de batería.

---

## DS3231 — reloj de tiempo real *(a comprar)*

Módulo I²C con pila de respaldo. **Imprescindible.**

- **Alimentación:** 3.3 V o 5 V
- **Conexión:** SDA a GPIO21, SCL a GPIO22
- **Dirección I²C:** 0x68

**Por qué no es opcional:** el ESP32 no tiene reloj propio que sobreviva a un corte de
alimentación. Sin RTC, después de cada reinicio la estación no sabe qué hora es, y una medición
sin marca temporal confiable no sirve para nada — el objetivo del proyecto es justamente
reconstruir la evolución del clima en el tiempo.

El DS3231 es notablemente preciso (±2 minutos al año) porque compensa la deriva del cristal con
la temperatura. Su hermano barato, el DS1307, se atrasa mucho más: conviene el DS3231.

---

## OLED SSD1306 + botones — interfaz local *(disponible)*

Para el nodo interfaz, en la fase opcional A.

- **OLED:** I²C, 3.3 V, dirección 0x3C, típicamente 128×64 píxeles
- **Botones:** entre el pin y GND, con `INPUT_PULLUP` interno; presionado = LOW

Los botones **rebotan**: un solo apretón genera varias transiciones eléctricas en pocos
milisegundos. Sin antirrebote por software, un apretón se lee como cinco.

---

## Conversores de nivel (×2, 8 canales cada uno) *(disponible)*

Adaptan señales **digitales** entre 5 V y 3.3 V en ambos sentidos.

- Un conversor completo se dedica a los **8 sensores Hall**
- El segundo queda de repuesto y para expansiones

**No sirven para señales analógicas.** El anemómetro y el MQ-135 usan divisores resistivos.
