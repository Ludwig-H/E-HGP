# Audit développeur de la v11, ports de la v10 et tranche 3 de performance

3 octobre 2026, soir (Claude, développeur). Cadre :
`exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`. Source auditée :
`a45daff3a` (moteur qualifié `c40f40798` et ses reçus). Demande de l'utilisateur : « Regarde tout ce qui a été fait
et audite à fond la v11 de Morse HGP 3D. Vérifie si des choses de la v10 ne pourraient pas améliorer la v11. Feu vert
pour des tests GCP G4. Essaie de faire en sorte de respecter le contrat 200ms à K5 sur des LiDAR sans sol. »

**GCP utilisé** : sessions gardées `v11_session.py` en `dev_snapshot`, VM `g4-standard-48`, verrou commun avec
l'auditeur ; chaque session close par un arrêt ciblé certifié (`targeted_shutdown_certified: true`, état
`TERMINATED` relu). Reçus : [pipeline_g4](../receipts/developpement_20261003/pipeline_g4/README.md).

## 1. Verdict

- **Exactitude** : aucun défaut établi dans les chemins lus (catalogue, census, MEB, descentes, table de populations,
  forêts, verticales). Les constats ci-dessous portent sur la structure, les défauts d'options et le coût.
- **Performance** : la tranche 3 ramène FULL K = 1..5, W48, mode 16379, de 447 / 440 / 440 ms (base `a45daff3a`)
  à 412 / 352 / 381 ms (qualification `claudeab7` : médianes de cinq prises appariées sur 08/000000, 08/000100,
  08/000200 ; sorties identiques octet pour octet à chacune des 36 prises). Les forêts gagnent 38 à 76 ms. **Le
  contrat de 200 ms n'est pas atteint** et ne peut pas l'être sans refonte du catalogue : le domaine seul prend 200 à
  255 ms à W48 (§ 6).

## 2. Où passe le temps (profil G4, base)

G4 : AMD EPYC 9B45, 24 cœurs × 2 SMT, un nœud NUMA. Sur 08/000000, mode 16379 : W1 10,46 s, dont domaine 5,49 s
(passe unique du catalogue 4,86 s) et forêts 4,97 s (résolution régulière 4,63 s, publication 0,13 s, verticales
0,07 s). À W48 : domaine ~230–255 ms (passe unique 175–200), forêts ~210–245 ms (régulière ~100, publication
d'ordre 5 ~45, balayage d'ordre 5 ~35–45, naissances ~14). Le CPU total à W48 (13,4 s) correspond à un parallélisme
moyen de ~29 : la passe unique est **au plafond SMT** (accélération W1→W48 ≈ 25–27 sur 24 cœurs) ; seule une baisse
de son travail CPU la raccourcit. Profil `perf` à W1 (part du CPU) : `extend` 14,5 %, `resolve_job` 7,6 %, filtre G1
6,9 %, `enumerate_leaf` 6,6 %, `side` 6,0 %, `PopulationLookup::find` 5,1 %, `center_line_meets` 4,6 %,
`power_bound_signs` 4,5 %. Les variantes `-march=x86-64-v3/v4` ne gagnent rien (sorties identiques).

## 3. Constats d'audit

| # | Constat | Effet | Suite proposée |
|---|---|---|---|
| A1 | La voie qualifiée rapide (mode 16379 : treize options) est entièrement opt-in ; `FullParams{}` reste la voie lente. | Le « produit » par défaut est 2 à 3 fois plus lent que la voie mesurée. | Un préréglage nommé, devenu le défaut après qualification ; la voie séquentielle reste la référence différentielle. |
| A2 | 14 bits d'options (16 384 combinaisons), dont des voies sans usage mesuré (mémos de lane : 2,6 % de succès ; lots sans ordres concurrents). | Chaque combinaison est un même-objet à garder vrai ; les portes n'en couvrent qu'une partie. | Retirer les voies dominées une fois le préréglage qualifié ; garder séquentiel + voie rapide. |
| A3 | 44 documents dans `docs/`, dont une note par optimisation et par banc ; l'état courant est réparti entre README, DEVELOPPEMENT, PERFORMANCE_FULL et les notes d'audit. | Lecture coûteuse, risque de contradiction. | Consolider : ARCHITECTURE, MATHEMATIQUES, PERFORMANCE, un état courant ; archiver le reste. |
| A4 | Les compteurs census dépendent de l'espace (réutilisé : une passe ; possédé : deux). | Ledgers non comparables entre W ou options sans exclure census. | Documenté ; la porte du pipeline compare les ledgers hors census. |
| A5 | Le différentiel canonique v10/v11 sur trames LiDAR entières reste ouvert (relevé par l'auditeur). | Seul témoin indépendant à l'échelle. | À faire avant toute revendication. |
| A6 | Chaque census repart de la racine de l'index (BVH aux boîtes chevauchantes) : ~30 bornes par requête. | ~1 µs par census, ~291 000 census à K = 5. | Pas de raccourci sûr : une boîte qui contient la boule ne garantit pas que les autres sites en sont exclus. |

## 4. Ports de la v10 examinés

| Levier v10 | État v11 | Décision et mesure |
|---|---|---|
| Semis H_K : population → naissance, sans relecture de niveau | Table I ∪ U → boule, puis `birth_node`, comparaison exacte des niveaux, rang du nœud | **Porté** (voie liée, § 5) : résolution régulière à W1 −26 à −30 % (4,63 → 3,41 s sur 08/000000). |
| Recensement filtré en flottant (`SiteTree::filtered`) | Bornes et côtés exacts natifs i128 | **Essayé puis retiré** : filtre F6 (§ 4 d'ARCHITECTURE) des signes de `power` et de ses bornes, conforme et tué par quatre mutants, mais gain ≈ 1 % du CPU ; la voie i128 native coûte déjà peu, le coût vient des accès et des branches. |
| MEB double Welzl + certification exacte | Énumération exacte bornée (≤ 716 candidats) | Non porté : `bounded_meb` ≈ 1 % du CPU. |
| Atlas : mémo partagé par cellule (149 000 succès sur 1,72 M pas à K = 5 dans la v10) | Mémos de lane (2,6 % de succès), désactivés en 16379 | Non porté : ~0,57 M pas non terminaux à K = 5 ; gain borné, à remesurer. |
| Kruskal par lots + pointeurs de saut pour les verticales | Publication DSU par plateaux, balayage fermé | Non porté tel quel : des requêtes par sauts sont des accès aléatoires ; remplacé par le recouvrement (§ 5). |
| Générateur : feuille J3, réordonnancement des quadruplets, `-march` | Graphe de paires, lignes vivantes, G3 avant J2 | `-march` mesuré sans gain ; le CPU de la passe unique v11 (4,9 s) est du même ordre que l'étage des boîtes de la v10. |

## 5. Tranche 3 livrée

Détails : [PERFORMANCE_FULL.md, tranche 3](../docs/PERFORMANCE_FULL.md). Aucune décision ne change ; forêts et
verticales identiques octet pour octet.

1. **Voie liée de la table de populations** : la ligne porte rang et naissance ; liaison contrôlée après les
   naissances ; la trace triée par fusion trouve (nœud, rang) en une case et une ligne ; rang < rang de la cellule
   remplace la comparaison exacte des niveaux (rangs denses des niveaux distincts).
2. **Naissances par blocs** : positions par sommes préfixes de la classification ; cohortes triées par centre ;
   table dense par blocs ; ordre 1 trié par `std::sort` (ordre strict, même permutation).
3. **Pipeline** : résolution par blocs dans l'ordre global des boules, publication de chaque ordre bloc par bloc,
   balayages verticaux qui suivent les deux publications ; attentes bloquantes (`std::atomic::wait`) ; décision du
   balayage par une fonction pure jugée contre le modèle séquentiel ; aucun interblocage quel que soit W (réclamation
   par indice croissant, seules des tâches d'indice supérieur attendent).

Portes (qualification `claudeab7`, verdict `conforme`) : suite `fast` 666/666, TSan sur
`mhgp11_tower_(pipeline|population_concurrent)` 7/7, onze mutants nouveaux ou réancrés tous tués, identité du dump
de chacune des 36 prises. Relecture de l'auditeur prise en compte : le tri des blocs
se fait en place (pas de tampon hors `MemoryBudget`), et le banc rend un verdict (codes, cardinalités, statuts,
identité de chaque prise, quiescence des groupes de processus).

## 6. Le contrat de 200 ms : ce qui manque

| Trame, W48, ms | Base | Tranche 3 | Domaine (tranche 3) | Forêts base → tranche 3 |
|---|---:|---:|---:|---:|
| 08/000000 | 446,5 | 412,4 | 253,0 | 209,2 → 171,0 |
| 08/000100 | 439,7 | 351,7 | 219,3 | 208,6 → 133,0 |
| 08/000200 | 440,4 | 380,7 | 222,2 | 204,5 → 157,7 |

(Qualification `claudeab7`, médianes de cinq prises alternées ; le domaine, inchangé, varie de ±30 ms entre prises.)

Les forêts sont désormais bornées par la résolution régulière (~85–120 ms avec 39 tâches) plus une queue de
publication de 20 à 35 ms. Le domaine reste à 200–255 ms, dont 150–200 ms de passe unique au plafond SMT. Pour
200 ms il faudrait, à W48 : catalogue ≤ ~100 ms (CPU de la passe unique divisé par deux au moins) et forêts ≤ ~80 ms.
Pistes, par rendement estimé :

1. Feuilles du catalogue : compteurs accumulés en registres et ajoutés une fois par feuille (≈ 300 M `checked_add`
   par trame dans `extend`), filtre G1 en binary64 exact (valeurs < 2^53, F2) vectorisable sans `-march`, pré-test
   flottant certifié du centre q3/q4 avant la construction exacte (10,3 M présentations q4, 3,3 M jugées).
2. Planification de la frontière (~20 ms à W48, rondes du haut presque sérielles) : filtre d'un gros nœud partagé
   entre workers.
3. Forêts : moins de pas non terminaux (mémo partagé par cellule à la v10), queue de publication de l'ordre 5.
4. Au-delà : catalogue sur GPU (la G4 en porte un), hors du cadre `cpu_reference` actuel.
