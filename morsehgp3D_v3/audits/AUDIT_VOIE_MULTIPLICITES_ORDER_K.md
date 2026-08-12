# Preuve épinglée — propriétaire shallow avec multiplicités

Date : 9 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## Objets séparés

À un sommet `v`, l'intérieur strict `B(v)`, la coquille complète `S(v)`, le
niveau de navigation `|B(v)|`, le rang fermé `|B(v)|+|S(v)|` et le support
HGP canonique sont des objets distincts. Le rang fermé filtre une
publication; il ne doit pas couper la navigation.

## 6. Théorème de propriétaire

Soit `U` un support affinement indépendant de taille `q`, avec `1<=q<=4`, et
soit `x_U` sa sphère minimale. Posons `B_U={i:L_i(x_U)<0}`. Si le nuage a
dimension affine trois, le polyèdre de signes fermé dans le flat de `U` est
non vide et pointé; il possède donc un sommet d'arrangement `o(U)` tel que
`B(o(U))` soit inclus dans `B_U`. Ainsi le niveau du propriétaire ne dépasse
pas le nombre d'intérieurs de la sphère minimale de `U`.

Pour un catalogue de rang fermé au plus `s_max`, les singletons sont directs
et toutes les arités deux à quatre possèdent un propriétaire sous la coupe :

$$\ell\leq k_{\mathrm{nav}},\qquad k_{\mathrm{nav}}=s_{\max}-2.$$

Le propriétaire canonique se choisit par optimisation rationnelle exacte sur
le polyèdre, avec tie-break lexicographique. Ce théorème retire le besoin
d'une table globale de propriétaires; il ne borne ni le nombre de flats, ni
le coût du census, ni le temps à 50 k.

GCP non utilisé.
