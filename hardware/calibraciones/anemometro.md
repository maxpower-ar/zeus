# Calibración del anemómetro

Instrumento: rotor de 3 cazoletas sobre rulemanes 10×22×6, disco ranurado y encoder
óptico FC-03 en GPIO18. Sketch: `pruebas/anemometro/`.

Instrumento de referencia: multímetro UT890C para las tensiones, velocímetro del auto para
la velocidad de viento.

---

## 1. Ranuras del disco — PENDIENTE

Hay una diferencia sin resolver:

| Fuente | Ranuras |
|---|---|
| Disco en uso (indicado) | **30** |
| Modelo OpenSCAD del rotor impreso (`markii/estacion_anemometro.scad`, `enc_ranuras`) | **20** |

**Hay que contarlas a mano antes de calibrar nada.** Si el disco fuera de 20 y el código
dijera 30, todas las velocidades saldrían un tercio más bajas, y las lecturas igual
parecerían razonables: es el error más difícil de detectar de esta fase.

Procedimiento: sketch `pruebas/encoder_optico`, marcar la rueda, teclear `r`, dar
**exactamente una vuelta lenta** con el dedo, leer el total.

| Fecha | Ranuras contadas | Quién |
|---|---|---|
| | | |

---

## 2. Geometría del rotor — PENDIENTE

| Dato | Valor | Cómo se mide |
|---|---|---|
| Radio del rotor | *(pendiente)* | Del centro del eje al centro de una cazoleta, con regla |
| Diámetro de cazoleta | *(pendiente)* | |
| Cantidad de cazoletas | 3 | |

No hacen falta para medir —la calibración los absorbe a los dos— pero sirven para
verificar que el resultado tenga sentido físico.

---

## 3. Constante de calibración `metrosPorPulso` — PENDIENTE

Es la única constante que convierte pulsos en velocidad:

```
velocidad [m/s] = metrosPorPulso × pulsos_por_segundo
velocidad [km/h] = velocidad [m/s] × 3.6        (exacto, no es calibración)
```

Equivale a `factor_anemómetro × 2π × radio / ranuras`. Se mide el producto completo de
una sola vez, porque el factor y el radio no se pueden separar con precisión.

**Valor provisional en el código: `0.0471`** — supone radio 0.090 m y factor 2.5, que son
cifras típicas pero **no son las de este rotor**. No sirve como medición.

### Método del auto

Anemómetro firme fuera de la ventanilla, lo más lejos posible de la carrocería (el aire
pegado al auto va más lento que el auto). Camino recto, día sin viento, velocidad
constante. Esperar ~5 s a que los pulsos/s se estabilicen y teclear `c 40` en el monitor
serie, siendo 40 los km/h del velocímetro.

**Cada velocidad se hace de ida y de vuelta y se promedia.** Si hay aunque sea poco
viento, de ida se suma y de vuelta se resta; promediando se cancela.

Maneja otra persona.

| Fecha | Referencia (km/h) | Sentido | pulsos/s | `metrosPorPulso` |
|---|---|---|---|---|
| | 20 | ida | | |
| | 20 | vuelta | | |
| | 30 | ida | | |
| | 30 | vuelta | | |
| | 40 | ida | | |
| | 40 | vuelta | | |
| | 50 | ida | | |
| | 50 | vuelta | | |

**Valor adoptado:** *(pendiente)*

**Criterio:** los valores de las distintas velocidades tienen que dar **parecidos entre
sí**. Si dan parecidos, la relación es lineal como se espera y se promedian. Si dan muy
distintos, hay algo mal en el montaje y no se puede seguir: anotarlo abajo.

---

## 4. Velocidad de arranque — PENDIENTE

Por debajo de qué viento el rotor no arranca. Es el límite conocido del instrumento y hay
que documentarlo: por debajo de ese valor la estación informa calma aunque haya brisa.

| Fecha | Velocidad de arranque (km/h) | Cómo se determinó |
|---|---|---|
| | | |

---

## 5. Comparación de rotores y fricción — PENDIENTE

Tiempo de giro libre tras un empujón: más tiempo = menos fricción = detecta brisa más
suave.

| Montaje | Tiempo de giro libre | Observaciones |
|---|---|---|
| Rotor sobre rulemanes 10×22×6 | | |
| Cooler Jamicon (si se prueba) | | |
| Eje del kit TT (si se prueba) | | |

---

## 6. Verificación contra referencia externa — PENDIENTE

Una vez calibrado, contrastar en un día ventoso contra lo que informa el servicio
meteorológico local. No van a coincidir exactamente —la estación oficial mide a 10 m de
altura y en campo abierto— pero el orden de magnitud y la evolución tienen que seguirse.

| Fecha | Estación propia | Servicio meteorológico | Diferencia |
|---|---|---|---|
| | | | |

---

## Observaciones
