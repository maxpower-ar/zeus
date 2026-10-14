# Modelos 3D

Solo las piezas cuya geometría el firmware necesita conocer. El resto del diseño mecánico
(caja, torre, eje, tapa) vive fuera del repositorio.

| Archivo | Qué define | Lo usa |
|---|---|---|
| `encoder_disco.scad` | Disco ranurado del anemómetro | `RANURAS_POR_VUELTA` en los tres sketches del encoder |

**`encoder_disco.scad` es la fuente de verdad de `enc_ranuras`.** Si se cambia ahí, hay que
cambiar `RANURAS_POR_VUELTA` en `pruebas/anemometro`, `pruebas/encoder_optico` y
`pruebas/encoder_rueda`. Una diferencia desplaza toda la escala de velocidad por un factor
constante, y las lecturas igual siguen pareciendo razonables.

El disco viejo de 36 mm con 20 ranuras quedó obsoleto: con ese diámetro no entraban 30
ranuras sin que el diente quedara en medio milímetro.
