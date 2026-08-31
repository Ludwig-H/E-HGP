# Provenance v6

Principe (AGENTS.md) : rien d'hérité implicitement. Pin v5 de référence :
`3bad233d` (31 août 2026). Trois régimes :

- **`port_contractuel`** : l'égalité bit à bit EST le contrat, épinglée par
  digests gravés.
- **`port_source_requalified`** : source v5 quasi littérale, épinglée au pin
  v5 ET requalifiée par une porte v6 dédiée exécutée.
- **`port_source_pending_requalification`** : même origine, portes dédiées
  pas encore portées — requalification par les seules portes transverses
  (conformité d'objet, boucle de mutants). C'est l'état honnête du gros du
  socle tant que les portes v5 correspondantes ne sont pas portées.
- **`re_derive`** : réellement réécrit depuis `docs/MATHEMATIQUES.md`,
  requalifié par oracle indépendant et mutants v6.
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

Monnaies de digest GELÉES (P0 du 31 août — aucun renommage conditionnel) :
la conformité d'objet v5↔v6 porte sur `digest_all` et chaque
`digest_forest_K*`. `digest_candidates_v5_compat` (tag v4, candidats uniques
post-RLE) est un diagnostic différentiel de génération : identique à la v5
au checkpoint J2 (le sweep est une transformation de coût), il DIVERGE
légitimement depuis le correctif du cover q4 au coefficient 4 (le
coefficient 3 v5 perdait des témoins intérieurs — P0, porte
`mhgp6_cover_coef4` ; sur la v5, `q4-cover-coef4` donnait 3 134 427 →
3 134 404 candidats uniques à uniform 8000, survivantes et `digest_all`
inchangés). `digest_postprefilter` (tag `mhgp6-digest-v1:postprefilter-candidates`)
signe les records survivants du préfiltre exact : non-régression interne v6,
golden gravé (uniform 400). Le pattern d'erreur n° 6 v5 (un digest qui
mesure un filtre) reste fermé : aucun digest de candidats n'est un critère
d'objet.

## Ports de source (pin v5 3bad233d)

Transcriptions quasi littérales (renommage mhgp5→mhgp6 seulement, diffs
mécaniques vérifiés à la livraison). Statut
`port_source_pending_requalification` par défaut ; passent
`port_source_requalified` avec leur porte dédiée (état au 31 août :
sector_kill/chord_kill/cell_grid via les fixtures du sweep et les mutants
exécutés ; census/candidates via conformité + mutants ; le reste en
attente) :
`src/core/{types,morton,intmath,wide,dint,mutants,parse,device}.hpp`,
`src/tree/cloud_index.hpp`, `src/wspd/wavefront.hpp`,
`src/spindle/{spindle,witness_count}.hpp`, `src/lanes/{keys,level,q2,q3,q4,
sector_kill,chord_kill,cell_grid,edge_cover}.hpp`,
`src/pipeline/{candidates,census,expand,float_filter}.hpp`,
`src/forest/{plateau,fold,render}.hpp`, `src/parallel/{pool,sort}.hpp`,
`oracle/obig.hpp`.

## Re-dérivés

`src/pipeline/run.hpp` (orchestration réécrite : front fusionné, grand-livre
global des paires à la place du ledger postsep, impression v6 ; le pipeline à
deux étages du fold reste une transcription de la machinerie v5 auditée),
`cli/mhgp6.cpp` (parsing exact de toutes les options).

## Neufs

Descente WSPD fusionnée à masques (`alive_rectangles_fused`) et sweep de
corde unifié — tous deux dans `src/pipeline/generate.hpp` ; familles
stationnaires (`terrain_stationnaire`, `scanline_stationnaire`, dans
`src/cloud/families.hpp` à côté des ports) ; grand-livre global des paires.
Prévus (J3+) : requêtes de facteurs saturées (route M),
`AnchorCredit`/`CoreCredit`/`ResidualTape`, porte `linked_arcs_u16`, Tier R
et moteur plan (E6).

## Non repris (dettes v5 fermées à la conception)

- Trois descentes WSPD séparées (une seule descente fusionnée existe en v6).
- Auto-produits `corner_histograms` sans saturation ni raccourci de facteurs.
- Rescan de profondeur par candidat q4 (mutualisé par seed dans le sweep de
  corde unifié ; l'incidence seed–complétion reste payée et publiée).
- `digest_balls` comme monnaie appelée à mesurer la force des tueurs : tant
  que le multiensemble reste identique à la v5, il sert de conformité ; dès
  qu'un tueur le rompt, bascule post-préfiltre (voir plus haut).
- Parsing tolérant (`atoi`/`atoll`) : **tout** le CLI v6 passe par
  `parse_i64_exact` ; suffixe, vide, débordement ⟹ code 2.
- Opt-in s < 8 : la branche est conditionnée `MHGP6_TESTING`, pas seulement
  le champ d'options.
