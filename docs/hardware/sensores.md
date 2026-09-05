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

## Anemómetro: encoder óptico ranurado — velocidad del viento

**Instrumento elegido.** Módulo fotointerruptor en U (los que se usan con los motores TT
amarillos) más su rueda ranurada, acoplada al eje de las paletas.

Un LED infrarrojo de un lado de la U, un fototransistor del otro y un comparador LM393 que
limpia la señal. Cada ranura del disco corta el haz y produce un pulso. La frecuencia de pulsos
es proporcional a la velocidad de giro, y esta a la del viento.

- **Alimentación:** probar primero a **3.3 V**. El LM393 funciona desde 2 V, así que muchos de
  estos módulos andan directo sin conversor de nivel. Verificar la tensión de salida con el
  UT890C antes de conectar al ESP32
- **Conexión:** salida digital `DO` → **GPIO18**
- **Lectura:** interrupción por flanco + `micros()`, midiendo el **período entre pulsos**

### Por qué este y no el motor DC

| | Motor DC | Imán + Hall | **Encoder óptico** |
|---|---|---|---|
| Fricción que agrega al eje | Escobillas (mucha) | Ninguna | **Ninguna** |
| Pulsos por vuelta | — | 1 o 2 | **~20** |
| Umbral de arranque | 8-15 km/h | 2-4 km/h | **2-4 km/h** |
| Señal | Analógica, ruidosa | Digital | **Digital, con histéresis** |
| Depende del ADC | Sí | No | **No** |
| Hay que comprar | — | Un sensor Hall | **Nada** |

Lo decisivo es la **resolución a viento bajo**. Con un imán y un Hall, una vuelta lenta da un
pulso cada dos segundos y hay que esperar mucho para saber la velocidad. Con 20 ranuras, esa
misma vuelta entrega 20 pulsos y la medición sale en una fracción de segundo.

Al ser óptico y sin contacto, no agrega **nada** de fricción: el umbral de arranque queda
determinado únicamente por el rodamiento que se use en el eje.

### El problema del sol — resolver en el montaje, no en el código

Es un sensor **infrarrojo**, y el sol emite muchísimo infrarrojo. A la intemperie, la luz
directa entrando en la ranura de la U puede saturar el fototransistor y dejarlo permanentemente
"iluminado": el anemómetro dejaría de contar pulsos justo en los días despejados.

Requisitos de construcción, no negociables:

- El sensor va **dentro de una carcasa opaca**, con la rueda ranurada adentro y solo el eje
  saliendo por un orificio ajustado
- Nunca dejar la ranura de la U expuesta al aire libre
- La misma carcasa lo protege del agua, el polvo y los insectos, que en una ranura de 3 mm son
  un problema real

**Verificación obligatoria: probarlo al sol del mediodía** antes de darlo por bueno. Es la única
prueba que importa acá.

### Dos trampas técnicas

**No conectarlo a GPIO36 ni a GPIO39.** El ESP32 tiene un defecto de silicio conocido: esos dos
pines producen micro-glitches espurios cuando el ADC hace conversiones. Para leer una tensión no
importa, pero acá se están **contando pulsos**: cada glitch se contaría como una ranura y la
velocidad saldría inflada. Por eso el encoder va a **GPIO18**.

**Contar las ranuras del disco.** Se asume que son 20 porque es lo habitual en los kits de motor
TT, pero también hay discos de 12 y de 24. Si el número está mal, toda la escala de velocidad
queda mal por un factor constante — y es el peor tipo de error, porque las lecturas siguen
pareciendo razonables. Contarlas a ojo y anotarlas en `hardware/calibraciones/anemometro.md`.

### Cómo calibrar (fase 3)

Con cazoletas hay un factor que no se puede calcular en el escritorio: **giran más lento que el
viento**, típicamente entre 2.5 y 3 veces más lento, según la geometría del rotor. Es empírico.

La ventaja frente al motor DC es que ahora se calibra **una sola constante**, no una curva:

```
velocidad_viento = K · (pulsos_por_segundo / ranuras_por_vuelta) · circunferencia
```

Sin túnel de viento, el método práctico es el auto: el anemómetro por la ventanilla, el vehículo
a velocidad constante, y anotar la frecuencia de pulsos a 20, 30, 40 y 50 km/h. Como la relación
es lineal, la recta que pasa por esos puntos da `K`. Después se contrasta con lo que informa el
servicio meteorológico local en un día ventoso.

---

## Motor DC + paletas — anemómetro de comparación *(experimento, no instrumento)*

Un motor de corriente continua girado por el viento funciona como **dinamo**: las bobinas del
rotor cortan el campo magnético de los imanes del estator y se induce una fuerza electromotriz
(ley de Faraday). Es la misma máquina funcionando al revés, sin ningún circuito adicional.

La relación es **genuinamente lineal**: `V = ke · ω`, donde `ke` es una constante física del
motor. La idea de "tantos voltios equivalen a tantos km/h" es conceptualmente correcta; solo hay
que medir la constante en vez de suponerla.

- **Conexión:** bornes del motor → **divisor resistivo** → GPIO36 (ADC1)
- **El divisor es obligatorio:** un motor girando rápido puede superar los 3.3 V y dañar la
  entrada del ESP32
- **No usar el conversor de nivel:** es para señales digitales; deformaría la tensión, que es
  justamente el dato que queremos medir

### Por qué quedó descartado como instrumento

El problema no está en la electricidad sino en la mecánica. Las escobillas van apretadas contra
el colector por resortes: eso es fricción estática permanente, y vencerla requiere un par
mínimo. El par que entrega el viento crece con el **cuadrado** de la velocidad, así que a viento
suave hay muy poco par disponible.

Por debajo de ese umbral —típicamente **8 a 15 km/h** en un armado casero— el rotor no arranca y
el sensor informa cero. No informa "poco viento": informa exactamente lo mismo que informaría en
calma absoluta. Y la brisa de un día cualquiera suele estar justo por debajo de esa cifra.

Como problema secundario, las escobillas rebotan y el colector produce ondulación, así que la
tensión sale sucia y hay que promediarla.

### Para qué se conserva

Como **experimento comparativo** en la fase 3. Medir con instrumentos propios que un anemómetro
no detecta nada por debajo de 11 km/h y el otro arranca a 3 convierte la elección de diseño en
un resultado experimental verificable, en lugar de una afirmación tomada de una tabla. Es
material valioso para la defensa del proyecto, y el motor ya está comprado.

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
