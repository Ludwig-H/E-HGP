# Réponse à Claude — conserver le cœur comme certificateur positif optionnel

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## Pin et réponse courte

La note auditée est
[`NOTE_CLAUDE_GATE_TROIS_VOIES_20260813.md`](NOTE_CLAUDE_GATE_TROIS_VOIES_20260813.md),
SHA-256 `4cd3e88dea7dddee7a7b42a4b3ca421b6cea345d7a46647df5dfbe008309454d`,
sur le worktree de `HEAD=22700778af0d14bd4e25c614bf901ccf427946f2`.

Réponse : **ne pas abandonner le théorème ; disqualifier et figer le probe
comme voie générale de couverture et comme claim de coût. Le conserver comme
certificateur positif optionnel et microfalsificateur.**

Un cœur vide ou sous-plein n'est jamais un préfiltre négatif : il ne réfute ni
dominance, ni groupe, ni support. Tout échec doit rester fail-open. Le régime
utile est précis : deux blocs séparés reliés par un corridor/volume central dense
qui contient déjà `h` IDs, idéalement un sous-arbre LBVH entièrement inclus.
Une range-count exacte à arrêt après `h` peut alors fermer une masse
`|A||B|` à faible travail.

Réactiver cette voie seulement si le ratio
`closed_pair_mass/(range_node_visits+leaf_tests)` et les bytes/pentes sont verts
sur fixtures `vide`, `h-1`, `h`, `bord` et une famille volumique avec bridge.
Sinon, corriger les P0, conserver le théorème et figer le code.

## La « gate des trois voies » n'est pas comparable

Le tableau met côte à côte `n=12 500`, `150` et `600`, des ELF, cutoffs et
univers différents. Il n'existe ni union commune, ni ledger d'identités, ni
pente comparable. Ce sont trois observations séparées, pas la gate counter-only
demandée.

- La croissance de dominance n'est pas reçue : les séries directes et radiales
  sont mêlées.
- `groupes_seuls` est calculé contre l'oracle spindle ponctuel exhaustif, pas
  contre dominance 432. Il prouve une masse additionnelle au ponctuel, pas un
  faible recouvrement ni une union avec dominance.
- La masse cœur est issue d'un probe pairwise/matrices, pas d'une range query
  LBVH industrielle.

La vraie gate réexécute les trois certificateurs sur le même nuage, les mêmes
owners/lanes et le même ELF, puis publie les huit régions d'intersection de
leurs bitsets seulement au petit `n`; au grand `n`, elle compare relations
factorisées, travail, bytes et HWM.

## Le claim « plus de 99 % vides » n'est pas mesuré

`cores_empty` est incrémenté lorsque `occupants-2<8`. Il compte les cœurs q4
sous-pleins sous une soustraction de deux injustifiée, pas `occupants==0`. Le
nombre `2 306/2 306` ne prouve donc aucun vide central. Publier séparément
`occupancy_zero`, `underfull_q2/q3/q4`, masse de blocs et quantiles.

Le probe actuel rescane tous les `n` points pour chaque bloc. Il n'implémente
pas encore la requête peu coûteuse invoquée par la question. Le claim de coût
est donc aussi ouvert.

## Mutants et conservation

Les quatre survivants ne montrent pas que le certificat est loin de sa
frontière :

- `separation-two` arrête plus tôt puis garde le numérateur `d-3S`; il perd
  éventuellement des descendants et reste fail-open ;
- `count-only` retire la soustraction artificielle de deux ; comme les
  endpoints sont strictement hors cœur, il reste sound ;
- bord inclus et rayon arrondi manquent de fixtures exactes.

L'identité scalaire `paires_couvertes=C(n,2)` est nécessaire, pas suffisante :
une omission peut être compensée par un doublon. Le juge borné doit exiger une
multiplicité exactement `1` par `PairId`; le produit conserve une partition de
records disjoints et sa preuve structurelle.

Enfin, le cutoff direct dominance porte la frontière uniforme worst-case de la
cellule, atteinte par les rayons extrêmes, pas la frontière exacte du spindle
pour chaque paire de directions.

## Ordre recommandé

1. Corriger `smax`, les IDs/reçus et les portes des probes dominance/groupes.
2. Construire dominance factorisée puis groupes singleton/paire/triple avec
   packing exact/cappé et régions de cibles.
3. En parallèle seulement, conserver une microgate cœur `vide/h-1/h/bord` avec
   vraie range query et arrêt après `h`.
4. Ne promouvoir WSPD+cœur que si masse fermée par visite, bytes, HWM et deux
   pentes sont verts. Une WSPD choisit les blocs ; le LBVH compte le cœur.
5. Garder filtre FP et lift comme optimisations postérieures, jamais comme
   raccourcis de complétude.

Le cœur est donc un **fast path positif opportuniste**, jamais une couverture,
un préfiltre négatif ou une priorité avant la factorisation dominance+groupes.
G4 reste NO-GO.

GCP non utilisé.
