# Lentille 3 — Générateur q3/q4 : audit C

Cadre : `phase=exploration_v9_hors_registre`, `backend=reference_cpu`, `profile=quantized_u18_input_only`, `mode=audit_independant`, `public_status=not_claimed`. Lecture seule au worktree `origin/main` **0125dc18** ; commit développeur non poussé `4530644b` (sonde v12) lu pour comparaison. Aucun build, aucun GCP. Un seul contrôle exécuté : un script Python exact (`bounds_u18.py`, scratchpad `agents/lentille3/`), `nice -n 19`, quelques secondes CPU. Étiquettes : **[prouvé]** (preuve écrite et relue ligne à ligne), **[testé]** (porte, fixture ou script, avec sa taille), **[mesuré]** (reçu épinglé), **[supposé]**.

## 0. Verdict court

1. Je ne trouve **aucun défaut de correction** dans le chemin q3/q4 de la chaîne. J'ai relu chaque prédicat, chaque seuil et chaque égalité. Les bornes 18 bits tiennent partout ; les commentaires périmés relevés par B et par l'audit v8 subsistent (§4).
2. Complétude par arête : l'induction q4 de A (`Q4_INDUCTION_ATLAS_EVENEMENTS_20260923.md`) est correcte. J'écris l'induction q3 symétrique (§3.1), avec un **lemme neuf** : un centre q3 ne tombe jamais dans une cellule `Outside`, sauf pour les arêtes dont la lentille ne contient qu'un seul site (§3.2, constat L3-01). Ce lemme explique exactement pourquoi `q3_atlas_outside_domain` vaut la même chose à K5 et à K10 (20 845 / 17 683 / 21 868 dans R7b). C'était la question ouverte 10 de l'audit v8.
3. Le trou réel reste **exécutable** : les certificats v9 (cœur, preuve sur cover, cache, census sur feuille, saturation) ne sont jugés contre un oracle exhaustif que sur de petits nuages. Aucune porte d'**omission** ne tourne à l'échelle d'une trame (L3-06).
4. Coûts (R7b) : q3/q4 prend **55–74 %** de la chaîne. Son mur W48 n'a baissé que de 5–10 % depuis R5. Deux leviers mesurables sortent de ce registre :
   - la preuve sur cover complet est **à peu près neutre** (L3-02) ;
   - une restructuration **par feuille d'atlas** unifie les graines q3 et q4 et supprime deux DFS globaux par arête (L3-04).
5. Ni ces leviers ni leurs combinaisons ne tiennent le contrat d'1 s sur CPU. Pour K10, et pour 100 ms, il faut un travail par arête sur GPU. Les données le permettent : cover moyen de 252–480 sites, cœur de 114–217 sites, arithmétique des certificats et de l'atlas entièrement en i64 (§7).

## 1. L'objet exigé

Une boule k-Gabriel émise par la voie q (q = 3 ou 4) est la circumboule d'un **support positif** : triangle strictement aigu, ou tétraèdre dont le centre est strictement intérieur. Sa profondeur stricte p doit vérifier `p < h_q = K−q+2`, soit `p < K−1` en q3 et `p < K−2` en q4. Ces seuils sont dans `wspd/front.cpp:92` (`thresholds_[lane]=kmax_−lane`), `lanes/q34_dead_lanes.cpp:97–98`, `pipeline/wspd_q34.cpp:621` et `lanes/q4_local.cpp:441`.

L'arête propriétaire est l'arête la plus longue du support, avec départage par la plus petite paire d'IDs. La règle est identique dans `wspd_q34.cpp:719–731` (q3) et `q4_local.cpp:27–37` (q4).

Le flux émet des **présentations** (clé `ExactBall`, support, profondeur, coquille complète). Le catalogue les déduplique. Il recense aussi exactement chaque clé émise et vérifie `q_min` (`src/chain/tower_chain.cpp`, PROVENANCE §Chaîne). Une clé émise fausse est donc détectée à l'exécution ; une clé **entièrement omise** ne l'est pas. Tout l'enjeu de complétude porte sur l'omission.

## 2. Parcours pas à pas (chemin de la chaîne, `tower_chain.cpp:347–363`)

**2.1 Front WSPD.** Il fait une décomposition LCA des paires non ordonnées (`front.cpp:113–128`). Il émet un produit séparé (`gap² ≥ s²·diag²`, :145–156) ou le scinde. Pour chaque voie, un témoin z est crédité si `H_min(A,B,z)>0` et `α·H²>Ξ_haut`, avec α3=3 et α4=2 (:315–331). Une voie tombe à K−1 (q3) ou K−2 (q4) témoins distincts.

Le calcul de `H_min` (`spindle/predicates.hpp:84–98`) est exact sur les boîtes continues. `Ξ_haut` (:61–79) est un majorant conservatif. Le registre des masses par voie est vérifié à la fin (`front.cpp:191–199`).

Le lemme du citron : pour un support positif d'arête maximale ab, `puissance(z) ≤ −H + √(Ξ/α)`, parce que `‖c−m‖² ≤ D/(4α)`. Il est **[prouvé]** (v8 `Q34_GLOBAL_ET_LIDAR_20260921.md` §Preuve), et je l'ai recalculé.

**2.2 Rectangle et expansion.** En mode `RectanglePair`, le même filtre tourne d'abord sur les boîtes des nœuds A×B (`wspd_q34.cpp:442–456`). Il utilise les bornes exactes de 4H (`q2_joint_bounds.hpp:51–74`). Le produit résiduel est ensuite développé paire par paire, une seule fois (:463–474, :498–504). En parallèle, les tâches sont des plages disjointes de rangs `a`, et l'identité `published=consumed` est vérifiée (:975).

**2.3 Filtre de paire et cache de témoins.** Pour des extrémités ponctuelles, la recherche `Affine` est exacte (`q34_witness_search.cpp:105–213`). L'admission est stricte : `α(4H_min)²>16Ξ_haut` (:174). Une exclusion ne donne jamais de crédit ; un nœud admis retire sa voie du masque de ses enfants.

Le cache (`:227–280`, `wspd_q34.cpp:524–538`) rejuge exactement, pour la nouvelle paire (a,b′), les nœuds admis de la dernière recherche de même `a`. Il refuse les recouvrements par voie. Il est sain.

**2.4 Cœur diamétral et certificat de voie morte.** Les centres des boules possédées vérifient `3|u₁A+u₂B|² ≤ D` (q3) et `2|…|² ≤ D` (q4) (`q34_dead_lanes.cpp:13–14`). La racine `[−2,2]²` contient les deux disques, puisque la plus petite valeur propre de la Gram vaut `h² ≥ D/3`.

Une cellule fermée est prouvée si elle est disjointe du disque (`outside`, :108–122, minorant par axe) ou si elle porte au moins T sites distincts de forme strictement négative à ses quatre coins (:154–161). T vaut K−1 pour q3 et K−2 pour q4.

La réfutation par la profondeur en un coin (:170–193) est une décision de coût seulement. Sur le cœur, ce compte n'est qu'un minorant ; cela ne change rien à la sûreté (B, `CONTRE_AUDIT_B_NOYAU_DIAMETRAL_WIP`). Le certificat est tenté d'abord sur le cœur `|2z−a−b|² ≤ |b−a|²`, puis sur le cover pour les voies restées ouvertes (`wspd_q34.cpp:549–574`). **[prouvé]**, relu.

**2.5 Cover.** C'est la boule fermée `|2z−a−b|² ≤ 4|b−a|²` (`edge_cover.cpp:44, 53–60, 88–131`). Elle contient toute boule possédée, car `R+|c−m| ≤ 0,866·|ab|` en q3 et `0,966·|ab|` en q4. Le cœur est refusé à tout consommateur autre que le prouveur (`require_complete_q34_cover`).

**2.6 Atlas des centres q4 (Local28).** Il n'est construit que si les deux voies survivent et K ≥ 3 (`wspd_q34.cpp:582–586`). Le domaine est `Positive` : `Z` est l'ensemble des sites de la lentille fermée hors extrémités, et sa boîte englobante est exacte (`q4_positive_domain.cpp:86–143`). La coque tient compte des huit coins projetés et de 0 (`q4_local_partition.cpp:159–215`).

Une cellule est `Outside` dans trois cas : disque (:227), `completion_count<2` (:228), facettes (:229–239). Les fragments partitionnent le cover (`:364–433`) en trois classes :
- sites uniformément intérieurs (`max<0`, :405) ;
- sites strictement extérieurs (`min>0`, :414) ;
- sites actifs, **y compris `min==0`**.

`Deep` vaut `inside ≥ K−2` (`q4_local.cpp:218`). Avec `retain_q3_fragments`, le fragment est gardé si le compte est inférieur à K−1 (:220). `saturate_deep` produit un certificat terminal à K−1 sans fragment (:203–207, :168–196). Les arrêts sont `leaf_sites=32`, `max_depth=7` et `node_budget=4096`, avec un raffinement terminal complet (:223–257).

**2.7 Voie q3.** `q3_edge` parcourt l'**index global** avec élagage par lentille et acuité (`wspd_q34.cpp:711–760`). Chaque graine aiguë possédée est localisée par `q3_center`, un point entier exact en i128 (`q4_local_partition.cpp:119–140`), puis par division longue (`q4_local.cpp:79–105`) et descente en cellules fermées (:311–348). Trois cas se présentent :
- `inside_count ≥ K−1` : rejet sans boule (`wspd_q34.cpp:621–624`) ;
- `ExactLeaf` : census sur la frontière, avec profondeur = compte certifié + sites de puissance négative, et coquille = sites de puissance nulle (:634–656) ;
- sinon : `GlobalBoxes` sur l'index global (`q3_ball_census.cpp:76–175`).

L'émission se fait ensuite (:686–694).

**2.8 Voie q4 (LiveOnly).** Elle recalcule les feuilles vivantes (`q4_local.cpp:614–639`), puis fait un DFS global des graines (:689–711). Pour chaque graine, elle descend l'atlas le long de sa droite (:672–687). Dans chaque feuille, elle balaie tous les sites actifs (:374–468) :
- les sites coplanaires sont constants ;
- les événements hors cellule sont écrêtés, avec leur signe au point de référence ;
- les événements restants sont triés par `compare_roots` (déterminant réduit, `q4_family.cpp:94–108`).

Par groupe de racines égales, le code soustrait les sorties, puis teste `inside<K−2`, la propriété, la positivité (`make_q4`) et la canonicité `y<x && acute(a,b,y)` ⇒ rejet (:441–463). Il émet **la première** présentation valide du groupe.

**2.9 Parallélisme.** Une seule équipe exécute les jobs du front, avec une file LIFO de plages. Une arête reste atomique (`wspd_q34.cpp:792–979`). La terminaison et l'annulation sont correctes : aucun blocage, puisque l'attente exige `busy≠0`.

## 3. Complétude

### 3.1 Induction q3 par arête (nouvelle rédaction, complément de A)

Soit `S={a,b,x}` strictement aigu, d'arête propriétaire ab et de profondeur `p<K−1`.

1. **Front.** La paire est dans exactement un produit terminal (ledger `front.cpp:197`). Un rejet q3 exigerait K−1 témoins universels stricts. Par le lemme, chacun serait strictement intérieur à B(S), donc `p ≥ K−1` : contradiction.
2. **Rectangle, paire, cache.** Même argument, avec des bornes sûres (2.2–2.3).
3. **Cœur et cover.** Un rejet exigerait une cellule contenant le centre c avec K−1 sites stricts pour tout centre de la cellule, dont c lui-même : contradiction. c est dans le disque q3, donc dans la racine.
4. **Énumération.** x vérifie `|ax|,|bx| ≤ D` et `|ax|+|bx| > D`. Les élagages de nœuds (:750–755) n'écartent que des boîtes qui violent l'une de ces conditions. Le test aigu et le test de propriété passent.
5. **Atlas.** Le compte certifié de la cellule fermée contenant c minore p, donc il est inférieur à K−1 et ne rejette pas S. Le saut par la racine obéit au même argument.
6. **Census.** Sur `ExactLeaf`, l'invariant du fragment et l'inclusion dans le cover donnent `depth=p` et la coquille complète ; `GlobalBoxes` est exact (bornes de réseau `q3_ball_census.cpp:17–58`).
7. **Émission.** Elle a lieu une fois, depuis cette arête.

**[prouvé sous les invariants de l'index et des prédicats]**, **[testé]** par l'oracle exhaustif de `tests/gen/wspd_q34_gate.cpp:107–186` (tous les triangles et tétraèdres de petits nuages, grille 3×3×2 avec égalités, coquille 30, extrêmes u18).

### 3.2 Lemme `Outside` (constat L3-01)

Pour une graine aiguë possédée x ∈ Z, on a `2(c−m) = λ_x·proj_{v⊥}(2(x−m))`, avec λ_x ∈ ]0,1[. Le centre appartient donc au segment [0, proj(2(x−m))], lui-même contenu dans la coque {0} ∪ coins projetés. Il vérifie aussi `2|t|² ≤ (2/3)D < D`. Aucune cellule fermée contenant c ne peut donc être `Outside` par le test du disque ou par une facette ; seul reste le cas `completion_count<2`.

Réciproquement, si Z={x} et x est aigu, aucun site n'est strictement dans la boule diamétrale. Ni les témoins (H>0 impossible), ni le cœur, ni le cover (la cellule contenant m n'a aucun intérieur au centre m) ne ferment l'arête, quels que soient K ≥ 3 et s. Donc `q3_atlas.outside_domain` égale exactement le nombre d'arêtes dont la lentille ne contient qu'un site, aigu et possédé : c'est indépendant de K et de s.

R7b donne 20 845 / 17 683 / 21 868 à K5 **et** à K10 **[mesuré]**. L'audit v8 (u18) donnait 20 845 à K5, sans cœur ni preuve de voie morte. Ce lemme précise `Q3_STRUCTURE_ET_BORNES.md:132` : le « repli global obligatoire » ne concerne que ces arêtes, soit une graine chacune.

### 3.3 Obligations globales

| obligation | statut |
|---|---|
| front : partition des paires, rejets sûrs | prouvé ; ledger exécuté |
| filtres rectangle, paire, cache | prouvé ; 3 mutants témoins actifs, `admitted_lane_recounted_in_children` toujours DISABLED |
| cœur, preuve sur cover | prouvé ; 7 mutants (seuils, contact, coin, disque q3) |
| induction q3 (3.1), induction q4 (A) | prouvé par relecture ; testé exhaustivement ≤ ~30 sites |
| flux entier à 12 sites, 108 flux | testé (`q4_global_12sites_20260923`) |
| clé difficile dans un index de 8k–32k | testé une clé (`COMPLETUDE_Q4_CLE_REMBOURREE`) |
| omission à l'échelle d'une trame avec les leviers v9 | **manquant** |
| registre des preuves | **manquant** (aucune entrée v8/v9 dans `docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md`) |

## 4. Bornes 18 bits (script `bounds_u18.py`, PASS)

Le script vérifie en valeurs exactes 23 majorants analytiques à M=262 143. Il échantillonne 27 768 triangles aigus et 4 709 tétraèdres positifs aux extrêmes. Il recoupe exactement le centre q3 en coordonnées de cellule avec −B/(2A). Il vérifie les lemmes `|c−m|² ≤ D/12` et `≤ D/8`, l'inclusion boule ⊂ cover et l'existence d'une face aiguë **[testé]**.

| prédicat | majorant | largeur |
|---|---|---|
| puissance `ExactBall` q3 | 216M⁶ (commentaire : 360M⁶) | < 2^117 |
| `make_q4` reste | 396M⁶ | < 2^117 |
| `q3_center` | dét ≤ 512M⁶ < 2^117, numérateurs ≤ 480M⁶ | voir ci-dessous |
| formes du prouveur | — | < 2^62 (i64) |
| bornes atlas | — | < 2^62 (i64) |
| admission des témoins | 192M⁴ | < 2^80 |
| `compare_roots` | 72M⁵ | < 2^97 |
| niveaux q3 | num < 2^113, den < 2^79, produits croisés < 2^192 | — |
| niveaux q4 | num < 2^160, den < 2^120, produits croisés < 2^280 | — |

Le garde `certified_cell` à 2^117 (`q4_local.cpp:284–290`, `312–315`) est **sans marge** : `512M⁶ = 2^117·(1−2^−18)^6`. Tout élargissement à 19 bits lèverait une exception bruyante, jamais une erreur silencieuse.

Commentaires faux ou périmés encore présents :
- `tower/lanes/q4.hpp:18,20`, `tower/lanes/level.hpp:31` : déjà signalés par B, non corrigés ;
- `wspd_q34.cpp:664`, `q34_witness_search.hpp:86–89`, `q34_witness_search.cpp:100` : relevés par l'audit v8 au §15.

## 5. Ventilation des coûts

R7b, répétition 0, MEB ON, W48, s8 **[mesuré]** :

| trame/K | q34 / chaîne | paires développées | cœurs → fermées | covers | atlas | graines q3 / q4 | émis q3 / q4 | tests prouveur | tests points atlas |
|---|---|---|---|---|---|---|---|---|---|
| 000000/K5 | 4,11/5,70 s (72 %) | 23,69 M | 2,04 → 1,14 M | 0,90 M | 0,57 M | 9,3 / 5,8 M | 0,69 / 0,16 M | 1,46 G | 0,27 G |
| 000000/K10 | 8,17/13,93 s (59 %) | 30,78 M | 4,51 → 2,57 M | 1,93 M | 1,33 M | 33,4 / 26,4 M | 2,90 / 1,73 M | 5,23 G | 1,47 G |
| 000100/K5 | 2,52/3,67 s (69 %) | 11,96 M | 1,73 → 0,95 M | 0,78 M | 0,49 M | 7,7 / 4,9 M | 0,58 / 0,12 M | 1,14 G | 0,25 G |
| 000100/K10 | 5,23/9,58 s (55 %) | 17,49 M | 3,67 → 2,02 M | 1,66 M | 1,15 M | 26,8 / 21,6 M | 2,36 / 1,27 M | 3,81 G | 1,31 G |
| 000200/K5 | 4,85/6,57 s (74 %) | 22,72 M | 2,24 → 1,29 M | 0,95 M | 0,60 M | 12,7 / 7,9 M | 0,75 / 0,14 M | 1,80 G | 0,34 G |
| 000200/K10 | 9,84/15,30 s (64 %) | 32,79 M | 4,93 → 2,90 M | 2,03 M | 1,41 M | 42,7 / 35,5 M | 3,02 / 1,49 M | 6,13 G | 1,74 G |

Quotients utiles :
- 4,8–28 paires développées par présentation émise ;
- 11–17 graines q3 par q3 émise, 15–55 graines q4 par q4 émise ;
- 4,8–5,6 événements retenus par graine q4 : le **tri** n'est pas le poste ;
- cover moyen de 252–480 sites, cœur de 114–217 sites.

Historique du mur q3/q4 : R5 2,79–5,19 s (K5) et 5,50–11,20 s (K10) ; R6 2,63–4,63 / 5,32–9,72 ; R7b 2,52–4,85 / 5,23–9,84. Les gains récents sont presque tous du côté de la tour.

La PASSATION (l. 152–154) ventile K10 en W8 locale, en Gcycles : q4 235, atlas 229, paires 187, rectangles 123, cœur 120, q3 120, preuve sur cover 86 (≈ 1 100 au total). Ce n'est pas un reçu. Le reçu `q34_dead_edges_20260923`, avec son erratum, montrait avant le certificat 96,7 % des cycles classés sur des arêtes sans sortie.

Occupation CPU de la chaîne : CPU / (mur × 48) = 40–58 %, sur 24 cœurs à deux fils (SMT) : à ne pas lire comme 50 % de pertes.

## 6. Constats (détail dans `findings`)

- **L3-01** : lemme `Outside` et identité `outside_domain` ; ferme la question v8 n° 10 et fournit une identité gratuite pour la réception.
- **L3-02** : la preuve sur cover complet ne prouve que 19–22 % des voies q3 et 23–27 % des voies q4 qui l'atteignent. C'est la plus grosse boucle comptée (0,80–4,04 G tests). Mon estimation la donne neutre. Ablation à faire.
- **L3-03** : les arêtes q3 seules (7–19 %) portent **99,2–99,6 %** des census `GlobalBoxes`. Un atlas q3 dédié serait à peu près neutre.
- **L3-04** : restructuration par feuille d'atlas (graine ⊂ frontière) pour q3 et q4.
- **L3-05** : garde-fou pour une fusion prouveur/atlas : le prouveur écarte les sites `min==0`.
- **L3-06** : porte d'omission à l'échelle, ciblée sur les arêtes fermées par les nouveaux certificats.
- **L3-07** : mutants manquants (propriété, canonicité, disque q4, élagage des graines, domaine).
- **L3-08** : bornes 18 bits vérifiées, commentaires périmés, garde 2^117 sans marge.
- **L3-09** : registre des preuves vide pour q3/q4.
- **L3-10** : localité mémoire de l'atlas et du balayage.

## 7. D'autres implémentations peuvent-elles tenir le contrat ?

**Budget.** 1 s sur G4 correspond à environ 48 CPU·s en parallélisme parfait. R7b consomme 87–126 CPU·s à K5 et 268–372 CPU·s à K10 pour toute la chaîne. Il faudrait diviser le travail par 2–2,6 (K5) et par 6–8 (K10), avant toute perte de parallélisme ; par 20–80 pour 100 ms.

**CPU, même objet.** Réunis, L3-02, L3-04 et L3-10, avec la palette par octant (shadow A/B, −5,2 M paires avant filtre) et un ticket de témoins avant expansion, visent peut-être 20–35 % du CPU q3/q4 (**[supposé]**, à mesurer). Cela peut approcher 2 s à K5, sûrement pas 1 s à K10. Aucun poste ne domine (≤ 22 % chacun dans la ventilation locale) : un levier isolé ne suffit pas.

**GPU (V9-4).** C'est la seule voie crédible pour K10 et pour 100 ms, et les données la rendent plausible :
- 0,8–2 M covers et 1,7–4,9 M cœurs par trame : un bloc de threads par arête ;
- un jeu de travail de ~17 Ko en mémoire partagée (coordonnées + formes pour ~480 sites) ;
- prouveur et atlas entièrement en i64 (< 2^62, vérifié), donc exacts sur GPU sans i128 ;
- prédicats i128 (puissances < 2^117, `compare_roots` < 2^97, admission < 2^80) : filtre flottant certifié et repli exact ;
- le découpage par feuille (L3-04) fournit l'unité de travail naturelle, une feuille avec |F_C| ≲ 64 sites.

Risques : queue lourde des tailles de cover (maximum non publié) ; copies de frontières de 0,16–1,14 G IDs, à remplacer par une arène ; repli exact pour les grandes coquilles.

**Écarté.** Marche du ≤K-niveau ou Delaunay d'ordre supérieur : interdit par l'invariant d'architecture et `docs/FAUSSES_PISTES.md` l. 14. Graphe kNN : contre-régime de `Q4_STRUCTURE_ET_BORNES`. Filtrer q4 par q3 ou q3 par q2 : fixtures. Fenêtre de tri : le tri ne porte que sur ~5 événements par graine.

## 8. Recoupements

- Parcours cachés : publiés par la sonde v12 du développeur (`4530644b`, non poussé) en réponse au grand-livre de B. Je n'y ajoute que la suppression de deux DFS par arête (L3-04).
- Graines dans le cover : déjà proposées par B (grand-livre) ; L3-04 va plus loin.
- Génération depuis la frontière : A, `Q3_STRUCTURE_ET_BORNES.md:242`, sur cellules 3D par ancre ; L3-04 l'applique à l'atlas existant, par arête, avec le lemme L3-01.
- Bornes q4 périmées : B, `CONTRE_AUDIT_B_U18_ET_SONDE_20260922.md:84–95`.
- Induction q4 : A, confirmée.
- Clé rembourrée : B, `COMPLETUDE_Q4_CLE_REMBOURREE_20260923.md`, dont L3-06 est le prolongement ciblé.