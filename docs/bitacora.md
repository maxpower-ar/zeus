# Bitácora

Registro de lo que se probó, se midió y falló. Es la memoria del proyecto: dentro de tres meses
nadie va a acordarse de por qué un sensor está en un pin raro o por qué una constante vale lo
que vale.

**Anotar sobre todo lo que salió mal.** Los éxitos se ven en el código; los fracasos, si no se
escriben, se repiten.

Entrada más reciente arriba.

---

## Datos permanentes del proyecto

Completar en la fase 0.

| Dato | Valor |
|---|---|
| MAC del nodo de campo | *(pendiente, fase 0)* |
| MAC del nodo interfaz | *(pendiente, fase 0)* |
| Versión del Arduino IDE | *(pendiente)* |
| Versión del core ESP32 | *(pendiente)* |
| Puerto COM del nodo de campo | *(pendiente)* |
| Puerto COM del nodo interfaz | *(pendiente)* |
| Ubicación de la estación | *(pendiente)* |
| Altitud del lugar | *(pendiente, se necesita para interpretar la presión)* |

---

## Plantilla de entrada

```
## AAAA-MM-DD — Fase N — Título

**Qué se hizo:**

**Qué se midió:** (valores reales, con el instrumento usado)

**Qué falló:** (y cómo se resolvió, o si quedó pendiente)

**Pendiente para la próxima:**
```

---

## 2026-09-06 — Fase 1 — DHT22 verificado en banco

**Qué se hizo:** se conectó el DHT22 a GPIO4 (VCC a 3.3 V, DATA a GPIO4, GND común) y se
probó con el sketch `prueba_dht22/`. Librerías: "DHT sensor library" de Adafruit más
"Adafruit Unified Sensor".

**Qué se midió:**

| Condición | Temperatura | Humedad |
|---|---|---|
| Reposo (ambiente) | 21.0 °C | 47.7 % |
| Soplando sobre el sensor | 23.6 °C | 98.0 % |
| Recuperación tras soplar | 21.3 °C | ~60 % |
| Sensor apoyado contra la pava caliente (sin vapor) | 26.7 °C | 41 % |

Lecturas estables en reposo (±0.3 °C entre muestras consecutivas) y respuesta rápida en las
dos magnitudes. Ninguna lectura inválida durante la prueba.

**Secuencia real de la prueba, en orden:** reposo, después vapor de la pava (la humedad llegó
a 98 %), después se retiró el vapor y se apoyó el sensor contra la pava caliente, y ahí la
temperatura subió a 26.7 °C mientras la humedad caía a 41 %.

**Observación que vale documentar:** en esa última etapa la temperatura **sube** y la humedad
**baja** al mismo tiempo. No es un error: el DHT22 informa humedad *relativa*, y el aire
caliente admite mucho más vapor, así que el mismo contenido de agua representa un porcentaje
menor a mayor temperatura. Se suma que el sensor venía saturado del vapor y se estaba secando.
Sirve como demostración de que las dos magnitudes se miden de forma independiente.

**Estado del sensor:** verificado y en uso. Es el único sensor de temperatura del proyecto
hasta que llegue el BME280 de la fase 4, después del descarte de los dos LM35.

**Pendiente para la próxima:** verificar que una desconexión del DHT22 produzca una lectura
marcada como inválida (y no un cero). Completar la fase 0: anotar las MAC de las dos placas.

---

## 2026-09-06 — Fase 1 — Los dos LM35 no funcionan

**Qué se hizo:** primer banco de sensores. Se cableó un LM35Z a GPIO34 (ADC1) alimentado
desde VIN, y se escribió un sketch de prueba en `prueba_lm35/` que informa promedio, mínimo
y máximo de cada ventana de muestreo.

**Qué se midió:** (multímetro UT890C, punta negra fija en GND del ESP32)

Con el sensor **bien orientado** (cara impresa hacia el observador: izquierda +Vs, centro
Vout, derecha GND):

| Punto | Valor |
|---|---|
| Alimentación entre patas extremas | 4.8 V (correcto) |
| Salida (pata del centro) | ~10-30 mV, errático |
| Salida apretando el sensor con los dedos | no responde |
| Temperatura ambiente esperada | ~250 mV |

Se probaron **los dos LM35**, con el mismo resultado. Inscripción del encapsulado:
`89015` sobre `LM35DZ`. El encapsulado se mantiene frío, así que no hay consumo anómalo.

**Conclusión:** los dos sensores están fallados o no son LM35 (el código `9015` de la
inscripción corresponde a un transistor común; se sospecha componente clonado). Un LM35 sano
entrega 10 mV/°C y responde en segundos al calor de la mano. Estos no hacen ninguna de las
dos cosas con alimentación verificada de 4.8 V.

**Qué falló durante la búsqueda:** (queda anotado porque costó varias horas)

- **Una parte del tiempo el sensor estuvo alimentado al revés.** Con polaridad invertida el
  LM35 no se calienta —queda frío, sin señal de alarma— así que el síntoma es idéntico al de
  un sensor muerto. La orientación se verifica midiendo cada pata contra el GND del ESP32,
  no a ojo
- **Se midió con la punta negra en una pata del sensor en vez de en GND del ESP32.** Con la
  referencia moviéndose, ninguna lectura significa nada. La punta negra va siempre a un GND
  conocido
- **Se midió en la escala de 60 V.** Con paso de 10 mV es inservible para una señal de
  250 mV. Para el LM35 hay que usar el rango de 600 mV
- **`142.0 mV` es el piso del ADC del ESP32** en escala 0 db (la cuenta cero traducida a
  milivoltios), no un cero real. Un pin al aire da ese valor exacto y quieto. Sirve como
  referencia de "nada conectado": se comprobó dejando GPIO35 sin conectar como control
- El mensaje ilegible del monitor serie al arrancar es normal: es la ROM de arranque del
  ESP32 hablando a 74880 baudios antes de que el sketch abra el puerto a 115200

**Decisión:** la temperatura queda a cargo del **DHT22**, que era el sensor de referencia de
los tres. Se pierde la redundancia pero no la magnitud. El BME280 de la fase 4 va a aportar
una segunda temperatura para contrastar.

**Pendiente para la próxima:** probar el DHT22 (temperatura y humedad). Conseguir LM35
confiables queda como mejora opcional, no como bloqueo.

---

## 2026-09-05 — Diseño — Cambio de anemómetro a encoder óptico

**Qué se decidió:** el anemómetro pasa de ser un motor DC usado como dinamo a un **encoder
óptico ranurado** (el fotointerruptor en U de los kits de motor TT), que ya estaba disponible.

**Por qué:** el motor DC funciona eléctricamente —es un dinamo, `V = ke · ω`, relación
lineal— pero sus escobillas apretadas contra el colector imponen una fricción que exige un par
mínimo para arrancar. Como el par del viento crece con el cuadrado de la velocidad, por debajo
de unos 8-15 km/h el rotor no gira y el instrumento informa cero, indistinguible de la calma.
Ese rango es justamente el del viento más frecuente.

El encoder óptico no toca el eje, así que no agrega fricción, y da ~20 pulsos por vuelta en
lugar de 1-2: resolución suficiente incluso a viento muy suave. No hubo que comprar nada.

**Riesgos identificados:**

- **El sol puede cegarlo.** Es un sensor infrarrojo y el sol emite mucho infrarrojo. Requiere
  carcasa opaca obligatoria, y hay que probarlo al sol del mediodía antes de darlo por bueno
- **No puede ir en GPIO36 ni GPIO39.** Esos pines producen glitches espurios cuando el ADC
  convierte; contando pulsos, cada glitch inflaría la velocidad. Se asignó **GPIO18**
- **Hay que contar las ranuras del disco.** Se asumen 20, pero existen de 12 y 24. Un número
  equivocado desplaza toda la escala por un factor constante, con lecturas que igual parecen
  razonables

**Pendiente:** el motor DC se conserva para la fase 3b, como experimento comparativo: medir el
umbral de arranque de cada uno y documentar la diferencia.

---

## 2026-09-05 — Fase inicial — Estructura del proyecto

**Qué se hizo:** se definió la arquitectura de dos nodos, se creó la estructura de carpetas, el
plan de fases y la documentación técnica inicial.

**Decisiones tomadas:**

- **Arduino IDE** con el repositorio como sketchbook, para poder compartir código entre los dos
  sketches sin duplicarlo (`libraries/EstacionComun/`)
- **ESP-NOW** como transporte, detrás de una interfaz que permita cambiarlo por LoRa más
  adelante sin reescribir el protocolo
- **LittleFS** en la flash interna para el log; microSD queda como fase opcional
- **Deep-sleep diferido a fase opcional**, porque reinicia el ESP32 en cada ciclo y eso vuelve
  mucho más difícil depurar todo lo demás. El proyecto funciona igual alimentado
- Los 8 sensores Hall se usan como **veleta de 8 rumbos**, no para el anemómetro
- La velocidad del viento se mide con el **motor DC como generador**, con calibración por tabla
  de puntos medidos (no por una constante inventada)

**Restricciones de hardware identificadas:**

- ADC2 no funciona con la radio encendida: todas las analógicas van a ADC1 (32, 33, 34, 35, 36, 39)
- GPIO 0, 2, 12, 15 son pines de strapping: ningún sensor con pull-up ahí
- El A3144 necesita 4.5 V mínimo, así que va a 5 V y su salida pasa por el conversor de nivel
- El conversor de nivel no sirve para señales analógicas: anemómetro y gas necesitan divisor

**Pendiente para la próxima:** fase 0 — instalar el entorno, compilar en ambas placas y anotar
las dos direcciones MAC en la tabla de arriba.
