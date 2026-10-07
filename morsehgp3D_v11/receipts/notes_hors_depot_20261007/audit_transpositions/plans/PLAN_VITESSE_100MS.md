# Plan de vitesse de la v11 vers 100 ms (K = 5, et K = 10 si possible) sur G4

4 octobre 2026, rédigé de 13 h 23 à 13 h 43 UTC (heures lues par `date -u`). Synthèse de l'audit des
transpositions ([contexte](../CONTEXTE.md)) : deux cartes, douze fouilles et douze contre-vérifications adverses,
recoupées avec le code et les reçus. Rappel de l'utilisateur : « l'objectif du contrat est toujours 100 ms ».

```text
phase=exploration_v11_hors_registre (plan, lecture seule)
backend=cpu_reference
profile=quantized_u21_input_only
public_status=not_claimed
GCP non utilisé ; aucune construction ni exécution native ; aucune commande git qui écrit
```

Base lue : `origin/main` = **`56216392e`** (13 h 13 UTC, q3 différé porté, « G4 A/B to follow »), précédé de
**`17514012b`** (13 h 06 UTC, réponses R1–R7 des auditeurs aux sept verrous du développeur). Moteur chronométré :
sources **`b87285378`** (reçu [AB7]) ; dernier moteur pleinement qualifié : **`c40f40798`** (reçu [Q]). Seuls calculs
de ce plan : `python3 -B ../cartes/CARTE_V11_derive.py` rejoué (mêmes chiffres que la carte) et une relecture Python
des quinze prises W48 et des prises W1 de l'archive [AB7] (mêmes chiffres que `../verif/v7.md` § 1.3).

Étiquettes : **M** mesuré (reçu nommé ; M-loc = mesure locale, rapports seulement), **E** estimé (arithmétique sur
des mesures, base dite), **C** conjecturé. Aucun gain de ce plan n'est mesuré sur la v11 : chacun attend son A/B G4
à sorties identiques.

## Sources et abréviations

Chemins relatifs à `morsehgp3D_v11/` sur `origin/main` sauf mention ; rapports de l'audit relatifs à ce dossier.

| Abr. | Source |
| --- | --- |
| [AB7] | `receipts/developpement_20261003/pipeline_g4/sessions/claudeab7/results.tar.gz` (b872, mode 16379, K1..5, u21, cinq prises W48 alternées et une W1 par trame, processus neufs) ; `perf_new_self.stdout` (profil W1 de 08/000000) |
| [Q] | `receipts/qualification_performance_20261003/` (c40, 81 prises W1/W8/W48) |
| [PROF1] | `.../pipeline_g4/sessions/claudeprof1/results.tar.gz` (profils `perf` W1/W48 de la base `a45daff3a`) |
| [S4], [S1] | v10 `777406b82` : `morsehgp3D_v10/receipts/g4_session4_j2c_20260929/` (48 fils, troisième passe chaude), `g4_session1_20260929/` (tour à un fil) |
| [C11], [C10] | `../cartes/CARTE_V11.md`, `../cartes/CARTE_V10_VITESSE.md` |
| [F:x], [V:x] | `../fouille/x.md`, `../verif/x.md` (x = v10_moteur, v10_tour, v9, v8, v7, v6, v5, v4, v2_v3, auditeurs, produit, ehgp_zoltan) |
| [Q100] | `audits/QUESTION_CLAUDE_VITESSE_100MS_20261004.md` (sept verrous du développeur, `d597ed9ba`) |
| [R1]–[R7] | réponses des auditeurs, `audits/AUDIT_CONTRATS_NUMERIQUES_ET_CAPACITE_20261002.md` § « Réponses R1–R7 » (`17514012b`) |
| [CT], [PR], [CG] | `build/v11-persist/conception/CONCEPTION_TOUR.md`, `PISTES_DE_RUPTURE.md`, `CONCEPTION_GENERATEUR.md` (conceptions du 2 octobre) |
| [L05], [L06] | `build/v11-persist/audit_v10/L05_CODE_CATALOGUE.md`, `L06_CODE_TOUR.md` |
| [PTS4] | `receipts/pts4_review_20261003/case_metadata.json.gz` (127 trames c08 sans sol, K1..10, hors contrat) |

---

## 0. Résumé

1. **Mesuré aujourd'hui** (b872, W48, K = 5) : FULL **412 / 352 / 381 ms** (08/000000, 08/000100, 08/000200 ;
   meilleure prise 327,7 ms). Deux postes portent 70 % du temps : la **passe unique du catalogue** (167–180 ms) et la
   **résolution régulière** (86–116 ms) ; tout le reste pèse encore **98–116 ms** [AB7, M]. La v10 faisait
   204–254 ms en passe chaude [S4, M] ; l'écart (×1,50–1,72) est un **coût par unité de travail** (feuille ×1,27,
   pas de descente ×3,2 à un fil), à travail logique égal [C10, M].
2. **Budget cible** : 100 ms exigent ≤ 2,0–2,2 s de CPU W1 par trame (÷3,5–4,1), passe unique ≤ 40 ms (÷4,2–4,5),
   résolution ≤ 30 ms (÷3–4), squelette séquentiel ≤ 25 ms, queue de publication ≤ 4 ms (§ 1.3). **Au pire de la
   plage déclarée** (30 000–60 000 sites), le travail vaut ×1,3 (médiane 50–60 k) à ×1,87 (c08_003412) celui de
   08/000000 : la cible effective sur 08/000000 tombe à **54–77 ms** [F:auditeurs, M sur les comptes, E sur les temps].
3. **Ce que rapportent toutes les transpositions retenues** (v10, v9–v6, v4, v2/v3, auditeurs), cumulées et non
   additives : FULL 08/000000 vers **≈ 215–350 ms** à froid (E/C), c'est-à-dire au mieux le niveau de la v10. Le plus
   gros levier CPU est la **feuille J3** de la v10 (−38 à −60 ms de passe unique, E ; ×1,37 mesuré sur la v10, M-loc) ;
   le plus gros levier immédiat est l'**ordonnancement du pipeline** (deux prises de 08/000000 sans queue de
   publication : 373,7 et 391,1 ms au lieu de 412–464, M).
4. **Ordre** : T0 mesurer (cible vraie, bruit A/A, cause de la queue, K = 10 de référence, census par arité, sondes
   de rupture) et poser les filets (Euler K+2, restriction, EMST d'ordre 1, fixtures) ; T2 petits leviers sûrs
   (ordonnancement, q3 différé déjà porté, compteurs R1, arènes R3) ; T3 census (borne exacte sur réseau + arbre radix
   sur l'ordre de Morton) ; T4 feuille J3 par sous-tranches ; T5 forêt sans lots ; T6 squelette ; T7 seconde vague de
   résolution ; T8 décision réécriture par lots ou GPU. Chaque tranche a sa session G4 nommée et son critère (§ 3).
5. **Jugement.** K = 5 : **100 ms n'est pas atteignable par les transpositions seules** ; le travail ne l'interdit
   pas (plancher de la famille ≈ 2 600 cycles locaux par boule contre un budget ≈ 9 900, [PR] § 1, E), mais il faut
   la feuille par lots vectorisée et l'étage G réécrits des conceptions du 2 octobre (cible 65–91 ms, « sans marge »,
   aucun coût unitaire mesuré), ou le catalogue sur GPU par sous-arbres ; au pire de la plage, même ces cibles
   dépassent 100 ms. **K = 10 : non** (plancher estimé ≈ 3 000 cycles par boule > budget ≈ 2 600 ; tour CPU estimée
   125–180 ms à elle seule) ; viser 0,3–0,4 s après une première mesure, jamais faite dans le contrat.
6. **À ne pas refaire** : mémos de lane, F6 sur les signes du census, `-march` global, sous-maille T6 sur LiDAR,
   `SiteTree` v10 tel quel, Kruskal par lots, frontière à barrières, publication par Borůvka à la v9, feuilles seules
   ou census seul sur GPU, verdicts tirés de médianes indépendantes de cinq prises W48 (§ 5).
7. **Décisions à demander à l'utilisateur** (§ 4.5) : froid ou chaud (processus résident) ; maximum ou médiane de la
   plage ; ouverture de la route GPU dès que les sondes T0 la justifient ; cible annoncée à K = 10.

---

## 1. Budget : mesuré, perdu face à la v10, cible

### 1.1 Mesuré aujourd'hui à W48, par étage disjoint (b872, prise médiane par FULL) [AB7, M]

| Étage (champ du banc) | ng00 | ng01 | ng02 | W1 ng00 (accélération W1→W48) | Code |
| --- | ---: | ---: | ---: | ---: | --- |
| **FULL** | **412,4** | **351,7** | **380,7** | 9 133 (×22,1) | `bench/full_probe.cpp` |
| Index | 0,4 | 0,35 | 0,4 | — | `src/index/build.cpp` |
| Préambule du catalogue (`prefix_ns`) | 20,1 | 18,8 | 20,7 | 85,5 (×4,3) | `adaptive_prepare.cpp`, `adaptive_frontier.cpp` |
| **Passe unique** (`single_pass_ns`) | **180,3** | **167,0** | **170,2** | 4 752 (×26,3) | `boxes.cpp`, `leaf.cpp`, `single_pass.cpp` |
| Compactage | 4,8 | 4,0 | 5,5 | 33,4 (×6,9) | `single_pass.cpp` |
| Tri | 11,5 | 9,1 | 13,6 | 262,6 (×22,9) | `sort_indices.cpp` |
| Rangs + assemblage + allocation | 9,7 | 7,7 | 10,2 | 209,1 (×21,6) | `assembly_parallel.cpp`, `assemble.cpp` |
| Résidu du domaine (table des supports, libérations ; non chronométré) | 13,4 | 11,8 | 13,9 | 32,2 (×2,4) | `src/tower/full_domain.cpp` |
| Contextes FULL (résidu des forêts) | 9,1 | 7,8 | 9,8 | 45,8 (×5,0) | `population_lookup.cpp`, `census_slots.hpp`, `forest_parallel.cpp` |
| Classification | 2,6 | 2,3 | 2,7 | 37,2 (×14,2) | `cells_classify.cpp` |
| Naissances + liaison | 8,4 | 6,8 | 13,2 | 53,7 (×6,4) | `forest_build.cpp` |
| **Résolution régulière** (fin de la dernière résolution) | **115,8** | **86,4** | **96,4** | 3 424 (×29,6) | `forest_parallel.cpp`, `descent.cpp`, `locate.cpp`, `census_workspace.cpp` |
| Queue de publication après la dernière résolution | 36,2 | 29,7 | 23,9 | 130,9 (étage complet) | `forest_concurrent.cpp`, `forest_plateau.cpp` |
| Queue des verticales | 0,0 | 0,0 | 0,0 | 66,6 (étage complet) | `forest_vertical_parallel.cpp` |
| CPU du processus (s) | 13,73 | 10,79 | 12,88 | 9,13 | |

Dispersion sur les cinq prises W48 [AB7, M, recalculée] : FULL 373,7–464,2 / 327,7–362,6 / 350,5–438,4 ms ; passe
unique 176,5–211,4 / 140,8–190,1 / 151,5–201,0 ; résolution 115,2–126,9 / 85,1–93,5 / 96,4–105,8 ; **queue de
publication 0,9–39,6 / 14,9–32,5 / 16,7–45,0**. À ng00, les deux prises sans queue (r1 : 0,9 ms ; r4 : 1,1 ms) font
373,7 et 391,1 ms pour une résolution identique ; à ng00 r2, l'ordre 3 traîne de 27,3 ms alors que sa publication
complète vaut 25,5 ms à W1 : le publieur a fait presque tout son travail **après** la fin de la résolution [AB7, M ;
lecture de [V:v7] § 1.3]. En étage séparé (base `a45daff3a`, sans pipeline), la publication de l'ordre 5 à W48 vaut
43,8–58,4 ms sur ng00, plus qu'à W1 (42,1 ms) [AB7, M]. Qualification c40 : publication W48 en étage séparé
51,6 / 35,8 / 48,3 ms, verticales 41,8 / 36,7 / 28,3 ms [Q, M].

Travail logique de 08/000000 (identique à tout W) [Q, M] : 1 306 696 boules, 783 071 nœuds de boîtes et 353 456
feuilles, 379,4 M tests du filtre G1, 120,4 M préfixes, 3,27 M jugements, 10,26 M candidats q4 ; 897 776 naissances,
1 541 750 nœuds K1..5, 4,80 M pas de descente dont 3,62 M succès de table, 291 515 census (21,15 M tests de points),
3,79 M présentations MEB. L'ordre 5 porte 81 % des tests de points du census, 70 % des MEB, 40 % des pas.

Profil W1 de 08/000000 [AB7 `perf_new_self.stdout`, M], parts du CPU du processus : feuille (`extend` 16,1 %,
`enumerate_leaf` 6,8, `center_line_meets` 5,0, `Q4Candidate::through` 3,0, `Sphere::through` 3,0, `center_in_box` 1,8,
plus l'essentiel de `num::side` 7,1), soit ≈ 80–85 % de la passe unique, contre ≈ 15–20 % pour le filtre G1 (8,0 % du
CPU) (E, [V:v10_moteur] § 2) ; census `power_bound_signs` 5,0 + `bound_terms` 2,0 + `query` 2,1 ; voie liée `bound`
2,9, `find_support` 2,7, `resolve_job` 2,6, `find` 2,3. À W48 (base, [PROF1]) :
fautes de page 3,1 % du CPU en cumul, verrou noyau 1,55 %, `__pte_offset_map_lock` 1,51 %, `buffer_acquire/release`
0,86 %. Le CPU W48 vaut ×1,50–1,53 celui de W1 à compteurs égaux, sans attribution [C11 § 2.2].

### 1.2 Où la v11 perd face à la v10, étage par étage, K = 5, 48 fils

v10 = [S4], troisième passe chaude d'un processus ; v11 = [AB7], médianes indépendantes de cinq processus neufs.
Protocoles différents : écart descriptif, pas un A/B causal [C10 § 3.1, M].

| Étage | v10 00 / 01 / 02 | v11 00 / 01 / 02 | Écart v11 − v10 | Nature de l'écart |
| --- | --- | --- | --- | --- |
| frontière / préambule | 21,8 / 23,2 / 23,0 | 20,7 / 18,4 / 20,8 | −1 à −5 | parité ; squelette quasi sériel des deux côtés |
| **boîtes / passe unique** | 106,7 / 84,8 / 101,5 | **195,0 / 167,0 / 159,5** | **+88 / +82 / +58** | CPU ×1,27 à W1 (02) et passage à l'échelle ×22,7–28,0 contre ×34,0 ; compteurs vérifiés, niveau q3 avant la boîte, DFS + cache au lieu de la table des triplets, atomiques et allocation par nœud, grain ≤ 1 024 tâches |
| ordre / tri | 11,2 / 9,2 / 12,6 | 11,7 / 9,3 / 13,1 | ≈ 0 | parité depuis les clés F3/F4 |
| assemblage + hors étages | 23,8 / 19,7 / 27,2 | ≈ 24 / 22 / 25 | ≈ 0 | parité |
| index des supports, atlas, semis / classification + naissances | 10,4 / 9,3 / 11,5 | 11,0 / 9,1 / 14,3 | ≈ 0 | parité |
| **descentes / résolution régulière** | 28,6 / 22,2 / 27,1 | **116,6 / 86,6 / 99,3** | **+88 / +64 / +72** | coût par pas ×3,2 à W1 (714 / 638 / 597 ns contre 188 ns) pour +8 % de pas (absence du mémo par cellule) ; census ≈ 3,1 µs par appel (parcours seul) contre ≈ 1 µs pour une boule fermée v10 **non certifiée** |
| Kruskal + verticales / queue de publication | 49,3 / 35,7 / 49,0 | 33,5 / 20,8 / 34,1 | −16 / −15 / −15 | pipeline v11 meilleur |
| **FULL** | **252,0 / 204,2 / 253,6** | **412,4 / 351,7 / 380,7** | **+160 / +148 / +127** | ×1,64 / ×1,72 / ×1,50 ; CPU ×1,50–1,61 |

À un fil sur 02 [S1, S4, AB7, M] : catalogue ×1,28, passe unique ×1,27, forêts ×2,87, résolution ×3,43, publication
×1,9, verticales ×0,73. **La v10 n'était pas plus rapide parce qu'elle calculait moins** : boules, nœuds, cellules,
traces, census, jugements et candidats q4 sont égaux à quelques pour cent [C10 § 0, M].

### 1.3 Budget cible de 100 ms à W48, K = 5

| Étage | Mesuré ng00 / ng01 / ng02 (M) | **Cible 100 ms** (trames mesurées) | Facteur | Cible au pire de la plage (≈ ×1,3 de travail) |
| --- | --- | ---: | ---: | ---: |
| Index (Cloud hors FULL) | 0,4 / 0,35 / 0,4 | ≤ 0,5 | — | ≤ 0,5 |
| Préambule | 20,1 / 18,8 / 20,7 | ≤ 5 | ÷4 | ≤ 4 |
| Passe unique | 180,3 / 167,0 / 170,2 | ≤ 40 | ÷4,2–4,5 | ≤ 30 |
| Compactage + tri + rangs/assemblage | 26,0 / 20,8 / 29,3 | ≤ 8 | ÷3 | ≤ 6 |
| Résidu du domaine | 13,4 / 11,8 / 13,9 | ≤ 3 | ÷4 | ≤ 2 |
| Contextes + classification + naissances | 20,1 / 16,9 / 25,7 | ≤ 7 | ÷2,5–3,5 | ≤ 5 |
| Résolution régulière (chemin critique du pipeline) | 115,8 / 86,4 / 96,4 | ≤ 30 | ÷3–4 | ≤ 23 |
| Queue de publication + verticales | 36,2 / 29,7 / 23,9 | ≤ 4 | ÷6–9 | ≤ 3 |
| Marge (étendue du FULL à W48 : 5–7 % sur trois prises [Q], jusqu'à 24 % sur cinq prises [AB7]) | — | ≈ 2–5 | | ≈ 2 |
| **FULL** | **412,4 / 351,7 / 380,7** | **≤ 100** | **÷3,5–4,1** | **≤ 77 sur ng00** |

Vue CPU (E, à l'accélération W1→W48 mesurée ×20,1–22,2) : ≤ **2,0–2,2 s de CPU W1** par trame au lieu de 7,07–9,13 s,
soit ≤ 1,6–1,8 µs de CPU W1 par boule au lieu de 6,0–7,0 µs ; à parallélisme parfait (×48), ≤ 4,8 s. Le CPU W48
gonfle aujourd'hui de ×1,5 sur W1 : le récupérer est un levier à part entière, à attribuer d'abord (T0).

### 1.4 Ce que ce budget impose

1. **Les deux grands postes doivent baisser chacun d'un facteur 3 à 4,5 en CPU par unité de travail** (M pour
   l'état, E pour l'exigence). La passe unique est au plafond SMT de la v11 (×3,3–3,7 de W8 à W48 pour ×3 cœurs,
   [Q]), alors que l'étage équivalent de la v10 passait de 1 à 48 fils à ×34 : une part (≈ 30 ms sur 02 si ×28 → ×34,
   [C10] levier 2, C) relève du passage à l'échelle, le reste du travail par boule.
2. **Les « petits » étages consomment à eux seuls 98–116 ms** (M) : rendre gratuites la passe unique et la
   résolution ne suffirait pas. Le squelette séquentiel de la structure actuelle (index, préambule, résidu du domaine,
   contextes, naissances, publication de l'ordre 5 en une tâche) vaut **82–107 ms** [C11 § 6.3, E].
3. **La publication de l'ordre 5 (34–46 ms de travail séquentiel à W1) n'est cachée que par la lenteur de la
   résolution.** Dès que la résolution passe sous ≈ 50–60 ms, elle devient le chemin critique : la forêt sans lots
   (T5) est nécessaire, pas suffisante.
4. **Au pire de la plage déclarée**, ÷5,4 à ÷7,7 sur 08/000000 au lieu de ÷4,1 (E ; c08_003412 est atypique :
   18,4 nœuds d'ordre 5 par site contre 13,3–14,4 pour les autres trames de 50–60 k, [V:auditeurs]).

---

## 2. Les idées une par une

Vingt idées de vitesse ou de mémoire gardées ou « à mesurer » par l'audit, regroupées par étage visé. Plusieurs sont
la même idée venue de sources différentes : elles ne font qu'**un** port (dernière colonne). Les gains sont ceux des
contre-vérifications, non additifs (chaque levier réduit l'assiette des autres) et convertis en mur W48 en supposant
que l'étage visé reste sur le chemin critique.

### 2.1 Passe unique et préambule du catalogue

| Idée | Étage visé | Gain attendu et base | Coût | Risque | Dépend de | Porte et mutants | Tranche |
| --- | --- | --- | --- | --- | --- | --- | --- |
| **v10_moteur_01** — feuille J3 : census par masques (`domby`, lemme R), triangle médian (M3), enveloppe du tétraèdre (E4), droite i64 à termes de paire, étages + table H des triplets vivants ; **garder** | feuille (≈ 80–85 % de la passe unique) | **−38 à −60 ms** de passe unique à W48 (E : feuille ×1,37 → passe ×0,77 ; ×1,65 → ×0,67) ; prudent **−25 ms** ; au-delà de T2 (compteurs, q3) **−20 à −40 ms** ; K = 10 ×1,55 sur l'étage. Base : v10 trame 02 entière, CPU de `t_boxes` 7,683 → 5,603 s (×1,371) à K5, 32,928 → 21,278 s (×1,548) à K10, un fil, ABBA ; ×1,36/×1,56 à quatre fils ; callgrind instructions ×1,46/×1,67, mauvaises prédictions ×2,16/×2,37 (M-loc, [F:v10_moteur] § 3 ; [V:v10_moteur]) | élevé : cinq sous-tranches ; contrat de ledger à renégocier ([R2] fige `prefixes` et `region_line_*`) | gain SMT inconnu au-delà de 4 fils ; bornes J3 écrites pour u18/T6 (\|lo\|, \|hi\| < 2^25) à refaire en u21/u24 T0 ; la v5 a fermé « étage i64 du préfiltre q4 » (1,0021) ; deux mutants de bord n'étaient tués que par des fixtures ; la table H n'est pas dans le gain ×1,37 (elle était déjà dans la base v10) | T1 (Euler K+2, restriction, fixtures de rang) ; T0 (bruit A/A) ; [R1] pour `side` total (certificat de coefficients et replis) | dumps FULL octet pour octet (3 trames + trames lourdes, W1/W48) ; fuzz différentiel J3/DFS (1 060 cas conclusifs en v10) ; compteur « tests de census évités » ; mutants m404 (9 tués + 1 prouvé équivalent), M-C3 (plans dyadiques), M-C5 (débordement de la voie étroite), `domby` omis, M3 non strict, E4 inversée ; fixtures F-DYA, F-TRI, F-L64 | T4 |
| **auditeurs-04** — compteurs locaux bornés (R1) et `Buffer` par tâche (R3) dans la boucle chaude | feuille (`checked_add`) ; `boxes.cpp` (`Buffer::allocate` par nœud : deux CAS et un `fetch_sub` partagés) | compteurs : ≈ 0,5 G additions vérifiées par trame (E) → borne haute 5–17 ms ; **calibrage v9** (M-loc, `morsehgp3D_v9/receipts/q34_micro_levers_20260923/`) : tout retirer rendait 4,0–4,4 % du CPU de l'étage, la version sûre 0,9 % → attendre **−2 à −7 ms** ; arènes **≤ −5 à −8 ms** (E/C ; 0,86 % `buffer_*` à W48, plus une part des fautes de page, [PROF1]) | faible | faible ; le compilateur garde peut-être déjà les compteurs en registre | [R1] (feuille réelle m ≤ 1024, flush `checked_add` avant publication), [R3] | ledger identique octet pour octet ; feuilles m = 32/33/256/1024 ; compteur global près de u64max ; refus sans sortie ; R3 : plafond/−1, panne d'allocation, abandon, retour au budget, zéro après destruction ; mutants « flush omis », « borne m ≤ 32 », « majorant amputé d'un terme » joué sur trames entières (profondeur 36) | T2 |
| **v10_moteur_05** — zéro allocation ni atomique par nœud, arènes réutilisées et pré-touchées d'une trame à l'autre | passe unique, résidu du domaine (×2,4 seulement), contextes (×5–6) | **−5 à −15 ms** dans la trame (C ; symptômes M : `asm_exc_page_fault` 3,12 %, `do_anonymous_page` 2,47 %, verrou noyau 1,55 %, `__pte_offset_map_lock` 1,51 % du CPU W48, [PROF1]) ; inter-trames : seulement en régime chaud ; « émission 993 → 115 cycles par boule pages touchées » est une mesure privée disparue | moyen (arènes de `Session`) | confondre froid et chaud ; budget à compter honnêtement | [R3] ; décision utilisateur froid/chaud | froid et chaud publiés côte à côte ; `ru_minflt` par étage ; sorties et budget compté identiques | T2 (intra), T6 (inter) |
| **v10_moteur_03** + **v2_v3-02** — filtre G1 sans branchement (SoA), tête parallèle par tranches (v3b) ; site intérieur à la boîte fermée gardé sans test | préambule (20,1/18,8/20,7 ms, ×3,8–4,3) ; filtre G1 (8,0 % du CPU W1) | **−5 à −20 ms** (C) ; racine sérielle **−1,2 / −1,4 ms** quasi sûrs (598 275 / 687 675 tests à ≈ 2 ns, E sur compteurs simulés = AB7) ; v3b : `t_frontier` ×1,90–2,45 mais catalogue entier ×0,88–1,05 et ×0,781 à K10/W4 (M-loc) ; modèle G4 ×1,13–1,15 sur le catalogue v10 (E) ; noyau : 5,2–5,5 cycles par test, −39 % d'instructions en `x86-64-v3`, ×1,08 sur l'étage (M-loc, [L05] § 6.4) | moyen (patch v3b privé ; noyau à écrire) | le chemin critique du préambule est peut-être le pilote (`capture`, `select` par insertion, 27 rondes), non simulé ; course d'ordonnanceur (N9 v10) | T0 (sous-chronos du préambule) ; empreinte des listes | listes et nœuds identiques (empreinte commutative (boîte, liste) sur tous les nœuds) ; `filter_tests`/`priority_tests` logiques inchangés ; TSan + stress ; mutants : bornes prises sur la boîte parente, `hi` demi-ouvert, seuil de taille de liste mal appliqué, réservoirs de tranches fusionnés hors ordre | racine T2 ; reste T6 |
| **v10_moteur_04** — table M(K) : feuille 24 à K = 10 ; **garder** | passe unique à K = 10 | 0 à K = 5 ; catalogue K = 10 ×1,6 (modèle) à ×1,77 (local bruité) en v10 ; M = 16 → 11,06 M nœuds, 137,8 candidats par boule ; M = 24 → 1,07 M, 57,9 (M déterministe, `preuves_l05_code_catalogue/ablation_M.txt`) ; E pour la v11 | nul (un paramètre) | arbre T0 de la v11 différent : refaire l'ablation | T0 | dumps identiques pour M ∈ {16, 20, 24, 28} à K = 10 ; compteurs W1 publiés | T0 (compteurs), T4 |

### 2.2 Census des descentes (résolution régulière)

| Idée | Étage visé | Gain attendu et base | Coût | Risque | Dépend de | Porte et mutants | Tranche |
| --- | --- | --- | --- | --- | --- | --- | --- |
| **v8-1 corrigé = v6-I1 = v7-3** (englobe les extrema q2 couplés de [R4] et auditeurs-03 b) — assistant de census à **borne exacte sur réseau** (minimiseur entier par axe, maximum au coin, préparé une fois par requête), puis **arbre radix (Karras) sur l'ordre de Morton existant** ; **garder** | census : parcours seul 8,2 % du CPU W1 = 0,90 s, ≈ 3,1 µs par appel ([V:v10_tour] § 1) | (a) seule : nœuds ×0,63–0,72, tests ×0,47–0,54 (simulations rejouées, E) → 4,0–4,6 % du CPU W1 → **−3 à −13 ms** ; (a) + radix : bornes ×0,35–0,38, tests ×0,26–0,28 (E, `../verif/v8_annexes/sim_radix.py`) → 7,3–8,2 % du CPU W1 (0,67–0,75 s, ≈ 20–22 % de la résolution) → **−5 à −25 ms** selon que la résolution reste critique ; rapports semblables à K10 | faible à moyen (≈ 60–100 lignes ; construction radix O(n), < 1 ms) | requêtes simulées plus légères que les vraies (≈ 50 contre 78,5 tests à l'ordre 5) ; voie `Wide` (q3 non certifié en u21) gardée sur la borne séparée ; coût par nœud +30 % possible ; gain absorbé si la publication devient critique | T0 (ledger census publié par arité : nœuds, bornes, blocs, tests) ; fixtures v8 ; fixtures de longues descentes (T1) | I et U **et leur ordre** identiques à la borne actuelle (requêtes tirées et LiDAR) ; pour (a), nœuds/bornes/tests ≤ ceux de la borne séparée (inclusion prouvée : « chevauche » exact implique « chevauche » séparé) ; pour le radix, mêmes I, U et témoins saturés ; dumps FULL identiques ; fixtures v8 (plafond, plancher, quart, demi, cube tangent, saturation d'un nœud) et F1 « minimum au sommet » ; mutants : plancher au lieu de l'entier le plus proche, troncature vers zéro, coin du maximum inversé, minorant de réseau appliqué à une boîte continue, assistant branché dans `power_bound_signs` (doit casser `tests/num/bounds_test.cpp`), coupe radix décalée d'un rang | T3 |
| **auditeurs-03** — (a) ventiler le census par arité (q1/q2/q3/q4) : appels, nœuds, bornes, blocs, tests, saturations ; (b) q2 couplé | instrumentation ; census q2 | (a) aucun gain : il **décide** entre les leviers du census ; (b) ≤ 0,2–0,6 % du CPU, ≈ 1–2 ms (E) | faible | aucun | — | compteurs hors du grand livre exact, jamais dans un digest de sortie | (a) T0 ; (b) fondu dans la ligne précédente |
| **v10_tour_02** — census « k plus proches » sur un k-d à coupes spatiales | census | **−5 à −20 ms** (C ; parcours 0,90 s W1, même ÷3 → ≈ −20 ms au plus) ; la projection K10 de −165 à −190 ms reposait sur le coût d'un parcours flottant v10 non certifié | moyen ; banc M5 de [CT] d'abord (les ≈ 0,29 M sphères de census de ng00 vidées) | contrat plus fort, plus de travail par requête ; les descentes changent (le mutant v10 `JUMP_ANY` ne coûtait que +3 % de MEB et +7 % de sauts) | après T3 ; seulement si le census coûte encore > 1,5 µs par appel | dumps FULL identiques ; mutants de politique de saut tués aux ordres 6–10 (fixtures de longues descentes) | T7 (conditionnel) |
| **v10_moteur_06** — `LeafOracle` : census par la liste K-certifiée de la feuille du centre | census | f × ≈ 33 ms (E) ; f inconnu, défavorable à l'ordre 5 (toute saturation retombe sur le repli) : **0 à −30 ms** (C) ; +40–50 Mo | moyen | contredit [CT] § 3.6 et [PR] R2.5 (arbre ajusté non total pour p ≥ K, localisation, 40 Mo, couplage aux listes que la tour doit pouvoir contredire) ; énoncé « centre hors de toute boîte ⇒ ≥ K intérieurs » à écrire au registre | compteur T0 « la liste de la feuille aurait-elle suffi ? » ; ne prototyper que si f ≥ 0,5 | identité au census global (I, U, témoins saturés) ; repli dès K intérieurs ; mutant « liste locale tenue pour complète avec p ≥ K » | T7 (conditionnel) |

### 2.3 Descentes et consultations

| Idée | Étage visé | Gain attendu et base | Coût | Risque | Dépend de | Porte et mutants | Tranche |
| --- | --- | --- | --- | --- | --- | --- | --- |
| **v10_tour_05** — `resolve1` : arrêt à la première cellule de fenêtre, pointeurs suivis après coup (D-G1 de [CT]) | pas sans succès de table (≈ 2,3 µs l'un à W1, 1 175 096 sur ng00) | ≈ 13 % de ces pas → ≈ 0,35 s W1 → **−5 à −12 ms** (E, borne haute ; v11 +6,5 % de pas hors table face à la v10, M ; v10 à 1,07–1,08 fois le minimum, statistique binary64) ; plus utile à K10 | moyen | le suivi des pointeurs se fait dans la publication, chemin séquentiel ; change les graines réemployées par les verticales ; lemme T3 à inscrire au registre | compteur T0 (pas évités par ordre) ; fixtures de longues descentes ; idéalement T5 | graines finales identiques à la voie actuelle après suivi ; forêts et verticales identiques ; fixture {0, 2, 4}, K = 2 ; mutants : pointeur lu avant λ_b, cible « cellule » prise pour naissance | T7 |
| **auditeurs-05** — mémo de cellule partagé, typé par sa date de validité λ_b ([R5]) | mêmes pas | **≤ 7–9 ms** à K5 (E, borne haute ; v10 : 307 177 arrêts mémo à K5, 2,14 M à K10, M) ; les pas évités sont peut-être des succès de table bon marché | moyen | objet partagé sur le chemin le plus chaud ; aujourd'hui `pipeline_lanes` rend 0 si un mémo est actif | compteur T0 des répétitions (cellule, ordre) à λ_b valide ; alternative à `resolve1` | témoins de [R5] `pieces_before_cell`, `same_date_different_cells`, `invalid_terminal_date` ; TSan ; compteurs partagés publiés à part | T7 (si `resolve1` est refusé) |
| **v10_tour_04** — tables de semis par ordre, empreinte additive, étiquette dans la table des supports | voie liée (≈ 170 ns par trace), `find_support` (2,73 % propre W1) | **−3 à −10 ms** (C) ; la résolution « ordre par ordre » contredit le pipeline (blocs triés par première boule) : écartée sans T5 | faible à moyen ; banc M3 de [CT] | gain sous le seuil d'intérêt s'il est seul | T5 pour la partie « ordre par ordre » | seule l'égalité exacte des sites décide ; collision d'empreinte forcée (PO-R4) ; mutant « décision sur l'empreinte » | T7 |

### 2.4 Forêts : publication et verticales

| Idée | Étage visé | Gain attendu et base | Coût | Risque | Dépend de | Porte et mutants | Tranche |
| --- | --- | --- | --- | --- | --- | --- | --- |
| **v10_tour_01** (= D-F1 de [CT], = v6-I2) — forêt d'un ordre **sans lots** : noyau union-find minimal, événements, **matérialisation parallèle** des plateaux, historique d'attache | publication (ordre 5 : 41,8 / 34,3 / 46,0 ms à W1, 43,8–58,4 ms à W48 en étage séparé ; queue 0,9–45,0 ms dans le pipeline) | aujourd'hui **0 à −30 ms** selon la cause de la queue ; publieur d'ordre 5 → **20–29 ms** à W1 (E : krbench ÷1,6 « multifusions à la volée », seul gain séquentiel complet mesuré, M-loc) ou **11–18 ms** si la matérialisation parallèle tient (C) ; K10 : publication d'ordre 10 ≈ 119–146 ms séquentielle (E) → 20–35 ms (E, [CT] § 9) | moyen à élevé | le prototype `noyau_v11.log` ne compte pas la matérialisation (33,98 ms séquentiels à l'ordre 5, au-dessus du Kruskal par lots) ; la v7 a vu une voie fenêtrée plus lente (188,6 → 250,4 s) ; renumérotation invisible à un digest agrégé | T0 (horodatage début/fin, CPU de fil, attente par tâche ; ablation bornante de `close`) ; registre : « contraction des plateaux » encore `proof_obligation` (T4–T6) ; juges EMST et Merkle (T1) | forêts, numérotation et verticales octet pour octet contre la voie actuelle gardée en référence ; W1/W4/W48 ; TSan ; mutants `mat_no_plateau_grouping`, `kernel_junction_on_head`, `mat_successor_max`, multifusion binarisée, image verticale ouverte, attache ouverte (L06-03) | T5 |
| **v7-1** — publication intra-ordre par certificats de forêt couvrante minimale composables, multifusions reconstruites après composition | publication, surtout ordre 10 | K5 : 0 à −5 ms au-delà de D-F1 (E) ; K10 : noyau 20–35 → 10–20 ms (E) ; théorème vérifié (modèle Python, 198 compositions, 4 720 comparaisons, cinq mutants, M-loc) ; aucun débit parallèle jamais mesuré | moyen à élevé | travail total en hausse (Kruskal local + composition + noyau) ; mémoire de la pile de certificats | D-F1 | identité octet pour octet ; cinq mutants du modèle v7 et contre-fixture de départage ; régions 1/7/256 ; plateaux à cheval sur deux régions. Règle : garder si D-F1 + MSF bat D-F1 d'au moins 5 ms à K5 ou 10 ms à K10 sur les trois trames ([V:v7]) | T5 (bras K10) |
| **auditeurs-02** — publier un ordre sans barrière de plateau (forêt minimale, lemme du maximum d'ID de la v9) | publication et verticales | chaîne exposée ≈ 35–50 ms une fois la résolution divisée par 3 (E) ; mais le constructeur min-label de la v9 coûtait **1 264,7 ms à K5 sur ng00** (M-loc, `morsehgp3D_v9/audits/b_full_a_real_20260927/RESULTATS.md`) et une publication par lots attend la fin de toute la résolution de l'ordre (perte du recouvrement) | élevé | voir gain | — | sidecar G4 d'identité (nœuds, parents, `next`, ancres, contributions, populations, verticales) ; abandon si le total dépasse ≈ 10 ms | **garder le seul lemme** comme preuve des identifiants de T5 (la v11 numérote les groupes d'un plateau dans l'ordre des racines DSU, `forest_sort(touched, a < b)`) ; écarter la route |
| **v7-2** — verticales et requêtes « nœud vivant à la coupe fermée » par chaînes lourdes, O(log N), en parallèle sur forêts closes | verticales (ordre 5 : balayage 16,5–22,3 ms à W1 ; 25,7–39,5 ms à W48 en étage séparé) | **0 aujourd'hui** (queue verticale nulle, < 0,05 ms, dans 10 prises W48 sur 15 et 0,9–7,1 ms ailleurs, M) ; après D-F1 : chaîne d'ordre 5 → **3–8 ms** (E, 100–200 ns par requête non mesurés) ; à K10 évite un mur séquentiel ; sert aussi le port natif des points | moyen | exige la forêt k − 1 close (perte du suivi incrémental) ; doublon de D-F3 ([CT]) | D-F1 | `lower_` identiques au balayage actuel (trois trames) ; peigne ≥ 8 191 nœuds ; requêtes avant naissance refusées ; cinq mutants v7 (fermé/ouvert, racine finale au lieu de l'image datée, date perdue, raffinement binaire non contracté, feuille hors descendants) ; fixture ABCZ | T5 |

### 2.5 Processus et mémoire

| Idée | Étage visé | Gain attendu et base | Coût | Risque | Dépend de | Porte et mutants | Tranche |
| --- | --- | --- | --- | --- | --- | --- | --- |
| **v10_tour_07** — processus résident, passes chaudes, arènes réutilisées entre trames | tous les étages qui touchent des pages neuves (préambule ×4, résidu ×2,4, contextes ×5) | **−3 à −7 %**, soit −10 à −29 ms (E : v10 passes 1 → 3, 259,8 → 252,0, 218,6 → 204,2, 265,5 → 253,6 ms, une prise par passe, M) ; la v9 ne mesurait que ≈ 2 % (943 → 923 ms, M) | moyen (`--repeat=R` dans `full_probe`) | biaise la mesure des leviers mémoire ; ne vaut pour le contrat que si 100 ms est une latence de flux | **décision utilisateur** | froid et chaud publiés côte à côte, jamais l'un pour l'autre ; sorties et budget compté identiques ; fautes par passe | T6 |

### 2.6 Correspondance avec les sept verrous du développeur

| Verrou [Q100] | Réponse des auditeurs | Place dans ce plan |
| --- | --- | --- |
| 1 compteurs | [R1] oui, m ≤ 1024, flush checked | T2 (auditeurs-04) ; J3 en T4 garde la même règle |
| 2 q3 différé | [R2] ledger identique | **porté** (`56216392e`) ; A/B G4 dans T2 |
| 3 arènes | [R3] oui, réservation effective budgétée | T2 (intra-trame) ; T6 (entre trames) |
| 4 census | [R4] q2 couplé, puis ablation de partition | T3 : borne de réseau toutes arités (généralise q2 couplé) puis radix (ablation de partition à résultats identiques) ; k-d et `LeafOracle` en T7 sous condition |
| 5 mémo | [R5] certificat typé λ_b | T7 : `resolve1` d'abord (déterministe, sans écriture partagée), mémo typé sinon |
| 6 T6 | [R6] mesurer d'abord | **écarté pour le LiDAR** : la v10 l'a mesuré neutre (dumps identiques, travail ±0,3 % pour T = 6/3/0, `ablation_kT.txt`) |
| 7 GPU | [R7] voie autorisée, route entière exacte | sonde précoce en T0, décision en T8 |

---

## 3. Ordre d'exécution en tranches

### 3.0 Protocole commun de mesure (sans lui, aucun levier de moins de ≈ 30 ms ne se décide à W48)

- **Le bruit W48 est plus gros que les leviers.** Dans [AB7], le domaine, de code identique dans les deux binaires,
  donne des rapports appariés new/base de 1,138 / 0,952 / 1,012 et 0/5 victoires sur ng00 (A/A de fait à ±14 %) ;
  σ du logarithme du rapport par paire : 0,111 (FULL), 0,125 (domaine), 0,137 (forêts), d'où **29, 37 et 44 paires**
  pour voir un effet de 5 % (E, [F:v4] § 2.1, rejoué par [V:v4]). Le gain FULL publié 446,5 → 412,4 ms sur ng00 n'est
  pas significatif (3/5) ; celui des forêts l'est (5/5 sur les trois trames).
- **Règle.** Décider un levier de **travail CPU** à W1 et W8 (étendue de la passe unique 0,0–0,5 %, [Q]) par paires
  AB/BA, médiane des rapports appariés et test des signes, avec un bras **A/A** ; le confirmer à W48 avec un nombre de
  paires fixé d'avance par le σ mesuré en T0 (≈ 8–12 paires pour 10 %, ≈ 30–45 pour 5 %). Décider un levier de
  **passage à l'échelle** (atomiques, fautes de page, SMT, ordonnancement) à W48 et **W24 épinglé** (un fil par cœur
  physique). Chaque prise compare l'empreinte du dump à la base (déjà fait par `protocol/ab_g4.py`).
- **Trames** : ng00/ng01/ng02 plus les trames lourdes choisies en T0 ; publier le **maximum** sur l'ensemble déclaré.
- **Sessions** : `gcp-migration/v11_session.py --commit` (seul mode qui produit un reçu), plans à portes Python sous
  Python 3.10 nu (`python3 -S`), TSan ciblé, mutants choisis ; froid (processus neuf) toujours publié. Les noms de
  sessions ci-dessous sont **proposés** ; plusieurs tranches peuvent partager une session si chaque levier y est mesuré
  seul, puis en combinaison ([R1]–[R7] : « Mesurer chaque changement séparément, puis leur combinaison »).

### T0 — Mesurer avant de couper (aucune décision ne change)

**Contenu.** Instrumentation hors du grand livre exact, sorties identiques :
- publier dans `full_probe` ce qui est déjà calculé ou presque : `single_task_sum/max_ns` de la passe unique
  ([V:v9]) ; ledger census par ordre et par arité (nœuds, bornes, blocs intérieurs/extérieurs, tests, saturations ;
  auditeurs-03 a) ; sous-chronos du préambule (racine, rondes, `select`, `capture`, attente du Pool) ; chronos de la
  table des supports, de la destruction des arènes et des contextes ; `getrusage` (CPU utilisateur/système, `ru_minflt`)
  par étage ([F:v4] v4-04) ;
- pour chaque tâche du pipeline : **instant de début**, fin, CPU du fil (`CLOCK_THREAD_CPUTIME_ID`, avec
  `cpu_clock_valid`), temps bloqué dans `await_job` et `await_lower` (v10_tour_01, [V:v8] v8-3 reformulé) ;
- compteurs de décision, sans changer le chemin : pas évitables par `resolve1` par ordre (v10_tour_05) ;
  répétitions (cellule, ordre) à λ_b valide (auditeurs-05) ; genres de census (complet, saturé p < K, p ≥ K) et
  « la liste de la feuille aurait-elle suffi ? » (v10_moteur_06) ;
- ablation **destructive** sous `MHGP11_TESTING` : `close` réduite à un comptage de composantes, en étage séparé,
  pour borner la part de la matérialisation dans les 34–46 ms de publication de l'ordre 5 ([V:v6] v6-I2) ;
- ablation de compteurs de la feuille à K = 10 pour M = 16/20/24/28 (v10_moteur_04) ;
- **sondes de rupture**, qui décident tôt de la route au-delà des transpositions : (i) **c(L)** sur l'arbre T0 de la
  v11 (part du travail de la passe unique qui resterait sur CPU si les sous-arbres de liste parente ≤ L partaient sur
  un appareil ; rdtsc par sous-arbre à W1 ; v10 : 52,3 % à L = 16, 15,1 % à L = 64, 7,9 % à L = 256, M-loc) ;
  (ii) vidage des candidats réels de la feuille (triplets, quadruplets, jugements) et des sphères de census de ng00,
  matière des microbancs M2, M4, M5 de [PR] § 8 et [CT] § 11.

**Mesure G4 nommée : `claudediag1`.** K = 5, W1/W8/**W24 épinglé**/W48 sur ng00/01/02 ; **A/A** intra- et
inter-processus (10 paires, même binaire) ; **matrice élargie** : c08_003412 (58 418 sites), c08_001518, c08_002992 et
deux ou trois trames d'autres séquences, préparées par `bench/points_lidar_prepare.py` épinglé (masque Patchwork++ v8 du
contrat ; recompter n) (auditeurs-01) ; **première prise K = 10 du contrat** (W48, processus neufs, trois prises par
trame, feuille 16 et 24) et porte de préfixe Kmax (sections d'ordre 1..5 du dump K10 = dump K5, octet pour octet hors
le mot `kmax` de l'en-tête ; v6-I4, v5-I2).

**Portes et mutants.** Dumps identiques avec et sans instrumentation ; somme des étages publiés ≥ 98 % de FULL
(code 3 sinon) ; compteur d'ouvriers alimenté dans la tâche (mutant v4 `parallel-hardcodes-one-worker`) ; porte
synthétique du rapporteur apparié (rapports 0,75 / 0,75 / 4,0 / 0,75 : médiane appariée 0,75, rapport des médianes
1,1667 ; plan non contrebalancé refusé en code 2, [F:v4]) ; mutant de préfixe « seuil lu sur `catalogue().kmax()` au
lieu de k » dans `regular_vertical_seeds.hpp` l. 32.

**Ce qu'on en attend.** Aucune milliseconde. Six réponses qui conditionnent tout le reste : (1) la **vraie cible**
(facteur de travail de la trame la plus lourde de la plage) ; (2) le **plancher de bruit** et le nombre de paires ;
(3) la **cause de la queue de publication** (démarrage tardif, famine SMT ou débit) ; (4) la **structure du census**
par arité ; (5) la **référence K = 10** (attendue vers 1,5–2,5 s, C : rapport v10 ×4,2–4,4 et feuille 16 codée en dur
pour tout K) ; (6) le partage **SMT contre contention** du gonflement ×1,5 du CPU W48 (W24 ≈ W48 : borné par la mémoire
et le série). Plus deux chiffres de rupture : c(L) et les candidats vidés.

### T1 — Filets de sûreté avant toute réécriture (en parallèle de T0)

**Contenu** (aucun gain de temps ; ce sont les juges qui manquent à l'échelle, chacun retenu par sa contre-vérification) :
- identité d'**Euler** par ordre à K + 2 (K7 pour K5, K12 pour K10 : la v11 admet K ≤ 12) et **restriction clé par
  clé** Cat_K = restrict(Cat_K+2) sur les trois trames et à 8k/16k/32k ; **épingle du mode 0** contre 16379 dans la
  même source (v10_moteur_02, v9-01) ; fixture D/T à 13 points gravée comme limite connue ;
- juge exact de l'**ordre 1** contre l'arbre couvrant minimal entier, plateaux N-aires (J2 de `docs/MATHEMATIQUES.md`,
  ehgp_zoltan_03 ; 88 enfants surnuméraires de vrais ex æquo sur ng00) ; différentiel de **Merkle** contre les ancres
  v10 (K5 sur les trois trames, K10 sur 00) et extension de `tests/tower/full_v10_diff.py` aux trames (v10_tour_03) ;
- fixtures : **longues descentes** (noyau serré et halo, six nuages à 3–4 sauts ; v10_tour_08), **portails
  silencieux** (A = (0,1,0) … E = (3,0,0), K = 2 ; v8-4), **non-hérédité du rang** R3v2, F64, Q2X, F16, CRUX
  (v2_v3-01), fixtures J3 F-DYA, F-TRI, F-L64 ;
- CI GitHub Python 3.10 nu sans numpy et Clang (produit-ci-v11), pour ne plus perdre de session G4 (`claudequal1`).

**Mesure G4 nommée : `claudegate1`** (les portes `lidar`/`scale*` nouvelles dans la matrice, K5 et K10). Mutants :
une boule en moins et un p déplacé (Euler en ÉCART, code 4), plateau binarisé et fusion q2 retirée (EMST),
`knn_from_support_site` (longues descentes), « paire ou face vivante décidée par le rang d'un sous-support » et
« seuil θ décalé » (fixtures de rang). **Ce qu'on en attend** : 0 ms ; le droit de réécrire la feuille (T4), le census
(T3) et les descentes (T7) sans perte silencieuse commune à tous les modes (la v10 a laissé passer un mutant de
frontière qui perdait 2 134 à 9 523 boules avec `status ok`, [L05] § 5.5).

### T2 — Leviers sûrs à sortie identique (ordonnancement, q3 différé, compteurs, arènes, racine G1)

**Contenu.**
- **Ordonnancement du pipeline**, selon le diagnostic de T0 : si démarrage tardif ou famine, donner aux publieurs et
  suiveurs des fils qui démarrent à temps (ou des publieurs qui **résolvent un bloc en attendant** au lieu de dormir),
  en gardant un argument d'absence d'interblocage pour tout W : aujourd'hui il repose sur « le Pool réclame les tâches
  par indice croissant et seules publications et balayages attendent, toujours des tâches d'indice inférieur »
  (`src/tower/forest_pipeline.cpp` l. 1–7 ; une tâche par fil à W48, grain 1, `src/sched/pool.cpp`) ; si la queue
  vient du débit du publieur, la renvoyer à T5.
- **q3 différé** (`56216392e`, déjà porté) : son A/B G4 annoncé.
- **Compteurs locaux** de la feuille ([R1]) et **`Buffer` par tâche** ([R3]).
- **Racine du filtre G1 sautée** (tous ses sites sont intérieurs ; v2_v3-02, part sûre).

**Mesure G4 nommée : `claudeab8`** — d'abord l'A/B du q3 différé (sources `3bd4d734e` contre `56216392e`), puis
base `56216392e` contre chaque autre levier seul, puis leur combinaison ; W1/W8 en paires pour le travail ; W48
(paires fixées par T0) et W24 pour l'ordonnancement et les arènes ; trois trames + trames lourdes ; dumps identiques à
chaque prise ; TSan sur le pipeline.

**Portes et mutants.** `mhgp11_tower_pipeline_decisions` (4 000 modèles) et `_equivalence` étendues au nouvel
ordonnancement ; `mhgp11_tower_pipeline_abandon` ; un cas W = 2K et W = 2K + 1 ; mutant « publieur réclamé avant ses
résolveurs, sans aide » (interblocage à W = 2K, marqueur causal et non un délai) ; portes R1/R3 du § 2.1 ; pour la
racine, `filter_tests` gardé comme compte logique (ajouter `selected` par site sauté) et mutant « intériorité jugée sur
une boîte plus large que la fermeture du nœud ».

**Ce qu'on peut attendre (E/C).** Queue de publication 36 → 5–25 ms si la cause est l'ordonnancement (deux prises
de ng00 le montrent possible, M), q3 −2 à −5 ms, compteurs −2 à −7, arènes 0 à −8, racine −1 : **FULL ng00
≈ 360–405 ms** (ng01–ng02 dans la même proportion). Petit, mais c'est aussi le premier test du protocole.

### T3 — Census des descentes : borne exacte sur réseau, puis arbre radix

**Contenu.** Assistant de census distinct (jamais dans `power_bound_signs`), préparé une fois par requête ; voie
native seulement, borne séparée gardée en repli ; puis index radix de Karras sur les clés Morton déjà triées (mêmes
plages contiguës, même ordre préfixe, donc **mêmes I, U et témoins saturés, mêmes descentes**). Compteurs déterministes
d'abord (local, par le développeur), puis G4.

**Mesure G4 nommée : `claudecens1`** — A/B (a) puis (a) + radix ; ledger census publié par arité ; W1/W8 en paires,
W48 confirmation. **Critère** : garder (a) si nœuds **et** tests baissent d'au moins 30 % ; garder le radix si la
médiane appariée de `regular_ns` W1 baisse d'au moins 10 %.

**Ce qu'on peut attendre.** −0,67 à −0,75 s de CPU W1 sur ng00 au mieux (E) ; **−5 à −25 ms** de FULL à W48 selon que
la résolution reste critique après T2 : **FULL ng00 ≈ 340–400 ms**. Si le census coûte encore plus de 1,5 µs par appel,
préparer M5 (k-d spatial, contrat « k plus proches ») pour T7.

### T4 — Feuille J3, par sous-tranches à dumps identiques

**Contenu.** Dans l'ordre de [F:v10_moteur] : (a) census par masques (`domby` rempli dans `prepare`, masque
d'extérieurs par profondeur, saut dans `census_and_emit`) ; (b) triangle médian avant la sphère q3, enveloppe du
tétraèdre avant `q4_of` ; (c) compteurs et q3 différé (faits en T2) ; (d) droite i64 à termes de paire, **sur A/B
seulement** ; (e) étages + table H à la place du DFS et du cache J2 pour m ≤ 32, le DFS restant référence et chemin des
feuilles larges — **à départager par le microbanc M2** contre la « feuille plate par lots » de [CG] G07–G08 (critère
de [PR] M2 : la variante par lots filtrés sous 0,6 fois la variante à droite exacte, zéro désaccord, repli exact non
vide). Feuille 24 à K = 10 (v10_moteur_04) dans la même tranche. Nouveau contrat de ledger à demander aux auditeurs
avant (e) (`QUESTION_CLAUDE_*`), puisque `prefixes` et `region_line_*` changent de sens.

**Mesures G4 nommées : `claudeleaf1`** (a), **`claudeleaf2`** (b, d), **`claudeleaf3`** (e et feuille 24 à K10),
**`claudeleafm2`** (microbanc M2 sur les candidats vidés en T0, AVX-512). **Critère** par sous-tranche : médiane
appariée de `single_pass_ns` W1 en baisse d'au moins 5 %, confirmée à W48 et W24 ; dumps identiques ; Euler K+2 vert.

**Ce qu'on peut attendre.** Passe unique ×0,67–0,77 (feuille ×1,37–1,65, E) ; **−20 à −40 ms** au-delà de T2,
**−15 ms** si le gain SMT fond à 48 fils : **FULL ng00 ≈ 300–380 ms** ; K = 10 : catalogue ÷1,6–1,8 (feuille 24) puis
÷1,55 (J3) (E).

### T5 — Forêt sans lots et verticales par requêtes

**Contenu.** D-F1 : noyau union-find sans lots par ordre, matérialisation **parallèle** des plateaux, numérotation
(rang, plus petite naissance) identique ; T4–T6 de [CT] inscrits au registre `docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md`
avant le port. Verticales par chaînes lourdes (v7-2) **ou** historique d'attache (D-F3), une seule des deux. Bras
K = 10 avec certificats MSF composables (v7-1). Réécrire seulement si l'ablation bornante de T0 montre que la
matérialisation pèse plus de ≈ 15 ms dans la chaîne de l'ordre 5.

**Mesure G4 nommée : `claudeforest1`** — en étage séparé puis en pipeline : (1) publication actuelle ; (2) D-F1 ;
(3) D-F1 + MSF, J = 48 et 256 régions ; K = 5 **et** K = 10, trois trames, W48 ; égalité octet pour octet ; TSan.
**Critère** : D-F1 retenu si la publication de l'ordre 5 en étage séparé passe sous 20 ms à W48 ; MSF retenu selon la
règle du § 2.4.

**Ce qu'on peut attendre.** À K = 5 aujourd'hui **0 à −10 ms** (la queue a déjà été traitée en T2 et la résolution
reste plus longue que la publication) : FULL ng00 ≈ 295–380 ms. Ce qui change : le plancher séquentiel de la tour
tombe de 35–58 ms à ≈ 10–20 ms (E), condition pour que T3/T7 et les réécritures de T8 se convertissent en temps ; à
K = 10, −80 à −120 ms (E : publication de l'ordre 10 ≈ 119–146 ms séquentielle → 20–35 ms).

### T6 — Squelette séquentiel restant

**Contenu**, décidé sur les sous-chronos de T0 : préambule (tête v3b par tranches et noyau G1 sans branchement,
v10_moteur_03 ; plus la part non sûre de v2_v3-02 au-dessus d'un seuil de liste) ; arènes pré-touchées et réutilisées,
sans rendre au noyau de grandes réservations dans la fenêtre (C : aujourd'hui `Buffer` passe par `operator new` et
`operator delete` à chaque usage, `src/core/buffer.cpp` l. 32–48, d'où des fautes de page à chaque prise, [V:v4]) ;
table des supports et contextes chronométrés puis parallélisés là où l'accélération reste ≤ ×6 (v10_moteur_05) ; mode
résident `--repeat=R` publié **à côté** du froid (v10_tour_07).

**Mesure G4 nommée : `claudeskel1`** — W1/W24/W48, froid et chaud, `ru_minflt` par étage. **Critère** : préambule
≤ 8 ms, résidu + contextes + naissances ≤ 20 ms à W48, sorties et budget identiques.

**Ce qu'on peut attendre.** **−15 à −45 ms** (C) : FULL ng00 ≈ 260–365 ms à froid ; ≈ 3–7 % de moins en chaud (E,
v10) si l'utilisateur fait du contrat une latence de flux.

### T7 — Seconde vague sur la résolution

**Contenu.** `resolve1` si T0 compte au moins ≈ 10 % de pas hors table évitables (sinon mémo typé de [R5]) ; tables de
semis par ordre et étiquette dans la table des supports (M3 de [CT] : garder l'existant sous 15 %) ; `LeafOracle`
seulement si f ≥ 0,5 ; k-d « k plus proches » seulement si le census coûte encore plus de 1,5 µs par appel (M5).

**Mesure G4 nommée : `claudedesc1`** — chaque levier seul puis combiné, W1/W48, K5 et K10. **Critère** : médiane
appariée de `regular_ns` W1 en baisse d'au moins 5 % par levier.

**Ce qu'on peut attendre.** **−8 à −35 ms** (E/C) : **FULL ng00 ≈ 215–350 ms** à froid. C'est la **fin des
transpositions** : au mieux le niveau de la v10 (204–254 ms), ×2,2 à ×3,5 au-dessus de 100 ms.

### T8 — Point de décision : réécriture par lots ou GPU

Les sondes de T0 (c(L), candidats vidés) et les microbancs M2/M4/M5 doivent être joués **pendant** T2–T4, pas après :
ce sont eux qui disent si 100 ms est accessible. Deux routes, non exclusives, détaillées au § 4.4 :
- **CPU** : feuille par étages sur lots, filtres F6 semi-statiques à repli exact, noyaux AVX-512 ([CG] G04–G12),
  étage G réécrit ([CT] D-G1 à D-G5) ;
- **GPU** : sous-arbres du catalogue (filtre + feuille) sur l'appareil sous [R7].

**Mesures G4 nommées : `claudegpu0`** (sonde de débit propre, sans débordement signé, double, 64 × 64 → 64 et → 128 ;
transferts épinglés de 50 et 600 Mo ; M6 de [PR]) puis **`claudegpu1`** (noyau de sous-arbres sur les sous-arbres
vidés de ng00, transferts, retour et canonicalisation compris, contre la passe unique CPU). **Critère** ([V:auditeurs]
auditeurs-07, [PR] M8) : abandonner la route si le total appareil ne passe pas sous ≈ 40 ms à K5 ; zéro désaccord ;
replis `unresolved` comptés et bornés.

### Synthèse : ce qu'on peut attendre après chaque tranche

| Après | FULL 08/000000, K = 5, W48, froid | Étiquette | Ce qui change surtout |
| --- | ---: | --- | --- |
| aujourd'hui (b872) | **412** (ng01 352, ng02 381) | M | — |
| T0, T1 | 412 | — | vraie cible, bruit, cause de la queue, K10 mesuré, filets posés |
| T2 | ≈ 360–405 | E/C | queue de publication, q3, compteurs, arènes, racine G1 |
| T3 | ≈ 340–400 | E | census des descentes |
| T4 | ≈ 300–380 | E | feuille J3 (et feuille 24 à K10) |
| T5 | ≈ 295–380 | E/C | plancher séquentiel de la tour levé ; K10 −80 à −120 ms |
| T6 | ≈ 260–365 | C | squelette (préambule, résidu, contextes) |
| T7 | **≈ 215–350** | E/C | résolution, seconde vague |
| T8 | seule route chiffrée sous 100 ms : conceptions 65–91 ms (trame 02, « sans marge »), ou GPU | C | voir § 4 |

ng01 et ng02 suivent dans la même proportion (≈ 0,85 et ≈ 0,92 fois ng00 aujourd'hui, M). K = 10 (C) : référence à
mesurer en T0 (≈ 1,5–2,5 s attendues), ≈ 0,6–1,0 s après T4, T5 et T7.

---

## 4. Jugement honnête

### 4.1 K = 5 sur les trois trames mesurées

- **Mesuré** : 352–412 ms (médianes), meilleure prise 327,7 ms ; aucune des 81 prises de [Q] sous 200 ms ; aucune
  version n'a fait 100 ms (v10 : 204–254 ms en passe chaude, [S4]).
- **Les transpositions seules ne suffisent pas** : ≈ 215–350 ms après T7 (E/C), ×2,2–3,5 au-dessus. Raison chiffrée :
  elles rendent ×1,3–1,6 sur la feuille (J3 : ×1,37 mesuré sur la v10, ×1,65 au mieux sur la v11) et ×1,2–1,4 sur la
  résolution (census ÷3 → −20 à −22 % de la résolution W1 ; `resolve1` −13 % des pas difficiles), quand le budget
  exige ÷4,2–4,5 et ÷3–4 (§ 1.3).
- **Le travail ne l'interdit pas.** Plancher de la famille d'algorithmes (boîtes de centres puis descentes) ≈ 2 600
  cycles locaux par boule à K5, contre un budget de ≈ 9 900 ([PR] § 1, E : coûts unitaires supposés, comptes mesurés) ;
  les candidats par boule (48–58) sont un plancher de la méthode, pas un défaut de réglage ([PR] § 2.1, [L05] § 6.10,
  M-loc) : le chemin passe par le **coût par candidat**, pas par leur nombre.
- **La seule route chiffrée sous 100 ms** est celle des conceptions du 2 octobre : catalogue 18–30 ms ([CG] § 0, modèle
  de 650–800 cycles par boule) ou 40–54 ms ([PR] § 7), tour 22–32 ms ([CT] § 9), moteur **65–91 ms** sur la trame 02
  ([PR] § 7) — « sans marge », et **aucun de leurs coûts unitaires n'est mesuré**. Les seules mesures d'un noyau par lots
  sont locales et partielles : quadruplets ×8–11 par lots filtrés, triplets ×1,5–2,0, recensement en double : aucun gain
  ([L05] § 6.7, M-loc). Le seul filtre flottant essayé dans la v11 (F6 sur les signes du census) n'a rendu que ≈ 1 % :
  il ne changeait ni le parcours ni la structure à branchements, il ne réfute donc pas les noyaux par lots.
- **Verdict K = 5** : **non démontré, pas exclu**. Inaccessible par les transpositions ; accessible sur le papier si
  la feuille par lots vectorisée **et** l'étage G réécrit tiennent leurs coûts unitaires (mesurables tôt : M2, M4, M5),
  ou si la passe unique part sur le GPU par sous-arbres **et** la tour CPU est divisée par 3–4. Je ne donne pas de
  probabilité : aucune borne inférieure ne la fonde (leçon des verdicts v9 réfutés par la v10, [V:auditeurs]).

### 4.2 Au pire de la plage déclarée (30 000–60 000 sites)

Sur les 127 trames c08 sans sol de [PTS4] (échantillon choisi pour ses objets, non aléatoire), les nœuds d'ordre 5
croissent comme n^1,39 ; entre 50 000 et 60 000 sites ils valent ×1,22–1,31 (masse) à ×1,87 (c08_003412) ceux de
08/000000 (M sur les comptes). Si le temps suit ce travail (E), il faut **54–77 ms sur 08/000000** ; même les cibles
des conceptions (65–91 ms sur 02) donneraient alors ≈ 85–170 ms sur la trame la plus lourde (E). Deux issues : la route
GPU pour le catalogue **plus** une tour CPU réécrite, ou un contrat redéfini (médiane, ou plage 30–50 k). En outre,
**85 des 127 trames c08 sans sol dépassent 60 000 sites** : la plage déclarée exclut une bonne part des trames réelles
de cette séquence (constat à remonter, [V:auditeurs]).

### 4.3 K = 10

- **Jamais mesuré dans le contrat.** Attendu ≈ 1,5–2,5 s (C : rapport v10 K10/K5 ×4,2–4,4 ; feuille 16 codée en dur
  pour tout K dans `bench/points_export.cpp` l. 377, ×1,6–1,8 sur le catalogue d'après la v10 ; les campagnes K1..10
  hors contrat prennent 43–50 s par trame de ≈ 40 000 sites à quatre fils sous 22 processus simultanés, [PTS4]).
- Après les transpositions (feuille 24, J3, census, D-F1, MSF, verticales par requêtes, `resolve1`) : **≈ 0,6–1,0 s** (C).
- **Plancher argumenté** : 5,48 M boules et 7,47 M nœuds sur 02 ; 100 ms laissent ≈ 2 600 cycles locaux par boule
  pour tout le moteur, contre un plancher de famille estimé ≈ 3 000 avant toute perte de parallélisme ([PR] § 1, E) ; le
  plancher séquentiel de la tour v10 vaut à lui seul 89–140 ms ([S4], M) ; la tour CPU réécrite est estimée 125–180 ms,
  dont l'étage G 75–130 ms ([CT] § 10, E) ; le catalogue 80–125 ms ([CG]) ou 150–202 ms ([PR]) ; un seul ordre K10
  160–230 ms ([PR] § 4.3, E, et ce n'est plus le contrat FULL).
- **Verdict K = 10 : non**, par aucun chemin chiffré en CPU. Seul un moteur entier sur l'appareil (catalogue **et**
  tour) pourrait changer d'échelle : c'est un second moteur égal octet pour octet au premier, de la recherche, pas un
  plan ([PR] § 5.3). Cible honnête : mesurer (T0), viser **0,3–0,4 s** ([PR] § 6), annoncer ce qui est mesuré.

### 4.4 Ce qu'il faudrait de plus

**(a) Algorithme et implantation CPU** (conçus le 2 octobre, jamais implantés ni mesurés) :
- feuille par étages sur lots d'indices, noyaux sans branchement séparés par une compaction, filtres de signe F6
  semi-statiques à repli exact pour les degrés 4 à 6, intérieur strict du tétraèdre par réemploi du centre de Cramer,
  recensement vectorisé ([CG] G07, G08, G10, G11, G12 ; PO-R2, PO-R3 de [PR] à prouver) ;
- nœuds à masques pour les listes ≤ 32 sites, filtre par dominance de toutes les paires (dumps identiques, nœuds −3,4 %,
  M-loc, [CG] G04) et réservoir vectorisé au-delà (G05) ;
- tri par seaux et une seule passe de remplissage (G15 : ordre et assemblage 33 → 2–4 ms à K5, E) ; catalogue en ordre
  d'émission et ordre canonique en permutation ([PR] R2.2 : −5 à −8 ms à K5, E) ;
- étage G réécrit : `resolve1`, certificat combinatoire de plus petite boule (lemme T1 : 76 % des plus petites boules
  sans géométrie exacte), requête bornée aux k plus proches sur index implicite ([CT] D-G1 à D-G5, D-I1).

**(b) GPU** (feu vert de l'utilisateur sur G4, contrat [R7]) : sous-arbres du catalogue, pas feuilles seules (c(16)
= 52 % resterait sur CPU, plafond ×1,9) ; si c(64) ≈ 15 % se transpose à l'arbre T0, la passe unique garderait
≈ 25–35 ms de CPU (C). Exigences : exact sur l'appareil ou `unresolved` repris sur CPU **avant admission**, avec
134–204 bits pour les puissances et niveaux q3 en u21/u24 (« centres i128 ne signifient pas catalogue i128 », [R7]) ;
capacités comptées sur l'appareil (count–scan–emit), jamais de pilotage hôte par tuile ; replis comptés et bornés par
une porte ; contexte et processus chauds déclarés ; lanceurs factices hostiles avant toute session payante ; témoin
device en phase 0, `-fmad=false`, sans FTZ ([V:produit] C1–C4, [F:v5] v5-I3). Les débits de la sonde v10 S7
(2 579 et 1 111 G op/s) sont **invalides** (boucles à débordement signé, `morsehgp3D_v10/receipts/ERRATA.md`) : seule
l'exactitude du comparateur `cmp128` est qualifiée. Même un catalogue gratuit laisse aujourd'hui FULL à 185–232 ms
(FULL moins passe unique, E sur [AB7]) : le GPU est nécessaire à la route « pire de la plage », jamais suffisant seul.

### 4.5 Décisions à demander à l'utilisateur

1. **Froid ou chaud** : 100 ms est-il la latence par trame d'un flux à processus résident (10 Hz), ou un processus neuf
   par trame ? La v10 publiait sa troisième passe chaude ; la v11 mesure à froid. Publier les deux, ne jamais
   substituer l'un à l'autre.
2. **Maximum ou médiane de la plage** 30 000–60 000 sites, et faut-il y ajouter d'autres séquences que la 08 ?
3. **Route GPU** : l'ouvrir dès que c(L) et `claudegpu0` la justifient (en parallèle de T3–T4), sans attendre la fin
   des transpositions.
4. **K = 10** : accepter une cible annoncée de 0,3–0,4 s plutôt que 100 ms.

---

## 5. Fausses bonnes idées à ne pas refaire

| Idée | Source | Pourquoi ne pas la refaire |
| --- | --- | --- |
| Mémo de descente et mémos de lane (bit 4) | [C11] § 5.3, `receipts/full_memo_20261003/memo1/` | 2,6 % de succès en lanes ; incompatibles avec le pipeline ; retirés de 16379 |
| Filtre F6 flottant des signes de `power` et de ses bornes dans le census | `docs/PERFORMANCE_FULL.md` l. 186–188, session `claudeab1` | ≈ 1 % du CPU : le coût est le parcours, pas l'arithmétique ; retiré. Le levier est le serrage des bornes et la forme de l'index (T3) |
| `-march=x86-64-v3/v4` sur tout le code | [PROF1] | dans le bruit ; seul un noyau écrit sans branchement en profite |
| Préchargement des états DSU de la publication | note d'audit du 3 octobre § 7 | aucun gain mesurable ; retiré |
| Sous-maille T6 des centres pour le LiDAR (verrou 6) | `preuves_l05_code_catalogue/ablation_kT.txt` | dumps identiques, travail ±0,3 % pour T = 6/3/0 ; copier kT = 6 en u24 casse la garde 2(B+T)+5 ≤ 63 ; utile seulement aux grilles synthétiques denses |
| `SiteTree` de la v10 tel quel | N1 de [C10] ; `site_tree.cpp` l. 64 | marge flottante figée (`kMargin` = 0,02) pour u18, arrondi supposé : hors F1–F6 ; son « ≈ 1 µs » est un parcours non certifié |
| Boule fermée entière à chaque saut | L06-05 | Θ(n) : jusqu'à 1 258 sites sur une trame, la moitié du nuage en contraste |
| MEB double Welzl certifiée ; MEB à quatre pivots ; « support + extérieur » | `tower.cpp` v10 l. 250–547 ; v8 ; [F:auditeurs] § 3 | `bounded_meb` ≈ 0,9 % (2,6 % inclusif) du CPU v11 ; la variante support + extérieur régresse (341 → 392 présentations) |
| Kruskal par lots de la v10 et pointeurs de saut | L06-04 | plancher séquentiel ; 2,6–3,2 fois plus lent que le noyau sans lots ; à garder comme référence de porte |
| Frontière en largeur à barrières, grain fin de 19 482 tâches | [S4], N2 de [C10] | ×2,1 de 1 à 48 fils ; le LPT v11 est à 7 % de l'idéal en simulation |
| Feuille « plate » sans droite des centres **sans** noyaux par lots | [PR] § 2.2 | ×2,4 de quadruplets ; 0,99 → 1,11 s à K5 au coût actuel du quadruplet : négative ; ne se décide que par M2 |
| « Lot D » de filtres flottants avant J3 | [L05] Q2 | gain propre ×1,06–1,11 contre l'ordre exact réordonné que la v11 a déjà (`Q4Candidate`) |
| Émission de 16 octets, niveau recalculé depuis S* | [L05] § 6.5 | 0,78 niveau distinct par boule : le recalcul ne s'éviterait presque jamais |
| Crédit de groupe par moments, palette ponctuelle | v10 `group_moments_20260930` ; [F:auditeurs] § 3 | ombre négative sur rectangles lourds en v9 ; aucun compte LiDAR |
| Certificats scalaires l/u dans le filtre G1 (cellules de centres v3) | [F:v2_v3] § 3 (simulé, compteurs = AB7) | −38 % de tests de paires, mais 230–320 ms de CPU W1 pour 279 ms évités |
| Minorant commun des extensions q4 | revues indépendantes 7 et 8 | signes à 153–201 bits par site et par triplet pour ≈ 0,33 candidat évité |
| Index des selles, saut au centre, catalogue scellé (v9) | `morsehgp3D_v9/receipts/saddle_index_negative_20260923` | mesurés négatifs ou sans objet en v11 |
| Saut par orthants | v10 `AUDIT_ETAT_COURANT.md` l. 651–661 | 1,32 pas par trace : le coût est par pas, pas leur nombre |
| Partager la géométrie entre les K forêts | v9 `FULL_PARTAGE_INTER_ORDRES_20260926.md` | une boule régulière ne travaille qu'aux ordres m − 1 et m |
| Publication par le constructeur min-label de la v9 (Borůvka par lots) | `morsehgp3D_v9/audits/b_full_a_real_20260927/RESULTATS.md` | 1 264,7 ms à K5 sur ng00 (M-loc) contre 36–52 ms ; perd le recouvrement ; garder le seul lemme des identifiants |
| Ordres par K décroissant sans noyau rapide ; forêt par diviser pour régner | [AB7] W1 par ordre ; TOWER_v2 § 6.5 | déplace la queue vers les ordres bas ; D&C 5 à 10 fois le travail du noyau |
| Réduction segmentée de la publication par blocs de Morton | [PR] R2.6 | 31–45 % des fusions restent à la couture séquentielle : gain borné ×2,2–3,2 |
| Feuilles seules sur GPU ; census GPU d'un étage isolé ; tout-device matérialisé ; frontière Morton–Yao48 du produit | GPU v10 § 3 ; v7 (noyaux 29,8 ms, étage 846 ms) ; v5 (plus lent, puis parité) ; produit (2,4 s ; 7,0–7,5 s dont 4,8–5,4 s de recertification) | plafond ×1,9 ; un noyau n'est pas un étage ; l'hôte mange le gain ; recertifier détruit l'étage |
| Étage G sur GPU | TOWER_v2 § 14 ; [PR] § 5.2 | « projection, pas promesse » ; rien à gagner à K5 |
| Moteur résident chaud présenté comme gain du contrat | v9 `g4_core_warm` (≈ 2 %) | petit, et ne vaut que si le contrat est une latence de flux |
| Médianes indépendantes de cinq prises W48 ; comparer des constantes entre processus | [F:v4] (σ 0,11–0,14 ; 29–44 paires pour 5 %) ; pistes fermées v4/v5 | ne tranchent pas un levier de 3–15 ms ; utiliser rapports appariés, test des signes, A/A, W1/W8 |
| Projeter en CPU·s/48 ; verdict d'impossibilité sans borne inférieure | v9 `AUDIT_C_ALTERNATIVES_CONTRAT_LIDAR_G4_20260923.md` l. 174, 538–543 ; contre-audit B l. 17–25 | SMT ×1,22–1,75 de 24 à 48 fils ; les verdicts v9 ont été réfutés par la v10 |
| Recalibrer la feuille à K5 ; marche sur le squelette de Voronoï ; q3/q4 engendrés par les q2 ; balayage de la droite des centres ; successeur fourni par le générateur ; forêt pendant l'émission | [PR] R1.2–R1.5, R2.3, R2.6 | écartés sur mesure le 2 octobre (16 est l'optimum ; hérédité absente pour 9–18 % ; certification de 13 % des premiers pas seulement) |
| Élaguer un préfixe par le **rang** de ses sous-supports | v2 R3v2 (0,63 % de boules perdues), v3 F64 | le rang n'est pas héréditaire ; les fixtures de T1 tuent ce mutant |
| Un seul ordre comme contrat | [PR] § 4 | change l'objet du contrat FULL ; chemin de produit à part |

---

## 6. Sources lues pour ce plan

Cartes `../cartes/CARTE_V11.md`, `../cartes/CARTE_V10_VITESSE.md` et `CARTE_V11_derive.py` (rejoué) ; les douze
fouilles `../fouille/*.md` et les douze contre-vérifications `../verif/*.md` (avec `../verif/v8_annexes/`) ; v11
`origin/main` : `audits/QUESTION_CLAUDE_VITESSE_100MS_20261004.md`, `audits/AUDIT_CONTRATS_NUMERIQUES_ET_CAPACITE_20261002.md`
(réponses R1–R7), `docs/CONCEPTION_MOTEUR.md` § 4–7, `docs/PERFORMANCE_FULL.md`, `docs/DEVELOPPEMENT.md`,
`src/tower/forest_pipeline.cpp`, `src/tower/forest_concurrent.cpp`, `src/sched/pool.cpp`, commit `56216392e` ;
reçus [AB7] (quinze prises W48 et prises W1 relues), `receipts/developpement_20261003/pipeline_g4/README.md` ;
conceptions `build/v11-persist/conception/CONCEPTION_TOUR.md` (§ 0, 3, 4, 9–11), `PISTES_DE_RUPTURE.md` (en entier),
`CONCEPTION_GENERATEUR.md` (§ 0–1) ; `build/v11-persist/audit_v10/L05_CODE_CATALOGUE.md` § 6.2, 6.7–6.10 ;
`morsehgp3D_v10/receipts/g4_session7_cuda_probe_20260929/README.md` et `morsehgp3D_v10/receipts/ERRATA.md` ;
`gcp-migration/v11_session.py` (en-tête). Aucune commande GCP, aucune construction, aucun binaire exécuté.

FIN
