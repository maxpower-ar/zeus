# Estación — firmware final (placa Delta)

Mide cada 5 minutos (alineado al reloj: 10:00, 10:05, 10:10…), **guarda cada medición en la
memoria flash** y después la manda por WiFi. Si no hay red, las mediciones quedan pendientes
y se mandan solas cuando vuelve, primero las más viejas. Medir y guardar nunca depende de la
red.

Cada dato viaja como un número, **`NaN`** (el sensor no está instalado) o **`ERR`** (el
sensor está instalado pero falló: hay que ir a revisarlo). Ver
[`docs/servidor.md`](../docs/servidor.md).

**Estado (2026-11-22):** compila sin advertencias. **No se probó todavía en la placa.**

## Archivos

| Archivo | Responsabilidad |
|---|---|
| `estacion_campo.ino` | Arranque, bucle principal, cuándo medir, teclas del monitor serie |
| `config.h` | Qué sensores están instalados, a dónde se manda, pines, intervalos, calibraciones |
| `secretos.h` | WiFi y claves. **No se sube** (`.gitignore`). Plantilla: `secretos_ejemplo.h` |
| `sensores.h/.cpp` | DHT22 (temperatura y humedad) y BMP180 (presión), con `NaN`/`ERR` |
| `anemometro.h/.cpp` | Encoder óptico: velocidad promedio del viento desde la medición anterior |
| `registro.h/.cpp` | Memoria flash (LittleFS): registro circular con un contador de envío por servidor |
| `red.h/.cpp` | WiFi (reconexión, tecla `w`) y hora por NTP |
| `rtc.h/.cpp` | DS3231, respaldo de la hora. **Desactivado** (no comprado) |
| `destinos.h` | Los dos servidores: planilla e ITR |
| `envio.h/.cpp` | Cola de envío: qué mandar, cuándo, reintentos |
| `envio_sheets.h/.cpp` | Envío a nuestra planilla de Google (`servidor/Codigo.gs`) |
| `envio_itr.h/.cpp` | Envío a la API del ITR. **Desactivado** hasta que acepte `null` y la fecha |
| `pantalla.h/.cpp` | OLED: hora, WiFi, última medición, pendientes, último envío |

El struct `Medicion`, la línea al servidor y el JSON del ITR **no van acá**: viven en
`libraries/EstacionComun/`.

## Antes de compilar

1. **`secretos.h`:** copiar el de `pruebas/registro_y_envio/` y agregarle al final la línea
   `CLAVE_API_ITR` de `secretos_ejemplo.h` (o copiar la plantilla y completarla). Ya hay uno
   preparado en esta carpeta en la máquina del proyecto
2. **Librerías** (Herramientas → Administrar bibliotecas): *DHT sensor library*, *Adafruit
   Unified Sensor*, *Adafruit BMP085 Library* (sirve para el BMP180), *Adafruit SSD1306* y
   *Adafruit GFX Library* (el IDE ofrece *Adafruit BusIO* sola)
3. **Placa:** ESP32 Dev Module. **Partition Scheme:** el que viene por defecto
   (*Default 4MB with spiffs*): LittleFS usa esa partición. El firmware ocupa el 87 % de la
   parte de programa
4. Monitor serie a **115200**

## Qué se configura en `config.h`

| Constante | Valor | Para qué |
|---|---|---|
| `BMP180_INSTALADO` | `false` | **Ponerlo en `true` cuando llegue el BMP180** y recompilar |
| `RTC_INSTALADO` | `false` | En `true` cuando se compre el DS3231 |
| `ENVIAR_A_ITR` | `false` | En `true` cuando la API del ITR acepte `null` y la fecha |
| `INTERVALO_MEDICION_S` | `5 * 60` | Para probar: `1 * 60`. Nunca menos de 60 (no compila) |
| `METROS_POR_PULSO` | `0.0471` | **TODO: calibrar** (anemómetro) |

Un sensor con `..._INSTALADO = false` sale siempre como `NaN`. Con `true`, si no responde,
sale como `ERR` y el monitor serie dice qué revisar.

## Teclas del monitor serie

| Tecla | Hace |
|---|---|
| `e` | Estado: hora, WiFi, mediciones guardadas, pendientes por servidor, sensores instalados |
| `m` | Lee los sensores **ahora** y los muestra. **No se guarda ni se manda** |
| `w` | Corta / reconecta el WiFi, para probar un corte sin tocar el router |
| `B` | Borra todo el registro (mayúscula, para no hacerlo sin querer) |

## Limitaciones conocidas

- **Sin hora no se mide.** Si la placa arranca sin internet y sin DS3231, espera a tener
  hora. Lo resuelve el DS3231
- **El anemómetro nunca sale como `ERR`:** "rueda quieta" y "sensor desconectado" se ven
  iguales desde el código. Con la tecla `m` se ve el nivel del pin
- **HTTPS sin validar el certificado** (`setInsecure()`): se corrige en la fase 10
- **Un envío bloquea el bucle** hasta ~30 s si el servidor no contesta. No se pierde nada:
  los pulsos del anemómetro se cuentan por interrupción y la medición se hace apenas termina
