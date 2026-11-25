# Panel web de la estación

Página que muestra las mediciones de la planilla de Google Sheets: los valores de ahora y
todo el historial, con gráficos, tabla y descarga en CSV.

**Dirección:** https://maxpower-ar.github.io/zeus/

## Cómo funciona

No tiene servidor propio. La página (HTML + JavaScript) se publica gratis en **GitHub
Pages**. Al abrirla, el navegador le pide los datos al `doGet` de `servidor/Codigo.gs`, que
lee la planilla y devuelve JSON. Se actualiza sola cada minuto.

```
navegador ──GET ?desde=…&hasta=…──► Apps Script (doGet) ──► planilla
          ◄──────── JSON ──────────
```

- El servidor da hasta 92 días por pedido; para "Todo" el panel pide en tramos de 90 días y
  los junta.
- Con muchos meses, cada gráfico se resume en 600 puntos (promedio por tramo de tiempo).
- `ERR` sale en rojo ("revisar el sensor"); `NaN`, en gris ("sensor no instalado").
- `?rango=7` (o `1`, `30`, `todo`) al final de la dirección abre directo en ese período.

| Archivo | Qué tiene |
|---|---|
| `index.html` | La estructura de la página |
| `estilos.css` | Colores y diseño (modo claro y oscuro según el sistema) |
| `app.js` | Pide los datos y dibuja tarjetas, gráficos y tabla |
| `config.js` | **Lo único para tocar:** URL del servidor, estación, fecha de inicio |

Los gráficos usan la librería **Chart.js** 4.4.1, desde cdnjs.

## La URL del servidor es pública

`config.js` tiene la URL del script (la misma de `URL_SCRIPT` en `secretos.h`). Con ella
cualquiera puede **leer** las mediciones, que es lo que se busca con un panel público.
**Escribir** sigue pidiendo `CLAVE_API`, que no está acá. Si alguna vez se crea una
implementación nueva del script (no "nueva versión"), la URL cambia: actualizarla en
`config.js`.

## Probarlo en la compu

Abrir `index.html` directo no siempre anda (el navegador limita lo que hace una página
abierta como archivo). Desde esta carpeta:

```
python -m http.server 8000
```

y abrir http://localhost:8000.

## Publicarlo

GitHub Pages sirve la rama `gh-pages`, que tiene solo esta carpeta. Después de cambiar algo
acá y hacer commit en `main`:

```
git subtree push --prefix dashboard origin gh-pages
```

En un minuto o dos se ve en la dirección de arriba. Si es la primera vez: en GitHub,
**Settings → Pages → Source: Deploy from a branch → `gh-pages` / `(root)` → Save**.
