# Plan de trabajo

Dividido en fases de lo más simple a lo más complejo. **Cada fase termina con algo que se puede
probar y ver funcionar** — nunca se escribe todo para probar al final.

Marcá la casilla cuando la fase esté terminada **y verificada sobre la placa**, no cuando el
código compile.

El plan tiene dos partes: el **núcleo** (fases 0–10), que es el proyecto completo y entregable,
y las **fases opcionales**, que solo se abordan una vez terminado el núcleo.

> **Cambio de arquitectura (2026-10-27):** el proyecto pasó de dos nodos con ESP-NOW a **un
> solo ESP32 ("Delta") que manda los datos por WiFi a un servidor del ITR**. Las fases 7 a 9
> se reescribieron y la veleta (fase 2) se posterga. Ver la bitácora de esa fecha.

---

## NÚCLEO

### Fase 0 — Puesta a punto del entorno
> Sin componentes nuevos.

- [ ] Instalar Arduino IDE y el soporte para placas ESP32
- [ ] Configurar la ubicación del sketchbook apuntando a este repositorio
- [ ] Crear el sketch `estacion_campo` y compilarlo
- [ ] Subir un parpadeo de LED a la placa Delta y confirmar que funciona
- [ ] Imprimir por serie la **dirección MAC** y anotarla en `docs/bitacora.md`
      (puede pedirla el área de sistemas del ITR para autorizar la placa en la red WiFi)

**Verificación:** la placa parpadea e informa su MAC por el monitor serie a 115200.

---

### Fase 1 — Sensores de banco, uno por vez
> Sin componentes nuevos. DHT22 + 2 × LM35Z.

- [x] DHT22 conectado y leyendo temperatura y humedad
- [ ] ~~LM35 #1 leyendo temperatura por ADC1~~ — **sensor fallado**, ver bitácora 2026-09-06
- [ ] ~~LM35 #2 leyendo temperatura por ADC1~~ — **sensor fallado**, descartado
- [ ] Contrastar las temperaturas entre sí y contra el UT890C *(solo queda el DHT22; se retoma en la fase 4 con el BMP180)*
- [x] Registrar el error de cada sensor en `hardware/calibraciones/` (ver `temperatura.md`)
- [ ] Cada función de lectura devuelve además si el dato es **válido** (sensor desconectado
      debe producir un dato inválido, no un cero silencioso) *(escrito 2026-11-22: estado
      por campo y `ERR` en `estacion_campo/sensores.cpp`; falta desconectar el DHT22 en la placa)*

**Verificación:** las tres temperaturas coinciden entre sí dentro de ~1 °C y con el multímetro.

> **Nota sobre el LM35:** entrega 10 mV por °C. A 25 °C son 250 mV, muy abajo del rango del
> ADC del ESP32, que es poco lineal cerca de 0 V. Esperar ruido y necesitar promediado de
> varias lecturas. Es el sensor más incómodo del proyecto; por eso está el DHT22 al lado.

---

### Fase 2 — Veleta de 8 rumbos — **POSTERGADA**
> Se rediseña con sensores Hall más adelante (ver fase opcional E). Mientras tanto el campo
> `direccion` se envía como `NaN`. Lo de abajo queda como referencia del diseño original.

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
> Sin componentes nuevos. Encoder óptico ranurado (el de los motores TT) + paletas.

**Instrumento:** módulo fotointerruptor en U con su rueda ranurada acoplada al eje de las
paletas. Cada ranura corta el haz infrarrojo y produce un pulso; la frecuencia de pulsos da la
velocidad de giro.

- [ ] **Elegir el rotor y el eje**, comparando a mano el cooler Jamicon (doble rodamiento a
      bolillas) contra el eje del kit TT: girarlos con el dedo y cronometrar cuánto siguen
      girando solos. Más tiempo de giro libre = menos fricción = detecta brisa más suave.
      Anotar en `hardware/calibraciones/anemometro.md`
- [x] **Ranuras del disco: 30**, confirmado contra el modelo OpenSCAD que genera la pieza
      (`enc_ranuras = 30`, disco de 69 mm, ranuras entre r = 23.5 y 33.5 mm)
- [ ] Posicionar el FC-03 con el haz a **r ≈ 28.5 mm**, el centro de la banda radial de las
      ranuras. Hay 10 mm de banda, así que sobra tolerancia, pero conviene no acercarse al
      borde exterior: afuera de r = 33.5 solo queda 1 mm de disco macizo
- [ ] Medir velocidad en **m/s y km/h** con `pruebas/anemometro/`, que trae el modo de
      calibración contra una referencia conocida
- [ ] Probar el módulo **a 3.3 V**: si funciona, no hace falta conversor de nivel. Verificar la
      tensión de salida con el UT890C antes de conectarlo al ESP32
- [ ] Contar pulsos con interrupción en **GPIO18** (nunca en GPIO36 ni 39: producen glitches
      espurios cuando el ADC convierte, y cada glitch se contaría como una ranura)
- [ ] Medir el **período entre pulsos** con `micros()` en vez de contar en una ventana fija:
      da buena resolución incluso a viento suave
- [ ] Determinar la **velocidad de arranque** del rotor
- [ ] Calibrar la constante `K` con el método del auto y guardarla en
      `hardware/calibraciones/anemometro.md`
- [ ] **Probar el sensor al sol del mediodía**

**Verificación:** la velocidad leída se corresponde con la del velocímetro del auto en la prueba
de calibración, y el sensor sigue contando pulsos bajo sol directo.

> **La prueba del sol no es opcional.** El sensor es infrarrojo y el sol emite mucho infrarrojo:
> la luz directa en la ranura de la U puede saturar el fototransistor y dejar el anemómetro
> ciego justo en los días despejados. Se resuelve en el montaje —carcasa opaca, con la rueda
> adentro y solo el eje saliendo—, no en el código. Esa misma carcasa lo protege del agua, el
> polvo y los insectos.

> **Sobre el cooler como rotor:** un ventilador brushless sin alimentar no gira libre — los
> imanes del rotor son atraídos por los polos de hierro del estator (*cogging*), y eso impone un
> umbral de arranque igual que las escobillas de un motor. Si al girarlo con el dedo se siente
> "por pasitos", conviene desarmarlo y quitarle el estator: queda solo el doble rodamiento, sin
> cogging, que es lo único que interesa de esa pieza.

#### Fase 3b — Comparación con el anemómetro de motor DC *(opcional pero recomendada)*

El motor ya está comprado y montarlo cuesta una tarde. Medir sus límites convierte la elección
de diseño en un resultado experimental propio.

- [ ] Divisor resistivo a la salida del motor (**el conversor de nivel no sirve para señales
      analógicas**) hacia GPIO36
- [ ] Medir su **velocidad de arranque** y compararla con la del encoder
- [ ] Documentar la comparación en `hardware/calibraciones/anemometro.md`

Poder escribir "el anemómetro de motor no detecta nada por debajo de 11 km/h, el de encoder
arranca a 3" con mediciones propias vale mucho más en la defensa del proyecto que citar una
tabla ajena.

---

### Fase 4 — Sensores nuevos: presión, gas y reloj
> Requiere BMP180 (**en camino**; reemplaza al BME280, que no se consiguió), sensor de gas y
> DS3231. El DS3231 ahora es **respaldo** de la hora NTP (fase 7): da la hora cuando la placa
> arranca sin red. El código del BMP180 y del DS3231 ya está en `estacion_campo/`,
> desactivado en `config.h` (2026-11-22).

- [ ] Escanear el bus I²C y ver las direcciones (OLED 0x3C, BMP180 0x77, DS3231 0x68)
- [ ] BMP180 entregando presión (absoluta, hPa) y temperatura. **No mide humedad**
- [ ] Comparar la presión con la del servicio meteorológico local (la diferencia se explica
      por la altitud, que hay que medir: no se inventa)
- [ ] DS3231 con la hora puesta y conservándola tras desenchufar la placa
- [ ] Sensor de gas leyendo (con su tiempo de precalentamiento respetado)

**Verificación:** presión creíble, la hora sobrevive a un corte de alimentación.

---

### Fase 5 — Muestreo unificado
> Sin componentes nuevos.

- [ ] Definir el struct `Medicion` en `libraries/EstacionComun/`
- [ ] Un ciclo de muestreo que lee **todos** los sensores y arma un `Medicion` completo
- [ ] Marca de tiempo en cada medición (RTC por ahora; NTP desde la fase 7)
- [ ] Campo de banderas de validez: qué sensores respondieron bien en ese ciclo
- [ ] Intervalo de muestreo configurable desde `config.h`

**Verificación:** una línea por ciclo en el monitor serie con todas las magnitudes y su hora.

---

### Fase 6 — Persistencia en la memoria del nodo
> Sin componentes nuevos. LittleFS en la flash interna.

Ahora el log cumple otra función: es el **buffer** que guarda las mediciones mientras no hay
red, para mandarlas cuando vuelva (fase 9).

**Adelantado a pedido (2026-11-22)** junto con la fase 9, como prueba de concepto en
`pruebas/registro_y_envio/` (registro circular con contadores `escritas`/`enviadas` y CRC por
medición). Compila; falta probarlo en la placa.

- [ ] Formatear y montar LittleFS
- [ ] Escribir cada `Medicion` al archivo de log en formato binario de tamaño fijo
- [ ] Log **circular**: cuando se llena, el registro más viejo cede lugar al más nuevo
- [ ] Leer el histórico de vuelta y verificar que no se corrompió
- [ ] Calcular y documentar cuántos días de autonomía de almacenamiento da
- [ ] Guardar en la cabecera el **índice de envío**: hasta qué registro confirmó el servidor

**Verificación:** se corta la alimentación, vuelve, y el histórico anterior sigue intacto.

---

### Fase 7 — WiFi y hora por NTP
> Sin componentes nuevos. Requiere saber qué red WiFi se va a usar.

- [ ] **Averiguar con sistemas del ITR** qué tipo de red hay: WPA2 con contraseña común (la
      fácil), WPA2-Enterprise (usuario y contraseña personales) o portal cautivo (página de
      login). Con portal cautivo el ESP32 prácticamente no puede conectarse: hace falta un
      router propio o una red aparte para dispositivos
- [ ] Probar en `pruebas/wifi/` que la placa se conecta e imprime su IP
- [ ] **Reconexión automática**: desenchufar el router y volver a enchufarlo; la placa se
      reconecta sola sin reiniciarse
- [ ] Hora por **NTP**, en hora de Argentina (UTC−3)
- [ ] Sincronizar el DS3231 desde NTP; al arrancar sin red, tomar la hora del DS3231
- [ ] Las credenciales WiFi van en un archivo que **no se sube a GitHub** (`secretos.h`, en el
      `.gitignore`)

**Verificación:** con la red cortada la estación sigue midiendo con la hora correcta, y al
volver la red se reconecta sola.

---

### Fase 8 — Envío a nuestro servidor (Google Sheets)
> Sin componentes nuevos. Contrato en `docs/servidor.md`, servidor en `servidor/`.

**Adelantado a pedido (2026-11-10)** como prueba de concepto en `pruebas/envio_sheets/`, con
el DHT22. Falta integrarlo al firmware una vez hechas las fases 5 y 6.

- [ ] Instalar el servidor de Google Sheets siguiendo `servidor/LEEME.md`, y probarlo con
      `probarGuardado` y con `curl` desde la PC
- [x] Función que arma la línea `[fecha;hora;codigo;...;EOF]`, con `NaN` en los datos
      inválidos (`LineaServidor` en `EstacionComun`) *(escrita y compilada, falta probar en placa)*
- [x] `pruebas/envio_sheets/` manda una medición por minuto y aparece en la planilla
- [ ] Una medición solo se marca como enviada cuando el servidor responde `OK`
- [ ] Integrarlo al firmware, armando la línea a partir del `Medicion` *(escrito y compilado
      en `estacion_campo/`, fase I; falta probar en placa)*

**Verificación:** la planilla muestra una fila por medición, en la pestaña del mes, con el
formato exacto de `docs/servidor.md`.

---

### Fase 9 — Cortes de red sin perder datos
> Sin componentes nuevos.

- [ ] Sin red, las mediciones quedan pendientes en el log
- [ ] Al volver la red, se mandan las pendientes **en orden** antes que las nuevas
- [ ] Si el servidor no responde, reintentar más tarde sin trabar el muestreo
- [ ] Prueba: cortar la red una hora y verificar que llegan todas las mediciones de esa hora,
      sin huecos y sin duplicados

**Verificación:** después del corte, la planilla tiene exactamente las mismas mediciones que
la estación registró.

---

### Fase I — Integración: el firmware final en `estacion_campo/`
> **Adelantada a pedido (2026-11-22).** Junta en un solo firmware lo probado por separado en
> `pruebas/` (fases 5 a 9), el BMP180 de la fase 4 y la pantalla de la fase opcional A.
> Componentes: DHT22, encoder FC-03, OLED SSD1306; BMP180 en camino; DS3231 sin comprar.

**Plan de la sesión del 2026-11-22** (en este orden):

1. Commit de lo pendiente de la sesión anterior
2. Hardware final en la documentación: BMP180 en lugar del BME280 (I²C 0x77), OLED SSD1306
   (0x3C) en la estación, DS3231 preparado pero desactivado
3. **Valor de falla `ERR`** en `EstacionComun`: `Medicion` guarda el estado de cada campo
   (ok / sin sensor / falla), `LineaServidor` escribe `NaN` o `ERR`, el registro en flash
   sube de versión. Armado del JSON para la API del ITR en el mismo lugar
4. `servidor/Codigo.gs` acepta `ERR`, lo guarda como texto en rojo y lo devuelve en el JSON
   distinto de `null`. Expresión regular probada con node
5. Firmware en `estacion_campo/`, un `.h/.cpp` por responsabilidad: `config`, `sensores`
   (DHT22 + BMP180), `anemometro`, `registro` (flash, con un contador de envío por destino),
   `red` (WiFi + NTP), `rtc` (DS3231, desactivado), `envio` (cola y reintentos),
   `envio_sheets`, `envio_itr` (desactivado) y `pantalla`
6. Compilar con `arduino-cli` y `--warnings all`: el firmware y las pruebas que usan
   `EstacionComun`
7. Documentación (`servidor.md`, `formato-datos.md`, `flujo-de-datos.md`, pinout, lista de
   componentes, README, bitácora) y commit final

**Al cierre de la sesión:** los 7 pasos hechos. El firmware y las pruebas compilan sin
advertencias y la expresión regular del servidor pasa `tools/probar_servidor.js`. **Nada de
esto se probó todavía en la placa** (detalle en la bitácora del 2026-11-22).

**Tareas** (casilla marcada = verificado en la placa):

- [ ] Medición alineada al reloj cada `INTERVALO_MEDICION_S` (5 min; 1 min para probar),
      sin depender de la red
- [ ] DHT22 desconectado a propósito → la medición sale con `ERR` y el monitor lo avisa
- [ ] BMP180 con `BMP180_INSTALADO = false` → presión `NaN`; al llegar, activarlo y ver la
      presión en hPa (absoluta, sin corregir a nivel del mar)
- [ ] Pantalla OLED: última medición, WiFi, pendientes y hora
- [ ] Tecla `w`: las mediciones se acumulan en la flash y se mandan solas al reconectar
- [ ] La planilla muestra `ERR` en rojo y `doGet` lo devuelve como `"ERR"`
- [ ] Envío al ITR: activar `ENVIAR_A_ITR` cuando su API acepte `null` y la fecha
- [ ] DS3231: activar `RTC_INSTALADO` cuando se compre y probar un arranque sin red

**Verificación:** con el firmware final la planilla recibe una fila por intervalo, sin huecos
después de un corte de WiFi, con `NaN` en lo que no está instalado y `ERR` en lo que falló.

---

### Fase 10 — Integración con el servidor del ITR
> Requiere el contrato cerrado con el equipo de software.

- [ ] Cerrar con el equipo de software todo lo pendiente de `docs/servidor.md`
- [ ] Envío por **HTTPS** validando el certificado del servidor
- [ ] Revisar el código del servidor del ITR y comparar con nuestro servidor de Google Sheets
- [ ] Prueba de varios días seguidos contra el servidor real

**Verificación:** los datos de varios días aparecen en el servidor del ITR y coinciden con el
log de la estación.

**Con la fase 10 terminada, el proyecto está completo y es presentable.**

---

## FASES OPCIONALES

Solo después de que el núcleo esté terminado y verificado. Ninguna de estas es necesaria para
que el proyecto funcione ni para presentarlo. Están ordenadas por relación entre lo que aportan
y lo que cuestan.

### Fase A (opcional) — Display OLED y navegación por botones
> Requiere: OLED I²C y botones (ya disponibles). **La pantalla se adelantó a pedido en la
> fase I (2026-11-22)**: muestra medición, WiFi, pendientes, último envío y hora. Faltan los
> botones y el menú.

Pantalla en la estación para ver el estado sin PC: medición actual, registros pendientes de
envío, estado del WiFi y del servidor, y un menú de tres botones (arriba / abajo / OK).

Es la más agradecida de las opcionales: mejora mucho la demostración del proyecto y es de
dificultad baja. Si sobra tiempo, hacer esta primero.

- [ ] OLED mostrando la medición actual *(escrito en `estacion_campo/pantalla.cpp`, falta
      probar en placa)*
- [ ] Tres botones con `INPUT_PULLUP` y antirrebote
- [ ] Menú de navegación
- [ ] Indicador de estado del WiFi y del último envío

### Fase B (opcional) — Ahorro de energía y deep-sleep
> Requiere: nada nuevo, pero cambia la estructura del firmware de campo.

Dormir entre mediciones para que la batería dure días en lugar de horas. Con WiFi pesa más:
el WiFi encendido consume bastante, y reconectarse en cada ciclo lleva varios segundos.

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

### Fase E (opcional) — Dirección del viento con sensores Hall
> Requiere: imán de neodimio y los A3144 disponibles.

Retoma la fase 2 con el diseño que se defina. Un solo A3144 detecta si el imán está o no
enfrente, pero no hacia dónde apunta: para dar un rumbo hacen falta varios Hall en círculo (como
en la fase 2) o un sensor Hall lineal.

- [ ] Definir el diseño y anotarlo en la bitácora
- [ ] Reemplazar el `NaN` del campo `direccion` por el rumbo

### Fase F (opcional) — Pluviómetro de balancín
> Requiere: un A3144 (disponible), un imán y el balancín impreso en 3D.

Un recipiente con dos cucharas que se vuelca cada vez que junta un volumen fijo de agua. Un
imán en el balancín pasa frente a un Hall y cada vuelco es un pulso, igual que el anemómetro.
Pulsos × milímetros por vuelco = lluvia caída. Hace falta un campo nuevo en el formato.

- [ ] Imprimir el balancín y calibrar los milímetros por vuelco con una jeringa (volumen
      conocido dividido por el área de la boca)
- [ ] Contar vuelcos con interrupción y antirrebote
- [ ] Acordar el campo con el equipo de software

---

## Cómo se usa este archivo

Al empezar cada sesión de trabajo, mirar cuál es la primera fase sin terminar: esa es la que se
trabaja. No adelantar trabajo de fases posteriores ni implementar fases opcionales antes de
cerrar el núcleo.
