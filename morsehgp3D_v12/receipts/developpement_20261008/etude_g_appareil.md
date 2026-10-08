# Étude : l'étage G (résolution de la tour) sur l'appareil de G4

8 octobre 2026, 09:27 à 10:28 UTC (heures lues par `date -u`). Étude seule : rien n'est modifié dans le dépôt, aucune
commande GCP (**GCP non utilisé** : ni lecture ni écriture ; le plan a été validé hors ligne par les fonctions
`validate_plan` et `validate_data` du contrôleur importées en Python, sans exécuter le script). Lignes de code citées au
commit `b2df42983` (HEAD au début de l'étude), chemins relatifs à `morsehgp3D_v12/`.

```text
phase=exploration_v12_hors_registre
backend=cpu_reference (étage G aujourd'hui) ; cuda_g4 (catalogue ; microbanc de cette étude)
objet=full_pi0 (tour FULL K1..5, verticales comprises)
quantification=quantized_u21_input_only
public_status=not_claimed
```

Sources chiffrées : reçu G4 `g4_t2da_20261008` (Session recouverte, 37 trames `v12set`, bras séquentiel « avant »
compris ; journaux `resultats/cmd/001_t2da_pilote/files/t2da/journaux/v12set/*.jsonl`), reçu `g4_t2db_20261008` (coût
interne de G, profil à un fil), reçu `g4_t0a_20261007` (MES-M6 : appareil, débits PCIe), `lscpu` du reçu T2-d-B ; et
**exécutions locales de cette étude** (codespace, 8 vCPU très chargés, charge 15 à 19) : compteurs déterministes (les
mêmes que sur G4) par `mhgp12_tower_probe` et par le microbanc ; les temps locaux ne jugent rien.

## 0. En bref

- **Faisable** : G est une fonction pure par représentant (`src/tower/resolve.cpp:271-336`), sans dépendance entre
  ordres ; ses trois postes archétypes (sondes de table, census par parcours d'arbre en `i128`, proposition en
  binaire64) s'écrivent en source unique `__host__ __device__` **avec identité à l'octet** : démontré sur l'hôte, sur
  les trois trames de référence (1,19 million de census, 18,9 millions de représentants, 4,58 millions de
  propositions, zéro écart, compteurs du travail compris) ; le noyau CUDA compile pour sm_120 ; la mesure sur
  l'appareil attend la session G4 préparée ici.
- **Gain** : G passerait de 77 à environ 9-19 ms sur la trame médiane (64 740 sites) et de 155 à 19-40 ms sur la
  trame maximale (99 099), estimation à calibrer par la session. **Mais** le mur ne baisse que de 21 à 25 % (médiane
  158 → 120-125 ms ; maximum 308 → 230-240 ms) : le noyau union-find de l'ordre 5 (étage T, séquentiel, environ 55 et
  120 ms sur G4) devient le chemin critique. Le contrat de 100 ms sur la médiane exige G sur l'appareil **et** un T
  parallèle ; le maximum exige en plus un C plus court (77,8 ms).
- **Recommandation** : jouer la session MES-G-APP (6 à 10 min de worker, 3,3 Mo de données, règle écrite d'avance) ;
  si elle adopte, ouvrir la tranche « G sur l'appareil » en deux temps (ouverture puis résolution), **appariée** à un
  chantier T parallèle ; en attendant, deux gains CPU sûrs : census « à plat » et tables de populations produites par
  la fin d'étage du catalogue.

## 1. Le constat

### 1.1 Le mur et ses étages (G4, 48 fils, K5, Session recouverte, catalogue sur l'appareil)

Médianes des passes du bras « après » de T2-d-A (avant le lot T2-d-B, qui retire 7 à 8 % de G) :

| Trame | Sites | Mur | P | C | G (dont ouverture, dont tables) | région de G | queue | noyau T5 après la fin de G5 |
| --- | ---: | ---: | ---: | ---: | --- | ---: | ---: | ---: |
| ng00 (`kitti_ng_08_000000`) | 39 885 | 100,6 | 1,7 | 28,8 | 61,5 (13,7 ; 9,8) | 47,8 | 5,4 | +4,8 |
| médiane `v12set` (`kitti_ng_02_001606`) | 64 740 | 163,7 | 2,8 | 45,1 | 82,1 (19,8 ; 13,5) | 62,3 | 31,2 | +42,2 |
| maximum `v12set` (`kitti_ng_08_002119`) | 99 099 | 319,3 | 4,8 | 77,8 | 167,5 (30,8 ; 20,7) | 136,7 | 67,9 | +92,5 |

ms ; « ouverture » comprend les tables (`full_probe.cpp:39` ; `pipeline.cpp:175-183`). Temps-fils de G sur la trame
maximale : 5 973 ms, soit 60 µs par site (41,7 µs sur la médiane, 48,4 sur ng00). Les 37 trames, médianes des passes de
trois tours : médiane 162 ms, maximum 330 ms (le reçu publie 160,6 et 327,3 ms pour le second tour). Après T2-d-B, G
vaut environ 77 ms (médiane) et 155 ms (maximum), le mur environ 158 et 308 ms.

**Le second goulot est déjà visible.** Sur la trame maximale, G5 finit à 108,5 ms mais le noyau de l'ordre 5 à
201,0 ms et son registre à 229,6 ms (`fins_par_ordre_ns`) ; sur la médiane, 54,1 → 96,3 → 111,1 ms. Le bras séquentiel
(G puis forêt, même session) donne l'étage T seul — le noyau union-find, un propriétaire par ordre
(`forest_build.cpp:197`, `forest_kernel.cpp:47-78`), donc le noyau de l'ordre 5 — à **28,5 / 58,0 / 120,7 ms** (ng00 /
médiane / maximum), et T, M, V, R à 57,7 / 107,6 / 200,7 ms. Le coût par site du noyau croît avec la taille (0,71, 0,90,
1,22 µs) : défauts de cache de l'union-find. Quand G se raccourcit, ce noyau sort de l'ombre.

### 1.2 Le coût interne de G (après T2-d-B)

Profil à un fil sur ng00 K5 (`g4_t2db`, ms cumulés sous `lfence`, total 4 203 pour un G réel de 1 509 ms) : LEM-T1
21,0 %, trace 19,5 %, census saturé 15,2 %, sonde 14,2 %, proposition 10,4 %, census complet 7,9 %, arrêt 7,0 %,
certificat 4,4 %, pas 0,4 %. Les deux tiers sont des accès mémoire dépendants (sondes, dichotomies, parcours) ; le reste
de l'arithmétique exacte et un peu de binaire64. C'est le profil type de ce que l'appareil fait bien : beaucoup de
petites tâches indépendantes, latence cachée par le nombre.

## 2. Q1 — Structure du travail de G

**Unité indépendante : le représentant.** `resolve_part(k, F, rang de la jonction)` (`resolve.cpp:271-336`) ne lit que
le domaine immuable et n'écrit que sa cible (4 octets, `tower.hpp:48-62`) et ses compteurs. Les représentants sont les
traces strictes I ∪ A des cellules de fenêtre (`passes.cpp:46-56`) ; le produit les groupe en tranches de 256
cellules (`stage.hpp:12`), réclamées par les fils (`pipeline_run.cpp:169-176`), ordres du plus grand au plus petit
(`pipeline_run.cpp:57`), avec une file de 16 représentants préchargés (`passes.cpp:66`, `92-138`).

**Aucune dépendance entre ordres dans G** : chaque ordre a sa table de populations (`pipeline.cpp:65-75`) et lit la
table des cibles de fenêtre, remplie à l'ouverture pour tous les ordres (`internal.hpp:57-61`). Les verticales (V) lient
k à k − 1, mais dans la forêt, après G. Dans une chaîne, chaque pas dépend du précédent (≤ 12 pas mesurés).

**Lu** : catalogue (`CatalogueBall` de 32 octets : S\*, rang, p, m, q_min ; populations I puis U en CSR ; table S\* →
boule ; niveaux `num::Level` de 64 octets), index global (nœuds de 40 octets, boîte, plage, sortie ; feuilles de 8,
`index.hpp:18-22`) et coordonnées, points exacts (`stage.cpp:87-96`), tables LEM-POP (`internal.hpp:196-244`).
**Écrit** : cibles (une case par représentant, un seul écrivain), compteurs par fil fusionnés dans l'ordre des fils,
espace de census de n `SiteIdx` par fil (`census_workspace.cpp:88-96`, `stage.cpp:196-198`).

**Tailles par trame** (K5 ; compteurs déterministes identiques sur G4, relevés en local par `mhgp12_tower_probe` et le
microbanc) :

| Grandeur | ng00 | médiane | maximum |
| --- | ---: | ---: | ---: |
| sites ; nœuds de l'index | 39 885 ; 16 523 | 64 740 ; 26 055 | 99 099 ; 40 295 |
| boules de Cat5 ; incidences ; niveaux distincts | 1,31 M ; 6,10 M ; 1,09 M | 2,11 M ; 9,80 M ; 1,52 M | 3,75 M ; 17,6 M ; 2,65 M |
| cellules (ordres 1..5) | 1,31 M | 2,11 M | 3,75 M |
| représentants des ordres 2..5 | 3 419 932 | 5 477 969 | 9 998 189 |
| dont arrêtés à la première sonde | 2 572 807 (75,2 %) | 4 224 368 (77,1 %) | 7 517 088 (75,2 %) |
| survivants (chaînes) | 847 125 | 1 253 601 | 2 481 101 |
| plus petites boules (dont LEM-T1) | 1 012 161 (760 009) | 1 434 270 (1 143 913) | 2 894 094 (2 247 473) |
| census saturés + complets | 194 136 + 58 016 | 212 553 + 77 804 | 470 539 + 176 082 |
| census distincts (même boule, même seuil) | 177 863 (70,5 %) | 211 461 (72,8 %) | 462 107 (71,5 %) |
| nœuds visités ; sites testés (par census) | 12,6 M ; 4,87 M (50) | 14,6 M ; 5,66 M (50) | 37,1 M ; 13,4 M (57) |
| sondes de table (toutes) | 3,78 M | 5,91 M | 10,95 M |
| voie du census | 100 % native | 100 % native | 100 % native |

L'ordre 5 porte 40 % des représentants et 77 % des census. Par ordre, sur la trame maximale :

| k | cellules | naissances | représentants | arrêts à la 1re sonde | LEM-T1 | census (saturés + complets) | nœuds visités |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 1 | 265 512 | 99 099 | 532 031 | (cibles directes) | — | — | — |
| 2 | 465 693 | 264 512 | 1 152 553 | 984 193 | 170 490 | 4 337 + 0 | 0,19 M |
| 3 | 712 240 | 464 633 | 1 945 471 | 1 520 512 | 443 361 | 26 284 + 6 | 1,24 M |
| 4 | 992 520 | 711 077 | 2 893 429 | 2 146 917 | 747 948 | 118 371 + 1 250 | 6,10 M |
| 5 | 1 317 221 | 991 311 | 4 006 736 | 2 865 466 | 885 674 | 321 547 + 174 826 | 29,6 M |

## 3. Q2 — Ce qui se porte, ce qui bloque

| Poste | Code | Sur l'appareil | État de la preuve |
| --- | --- | --- | --- |
| trace et première sonde LEM-POP | `passes.cpp:92-138`, `populations.cpp:207-238` | direct : tableaux, splitmix64, répertoire, dichotomie | **identité hôte** (MES-G-APP « sondes ») ; temps appareil en attente |
| proposition DWelzl | `resolve.cpp:91-105`, `proposal.hpp` | copie textuelle annotée HD ; récursion bornée (`proposal.hpp:144`, `161`) acceptée par nvcc sous `-Werror=all-warnings` (72 registres, 608 o de pile) | **bits identiques hôte** (2,48 M parties sur le maximum) ; appareil en attente |
| census gardé à témoins | `census_workspace.cpp:59-84`, `guard.cpp:70-198` | parcours préfixe **sans pile** (pointeurs de sortie) : un fil par requête ; garde et puissance en `i128` (voies native et certifiée) | **identité hôte** : genre, p, m, I, U et neuf compteurs du travail sur 1,19 M requêtes ; 96 registres |
| LEM-T1 (`find_support`, `part_in_population`) | `resolve.cpp:56-66`, `230-237` | direct : CSR et dichotomies, mêmes tableaux que les sondes | archétype « sondes » |
| certificat exact, support canonique | `resolve.cpp:134-153`, `guard.cpp:37-68`, `supports.cpp` | à porter au palier étroit (`i128`) ; le support canonique est l'algorithme de la feuille, **déjà HD** (`leaf_census.hpp:50-114`) | non mesuré (4,4 % du profil) |
| census complet : canonique et contrôle croisé | `resolve.cpp:199-225` | idem | non mesuré |
| contrôles de décroissance | `resolve.cpp:26-43`, `level.hpp:54-72` | rangs `u32` directs ; niveau exact : produits croisés jusqu'à 512 bits → `Wide<N>` (`wide.hpp`, `constexpr`) à rendre HD | à porter |
| ouverture : cellules et fenêtres | `stage.cpp:119-160`, `internal.hpp:120-158` | par boule ; coquilles étendues (plafonds de 4 096 combinaisons) → hôte | 10 ms CPU (maximum) |
| tables LEM-POP | `populations.cpp`, `pipeline.cpp:65-75` | tri par base (celui de la fin d'étage, `finish_sort.hpp`) | 20,7 ms CPU (maximum), peu parallèle |

**Ce qui ne bloque pas.** `i128` : l'appareil le fait déjà dans le produit (feuille du catalogue, politique `Narrow`,
`leaf_arith.hpp:77-81`) ; nvcc 12.9 compile multiplication et division `__int128` pour sm_120 (essai local). Récursion :
seule DWelzl en a, bornée, acceptée ; le census est sans pile. Allocation par requête : aucune dans le produit
(espace préalloué) ; sur l'appareil, tableaux bornés par fil (I ≤ 12, U ≤ 64 : au-delà le produit refuse
`shell_capacity`, l'appareil signale et l'hôte rejoue). Déterminisme : cibles et compteurs sont des fonctions pures par
représentant, sommes et maxima indépendants de l'ordre, donc identiques à 1, à 48 fils et sur l'appareil — le
microbanc contrôle les neuf compteurs de chaque census.

**Ce qui demande un soin déclaré.**
1. *Largeurs U192–U320, `Big`* : seulement sur les voies contrôlée et large (étendue > 16) et pour les niveaux de plus de
   127 bits. Sur LiDAR u21 : 0 requête non native sur les trois trames ; aux profils 24 et 32, le repli exact se fait
   sur l'hôte (contrat R7 du catalogue : « non résolu », rejoué en exact avant admission).
2. *Binaire64 et compteurs* : la proposition ne décide rien, mais la route de chaque plus petite boule (LEM-T1,
   certificat, repli) en dépend, donc les compteurs du travail, qui doivent être égaux entre l'hôte et l'appareil
   (`CONTRAT_TOUR.md` § 8). Il faut `-fmad=false` (déjà dans le produit, `CMakeLists.txt:114`) et un hôte sans FMA
   (`MHGP12_MARCH` vide ; si un jour x86-64-v3, ajouter `-ffp-contract=off` aux sources HD). Débit FP64 des GPU GB202 :
   1/64 du FP32 (donnée constructeur) ; c'est le poste « propositions » du banc.
3. *Divergence* : 75 % des représentants s'arrêtent à la première sonde, les autres font 1 à 12 pas. Un noyau
   monolithique « un fil par représentant » gaspillerait le warp ; il faut une organisation **par fronts** (§ 4).
4. *Refus et budget* : `Result<>` et `std::optional` ne vont pas sur l'appareil ; codes de raison (`u16`) et réduction
   déterministe (plus petit ordre, puis raison : l'ordre de `merge`, `status.hpp`) par `atomicMin` sur une clé
   empaquetée ; mémoire de l'appareil réservée dans le budget de la Session avant `cudaMalloc`, comme
   `CudaExecutor::grow` (`device_cuda.cu:102`), refus `memory_budget` avant tout calcul.
5. *Une seule implantation* (règle 2 d'`ARCHITECTURE.md`) : la source HD devient **la** résolution, jouée par deux
   exécuteurs (Pool et CUDA), comme la feuille J3 ; l'actuel G sur l'hôte disparaît au profit de cette source.

## 4. Q3 — Architecture proposée

**Point de départ favorable : le catalogue est déjà sur l'appareil, en ordre canonique.** La fin d'étage garde
`balls`, `offset`/`values` (CSR des populations), `table`/`table_offset` (S\* → boule) et `level_of_rank`
(`finish_driver.hpp:25-35`), puis les recopie vers l'hôte (`finish_driver.hpp:204-213`) ; les coordonnées sont
résidentes (`device_pipeline.hpp:225-228`). G sur l'appareil les lit sur place. Seul l'index global (1,6 Mo au
maximum) est à téléverser ou à construire.

**G1 — ouverture sur l'appareil** (gain immédiat, utile même si la résolution reste sur l'hôte) : cellules (comptage et
remplissage par blocs de boules, coquilles étendues sur l'hôte), fenêtres, tables LEM-POP par tri par base. C'est la
« table de populations produite par la fin d'étage du catalogue, résidente » déjà prévue (`ARCHITECTURE.md` § 4.2).
Retire brut 13 à 20 ms du chemin critique (médiane, maximum) ; si la résolution reste sur l'hôte, il faut y rapatrier
cellules, masques, fenêtres et tables (environ 300 Mo au maximum, 5 à 6 ms en mémoire épinglée à 56 Go/s, MES-M6) :
gain net de 8 à 15 ms.

**G2 — résolution sur l'appareil, par fronts, ordre 5 d'abord** :
1. *front des premières sondes* : un fil par cellule ; une réussite écrit la cible ; un échec écrit le représentant
   dans la file des survivants (sommes préfixes, ordre conservé) ;
2. tant que la file n'est pas vide (≤ 12 tours) : *propositions* (un fil par survivant) → *LEM-T1* → *certificats* →
   *census* (un fil par requête, parcours sans pile ; si la divergence coûte trop, variante « un warp par requête »
   qui teste les huit sites d'une feuille et les frères en parallèle, compteurs recalculés comme en séquentiel, comme
   le census de la feuille J3 ; dédoublonnage facultatif par tri sur (S, k) : 28 à 30 % de doublons) → *pas*
   (saut, pas inerte, arrêt, contrôles de rang ou de niveau) → *sondes après pas* → compactage. Chaque noyau est
   uniforme, donc efficace ; la boucle de tours vit sur l'appareil (graphe CUDA ou noyau persistant) pour éviter les
   allers-retours à 8 µs (MES-M6) ;
3. *non résolus* (voies non natives, coquilles > 64, cellules étendues, repli exact) : rejoués sur l'hôte par la même
   source (exécuteur Pool) avant de publier l'ordre ;
4. *cibles* rapatriées par ordre (4 octets par représentant : 16 Mo pour l'ordre 5 du maximum, 0,3 ms épinglé), pour
   que le noyau union-find de l'ordre 5 démarre quelques millisecondes après C au lieu de 35 à 55 ms après.

**Compteurs et refus** : accumulateurs par bloc, puis `atomicAdd`/`atomicMax` sur `u64` (sommes commutatives :
déterministes) ; issue par `atomicMin` (§ 3). **Mémoire de l'appareil par site** (K5, trame maximale) : en plus des
tableaux du catalogue déjà résidents (environ 4 Ko par site), cellules 64 Mo, masques 84, cibles 42, fenêtres 52, tables
LEM-POP 107, files de survivants 160, census en vol 50 : **environ 560 Mo, 5,7 Ko par site**. Les scènes de plusieurs
millions de sites (régime b) dépassent la carte ; mais les ordres sont indépendants dans G : un ordre à la fois borne le
pic au tiers environ, et la voie Pool de la même source sert le reste. **Recouvrement** : avec C, aucun dans une trame (G
lit le catalogue complet) ; entre deux trames (cadence), oui, sur deux flux ; avec la forêt, oui, par l'ordre 5 d'abord.

## 5. Q4 — Gain estimé et microbanc

### 5.1 Estimation (à calibrer par la session)

Débits attendus des archétypes sur la trame maximale (raisonnement de débit, pas une mesure) : census, 37 M visites
de nœuds à environ 1,5 puissance `i128` chacune (environ 3,5 G multiplications entières de 32 bits) sur 188 SM : 0,1 à
0,3 ms, contre environ 28 ms de mur-équivalent à 48 fils (23 % de 5 973 ms-fils) ; sondes, 10 M représentants à
environ 4 accès aléatoires : 0,3 à 0,5 ms contre environ 42 ms ; propositions, 2,9 M DWelzl à quelques centaines
d'opérations FP64 : 0,5 à 2 ms contre environ 13 ms. Rapports attendus : census ≤ 0,03, sondes ≤ 0,05, propositions
≤ 0,2.

Région de G sur l'appareil ≈ région sur l'hôte × Σ (part du poste × rapport) × pénalité d'intégration. Avec les parts du
§ 1.2 (sondes, trace, T1 et arrêt traités comme les sondes ; certificat comme le census) et une pénalité de 2 à 4
(fronts, compactages, lancements, census dans un noyau plus gros) : facteur 0,1 à 0,25. D'où :

| | médiane | maximum |
| --- | ---: | ---: |
| G aujourd'hui (après T2-d-B) | ≈ 77 ms | ≈ 155 ms |
| G sur l'appareil : ouverture + résolution (région de 57 / 127 ms × 0,1 à 0,25) + cibles | 2-4 + 6-14 + 1 ≈ **9-19 ms** | 4-6 + 13-32 + 2 ≈ **19-40 ms** |
| mur, G seul sur l'appareil (T inchangé) | 158 → **≈ 120-125 ms** (−21 à −25 %) | 308 → **≈ 230-240 ms** (−22 à −25 %) |
| mur, G sur l'appareil **et** noyau T5 parallélisé par 4 | **≈ 80-90 ms** | ≈ 150 ms (C = 77,8 ms domine) |

Le mur « G seul » vaut P + C + (G5 sur l'appareil et ses cibles, environ 5 à 8 ms) + noyau T5 (50 à 58 / 110 à 121 ms)
+ queue de l'ordre 5 (M, V, R : 15 / 29 ms, `fins_par_ordre_ns`) ; G des autres ordres et leur forêt finissent avant.

**Conclusion** : G sur l'appareil est la plus grosse réduction disponible (−60 à −120 ms de G), mais elle ne se
convertit en mur que si T suit. Pour la médiane, il faut les deux ; pour le maximum, il faut aussi raccourcir C.

### 5.2 Le microbanc MES-G-APP (livré, compilable et joué en local)

Dossier `mes_g_appareil/` (à copier dans `morsehgp3D_v12/microbancs/`) : `noyau_g.hpp` (source unique HD : census
gardé à témoins, sondes LEM-POP, DWelzl annoté), `appareil.cu`/`.hpp` (exécuteur CUDA, un flux, durées des noyaux par
événements, copies chronométrées à part), `mes_g_app.cpp` (programme hôte lié à `libmhgp12.a`), `pilote_g_appareil.py`
(construction, campagne, juge ; Python 3.10 nu, sans `assert`, jouable sous `-S -O`, auto-test du juge),
`test_pilote_g_appareil.py` (porte locale du juge, 9 cas), `README.md`.

**Aucune copie du résolveur** : les requêtes de census sont **interceptées dans le produit** à l'édition de liens
(`-Wl,--wrap` sur `CensusWorkspace::query`, symbole vérifié par `nm` : défini par `census_workspace.cpp.o`, appelé par
`resolve.cpp.o`) pendant un vrai `resolve_tower` ; la récolte est contrôlée contre les compteurs de G (nombre de
census, `census_sites`, `census_nodes`). Les sondes rejouent la voie G-L7 du produit (`PopulationTable::find`) ; les
propositions comparent la copie HD au `DWelzl` du produit. Postes : census (toutes les requêtes), sondes (tous les
représentants), propositions (traces dont la première sonde échoue). Bras hôte : le produit à 48 fils ; bras
appareil : le noyau seul, données résidentes ; chaque mesure deux fois par passe (A/A), ordre hôte/appareil alterné ;
mutant causal « côté nul » (copie compilée à part) qui doit tomber en code 1.

**Joué en local (sans GPU)** : identité complète du noyau HD sur l'hôte sur les trois trames (0 écart sur 252 152,
290 357 et 646 621 census ; sondes égales aux `first_probe_hits` de G ; 847 125, 1 253 601 et 2 481 101 propositions
aux bits identiques) ; mutant tué (232 162 écarts, code 1) ; noyaux CUDA compilés pour sm_120 (`-fmad=false`,
`-Werror=all-warnings`) ; refus propre sans appareil (code 2) ; pilote complet en 1 min 40 s (verdict « refusé » attendu :
pas d'appareil). Information locale non décisive : le census HD « à plat » sur l'hôte coûte **0,50 à 0,67 fois** le
census du produit (`Result`, registres, `Point::make`, verrou par requête) — à confirmer sur G4.

**Plan de session** `plan_g_appareil.json` (format `ehgp.v12.session_plan.v1`, validé hors ligne par les fonctions du
contrôleur) : construction par défaut limitée à la cible `mhgp12` ; `g_app_autotest` (60 s) ; `g_app_pilote` (900 s) :
3 trames × 5 processus neufs × (1 échauffement + 5 passes), puis le mutant ; plafond total 24 min, durée attendue 6 à
10 min. Données : 6 fichiers, **3,26 Mo** (`lidar_ng00`, `kitti_ng_02_001606`, `kitti_ng_08_002119`, `.u32le` et
`.ids.u32le` ; empreintes dans `donnees_session.sha256`), aucune n'entre dans le dépôt.

**Règle écrite d'avance** (`REGLE_G_APPAREIL`, 8 octobre 2026 à 10:04 UTC, en tête du pilote) : par processus, rapport
= médiane des noyaux de l'appareil / médiane du lot du produit à 48 fils ; moyenne géométrique sur les processus,
IC 95 % par bootstrap (10 000 tirages, graine 20261008). **Adopté** (la poursuite est fondée) si l'identité est
complète partout et si la borne haute est ≤ 0,20 (census), ≤ 0,20 (sondes), ≤ 0,50 (propositions) sur chacune des
trois trames ; **rejeté** sur un écart d'identité ou un seuil manqué ; **refusé** si une preuve manque (prise, récolte,
isolation du GPU, mutant vivant, binaire changé, A/A hors [0,90 ; 1,10], plus de 0,1 % de requêtes non résolues).
Publiés sans verdict : temps absolus, transferts, rapport du census HD sur l'hôte au census du produit, doublons,
propriétés de l'appareil (dont la taille de L2, non relevée par MES-M6).

**Pour lancer** (à faire par l'utilisateur) : copier `mes_g_appareil/` dans `morsehgp3D_v12/microbancs/` et pousser
(ou `--snapshot` pour un essai) ; assembler les données hors de `/tmp` :

```bash
mkdir -p build/v12-data-20261007/g4data_g_app && cd build/v12-data-20261007/g4data_g_app
cp ../g4data_m/lidar_ng00.u32le ../g4data_m/lidar_ng00.ids.u32le .
tar xf ../g4_kitti_v12set_xyz.tar kitti_ng_02_001606.u32le kitti_ng_02_001606.ids.u32le \
    kitti_ng_08_002119.u32le kitti_ng_08_002119.ids.u32le
sha256sum -c <dossier d'étude>/donnees_session.sha256
```

puis la session gardée habituelle avec `plan_g_appareil.json` copié dans `build/v12-data-20261007/plans/`
(`--max-run-seconds` d'environ 2 400 suffit).

## 6. Q5 — Risques, alternatives CPU, déclarations

**Risques.** (1) La pénalité d'intégration (fronts, compactages, tours, census dans un noyau plus gros) n'est pas
mesurée par le microbanc, qui donne une borne optimiste par poste ; seul un prototype de G2 la mesure. (2) Le gain sur
le mur est plafonné par T (§ 5.1) : sans T parallèle, environ −25 %. (3) Coût de portage comparable à T1-b : 1 500 à
2 500 lignes HD (boucle de résolution, certificat q2/q3/q4 au palier étroit, canonique, census complet, comparaison de
niveaux multi-mots, cellules, tables), un exécuteur, des portes (oracle, W1/W48, hôte/appareil, `MES-M0`, mutants).
(4) GCC 11.4 de la VM n'a pas compilé le microbanc (seul GCC 13 local l'a fait) : un avertissement nouveau sous
`-Werror` ferait refuser la session à la construction (5 min perdues, aucun coût GCP au-delà). (5) Les copies du
microbanc ne sont pas épinglées (empreintes des sources portées publiées seulement) : un écart du produit se verrait en
écart d'identité ; l'interception suppose un appel non inliné (pas d'IPO/LTO dans le produit) — sinon la récolte se
refuse d'elle-même (aucun census récolté, contre les compteurs de G).
(6) Le lot hôte du census, trié et sans le reste de G, est plus favorable au CPU que le census en place : le rapport
mesuré est conservateur.

**Ce qui resterait à gagner sans GPU** (ordres de grandeur, région de G de la médiane = 62 ms) : census « à plat » (même
source HD jouée par le Pool : 0,5 à 0,67 du census actuel en local, soit −8 à −11 % de G) ; mémo des census (28 à 30 %
de doublons : −6 %) ; recouvrement mémoire par lots pour LEM-T1 et le census, comme G-L7 pour les sondes (−15 à −25 %,
non mesuré) ; G-L4 (census complets évités, 77 k à 175 k à l'ordre 5) ; ouverture et tables produites par la fin
d'étage du catalogue (−8 à −15 ms nets de chemin critique, § 4 G1). Total plausible : G de 77 à 40-50 ms sur la
médiane, de 155 à 85-100 ms sur le maximum. Avec un T parallèle, la médiane resterait vers 100-115 ms sans GPU dans G,
le maximum loin au-dessus.

**À déclarer si l'on y va** (`docs/DECISIONS.md`, même commit que `README.md`, `AGENTS.md` et `CLAUDE.md`, § 3 du
fichier) : une décision D16 « étage G dans le chemin contractuel de l'appareil » — backend de G `cpu_reference` →
`cuda_g4` ; cadre `backend=cpu_reference ; cuda_g4 pour le catalogue et l'étage G` ; la voie CPU complète reste la
référence exacte, **même source HD** jouée par le Pool (petits nuages sous le seuil de `MES-P`, scènes de plusieurs
millions de sites, rejeu des non résolus) ; changement d'exécuteur et d'organisation (fronts), sans changement d'objet
ni de compteurs du travail (fonction pure par représentant, sommes) ; identité hôte/appareil exigée, y compris les bits
de la proposition (`-fmad=false`, hôte sans FMA) ; budget de l'appareil étendu à G. Contrats à mettre à jour :
`CONTRAT_TOUR.md` § 3 (ligne G), § 4.4 (parallélisme), § 7 (mémoire de l'appareil), § 9 (porte hôte/appareil) ;
`ARCHITECTURE.md` § 3 (budgets), § 4.2, § 5 (module `tower` : voie appareil, tableaux résidents du catalogue) ; `PLAN.md`
(tranche T2-e, entrée : MES-G-APP adopté ; sortie : `MES-M0` identique, budget de G sur G4) ; `MESURE.md` (MES-G-APP).

## 7. Recommandation

1. **Jouer MES-G-APP** (session courte, règle déjà écrite). Elle tranche la faisabilité matérielle (débits, FP64, bits
   identiques sur l'appareil) pour un coût minime.
2. **S'il est adopté, ouvrir T2-e en deux temps** : G1 (ouverture et tables sur l'appareil, gain immédiat, peu de
   risque) puis G2 (résolution par fronts, source unique, exécuteurs Pool et CUDA). **L'apparier** à un chantier T
   parallèle (`LEM-MSTC` ou A6 à l'intérieur de l'ordre 5), sans lequel G2 ne rend qu'environ 25 % de mur.
3. **Dans tous les cas**, prendre les gains CPU sûrs : census « à plat » (à confirmer par l'information de la session)
   et tables de populations produites par la fin d'étage du catalogue.

## Annexe — fichiers de l'étude et reproduction locale

Dossier : `/tmp/claude-1000/-workspaces-E-HGP/6acaaf62-b44a-41e7-b5d6-81e4c7b6b7f1/scratchpad/v12_g_appareil/`
(copie de sécurité des sources, du plan et des mesures dans `~/v12_sauvegarde/etude_g_appareil/`, `/tmp` étant vidé
aux redémarrages du codespace).

| Fichier | Rôle | SHA-256 |
| --- | --- | --- |
| `mes_g_appareil/noyau_g.hpp` | noyaux en source unique | `c5c45d59…` |
| `mes_g_appareil/appareil.cu`, `appareil.hpp` | exécuteur CUDA | `09593456…`, `118e7692…` |
| `mes_g_appareil/mes_g_app.cpp` | récolte, lots hôte, identité | `cbef723c…` |
| `mes_g_appareil/pilote_g_appareil.py` | pilote, juge, règle | `291c7f3a…` |
| `mes_g_appareil/test_pilote_g_appareil.py` | porte locale du juge (9 cas, `python3 -S -O`) | `48da31d9…` |
| `mes_g_appareil/README.md` | mode d'emploi | `90c41f55…` |
| `plan_g_appareil.json` | plan de session | `181f62e9…` |
| `donnees_session.sha256`, `donnees_session/` | empreintes et copie des 6 trames (hors dépôt) | — |
| `mesures_locales/` | compteurs locaux (sondes de G, microbanc) | — |
| `essai_pilote/sortie/` | essai local complet du pilote (10:25-10:27 UTC) | — |
| `outils/` | scripts d'extraction des reçus et sonde locale de la forêt par ordre | — |

Reproduction locale (construction hors de `/workspaces`) :

```bash
git archive HEAD morsehgp3D_v12 | tar -x -C <dossier>/src
cmake -S <dossier>/src/morsehgp3D_v12 -B <dossier>/build_cpu -DCMAKE_BUILD_TYPE=Release
cmake --build <dossier>/build_cpu -j8 --target mhgp12 mhgp12_tower_probe
python3 -S -O mes_g_appareil/pilote_g_appareil.py tout --src <dossier>/src --produit <dossier>/build_cpu \
    --travail <t> --donnees <trames> --sortie <s> --fils 8 --processus 1 --passes 2 --sans-appareil
```
