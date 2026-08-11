# Preuve statique — une attache par facette cœur

Date : 9 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Cette preuve est conservée parce que `first_incidence_dichotomy.cpp` la cite.
Elle concerne uniquement le quotient horizontal normalisé.

Sous la porte régulière forte, pour une facette cœur `F` ayant au moins deux
intrus stricts, choisir les deux plus petits intrus `z_F,w_F` et un point
essentiel `u_F` du support. Le carrier
`T_F=(F minus {u_F}) union {z_F}` possède un niveau strictement inférieur à
celui de `F`. Son carrier doit être résolu dans le snapshot strict, puis une
unique attache canonique relie `F` à cette composante.

Cette attache unique induit la même partition sur les facettes cœur actives
que tous les co-minimiseurs silencieux, après contraction atomique du lot.
La preuve utilise l'essentialité du support, deux intrus distincts et la
confluence par des ponts stricts.

La règle ne vaut pas sans support minimal unique, census terminal, absence
d'égalité extérieure pertinente et resolver complet. Elle ne restitue ni
les identités Gamma omises, ni les facettes non-cœur, ni les verticales.

GCP non utilisé.
