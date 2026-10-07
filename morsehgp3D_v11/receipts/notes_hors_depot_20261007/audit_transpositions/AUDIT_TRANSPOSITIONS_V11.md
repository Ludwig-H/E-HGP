# Audit des transpositions vers la v11 : verdict final

4 octobre 2026, rédigé de 13 h 52 à 13 h 57 UTC (heures lues par `date -u`). Juge final de l'audit géant des
transpositions demandé par l'utilisateur ([`CONTEXTE.md`](CONTEXTE.md)). Ce document tranche entre deux cartes
(`cartes/`), douze fouilles (`fouille/`), douze contre-vérifications adverses (`verif/`) et deux plans (`plans/`),
recoupés avec le dépôt lu à 13 h 49 UTC : `origin/main` = `0c358261c`.

```text
phase=exploration_v11_hors_registre (audit, lecture seule)
backend=cpu_reference
profile=quantized_u21_input_only (mesures v11) ; quantized_u18_input_only (mesures v10)
public_status=not_claimed
GCP non utilisé ; aucune construction ni exécution native ; aucune commande git qui écrit
```

Rappel de l'utilisateur : **« l'objectif du contrat est toujours 100 ms »** — tour HGP FULL K = 1..5 des trames
SemanticKITTI sans sol (30 000 à 60 000 sites), grille 1 mm, moteur entier exact, G4 à 48 fils ; K = 10 si possible.

Étiquettes : **M** mesuré (reçu G4 nommé ; **M-loc** = mesure locale, qui ne vaut que pour des rapports), **E** estimé
(calcul sur des mesures), **C** conjecturé. **Aucun gain annoncé ici n'est mesuré sur la v11** : chacun attend un A/B
G4 à sorties identiques octet pour octet.

Contrôles refaits par le juge : `cartes/CARTE_V11_derive.py` et `fouille/v4_apparie_ab7.py` rejoués sur les reçus
G4 (mêmes chiffres) ; relus à la source : mesures de la feuille J3 (`ab_abba_t1.txt`), calibrage v9 des compteurs,
coût du constructeur min-label v9, journal du noyau de forêt (`noyau_v11.log`), errata des reçus v10, budget par boule
de `PISTES_DE_RUPTURE.md`, registre des preuves (ligne 240), et les sept commits v11 publiés depuis 13 h 06.

---

## 1. En bref

1. **Verdict 100 ms (K = 5) : aucune transposition, ni toutes ensemble, n'y mène** ; elles ramènent FULL de 352–412 ms (médianes G4 du dernier moteur mesuré) vers ≈ 215–350 ms sur 08/000000 (estimé), au mieux le niveau de la v10 (204–254 ms), qui n'a elle-même jamais approché 100 ms.
2. **Pourquoi la v10 allait plus vite** : même objet, même travail logique (56,6 candidats par boule, mêmes boules, mêmes pas à 9 % près), même parallélisme ; la v11 paie plus cher chaque unité de travail dans deux étages, les feuilles du catalogue (+58 à +88 ms : CPU ×1,27 à un fil, passage à 48 fils ×23–28 contre ×34) et les descentes de la tour (+64 à +88 ms : ×3,2 par pas ; recensement exact ≈ 3 µs contre ≈ 1 µs pour une boule fermée v10 calculée en flottant non certifié).
3. **À transposer d'abord : la feuille J3 de la v10**, restée privée dans un reçu (×1,37 à K = 5 et ×1,55 à K = 10 sur la v10, mêmes sorties ; −25 à −55 ms estimés) ; le développeur l'a commencée (recensement par masques porté, triangle médian et enveloppe du tétraèdre en cours).
4. **Ensuite le recensement des descentes** par bornes exactes sur réseau (v6, v7, v8) et un arbre radix sur l'ordre de Morton (travail ÷3 en simulation, mêmes descentes, −5 à −25 ms), puis la **forêt sans lots** à matérialisation parallèle (conception tirée de la v10), seule à lever le plancher séquentiel de 35–58 ms de la publication de l'ordre 5.
5. **Le gros levier immédiat n'est pas une transposition** : l'ordonnancement du pipeline des forêts ; deux prises de 08/000000 sans queue de publication font 373,7 et 391,1 ms au lieu de 412–464 ms ; il faut d'abord horodater le début de chaque tâche.
6. **Petits leviers** (niveau q3 différé et compteurs locaux déjà portés, arènes par tâche, racine du filtre sautée, arrêt des descentes à la première cellule) : quelques millisecondes chacun ; la v9 a mesuré qu'ôter les compteurs vérifiés ne rendait que 0,9 à 4,4 % du CPU de l'étage.
7. **La cible est plus dure qu'annoncé** : dans la plage de 30 000–60 000 sites, une trame de 50–60 k porte ×1,3 (médiane) à ×1,87 (pire) le travail de 08/000000, soit une cible effective de 54–77 ms sur 08/000000 (÷5,4 à ÷7,7 au lieu de ÷4,1) ; et 85 des 127 trames c08 sans sol mesurées dépassent 60 000 sites.
8. **100 ms à K = 5 : non démontré, pas exclu** ; le travail minimal estimé de la méthode tient environ quatre fois dans le budget, mais seules deux routes jamais mesurées y descendent : la réécriture CPU par lots vectorisés conçue le 2 octobre (cible 65–91 ms « sans marge ») ou le catalogue sur le GPU de la G4 par sous-arbres, avec une tour divisée par 3 à 4.
9. **K = 10 : 100 ms hors de portée** par tout chemin chiffré ; jamais mesuré dans le contrat (feuille de 16 sites codée en dur, alors que la feuille 24 de la v10 gagne ×1,6–1,8 sur ce catalogue) ; viser 0,3–0,4 s après une première mesure.
10. **Avant de réécrire** : poser les filets qui manquent à l'échelle (Euler à K+2 et restriction, épingle du mode de référence, ordre 1 contre l'arbre couvrant minimal, différentiel v10 sur trames entières, fixtures) et un protocole apparié (bras A/A, médiane des rapports, test des signes), car à 48 fils cinq prises ne tranchent rien sous ≈ 30–40 ms ; à décider par l'utilisateur : froid ou flux résident, maximum ou médiane de la plage, ouverture de la route GPU, cible annoncée à K = 10.

### Lexique

- **FULL** : la tour entière, ordres 1 à K, chronométrée par `bench/full_probe.cpp` (index + domaine + forêts), dans
  un processus neuf.
- **Passe unique** : la génération du catalogue de boules (arbre de boîtes de centres, feuilles de 16 sites au plus).
- **Résolution régulière** : les descentes de la tour, qui rattachent chaque face de cellule à une naissance.
- **Recensement** (*census* dans le code) : liste exacte des sites intérieurs et de la coquille d'une boule.
- **Publication** : construction des forêts par plateaux, aujourd'hui une tâche séquentielle par ordre.
- **W1, W24, W48** : nombre de fils ; la G4 a 24 cœurs et 48 fils.
- **A/B apparié** : deux variantes jouées prise contre prise ; **bras A/A** : le même binaire contre lui-même, pour
  mesurer le bruit.
- **Froid / chaud** : un processus neuf par trame / un processus résident qui réutilise sa mémoire.
- Trames du contrat : **ng00** = 08/000000 (39 885 sites), **ng01** = 08/000100 (35 551), **ng02** = 08/000200
  (45 845).

---

## 2. Pourquoi la v10 était plus rapide

### 2.1 Ce qui est mesuré

K = 5, 48 fils, millisecondes. v10 = `777406b82`, troisième passe chaude d'un même processus (reçu
`morsehgp3D_v10/receipts/g4_session4_j2c_20260929/`) ; v11 = `b87285378`, médianes de cinq processus neufs (reçu
`morsehgp3D_v11/receipts/developpement_20261003/pipeline_g4/sessions/claudeab7/`). Protocoles différents : l'écart est
descriptif, pas un A/B causal ([`cartes/CARTE_V10_VITESSE.md`](cartes/CARTE_V10_VITESSE.md) § 3.1, **M**).

| Étage | v10 ng00 / ng01 / ng02 | v11 ng00 / ng01 / ng02 | Écart v11 − v10 |
| --- | --- | --- | --- |
| frontière / préambule du catalogue | 21,8 / 23,2 / 23,0 | 20,7 / 18,4 / 20,8 | ≈ 0 |
| **boîtes / passe unique** | 106,7 / 84,8 / 101,5 | **195,0 / 167,0 / 159,5** | **+88 / +82 / +58** |
| ordre, tri, assemblage, reste du domaine | 35,0 / 28,9 / 39,8 | ≈ 36 / 31 / 38 | ≈ 0 |
| supports, atlas, semis / classification, naissances | 10,4 / 9,3 / 11,5 | 11,0 / 9,1 / 14,3 | ≈ 0 |
| **descentes / résolution régulière** | 28,6 / 22,2 / 27,1 | **116,6 / 86,6 / 99,3** | **+88 / +64 / +72** |
| Kruskal + verticales / queue de publication | 49,3 / 35,7 / 49,0 | 33,5 / 20,8 / 34,1 | −16 / −15 / −15 |
| **FULL** | **252,0 / 204,2 / 253,6** | **412,4 / 351,7 / 380,7** | **×1,64 / ×1,72 / ×1,50** |
| CPU par passe (CPU·s) | ≤ 8,6 / 6,9 / 8,3 | 13,7 / 10,3–10,8 / 12,9–13,0 | ×1,5–1,6 |

À un fil sur ng02 (**M**) : boîtes v10 3 508,5 ms contre passe unique v11 4 461,5 ms (×1,27, dont ×1,06 dû au profil
u21) ; descentes 851,7 ms contre 2 918,5 ms (×3,43) pour 4,53 M contre 4,89 M pas, soit 188 ns contre 597 ns par pas
(714 ns sur ng00). K = 10 en v10 : 861–1 125 ms ; jamais mesuré en v11 dans le contrat.

### 2.2 Même objet, même travail

Sur ng00 (**M**, reçus S4 et AB7) : 1 306 696 boules ; 783 071 nœuds de boîtes contre 781 865 ; 3,27 M candidats
jugés ; **56,6 candidats testés par boule dans les deux** (24,9 dominances, 23,9 droites, 7,9 quadruplets) ;
3,62 M représentants résolus ; 291 515 recensements contre 285 534 boules fermées. Le parallélisme moyen est le même
(CPU/mur ≈ 33–34 en v10, 29–34 en v11). **La v10 n'était pas plus rapide parce qu'elle calculait moins** : elle
payait moins cher chaque unité de travail.

### 2.3 Les mécanismes, et leur état dans la v11 au 4 octobre, 13 h 49

| Mécanisme v10 (source) | Effet mesuré | État v11 |
| --- | --- | --- |
| Compteurs nus dans la boucle chaude des feuilles (`generator.cpp`) | v11 : ≈ 0,5 milliard d'additions vérifiées par trame, `extend` = 16 % du CPU W1 (**M**, profil AB7) | **porté** à 13 h 47 (`0c358261c`, compteurs locaux vidés une fois par feuille, dumps identiques) ; non mesuré sur G4 |
| Niveau q3 calculé après admission (`emitted_level`, l. 196–221) | v11 : `Sphere::through` 2,4–3,0 % du CPU | **porté** (`56216392e` : 9,3 M niveaux évités sur ng00) ; non mesuré sur G4 |
| Feuille par étages et table des triplets vivants (l. 319–465) | v11 : 43,7 M consultations de cache de droites en plus, `center_line_meets` 5 % du CPU | non porté ; contrat de compteurs demandé aux auditeurs (question D, `49831e9e9`) |
| Aucune allocation ni opération atomique partagée par nœud | v11 : deux CAS et un `fetch_sub` partagés par nœud (`Buffer::allocate`) ; à W48 `buffer_*` 0,86 %, fautes de page 3,1 %, verrou noyau 1,55 % du CPU (**M**, PROF1) | non porté (contrat R3 accepté par les auditeurs) |
| Grain fin (19 482 tâches) | boîtes v10 ×34 de 1 à 48 fils, passe unique v11 ×23–28 | partiel (≤ 1 024 tâches, plan LPT à 7 % de l'idéal en simulation) ; la part de chaque cause n'est pas mesurée |
| Boule fermée par arbre k-d serré (`site_tree.cpp`) | ≈ 1,06 µs par requête en v10 contre ≈ 3,1 µs par recensement v11 (parcours seul, **E**) | **non transposable tel quel** : parcours en flottant avec une marge figée pour u18 (`kMargin = 0,02`), arrondi supposé ; seul l'écart de structure (index et bornes de boîte) se transpose (idée V3) |
| Mémo partagé par cellule (`tower.cpp` l. 951–991) | 307 177 arrêts à K = 5 (7 % des pas), 2,14 M à K = 10 (**M**) | retiré (mémos de lane à 2,6 % de succès, incompatibles avec le pipeline) ; d'où +8 % de pas |
| Semis H_k consultés avant toute MEB | 86 % des chaînes finissent sur un semis | **porté et étendu** (table de populations, voie liée : −26 à −30 % de résolution à W1, **M**) |

Ce que la v11 fait **mieux** : publication recouverte par la résolution (queue de 21–34 ms contre 36–49 ms de Kruskal
et verticales en v10), MEB exacte bornée (≈ 1 % du CPU contre 28 % des cycles de `resolve` en v10), niveau q4 différé,
graphe de paires, budget mémoire honnête, mesures en processus neufs.

**Ce qui n'explique pas l'écart** : `-march` (dans le bruit sur la v11), l'index (0,4 ms), l'échauffement (la première
passe v10 ne coûtait que 3–7 % de plus), le profil u21 (+5–6 % sur le catalogue), la MEB.

### 2.4 Ce que la v10 enseigne pour les 100 ms

- **La v10 n'a jamais approché 100 ms** : 204–254 ms à K = 5 ; son CPU (≤ 6,9–8,6 CPU·s) donnerait encore 143–178 ms
  avec un parallélisme parfait (**E**). Rattraper la v10, c'est ÷1,5–1,7 ; 100 ms, c'est ÷3,5–4,1.
- **Budget à W48, K = 5** ([`plans/PLAN_VITESSE_100MS.md`](plans/PLAN_VITESSE_100MS.md) § 1.3 ; mesures **M**,
  cibles **E**) :

| Étage | Mesuré ng00 / ng01 / ng02 (ms) | Cible pour 100 ms | Facteur |
| --- | --- | ---: | ---: |
| Préambule du catalogue | 20,1 / 18,8 / 20,7 | ≤ 5 | ÷4 |
| Passe unique | 180,3 / 167,0 / 170,2 | ≤ 40 | ÷4,2–4,5 |
| Compactage, tri, assemblage | 26,0 / 20,8 / 29,3 | ≤ 8 | ÷3 |
| Résidu du domaine (table des supports, libérations) | 13,4 / 11,8 / 13,9 | ≤ 3 | ÷4 |
| Contextes, classification, naissances | 20,1 / 16,9 / 25,7 | ≤ 7 | ÷3 |
| Résolution régulière | 115,8 / 86,4 / 96,4 | ≤ 30 | ÷3–4 |
| Queue de publication | 36,2 / 29,7 / 23,9 | ≤ 4 | ÷6–9 |
| **FULL** | **412,4 / 351,7 / 380,7** | **≤ 100** | **÷3,5–4,1** |

  En CPU : au plus 2,0–2,2 s de CPU à un fil par trame, au lieu de 7,1–9,1 s. **Hors passe unique et résolution, le
  reste pèse déjà 98–116 ms** : rendre gratuits les deux grands étages ne suffirait pas.

---

## 3. Les idées retenues

Classement : **vitesse d'abord**, par gain attendu à W48 pondéré par la solidité de la preuve ; puis les **filets** sans
lesquels une réécriture n'est pas sûre ; puis la piste **points**. Plusieurs sources donnent souvent la même idée : elle
ne compte qu'une fois. Verdict : celui de la contre-vérification adverse (`verif/`), suivi de celui du juge quand il
diffère. « État » = dépôt lu à 13 h 49 UTC.

### 3.1 Tableau de synthèse

| Rang | Idée | Source principale | Verdict | Gain W48, K = 5 (K = 10) | Coût / risque | Où dans la v11 | État |
| ---: | --- | --- | --- | --- | --- | --- | --- |
| V1 | Feuille J3 par tranches | feuille J3 privée de la v10 | garder | −25 à −55 ms (**E**) ; K = 10 ×1,55 sur l'étage (**M-loc**) | élevé / moyen | `src/catalogue/leaf.cpp` | (a) porté, (b) en cours |
| V2 | Ordonnancement du pipeline des forêts | constat d'audit sur AB7 ; instrument v9 | à mesurer | 0 à −30 ms (**E**) | faible / interblocage | `src/tower/forest_pipeline.cpp`, `src/sched/pool.cpp` | rien |
| V3 | Recensement : borne exacte sur réseau + arbre radix Morton | v6, v7, v8 ; audit d'héritage v11 | garder | −5 à −25 ms (**E**) ; plus à K = 10 (**C**) | faible / faible | `src/index/`, nouvel assistant `src/num/` | rien |
| V4 | Forêt sans lots, matérialisation parallèle, verticales par requêtes | TOWER_v2 v10, conception v11 D-F1, v6, v7, v9 | à mesurer (haute) | 0 à −10 ms aujourd'hui ; plancher 35–58 → 10–20 ms ; K = 10 −80 à −120 ms (**E**) | élevé / moyen | `src/tower/forest_plateau.cpp`, `forest_concurrent.cpp`, `forest_vertical.cpp` | rien |
| V5 | Compteurs locaux (R1) et arènes par tâche (R3) | v10 G4/G6 ; contrats R1, R3 | à mesurer | −2 à −7 ms + 0 à −8 ms (**E/C**) | faible / faible | `leaf.cpp`, `boxes.cpp` l. 60, `core/buffer.cpp` | compteurs portés |
| V6 | Préambule : racine du filtre sautée, tête par tranches, noyau sans branchement | v3 (lemme), frontière v3b de la v10 | à mesurer | −1 ms sûr ; −5 à −20 ms (**C**) | moyen / course | `adaptive_frontier.cpp`, `adaptive_prepare.cpp`, `boxes.cpp` | rien |
| V7 | Arrêt à la première cellule (`resolve1`) ou mémo de cellule daté | mémo v10, D-G1, contrat R5 | à mesurer | −5 à −12 ms (**E**, borne haute) ; plus à K = 10 | moyen / moyen | `src/tower/descent.cpp` | rien |
| V8 | Feuille 24 à K = 10 (table M(K)) | `generator.cpp` v10 l. 641 | garder (K = 10) | 0 à K = 5 ; catalogue K = 10 ×1,6–1,8 (**E**) | nul / nul | `bench/points_export.cpp` l. 377, pilotes, défauts de `CatalogueParams` | rien |
| V9 | Processus résident, arènes pré-touchées | `--repeat` de la v10 | à mesurer + décision | −10 à −29 ms en chaud (**E**) | moyen / biais de mesure | `bench/full_probe.cpp`, `core/buffer.cpp` | rien |
| V10 | Tables de semis par ordre, étiquette dans la table des supports | `FlatIndex` v10 | à mesurer (basse) | −3 à −10 ms (**C**) | faible | `population_lookup.cpp`, `full_domain.cpp` | rien |
| V11 | Recensement par la feuille du catalogue ou k-d « k plus proches » | GEN_v2 v10, `SiteTree` v10 | conditionnel ; juge : improbable | 0 à −30 / −5 à −20 ms (**C**) | moyen | `census_workspace.cpp` | rien |
| V12 | Catalogue sur GPU par sous-arbres | conception GPU v10 ; R7 | à mesurer, dernier recours | −100 à −150 ms (**C**) | très élevé / élevé | nouveau `backend=cuda_g4` | rien |
| F1 | Euler à K+2, restriction clé par clé, épingle du mode 0 | audit v10 L01, v9 | garder, **urgent** | 0 ms | 1,5–2 j | `bench/`, portes `scale*`, `lidar` | en cours, non commis |
| F2 | Protocole apparié (A/A, médiane des rapports, test des signes, W24) | v4, v5, v6 | garder | 0 ms | quelques heures | `bench/ab_g4.py` | banc à N variantes porté, sans statistique |
| F3 | Instrumentation réduite (début des tâches, sous-chronos, recensement par arité) | v4, v8, v9 | garder | 0 ms | 0,5–1 j | `bench/full_probe.cpp`, `FullTimings`, `CatalogueTimings` | rien |
| F4 | Cible dimensionnée au pire de la plage, plusieurs séquences | auditeurs v9/v11 | garder | 0 ms ; corrige la cible | une session | `bench/full_timing.py` | en cours, non commis |
| F5 | Ordre 1 exact contre l'arbre couvrant minimal | E-HGP ; J2 de la v11 | garder | 0 ms | 0,5–1 j | porte stdlib sur le dump FULL | rien |
| F6 | Différentiel v10/v11 sur trames entières | ancres Merkle de l'audit v10 | garder | 0 ms | 0,5–1 j | `tests/tower/full_v10_diff.py` | rien |
| F7 | Fixtures gravées (rang, portails, longues descentes, limites d'Euler) | v2/v3, v8, audit v10 | garder | 0 ms | 1,5 j | `tests/catalogue/`, `reference/`, `tests/tower/` | rien |
| F8 | Préfixe Kmax (ordres 1..5 de K = 10 = K = 5) | v5, v6 | garder (basse) | 0 ms | 0,5 j | porte sur le dump FULL | rien |
| F9 | Juge bilatéral d'échantillon du catalogue | v8 | garder (validité) | 0 ms | 2–3 j | sonde hors produit | rien |
| F10 | CI GitHub : Clang et Python 3.10 nu | bibliothèque produit, v9 | garder | 0 ms | 80 lignes | `.github/workflows/` | rien |
| P1 | E1 : bras z = 2, borne d'IC de H_L2 ; bras témoin MR_k-bord | auditeurs v11 ; v10 lot C | garder | hors chrono | 1 h + 0,5 j | `bench/points_flat_*`, `plans/` | z = 2 et IC portés |
| P2 | Empreintes chaînées tour → points → sortie plate | bibliothèque produit | garder (basse) | hors chrono | 0,5–1 j | `bench/points_*.py` | rien |

### 3.2 Vitesse

#### V1 — Porter la feuille J3 de la v10, par tranches à sorties identiques

- **Source.** Feuille J3, privée mais conservée :
  `morsehgp3D_v10/receipts/audit_continu_20260929/performance_corrected/observed/feuille/src/morsehgp3D_v10/src/catalogue/leaf.hpp`
  (412 lignes) et `J3_feuille_v3.patch` ; contre-audit
  `morsehgp3D_v10/audits/audit_continu_20260929/performance/CONTRE_AUDIT_PROTO_CPU_20260929.md` § 1 ; fouille
  [`fouille/v10_moteur.md`](fouille/v10_moteur.md) idée 01. Contenu : recensement limité aux sites que les masques de
  dominance ne décident pas (lemme R), triangle médian avant le centre q3 (M3), enveloppe du tétraèdre avant q4 (E4),
  droite des centres en i64 à termes de paire calculés une fois, étages et table des triplets vivants.
- **Preuve.** **M-loc** sur la v10, trame 02 entière, CPU de l'étage des boîtes, A/B ABBA : ×1,371 à K = 5 (appariés
  ×1,359–1,381) et ×1,548 à K = 10 à un fil (relu dans `feuille-verif/mesures/ab_abba_t1.txt`) ; ×1,36 / ×1,56 à
  quatre fils ; mauvaises prédictions de branchement ÷2,2–2,4 (callgrind) ; mêmes 13 compteurs et dumps identiques sur
  10 couples ; fuzz 1 060 cas conclusifs (940 pleinement conclusifs selon le contre-audit), 0 désaccord ; 9 mutants
  tués, le dixième prouvé équivalent. Nuance de la contre-vérification : la table des triplets vivants était **déjà**
  dans la base mesurée ; le ×1,37 vient du reste.
- **Verdict.** Garder ([`verif/v10_moteur.md`](verif/v10_moteur.md)). Juge : garder par tranches ; la tranche (e)
  (étages et table H à la place du DFS) seulement si le microbanc M2 la préfère à la feuille « plate par lots » de la
  conception du 2 octobre.
- **Gain.** La feuille fait ≈ 80–85 % de la passe unique : passe ×0,67–0,77 → **−38 à −60 ms** à W48, **−25 ms** en
  borne prudente (aucune mesure J3 au-delà de quatre fils ; à 48 fils, moins de mauvaises prédictions peut réduire le
  gain SMT) ; une part est déjà prise par les compteurs et le q3 différé : incrément propre **−20 à −40 ms** (**E**).
  K = 10 : ×1,55 sur l'étage (**M-loc** v10).
- **Coût, risque.** Élevé (cinq tranches). Bornes J3 écrites pour u18 et une sous-maille T = 6, à refaire en u21/u24 ;
  la v5 a fermé « étage i64 du préfiltre q4 » faute de gain (médiane appariée 1,0021) : la droite i64 ne se garde que
  sur A/B ; deux mutants de bord n'étaient tués que par des fixtures (F-DYA, F-TRI, F-L64, à porter).
- **Où.** `src/catalogue/leaf.cpp` (`prepare`, `extend`, `census_and_emit`, `q3_of`, `q4_of`) ; un nouveau bit
  d'optimisation ; le DFS reste la référence différentielle et le chemin des feuilles de plus de 32 sites.
- **État.** (a) recensement par masques **porté** (`9b9244a00` : 43 % des tests de puissance du recensement du
  catalogue décidés par masque sur ng00, dumps identiques) ; (b) triangle médian et enveloppe du tétraèdre **en cours**
  (diff non commis de `leaf.cpp`) ; (c) compteurs et q3 différé portés ; (d), (e) à faire. Aucune A/B G4 encore.

#### V2 — Ordonnancement du pipeline des forêts : supprimer la queue de publication

- **Source.** Constat de la contre-vérification v7 ([`verif/v7.md`](verif/v7.md) § 1.3) sur l'archive AB7. Précédent
  de méthode : l'instrument d'occupation de la v9 avait montré 35–49 % d'attente à W48, invisible à W8 en local, et sa
  correction avait rendu ×1,5–2,0 sur l'étage (`morsehgp3D_v9/receipts/g4_tower_r8_20260923/`,
  `g4_tower_r9_20260923/`, **M**).
- **Preuve.** **M** (rejoué par le juge) : sur ng00, les deux prises sans queue de publication (0,9 et 1,1 ms) font
  **373,7 et 391,1 ms**, les trois autres 412–464 ms ; les forêts y tombent à 137 ms contre 171–189 ms, pour une
  résolution identique. Sur ng00 r2, l'ordre 3 traîne de 27,3 ms alors que sa publication entière vaut 25,5 ms à W1 :
  le publieur a travaillé **après** la fin de la résolution. Cause **C** : démarrage tardif ou famine des publieurs. À
  W48, 48 tâches pour 48 fils, réclamées par indice croissant : 39 résolveurs, puis 5 publieurs, puis 4 suiveurs
  (`src/tower/forest_pipeline.cpp` l. 1–7).
- **Verdict.** À mesurer : l'horodatage de **début** de chaque tâche manque (le banc ne publie que les fins).
- **Gain.** 0 à −30 ms à W48 (**E**, borne haute appuyée sur deux prises).
- **Coût, risque.** Faible à moyen. L'absence d'interblocage repose aujourd'hui sur « seules publications et balayages
  attendent, toujours des tâches d'indice inférieur » : tout changement (publieurs démarrés en premier, fils dédiés,
  ou publieurs qui résolvent un bloc en attendant) doit garder un argument valable pour tout W ; TSan, cas W = 2K.
- **Où.** `src/tower/forest_pipeline.cpp`, `src/tower/forest_concurrent.cpp` (`await_job`), `src/sched/pool.cpp`.

#### V3 — Recensement des descentes : borne exacte sur réseau, puis arbre radix sur l'ordre de Morton

- **Source.** Bornes exactes de la puissance par axe : `AxisBounds` de la v6 (`morsehgp3D_v6/src/pipeline/census.hpp`
  l. 61–84) et de la v7 (`morsehgp3D_v7/src/pipeline/census.hpp` l. 61–100, qualifiées par un oracle indépendant et
  cinq mutants), `PreparedPower` de la v8 (`morsehgp3D_v8/src/lanes/q3_ball_census.cpp` l. 17–62) ; extrema q2
  couplés retenus par l'audit d'héritage v11 (`receipts/audit_heritage_20261004/q2_coupled/`), contrat d'implantation
  de la revue indépendante 10 (`receipts/audit_independant_20261002/index_lattice_bounds_review_10/`) ; arbre radix de
  Karras = structure de la v4 ; variante radix trouvée par la contre-vérification v8 ([`verif/v8.md`](verif/v8.md)
  § 1.4).
- **Preuve.** **E**, simulations sur les trames réelles, rejouées par deux contre-vérifications : borne exacte seule,
  nœuds visités ×0,63–0,72 et tests de points ×0,47–0,54 ; avec l'arbre radix, bornes ×0,35–0,38 et tests
  ×0,26–0,28 ; mêmes rapports à K = 10. Les sites restent rencontrés dans l'ordre de Morton : **mêmes listes
  intérieures et coquilles, mêmes témoins saturés, donc mêmes descentes**. Poids **M** : parcours du recensement 8,2 %
  du CPU W1 (≈ 3,1 µs par appel), 291 515 appels et 21,15 M tests de points sur ng00. Limite : les requêtes simulées
  sont plus légères que les vraies (≈ 50 contre 78,5 tests à l'ordre 5).
- **Verdict.** Garder (v6, v8 corrigé), à mesurer (v7). Juge : garder, **un seul port** (les trois sources et le q2
  couplé), décidé d'abord sur compteurs déterministes. Le k-d permuté de la v8 est écarté : il change les descentes.
- **Gain.** −0,67 à −0,75 s de CPU W1 sur ng00 (≈ 20–22 % de la résolution) → **−5 à −25 ms** à W48 selon que la
  résolution reste le chemin critique (**E**) ; plus à K = 10 (**C**).
- **Coût, risque.** Faible à moyen (≈ 60–100 lignes ; construction radix O(n), sous la milliseconde). **Assistant
  distinct obligatoire** : jamais dans `power_bound_signs`, dont `tests/num/bounds_test.cpp` épingle le contrat
  continu ; voie `Wide` (q3 non certifié en u21) gardée sur la borne actuelle.
- **Où.** Nouvel assistant dans `src/num/`, appelé par `src/index/census.cpp` et `census_workspace.cpp` ; coupe au
  bit de Morton le plus haut qui diffère dans `src/index/build.cpp`. Répond au verrou 4 du développeur et à la réponse
  R4 des auditeurs (« d'abord q2 couplé, puis ablation de partition » : la borne de réseau généralise le q2 couplé, le
  radix est une ablation de partition à résultats identiques).

#### V4 — Forêt d'un ordre sans lots, matérialisation parallèle, verticales par requêtes

- **Source.** `docs/conception/TOWER_v2.md` de la v10 (§ 6.5, § 17.1 ; jamais implanté) ; conception v11 du 2 octobre
  (`build/v11-persist/conception/CONCEPTION_TOUR.md`, D-F1 et D-F3), reprise par `docs/CONCEPTION_MOTEUR.md`
  (priorité 5) ; « cœur DSU minimal » de la v6 ; chaînes lourdes de la v7
  (`audits/receipts_parallel_objects_20260911/`) ; certificats de forêt couvrante composables de la v7
  (`audits/receipts_composable_msf_20260911/`) pour K = 10 ; lemme du maximum d'identifiant de la v9
  (`audits/PHASE_A_MAX_ID_COMPOSANTE_20260923.md`) pour la numérotation.
- **Preuve.** **M-loc** : noyau sans lots ×2,5–2,9 contre le Kruskal par lots v10 sur les vrais vidages (ordre 5 :
  47,6 → 18,2 ms), forêt et numérotation identiques aux ordres 1–5 et 10 ; **mais** le même journal donne 34,0 ms de
  matérialisation séquentielle à l'ordre 5 (noyau + matérialisation = 52,2 ms, plus que les lots) : seul le noyau
  « multifusions à la volée » (÷1,6) est un gain séquentiel complet mesuré. **M** : publication de l'ordre 5 en v11 =
  41,8 / 34,3 / 46,0 ms à W1, 51,5 / 35,7 / 47,2 ms à W48 en étage séparé.
- **Verdict.** À mesurer, priorité haute pour le plancher ([`verif/v10_tour.md`](verif/v10_tour.md),
  [`verif/v7.md`](verif/v7.md), [`verif/v6.md`](verif/v6.md)). Juge : idem ; réécrire seulement si l'ablation de la
  matérialisation (`close` réduit à un comptage) montre plus de ≈ 15 ms dans la chaîne de l'ordre 5.
- **Gain.** Aujourd'hui 0 à −10 ms (la résolution reste plus longue que la publication). Mais **dès que la
  résolution passe sous ≈ 50–60 ms, la publication de l'ordre 5 devient le chemin critique** ; ce levier fait tomber
  ce plancher de 35–58 ms vers ≈ 10–20 ms (**E**). K = 10 : publication de l'ordre 10 ≈ 119–146 ms séquentielle (**E**)
  → 20–35 ms, 10–20 ms avec les certificats composables (**E**).
- **Coût, risque.** Moyen à élevé. Le registre porte encore « contraction des plateaux par composantes fortement
  connexes » en `proof_obligation` (`docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md` l. 240) : y inscrire T4–T6 avant le
  port. La v7 a mesuré une voie fenêtrée plus lente (188,6 → 250,4 s) ; un digest agrégé peut être aveugle à une
  renumérotation : comparer les dumps entiers ; la v11 numérote les groupes d'un plateau dans l'ordre des racines.
- **Où.** `src/tower/forest_plateau.cpp` (`regular_cell`, `close`), `forest_concurrent.cpp` (`publish`),
  `forest_pipeline.cpp`, `forest_ancestor_sweep.hpp`, `forest_vertical.cpp`.

#### V5 — Compteurs locaux et arènes par tâche dans la boucle chaude

- **Source.** Mécanismes G4 et G6 de la v10 (compteurs nus, listes locales sans atomique) ; contrats R1 et R3 des
  auditeurs (`audits/AUDIT_CONTRATS_NUMERIQUES_ET_CAPACITE_20261002.md`, commit `17514012b`).
- **Preuve.** Symptômes **M** (W48, PROF1) ci-dessus. Calibrage **M-loc** de la v9 sur le même levier : le profil
  attribuait 8,9 % aux compteurs ; tout retirer n'a rendu que 4,0–4,4 % du CPU, la version sûre 0,9 %
  (`morsehgp3D_v9/receipts/q34_micro_levers_20260923/README.md`, relu).
- **Verdict.** À mesurer. **Gain** : compteurs −2 à −7 ms, arènes 0 à −8 ms (**E/C**) ; la carte v10 avançait 10–20 %,
  la v9 dit de ne pas y compter.
- **Où.** `src/catalogue/leaf.cpp` (fait), `src/catalogue/boxes.cpp` l. 60, `src/core/buffer.cpp`.
- **État.** Compteurs **portés** (`0c358261c`, 13 h 47 : un vidage vérifié par feuille, feuilles bornées à 1 024 sites,
  dumps et grands livres identiques sur ng00 et ng02) ; arènes : rien.

#### V6 — Préambule du catalogue : racine sautée, tête par tranches, noyau sans branchement

- **Source.** Lemme trivial tiré de la note des cellules de centres de la v3 (un site de la boîte fermée n'est jamais
  dominé) ; frontière v3b privée de la v10 (`.../observed/frontiere/frontiere_v3b.patch`) ; noyau sans branchement de
  l'audit v10 (`build/v11-persist/audit_v10/L05_CODE_CATALOGUE.md` § 6.4).
- **Preuve.** **M** pour les comptes (simulation qui redonne exactement les compteurs AB7) : la racine paie
  598 275 / 687 675 tests inutiles et sériels (≈ 1,2 / 1,4 ms). v3b : `t_frontier` ×1,9–2,45 mais catalogue entier
  ×0,88–1,05 en local (**M-loc**).
- **Verdict.** À mesurer ([`verif/v2_v3.md`](verif/v2_v3.md), [`verif/v10_moteur.md`](verif/v10_moteur.md)) : le
  chemin critique des 18,8–20,7 ms du préambule (27 rondes, `capture` et `select` sériels) n'est pas connu.
- **Gain.** Racine −1 ms quasi sûr ; −5 à −20 ms en tout (**C**). Ce poste compte double : il est dans le squelette
  séquentiel. **Où** : `src/catalogue/adaptive_frontier.cpp` (`prepare_root`), `adaptive_prepare.cpp`, `boxes.cpp`.

#### V7 — Moins de pas de descente : arrêt à la première cellule, ou mémo de cellule daté

- **Source.** Mémo partagé par cellule de la v10 ; D-G1 de la conception v11 ; réponse R5 des auditeurs (certificat
  typé par sa date de validité, `receipts/audit_selfreview_20261004/cell_memo/`).
- **Preuve.** **M** : +8 % de pas en v11 (4,80 M contre 4,40 M sur ng00) ; **E** : la descente v10 est à 7–8 % du
  minimum de sa règle.
- **Verdict.** À mesurer, par un compteur d'abord (pas évitables par ordre, répétitions de cellule). **Gain** : −5 à
  −12 ms (borne haute) ; plus à K = 10. **Risque** : le suivi des pointeurs se fait dans la publication, le chemin
  séquentiel que V4 veut raccourcir. **Où** : `src/tower/descent.cpp` (`DescentBuilder::run`).

#### V8 — Feuille de 24 sites à K = 10

- **Source.** Table M(K) de la v10 (`morsehgp3D_v10/src/catalogue/generator.cpp` l. 641 : 12 / 16 / 24 / 28) ;
  ablation `build/v11-persist/audit_v10/preuves_l05_code_catalogue/ablation_M.txt`.
- **Preuve.** **M** (compteurs déterministes, v10, trame 02, K = 10) : feuille 16 → 11,06 M nœuds et 137,8 candidats
  par boule ; feuille 24 → 1,07 M nœuds et 57,9 ; `t_boxes` local 32,8 → 18,6 s. La v11 code 16 en dur pour tout K
  (`bench/points_export.cpp` l. 377, vérifié) : les onze catalogues K = 10 des sessions G4 `catalogue_parallel_20261002`
  et `catalogue_20261002` ont expiré au plafond de 15 s, ce qui est compatible avec cette explosion (sans la prouver).
- **Verdict.** Garder, pour K = 10 seulement. **Gain** : ×1,6–1,8 sur le catalogue K = 10 (**E** pour l'arbre de la
  v11). **Coût** : un paramètre, après une ablation de compteurs à W1.

#### V9 à V11 — Leviers secondaires ou conditionnels

- **V9, processus résident** (source : `--repeat` de la v10, conception D-M1). **M** v10 : passe 1 → 3, −3,0 / −6,6 /
  −4,5 % ; **M** v9 : ≈ 2 %. À mesurer, et **seulement si l'utilisateur fait du contrat une latence de flux** ; toujours
  publier le froid à côté.
- **V10, semis par ordre et étiquette dans la table des supports** (source : `FlatIndex` de la v10). À mesurer,
  priorité basse : −3 à −10 ms (**C**). La résolution « ordre par ordre » contredit le pipeline actuel : sans V4, non.
- **V11, recensement par la feuille du catalogue ou k-d « k plus proches »** (source : GEN_v2 § 3.4 et § 9 de la v10,
  jamais codé ; `SiteTree`). Conditionnel. Juge : **improbable** pour la feuille, car 81 % des tests de points du
  recensement sont à l'ordre 5 = K, où toute saturation retombe sur le repli, et la conception v11 a déjà écarté les
  feuilles du générateur comme index (`PISTES_DE_RUPTURE.md` R2.5) ; le k-d seulement si le recensement coûte encore
  plus de 1,5 µs après V3.

#### V12 — Catalogue sur le GPU de la G4, par sous-arbres (route de rupture)

- **Source.** Conception GPU de la v10 (`build/v10-persist/gpu_design/voie_partielle_feuilles/`) ; contrat R7 des
  auditeurs ; leçons mesurées des v5, v6, v7, v9 et de la bibliothèque produit (§ 5.4).
- **Preuve.** **M-loc** v10 : part de l'étage des boîtes qui resterait sur CPU si le GPU prenait les sous-arbres de
  liste ≤ L : 52,3 % à L = 16 (feuilles seules : plafond ×1,9), 23,8 % à 32, 15,1 % à 64, 7,9 % à 256. Aucun noyau du
  catalogue n'a tourné sur G4. **Correction du juge** : les débits de la sonde G4 « S7 » de la v10 (×2,3 entre 128 et
  64 bits) sont **invalides**, les boucles débordaient en entiers signés (`morsehgp3D_v10/receipts/ERRATA.md`) ; seule
  l'exactitude du comparateur `cmp128` est qualifiée.
- **Verdict.** À mesurer ; dernier recours, mais **la seule route dont le plafond touche la passe unique à l'échelle
  des 100 ms**. Étape 0 sans GPU : mesurer c(L) sur l'arbre de la v11.
- **Gain.** −100 à −150 ms de passe unique (**C**). Même un catalogue gratuit laisse FULL à 185–232 ms : nécessaire au
  pire de la plage, jamais suffisant seul.
- **Coût, risque.** Très élevé (8 jours-agent pour les feuilles, 6–8 de plus pour les sous-arbres, estimation v10) ;
  historique défavorable (cinq ports GPU du dépôt, jamais plus de ≈ 10 % de bout en bout) ; « des centres en 128 bits
  ne font pas un catalogue en 128 bits » : puissances et niveaux q3 demandent 134 à 204 bits en u21/u24 (R7).
- **Où.** Nouveau `backend=cuda_g4`, déclaré ; sous-arbres (filtre + feuille) de la passe unique.

Déjà fait et non recompté : **niveau q3 différé** (`56216392e`), ≤ 1–1,5 % du CPU (**E**), A/B G4 prévue par le
développeur (`claudeab8`).

### 3.3 Filets et outils : aucun gain de temps, mais la condition des réécritures

- **F1 — Euler à K+2, restriction clé par clé, épingle du mode 0** (sources : juge d'Euler de l'audit v10,
  `build/v11-persist/audit_v10/preuves_l01_math_catalogue/euler_juge.py`, bibliothèque standard seule ; v9
  `src/chain/tower_chain.cpp` l. 866 et 977–990, `audits/NOTE_C_INVARIANT_EULER_20260923.md`). Preuve **M** : en v10,
  un mutant de frontière perdait 2 134 à 9 523 boules avec un statut « ok », invisible à la porte du dépôt, tué par
  Euler et la restriction ; en v9, 9 mutants sur 35 n'étaient vus que par l'invariant. Dans la v11 : seulement écrit
  (`docs/MATHEMATIQUES.md` § 8) ; la restriction n'est jugée que sur 14 sites au plus ; les labels `scale*` et
  `lidar` ne portent que des portes du harnais (`cmake -E true`). Verdict : **garder, urgent** — la feuille est en
  réécriture. Limite : condition nécessaire, jamais un certificat (fixture D/T à 13 points : deux omissions se
  compensent) ; Euler ne voit ni niveaux ni identités, d'où l'épingle du mode de référence contre le mode rapide dans
  la même source. État : en cours chez le développeur, non commis. Où : `bench/catalogue_euler.py`, portes
  `mhgp11_catalogue_euler`, `mhgp11_catalogue_restriction`, `mhgp11_full_mode0_pin` ; identité au registre racine
  (absente).
- **F2 — Protocole apparié** (sources : banc apparié de la v4, `bench/forest_probe.cpp` l. 2643–2784 ; v5
  `tests/fold_bench.cpp` ; sonde d'ablation de la v6). Preuve **M** (rejouée par le juge) : par paire de processus,
  σ du logarithme du rapport 0,111 (FULL), 0,125 (domaine), 0,137 (forêts) : **29 à 44 paires** pour voir 5 % ; sur le
  domaine, de code identique, rapports 1,138 / 0,952 / 1,012 et différences +7 à +50 ms cinq fois sur cinq sur ng00
  (biais, pas seulement du bruit) ; le gain FULL publié sur ng00 (446,5 → 412,4 ms) n'est pas significatif (3 victoires
  sur 5). Verdict : **garder** la partie statistique (médiane des rapports, test des signes, bras A/A, nombre de paires
  fixé par σ) ; le banc intra-processus est **à mesurer** et jamais pour un levier mémoire. Règle : décider un levier de
  travail à W1/W8 (étendue de la passe unique 0,0–0,5 %), le confirmer à W48 ; décider un levier d'échelle à W48 et
  W24 épinglé. Où : `bench/ab_g4.py` (porté à N variantes par `54c167bb6`, « sans test statistique »).
- **F3 — Instrumentation réduite** (sources : décomposition fermée de la v4, chronos par ouvrier de la v8, occupation
  de la v9). Publier : début, fin et CPU de chaque tâche du pipeline et attente dans `await_job` ; sous-chronos du
  préambule ; chrono de la table des supports et des contextes ; `getrusage` par étage ; grand livre du recensement par
  ordre **et par arité** (nœuds, bornes, blocs, tests) ; `single_task_max_ns` déjà calculé mais non publié ; prises
  W24. Pourquoi : 38–44 ms de W48 non décomposés, gonflement ×1,5 du CPU de W1 à W48 non attribué, cause de la queue
  inconnue. Verdict : garder, périmètre réduit (le CPU d'un fil ne sépare pas SMT et contention).
- **F4 — Cible au pire de la plage** (sources : `morsehgp3D_v9/audits/PLAN_CRITIQUE_100MS_FULL_20260924.md`,
  `AUDIT_C_ALTERNATIVES_CONTRAT_LIDAR_G4_20260923.md` ; recalcul sur `receipts/pts4_review_20261003/`). Preuve **M**
  pour les comptes (127 trames c08 sans sol) : nœuds d'ordre 5 ∝ n^1,39 ; entre 50 000 et 60 000 sites, ×1,22–1,31
  (masse) et ×1,87 (c08_003412, atypique) ceux de ng00. « Temps ∝ travail » reste **E**. Verdict : garder. État :
  `bench/full_timing.py` non commis (tranches de 30–40 k, 40–50 k, 50–60 k, 60 k et plus).
- **F5 — Ordre 1 exact contre l'arbre couvrant minimal** (source : E-HGP `docs/OBJET_ET_DIMENSION.md` ; juge J2 écrit
  dans la v11, jamais implanté). Égalité exacte d'un ordre entier ; voit la voie q2, les courses à W48 et les plateaux
  N-aires (87 sur ng00). Verdict : garder ; porte stdlib (Python 3.10 nu de la G4).
- **F6 — Différentiel v10/v11 sur trames entières** (source : `build/v11-persist/audit_v10/preuves_l06_code_tour/`,
  ancres `anchors.jsonl` K = 5 sur les trois trames et K = 10 sur ng00, mêmes empreintes d'entrée que les mesures
  v11). Premier juge de toute la tour K ≥ 2 et de K = 10 sur une trame ; la v10 reste une source différentielle, pas
  une autorité. Verdict : garder ; étendre `tests/tower/full_v10_diff.py`, recopier ancres et script dans le dépôt
  (`build/` est ignoré par git).
- **F7 — Fixtures gravées.** Non-hérédité du rang (R3v2, F64, Q2X, F16, CRUX ; v2 et v3, recalculées exactement) ;
  portails silencieux (cinq points de la v8, vérité recalculée par la référence v11 : fusion finale à 25/4) ; longues
  descentes (« noyau serré et halo », six nuages à 3–4 sauts ; audit v10 et TOWER_v2) ; limites d'Euler (D/T à 13
  points, cinq sites de `MATHEMATIQUES.md` § 8). Verdict : garder ; **avant** les tranches (b)–(e) de J3 pour les
  premières, avant toute retouche des descentes pour les autres.
- **F8 — Préfixe Kmax** (v5 `tests/prefix_gate.cpp`, v6 palier P1) : les sections d'ordres 1..5 d'un dump K = 10
  égales octet pour octet au dump K = 5. Seul invariant global bon marché reliant K = 10 au K = 5 qualifié. Garder,
  priorité basse, avec la première prise K = 10.
- **F9 — Juge bilatéral d'échantillon du catalogue** (v8 `audits/q34_stream_crosscheck_spatial_20260921/`) : validité
  exhaustive de chaque boule avec des prédicats propres au juge ; la direction « complétude » de la v8 n'a énuméré
  qu'une boule admissible sur la trame entière : à reconcevoir avec un plancher. Lemme du citron à inscrire au
  registre d'abord. Garder (validité).
- **F10 — CI GitHub** (source : `.github/workflows/ci.yml` du produit, workflow v9) : Clang et portes Python sous
  Python 3.10 nu. Preuve **M** : la session `claudequal1` a été perdue sur un `ModuleNotFoundError` ; la qualification
  v11 n'a jamais vu Clang alors que la règle 11 de l'architecture l'exige. Garder.

### 3.4 Points et sortie plate (hors du chrono FULL)

- **P1 — E1.** Le bras z = 2 contre l'oracle et la borne d'IC de H_L2 (auditeurs v11) sont **portés** (`723cf6e43`),
  à rejouer sur G4 avant les mesures primaires. Reste le **bras témoin MR_k-bord** de la v10
  (`morsehgp3D_v10/tests/head/mreach.hpp` ; lot C préenregistré : écart tour − MR₂-bord de −0,001 à +0,010 de K = 2 à
  10) : sans lui, « T − A » confond l'entrée des points et la hiérarchie. Garder, en lignes descriptives déclarées
  avant toute lecture d'E1.
- **P2 — Empreintes chaînées** FULL → hiérarchie de points → sortie plate, numérotation canonique par plateau, rejeu
  O(n) des invariants, test de permutation de l'entrée sur une trame entière (source : `morsehgp3d/src/cpu/api/
  point_hierarchy.cpp` l. 270–390, 910–943, 1990–2042). Garder, priorité basse : préalable du port natif.
- Le **port natif** de la hiérarchie de points et de la tête plate reste un chantier v11 (contrat Q8 des auditeurs),
  pas une transposition ; on reprend des versions antérieures la forme du contrat, jamais le code.

---

## 4. Plan d'exécution en tranches mesurables sur G4

### 4.0 Protocole commun

- **Sorties identiques octet pour octet** à chaque prise ; chaque levier mesuré seul, puis en combinaison (règle des
  auditeurs) ; banc `bench/ab_g4.py` enrichi de F2.
- Travail CPU décidé à W1/W8 en paires AB/BA avec bras A/A ; confirmation à W48 avec un nombre de paires fixé d'avance ;
  leviers d'échelle à W48 **et** W24 épinglé.
- Trames : ng00, ng01, ng02 **et** les trames lourdes de F4 ; publier le **maximum** et la médiane ; froid toujours
  publié.
- Sessions par `gcp-migration/v11_session.py --commit`, portes Python sous Python 3.10 nu (`python3 -S`), TSan ciblé.
  Les noms de sessions ci-dessous sont proposés.

### 4.1 Tranches

| Tranche | Contenu | Session G4 | Critère | FULL ng00 attendu (froid) |
| --- | --- | --- | --- | ---: |
| **T0 Mesurer** | F3 ; F4 (trames lourdes, autres séquences) ; bras A/A ; **première prise K = 10** du contrat (feuille 16 et 24) et F8 ; compteurs de décision (pas évitables par `resolve1`, répétitions de cellule, genres de recensement) ; ablation de la matérialisation de `close` ; **c(L)** sur l'arbre v11 ; vidage des candidats réels pour les microbancs M2, M4, M5 | `claudediag1`, ou greffée sur `claudeab8` | dumps identiques avec et sans instrumentation ; étages publiés ≥ 98 % de FULL | 412 (aucune décision) |
| **T1 Filets** | F1, F5, F6, F7, F10 | `claudegate1` | portes vertes, mutants de données tués (code 4) | 412 |
| **T2 Leviers sûrs** | A/B du q3 différé, du recensement par masques et des compteurs locaux (déjà portés) ; arènes R3 ; racine G1 ; V2 selon le diagnostic de T0 | `claudeab8` (prévue), puis `claudeab9` | médiane appariée en baisse au-delà de l'A/A ; TSan sur le pipeline | ≈ 360–405 (**E/C**) |
| **T3 Recensement** | V3 : borne de réseau, puis arbre radix | `claudecens1` | borne gardée si nœuds **et** tests baissent ≥ 30 % ; radix si `regular_ns` W1 baisse ≥ 10 % | ≈ 340–400 (**E**) |
| **T4 Feuille** | V1 tranches (b), (d), (e) ; V8 à K = 10 ; microbanc M2 contre la feuille par lots | `claudeleaf1..3`, `claudeleafm2` | `single_pass_ns` W1 −5 % par tranche, confirmé à W48/W24 ; Euler K+2 vert | ≈ 300–380 (**E**) |
| **T5 Forêts** | V4 en étage séparé puis en pipeline ; bras K = 10 avec certificats composables | `claudeforest1` | publication de l'ordre 5 < 20 ms à W48 en étage séparé | ≈ 295–380 ; K = 10 −80 à −120 ms (**E/C**) |
| **T6 Squelette** | V6 (préambule) ; arènes pré-touchées ; table des supports et contextes parallèles ; V9 publiée à côté du froid | `claudeskel1` | préambule ≤ 8 ms ; résidu + contextes + naissances ≤ 20 ms | ≈ 260–365 (**C**) |
| **T7 Résolution, seconde vague** | V7 ; V10 ; V11 sous condition | `claudedesc1` | `regular_ns` W1 −5 % par levier | **≈ 215–350** (**E/C**) |
| **T8 Décision** | réécriture CPU par lots (conceptions du 2 octobre) ou V12 GPU par sous-arbres | `claudegpu0` (sonde de débit propre), `claudegpu1` (noyau de sous-arbres de bout en bout) | GPU abandonné si le total appareil ne passe pas sous ≈ 40 ms à K = 5 ; zéro désaccord ; replis comptés et bornés | seule route chiffrée sous 100 ms (**C**) |

ng01 et ng02 suivent dans la même proportion (≈ 0,85 et ≈ 0,92 fois ng00 aujourd'hui, **M**). Les gains ne
s'additionnent pas : chaque levier réduit l'assiette des autres. **T7 marque la fin des transpositions : au mieux le
niveau de la v10, ×2,2 à ×3,5 au-dessus de 100 ms.** Les microbancs M2, M4, M5 et la mesure c(L) se jouent **pendant**
T2–T4, pas après : ce sont eux qui disent si la route T8 existe.

### 4.2 Portes à verdir avant chaque famille de leviers

| Avant… | Portes vertes exigées |
| --- | --- |
| les tranches (b)–(e) de J3, la feuille 24, les arènes | F1, fixtures de rang de F7 — **urgent : la tranche (b) est en cours d'écriture** |
| toute retouche du recensement, de la politique de saut, de `resolve1` ou d'un mémo | portails et longues descentes de F7, F6 |
| toute réécriture de la publication ou des verticales | registre (ligne 240, T4–T6), porte d'équivalence contre la publication actuelle, mutants (plateau binarisé, image verticale ouverte, attache ouverte…), F3 (cause de la queue), F5, F6 |
| la première revendication K = 10 | F8, F1 à K = 12, F6 à K = 10 |
| tout code GPU | c(L), contrat R7 complété (§ 5.4), F1 et F6 comme témoins de bout en bout |
| toute lecture d'E1 | P1 |

---

## 5. Ce qui a été écarté, et pourquoi

### 5.1 Mesuré sans gain dans la v11 (ne pas refaire)

| Idée | Raison | Source |
| --- | --- | --- |
| Mémo de descente et mémos de lane | 2,6 % de succès ; incompatibles avec le pipeline | `receipts/full_memo_20261003/memo1/` ; carte v11 § 5.3 |
| Filtre flottant des signes de la puissance dans le recensement | ≈ 1 % du CPU : le coût est le parcours, pas l'arithmétique (V3 attaque le parcours) | `docs/PERFORMANCE_FULL.md` l. 186–188 |
| `-march=x86-64-v3/v4` sur tout le code | dans le bruit | session `claudeprof1` |
| Préchargement des états DSU de la publication | aucun gain | note d'audit du 3 octobre § 7 |

### 5.2 Mécanismes de la v10 à ne pas faire revenir

| Mécanisme | Raison | Source |
| --- | --- | --- |
| `SiteTree` tel quel, marges flottantes figées (`kMargin = 0,02`, `kApproxMargin`) | valables pour u18 seulement, arrondi supposé ; contraires à la doctrine F1–F6 | `site_tree.cpp` l. 64, `tower.cpp` l. 27 ; carte v10 N1 |
| Boule fermée entière à chaque saut | Θ(n) : jusqu'à 1 258 sites sur une trame | audit v10 L06-05 |
| Kruskal par lots et pointeurs de saut | plancher séquentiel 33–47 ms à K = 5, 89–140 ms à K = 10 ; 2,6–3,2 fois plus lent que le noyau sans lots ; à garder comme référence de porte | L06-04 |
| Frontière en largeur à barrières, grain de 19 482 tâches | ×2,1 de 1 à 48 fils ; le plan LPT v11 est à 7 % de l'idéal | reçu S4 ; carte v10 N2 |
| MEB proposée en flottant puis certifiée | la MEB exacte bornée de la v11 pèse ≈ 1 % du CPU | profils AB7, PROF1 |
| Entrée `cover` dépendante du rang de Morton ; coquilles par énumération brute ; mémoire hors budget ; course du Pool ; monolithes ; portes trop étroites | défauts relevés par l'audit v10 | carte v10 N3–N11 |
| Passe chaude présentée comme chiffre du contrat | protocole non apparié | carte v10 N12 |

### 5.3 Idées des versions antérieures dépassées ou négatives

| Idée | Raison | Source |
| --- | --- | --- |
| Sous-maille T6 des centres pour le LiDAR (verrou 6) | dumps identiques, travail ±0,3 % pour T = 6/3/0 ; T = 6 casse une garde i64 en u24 | `preuves_l05_code_catalogue/ablation_kT.txt` |
| Certificats scalaires l/u dans le filtre G1 (v3) | −38 % de tests, mais 230–320 ms de CPU pour 279 ms évités (simulé, compteurs = AB7) | `fouille/v2_v3.md` § 3 |
| Minorant commun des extensions q4 | signes à 153–201 bits par site et par triplet pour ≈ 0,33 candidat évité | revues indépendantes 7 et 8 |
| Crédit de groupe par moments ; index des selles ; saut par orthants ; partage géométrique entre ordres | négatifs mesurés (v9) ou sans objet (1,32 pas par trace ; une boule ne travaille qu'aux ordres m − 1 et m) | `fouille/auditeurs.md` § 3 |
| Feuille « plate » sans droite des centres, sans noyaux par lots | ×2,3–2,4 de quadruplets ; négative au coût actuel ; ne se décide que par M2 | `PISTES_DE_RUPTURE.md` § 2.2 |
| « Lot D » de filtres flottants avant J3 ; émission de 16 octets | ×1,06–1,11 seulement ; 0,78 niveau distinct par boule | L05 § 6.5, Q2 |
| Générateur WSPD à voies q2/q3/q4 (v3 à v9), certificats de blocs, fuseaux, ancres | autre architecture, ×20 à ×100 plus lente par site ; v9 0,76–0,98 s à K = 5 avec GPU | `fouille/v9.md`, `fouille/v4.md`, `fouille/v5.md` |
| Publication par Borůvka et étiquette minimale (v9) | 1 264,7 ms à K = 5 sur ng00 contre 36–52 ms ; perd le recouvrement ; seul le lemme des identifiants survit (V4) | `morsehgp3D_v9/audits/b_full_a_real_20260927/RESULTATS.md` (relu) |
| Réduction segmentée de la publication par blocs ; forêt par diviser pour régner ; ordres par K décroissant sans noyau | 31–45 % des fusions restent à la couture ; 5–10 fois le travail ; déplace la queue | `PISTES_DE_RUPTURE.md` R2.6 ; TOWER_v2 |
| Grand livre de descente sans copie (v10_tour_06) | ≤ 1,8 % du CPU W1 ; replié dans V5 | `verif/v10_tour.md` |
| MEB à quatre pivots (v8), MEB « support + extérieur » | la MEB pèse ≈ 1 % ; la seconde régresse (341 → 392 présentations) | `fouille/v8.md`, `fouille/auditeurs.md` |
| Dédoublonnage statique, cache évictif, gardes par rangs (v7) ; atlas q4 en i64, témoins hérités, file de plages (v8) | déjà dans la v11 sous une meilleure forme, ou négatifs | `fouille/v7.md` § 5, `fouille/v8.md` § 3 |
| Porte à deux autorités (v4), pic par étage et majorant unique (v5) | absorbés par la réponse R3 ; le majorant proposé par la fouille v5 serait faux | `verif/v4.md`, `verif/v5.md` |
| Catalogue « sans énumération » par descentes MEB-Lloyd ; couches douces et spectrales (E-HGP) | complétude non acquise, faux positif cosphérique ; changent l'objet | `fouille/ehgp_zoltan.md` § 5 |
| Contrat sur un seul ordre | change l'objet du contrat FULL | `PISTES_DE_RUPTURE.md` § 4 |

### 5.4 GPU : fausses routes mesurées

| Route | Mesure | Source |
| --- | --- | --- |
| Feuilles seules sur l'appareil | plafond ×1,9 (52 % du travail reste sur CPU) | conception GPU v10 § 3 |
| Recensement sur l'appareil, étage isolé (v7) | noyaux 29,8 ms, étage 846 ms : aucun gain de bout en bout | `morsehgp3D_v7/docs/RESULTATS_TOUR_CACHE_G4_20260910.md` |
| Recensement GPU de la v6 | noyaux 154 ms, étage 7,7 s dont 88 % de code hôte ; ×1,03–1,12 de bout en bout | `morsehgp3D_v6/receipts/session_g4_20260901_*` |
| Tout sur l'appareil, matérialisé (v5) | plus lent, puis parité, jamais un gain | `morsehgp3D_v5/docs/GPU.md` |
| Frontière Morton–Yao48 de la bibliothèque produit | 2,4 s pour les seules paires ; 7,0–7,5 s à chaud dont 4,8–5,4 s de recertification CPU | `docs/validation/phase15_*` |
| Proposition flottante sur l'appareil puis recertification CPU | détruit l'étage (même source) | idem |
| Étage des descentes sur l'appareil | « projection, pas promesse » | TOWER_v2 § 14 |

Ce qui survit, à joindre au contrat R7 avant tout code : exact sur l'appareil ou `unresolved` repris sur CPU avant
admission ; taux de replis compté et borné par une porte ; capacités comptées sur l'appareil (count–scan–emit), y
compris les feuilles larges jusqu'à 256 sites ; contexte chaud déclaré hors chrono ; lanceurs factices hostiles avant
toute session payante ; témoin de l'appareil en phase 0, `-fmad=false`, sans FTZ ; reconstruction hôte parallèle à
offsets fixes (`verif/produit.md`, `verif/v5.md`, `verif/v6.md`).

### 5.5 Points et clustering

| Idée | Raison | Source |
| --- | --- | --- |
| Routage descendant par vote (produit), routage médian (v3), vote du § 9.1, ER0h | rejetés par la v11 : discontinus, aucune constante de stabilité uniforme ; masses par k-facette ∝ C(n, k) | `docs/HIERARCHIE_POINTS.md` § 5–6 |
| Arbre multi-ordres λ = k/r^z (produit), têtes multi-K de la v10 | jamais mesuré, ou mesurées négatives (EOM intégrée 0,722–0,777 contre 0,795) | `fouille/produit.md` § 4 ; `fouille/v10_tour.md` § 4 |
| Code du réducteur produit ou de la tête v10 comme base du port natif | `cpp_int`, `std::map`, mono-fil (2,79 s à 50 000 points d'ordre 1) ; condensation fausse aux cohortes | idem |
| Cascade flottante du produit | suppose l'arrondi au plus proche ; contraire à F1–F6 | `morsehgp3d/include/morsehgp3d/exact/fp64_interval.hpp` |
| Masses fractionnaires du § 9.1, seuil relatif α (Zoltan) | la v11 a choisi des masses entières ; α jamais mesuré | `fouille/ehgp_zoltan.md` § 5 |

### 5.6 Méthode

Comparer des constantes entre processus ou des médianes indépendantes de cinq prises à W48 (piste fermée de la v4 et
de la v5) ; projeter en CPU·s/48 (le SMT rend ×1,22–1,75 de 24 à 48 fils) ; prononcer « 100 ms impossible » sans
borne inférieure (les verdicts de la v9 ont été réfutés par la v10) ; lire Euler comme un certificat ; importer le
harnais fail-closed de la v6 (directive « minimum de garde-fous »).

### 5.7 Pistes fermées : aucune n'est rouverte

Aucune idée retenue ne matérialise la mosaïque de Delaunay d'ordre supérieur ni un catalogue en C(n, k). Les forêts
couvrantes de V4 portent sur les naissances datées d'un ordre, pas sur les points (la piste fermée est l'arbre couvrant
d'atteignabilité mutuelle des points, `docs/archive/abandoned/README.md`). Restent fermées : surrogate point-MST,
Geogram/PDEL, frontière `prune-only` pilotée par l'hôte, parcours relancé par paire
(`docs/archive/abandoned/README.md`) ; dual inversif, préfixes kNN, plafond de degré de Gabriel, filtres par rang
(`morsehgp3D_v3/audits/PISTES_FERMEES.md`) ; DTM et Delaunay pondéré vers D_k (E-HGP).

---

## 6. Questions ouvertes et mesures à faire

### 6.1 Décisions à demander à l'utilisateur

1. **Froid ou chaud** : 100 ms est-il la latence par trame d'un flux à processus résident (10 Hz), ou un processus
   neuf par trame ? La v10 publiait une passe chaude, la v11 mesure à froid (écart 3–7 % en v10). Publier les deux.
2. **Maximum ou médiane de la plage**, et quelles séquences : sur l'échantillon c08 (choisi pour ses objets), 85 trames
   sur 127 dépassent 60 000 sites sans sol ; la plage « 30 000–60 000 » du contrat en exclut la majorité.
3. **Route GPU** : l'ouvrir dès que c(L) et `claudegpu0` la justifient, en parallèle de T3–T4, sans attendre la fin des
   transpositions.
4. **K = 10** : accepter une cible annoncée de 0,3–0,4 s plutôt que 100 ms.
5. **Périmètre** : le retrait du sol (≈ 30 ms de CPU par trame, mesure locale de la v8) est hors du chrono FULL ; si
   100 ms devait s'entendre de bout en bout, il en prendrait près du tiers.

### 6.2 Questions aux auditeurs et registre des preuves

- Contrat de compteurs de la voie J3 (question D du développeur, `49831e9e9`) : en attente.
- À inscrire au registre racine avant les ports correspondants : identité d'Euler par ordre (F1) ; lemme du citron
  (F9) ; T4–T6 de la contraction des plateaux (V4, ligne 240) ; lemme T3 (V7) ; lemmes R, M3, E4 de J3 s'ils n'y sont
  pas ; l'énoncé « centre hors de toute boîte ajustée ⇒ au moins K intérieurs » si V11 est poursuivi.

### 6.3 Mesures à faire, dans l'ordre, avec leur seuil de décision

1. Bras **A/A** apparié à W48 et W1/W8 (σ par paire, nombre de paires pour 5 % et 10 %).
2. **Début de chaque tâche** du pipeline et attente des publieurs : la queue vient-elle du démarrage ou du débit ?
3. **Trames lourdes** et autres séquences, K = 5, W48, cinq prises : facteur réel du pire de la plage.
4. **Première prise K = 10** du contrat (W48, processus neufs, feuille 16 et 24) et porte de préfixe.
5. **W24 épinglé contre W48** : le gonflement ×1,5 du CPU est-il le SMT ?
6. Recensement **par arité** (nœuds, bornes, blocs, tests) ; fraction f servie par la feuille du catalogue ; pas
   évitables par `resolve1` (seuil : ≥ 10 % des pas hors table) ; répétitions de cellule (seuil : ≥ 5 % des pas
   coûteux).
7. Ablation de la matérialisation dans la publication de l'ordre 5 (seuil : > 15 ms pour réécrire).
8. **c(L)** sur l'arbre de la v11 (poursuivre le GPU si c(64) ≤ 20 %) ; vidage des candidats réels pour M2, M4, M5.
9. A/B G4 des leviers déjà portés (q3 différé, recensement par masques, compteurs locaux), chacun seul puis combinés.

### 6.4 Incertitudes qui restent

- Le gain J3 au-delà de quatre fils (aucune mesure à 48 fils) ; la conversion des gains de résolution en temps FULL si
  la publication devient critique ; la cause réelle de la queue ; c(L) sur l'arbre T0 ; les coûts unitaires des
  conceptions par lots (jamais mesurés) ; la référence K = 10 ; le lien entre travail et temps à 50–60 k sites.
- Cet audit n'a rien exécuté de natif : tous les gains sont **E** ou **C**, plusieurs preuves sont des mesures locales,
  et le dépôt avance pendant la lecture (sept commits v11 entre 13 h 06 et 13 h 47). Les ancres, scripts et
  coordonnées sous `build/` sont ignorés par git : tout ce qui devient porte se recopie dans le dépôt avec sa
  provenance.

---

## Sources de l'audit

- Cartes : [`cartes/CARTE_V10_VITESSE.md`](cartes/CARTE_V10_VITESSE.md), [`cartes/CARTE_V11.md`](cartes/CARTE_V11.md)
  et `cartes/CARTE_V11_derive.py` (rejoué).
- Fouilles (`fouille/`) et contre-vérifications (`verif/`) : `v10_moteur`, `v10_tour`, `v9`, `v8` (et
  `verif/v8_annexes/`), `v7`, `v6`, `v5`, `v4`, `v2_v3`, `auditeurs`, `produit`, `ehgp_zoltan`.
- Plans : [`plans/PLAN_VITESSE_100MS.md`](plans/PLAN_VITESSE_100MS.md),
  [`plans/PLAN_AUTRES.md`](plans/PLAN_AUTRES.md).
- Reçus G4 v11 (`morsehgp3D_v11/receipts/`) : `developpement_20261003/pipeline_g4/sessions/claudeab7/` (moteur
  `b87285378`), `claudeprof1/`, `qualification_performance_20261003/` (moteur `c40f40798`),
  `pts4_review_20261003/` ; v10 : `g4_session4_j2c_20260929/`, `g4_session1_20260929/`, `ERRATA.md`.
- Dépôt v11 lu à 13 h 49 UTC : `origin/main` = `0c358261c` ; commits postérieurs aux plans : `723cf6e43`,
  `9b9244a00`, `54c167bb6`, `49831e9e9`, `0c358261c` ; diff non commis du développeur (triangle médian et enveloppe du
  tétraèdre dans `leaf.cpp`, `bench/full_timing.py`).
- Conceptions du 2 octobre : `build/v11-persist/conception/` (`CONCEPTION_TOUR.md`, `CONCEPTION_GENERATEUR.md`,
  `PISTES_DE_RUPTURE.md`, `preuves_tour/noyau_v11.log`).

FIN
