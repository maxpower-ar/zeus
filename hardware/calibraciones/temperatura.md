# Calibración de los sensores de temperatura

## DHT22 — en uso

Sensor de referencia del proyecto para temperatura y humedad.

| Fecha | Condición | Temperatura | Humedad | Instrumento de contraste |
|---|---|---|---|---|
| 2026-09-06 | Ambiente en reposo | 21.0 °C | 47.7 % | *(pendiente: contrastar con el UT890C)* |

**Sin offset de calibración aplicado.** El fabricante especifica ±0.5 °C y ±2 % HR, que para
este proyecto alcanza. Si más adelante se contrasta contra otro instrumento y aparece un
desvío sistemático, se anota acá y recién entonces se corrige en el código.

Verificación de respuesta (2026-09-06): soplando llega a 98 % HR y vuelve solo; apoyado contra
una pava caliente (ya sin vapor) la temperatura sube a 26.7 °C y la humedad relativa cae a
41 %. Responde a las dos magnitudes de forma independiente.

---

## LM35Z ×2 — DESCARTADOS

**No se pudieron calibrar: no funcionan.** Ver la entrada del 2026-09-06 en
`docs/bitacora.md` para el detalle de las mediciones.

Resumen: con alimentación verificada de 4.8 V y orientación correcta, la salida se queda en
10-30 mV erráticos (deberían ser ~210 mV a 21 °C) y no responde al calor de la mano. Los dos
se comportan igual. Inscripción del encapsulado: `89015` sobre `LM35DZ`; el código `9015`
corresponde a un transistor común, lo que hace sospechar componente clonado.

**Consecuencia para el proyecto:** se pierde la redundancia de temperatura. La magnitud queda
cubierta por el DHT22, y el BME280 de la fase 4 va a aportar la segunda temperatura para
contrastar. No bloquea ninguna fase.
