# Preuve statique — parent local de reverse search

Date : 9 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## Théorème

À un sommet d'arrangement `v`, supposons connus la coquille complète `S(v)`,
l'intérieur strict `B(v)`, un germe canonique de niveau zéro et un oracle
exact `next(v,d)`. Le cône tangent de la chambre est défini uniquement par
les inégalités des points de `S(v)`.

Un programme linéaire rationnel en dimension quatre choisit un rayon extrême
canonique du cône qui augmente une forme d'un intérieur choisi lorsque le
niveau est positif, ou décroît la fonction du germe au niveau zéro. Le
premier événement dans cette direction est un parent adjacent unique.

Le parent vérifie que son ensemble intérieur est inclus dans celui de `v`.
Si le niveau ne baisse pas, la forme choisie augmente strictement; au niveau
zéro, la fonction du germe décroît strictement. Ce potentiel exclut les
cycles et mène tout sommet shallow au germe sans table globale `seen`.

La règle couvre les sommets multiples uniquement si coquille, intérieur,
rangs et premier lot sont exacts. Elle ne ferme ni l'énumération des enfants,
ni le nombre de flats incidents, ni le census, ni le contrat 50 k.

GCP non utilisé.
