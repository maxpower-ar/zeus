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
| MAC de la estación (Delta) | *(pendiente, fase 0)* |
| MAC del nodo interfaz (Beta, retirado) | — |
| Versión del Arduino IDE | *(pendiente)* |
| Versión del core ESP32 | *(pendiente)* |
| Puerto COM de la estación | *(pendiente)* |
| Código de la estación | `EM_ITR_001` |
| Red WiFi | *(pendiente: tipo de red a averiguar con sistemas del ITR)* |
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

## 2026-11-24 — Fase I — MQ-7, sensores en ERR, envío al ITR activado y panel web

**Qué se hizo (sin hardware, solo código):**

- **MQ-7 (CO)** en el firmware: GPIO39 (VN, ADC1) por un divisor 10 kΩ / 20 kΩ. De la
  tensión se despeja Rs y se pasa a ppm con la curva del datasheet. Va en el campo `gas` de
  la planilla y en `co_concentracion` del ITR. La pantalla lo muestra junto a la presión.
- **BMP180 y MQ-7 marcados como instalados** aunque todavía no estén conectados: así la
  planilla muestra ERR en rojo hasta que se conecten (pedido del usuario).
- **Envío al ITR activado.** El formato que pasó el equipo de software
  (`temperatura`, `humedad`, `co_concentracion`, `velocidad_viento`, `presion`) ya coincidía
  con el JSON del firmware: no hizo falta cambiarlo. Si `CLAVE_API_ITR` es la de ejemplo,
  no se intenta (las mediciones quedan pendientes para el ITR).
- **Panel web** en `dashboard/`, publicado en GitHub Pages (rama `gh-pages`): lee el `doGet`
  de la planilla; muestra lo actual, gráficos, tabla y CSV, de 24 h a todo el historial.
- **Guía de conexiones** cable por cable: `hardware/conexiones.md`. Calibración del MQ-7:
  `hardware/calibraciones/mq7.md`.

**Decisiones:**

1. **GPIO39 para el MQ-7:** ya estaba reservado para el gas en `pinout.md`; es ADC1 (el ADC2
   no anda con WiFi). El defecto de GPIO36/39 (pulsos falsos) solo afecta a contar pulsos,
   no a leer una tensión.
2. **Falla del MQ-7 = menos de 50 mV en el pin.** Sin módulo, la resistencia de abajo del
   divisor lleva el pin a 0 V. Arriba de 3100 mV también es ERR (ADC saturado). Riesgo: si
   en aire muy limpio la salida real es menor a 50 mV, marcaría ERR falso; se revisa con el
   multímetro en la primera prueba.
3. **Constantes del MQ-7 provisionales** (`TODO: calibrar`): RL 1 kΩ, R0 1 kΩ, VCC 5 V,
   divisor nominal. La curva (99.042, −1.518) es del datasheet, vía la librería
   MQUnifiedsensor. Los ppm no son confiables hasta calibrar.
4. **No hay precalentamiento en el código:** las primeras mediciones después de encender
   salen altas. Se avisa al arrancar.
5. **El anemómetro sigue sin poder dar ERR:** desconectado se ve igual que sin viento.
6. **ITR y ERR:** su API pide números, así que una falla va como `null` y la rechazan si el
   campo es obligatorio para ellos. Se saltea (queda en la planilla). No se inventan números
   para que la acepten.
7. **La URL del script queda pública** en `dashboard/config.js`: el panel la necesita para
   leer. Solo permite leer; escribir pide `CLAVE_API`.
8. **gh-pages con solo la carpeta `dashboard/`** (git subtree), en vez de publicar `docs/`
   entero.

**Verificación:**

- Firmware: compilación limpia con `--warnings all`, **0 advertencias**, 88 % de la flash.
- Panel: probado en Chrome sin ventana contra la planilla real (134 mediciones de
  noviembre): tarjetas, gráficos de "Todo", tabla y modo claro/oscuro. A 375 px de ancho no
  se desborda.
- La planilla todavía responde con la **versión vieja** de `Codigo.gs` (el JSON no trae
  `presion` ni `luminosidad`): falta publicar la nueva. Con la vieja, las líneas con `ERR`
  se rechazan y **la estación no puede vaciar su cola**.
- **Nada probado en la placa.** El MQ-7 no se conectó nunca.

**Pendiente:**

- [ ] Publicar `Codigo.gs` (Implementar → Administrar implementaciones → Nueva versión)
- [ ] Pegar la clave real del ITR en `CLAVE_API_ITR`
- [ ] Probar el MQ-7 en la placa y calibrarlo
- [ ] Ver que el ITR acepte las mediciones con todos los sensores conectados

---

## 2026-11-22 — Fase I (integración) — Firmware final y valor de falla `ERR`

**Qué se hizo:** el firmware final en `estacion_campo/`, armado a partir de
`pruebas/registro_y_envio/`, con un `.h/.cpp` por responsabilidad (ver su `LEEME.md`). Mide
cada 5 minutos, guarda en la flash, manda a la planilla y muestra el estado en la pantalla
OLED. El código viejo de la fase 1 (LM35) se borró de esa carpeta.

**Hardware final:** DHT22 (GPIO4) y encoder FC-03 (GPIO18) instalados; **BMP180 en camino**
(reemplaza al BME280; I²C 0x77, `BMP180_INSTALADO = false` hasta que llegue); OLED SSD1306 en
0x3C; DS3231 preparado y desactivado. Gas, dirección y luminosidad sin sensor.

**Cambio de formato — valor de falla `ERR`:**

- `NaN` = sensor **no instalado** (esperable). `ERR` = sensor **instalado que falló** (no
  responde, CRC del DHT22, BMP180 no detectado, lectura fuera de rango): hay que revisarlo
- `Medicion` guarda un **estado por campo** (`DATO_OK`, `DATO_SIN_SENSOR`, `DATO_FALLA`).
  Pasó a 40 bytes, `VERSION_MEDICION = 2`, con un `static_assert` del tamaño para que nadie
  cambie el struct sin subir la versión. El CRC por registro se mantiene
- `LineaServidor` escribe `ERR`; `Codigo.gs` lo acepta, lo guarda como texto **en rojo** y lo
  devuelve en `doGet` como `"ERR"` (distinto de `null`)
- **Hay que avisar al equipo de software** (mensaje listo al final de esta entrada)

**Decisiones tomadas (sin consultar, a pedido):**

1. **Mediciones alineadas al reloj** (10:00, 10:05…), no "cada 5 minutos desde que arrancó".
   La primera se hace en el próximo turno, no al arrancar: así un reinicio no repite el
   minuto de la medición anterior (el servidor la tomaría como duplicada). Para ver los
   sensores sin esperar se agregó la tecla **`m`** (lee y muestra, no guarda)
2. **`INTERVALO_MEDICION_S`** en segundos; un `static_assert` impide compilar con menos de 60
3. **Rangos de plausibilidad** del fabricante (DHT22 −40…80 °C y 0…100 %; BMP180 300…1100
   hPa): un valor afuera es `ERR`, nunca se recorta
4. **El BMP180 se busca en el bus antes de cada lectura**: la librería no avisa si se
   desconectó y devolvería un número cualquiera. Si vuelve, se reinicia solo
5. **El anemómetro nunca da `ERR`**: "quieto" y "desconectado" se ven iguales. La tecla `m`
   muestra el nivel del pin para revisarlo a mano
6. **Presión absoluta** de la estación, en hPa, sin corregir a nivel del mar (no hay altitud
   medida). La temperatura del BMP180 se muestra en el monitor serie para contrastar con el
   DHT22, pero no se manda (el formato no tiene ese campo)
7. **Registro con un contador de envío por servidor** (planilla e ITR): si uno no responde,
   no traba al otro. Un servidor desactivado no acumula pendientes: al activarlo recibe desde
   la medición siguiente. Firma del archivo `EMES` (la prueba usa `EMRG`): si se sube una y
   después la otra, cada una reconoce que el archivo no es suyo y empieza de cero
8. **Envío al ITR preparado y desactivado** (`ENVIAR_A_ITR = false`). JSON armado en
   `EstacionComun/JsonItr.cpp`: sin sensor y falla van como `null` y los campos con falla se
   listan en `"fallas"`; `fecha` en UTC. Una medición por pedido. Si responden 400/422 la
   medición se saltea (reintentarla no sirve y trabaría la cola); queda en la planilla. La
   URL va en `config.h` (no es secreta); la clave en `secretos.h` (`CLAVE_API_ITR`)
9. **DS3231 sin librería extra:** 7 registros por I²C. Guarda la hora en UTC; si el chip
   marca que perdió la hora (bandera OSF), no se usa. **No probado**: no hay DS3231
10. **Pantalla:** `--` para "sin sensor" (más legible que NaN) y `ERR` para falla. Se
    redibuja cada segundo; bus I²C a 400 kHz (~25 ms por dibujo). Si con cables largos hay
    fallas de I²C, bajar `FRECUENCIA_I2C_HZ` a 100000
11. **Expresión regular del servidor más estricta:** cada dato tiene que ser número, `NaN` o
    `ERR` (antes aceptaba cualquier texto y lo filtraba después). Rechaza `err` en minúscula
    y datos vacíos
12. **Se guardó `tools/probar_servidor.js`**, que carga el `Codigo.gs` real y lo prueba con
    node, para no reescribir la prueba cada vez

**Qué se verificó:**

- `estacion_campo` compila **sin advertencias** con `--warnings all` (compilación limpia,
  core ESP32 3.3.10): usa el 87 % de la flash de programa y el 15 % de la RAM. También
  compila sin advertencias una copia con BMP180, DS3231 e ITR activados e intervalo de 1 min
- `pruebas/envio_sheets` y `pruebas/registro_y_envio` (adaptadas al estado por campo)
  compilan sin advertencias
- `node tools/probar_servidor.js`: 13 pruebas bien (líneas con `ERR` aceptadas; hora mal,
  texto, `err`, dato vacío, formato viejo y falta de `EOF` rechazados; `doGet` devuelve
  `null` y `"ERR"`)
- Librerías instaladas con el arduino-cli del IDE: *Adafruit BMP085 Library* 1.2.4,
  *Adafruit SSD1306* 2.5.17, *Adafruit GFX Library* 1.13.0

**Revisión del código (después del primer commit, a pedido):** se encontró y corrigió:

- Cada registro en la flash ocupa **44 bytes, no 41**: el compilador agrega 3 de relleno para
  alinear. Las cuentas de espacio decían ~820 KB y son ~880 KB (entra igual; los días de
  autonomía no cambian). Se agregó un `static_assert` que lo fija
- Si había pendientes pero la memoria no se podía leer, la cola reintentaba en cada vuelta
  del loop sin pausa. Ahora cuenta como envío fallido y espera el reintento
- Un envío **rechazado** por el ITR se informaba como "CONFIRMADAS". Ahora el mensaje dice
  qué pasó

Quedó anotado y sin cambiar: si NTP corrige el reloj **hacia atrás** cruzando un turno, se
puede medir dos veces el mismo minuto; la planilla descarta la segunda como duplicada (en la
flash quedan las dos). Es raro y no pierde datos.

**Qué NO se verificó:** nada se probó en la placa ni en Google. El armado de la línea y del
JSON solo se compiló (no hay compilador de C++ para la PC). `Codigo.gs` nuevo **no está
publicado**: hasta publicarlo, una línea con `ERR` vuelve como `formato invalido` y la cola de
la planilla se queda esperando (no se pierde nada; el monitor serie avisa).

**Pendiente:**

1. Publicar `Codigo.gs` (Implementar → Gestionar implementaciones → editar → Nueva versión)
   y ejecutar `probarFalla`
2. Probar el firmware en la placa (pasos en la respuesta de la sesión y en
   `estacion_campo/LEEME.md`)
3. Mandarle al equipo de software el aviso de `ERR`
4. Al llegar el BMP180: `BMP180_INSTALADO = true`, escanear el bus, comparar presión
5. Calibrar `METROS_POR_PULSO`

**Mensaje para el equipo de software:**

> Hola. Cambiamos cómo marcamos un dato que falta, para distinguir dos casos:
> - `NaN` = ese sensor no está instalado (por ejemplo, el gas). Es normal.
> - `ERR` = el sensor está instalado pero falló la lectura. Hay que ir a revisarlo.
>
> En la línea: `[22/11/2026;10:05;EM_ITR_001;ERR;ERR;3,2;NaN;NaN;NaN;NaN;EOF]`. En nuestra
> planilla `ERR` queda en rojo y la consulta JSON lo devuelve como el texto `"ERR"` (y
> `NaN` como `null`): antes de graficar, fíjense que el valor sea número.
>
> Para su API (JSON) mandaríamos `null` en los dos casos, más una lista con los campos que
> fallaron: `"fallas": ["temperatura","humedad"]`, y la hora de la medición en
> `"fecha": "2026-11-22T13:05:00Z"` (UTC). Para activar el envío necesitamos que acepten
> `null` en `co_concentracion` y `velocidad_viento` (no hay sensor de gas) y que guarden
> `fecha` y `codigo`. ¿Les sirve así? Y si no, ¿qué responde su API si una lectura no les
> gusta: 400? Nunca vamos a usar -1 ni 0 como marca: pueden ser datos reales.

---

## 2026-11-22 — Fases 6 y 9 (adelantadas) — Guardar primero, mandar después

**Qué se escribió:** `pruebas/registro_y_envio/`. Cada minuto mide DHT22 + anemómetro,
**guarda en la flash (LittleFS)** y después manda las pendientes a la planilla, en tandas de
hasta 20, de la más vieja a la más nueva. Una medición se da por enviada solo si el servidor
confirma todas las líneas de la tanda (`guardadas + duplicadas = mandadas`).

- Registro circular de 20 000 mediciones (~14 días sin red a 1 por minuto, ~69 a 1 cada 5).
  Si se llena, la más vieja cede su lugar y se cuenta como perdida
- Cada medición guardada lleva un CRC: si la flash se corrompe, se descarta en vez de mandarse
- `Medicion` pasó a `libraries/EstacionComun/src/Medicion.h`, y `armarLinea` ahora recibe un
  `Medicion` (se adaptó `envio_sheets`)
- Velocidad del viento: **promedio desde la medición anterior**, con `METROS_POR_PULSO`
  todavía sin calibrar (`TODO: calibrar`)
- Sensores que faltan (gas, dirección, presión, luminosidad): `NaN`, no `-1`. En la planilla
  quedan como celda vacía; un `-1` se confundiría con un dato real (−1 °C existe)
- Teclas para probar: `e` estado, `w` cortar/reconectar el WiFi, `B` borrar el registro

**Qué se verificó:** los dos sketches compilan sin advertencias (core ESP32 3.3.10). **No se
probó en la placa.**

**Limitación conocida:** si la placa arranca sin internet no tiene hora y no registra nada
hasta conectarse una vez. Lo resuelve el DS3231.

---

## 2026-11-22 — Fase 10 (adelantada) — Primer envío aceptado por la API del ITR

**Qué cambió en su API desde el 17/11:** ahora los campos **obligatorios** (numéricos, sin
`null`) son `temperatura`, `humedad`, `velocidad_viento` y `co_concentracion` (ppm).
`presion` y `luminosidad` ya **no se validan**: se pueden mandar en `null`. La página dice
"ESP32 · API Flask · React · **SQLite**": parece que dejaron Firebase.

**Qué se hizo:** un envío de prueba desde la PC con `curl`:

```
{"temperatura":22.5,"humedad":48,"velocidad_viento":0.0,"co_concentracion":0,
 "presion":null,"luminosidad":null,"codigo":"EM_PRUEBA","prueba":true}
→ HTTP 201 CREATED  {"id":"hAtJzXuvSO7pkqjVuxhk","ok":true}
```

La lectura apareció en su página (captura:
`docs/capturas/2026-11-22-lectura-en-pagina-itr.png`). **Envío y recepción funcionan.**

**Ojo:** `velocidad_viento` y `co_concentracion` se mandaron en 0 porque son obligatorios. En
su página se ven como valores reales: hay que pedirles que borren la lectura
`hAtJzXuvSO7pkqjVuxhk`. Sigue faltando que acepten `null` en esos dos campos (no hay sensor de
gas todavía) y que reciban la fecha de la medición y el código de estación.

**Hardware:** no se consiguió el BME280; la presión queda sin sensor por ahora.

---

## 2026-11-17 — Fase 10 (adelantada) — Primera prueba contra la API del ITR

**Qué se hizo:** el equipo de software pasó la URL y la clave de su API. Se probó desde la PC
con `curl`. Captura: `docs/capturas/2026-11-17-prueba-api-itr.png`. La clave **no** se
anota acá: va en `secretos.h`.

**Qué se encontró:**

- URL: `https://estacionmeteorologica.pythonanywhere.com/api/lecturas`. Flask en
  PythonAnywhere; los datos se guardan en Firebase (Firestore, colección `lecturas`) y la
  página los lee de ahí
- **Hay conexión y la clave es válida.** Va en el encabezado `X-API-Key` (en
  `Authorization: Bearer` o en la URL da 401)
- **Solo POST.** `GET /api/lecturas` da 404
- **Pide JSON**, no la línea acordada: con `[...;EOF]` responde `El cuerpo debe ser un JSON`
- Campos **obligatorios** y numéricos: `temperatura`, `humedad`, `presion`, `luminosidad`. Si
  falta alguno, o va `null`, rechaza la lectura entera
- No acepta números como texto con coma (`"22,0"`)

**Qué NO se hizo:** no se guardó ninguna lectura. Con nuestros sensores actuales no hay
presión ni luminosidad, y mandar valores inventados los mostraría como reales en su página.

**Diferencias con lo acordado (para hablar con el equipo de software):**

1. Formato: línea `[...;EOF]` acordada contra JSON implementado
2. Faltan en su API: código de estación, fecha y hora de la medición, velocidad del viento,
   gas y dirección
3. Sobra `luminosidad`: no hay sensor de luz en el proyecto
4. Sensor sin dato: necesitamos poder mandar `null` (nuestro `NaN`) sin que rechace todo
5. Sin fecha en el envío, las mediciones guardadas durante un corte de red llegarían con la
   hora de llegada, no la de medición

**Cambio en nuestro formato (mismo día):** se agregaron **presión** (hPa) y **luminosidad**
(lux) al final de la línea, antes de `EOF`. Actualizados `LineaServidor`, `envio_sheets` y
`Codigo.gs`; el sketch recompila sin advertencias. Las pestañas viejas de la planilla se migran
solas. Falta: publicar la nueva versión del script y volver a subir el sketch a la placa.

**Pendiente:** acordar el formato con el equipo de software antes de escribir el envío al ITR.

---

## 2026-11-10 — Fase 8 (adelantada) — Servidor propio en Google Sheets

**Qué se decidió:** nuestro servidor es **Google Sheets + Google Apps Script**. Es gratuito, no
pide tarjeta de crédito, guarda los datos en columnas como una planilla y sirve también para
la página web (`doGet` devuelve JSON). Se descartaron:

- **Cloud Storage:** guarda archivos, no filas
- **Cloud Run / Cloud Functions:** piden tarjeta de crédito
- **BigQuery sandbox:** borra las tablas a los 60 días
- **Firestore:** cobra por cada dato leído (unas 50 000 lecturas por día en el plan gratuito)
  y no se ve como planilla

**Diseño:** una pestaña por mes (`AAAA-MM`), elegida por la fecha de la medición y no por la
de llegada. Duplicados descartados por la columna `Id`. Clave propia en las propiedades del
script y en `secretos.h`.

**Qué se escribió (a pedido, antes de su fase):**

- `servidor/Codigo.gs` y `servidor/LEEME.md`: el servidor y cómo instalarlo
- `libraries/EstacionComun/src/LineaServidor.h/.cpp`: arma la línea. Se agregó
  `library.properties` para que el IDE reconozca la librería
- `pruebas/envio_sheets/`: manda el DHT22 una vez por minuto

**Qué se verificó:** `envio_sheets` **compila sin advertencias** con arduino-cli (core ESP32
3.3.10). La expresión regular del servidor se probó en la PC: acepta la línea del ejemplo y
rechaza la hora con `;` y los valores no numéricos. **Nada de esto se probó todavía en la
placa ni en Google.**

**Detalle técnico:** Apps Script responde al POST con una redirección 302; la respuesta se lee
haciendo GET a la dirección nueva. El sketch sigue la redirección a mano, para que el POST no
se repita.

**Servidor instalado y verificado** (cuenta `emitr.zeus@gmail.com`): `probarGuardado` dio
GUARDADA y DUPLICADA. Desde la PC con `curl`: primer envío `OK guardadas=1 duplicadas=0`,
segundo `OK guardadas=0 duplicadas=1`, con clave equivocada `ERROR clave incorrecta`, y la
consulta GET devolvió el JSON con las filas de prueba (gas y dirección como `null`).

**Probado en la placa:** `envio_sheets` con el DHT22 conectado se conecta al WiFi, toma la
hora por NTP y las mediciones llegan a la planilla. Primero no compilaba
(`LineaServidor.h: No such file or directory`): la ubicación del sketchbook del IDE apuntaba a
`Documents\Arduino` y no al repositorio. Se corrigió y se reinstalaron ahí las librerías del
DHT22 (con Adafruit Unified Sensor).

**Pendiente:**

- HTTPS sin validar el certificado (`setInsecure()`): aceptable en banco, se corrige en la
  fase 10

---

## 2026-10-27 — Cambio de arquitectura — Un solo ESP32 que manda a un servidor

**Qué cambió:** se retira el ESP32 **Beta** (nodo puente USB que recibía los datos de Delta por
ESP-NOW). Queda solo **Delta**, que ahora manda cada medición por **WiFi** a una API del ITR.
El servidor lo hace el equipo de software de la escuela.

**Sensores del núcleo:** velocidad del viento (encoder, ya funciona en banco), temperatura y
humedad (DHT22, falta terminar), gas (después de los dos anteriores) y presión (BME280,
pedido). La dirección del viento se posterga y se rediseña con Hall. Idea a futuro: un
pluviómetro de balancín con uno de los A3144.

**Formato acordado de la línea:**

```
[26/10/2026;09:56;EM_ITR_001;26,1;24;12,4;NaN;NaN;EOF]
 fecha      hora  código     temp hum vel  gas dirección
```

- El "nombre" y el "código EM" son el mismo campo (EM = Estación Meteorológica)
- Hora con `:` (`09:56`), no con `;`
- Gas antes que dirección del viento
- Dato inválido o sensor sin instalar = `NaN`
- Separador decimal: **pendiente**

**Decisiones tomadas:**

- La hora sale de **NTP**, con el **DS3231 como respaldo** por si la placa se reinicia sin red
- El log en LittleFS se conserva como **buffer**: sin red la estación sigue midiendo y al
  volver la conexión manda lo pendiente, en orden
- Se arma un **servidor de prueba propio** en `tools/` para no depender del servidor real
  durante el desarrollo. Al final se revisa y compara con el código del equipo de software
- Las contraseñas van en `secretos.h`, fuera del repositorio
- Plan reescrito: fases 7 (WiFi + NTP), 8 (envío al servidor de prueba), 9 (cortes de red) y
  10 (integración con el ITR). Ver `docs/servidor.md`

**Corrección a la URL dictada:** `www.itr.edu.ar/estacion001:9090` no es una URL válida, porque
el puerto va pegado al dominio. Lo probable es `https://itr.edu.ar:9090/estacion001`. Hay que
confirmarlo con el profesor o el equipo de software.

**Preguntas para el equipo de software:**

1. URL exacta: dominio, puerto y ruta
2. Método (POST o GET) y cuerpo: ¿la línea sola, o con el prefijo `DATOS:`? ¿`text/plain` o JSON?
3. Qué código HTTP devuelven si llegó bien y cuál si el formato es inválido
4. Si llega dos veces la misma medición (pasa después de un corte), ¿la descartan?
5. ¿Se pueden mandar varias líneas juntas en un solo pedido?
6. ¿Hace falta una clave o token?
7. Zona horaria: ¿hora de Argentina (UTC−3) o UTC?
8. Unidades de velocidad (km/h o m/s) y de gas, y separador decimal
9. Agregar un campo para la presión del BME280
10. Cada cuánto esperan una medición
11. ¿Hay un servidor de prueba, o se prueba contra el real?

**Preguntas para sistemas del ITR:**

1. Tipo de red WiFi: contraseña común (WPA2-Personal), usuario y contraseña personales
   (WPA2-Enterprise) o página de login (portal cautivo)
2. Que tenga una red de **2.4 GHz** (el ESP32 no se conecta a 5 GHz)
3. Si dejan salir conexiones al puerto 9090, o si el firewall lo bloquea
4. Si hace falta registrar la MAC de la placa

**Pendiente para la próxima:** terminar el DHT22 (fase 1: bandera de validez) y averiguar el
tipo de red.

---

## 2026-10-14 — Fase 3 — Ranuras del disco: resuelto, son 30

**Qué se resolvió:** la contradicción anotada más abajo entre 30 y 20 ranuras. El modelo
OpenSCAD del disco vigente tiene `enc_ranuras = 30`. El `enc_ranuras = 20` correspondía al
disco viejo de 36 mm, que quedó obsoleto: con 36 mm de diámetro no entran 30 ranuras — el
diente quedaba en medio milímetro, más angosto que la ranura óptica del FC-03.

El `.scad` del disco se copió a `hardware/modelos/encoder_disco.scad` y queda como fuente
de verdad de ese número: antes la geometría vivía solo en la carpeta de impresión, fuera
del repositorio, y el firmware no tenía contra qué verificarse.

**Geometría del disco:** 69 mm de diámetro, 2 mm de espesor, 30 ranuras trapezoidales entre
los radios 23.5 y 33.5 mm, de 2.4 mm de ancho adentro a 3.5 mm afuera.

**Qué se verificó por cálculo:** el ciclo de trabajo de las ranuras da **48.8 % a 49.9 % en
toda la banda radial**, contra un ideal de `πr/30`. Los anchos del trapecio siguen el
crecimiento del perímetro con menos de 2.5 % de desvío en el punto peor. Contra la ranura
óptica del FC-03, de alrededor de 1 mm, las transiciones salen limpias.

**Dos observaciones para el armado:**

- Los 10 mm de largo radial de las ranuras dan mucha tolerancia para ubicar el sensor.
  Conviene apuntar el haz a **r ≈ 28.5 mm**, el centro de la banda, y no cerca del borde:
  pasado r = 33.5 solo queda 1 mm de disco macizo.
- Ese borde de 1 mm es el anillo que mantiene unidos los 30 dientes. En PLA de 2 mm a 69 mm
  de diámetro es delicado y propenso a alabearse. Control con regla de canto sobre el disco
  recién impreso; si se comba, subir `enc_margen` a 1.5 o 2 mm no cuesta nada porque sobra
  banda radial.

**Qué no cambia:** el firmware. `RANURAS_POR_VUELTA = 30` ya estaba bien; se reemplazaron
las advertencias de contradicción por la geometría confirmada.

**Pendiente:** medir el radio del rotor con regla, y calibrar `metrosPorPulso`.

---

## 2026-10-14 — Fase 3 — Anemómetro: velocidad en m/s y km/h

**Qué se hizo:** se escribió `pruebas/anemometro/`, que imprime velocidad del viento en m/s y
km/h además de vueltas por minuto, con modo de calibración incorporado (tecla `c <km/h>`). Se
pasaron `pruebas/encoder_optico` y `pruebas/encoder_rueda` a 30 ranuras. Se creó la ficha
`hardware/calibraciones/anemometro.md` con las tablas vacías a completar.

**Decisión de diseño — una sola constante de calibración.** El radio del rotor y el factor del
anemómetro (las cazoletas giran 2.5 a 3 veces más lento que el viento) no se miden por separado:
no hace falta y no se puede con precisión. Lo que se mide de una vez es su producto,
`metrosPorPulso`, con el que `velocidad [m/s] = metrosPorPulso × pulsos_por_segundo`. Un solo
número, un solo experimento.

La conversión m/s → km/h es ×3.6 exacto y no es una calibración. Se imprimen las dos unidades
por comodidad: m/s para las pruebas, km/h para leerlo de un vistazo.

**Contradicción detectada, sin resolver:** el disco en uso es de **30 ranuras**, pero el modelo
OpenSCAD del rotor impreso (`markii/estacion_anemometro.scad`) tiene `enc_ranuras = 20`. Si el
disco real fuera de 20 y el código dijera 30, todas las velocidades saldrían un tercio más bajas
y las lecturas igual parecerían razonables. **Hay que contarlas a mano antes de calibrar.**

**Estado del rotor:** 3 cazoletas sobre rulemanes 10×22×6 (no se usó el cooler Jamicon).

**Pendiente para la próxima:**

1. Contar las ranuras del disco (sketch `encoder_optico`, tecla `r`, una vuelta lenta)
2. Medir el radio del rotor con regla, del centro del eje al centro de una cazoleta
3. Calibrar `metrosPorPulso` con el método del auto, ida y vuelta a 20/30/40/50 km/h
4. Medir la velocidad de arranque del rotor

---

## 2026-09-06 — Fase 1 — DHT22 verificado en banco

**Qué se hizo:** se conectó el DHT22 a GPIO4 (VCC a 3.3 V, DATA a GPIO4, GND común) y se
probó con el sketch `pruebas/dht22/`. Librerías: "DHT sensor library" de Adafruit más
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
desde VIN, y se escribió un sketch de prueba en `pruebas/lm35/` que informa promedio, mínimo
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
