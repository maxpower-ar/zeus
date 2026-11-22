# Servidor propio: Google Sheets + Apps Script

Nuestro servidor, gratuito, para recibir las mediciones de la estación y guardarlas en una
planilla de Google. Es independiente del servidor del ITR: la estación manda **la misma
línea** a cualquiera de los dos (formato en [`docs/servidor.md`](../docs/servidor.md)).

```
ESP32 ──HTTPS POST──► Apps Script (doPost) ──► Planilla: una pestaña por mes (2026-11, ...)
Página web ─────────► Apps Script (doGet)  ──► JSON con las mediciones de un rango de fechas
```

- `Codigo.gs` — el código del servidor. **Copia de referencia**: el que corre de verdad es el
  pegado en el editor de Apps Script. Si se cambia en un lado, se actualiza el otro

---

## Instalación (una sola vez, unos 15 minutos)

### 1. Cuenta de Google de la estación

Crear una cuenta de Gmail **exclusiva para la estación** (por ejemplo
`estacion.itr.001@gmail.com`), no la personal de nadie ni la del colegio. Guardar la contraseña
fuera del repositorio y que la tengan al menos dos personas.

### 2. La planilla

1. Con esa cuenta, entrar a [sheets.google.com](https://sheets.google.com) y crear una planilla
   en blanco. Nombre: `Estación EM_ITR_001`
2. **Archivo → Configuración → Zona horaria:** `(GMT-03:00) Buenos Aires`

No hace falta crear pestañas ni columnas: el script crea la pestaña de cada mes sola, con sus
títulos, la primera vez que llega una medición de ese mes.

### 3. El script

1. En la planilla: **Extensiones → Apps Script**
2. Borrar todo lo que haya en el editor y pegar el contenido completo de `Codigo.gs`
3. Guardar (ícono del disquete o Ctrl+S). Nombre del proyecto: `Servidor estación`
4. **Configuración del proyecto** (engranaje, a la izquierda) → **Zona horaria:**
   `(GMT-03:00) Buenos Aires`

### 4. La clave

1. En el editor, arriba, elegir la función **`generarClave`** en la lista y apretar
   **Ejecutar**
2. La primera vez Google pide permisos:
   **Revisar permisos → elegir la cuenta de la estación →** aparece *"Google no verificó esta
   app"* → **Configuración avanzada → Ir a Servidor estación (no seguro) → Permitir**.
   Es normal: la app es nuestra y nadie la revisó
3. Abajo, en **Registro de ejecución**, aparece la clave. Copiarla: va en `secretos.h`

Si se pierde, volver a ejecutar `generarClave`: no la cambia, solo la vuelve a mostrar.

### 5. Prueba sin ESP32

Elegir la función **`probarGuardado`** y **Ejecutar**. En el registro tiene que aparecer:

```
[10/11/2026;15:42;EM_PRUEBA;21,5;55;3,2;NaN;NaN;1013,2;NaN;EOF]  ->  GUARDADA
Segunda vez (tiene que decir DUPLICADA)  ->  DUPLICADA
```

En la planilla aparece una pestaña nueva con el mes actual y una fila `EM_PRUEBA`. Las celdas de
gas y dirección quedan **vacías**: así se ve un dato que no existe (nunca un cero). La fila de
prueba se puede borrar.

### 6. Publicar la API

1. **Implementar → Nueva implementación**
2. Engranaje al lado de "Seleccionar tipo" → **Aplicación web**
3. Descripción: `v1`. **Ejecutar como: Yo**. **Quién tiene acceso: Cualquier persona**
4. **Implementar** y copiar la **URL de la aplicación web** (termina en `/exec`). Va en
   `secretos.h`

"Cualquier persona" es necesario porque el ESP32 no puede iniciar sesión en Google. Lo que
impide que un extraño cargue datos es la clave.

### 7. Probar la API desde la PC

En PowerShell (cambiando `URL` y `CLAVE`):

```powershell
curl.exe -L -H "Content-Type: text/plain" -d "[10/11/2026;15:45;EM_PRUEBA;22,0;50;1,0;NaN;NaN;1013,2;NaN;EOF]" "URL?clave=CLAVE"
```

Tiene que responder `OK guardadas=1 duplicadas=0`. Repetirlo: ahora `OK guardadas=0 duplicadas=1`.
Con una clave equivocada: `ERROR clave incorrecta`.

Para ver la consulta de la página, abrir en el navegador `URL` (últimos 7 días) o
`URL?desde=2026-11-01&hasta=2026-11-30`.

---

## ⚠ Al cambiar el código del script

**No** usar "Nueva implementación": genera **otra URL** y habría que regrabar el ESP32.
Lo correcto es:

**Implementar → Gestionar implementaciones → lápiz (editar) → Versión: Nueva versión →
Implementar.** La URL sigue siendo la misma.

---

## Respuestas del servidor

| Respuesta | Significa | Qué hace el ESP32 |
|---|---|---|
| `OK guardadas=1 duplicadas=0` | Guardada | La da por enviada |
| `OK guardadas=0 duplicadas=1` | Ya estaba guardada (reenvío después de un corte) | La da por enviada |
| `ERROR clave incorrecta` | Falta la clave o es otra | Revisar `secretos.h` |
| `ERROR linea 1: formato invalido ...` | La línea no respeta el formato | Bug en el firmware |
| `ERROR servidor ocupado, reintentar` | Llegaron muchos pedidos juntos | Reintentar más tarde |

Varias líneas en un mismo envío (una por renglón) también se aceptan: es lo que va a usar la
estación para mandar las mediciones pendientes después de un corte (fase 9).

## Detalles de diseño

- **Pestaña por mes, según la fecha de la medición** y no la de llegada: una medición del 31/10
  que llega el 1/11 va a `2026-10`
- **Duplicados:** la columna `Id` (`código|fecha hora`) identifica cada medición; si ya existe, no
  se vuelve a guardar. Además, un bloqueo hace que dos envíos simultáneos se atiendan de a uno
- **Columna `Recibido`:** hora en que llegó al servidor. Si difiere mucho de `Fecha y hora`, esa
  medición quedó pendiente durante un corte de red
- **Pestañas viejas:** las creadas antes del 2026-11-17 no tenían presión ni luminosidad. La
  primera medición nueva que llega a una de esas pestañas le agrega las dos columnas (vacías
  en las filas anteriores)
- **Capacidad:** una planilla admite 10 millones de celdas en total (sumando pestañas). A una
  medición cada 5 minutos alcanza para unos 9 años
