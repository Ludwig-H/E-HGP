# Provenance v6

Principe (AGENTS.md) : rien d'hérité implicitement. Pin v5 de référence :
`3bad233d` (31 août 2026). Trois régimes :

- **`port_contractuel`** : l'égalité bit à bit EST le contrat, épinglée par
  digests gravés.
- **`re_derive`** : réécrit depuis `docs/MATHEMATIQUES.md`, requalifié par
  oracle indépendant et mutants v6. Une ressemblance de code avec la v5 n'est
  pas une autorité ; seules les portes v6 qualifient.
- **`neuf`** : sans équivalent v5.

## Ports contractuels (les seuls)

| Élément | Contrat | Autorité |
|---|---|---|
| `src/core/sha256.hpp` | FIPS 180-4, chemins SHA-NI et portable bit à bit | vecteurs FIPS + porte d'égalité des deux chemins |
| `src/cloud/families.hpp` (familles v5 dilatées + contre-familles) | générateurs bit à bit : même (famille, n, coord, graine) ⟹ même nuage, même ordre | digests v4/v5 gravés (`mhgp6_families_fixture`) |
| `src/pipeline/digest.hpp` | sérialisation `mhgp4-digest-v1` reproduite à l'identique | égalité aux digests v5 gravés de `receipts/conformite_v5/` |

## Conformité différentielle v5 ↔ v6

Campagne appariée : mêmes entrées (famille, n, coord par défaut, s=8,
smax=11, seed=3), tailles 8000/16000/32000, cinq familles dilatées. Digests
calculés une fois par la v5 au pin `3bad233d` et gravés dans
`receipts/conformite_v5/` (binaire sha256 `945c9a7f…`, code 0 sur les
15 runs). La monnaie de conformité est **l'objet** : `digest_all` et les dix
`digest_forest_K*`. Un digest égal prouve « même objet que la v5 », jamais
l'exactitude HGP.

`digest_balls` v6 est une **nouvelle base**, prise après le préfiltre exact
count-only (frontière canonique v6). Raison : le multiensemble de candidats
pré-préfiltre mesure la force des tueurs fail-open, pas l'objet (pattern
d'erreur n° 6 v5). La v6 grave ses propres `digest_balls` post-préfiltre
comme non-régression interne.

## Re-dérivés (extraits, liste tenue à jour)

`src/core/{types,morton,intmath,wide,dint,mutants,parse}.hpp`,
`src/tree/cloud_index.hpp`, `src/wspd/wavefront.hpp`,
`src/spindle/{spindle,witness_count}.hpp`, `src/lanes/{keys,level,q2,q3,q4,
sector_kill,chord_kill,cell_grid,edge_cover}.hpp`,
`src/pipeline/{candidates,census,expand,float_filter,run}.hpp`,
`src/forest/{plateau,fold,render}.hpp`, `src/parallel/{pool,sort}.hpp`,
`oracle/obig.hpp`.

## Neufs

Descente WSPD fusionnée à masques (`alive_rectangles_fused`), requêtes de
facteurs saturées (route M), `AnchorCredit`/`CoreCredit`/`ResidualTape`
(`src/credit/`), sweep de corde unifié (`src/carrier/chord_sweep.hpp`),
familles stationnaires (`terrain_stationnaire`, `scanline_stationnaire`),
porte `linked_arcs_u16`, grand-livre (`GenLedger`), Tier R et moteur plan
(E6, prévus).

## Non repris (dettes v5 fermées à la conception)

- Trois descentes WSPD séparées (une seule descente fusionnée existe en v6).
- Auto-produits `corner_histograms` sans saturation ni raccourci de facteurs.
- Boucle C×D des complétions q4 et filtre de profondeur par candidat
  (remplacés par le sweep de corde unifié).
- `digest_balls` pré-préfiltre comme monnaie de conformité.
- Parsing tolérant (`atoi`/`atoll`) : **tout** le CLI v6 passe par
  `parse_i64_exact` ; suffixe, vide, débordement ⟹ code 2.
- Opt-in s < 8 : la branche est conditionnée `MHGP6_TESTING`, pas seulement
  le champ d'options.
