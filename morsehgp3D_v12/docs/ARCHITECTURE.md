# Architecture proposée pour la v12

7 octobre 2026. Proposition tirée de l'audit géant de la v11 (§ 7 et § 9) et des conceptions d'origine de la v11
(`../../morsehgp3D_v11/receipts/conception_v11_20261002/conception/`). **Les budgets sont des hypothèses** : chacun est
confirmé ou révisé par un microbanc sur G4 avant le port de l'étage ([`PLAN.md`](PLAN.md)).

## 1. Règles de simplicité

1. **Un seul chemin produit, qui est le chemin mesuré.** Pas de masque d'options dans le produit. Les variantes de
   recherche vivent dans des microbancs, hors du produit, avec des noms (jamais un entier opaque).
2. **Une seule implantation par noyau.** La feuille du catalogue est écrite une fois, en source unique, compilée en
   SIMD sur l'hôte et en CUDA sur l'appareil ; le DFS scalaire de la v11, gelé, reste un témoin de test, jamais un
   second produit ni le chemin des feuilles larges, que la même source rejoue en exact plus large.
3. **Un seul profil de quantification dans le produit**, choisi selon la décision D6 (le plus large qualifié dont le
   surcoût sur LiDAR reste sous 3 % de u21) ; pendant le développement, u21 est la base de mesure, u24 et u32 des
   candidats, jamais un second chemin produit.
4. **Des compteurs logiques indépendants de l'ordre de visite**, définis avant la forêt parallèle ; des diagnostics
   physiques séparés, jamais dans une empreinte de sortie. Pas de registre transactionnel recopié à chaque pas : des
   accumulateurs locaux par voie, une garde de débordement unique en fin de voie.
5. **Un départage canonique invariant par translation** partout où un ordre est publié : ordre lexicographique des
   coordonnées exactes (centres, supports), jamais le rang de Morton.
6. **Transactions** : une opération rend son résultat entier ou un refus ; jamais un préfixe publié (règle v11 conservée).
7. **Mémoire comptée** : hôte, mémoire épinglée et appareil dans un même budget ; arènes et caches **comptés** (le cache
   de blocs de la v11 ne l'était pas) ; plusieurs pilotes déclarés explicitement.
8. **Déterminisme** : sorties identiques à l'octet quel que soit le nombre de fils et la voie (CPU ou GPU).
9. **Exactitude** : doctrine numérique F1–F6 et budgets `constexpr` de la v11, portés tels quels.
10. **Pas plus de fichiers qu'il n'en faut** : la v11 comptait 22 200 lignes de moteur, 50 300 de tests, 19 100 de bancs
    et 357 Mo de reçus ; la v12 vise moins de modes, moins de doublons, et des reçus sans copies de sources.

## 2. La Session résidente

Une `Session` possède, et ouvre une seule fois : le budget mémoire, le Pool de fils, le contexte CUDA, ses flux, son
pool de mémoire d'appareil et ses tampons épinglés, les arènes réutilisées d'une trame à l'autre, les modules CUDA
chargés. Elle reçoit des trames successives. Le temps à froid (ouverture comprise) est publié à côté du temps à chaud.
Mesures de `MES-M6` sur G4 ([reçu](../receipts/g4_t0a_20261007/README.md)) : contexte 116 ms, payés une fois ; puis
8 µs par lancement synchronisé, 11 µs pour un graphe de dix noyaux, 20 µs pour copier une trame de 60 000 sites ;
la Session attend en mode `yield` (l'attente bloquante double le coût des petites copies) et groupe ses suites de
lancements en graphes.
Si la décision D2 retient la cadence, la Session peut recouvrir le catalogue de la trame $t+1$ (GPU) et la tour de la
trame $t$ (CPU).

## 3. Le pipeline et ses budgets (K = 5, trame de type ng00, G4, à chaud)

| Étage | Rôle | Au gel de la v11 | Budget v12 (hypothèse) | Changement |
| --- | --- | ---: | ---: | --- |
| P | entrée, tri de Morton, index radix, préparation de la Session | < 1 ms | ≤ 5 ms | port v11 |
| C | catalogue $\mathrm{Cat}_K$ résident sur le GPU, en flux | 138 ms (GPU) ; 199 ms (CPU) | ≤ 35–45 ms | **algorithme** |
| G | résolution des graines (descentes) | 63–83 ms | ≤ 25–30 ms | **algorithme** (plus petite boule certifiée) |
| T | noyau union-find par ordre, recouvert par G | 81–102 ms (publieur de l'ordre 5) | 8–12 ms, recouvert | **algorithme** (sans lots) |
| M, V | contraction des plateaux, numérotation, verticales | collées au publieur | ≤ 5 ms | **algorithme** |
| R | registre d'événements | — | à mesurer | nouveau |
| **Total** | | **212–314 ms** | **≈ 80–100 ms** | |

À K = 10, l'objectif est de 0,3 à 0,5 s (catalogue résident 70–130 ms estimé ; résolution 150–250 ms ; noyau 20–35 ms).

**Mesures de la tranche T0** (G4, microbancs hors produit, 7 octobre 2026 ; reçus
[`g4_t0a`](../receipts/g4_t0a_20261007/README.md), [`g4_t0b`](../receipts/g4_t0b_20261007/README.md),
[`g4_t0c`](../receipts/g4_t0c_20261007/README.md)), sur ng00 à K5 sauf mention :

- **C** : parcours des boîtes en largeur 4,7 ms, transferts compris (`MES-M5`, contre 47,8 ms pour la frontière et la
  passe unique de la v11) ; feuille J3 11,6 ms, noyau seul (`MES-M2`, contre 69,6 ms) ; soit environ 16 ms avant la
  fin d'étage, dans le budget de 35 à 45 ms. À K10 : 10,9 et 37,2 ms.
- **G** : résolution de toutes les descentes à un fil, 1,37 s avec la plus petite boule certifiée (`MES-M3`), contre
  1,47 s pour la réplique de la v11 : à K5 le recensement domine, et le gain n'est que de 7 % (il est de 45 % à K10).
  Même parallèle à 48 fils, cet étage dépassera son budget de 25 à 30 ms sans le recensement borné aux $k$ plus
  proches et le mémo de cellule (`LEM-T3`) : **c'est le prochain verrou**, à traiter en T2.
- **T, M** : noyau de l'ordre 5 en 8,8 ms à un fil, ordres indépendants ; contraction 1,7 ms à 48 fils (`MES-M4`). À
  l'ordre 10 : 26,4 ms et 4,1 ms.
- **Session** : 116 ms d'ouverture payées une fois ; quelques microsecondes de coûts fixes par trame (`MES-M6`).

## 4. Étage par étage

### 4.1 C — catalogue résident sur le GPU

- **Parcours des boîtes en largeur sur l'appareil** (profondeur au plus $3B$, soit 63, 72 et 96 aux profils 21, 24 et
  32, par le potentiel $\sum_i\lceil\log_2\text{largeur}_i\rceil$ de `boxes.cpp` ; la borne de 38 niveaux écrite
  ici d'abord était fausse : deux témoins u21 atteignent 60 et 63, `CST-0205` ; le nombre de nœuds a son propre budget) : réservoir des $3K$ témoins les plus proches
  par nœud (clé : distance puis rang dans la liste parente), filtre G1 par couple nœud–site dans le repère du parent,
  natif `i64` tant que $2s+4\leq 63$ (réservoir compris, $s\leq 29$ ; [contrat numérique](CONTRAT_NUMERIQUE.md) § 3),
  voie contrôlée au-delà ; compactage stable par préfixes, enveloppe par réduction segmentée, bissection.
- **Feuilles consommées en flux** pendant le parcours, par un noyau **data-parallèle** : phases de la feuille J3
  (paires, puis triplets avec termes de paire et table H, puis quadruplets par ET de trois lignes de H, census par vote
  du warp) ou forme « cohérente » (tout le warp sur un même préfixe). **Choix fait par `MES-M2` sur G4 le 7 octobre :
  J3 par phases, 168 registres** (`j3_r168`, 0,18 du témoin un-fil de la v11 en moyenne géométrique, 11,6 ms contre
  69,6 ms à K5/24 et 37,2 ms contre 183,9 ms à K10/24 sur ng00, noyau seul ; forme cohérente rejetée ;
  [reçu](../receipts/g4_t0a_20261007/README.md)). **Jamais un fil par feuille** (3 fils actifs sur 32 en v11).
- **Arènes proportionnelles aux émissions** (environ 16 octets par boule et 1 par incidence), et non aux feuilles.
- **Fin sur l'appareil** : tri radix des clés F3, chaînes de voisins non certainement ordonnés résolues en exact, rangs,
  CSR, table $S^{*}$ → boule ; rapatriement compact ; niveaux exacts matérialisés à la demande, seulement pour les rangs
  distincts.
- **Contrat R7 porté** : seuls les chemins `i128` prouvés décident sur l'appareil ; sinon `unresolved`, rejoué en exact
  avant admission, **en parallèle** (le repli de la v11 était en série).
- **Référence CPU** : le même noyau J3 sur l'hôte (warp simulé), rejoué en exact plus large pour les feuilles larges ;
  le DFS de la v11, gelé, ne reste qu'un témoin de test ([contrat du catalogue](CONTRAT_CATALOGUE.md) § 2).
- **Ce qui est porté** : contrat G1–G4, filtres de feuille (`LEM-CLIQUE`, `LEM-ZONO`, `LEM-R`, `LEM-DEFER`), $S^{*}$
  canonique (avec départage par coordonnées), admission $p+q\leq K+1$, clés F3/F4, budget transactionnel.
- **Risques** : le gain d'une feuille data-parallèle (×3 sur le noyau un-fil) n'est pas établi ; la feuille coopérative
  par paires a échoué en v11 ; reproduire exactement l'ordre du réservoir et les compteurs ; mémoire des listes par
  niveau sur l'appareil.

### 4.2 G — résolution des graines

Contrat, profil mesuré à K5 et leviers à juger : [`CONTRAT_TOUR.md`](CONTRAT_TOUR.md) § 4.

- **Fonction pure** du domaine immuable ; elle ne lit jamais la structure d'union : toute la résolution se calcule en
  parallèle.
- **Plus petite boule proposée puis certifiée** (`LEV-MEB-CERT`, mécanisme de la v10 et décision D-G3 de la conception
  d'origine) : proposition en flottant (Welzl), qui ne décide rien ; si le support proposé $S$ est le $S^{*}$ d'une
  boule $b$ du catalogue et que $S\subseteq F\subseteq P_b$ (deux inclusions testées sur les identifiants ; sans la
  première, le carré `WIT-T1-CARRE` certifie une fausse boule), la boule est certifiée sans arithmétique (`LEM-T1`) ;
  sinon certificat exact
  du support proposé, puis canonisation parmi les points de $F$ sur la sphère ; repli exact. L'énumération exhaustive
  (74 présentations par boule à l'ordre 10 en v11) ne reste que dans l'oracle.
- **Mémo de cellule déterministe** (`LEM-T3`) : arrêt sur la première cellule de fenêtre, pointeurs datés suivis après
  coup ; aucun atomique partagé.
- **Census borné** : arrêt au seuil $k$ ; saut vers les $k$ plus proches avec un comparateur exact.
- **Table de populations** (`LEM-POP`) produite par le catalogue, compacte (cases à étiquette vérifiées contre la CSR),
  gardée par la Session (en v11 : 44 Mo à K5 et environ 360 Mo à K10, reconstruite à chaque appel).
- **Garde de date** : date initiale strictement inférieure au niveau de la cellule (témoins `WIT-MEMO`, `WIT-D2`).

### 4.3 T, M, V — forêt sans lots et verticales

- **T** : pour chaque ordre, un noyau union-find par taille sur des événements binaires (20 octets), un fil propriétaire
  par ordre, recouvert par G ; les propriétaires résolvent quand ils attendent (D-F1, D-F2). Prototype de la conception :
  ×2,6 plus rapide que le Kruskal par lots de la v10, forêts identiques.
- **M** : contraction parallèle des plateaux (`LEM-T4`), numérotation canonique par tri parallèle (rang, plus petite
  naissance). Les plateaux sont presque singletons sur le LiDAR (1,02 cellule par plateau à l'ordre 5, observation et non borne :
  la famille alignée $\lbrace(i,0,0)\rbrace$ met $b_k-1$ événements dans un seul plateau) : une barrière par plateau est
  exclue.
- **V** : image d'une naissance en $O(1)$ depuis la jonction de la même boule à l'ordre $k-1$ (`LEM-T6`) ; image d'une
  fusion par une requête d'ancêtre sur l'historique d'attache (`LEM-T5`) ; naturalité contrôlée dans les portes.
- **Second recours**, seulement si G descend sous le temps du noyau : la forêt comme arbre couvrant minimal parallèle et
  déterministe (`LEM-MSTC`), sur un ordre total des arêtes (rang, boule, indice de graine), en contractant le **lien
  choisi**, jamais toute l'étoile (piège `REG:238`).

### 4.4 R — le registre d'événements

Une seule structure par ordre, produite au fil du calcul (et non par un balayage a posteriori : l'`attach` sériel de la
v11 coûtait 75 ms) :
- nœuds : rang exact, parent, genre (naissance, fusion), référence de la boule de naissance, vie rapportée à
  $\delta=\sqrt{3}/2$ mm ;
- hyperarêtes de fusion retenues par Kruskal, avec leurs branches $\mathrm{ant}(b)$ ;
- verticales ;
- pendaisons de points ($H^{r}_{K+1}$) ;
- sur demande, incidences coface–facette pour les masses du § 9.1 (séquence compter, réserver, remplir).

**Toutes les sorties en sont des vues**, chacune avec son lecteur en bibliothèque standard et la signature commune
`tree_k_sha256` : `full` compact ; squelette (arbre couvrant d'ordre K, $S^{*}$ seul, départage par coordonnées) ;
`points` ; `condense` ; `plat` ; selon D11, `coverage_v1` et `weighted_gabriel_v1` (pilotes Zoltan) ; selon D13, un
`CertifiedTowerInput` v2 ; les polyèdres d'ordre $k$ à la demande (D12).

### 4.5 Écriture

SHA-256 avec SHA-NI, ou calculé hors du chemin critique ; tampons d'écriture réels (le `setvbuf` de la v11 laissait
glibc choisir 4 Kio) ; colonnes écrites en bloc ; un format `full` compact (le vidage de la v11 pèse 301 Mo à K5 et
1,46 Go à K10 sur ng00, soit 5,8 To pour une passe sur les 19 130 scans d'entraînement de SemanticKITTI).

### 4.6 Échelle : du petit nuage à plusieurs millions de sites

La décision D7 ajoute deux régimes au contrat principal : des scènes LiDAR réelles de **plusieurs millions de sites**,
et des **petits nuages** de 100 à 10 000 sites. Un seul chemin les sert tous ; seule la taille des lots change.

**Volumes par site** (v11, [`MESURE.md`](MESURE.md) § 3.3) : sur le LiDAR sans sol, 33 boules et 153 incidences par
site à K5, 138 boules et 1 137 incidences à K10 ; sur nuages uniformes, 75 à 79 boules par site à K5. Mémoire de
l'appareil de la v11 : 6 Ko par site à K5, 36 Ko à K10. Projection à 5 millions de sites : 164 M boules et 0,77 G
incidences à K5 ; 690 M boules et 5,7 G incidences à K10, au-delà de $2^{32}$, et 178 Go d'appareil au tarif de la
v11, contre 96 Go sur la carte de G4 (hôte : 180 Gio, 48 fils).

**Règles** :

- **Types** (`CST-0212`) : chaque espace d'indices déclare son domaine exact (naissance, événement brut, nœud final,
  rang, feuille, boule, décalage), sa sentinelle exclue des indices valides, et un refus explicite à sa vraie limite ;
  un codage qui réserve un bit de genre n'a que 31 bits utiles et le dit. Le nombre d'objets, le dernier indice et le
  `PointId` externe (qui peut valoir `0xffffffff`) ne se confondent pas. **Décalages et compteurs sur 64 bits**
  partout, prototypes de forêt compris (CSR d'incidences, listes de représentants, volumes, budgets) : 5 millions de
  sites à 1 137 incidences par site font 5 685 000 000 incidences.
- **Catalogue en flux par lots de feuilles** : le parcours des boîtes produit les feuilles dans l'ordre de Morton ; un
  lot tient dans un budget d'appareil fixé par la Session ; ses boules et incidences sont rapatriées dans des CSR de
  l'hôte, puis l'appareil est réutilisé. Un nuage de 60 000 sites tient en un seul lot : le contrat principal ne paie
  rien pour le flux.
- **Tour** : les événements de chaque ordre sont triés par rang (tri par base, parallèle) ; le noyau union-find
  (`LEM-T4`) est le seul passage séquentiel par ordre, et les $K$ ordres sont indépendants jusqu'aux verticales, donc
  traités en parallèle.
- **Mémoire en trois temps** (`CST-0211`) : une **prévision** tirée des lois par site mesurées dimensionne les lots,
  sans rien garantir ; une **admission certifiée** vient d'une passe de comptage exacte par lot avant toute
  matérialisation (ou d'une borne combinatoire démontrée, souvent trop pessimiste : $C\leq\sum_{q=2}^{4}\binom{n}{q}$
  boules, $I\leq nC$ incidences, $b_k\leq C$ naissances, $N_k\leq 2b_k-1$ nœuds, ces entiers étant eux-mêmes bornés
  avant tout produit) ; une **réservation effective** contrôlée rend un refus transactionnel (`resource_exhausted`),
  jamais un préfixe publié. Pas de stockage sur disque dans la v12 : la v5 l'avait conçu (`morsehgp3D_v5/docs/ECHELLE.md`), il n'est
  repris que si une scène réelle utile dépasse l'hôte.
- **Multiplicités** : les nuages agrégés contiennent des doublons au millimètre ; refus par défaut, option « sites
  distincts » déclarée (décision D8).
- **Petits nuages** : les coûts fixes (lancements, allocations, contexte) sont payés une fois par la Session ; sous un
  seuil mesuré (`MES-P`), la voie CPU complète sert la trame sans toucher l'appareil.

## 5. Modules

Un module = un dossier `src/<module>/` = un en-tête public `src/<module>/<module>.hpp`. Un module ne dépend que des
modules placés avant lui dans la table ci-dessous, sans cycle. Cette table est celle des **modules présents** ; sa copie
lisible par CMake est `cmake/modules.cmake`, et `tools/check_style.py` (porte `mhgp12_style`) refuse tout écart entre
les deux. Une tranche qui livre un module ajoute sa ligne ici et dans `cmake/modules.cmake`, dans le même commit.

| Module | Rôle | Dépend de |
| --- | --- | --- |
| `core` | statuts et raisons, `Result`, budget mémoire, `Buffer`, `Csr`, registre de compteurs ; port v11 (budget à étendre à la mémoire épinglée et à l'appareil, arènes) | — |
| `num` | entiers à budget, prédicats exacts, certificats, clés F3/F4, racines et sommes de radicaux ; port v11 | `core` |
| `sched` | `Pool`, `parallel_for` ; port v11, à réécrire (le Pool v11 réveillait 47 fils par appel ; flux CUDA) | `core` |
| `cloud` | domaine, sites en ordre de Morton, multiplicités, table site → `PointId` ; port v11 | `core` |
| `io` | lecture `u32le`, SHA-256, écrivains, dossier transactionnel ; port v11 | `core`, `cloud` |
| `index` | arbre radix de Morton, bornes et census exacts sur sites ; port v11 | `num`, `cloud` |
| `catalogue` | parcours des boîtes en largeur (source de `MES-M5`), feuille J3 en source unique (source de `MES-M2`) jouée en flux sur le warp simulé, repli exact plus large, fin d'étage partagée (tri par base, sommes préfixes), table $S^{*}$ → boule, export `MHGP12DP` ; réécrit (§ 4.1) ; voie CPU de référence et voie appareil (T1-b, même code, exécuteurs Pool et CUDA) | `num`, `sched`, `cloud`, `io` |
| `tower` | étage G (résolution) : cellules de fenêtre (naissance, jonction, inerte) et leurs traces strictes, table de populations (`LEM-POP`), résolution des représentants (`LEM-T1`, certificat exact, census gardé, arrêt `LEM-T3`), cibles de 4 octets ; étages T (noyau union-find par taille sans lots), M (contraction des plateaux, numérotation canonique), V (verticales par `LEM-T6` et `LEM-T5`), R (registre) et export FUL1 de la v11 ; réécrit (§ 4.2, § 4.3), voie CPU de référence | `core`, `num`, `sched`, `cloud`, `io`, `index`, `catalogue` |

Modules prévus, ajoutés à la table ci-dessus par leur tranche :

| Module prévu | Rôle | Origine |
| --- | --- | --- |
| `registry` | registre d'événements | nouveau |
| `views` | `full`, squelette, `points`, `condense`, `plat`, exports | port des règles v11, nouvelle structure |
| `api`, `cli` | `Session`, un exécutable à sortie obligatoire | port v11 |

## 6. Ce qui reste hors du produit

Les microbancs (`microbancs/`, un dossier par mesure `MES-*`, règle d'adoption écrite d'avance, juge à trois
verdicts) peuvent lier la v11 gelée pour leurs vidages et témoins ; ils échappent pour cette raison au contrôle de
style du produit ([`../microbancs/README.md`](../microbancs/README.md)). L'oracle exhaustif borné (`reference/`, $n\leq 14$) ; l'énumération exhaustive des supports ; la mosaïque d'ordre $k$
(en aval, à la demande) ; la tour pondérée (refus explicite tant que le contrat n'est pas prouvé) ; les coquilles
étendues au-delà d'un plafond déclaré (refus explicite) ; tout juge qui re-vérifie un théorème dans le chemin produit.

## 7. Règles du code portées de la v11

Le socle (modules du § 5, harnais de portes, lanceur de mutants, contrôle de style, oracle borné `reference/`) est un
port explicite de la v11 gelée (`ac081a06f`), consigné dans [`PROVENANCE.md`](PROVENANCE.md). Valent pour tout le code
de la v12 les paragraphes suivants de l'architecture de la v11
([`../../morsehgp3D_v11/docs/ARCHITECTURE.md`](../../morsehgp3D_v11/docs/ARCHITECTURE.md)), que les commentaires du
code citent comme « ARCHITECTURE.md de la v11 » : règles de propreté (§ 1), profil numérique (§ 3), doctrine flottante
F1–F6 (§ 4), construction et portes (§ 5), contrats du budget mémoire, de l'opération atomique et des identifiants
(§ 7). Ils s'appliquent avec ces adaptations :

- préfixes `mhgp12` (cibles, portes, namespace, jetons), `MHGP12_` (macros, options) et `hgp12_ref` (oracle) ;
- profil `MHGP12_COORD_BITS` : 21 (défaut), 24 ou 32. Le profil 18 est abandonné (décision D6) et refusé à la
  configuration comme à la compilation ; 32 est admis depuis l'arithmétique en repère local
  ([`CONTRAT_NUMERIQUE.md`](CONTRAT_NUMERIQUE.md)) ;
- la table des modules est celle du § 5 ;
- variables d'environnement : `MHGP12_DATA_DIR` (portes `lidar`), `MHGP12_V10_FROZEN_DIR` (portes `diff_v10` de
  l'oracle), `MHGP12_V11_CATALOGUE_DIR` (vidages `cat.bin` de la v11 gelée par cas, portes `diff_v11` du catalogue),
  `MHGP12_V11_TOWER_DIR` (vidages `cat.bin`, `ordre_<k>.bin`, `foret_<k>.bin` de la v11 gelée par cas, portes
  `diff_v11` de la tour) ;
- l'option CUDA revient avec la voie appareil du catalogue (tranche T1-b) : `MHGP12_ENABLE_CUDA` (désactivée par
  défaut, `sm_120`), exclusive des sanitizers ; sans elle, la voie appareil rend `device_unavailable`.
