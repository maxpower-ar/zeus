# Plan de trabajo

Dividido en fases de lo más simple a lo más complejo. **Cada fase termina con algo que se puede
probar y ver funcionar** — nunca se escribe todo para probar al final.

Marcá la casilla cuando la fase esté terminada **y verificada sobre la placa**, no cuando el
código compile.

El plan tiene dos partes: el **núcleo** (fases 0–9), que es el proyecto completo y entregable,
y las **fases opcionales**, que solo se abordan una vez terminado el núcleo.

---

## NÚCLEO

### Fase 0 — Puesta a punto del entorno
> Sin componentes nuevos.

- [ ] Instalar Arduino IDE y el soporte para placas ESP32
- [ ] Configurar la ubicación del sketchbook apuntando a este repositorio
- [ ] Crear los dos sketches vacíos (`estacion_campo`, `estacion_interfaz`) y compilarlos
- [ ] Subir un parpadeo de LED a **ambas** placas y confirmar que las dos funcionan
- [ ] Imprimir por serie la **dirección MAC** de cada placa y anotarla en `docs/bitacora.md`
      (la vamos a necesitar en la fase 7 para el enlace ESP-NOW)

**Verificación:** las dos placas parpadean y cada una informa su MAC por el monitor serie a 115200.

---

### Fase 1 — Sensores de banco, uno por vez
> Sin componentes nuevos. DHT22 + 2 × LM35Z.

- [ ] DHT22 conectado y leyendo temperatura y humedad
- [ ] LM35 #1 leyendo temperatura por ADC1
- [ ] LM35 #2 leyendo temperatura por ADC1
- [ ] Contrastar las tres temperaturas entre sí y contra el UT890C
- [ ] Registrar el error de cada sensor en `hardware/calibraciones/`
- [ ] Cada función de lectura devuelve además si el dato es **válido** (sensor desconectado
      debe producir un dato inválido, no un cero silencioso)

**Verificación:** las tres temperaturas coinciden entre sí dentro de ~1 °C y con el multímetro.

> **Nota sobre el LM35:** entrega 10 mV por °C. A 25 °C son 250 mV, muy abajo del rango del
> ADC del ESP32, que es poco lineal cerca de 0 V. Esperar ruido y necesitar promediado de
> varias lecturas. Es el sensor más incómodo del proyecto; por eso está el DHT22 al lado.

---

### Fase 2 — Veleta de 8 rumbos
> Sin componentes nuevos. 8 × Hall A3144 + conversor de nivel.

- [ ] Un solo Hall funcionando: acercar un imán y ver el cambio en el serie
- [ ] Determinar la **polaridad** del imán que activa al A3144 (es unipolar: responde a una
      sola cara del imán)
- [ ] Cablear los 8 Hall a 5 V, pasando sus salidas por el conversor de nivel a 3.3 V
- [ ] Traducir el patrón de los 8 a un rumbo (N, NE, E, SE, S, SO, O, NO)
- [ ] Manejar los casos raros: ningún sensor activo, o dos activos a la vez (el imán entre
      dos posiciones)

**Verificación:** girás el imán a mano y el monitor serie anuncia el rumbo correcto.

---

### Fase 3 — Anemómetro
> Sin componentes nuevos. Motor DC + paletas.

- [ ] Divisor resistivo a la salida del motor para no superar 3.3 V en el ADC
      (**el conversor de nivel no sirve para señales analógicas**)
- [ ] Leer la tensión generada y verificarla con el UT890C en paralelo
- [ ] Construir la tabla de calibración tensión → km/h y guardarla en `hardware/calibraciones/`
- [ ] Implementar la conversión por interpolación entre los puntos medidos
- [ ] Determinar la **velocidad de arranque**: por debajo de qué viento el motor no gira

**Verificación:** una tabla de calibración real, medida, documentada, y la lectura en km/h
coherente con ella.

> **Advertencia esperada:** un motor DC tiene zona muerta a baja velocidad (la fricción de las
> escobillas impide que gire con brisa suave) y la relación tensión→velocidad no es
> perfectamente lineal. Por eso la calibración es una **tabla de puntos medidos**, no una
> fórmula inventada de antemano.

---

### Fase 4 — Sensores nuevos: presión, gas y reloj
> Requiere comprar BME280, sensor de gas y DS3231.

- [ ] Escanear el bus I²C y ver las tres direcciones (BME280 0x76, DS3231 0x68, OLED 0x3C)
- [ ] BME280 entregando presión, temperatura y humedad
- [ ] Comparar la presión con la del servicio meteorológico local (corregida por altitud)
- [ ] DS3231 con la hora puesta y conservándola tras desenchufar la placa
- [ ] Sensor de gas leyendo (con su tiempo de precalentamiento respetado)

**Verificación:** presión creíble, la hora sobrevive a un corte de alimentación.

---

### Fase 5 — Muestreo unificado
> Sin componentes nuevos.

- [ ] Definir el struct `Medicion` en `libraries/EstacionComun/`
- [ ] Un ciclo de muestreo que lee **todos** los sensores y arma un `Medicion` completo
- [ ] Marca de tiempo del RTC en cada medición
- [ ] Campo de banderas de validez: qué sensores respondieron bien en ese ciclo
- [ ] Intervalo de muestreo configurable desde `config.h`

**Verificación:** una línea por ciclo en el monitor serie con todas las magnitudes y su hora.

---

### Fase 6 — Persistencia en la memoria del nodo
> Sin componentes nuevos. LittleFS en la flash interna.

- [ ] Formatear y montar LittleFS
- [ ] Escribir cada `Medicion` al archivo de log en formato binario de tamaño fijo
- [ ] Log **circular**: cuando se llena, el registro más viejo cede lugar al más nuevo
- [ ] Leer el histórico de vuelta y verificar que no se corrompió
- [ ] Calcular y documentar cuántos días de autonomía de almacenamiento da

**Verificación:** se corta la alimentación, vuelve, y el histórico anterior sigue intacto.

---

### Fase 7 — Enlace por radio entre los dos nodos
> Sin componentes nuevos. ESP-NOW.

- [ ] Definir el formato de paquete en `libraries/EstacionComun/`
- [ ] Emparejar los dos nodos por sus MAC
- [ ] El nodo interfaz envía "dame la medición actual" y el de campo responde
- [ ] Verificación de integridad (CRC) en cada paquete
- [ ] Manejar la falta de respuesta: reintentos y un mensaje claro de "nodo no encontrado"
- [ ] Medir el **alcance real** en el lugar donde va a estar instalada

**Verificación:** apretás un botón en el nodo interfaz y aparece la medición actual del campo.

---

### Fase 8 — Descarga del histórico
> Sin componentes nuevos. Es la fase más difícil del proyecto.

- [ ] Comando "cuántos registros tenés" y respuesta con el total
- [ ] Descarga por bloques con numeración de secuencia
- [ ] Confirmación (ACK) por bloque y reenvío de los que se perdieron
- [ ] Detección de huecos: la descarga falla ruidosamente antes que entregar datos incompletos
- [ ] Descarga incremental: pedir solo lo posterior al último registro ya bajado
- [ ] Prueba con el log lleno de miles de registros

**Verificación:** se descargan miles de registros, se cuentan, y coinciden exactamente con los
que el nodo de campo dijo tener.

---

### Fase 9 — Salida a la computadora
> Sin componentes nuevos.

- [ ] Formato de salida por puerto serie definido y documentado en `docs/formato-datos.md`
- [ ] Una línea CSV por medición, con encabezado
- [ ] Los datos inválidos se distinguen de los válidos (no se disfrazan de cero)
- [ ] Script de prueba en `tools/` que lee el puerto y guarda un archivo
- [ ] Abrir el archivo resultante en una planilla de cálculo y graficarlo

**Verificación:** un archivo CSV abierto en una planilla, con un gráfico de temperatura contra
tiempo que tenga sentido.

**Con la fase 9 terminada, el proyecto está completo y es presentable.**

---

## FASES OPCIONALES

Solo después de que el núcleo esté terminado y verificado. Ninguna de estas es necesaria para
que el proyecto funcione ni para presentarlo. Están ordenadas por relación entre lo que aportan
y lo que cuestan.

### Fase A (opcional) — Display OLED y navegación por botones
> Requiere: OLED I²C y botones (ya disponibles).

Pantalla en el nodo interfaz para ver el estado y operar sin PC: medición actual, cantidad de
registros almacenados, estado del enlace, y un menú de tres botones (arriba / abajo / OK) para
disparar la descarga.

Es la más agradecida de las opcionales: mejora mucho la demostración del proyecto y es de
dificultad baja. Si sobra tiempo, hacer esta primero.

- [ ] OLED mostrando la medición actual
- [ ] Tres botones con `INPUT_PULLUP` y antirrebote
- [ ] Menú de navegación
- [ ] Indicador de estado del enlace con el nodo de campo

### Fase B (opcional) — Ahorro de energía y deep-sleep
> Requiere: nada nuevo, pero cambia la estructura del firmware de campo.

Dormir entre mediciones para que la batería dure días en lugar de horas.

**Se difiere a propósito:** el deep-sleep reinicia el ESP32 en cada ciclo, lo que complica la
depuración de todo lo demás (se pierden las variables, el monitor serie se desconecta, los
sensores se reinician). Meterlo antes de tiempo hace que todos los bugs anteriores sean más
difíciles de encontrar. Con el nodo alimentado y despierto, el proyecto funciona igual.

- [ ] Medir el consumo real despierto con el UT890C
- [ ] Deep-sleep entre muestras, despertando por temporizador
- [ ] Verificar que el log en LittleFS sobrevive al reinicio de cada ciclo
- [ ] Medir la tensión de batería por divisor y registrarla en cada medición
- [ ] Estimar y documentar la autonomía en días

### Fase C (opcional) — Tarjeta microSD
> Requiere comprar: módulo microSD SPI.

Almacenamiento prácticamente ilimitado, y la posibilidad de leer los datos sacando la tarjeta
y poniéndola en la PC. Los pines SPI (5, 18, 19, 23) ya están reservados para esto.

La capa de almacenamiento del nodo de campo se diseña desde la fase 6 con una interfaz común
(`guardar` / `leer` / `contar`), de modo que agregar la SD no obligue a tocar el resto.

- [ ] Módulo SD por SPI, leyendo y escribiendo
- [ ] Implementar la misma interfaz de almacenamiento sobre SD
- [ ] Elegir el destino desde `config.h`

### Fase D (opcional) — Intemperie y robustez
> Requiere: abrigo meteorológico, caja estanca.

Lo que separa un prototipo de banco de una estación que sobrevive a la lluvia.

- [ ] Watchdog: la placa se reinicia sola si se cuelga
- [ ] Recuperación automática de fallos de sensores
- [ ] Abrigo meteorológico ventilado para los sensores de temperatura (el sol directo sobre
      un sensor arruina la medición)
- [ ] Caja estanca y pasacables
- [ ] Prueba de varios días continuos

### Fase E (opcional) — Alcance largo con LoRa
> Requiere comprar: 2 × módulo LoRa SX1276.

Si el nodo de campo termina quedando a más de ~150 metros, ESP-NOW no alcanza. La capa de
transporte está detrás de una interfaz (`enviar` / `recibir`) justamente para poder cambiar la
radio sin reescribir el protocolo ni el almacenamiento.

---

## Cómo se usa este archivo

Al empezar cada sesión de trabajo, mirar cuál es la primera fase sin terminar: esa es la que se
trabaja. No adelantar trabajo de fases posteriores ni implementar fases opcionales antes de
cerrar el núcleo.
