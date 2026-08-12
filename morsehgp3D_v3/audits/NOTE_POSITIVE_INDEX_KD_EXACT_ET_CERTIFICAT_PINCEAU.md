# Preuve statique — index spatial exact et pinceau

Date : 9 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## 1.3 Boîte contre boule rationnelle

Pour un centre `C/d`, une boîte entière et un numérateur de rayon carré `N`,
la distance minimale exacte s'obtient axe par axe en prenant l'écart nul si
`C_j` appartient à l'intervalle dilaté, sinon l'écart à l'extrémité la plus
proche. La boîte est strictement extérieure si la somme des trois carrés est
strictement supérieure à `N`. La distance maximale emploie l'extrémité la
plus éloignée; la boîte est strictement intérieure si sa somme est
strictement inférieure à `N`. Toute égalité descend.

Sur le profil u16, les carrés intermédiaires peuvent atteindre environ
`2^182`; l'accumulation exige le type multiprécision reçu, après promotion.

Fixture permanente du filtre flottant réfuté :

```text
a=(32767,32767,0)
b=(57863,57862,0)
c=(7672,7673,0)
d=(60104,30135,1)
```

Les quatre points sont exactement sur la sphère; une enveloppe flottante
historique élaguait pourtant la racine.

## Pinceau

La puissance d'un point le long d'un pinceau de sphères est affine en son
paramètre. Tout événement intérieur à un segment change donc de signe entre
les deux sphères terminales, sauf les points constants déjà dans la fermeture
du flat. Une requête exacte des signes différents, suivie d'un tri rationnel
des événements, suffit; ce n'est pas la différence symétrique des boules
fermées.

GCP non utilisé.
