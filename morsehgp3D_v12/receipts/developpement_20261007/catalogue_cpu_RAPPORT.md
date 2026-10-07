# Rapport — tranche T1, première partie : module produit `catalogue` de la v12, voie CPU de référence

7 octobre 2026 (heures lues par `date -u`). Agent développeur du chantier v12, travail dans
`scratchpad/v12_catalogue/` (aucune écriture sous `/workspaces/E-HGP`, aucune commande git d'écriture, `-j4` au plus,
GCP non utilisé). Le codespace a redémarré à 16:05 (`uptime -s` : 16:05:44) : tout `/tmp` perdu ; reprise à 16:16 depuis
le HEAD `df9140b5d` (code de `src/` identique à `f4a11f49e`) ; v11 gelée, outil de vidage et vidages reconstruits de
16:17 à 16:30 ; sauvegarde légère au fil de l'eau dans `$HOME/v12_catalogue_sauvegarde/` (correctif courant,
outils, notes ; hors de `/tmp` et de `/workspaces/E-HGP`). Correctif : `patch_catalogue.diff` ; détail fichier par
fichier : `CHANGEMENTS.md` ; journal horodaté : `notes/JOURNAL.md`.

**Base du correctif livré : `main` au HEAD `76adb8fa9`.** Le HEAD a avancé pendant le travail (`df9140b5d`, puis
`1f7642e10`, puis `76adb8fa9`, commits datés 18:02:14 et 18:06:17) sans toucher `src/` hors d'un commentaire de `num/geometry.hpp` ; le correctif
a été rejoué sur une copie fraîche de `76adb8fa9` (`git archive`, dépôt jetable `fresh76/`), où il s'applique sans
conflit, et c'est là qu'ont été faites les dernières modifications (§ 4 : portes du différentiel basculées sur le
lecteur de transition général paru entre-temps ; nom de trame de l'export en ASCII imprimable, `CST-0227`) et la
validation finale (§ 8). Le HEAD a encore avancé à `576e7aaf9` (commit daté 18:31:42) en ne touchant que `microbancs/`,
`receipts/` et `audits/` (aucun fichier de `src/`, `cmake/`, `tests/`, `tools/`, `reference/` ni des contrats) :
`git -C /workspaces/E-HGP apply --check patch_catalogue.diff` passe sur `576e7aaf9` (18:42). Correctif : 37 fichiers,
6 000 lignes, SHA-256 `2572e42d…`.

```text
phase=exploration_v12_hors_registre
backend=cpu_reference (voie GPU du catalogue : à venir, même source)
objet=full_pi0 (étage C : Cat_K)
quantification=quantized_u21_input_only (profils 21, 24 et 32 construits et jugés localement)
public_status=not_claimed
GCP non utilisé
```

## 1. Résumé

| Exigence de la mission | Résultat |
| --- | --- |
| module `src/catalogue/` (en-tête public, table des modules) | construit aux profils 21, 24 et 32, `-Wall -Wextra -Wpedantic -Werror` sans avertissement, `style_ok fichiers=246` sur `76adb8fa9` (règle `[recus]` comprise) |
| parcours en largeur (source de MES-M5) sur le warp simulé, ordre parent de la v11 | porté ; parcours **identique** à la v11 (nœuds, feuilles, tests G1, profondeur, feuille maximale) sur les six cas et les témoins |
| feuilles J3 (source de MES-M2) en flux, par lots sur les fils, repère local et paliers ; repli exact | porté ; même source J3 jouée en arithmétique native (s ≤ 16) ou exacte plus large (s > 16), choisie avant de jouer la feuille ; feuilles de 33 à 256 sites par la même source sur un warp virtuel de 256 voies (§ 3.2) |
| support canonique par positions | fait (CST-0113) ; ordre des boules de même niveau par positions aussi |
| fin d'étage : ordre canonique, rangs, CSR, table $S^{*}\to$ boule | faite, sur l'hôte ; `find_support` |
| capacité : comptage avant réservation, domaines 32/64 bits, profondeur 3B, refus | faits (§ 3.4) ; refus `wide_leaf`, `shell_capacity` (WIT-SPHERE50), `multiplicity_unsupported`, paramètres, budget |
| export `MHGP12DP` v1 et sonde `mhgp12_catalogue_probe` | faits ; même format que les vidages de la v11 (`mhgp12_vidage`), lu par le lecteur de transition général (§ 4) ; empreinte canonique `catalogue_digest` |
| différentiel contre la v11 (ng00–02 à K5, uniformes 8 000 / 16 000 / 32 000) | **conforme** aux profils 21 et 32, d'abord par le lecteur provisoire, puis par le **lecteur de transition général** `reference/transition_catalogue.py` (commit `76adb8fa9`), qui juge désormais les portes ; ng02 : 2 changements de $S^{*}$, tous deux exigés par la convention ; **vingt compteurs logiques égaux** sur les six cas ; **ng00 à K10 aussi** (5 512 670 boules, 1 changement de $S^{*}$, compteurs égaux) |
| oracle borné (suite rapide de `hgp12_ref`) | **conforme** : 342 nuages, 6 988 boules, 34 nuages à doublons refusés |
| témoins | `WIT-FEUILLES`, `WIT-SPHERE50`, `WIT-T1-CARRE`, `WIT-TRANSL`, rectangle des deux conventions, profondeurs 60 et 63, boîte fermée à $2^{32}$, paliers étroit / moyen / large : **conformes** |
| filets Euler à K+2 et J1 | **conformes** sur ng00 et les trois uniformes, mêmes comptes que le juge de la v11 |
| déterminisme 1, 4, 8 fils | **mêmes octets** (empreinte de l'export) sur les trames, les uniformes et les témoins |
| mutants causaux | **6/6 tués** par le code de leur porte (§ 7) |
| échelle 8 000 / 16 000 / 32 000 | comptes et compteurs égaux à ceux de la v11, jamais un juge exhaustif |
| mesure locale | voir § 9 (indicative, aucune décision) |

## 2. Ce qui est construit

Un seul chemin produit : `build_catalogue(cloud, params, budget, pool)`.

1. **Parcours des boîtes en largeur** (`traversal_*.hpp`, `traversal.cpp`) : le texte de `bfs.hpp` de MES-M5 (noyaux
   Select, Merge, Filter, Close, ScanA/B/C, Scatter, Emit, sans mutants), joué sur l'hôte par le warp simulé de
   `simt.hpp` ; un noyau est une boucle sur ses warps, répartis sur le Pool par tranches fixes ; chaque warp n'écrit que
   ses sorties (aucun atomique), d'où l'indépendance au nombre de fils. Filtre G1 d'un enfant dans le repère du
   **parent** (`CST-0112`), voie `i64` si $2s+4\leq 63$, `i128` au-delà (`CST-0208`), boîtes fermées jusqu'à
   $2^{32}$ en `i64` (`CST-0204`), profondeur au plus $3B$ (`CST-0205`). Tous les tableaux du front sont des `Buffer`
   du budget. Ordre des sites : celui du nuage (clé de Morton exacte sur coordonnées absolues), donc la liste parente de
   la v11 : le parcours rend les **mêmes feuilles** que la v11 (grand livre égal sur tous les cas).
2. **Feuilles en flux** (`leaves.cpp`) : à la fin de chaque niveau du parcours, ses feuilles sont consommées par lots
   d'au plus 16 384, répartis sur le Pool ; l'arène des feuilles est réutilisée d'un niveau à l'autre. Un lot :
   comptage (feuille J3, case de 64 émissions par feuille), issues fusionnées, décalages contrôlés, réservation exacte des
   boules et incidences du lot, écriture (copie des cases, rejeu des feuilles qui débordent leur case, par la même source
   et la même arithmétique), contrôle que le rejeu rend les mêmes comptes. Les compteurs logiques d'une feuille sont
   pris au comptage, une fois, après succès, par une somme contrôlée.
3. **Feuille J3** (`leaf_*.hpp`) : le texte de MES-M2 (`leaf_common.hpp`, `leaf_j3.hpp`, `predicates.hpp`), rendu
   générique en deux paramètres : la **politique arithmétique** (`Narrow` : repère d'étendue $s\leq 16$, types natifs ;
   `Exact` : toute étendue, entier exact de 320 bits) et la **largeur du warp** (32, ou 256 pour les feuilles de plus de
   32 sites, hôte seulement). Repère d'une feuille : fermeture de sa boîte et tous les sites de sa liste
   (`NUM-COUVERTURE`, calculé par `num::Frame`) ; coordonnées locales. Le choix (politique, largeur) est fait avant de
   jouer la feuille, uniforme sur la feuille : sur l'hôte, aucune feuille n'est « non résolue ».
4. **Support canonique** (`leaf_census.hpp`) : cardinal minimal, puis plus petite liste triée des positions ; la coquille
   est rangée par coordonnées et les supports sont énumérés dans l'ordre lexicographique des indices de cette liste
   (paires, triangles strictement aigus coplanaires au centre, tétraèdres contenant strictement le centre) : le premier
   trouvé est le minimum. L'émission n'a lieu que depuis la présentation égale à $S^{*}$.
5. **Fin d'étage** (`assemble.cpp`, `sort.cpp`, `table.cpp`) : boules des lots mises à plat, niveaux exacts
   (`num::Sphere::through` de $S^{*}$, même arité que la présentation émettrice), clés F3, tri parallèle (niveau exact par
   F4 puis repli exact, puis $S^{*}$ par positions), rangs denses (niveau d'un rang : celui de sa première boule), CSR
   I puis U, table $S^{*}\to$ boule (CSR par premier site, dichotomie), refus d'un doublon d'émission.
6. **Export** (`export.cpp`) : MHGP12DP v1 genre catalogue sur `io::FileWriter` (dossier transactionnel du socle) ;
   nom de trame d'au plus 23 octets ASCII imprimables, sinon `parameter_out_of_range` (le lecteur de transition refuse
   tout autre nom depuis `CST-0227`) ; `catalogue_digest` hache les mêmes octets ; la sonde `mhgp12_catalogue_probe` (`bench/catalogue_probe.cpp`) lit
   `u32le` + `ids.u32le` (ou un nuage synthétique `--uniform=N,GRAINE,BITS`), écrit une ligne JSON par passe (statut,
   comptes, grand livre, diagnostics physiques, durée) et exporte hors du chemin chronométré.

## 3. Décisions prises dans la latitude du contrat, à confirmer par les auditeurs

### 3.1 Deux politiques arithmétiques de feuille, pas trois

Le contrat numérique propose trois paliers (étroit $s\leq 16$, moyen $s\leq 24$, large). La feuille n'en a que deux
instanciations : `Narrow` pour $s\leq 16$ (toutes les expressions natives, bornes prouvées dans `leaf_arith.hpp` :
orientation avec centre $7s+9=121$ bits, côté $6s+8=104$, centre q3 $5s+5=85$, `t` du q3 et déterminant q4 en `i64`
jusqu'à $s=19$), et `Exact` (320 bits, drapeau de dépassement collant, lu en fin de prédicat) pour tout $s>16$, paliers
moyen et large confondus. Raison : une seule implantation de plus, rare sur les données (0 à 3 feuilles à $s=17$ par
trame à K5, ce que MES-S avait mesuré), et aucune voie « contrôlée » à valider. Les compteurs physiques publient le
palier de chaque feuille (`leaves_narrow`, `leaves_medium`, `leaves_wide`, `leaves_exact`). Les budgets du palier moyen
(centres natifs, côté contrôlé ou certifié) restent disponibles pour la voie GPU si la mesure le demande.

### 3.2 Feuilles de plus de 32 sites : même source J3, warp virtuel de 256 voies

La feuille J3 de MES-M2 n'admet que $m\leq 32$ (une voie par site, masques `u32`) ; la v11 admettait `max_leaf` = 256
et jouait ces feuilles par le DFS historique de `leaf.cpp`. Une feuille de plus de 32 sites n'existe que si sa boîte
ne se coupe plus (largeur au plus 1) alors que plus de 32 sites restent K-certifiés : cosphéricités massives. **Témoin
exact** : la coquille de 48 sites de `CST-0205` (permutations signées de (1,2,3), échelle $2^{18}$, K5, feuille 24),
témoin de profondeur 63 du contrat lui-même, a huit feuilles de 48 sites. Refuser ces feuilles aurait perdu un témoin
du contrat et rendu la v12 incomplète là où la v11 répond. Décision : la même source J3, avec une largeur de warp N
en paramètre (N = 256 sur l'hôte seulement, masques de quatre mots, un site par voie, indices de file sur 8 bits,
rangs locaux sur 16 bits) ; sur l'appareil, une feuille de plus de 32 sites sera rejouée par l'hôte avant admission
(`static_assert` dans `run_leaf`). Les **compteurs** de ces feuilles reproduisent ceux de la v11, champ par champ : la
v11 y joue le DFS historique (sans graphe de paires ni cache J2) ; ses enfants visités sont tous les sites au-dessus du
dernier, testés contre le préfixe jusqu'au premier couple dominant, et chaque demande de droite est une évaluation et
un repli. Sur les mêmes préfixes développés (l'ensemble des présentations jugées est le même dans les deux voies), cela
donne une forme close (`children` dans `leaf_common.hpp`) : `prefixes` = $m+\sum_{P}\lvert\lbrace x>\ell(P)\rbrace\rvert$,
`region_pair_tests` = $\sum_P\sum_{j<\lvert P\rvert}\lvert\lbrace x>\ell(P)\rbrace\cap N_j(P)\rvert$ ($N_j$ : voisins communs des $j$
premiers sites), `region_pair_rejects` = $\sum_P\lvert\lbrace x>\ell(P)\rbrace\setminus N_{\lvert P\rvert}(P)\rvert$,
`region_line_evaluations` = `region_line_fallbacks` = tous les tests de droites, `region_line_cache_hits` = 0.
**Vérifié** sur la coquille de 48 sites : les vingt compteurs égaux à ceux de la v11 (4 955 680 tests de couples,
4 808 288 replis), catalogue identique (91 changements de $S^{*}$, tous vérifiés en exact).

### 3.3 Plafond déclaré de la coquille : 64 sites

Le contrat demande un refus explicite de `WIT-SPHERE50` (« coquille étendue au-delà du plafond déclaré »). Plafond
choisi : **64 sites**, la largeur des masques de traces de la tour (la v11 refusait ses vidages au-delà de 64, son
module `supports` au-delà de 24). Une boule **émise** dont la coquille dépasse 64 sites rend `shell_capacity`
(`unsupported_degeneracy`) et rien n'est publié. `WIT-SPHERE50` (84 sites) est refusé ; la coquille de 48 sites passe.
C'est un écart déclaré avec le catalogue de la v11, qui calculait `WIT-SPHERE50` (435 boules à K2).

### 3.4 Capacité et refus

- comptage exact par lot avant réservation, sommes et décalages contrôlés ; tableaux du front, lots, fin d'étage dans le
  budget ; un refus (y compris `memory_budget` en cours de parcours) ne publie rien (`Result<Catalogue>` entier ou
  refus ; porte `refusals` : budget à la moitié du pic, réservations rendues) ;
- domaines : sites par le nuage (refus à `kNone`) ; boules sur 32 bits, refus `index_overflow_u32` à la vraie limite ;
  tâches et enfants d'un niveau sur 32 bits (refus) ; décalages, compteurs et nombres de feuilles en 64 bits ; somme des
  compteurs contrôlée (`catalogue_counter_overflow`) ;
- profondeur : refus `catalogue_invariant` au-delà de $3B$ (impossible par le potentiel) ; **pas de quota de nœuds**
  séparé : le nombre de nœuds d'un niveau est borné par les indices 32 bits et par le budget des tableaux du front
  (l'option `max_nodes` de la v11 n'est pas reprise : un seul chemin, aucune option) ;
- refus explicites : paramètres (`kmax_out_of_range`, `parameter_out_of_range`), multiplicités
  (`multiplicity_unsupported`, D8 ; l'option « sites distincts » relève du nuage et n'est pas faite ici), feuille au-delà
  de `max_leaf` (`wide_leaf`), coquille (`shell_capacity`).

### 3.5 Ordre publié des boules de même niveau

Les boules de même niveau sont rangées par $S^{*}$ comparé par positions (règle 5 de l'architecture), pas seulement
$S^{*}$ choisi par positions : `WIT-TRANSL` (triangle de l'auditeur Codex) le grave, AB, AC, BC au lieu de BC, AC, AB.
Les rangs de niveau ne changent pas. Les deux lecteurs (§ 4) comparent donc les boules par identité, jamais par
position dans la liste ; le lecteur général vérifie cet ordre (listes triées des positions de $S^{*}$, règle que
`CONTRAT_CATALOGUE.md` impose depuis à l'exportateur ; le bourrage d'un $S^{*}$ plus court y est sans effet, aucun
$S^{*}$ n'étant préfixe d'un autre à niveau égal).

## 4. Différentiel contre la v11 gelée (K5 et K10, feuille 24)

Vidages de la v11 par `mhgp12_vidage` (`microbancs/mes_m3_m4_tour`, v11 `ac081a06f` construite depuis l'archive
`v11_src_ac081a06f.tar.gz` par `microbancs/outils/source_v11.py` : SHA-256 de l'archive `6f3454ad…`, `libmhgp11.a`
`050532a9…`, identique à celle des microbancs), entrées `build/v12-data-20261007/g4data_v12_t0/`, trame nommée par le
cas (`ng00`, …, `u32000`). Les vingt compteurs logiques sont comparés à ceux de la v11 sur la même entrée
(`build_catalogue` de la v11 au masque 802811 : feuille 24, `max_leaf` 256, cache J2, graphe de paires), gravés dans
`tests/catalogue/v11_counts.json`.

Deux lecteurs, dans l'ordre du travail :

1. **lecteur provisoire** (`tests/catalogue/diff_v11.py`, écrit pour cette tranche faute de lecteur général) : mêmes
   sites (SITEXYZ identique : même ordre de Morton absolu), bijection des boules (même $S^{*}$, sinon centre exact et
   rayon carré), rang, p, m, q_min, I, U égaux, chaque changement de $S^{*}$ vérifié (même boule, même cardinal minimal,
   chacun minimum de sa convention), ordre publié de la v12 ; conforme sur tous les cas ci-dessous aux profils 21 et 32
   (16:41, 16:59), puis sur ng00 à K10 (18:05) ;
2. **lecteur de transition général** `reference/transition_catalogue.py`, paru sur `main` pendant le travail (copie
   du commit `76adb8fa9`, SHA-256 `09ead04b…`, après `CST-0227`) : il juge chaque vidage dans sa convention (support
   minimal, populations, admission, rangs, ordre, règle de convention sur **toute** coquille étendue) puis la bijection
   par centre exact et rayon carré. **Conforme sur tous les cas** (18:08–18:18, tableau). Les portes
   `mhgp12_catalogue_diff_v11_*` passent désormais par lui (`diff_case.py` : la sonde exporte, le lecteur juge ; ligne
   attendue = sa ligne de conformité, comptes gravés) ; le lecteur provisoire est retiré du correctif (il ne reste de
   lui que le lecteur MHGP12DP des portes oracle et Euler, `catalogue_dump.py`).

| Cas | Sites | Boules | Incidences | Niveaux | Coquilles étendues | Supports multiples | $S^{*}$ changés (= exigés par la convention) | Renumérotées | Compteurs (20) | Lecteur général, durée |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- | ---: |
| ng00 K5 | 39 885 | 1 306 696 | 6 097 121 | 1 085 776 | 227 | 3 | 0 | 170 938 | égaux | conforme, 43 s |
| ng01 K5 | 35 551 | 1 095 926 | 5 085 683 | 941 217 | 135 | 1 | 0 | 131 082 | égaux | conforme, 36 s |
| ng02 K5 | 45 845 | 1 407 885 | 6 514 697 | 1 099 582 | 572 | 8 | 2 | 212 365 | égaux | conforme, 48 s |
| uniforme 8 000 K5 | 8 000 | 597 998 | 2 895 136 | 597 987 | 0 | 0 | 0 | 12 | égaux | conforme, 29 s |
| uniforme 16 000 K5 | 16 000 | 1 233 046 | 5 979 160 | 1 232 923 | 0 | 0 | 0 | 136 | égaux | conforme, 68 s |
| uniforme 32 000 K5 | 32 000 | 2 536 732 | 12 316 439 | 2 535 983 | 0 | 0 | 0 | 652 | égaux | conforme, 143 s |
| ng00 K5, profil 32 | 39 885 | 1 306 696 | 6 097 121 | 1 085 776 | 227 | 3 | 0 | 170 938 | égaux | conforme, 55 s |
| **ng00 K10** | 39 885 | **5 512 670** | **45 383 538** | **5 085 986** | 444 | 7 | 1 | 327 964 | égaux | conforme, 241 s |

« Renumérotées » : boules dont l'indice change entre les deux vidages (ordre des boules de même niveau par positions,
§ 3.5), jamais un écart. Les exports des profils 21 et 32 ne diffèrent que d'un octet sur chacun des six cas (`cmp -l`) :
les bits de coordonnées de l'en-tête (21 contre 32) ; même ordre des sites, mêmes boules, même ordre publié. ng00 à K10 (feuille 24) : v12 13,8 s, v11 9,5 s à 4 fils (indicatif) ; vidage de la v11 de
402 519 776 octets, identique en taille à l'export de la v12.

Grand livre de ng00 (égal à la v11) : 272 249 nœuds, 123 581 feuilles, 248 216 364 tests G1, profondeur 33, feuille
maximale 24 ; `dominance_tests` 23 172 225, `prefixes` 174 386 434, `judged` 5 302 831, `census_tests` 71 766 408,
`q4_candidates` 22 149 226, `q4_levels` 158 494, `region_line_tests` 133 118 853, `region_line_rejects` 26 845 459,
évaluations 38 074 969, succès du cache 95 043 884.

Témoins synthétiques vidés aussi par la v11 (les deux lecteurs) : coquille de 24 sites K2/16 (99 boules, 7 coquilles
étendues, 7 changements de $S^{*}$), coquille de 48 sites K5/24 (639 boules, 183 coquilles étendues à plusieurs
supports, 91 changements), cube et centre K5/8 (`WIT-FEUILLES`, 47 boules, 7 coquilles étendues, 0 changement) :
conformes. `WIT-SPHERE50` : la v11 calcule 435 boules à K2 (son vidage refuse ensuite la coquille de 84 sites,
au-delà de 64) ; la v12 refuse (`shell_capacity`, § 3.3).

## 5. Oracle borné

`tests/catalogue/oracle.py` (porte `mhgp12_catalogue_oracle`, et sa jumelle sous `python3 -O`) : pour chaque nuage de
`families.fast_suite()` de `reference/hgp12_ref` ($n\leq 14$), la sonde calcule Cat_K (K du nuage, feuille K+3, deux
fils) ; l'étage B de la référence (entiers Python, force brute, admission unique $p+q_{\min}\leq K+1$) donne ses boules
positives ; bijection par (centre exact, rayon carré), p, m, q_min, I, U comme ensembles de positions, $S^{*}$ égal au
minimum des positions calculé par la référence sur la coquille rangée par positions, rangs denses, nombre de niveaux,
ordre publié. **Résultat : 342 nuages, 6 988 boules conformes ; 34 nuages à doublons refusés (`multiplicity_unsupported`,
code 2).** Durée 2,5 s.

## 6. Témoins, filets, déterminisme, échelle

Porte `mhgp12_catalogue_unit` (12 tests et l'inventaire ; 118 contrôles au profil 21, 127 au profil 32 ; plancher de
chaque test atteint) :

| Test | Contenu |
| --- | --- |
| `witness_square` | `WIT-T1-CARRE` côté catalogue : la boule circonscrite (m = 4, q_min = 2), $S^{*}$ = diagonale de plus petites positions ; `find_support` la trouve, rend rien pour l'autre diagonale (support minimal non canonique) et pour un support non croissant, rend la boule d'un côté |
| `witness_rectangle` | rectangle (3,3,4), (3,4,3), (5,5,4), (5,4,5) : minimum de Morton {(3,4,3),(5,4,5)} ≠ minimum des positions {(3,3,4),(5,5,4)} ; la v12 publie le second |
| `witness_transl` | `WIT-TRANSL` : AB, AC, BC (niveau 1/2, rang 1) dans cet ordre, inchangé par translation |
| `witness_long_triangle` | triangle équilatéral (M,0,0), (0,M,0), (0,0,M), M = $2^{B}-1$ : 4 boules, une feuille d'étendue B + 1 jouée par la politique exacte (`leaves_exact` = 1) ; la fabrique q3 y calcule $t=2M^{3}\geq 2^{63}$ |
| `witness_sphere50` | `WIT-SPHERE50` : refus `shell_capacity`, statut `unsupported_degeneracy`, budget rendu |
| `witness_leaves` | `WIT-FEUILLES` (cube {0,16}^3 et centre, K5/8) : 80 feuilles pour 9 sites, grand livre complet égal à la v11 |
| `depth_witnesses` | `CST-0205` : coquille de 24 sites K2/16, profondeur 60 ; coquille de 48 sites K5/24, profondeur 63, huit feuilles de 48 sites sur le warp virtuel ; grands livres complets égaux à la v11 |
| `refusals` | `kmax_out_of_range` (0, 13), `parameter_out_of_range` (feuille < K+3, `max_leaf` > 256, feuille > `max_leaf`), `multiplicity_unsupported`, `wide_leaf` (coquille de 48 sites, `max_leaf` 8), `memory_budget` à la moitié du pic, réservations rendues ; nom de trame de l'export : octets 0x01, 0x7f, 0xff, NUL et 24 octets refusés (`parameter_out_of_range`), nom imprimable accepté |
| `counter_overflow` | somme contrôlée des compteurs à $2^{64}-1$ (`catalogue_counter_overflow`) |
| `tiers` | paliers : nuage de 40 sites (étendue ≤ 9, feuilles étroites), puis le même dilaté pour toucher les deux bords du domaine ($[0,2^{B}-1]$ par axe ; racine fermée à $2^{B}$, soit $2^{32}$ et $s=33$ au profil 32, `CST-0204`) : même catalogue à l'homothétie près ; feuilles exactes (palier moyen) au profil 21, palier large au profil 32, compteurs de palier non nuls |
| `wide_traversal` | profil 32 : coquille de 48 sites à l'échelle $2^{29}$ autour de $(2^{31})^{3}$ (fixture de MES-M5) : parcours égal à l'oracle indépendant de MES-M5 (1 479 nœuds, 504 feuilles, 880 393 tests G1, profondeur 96 = 3B, feuille 48), catalogue égal à celui du profil 21 à l'homothétie $2^{11}$ et la translation $2^{29}$ près ; profils 21 et 24 : refus du nuage hors domaine |
| `determinism` | mêmes octets à 1, 4 et 8 fils (empreinte de l'export, grand livre) : nuage de 2 500 sites, coquille de 48 sites |

Filets (`tests/catalogue/euler.py`, règles du juge de la v11 réécrites en Python exact ; jamais un certificat) :

| Entrée | Cat_K / Cat_{K+2} | Coquilles étendues | Euler (ordres 1 à 5) et J1 | Durée |
| --- | ---: | ---: | --- | ---: |
| ng00 K5 | 1 306 696 / 2 565 656 | 320 | conformes | 28 s |
| uniforme 8 000 | 597 998 / 1 301 414 | 0 | conformes | 12 s |
| uniforme 16 000 | 1 233 046 / 2 698 867 | 0 | conformes | 24 s |
| uniforme 32 000 | 2 536 732 / 5 578 606 | 1 | conformes | 60 s |
| synthétique 3 000 (recensement exact de chaque site listé) | 210 786 / 454 667 | 0 | conformes | 16 s |

Les comptes de Cat_{K+2} et des coquilles étendues sont ceux de la table du juge de la v11 (`CATALOGUE.md` de la v11).
Portes : `mhgp12_catalogue_euler_small` (fast), `_euler_scale8000`, `_euler_lidar_ng00_k5` (long).

Échelle (`ledger_gate.py`, invariants globaux) : nuages synthétiques de 8 000, 16 000 et 32 000 sites
(`--uniform=N,20261007,18`) : boules, incidences, niveaux et vingt compteurs égaux à ceux de la v11 sur la même entrée,
mêmes octets à 1, 4 et 8 fils (`mhgp12_catalogue_scale{8000,16000,32000}`, 9 s, 21 s, 45 s environ).

## 7. Mutants causaux

Manifeste `tests/mutants/catalogue.json` (plancher 6), lanceur du socle (`tests/mutants/run_mutants.py`, copies mutées
construites avec `-DMHGP12_MODULES=catalogue`, deux mutants à la fois, deux fils de construction chacun). Campagne
(17:38–17:42, base `df9140b5d`) : **6 mutants, 6 tués, tous par le code de leur porte** (aucun par signal, délai ou
construction) ; rejouée sur le correctif final (`fresh76/`, 18:38–18:42) : même résultat, 6/6 par code.

| Mutant | Faute | Porte qui le tue |
| --- | --- | --- |
| `temoin_perdu_du_reservoir` | réservoir de 3K − 1 témoins | `mhgp12_catalogue_unit_depth_witnesses` (grands livres gravés de la v11) |
| `g1_repere_enfant` | filtre G1 dans le repère de la seule boîte de l'enfant (profil 32 : au profil 21 la branche est équivalente, tout y est natif) | `mhgp12_catalogue_unit_wide_traversal` (profil 32 ; parcours de l'oracle de MES-M5) |
| `sstar_par_rang_de_morton` | $S^{*}$ départagé par les rangs locaux (convention de la v11) | `mhgp12_catalogue_unit_witness_rectangle` |
| `ordre_par_rang_de_morton` | boules de même niveau rangées par SiteIdx de $S^{*}$ | `mhgp12_catalogue_unit_witness_transl` |
| `feuille_large_sans_elargir` | feuille d'étendue > 16 jouée en arithmétique native | `mhgp12_catalogue_unit_witness_long_triangle` |
| `feuille_comptee_deux_fois` | compteurs logiques d'une feuille ajoutés deux fois | `mhgp12_catalogue_unit_witness_leaves` |

Première campagne (17:29–17:32), publiée telle quelle : 3 tués, 2 survivants, 1 invalide. `temoin_perdu_du_reservoir`
survivait à `WIT-FEUILLES` (9 sites, moins que 3K − 1 = 14 témoins : la faute y est sans effet) ; il est tué par les
coquilles de 24 et 48 sites. `g1_repere_enfant` ne se construisait pas (paramètre `p` inutilisé sous `-Werror`) ;
corrigé, il est tué au profil 32. `feuille_large_sans_elargir` survivait aux coquilles à l'étendue 21 et au nuage
dilaté du profil 21 : leurs intermédiaires ne dépassent pas les types natifs **en pratique** (les bornes du palier
étroit sont suffisantes, pas nécessaires) ; un témoin exact a été ajouté : le triangle équilatéral (M,0,0), (0,M,0),
(0,0,M), M = $2^{B}-1$, dont la fabrique q3 calcule $t=2M^{3}\geq 2^{63}$ (feuille d'étendue B + 1, repli exact).

Les deux mutants du contrat propres à l'appareil (« feuille non résolue admise sans rejeu », « un fil par feuille » qui
doit perdre son budget de temps) n'ont pas d'objet sur l'hôte, où aucune feuille n'est non résolue : ils viendront
avec la voie GPU.

## 8. Portes du socle, profils, sanitizers, style

Validation finale sur `76adb8fa9` (copie fraîche `fresh76/` + correctif final ; `MHGP12_DATA_DIR` posé, donc la
sentinelle LiDAR est jouée et non sautée) :

| Contrôle | Résultat |
| --- | --- |
| profil 21 : construction complète (aucun avertissement), `ctest -LE long --no-tests=error -j4` | **607/607** (18:29–18:33) ; unité du catalogue : 12 tests, 118 contrôles |
| profil 32 : idem | **607/607** (18:33–18:38) ; unité du catalogue : 12 tests, 127 contrôles |
| portes `lidar` du catalogue, profil 21 (`MHGP12_V11_CATALOGUE_DIR`) | **10/10** (18:23–18:29) : grands livres de la v11 et mêmes octets à 1, 4 et 8 fils sur ng00–02, Euler ng00, différentiel des six cas par le lecteur général |
| profil 24 (`-DMHGP12_MODULES=catalogue`), construction sans avertissement | **30/30** portes rapides (18:42–18:45) : unités, oracle et sa jumelle `-O`, échelle, Euler, style, manifeste des mutants |
| ASan + UBSan (`-fno-sanitize-recover=all`, profil 21, `-DMHGP12_MODULES=catalogue`) : 13 portes de l'unité, oracle (342 nuages, sonde sous sanitizers), Euler sur 3 000 sites, refus d'usage | **16/16** (18:45–18:46), aucun rapport de sanitizer |
| mutants (§ 7) | **6/6 tués par le code de leur porte** (18:38–18:42), plancher 6 |
| `tools/check_style.py --root morsehgp3D_v12` ; `tools/check_constats.py` | `style_ok fichiers=246` (règle `[recus]` comprise : aucune porte du catalogue ne lit un reçu) ; `OK: 66 constats` |
| table gravée des raisons (`tests/core/status_test.cpp`) | six raisons ajoutées en fin de table, plancher 65 → 83 |

Le correctif tel qu'il était avant la bascule du différentiel passait déjà 607/607 sur `76adb8fa9` au profil 21
(18:10–18:15). Sur l'ancienne base `df9140b5d` : 443/443 aux profils 21 et 32 (17:54–18:01), profil 24 30/30,
ASan + UBSan 16/16. Une première série complète au profil 21 (16:59) avait deux échecs, corrigés :
`mhgp12_core_unit_reasons` (la table gravée des raisons du test de `core` doit suivre `reasons.def`) et l'inventaire de
l'unité du catalogue (un test ajouté pendant la construction).

## 9. Mesure locale indicative (aucune décision)

Codespace partagé, 8 cœurs AMD EPYC, **chargé par un autre agent** (charge moyenne 6,3 à 8,5 pendant les prises, lue
par `uptime` au début et à la fin) ; Release u21, GCC 13.3 ; trois passes par prise dans un même processus (`--passes=3`
de la sonde ; trois appels de `build_catalogue` de la v11 gelée au masque 802811, feuille 24, par l'outil de travail
`v11_ledger`, non livré) ; mêmes entrées ; durées murales en ms (médiane, puis extrêmes). Ces chiffres ne décident rien :
la règle de `MES-P` et le budget de l'étage C se jugent sur G4 (référence de la v11 sur G4, 48 fils : `domain` de la
voie CPU 200 / 163 / 195 ms sur ng00 / ng01 / ng02 à K5, `MESURE.md` § 3.1 et § 3.2).

| Cas (K5, feuille 24) | Fils | v12, voie CPU | v11 gelée, voie CPU | Rapport des médianes |
| --- | ---: | ---: | ---: | ---: |
| ng00 | 1 | 13 887 (13 196–14 386) | 11 787 (10 678–11 902) | 1,18 |
| ng00 | 4 | 4 653 (4 391–4 788) | 3 842 (3 156–4 360) | 1,21 |
| ng00 | 8 | 3 790 (3 444–4 250) | 2 841 (2 465–3 027) | 1,33 |
| uniforme 8 000 | 1 | 3 789 (3 763–3 883) | 3 219 (3 192–3 374) | 1,18 |
| uniforme 8 000 | 4 | 1 583 (1 343–1 626) | 1 243 (1 151–1 250) | 1,27 |
| uniforme 8 000 | 8 | 1 157 (1 093–1 249) | 845 (789–885) | 1,37 |
| uniforme 16 000 | 1 | 8 804 (8 738–8 921) | 8 157 (6 986–8 991) | 1,08 |
| uniforme 16 000 | 4 | 5 157 (3 448–5 236) | 2 865 (2 784–3 151) | 1,80 |
| uniforme 16 000 | 8 | 2 733 (2 295–3 398) | 2 242 (2 188–2 578) | 1,22 |
| uniforme 32 000 | 1 | 18 575 (17 158–19 304) | 14 654 (14 455–14 711) | 1,27 |
| uniforme 32 000 | 4 | 5 644 (5 050–6 714) | 4 904 (4 734–5 312) | 1,15 |
| uniforme 32 000 | 8 | 6 013 (5 852–6 230) | 3 392 (3 181–3 699) | 1,77 |

Répartition de la voie CPU de la v12 sur ng00 (seconde passe d'un processus, 17:49) :

| Étape | 1 fil | 8 fils |
| --- | ---: | ---: |
| parcours en largeur (warp simulé) | 2 609 ms | 601 ms |
| comptage des feuilles (J3 simulée) | 8 209 ms | 1 709 ms |
| écriture (cases copiées ; 3 990 feuilles sur 123 581 rejouées) | 960 ms | 216 ms |
| niveaux exacts / tri / CSR et rangs / table $S^{*}\to$ boule | 250 / 410 / 284 / 63 ms | 45 / 73 / 227 / 63 ms |
| **total** | **12 817 ms** | **2 968 ms** |

Pic du budget : 308 Mo (ng00). Feuilles : 123 580 au palier étroit, 1 au palier moyen (repli exact), aucune au palier
large ; 34 niveaux. Lecture : à un fil la voie CPU de la v12 coûte 1,1 à 1,3 fois celle de la v11 (le microbanc MES-M2
annonçait 1,4 à 1,9 fois pour la seule feuille ; les cases de 64 émissions évitent presque tout rejeu à l'écriture) ;
l'écart grandit avec les fils sur cette machine chargée (assemblage en partie séquentiel, 0,23 s à 8 fils ; parcours
de 34 niveaux avec une synchronisation par noyau). Ce sont des pistes pour MES-P, pas des décisions.

## 10. Ce qui reste pour la voie GPU (même source)

- **Exécuteur CUDA du parcours** : le pilote de `traversal.cpp` est écrit contre les tableaux du front et le Pool ; le
  rendre générique en l'exécuteur (comme `Driver<B>` de MES-M5 : tableaux, `launch`, lecture des totaux) et reprendre
  l'exécuteur appareil de `cuda/traversal_bench.cu`. Les noyaux (`traversal_*.hpp`) sont déjà en source unique
  `MHGP12_HD`, collectives en code uniforme ; rien n'a été compilé par `nvcc` dans ce travail (aucun GPU local).
- **Noyau de feuille** : `run_leaf<32, Narrow>` (un warp par feuille), puits d'appareil (comptage, cases, rejeu des
  feuilles qui débordent) ; les feuilles de plus de 32 sites ou d'étendue supérieure à 16 sont classées avant le
  lancement et jouées par l'hôte (`run_leaf<256, …>` ou `run_leaf<32, Exact>`) **en parallèle**, avant admission
  (contrat R7, `CST-0009`) ; `static_assert` interdit déjà le warp de 256 voies sur l'appareil.
- **Fin d'étage sur l'appareil** (tri par base des clés F3, chaînes de voisins non certainement ordonnés résolues en
  exact, rangs, CSR, table $S^{*}\to$ boule), lots de feuilles résidents, réservations d'appareil comptées
  (`BudgetReservation`), Session résidente (MES-M6).
- **Portes à ajouter** : déterminisme octet pour octet CPU contre GPU (`catalogue_digest`), mutants GPU du contrat
  (« feuille non résolue admise sans rejeu », « un fil par feuille » qui doit perdre son budget de temps), Compute
  Sanitizer, mesure MES-P du seuil CPU/GPU des petits nuages, budget de l'étage C sur G4 (35 à 45 ms à K5 sur ng00).
- **Ce que la voie CPU fixe déjà pour la voie GPU** : ordre des sites, partition en feuilles, compteurs logiques,
  catalogue, export et empreinte : la voie GPU doit rendre les mêmes octets.

## 11. Points ouverts et questions

1. **Feuilles de plus de 32 sites** (§ 3.2) : la décision (même source J3 sur un warp virtuel de 256 voies, compteurs de
   la voie DFS de la v11 en forme close) est à confirmer ; l'alternative serait un plafond de 32 sites, qui refuserait
   la coquille de 48 sites de `CST-0205`.
2. **Plafond de coquille de 64 sites** (§ 3.3) : choix du développeur pour `WIT-SPHERE50` ; à inscrire au contrat si
   retenu (la v11 n'en avait pas dans son catalogue).
3. **Deux politiques au lieu de trois paliers** (§ 3.1) : à confirmer, ou à réviser par la mesure de la voie GPU.
4. **Quota de nœuds** : aucun quota séparé (§ 3.4) ; le contrat dit « nombre de nœuds budgété à part » : le budget des
   tableaux du front et les indices 32 bits en tiennent lieu ; à préciser.
5. **Lecteur de transition** : fait ; le lecteur général (`reference/transition_catalogue.py`, `76adb8fa9`) juge les
   six portes du différentiel et a rejoué tous les vidages de ce travail (§ 4) ; le lecteur provisoire est retiré. Les
   vidages de la v11 sont reproductibles par `mhgp12_vidage` (commandes au § 12), ceux de la v12 par la sonde.
6. **K10** : joué sur ng00 seulement, hors porte (catalogue et vingt compteurs égaux à la v11, lecteur général
   conforme, § 4) ; une porte K10 (vidage de 400 Mo, 4 min de lecteur) est laissée à la campagne G4.
7. **Option « sites distincts » (D8)** : non faite ; seul le refus par défaut l'est.
8. **Coût de la voie CPU** : 1,1 à 1,3 fois la voie CPU de la v11 à un fil sur cette machine ; la règle de `MES-P`
   tranchera (la contre-lecture de Codex conditionne l'adoption de la source J3 sur l'hôte à cette mesure).

Aucune contradiction mathématique rencontrée : aucune fixture de contradiction ni mise à jour du registre des preuves
n'est nécessaire. Aucun écart avec la v11 hors de la convention déclarée de $S^{*}$ et de l'ordre des boules de même
niveau.

## 12. Rejeu et livrables

Livrables dans `scratchpad/v12_catalogue/` : `patch_catalogue.diff` (base `76adb8fa9`, s'applique sur `576e7aaf9`),
`CHANGEMENTS.md`, ce `RAPPORT.md` ; `fresh76/` (copie fraîche de `76adb8fa9` avec le correctif, celle qui a été
validée) ; `notes/patch_catalogue_avant_bascule.diff` (version précédente, lecteur provisoire, pour mémoire) ;
`notes/` (journal, comptes de la v11, sorties des portes, mesures) ; `tools/` (scripts de travail : reconstruction de la
v11 et vidages, différentiel, mesures, sauvegarde ; `v11_ledger.cpp`, outil de travail lié à `libmhgp11.a`, non livré
dans le correctif). Aucune donnée SemanticKITTI ni vidage n'est dans le correctif ni dans ces fichiers de rapport (les
vidages sont restés dans `scratchpad/v12_catalogue/dumps/`, hors dépôt).

```bash
# depuis la racine du depot, apres git apply patch_catalogue.diff
cmake -S morsehgp3D_v12 -B <b> -DCMAKE_BUILD_TYPE=Release            # -DMHGP12_COORD_BITS=24 ou 32
cmake --build <b> -j4
ctest --test-dir <b> -LE long --no-tests=error -j4                     # portes rapides (catalogue compris)
# portes lidar du catalogue : donnees et vidages de la v11
export MHGP12_DATA_DIR=<dossier des lidar_ng0X.* et uniform_u18_n*.*>
python3 morsehgp3D_v12/microbancs/outils/source_v11.py --archive <data>/v11_src_ac081a06f.tar.gz \
    --sha256 6f3454ad9d9ad2f6bb0b846f1aaad7c1a4c509d14cd450a99381c72b04a57885 --dest <v11src> --build <v11build> \
    --jobs 4 --report <v11build>/source_v11.json
cmake -S morsehgp3D_v12/microbancs/mes_m3_m4_tour -B <tour> -DMHGP11_SOURCE=<v11src>/morsehgp3D_v11 \
    -DMHGP11_BUILD=<v11build> && cmake --build <tour> --target mhgp12_vidage -j4
<tour>/mhgp12_vidage $MHGP12_DATA_DIR/lidar_ng00.u32le $MHGP12_DATA_DIR/lidar_ng00.ids.u32le ng00 5 24 3 \
    <vidages>/ng00_k5 --journal aucun     # trame = nom du cas ; idem ng01, ng02, u8000, u16000, u32000 (uniform_u18_n*)
cmake -S morsehgp3D_v12 -B <b> -DMHGP12_V11_CATALOGUE_DIR=<vidages>
ctest --test-dir <b> -R catalogue -L lidar                             # 10 portes (dont 6 par le lecteur general)
# hors porte : ng00 a K10, puis le lecteur de transition general sur les deux vidages
<tour>/mhgp12_vidage $MHGP12_DATA_DIR/lidar_ng00.u32le $MHGP12_DATA_DIR/lidar_ng00.ids.u32le ng00 10 24 3 \
    <vidages>/ng00_k10 --journal aucun
<b>/mhgp12_catalogue_probe $MHGP12_DATA_DIR/lidar_ng00.u32le $MHGP12_DATA_DIR/lidar_ng00.ids.u32le --k=10 --leaf=24 \
    --threads=4 --frame=ng00 --out=<v12_k10>
python3 -S morsehgp3D_v12/reference/transition_catalogue.py <vidages>/ng00_k10/cat.bin <v12_k10>/cat.bin
python3 morsehgp3D_v12/tests/mutants/run_mutants.py --manifest morsehgp3D_v12/tests/mutants/catalogue.json \
    --source morsehgp3D_v12 --work <w> --jobs 2 --build-jobs 2 --cmake-arg=-DCMAKE_BUILD_TYPE=Release
```
