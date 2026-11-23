# Flujo de datos

Cómo viaja una medición desde el aire hasta la pantalla. Pensado también para la presentación.

```mermaid
flowchart LR
    A["🌡️ Sensores<br/>DHT22 · encoder · BMP180"]
    B["🧠 ESP32 Delta<br/>lee · valida (número / NaN / ERR)"]
    P["📟 Pantalla OLED<br/>hora · WiFi · última medición"]
    C["💾 Memoria flash<br/>(LittleFS)"]
    D["📶 WiFi + HTTPS"]
    E["⚙️ Apps Script<br/>clave · formato · duplicados"]
    F["📊 Google Sheets<br/>una pestaña por mes"]
    G["🔥 Firebase<br/>Firestore"]
    H["🖥️ Página web"]

    A -->|señal eléctrica| B
    B --> P
    B -->|guarda primero| C
    C -->|pendientes, en orden| D
    D --> E
    E --> F
    E --> G
    G --> H
    E -.->|"OK" / "ERROR"| B
```

> GitHub dibuja este diagrama solo. Para tenerlo como imagen: pegar el bloque en
> [mermaid.live](https://mermaid.live) y exportar PNG o SVG.

---

## Paso a paso

| # | Etapa | Qué pasa | Por qué |
|---|---|---|---|
| 1 | **Sensor** | Convierte una magnitud física en señal eléctrica: el DHT22 manda pulsos digitales, el encoder corta un haz infrarrojo 30 veces por vuelta, el BMP180 responde por I²C | El ESP32 solo entiende voltajes |
| 2 | **ESP32: medir** | Cada 5 minutos, a hora redonda, lee cada sensor y lo pasa a unidades físicas (°C, %, km/h, hPa). Un sensor que no está instalado da `NaN`; uno instalado que no responde da **`ERR`**. Nunca 0 | Un 0 es un dato real (0 °C, viento en calma); "no hay sensor" y "se rompió el sensor" son otra cosa, y distintas entre sí |
| 3 | **ESP32: hora** | Pone fecha y hora de la medición, sacada de internet (NTP) o del reloj DS3231 si no hay red | La medición tiene que llevar la hora en que se midió, no la hora en que llegó |
| 4 | **Memoria flash** | Guarda la medición **antes** de intentar mandarla | Si se corta el WiFi, la luz o el servidor, no se pierde nada |
| 5 | **Envío** | Arma la línea `[fecha;hora;código;...;EOF]` y la manda por WiFi, cifrada (HTTPS), con una clave | El formato es el contrato con el servidor; la clave impide que un extraño cargue datos |
| 6 | **Apps Script** | Verifica la clave, valida el formato, descarta duplicados y responde `OK` o `ERROR` | Un reenvío después de un corte no debe guardarse dos veces |
| 7 | **Confirmación** | Solo con `OK` el ESP32 marca la medición como enviada. Si no, la reintenta más tarde | Lo que no se confirmó, sigue pendiente |
| 8 | **Almacenamiento** | La medición se guarda en Google Sheets (planilla, respaldo legible) y en Firebase (base de datos de la página) | Dos copias, cada una con su uso |
| 9 | **Página web** | Lee Firebase y muestra gráficos, tablas y alertas | Lo que ve el usuario |
| — | **Pantalla OLED** | En la estación misma: hora, WiFi, última medición, pendientes y último envío | Ver el estado sin PC |

## Estado (2026-11-22)

| Etapa | Estado |
|---|---|
| 1–3, 5–6, 8 (Sheets) | ✅ Funcionando en banco con el DHT22 (`pruebas/envio_sheets/`) |
| Todo integrado en `estacion_campo/` (memoria, reintentos, `ERR`, pantalla) | Escrito y compilado, **falta probar en la placa** |
| Presión (BMP180) | Código listo y desactivado: el sensor está en camino |
| 8 (API del ITR) | Código listo y desactivado: su API todavía no acepta `null` ni la fecha |
| 9 (página) | La hace el equipo de software |
