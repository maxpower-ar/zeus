// =====================================================
//  Encoder del anemómetro (disco ranurado + buje con prisionero M3)
//  Pieza suelta para probar ajustes. Unidades: mm
//  Se imprime así como sale (disco sobre la cama, buje hacia arriba)
//  y se monta dado vuelta (buje hacia abajo).
//
//  FUENTE DE VERDAD de enc_ranuras para el firmware. Los sketches
//  pruebas/anemometro, pruebas/encoder_optico y pruebas/encoder_rueda
//  tienen RANURAS_POR_VUELTA = 30 por este archivo. Si se cambia acá,
//  hay que cambiarlo en los tres.
//
//  Reemplaza al disco viejo de 36 mm y 20 ranuras: con 36 mm no entraban
//  30 ranuras (el diente quedaba en medio milímetro).
// =====================================================

/* [Disco] */
enc_diametro  = 69;    // diámetro del disco
enc_espesor   = 2;     // espesor del disco (tiene que pasar holgado por la U del sensor)

/* [Ranuras] */
enc_ranuras   = 30;    // cantidad = pulsos por vuelta (si lo cambiás, cambialo en el código)
enc_ranura_l  = 10;     // largo radial de cada ranura
enc_ranura_ai = 2.4;   // ancho del lado de adentro
enc_ranura_ae = 3.5;   // ancho del lado de afuera
enc_margen    = 1.0;   // cuánto disco queda afuera de las ranuras (borde exterior)

/* [Buje] */
enc_buje_d    = 26;    // diámetro exterior del buje
enc_buje_alto = 8;     // alto total (incluye el disco)

/* [Agujero para el eje] */
eje_cuerpo_d  = 16;    // diámetro de la parte gruesa del eje
plano_dist    = 4;     // distancia del plano (D) del eje al centro
enc_holgura   = 0.2;   // juego del agujero: subilo si entra duro, bajalo si baila

/* [Prisionero M3] */
torn_d        = 3.3;   // agujero del tornillo
torn_z        = 4.5;   // altura del tornillo desde la base
tuerca_ancho  = 6.2;   // ancho del alojamiento de la tuerca (M3 = 5.5 entre caras + juego)
tuerca_esp    = 2.8;   // espesor del alojamiento de la tuerca

/* [Calidad] */
$fn = 96;

// ---------- Derivados ----------
ri_ext = enc_diametro/2 - enc_margen;
ri_int = ri_ext - enc_ranura_l;
plano_y = plano_dist;

assert(ri_int > enc_buje_d/2 + 1, "Las ranuras se meten en el buje: agrandá el disco o achicá enc_ranura_l");
assert(enc_buje_d/2 > eje_cuerpo_d/2 + tuerca_esp + 2, "El buje es muy chico para la tuerca: agrandá enc_buje_d");

echo(str("Ranuras entre r = ", ri_int, " y r = ", ri_ext, " mm (el haz del sensor tiene que caer ahí)"));

// ---------- Pieza ----------
module encoder() {
    difference() {
        union() {
            cylinder(d = enc_diametro, h = enc_espesor);
            cylinder(d = enc_buje_d, h = enc_buje_alto);
        }
        // ranuras
        for (i = [0 : enc_ranuras - 1]) rotate([0, 0, i*360/enc_ranuras]) translate([0, 0, -1])
            linear_extrude(enc_espesor + 2)
                polygon([[ri_int, -enc_ranura_ai/2], [ri_ext, -enc_ranura_ae/2],
                         [ri_ext,  enc_ranura_ae/2], [ri_int,  enc_ranura_ai/2]]);
        // agujero en D para el eje
        translate([0, 0, -1]) linear_extrude(enc_buje_alto + 2)
            intersection() {
                circle(d = eje_cuerpo_d + enc_holgura);
                translate([-50, -plano_y - enc_holgura/2]) square(100);
            }
        // prisionero M3 contra el plano
        translate([0, 0, torn_z]) rotate([90, 0, 0]) cylinder(d = torn_d, h = enc_buje_d);
        // alojamiento de la tuerca (se carga desde arriba)
        translate([-tuerca_ancho/2, -plano_y - 4.6, torn_z - tuerca_ancho/2]) cube([tuerca_ancho, tuerca_esp, 10]);
    }
}

encoder();
