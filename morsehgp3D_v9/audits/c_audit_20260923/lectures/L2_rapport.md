# Lentille 2 — Générateur q2 : audit de l'auditeur C

```text
phase=exploration_v9_hors_registre
backend=reference_cpu
profile=quantized_u18_input_only
mode=audit_independant
public_status=not_claimed
GCP non utilisé. Aucune compilation. Deux scripts Python exacts (fractions/entiers), sous nice, moins d'une minute CPU.
```

Base lue : worktree détaché `origin/main` **0125dc18**. Les chemins sont relatifs à la racine du dépôt. Le worktree du développeur porte désormais **4530644b** et non 4cde1502. Ce commit ne touche pas la voie q2 : seuls `tower_chain.*` et la sonde reçoivent des compteurs q3/q4.

Statuts employés dans ce rapport :

- **prouvé** : démonstration écrite ;
- **testé** : porte ou fixture, avec les tailles précisées ;
- **mesuré** : valeur recopiée d'un reçu ;
- **script** : recalculé ici ;
- **supposé** : hypothèse non vérifiée.

## 0. Résumé

1. **Correction.** La relecture ligne à ligne ne trouve aucun défaut de correction, de complétude, d'arithmétique ou de concurrence sur le chemin appelé par la chaîne. C'est le chemin `run_wspd_q2_census_parallel` : SharedBlocks, frère saturant, ComplementFirst, ancres Individual, Pool64, Coarse avec 16 jobs par worker, propositions `{2,16,true}` (`morsehgp3D_v9/src/chain/tower_chain.cpp:334-338`).
2. **Complétude.** Elle est **prouvée** dans les notes v8 : partition des paires au plus bas ancêtre commun, certificats sûrs, census exact. Elle est **testée** seulement sur de petits nuages : au plus 20 sites dans le corpus exhaustif, 320 sites pour l'héritage, 14 sites pour le juge T2 de chaîne. À l'échelle LiDAR, rien ne détecte une paire omise dont la boule n'a pas d'autre présentation. Je propose trois juges bon marché (§ 3.4).
3. **Arithmétique.** Elle tient largement en i64 et i128. Les formules sont confirmées par **7 315 contrôles exacts** en force brute (script). Fait nouveau utile au GPU : tous les intermédiaires q2 restent sous 2^44, donc la voie q2 est **exacte en binary64**, quels que soient l'arrondi et la contraction FMA.
4. **Coût.** q2 prend **0,228–0,516 s à K5** et **0,413–0,842 s à K10** sur G4 à 48 fils, sessions R5 à R7b comprises. Passer de 24 à 48 fils n'apporte que **×1,07 à K5** et **×1,24–1,28 à K10**, car le G4 n'a que 24 cœurs physiques en SMT 2. Avec l'algorithme actuel, q2 seul occupe environ 75 % du budget de 1 s à K10 sur 000000 et dépasse de 2,3 à 8,4 fois la cible de 100 ms. Des leviers exacts existent : trois sont déjà dans le code mais non mesurés, et un port GPU est envisageable (§ 7).

## 1. Quel objet q2 la tour exige

Le manuscrit, § 8.2 (Déf. 28–29 et Th. 4, p. 88–90 du chapitre 8), fixe les définitions :

- un K-simplexe σ (K+1 points) est de Gabriel si l'intérieur de sa miniball ne contient aucun point de X∖σ ;
- tout simplexe K-séparant est de Gabriel ;
- le K-graphe de Gabriel relie les facettes de ces simplexes.

Soit une boule de support diamétral {a,b}, avec p sites strictement intérieurs. Elle porte un K-simplexe de Gabriel dès que K+1 ≥ p+2. Pour une tour K=1..Kmax, les boules utiles sont donc exactement celles où **p ≤ Kmax−1**. Le seuil de rejet q2 vaut Kmax témoins stricts distincts. La règle générale est h_q = Kmax+2−q (`morsehgp3D_v9/src/gen/wspd/front.hpp:150-152`, `front.cpp:90-95`). La chaîne revérifie `p+q_min ≤ min(Kmax+1,n)` (`tower_chain.cpp:493-494`).

Une boule dont le support minimal q_min vaut 2 contient forcément une paire diamétrale dans sa coquille. La voie q2 la présente donc par chacun de ses diamètres, avec la liste complète des intérieurs et de la coquille, extrémités comprises (`morsehgp3D_v9/src/gen/pipeline/q2_census.hpp:64-73`).

## 2. Pas à pas : comment toutes les boules q2 utiles sont produites

### 2.1 Nuage

`prepare_cloud` (`morsehgp3D_v9/src/gen/pipeline/prepared_cloud.cpp:112-172`) procède ainsi :

- il copie les points ;
- il refuse toute coordonnée hors de [0, 262 143] (l. 130-135) ;
- il refuse les doublons via une clé de 54 bits triée (l. 136-151).

Il construit aussi un arbre de plages sur l'ordre d'entrée (l. 154-169), que seul `local_credits.cpp:307-308` interroge. Hors chemin de chaîne, cet arbre est inutile. Les identifiants restent les rangs d'entrée : la chaîne peut ainsi relire `points[id]` (`tower_chain.cpp:321-331`).

### 2.2 Index global

`Q2CensusIndex::build` (`q2_census.cpp:112-171`) découpe récursivement au milieu de la plus grande étendue, par `std::partition`. Chaque feuille ne contient qu'un site : des sites distincts ne donnent jamais un côté vide (l. 146-148). Les nœuds sont rangés en préordre, avec un lien d'échappement vérifié à chaque création (l. 163-168). Au total, l'index compte 2n−1 nœuds.

La profondeur est bornée à 3×18 = 54 découpes (`morsehgp3D_v9/src/gen/core/types.hpp:23-29`). Mon script confirme 18 divisions entières par axe depuis M. Le vecteur `nodes_` n'est jamais réservé, alors que `HERITAGE_V7_V8.md:39` le demandait.

### 2.3 Front WSPD (`morsehgp3D_v9/src/gen/wspd/front.cpp:75-391`)

Le parcours part du produit racine × racine :

- **Produit diagonal U×U** : il se divise en LL, LR et RR (l. 113-128). Chaque paire non ordonnée apparaît ainsi exactement une fois, à son plus bas ancêtre commun. Une feuille diagonale ne contient aucune paire (l. 116-119).
- **Produit disjoint A×B** : le filtre de témoins s'applique d'abord (l. 134). Si le masque q2 tombe à zéro, le produit est un **rectangle mort**, et sa masse |A|·|B| passe au registre des rejets (l. 135-144).
- **Test de séparation** : il suit la convention `box_gap_diameter_v1`, soit gap² ≥ s²·max(diag_A², diag_B²), calculé en i128 (l. 145-149). S'il réussit, le produit est émis comme **rectangle terminal** (l. 150-156). Sinon, le facteur de plus grande diagonale est divisé (l. 158-169). Deux feuilles distinctes sont toujours séparées, car leur diagonale est nulle.

En mono, la somme des masses rejetées et résiduelles doit valoir C(n,2) (l. 190-200). En parallèle, la même identité est vérifiée après réduction (`q2_census.cpp:1354-1356`). La chaîne impose s ≥ 8 (`tower_chain.cpp:291`).

Point important : **aucun certificat q2 n'utilise la séparation**. `q2_node_pool.hpp:46` le dit explicitement, et le census est exact paire par paire. s ne règle que la granularité d'émission.

### 2.4 Filtre MidpointSamples : témoins universels, fenêtre 2K, héritage (`front.cpp:214-357`)

- **Saut.** Si moins de Kmax sites sont hors de A∪B, aucune recherche n'a lieu (l. 221-227).
- **Descente.** Une seule descente sans retour mène à la feuille la plus proche de 4× le milieu des centres des boîtes (l. 229-244). Ce n'est qu'un proposeur.
- **Fenêtre historique.** Elle propose Kmax rangs contigus autour du pivot (l. 245-247). Pour un facteur max ≤ 16, deux intervalles disjoints complètent une fenêtre de 2Kmax autour du **même** pivot (l. 268-291, option `{2,16,…}`).
- **Crédit.** Un rang de A ou de B est sauté (l. 295-298). Sinon, z est crédité si `h_minimum(A.box,B.box,{z}) > 0` (l. 313-316). C'est le minimum exact de H=(z−a)·(b−z) sur les boîtes continues : H est bilinéaire en (a,b) et concave en z, donc son minimum est atteint aux sommets (`morsehgp3D_v9/src/gen/spindle/predicates.hpp:81-98`). Le site z est alors strictement intérieur à toutes les boules diamétrales de A×B.
- **Rejet.** Il faut Kmax rangs distincts (l. 323-337).
- **Héritage.** Un produit survivant transmet à ses deux enfants au plus Kmax−1 **rangs** certifiés (l. 346-355, `WitnessList` l. 52-56). L'enfant repart de ce compte et saute sans test un rang déjà reçu (l. 300-312). Le Théorème H (`morsehgp3D_v8/docs/P0_TEMOINS_HERITES_Q2.md:36-48`) le justifie : le minimum sur des sous-boîtes est au moins le minimum sur les boîtes du parent, et un rang hors de A∪B reste hors de A'∪B'.
- **Restriction.** `validate_front` réserve ces options à la voie q2 seule (l. 393-414).

### 2.5 Rectangle terminal : Pool, ancres, frère (`q2_census.cpp:1073-1143`)

Le plus petit facteur fournit les ancres (l. 1084). Si le plus grand facteur a au moins 64 sites, le **Pool** `Q2NodePoolPlan` s'applique (`morsehgp3D_v9/src/gen/pipeline/q2_node_pool.hpp`) :

- chaque facteur propose ses K+1 sites les plus projetés vers la boîte opposée (l. 167-208) ;
- h_a(a) compte les témoins de A∖{a} universels sur la boîte B, et symétriquement pour h_b (l. 209-224) ;
- les deux populations sont disjointes, donc p ≥ h_a+h_b ;
- les survivants de h_a+h_b < K forment au plus K bandes « classe A × préfixe B » (l. 79-86) ;
- chaque survivant est recompté **depuis zéro** sur tout le nuage par `pair_task` (`q2_census.cpp:678-687`, 734-747).

Si le Pool ne retire rien, la voie normale reprend (l. 722-729). Sinon, chaque ancre lance `shared_task<frère, ComplementFirst>` depuis le curseur 0 (l. 1112-1130).

### 2.6 Census exact et charge utile (`q2_census.cpp:397-547`, 304-388)

L'état d'une tâche comprend :

- l'ancre a ;
- un nœud requête B ;
- le compte acquis ;
- un curseur de préordre, qui désigne le suffixe non consommé.

Les bornes exactes de 4H sur {a}×B×Z sont précalculées par `Q2PreparedBounds` (`morsehgp3D_v9/src/gen/spindle/q2_prepared_bounds.hpp:274-330`). Pour chaque nœud Z :

- min > 0 : crédit de toute sa population ;
- max ≤ 0 : saut du nœud ;
- sinon : division de Z ou de B (l. 501-544).

Quand B est divisé, les deux enfants héritent **ensemble** du compte et du curseur (l. 530-543). Le compte sature à Kmax, puis la tâche rejette (l. 505-512).

**ComplementFirst** (l. 444-490) visite d'abord tout l'index hors de B, puis B. Les ancêtres de B et de l'ancre sont divisés avant toute décision géométrique. **Le certificat frère** (l. 420-442) rejette si le frère de requête contient au moins K sites tous strictement intérieurs ; il ne modifie jamais le compte.

Une paire acceptée relance `collect` depuis la racine (l. 332-362). Cette passe émet tous les intérieurs et toute la coquille, avec gestion exacte de H=0, et vérifie que le nombre d'intérieurs égale le compte (l. 379-381).

### 2.7 Parallélisme

`make_wspd_front_jobs` prépare un préfixe en largeur jusqu'à `W×16` états, soit 768 à W48 (`front.cpp:438-464`). Les jobs sont ensuite pris par compteur atomique : c'est Coarse (`q2_census.cpp:1298-1306`). Chaque worker possède :

- un moteur privé ;
- son propre consommateur, qui écrit dans `slots[w]` (`tower_chain.cpp:317-333`).

Les réductions sont membre à membre et vérifiées (`morsehgp3D_v9/src/gen/parallel/work_reduction.hpp`). Les identités de registre sont contrôlées après la jointure (`q2_census.cpp:1345-1369`). Je n'ai trouvé aucune course : pas d'état partagé mutable hors des compteurs atomiques et des tranches par worker. Le dispatcher Donate (`front.cpp:502-717`) est porté mais n'est pas utilisé.

### 2.8 Raccord à la chaîne

Chaque support devient une présentation portant :

- la clé `ExactBall::make_q2` ;
- la profondeur `interior.size()` ;
- la coquille `shell.size()`.

Les **listes d'identifiants sont jetées** (`tower_chain.cpp:329-330`). Suivent :

1. un sample-sort des présentations ;
2. la détection des doublons de présentation (l. 137-140) ;
3. pour chaque clé, le recalcul de la clé v7 (l. 469) et un recensus exact par l'index de la tour (l. 470-474) ;
4. la vérification q_min = plus petite arité présentée (l. 484-492).

## 3. Argument de complétude : ce qui est prouvé, testé, mesuré ou supposé

### 3.1 Chaîne logique

- **(P1) Couverture.** Toute paire non ordonnée tombe dans exactement un produit terminal ou rejeté. Preuve dans `morsehgp3D_v8/docs/P0_FRONT_REEL.md:40-66`. Contrôle nécessaire à l'exécution : le registre de masse.
- **(P2) Rejet du front.** Kmax rangs distincts hors de A∪B, avec minimum exact de H strictement positif, donnent p ≥ Kmax pour chaque paire du produit. Héritage : Théorème H.
- **(P3) Pool.** La disjonction A∖{a} / B∖{b} donne p ≥ h_a+h_b (`morsehgp3D_v8/docs/P0_POOL_TERMINAL_Q2.md:34-64`).
- **(P4) Frère.** K sites distincts de a et b, tous strictement intérieurs (`morsehgp3D_v8/docs/P0_CERTIFICAT_FRERE_Q2.md:8-47`).
- **(P5) Census.** L'ordre de préordre est fixe (ComplementFirst en est une permutation). Chaque site est décidé une fois par chemin de requête, avec des bornes exactes. Le compte est donc exact jusqu'à saturation (`morsehgp3D_v8/docs/P0_FRONT_ET_CENSUS_Q2.md:31-45`, `P0_ORDRE_TEMOINS_Q2.md`).
- **(P6) Charge utile.** Elle est complète, y compris sous égalité, et recoupée par le compte.

### 3.2 Statuts

- **Prouvé** : dans les notes v8, reprises comme « prouvé + testé » par `morsehgp3D_v9/docs/HERITAGE_V7_V8.md:24`. Ces preuves ne sont consolidées ni dans un document v9, ni dans le registre racine.
- **Testé** :
  - `morsehgp3D_v9/tests/gen/wspd_q2_census_gate.cpp` : nuages d'au plus 20 sites, plus les jumeaux u18 à 262 143 ; K ∈ {1,2,5,10}, s ∈ {8,10,12}, modes Pure et MidpointSamples, deux permutations, oracle exhaustif (l. 346-417) ;
  - `wspd_front_inheritance_gate.cpp:777-797` : oracle force brute jusqu'à 320 sites ;
  - juge T2 de chaîne : n ≤ 14.
- **Mesuré à l'échelle** : seulement des invariants nécessaires. Masse du front, absence de doublons, recensus des **clés émises**, identité des digests entre répétitions (R5–R7b).
- **Non couvert à l'échelle** : l'omission d'une paire dont la boule n'a pas d'autre présentation. Le contrôle q_min ne la voit que si q3 présente la même boule. Ce point recoupe `morsehgp3D_v9/audits/ETAT_COURANT.md:43-48`.

### 3.3 Domaine

Sites distincts, u18. Aucune position générale n'est requise pour q2. La coquille n'est pas plafonnée par le générateur ; la chaîne refuse au-delà de 12 sites.

### 3.4 Juges proposés

**(a) Invariant K-NN ⇒ q2.** Si z est strictement intérieur, alors |z−a|² < (z−a)·(b−a) ≤ |z−a||b−a|, donc |z−a| < |b−a|, et symétriquement pour b. Par conséquent p(a,b) ≤ min(ρ_a(b), ρ_b(a)), où ρ_a(b) compte les sites strictement plus proches de a que b.

Toute paire avec min ρ ≤ Kmax−1 doit donc être émise, avec une profondeur au plus égale à ce rang. Le contrôle coûte O(nK log n) avec un index indépendant.

Mon script (`check_knn_invariant.py`) donne :

- 3 312 paires avec égalités, zéro violation ;
- sur des nuages synthétiques de 110 sites, l'invariant couvre 25–32 % des paires q2 acceptées, sans aucune violation.

**(b) Invariant K=1.** L'EMST, calculé indépendamment, doit être inclus dans les présentations de profondeur 0. Le multiensemble des longueurs d'arêtes, divisées par deux, doit égaler les niveaux des fusions K=1, chaque fusion comptant `parent_count−1` fois.

**(c) Juge d'échantillon par ancre.** Prendre m ancres tirées d'une graine. Pour chaque b, calculer p(a,b) saturé à K par `tower::ball_census` (`morsehgp3D_v9/src/tower/pipeline/census.hpp:174`), puis comparer à l'ensemble des présentations de a. Le coût est O(m·n·log n), sans table indexée par paire.

## 4. Arithmétique 18 bits (M = 262 143)

| Grandeur | Majorant (script) |
| --- | --- |
| `h_minimum` (i64) | ≤ 3M² < 2^38 |
| 4H préparé, conjoint ou paire (i64) | ≤ 12M² < 2^40 |
| Carrés (2z−C)² | < 2^38 |
| `Q2BallKey` : centre ×2 | ≤ 2M < 2^32 (u32) |
| `Q2BallKey` : diamètre² | < 2^38 |
| Ξ (q3/q4) | ≤ 12M⁴ < 2^76 (i128) |
| 3H² | < 2^77 |
| 16Ξ | < 2^80 |
| 3·h_max4² | < 2^81 |
| s²·diag², s = 2^32−1 | < 2^102 (i128) |
| `midpoint_distance4` | < 2^42 |
| Score du Pool | < 2^39 |

- Les promotions précèdent tous les produits : `predicates.hpp:27-44`, `q2_prepared_bounds.hpp:278-284`, `front.cpp:26-43`, 148-149.
- La garde de domaine est présente : `types.hpp:64-105`, constructeur de `Q2PreparedBounds` l. 275-276. Les jumeaux à 262 143 sont gravés.
- Ces éléments ferment les points 4 et 5 de `morsehgp3D_v9/docs/audit_v8/02_chaine_q2.md` § 5.

Mon script `check_q2_bounds.py` confronte à la force brute, sur grille entière et demi-grille :

- `h_minimum`, `h_maximum_times_four` ;
- les bornes préparées et conjointes ;
- `pair_bounds`, `point_power4`.

Bilan : **7 315 contrôles, zéro échec**, y compris près de 0 et de M.

Corollaire nouveau : hors Ξ, chaque intermédiaire q2 est un entier inférieur à 2^44, donc représentable exactement en binary64. Un census q2 en double précision serait donc **exact sans filtre ni repli**, même avec FMA ou réassociation.

## 5. Coûts q2 dans les reçus

Mesures G4, 48 fils, s8, sur les reçus R5, R6 et R7b (`morsehgp3D_v9/receipts/g4_tower_r{5,6,7b}_20260923/vm/probe_*.stdout`) :

| 08/ (sites) | K | q2 (ms) | Part de la chaîne | Candidates | Acceptées | Rectangles |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 000100 (35 551) | 5 | 228–254 | 5,4–6,7 % | 7 332 740 | 396 630 | 764 152 |
| 000000 (39 885) | 5 | 452–477 | 7,5–8,6 % | 9 159 254 | 456 919 | 960 535 |
| 000200 (45 845) | 5 | 490–517 | 6,6–7,9 % | 11 087 915 | 518 234 | 967 619 |
| 000100 | 10 | 413–436 | 3,5–4,5 % | 10 141 555 | 755 069 | 1 230 636 |
| 000000 | 10 | 747–778 | 4,4–5,5 % | 12 804 414 | 881 908 | 1 567 191 |
| 000200 | 10 | 814–842 | 4,2–5,4 % | 15 011 919 | 977 537 | 1 583 462 |

Ratios :

- supports acceptés par site : 11,2–11,5 à K5, 21,2–22,1 à K10 ;
- candidates par supports acceptés : 18,5–21,4 à K5, 13,4–15,4 à K10. Le census rejette donc environ 93–95 % des candidates.

Passage de 24 à 48 fils :

- R1 `probe_6` contre `probe_0` (000000/K5) : 490,2 → 456,1 ms, soit ×1,07 ;
- R3 `probe_13` contre `probe_2/3/12` (K10) : 971,2 → 760–783 ms, soit ×1,24–1,28 ;
- R5 `probe_12` (K10) : 962,6 → 755–778 ms.

La topologie G4 compte 24 cœurs × 2 SMT (`g4_tower_r5_20260923/vm/lscpu.stdout`). Le mur q2 actuel est donc près de son plancher CPU.

Mesures locales, hôte partagé à 8 CPU, W8, ordre de grandeur seulement :

- 000000/K5 : 1,515 s (`lidar_scaling_local_partial_20260923`) et 1,967 s (`first_tower_20260922`), soit au plus 12–16 fils·s ;
- sous-nuages emboîtés 8k/16k/32k à K5 : 419 → 981 → 2 211 ms, soit ×2,34 puis ×2,25 par doublement. Diagnostic seulement, les sous-nuages ne sont pas homogènes.

## 6. Défauts et risques

Il n'y a pas de défaut de correction. Les constats portent sur la preuve à l'échelle, la mesure et la performance :

- aucun juge d'omission à l'échelle ;
- q2 seul frôle le budget de 1 s à K10 ;
- le grand-livre q2 n'est pas publié (3 compteurs sur plusieurs dizaines) ;
- trois leviers exacts ne sont pas mesurés en v9 :
  - `{4,all,true}`, avec −33 à −63 % de visites de census sur LiDAR selon l'auditeur A v8 ;
  - Donate ou un grain plus fin ;
  - une charge utile réduite aux comptes ;
- l'hygiène n'est pas faite (code hérité compilé, nœuds non réservés, arbre de plages inutile) ;
- la preuve reste dispersée dans les notes v8 ;
- deux index séquentiels coûtent 20–27 ms.

## 7. Autres implémentations pour le contrat (LiDAR, K5/K10, G4)

### A. Leviers exacts déjà dans le code, à ablater sur G4 avec le grand-livre q2

1. **Propositions `{4,all,true}`.** Aucun changement de code hors un levier.
2. **Donate, ou `jobs_per_worker` à 64 ou 256.** À mesurer à W24 et W48 avant de retirer Donate, comme le prévoit `HERITAGE_V7_V8.md:41`.
3. **Charge utile réduite aux comptes**, ou identifiants transmis au catalogue.
4. **Chevauchement de q2 avec q3/q4.** Les deux voies sont indépendantes à index fixé, avec des tranches de sortie séparées. Le gain au mur est au plus le mur q2, soit 0,23–0,84 s, et seulement s'il reste du CPU inoccupé. Supposé, à mesurer.

### B. GPU exact

La voie q2 se prête au GPU :

- le front se parcourt par vagues de tâches de 72 octets, sans pile ;
- la recherche de témoins est une descente suivie d'une fenêtre ;
- le census par paire est sans pile grâce aux échappements du préordre (`count_pair`) ;
- l'arithmétique est exacte en fp64, ou en i64.

Il y a 9 à 15 millions de candidates par trame, et la charge utile se prête à un schéma count + scan. Comme la séparation n'entre dans aucun certificat, la partition peut être choisie pour le GPU sans toucher à l'objet. Hypothèse non mesurée.

### C. Parcours par ancre (voisins k-Gabriel)

Il s'agit de `shared_task(a, racine)` avec une propriété b > a en rang : n tâches indépendantes, sans front ni Pool. À prototyper en shadow, en comptant les visites face au grand-livre actuel. Supposé.

### D. Ne pas rouvrir

- **Coupe Yao48.** Elle est prouvée (`docs/math/CATALOGUE_PAIRES_DIAMETRALES_EXACT.md` § 2) et a été mesurée sur G4 GPU : lanceur de 2,434 s et recertification CPU de 8,628 s pour les seules 7,96 M candidates à 50k (`docs/validation/phase15_session_g4_20260807/RESULTATS.md:18-37`). C'est plus lent que toute la voie q2 v9 sur CPU.
- **Filtrer q3/q4 par q2** (`morsehgp3D_v9/docs/FAUSSES_PISTES.md:32`).

## 8. Apport par rapport aux audits A et B

A et B citent q2 surtout de façon agrégée :

- `CONTRAT_COUTS_ET_PARALLELISATION.md:44-64` : q2 + fusion + recensus dépasse 1 s à K10 ;
- `ETAT_COURANT.md:43-48` : aucune complétude des clés omises.

Ce rapport ajoute :

- l'explication q2 relue ligne à ligne ;
- l'isolement du coût q2 et de son plancher SMT ;
- trois juges d'omission q2 concrets, dont un lemme vérifié par script ;
- la vérification exacte des bornes et l'exactitude fp64 ;
- trois leviers existants non mesurés, étayés par des reçus v8 LiDAR ;
- le gaspillage de la charge utile ;
- les points d'hygiène laissés ouverts depuis l'audit v8.
