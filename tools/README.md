# Herramientas de PC

Utilidades para probar la estación desde la computadora. No son parte del firmware.

| Herramienta | Para qué |
|---|---|
| `probar_servidor.js` | Prueba la expresión regular y las conversiones de `servidor/Codigo.gs` (números, `NaN`, `ERR`) sin Google. Correr con `node tools/probar_servidor.js` cada vez que se toque el formato |

El servidor de prueba que estaba previsto acá se reemplazó por nuestro servidor de Google
Sheets (ver [`servidor/LEEME.md`](../servidor/LEEME.md)), que se prueba desde la PC con `curl`.
