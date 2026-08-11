# Note de livraison Claude — gate de réception du delta L4/héritage

Date : 11 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Note de livraison répondant à la porte 1 de
[`AUDIT_ETAT_COURANT.md`](AUDIT_ETAT_COURANT.md) (« recevoir L4 et
l'héritage par différentiel baseline, mutants ciblés et extrêmes u16 »). La
réception appartient aux auditeurs.

## Le différentiel bi-mode (`--differential 1`)

Le même binaire exécute la BASELINE reçue de `40050c4` (ni L4, ni héritage,
sans les injections du sujet — le juge n'hérite jamais du mutant) puis le
mode optimisé, et exige l'égalité de TOUS les sorts de paires et de toutes
les masses (états, prunées, microtuiles, états prunés/microtuiles). Un
budget ou une inclusion unilatérale survivrait à une perte de prune ;
l'égalité non. Mesuré (n=400 terrain) : baseline 197 948 visites /
579 764 tests contre optimisé 94 109 / 159 624, sorts identiques.

Le reçu engage désormais l'arbre : `order-digest` (SHA-256 de `tree.order`)
est publié — les positions héritées sont des handles locaux dans cet ordre,
pas des `PointId` persistants. Les compteurs séparent `L4-retraits` (et
points en MULTIPLICITÉ de travail), `U4-credits`, `hérités` (multiplicité),
sorties précoces et le cas `9+1` (seuil atteint avec neuf hérités plus du
neuf) ; le plancher `--min-nine-plus-one` arme le mutant correspondant
(9+1 = 55/41/196 sur terrain 400 / uniforme 400 / uniforme 1000).

## Les cinq mutants, tous à code exact 4

| mutant | mutation | tué par |
| --- | --- | --- |
| `l4-sign` | la borne SUPÉRIEURE prise pour l'infimum | égalité des masses |
| `inherit-shift` | handle hérité décalé d'une position | soundness par paire |
| `inherit-double-count` | hérité recompté dans un crédit de nœud | soundness par paire |
| `inherit-stale-sibling` | récolte d'un bloc transmise à des blocs étrangers | égalité des masses |
| `stop-at-inherited` | neuf hérités : la recherche du dixième sautée | égalité des masses |

## Fixture `u16-extremes`

Paire diagonale `(0,0,0)`-`(65535,65535,65535)`, quatre coins EXACTEMENT
sur sa sphère diamétrale (angle droit, contact), douze témoins stricts au
centre : L4/U4/sup travaillent aux magnitudes maximales du profil
(`|t-2x| < 2^18`, produits < 2^36). Fate/inclusion ferment (131 paires non
inertes). La campagne sanitizers doit rejouer cette fixture.

## Divers

- Le NO-GO de parcimonie à 50 % est orthogonal au différentiel : uniforme à
  n=400 rend 62 % de microtuiles (code 3) — la porte uniforme utilise
  n=1000 (41 %).
- Commentaires corrigés : capacité de pile réservée (plus « une seule
  allocation »), multiplicités documentées, compte des mutants CMake.
- Sous-ensemble ciblé : 66/66 (self-join 39, ancres 21, sidecar 6).

Restent, dans l'ordre de l'état courant : la comparaison Yao48/LBVH avec
classifieur terminal (porte 2), la campagne sanitizers sidecar et la
sémantique du digest de version (porte 3), le filtre terminal de profondeur
et center-cover du falsificateur d'ancres (porte 4).

GCP non utilisé pour cette livraison.

## Addendum — porte 3 de l'état courant (sidecar)

- `producer_code_digest` renommé `producer_version_digest`, avec la
  sémantique honnête gravée : identifiant SÉMANTIQUE de la convention du
  producteur (chaîne de version digérée), jamais un SHA du source ou de
  l'ELF — la provenance binaire vit dans les reçus de run.
- Mutants d'IDENTITÉ du producteur (fixture 17) : un reçu au mauvais
  contrat, profil, schéma de tâches ou statut terminal laisse la fermeture
  `kUnknown`, digests corrects compris — les quatre variantes sont testées.
- Injection `sha-fault` : la factory refuse par SON contrôle interne, sans
  prétest externe (mutant à code 4).
- Grille : borne HAUTE couverte (coordonnée 65536 et base 65536 refusées).
- CAMPAGNE SANITIZERS sur l'empreinte finale : gate compilée
  `-fsanitize=address,undefined -fno-sanitize-recover=all`, nominal 0,
  cinq mutants à 4, AUCUNE erreur sanitizer.
