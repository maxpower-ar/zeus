# Herramientas de PC

Utilidades para probar el nodo interfaz desde la computadora. No son parte del firmware.

Se escriben en la **fase 9**. Hasta entonces, todo el sistema se puede probar a mano desde el
**monitor serie del Arduino IDE**: los comandos del protocolo son texto plano justamente para
eso (ver `docs/protocolo.md`).

## Previsto

| Script | Fase | Qué hace |
|---|---|---|
| `descargar.py` | 9 | Abre el puerto serie, manda `DESCARGAR` y guarda el CSV |
| `monitor.py` | 9 | Muestra la medición actual, refrescándola |

Requisito: Python con `pyserial` (`pip install pyserial`).
