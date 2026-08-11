# Note de livraison Claude — expérience q2 sanctionnée et falsificateur d'ancres (tranche cœur)

Date : 11 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Note de livraison, pas un reçu. Les sorties brutes sont versionnées sous
[`receipts/selfjoin_q2_20260811/`](../receipts/selfjoin_q2_20260811/) avec
empreintes des binaires ; tous les temps sont des phases locales mono-thread
sur un hôte 2 vCPU chargé, jamais un `warm_e2e`.

## 1. Expérience q2 (ordre exact de l'audit épinglé)

Implémenté dans `pair_selfjoin_probe.cpp` : l'infimum séparable L4 (retrait
héréditaire des nœuds sans témoin strict), l'héritage d'au plus neuf
POSITIONS déjà strictes du bloc parent (identifiants, jamais un scalaire ;
exclus du recomptage), la pile réutilisée, et les compteurs
`L4-retraits / hérités / sorties précoces`. Nomenclature « P1a » remplacée
par « self-join q2 » dans source et CMake.

**Les décisions sont inchangées partout** : états, prunées, microtuiles
identiques au triplet près sur chaque famille et chaque taille (le fate
ledger et la porte terrain 400 le vérifient) — un nœud `L4 >= 0` ne porte
aucun témoin, et les hérités appartiennent à l'ensemble découvrable du
fils. `22/22` CTests verts.

**Facteurs à n=2400 contre la table des auditeurs (sortie précoce seule)** :
visites de nœuds divisées par 8,8 (terrain), 10,6 (scanline), 7,0
(multi-écho), 3,6 (uniforme) ; tests ponctuels divisés par 16,5 / 21,6 /
11,3 / 5,6.

**Campagne d'échelle (5 000 → 50 000, quatre familles)** — compteurs
déterministes, ledger fermé partout :

| famille, n=50 000 | états | visites | tests ponctuels | résiduel | phase locale |
| --- | ---: | ---: | ---: | ---: | ---: |
| terrain | 710 396 | 240 347 699 | 495 522 203 | 0,50 % | 47,98 s |
| scanline simple | 367 890 | 53 240 637 | 86 172 879 | 0,29 % | 6,67 s |
| multi-écho | 950 500 | 393 107 357 | 801 949 159 | 0,65 % | 80,74 s |
| uniforme | 1 580 440 | 723 579 105 | 1 364 858 170 | 1,19 % | 120,3 s |

**Exposants des visites par doublement** (5k→12,5k→25k→50k, base
log(12500/5000) pour le premier) : terrain 1,38 / 1,81 / 1,67 ; scanline
0,90 / 1,17 / 1,27 ; multi-écho ~1,3 / 1,9 / 1,4 ; uniforme 1,05 / 1,31 /
1,22. **Le seuil de revue des auditeurs (deux exposants consécutifs
au-dessus de 1,35) MORD sur terrain et multi-écho** — la revue est due
avant tout port CUDA de cette route ; scanline et uniforme restent sous le
seuil. La comparaison Yao48/LBVH (point 5 de l'audit épinglé) reste à
faire.

**Une violation d'identité FANTÔME trouvée et fermée** : à
n=12 500/multi-écho/coord 707, `make_family_cloud` rend 12 501 points pour
12 500 demandés (écho de recouvrement) ; le ledger comparait `C(12500,2)`
au parcours du nuage réel. La machine était exacte ; le correctif fait
porter le ledger sur le nuage RENDU et publie les deux tailles. Rerun
fermé, gravé dans le reçu.

## 2. Falsificateur d'ancres q3/q4 — tranche cœur universel

`prototype/pair_anchor_probe.cpp` (`mhgp3v_pair_anchor_probe`), suivant la
porte en cinq points de
[`NOTE_COEUR_UNIVERSEL_JUNG_ANCRES_Q3_Q4_20260811.md`](NOTE_COEUR_UNIVERSEL_JUNG_ANCRES_Q3_Q4_20260811.md) :

- prédicat polynomial exact (`g>0` puis `3g^2>4Q` / `g^2>2Q`, i128) ;
  pré-prune de boule inscrite par intervalles (q3 strict, q4 LARGE licite) ;
  test des HUIT COINS contre une paire exacte (convexité du spindle) ;
  résolution terminale par paire ; sorts et ledgers `prunées + résiduelles
  = C(n,2)` INDÉPENDANTS par lane ; `--mode core` seul, `depth`/`combined`
  refusent explicitement (la profondeur fermée est la tranche suivante).
- ORACLE EXHAUSTIF `n<=32` : miniboule de chaque tuple, support propre
  positif ssi `n_support == q`, census `p`, non-inerte ssi `p < K+2-q`,
  ancre canonique (plus petite paire des arêtes maximales) exigée
  RÉSIDUELLE ; chaque prune conserve plages et témoins, l'oracle REJOUE
  chaque certificat au prédicat nominal (le juge n'hérite jamais des
  injections du sujet).
- SEPT FIXTURES gravées : `thin-acute` (triangle acutangle mince sur le
  cercle de rayon 1025 — l'arête courte est prunée, les arêtes maximales ex
  æquo survivent, l'ancre canonique est (0,2)) ; `contact-q3`/`contact-q4`
  (les contacts EXACTS de la note auditeur, `3g^2=4Q` et `g^2=2Q` vérifiés
  — strict refuse, large compterait) ; `nine/eight/seven-axial` (les
  fixtures axiales de la réponse d'audit : 9 prune q3 ET q4, 8 = q3
  résiduelle/q4 prunée, 7 = q4 résiduelle) ; `q4-tetra` (tétraèdre régulier
  non inerte, six arêtes ex æquo, ancre (0,1) résiduelle).
- HUIT MUTANTS à code exact 4 : `gt-to-ge` (×2, tués par les contacts),
  `threshold-minus-one` (×2, tués par le REJEU des certificats — le juge
  exige le seuil nominal), `witness-duplicated`, `last-block-omitted`,
  `duplicate-compensated` (fate), `anchor-non-maximal` (tué par l'oracle
  exhaustif). Le mutant « plage recouvrant une extrémité » est MORT-NÉ
  (l'extrémité donne `g=0`, le prédicat strict la refuse déjà) et documenté
  dans le source. La fixture tueuse d'`oracle-accept-nonpositive` est un
  TRAVAIL RESTANT : sur les tuples à boule diamétrale le mutant est
  structurellement invisible (les témoins universels sont dans la boule,
  donc `p >= t` et le tuple est inerte) ; la géométrie tueuse dérivée est
  un tuple q4 à `n_support=3` avec quatrième point à ~90 degrés — à graver.

`48/48` CTests (self-join + ancres). Premières masses (terrain 400, un
thread) : **q3 12 093 résiduelles (15,15 % de C(n,2)), q4 12 331
(15,45 %)** ; la boule inscrite ne mord presque pas au niveau des blocs (93
hits pour 3 802 états) — le travail est porté par les coins terminaux
(3,7 M de tests). Les deux accélérateurs suivants sont évidents : un
infimum de type L4 pour la lane d'ancres (« aucun témoin universel dans ce
nœud ») et le filtre terminal de profondeur fermée. Campagne
400/1200/2400 quatre familles en cours, reçu brut
`anchor_core_counters_raw.txt`.

## 3. Ce que cette livraison ne prononce pas

Aucune admission : le résiduel d'ancres à 15 % de C(n,2) à n=400 n'est ni
une borne ni une tendance ; les reporters médiateurs q3/q4, le census, les
`BallActivation` et le resolver restent les portes 5-6 de l'état courant.
La revue d'exposants q2 (terrain, multi-écho au-dessus de 1,35) est DUE
avant tout port CUDA de la route self-join.

GCP non utilisé pour cette livraison.
