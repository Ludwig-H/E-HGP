# Mesure : régimes, données, chiffres de référence, empreintes, protocole

7 octobre 2026. Sources : audit géant de la v11 (§ 5, § 7.8, § 7.10) et rapports bruts H et G
(`../../morsehgp3D_v11/receipts/audit_geant_v11_20261007/`). **Seule G4 juge les temps** ; le local ne prédit pas G4,
et le nombre d'instructions ne prédit pas le temps (V3 : instructions ×0,565, temps ×0,925 ; pages de 2 Mio : −17 % en
local, +6,4 % sur G4).

## 1. Régimes

| Régime | Définition | Usage |
| --- | --- | --- |
| à froid | un processus neuf par prise ; ouverture du contexte CUDA comprise | publié à côté du contrat |
| à chaud | une Session résidente qui enchaîne des trames ; médiane des passes 2 à 10 d'un processus, **plusieurs processus** | contrat proposé (décision D1) |
| latence | durée d'une trame, de l'entrée à la tour en mémoire | contrat proposé (décision D2) |
| cadence | trames par seconde avec recouvrement de deux trames | repli déclaré |

Toujours publier : CPU ou GPU, nombre de fils, profil, K, étages (`domain`, forêts, écriture, vues), pic mémoire hôte
et appareil, CPU·s par trame.

## 2. Données

| Jeu | Contenu | Rôle |
| --- | --- | --- |
| ng00, ng01, ng02 | SemanticKITTI 08/000000, 000100, 000200 sans sol (Patchwork++ épinglé), grille 1 mm : 39 885, 35 551, 45 845 sites | contrat historique et différentiel ; hors dépôt (`build/v11-full-data-20261002/`) |
| uniformes | 8 000, 16 000, 32 000 sites, coordonnées sur 18 bits (« u18 » désigne la **donnée**, jouée au profil u21) | tailles d'intérêt de `CLAUDE.md` ; oracle d'échelle |
| trames de plusieurs séquences | à préparer (00–10), sans sol, toutes tailles | **base de toute décision de vitesse** (consigne du 6 octobre) |
| scènes de plusieurs millions de sites | LiDAR réel, mobile, terrestre ou aérien, téléchargeable pour la recherche ; jeux retenus dans `DONNEES.md` (recensement en cours) ; tailles 1, 2, 4 et 8 millions par découpe spatiale (jamais par sous-échantillonnage), puis la scène entière ; avec et sans sol | régime (b) de la décision D7 |
| petits nuages | 100, 300, 1 000, 3 000 et 10 000 sites : familles synthétiques et découpes de trames LiDAR | régime (c) de la décision D7 ; oracle borné en dessous de 14 sites |

**Tailles réelles.** Sur les 132 trames sans sol distinctes de la séquence 08 du reçu `pts4_review_20261003` de la v11,
89 dépassent 60 000 sites, aucune n'est sous 30 000 ; médiane 68 049, maximum 126 267. Les trois trames du contrat
sont parmi les plus petites. La plage « 30 000 à 60 000 sites » est à redéfinir (décision D7).

**Licence.** Aucune donnée ni coordonnée SemanticKITTI dans le dépôt (CC BY-NC-SA) : ni trames, ni vidages, ni sorties
de CLI. Seuls des empreintes, des comptes et des manifestes.

**Objectifs des régimes (b) et (c)** (hypothèses du 7 octobre, à confirmer ou réviser après les premières mesures de
la v11 gelée sur ces données, `MES-E` et `MES-P` de [`PLAN.md`](PLAN.md)) :

| Régime | Mesures publiées | Objectif proposé |
| --- | --- | --- |
| (b) plusieurs millions | mur, CPU·s, pic de mémoire hôte et appareil, exposant d'échelle sur 1, 2, 4, 8 millions, à K5 et K10, à froid et à chaud | exposant au plus 1,1 ; K5 au plus 2 s par million de sites (le contrat principal, 100 ms pour 60 000 sites, en vaut 1,7) ; K10 au plus 10 s par million ; aucune scène refusée sous 10 millions de sites à K5 |
| (c) petits nuages | latence à chaud et à froid, voie CPU et voie GPU | coût fixe à chaud au plus 2 ms ; coût par site jamais supérieur à celui du régime principal |

## 3. Chiffres de référence

### 3.1 Temps (G4, 48 fils, à chaud, ng00 / ng01 / ng02)

| Mesure | K5 | K10 |
| --- | --- | --- |
| v11 voie GPU (`868347:400` à K5, `868347` à K10, feuilles 24) : mur | 251 / 212 / 255 ms | 1 782 / 1 336 / 1 536 ms |
| v11 voie GPU : `domain` / forêts | 138 / 114, 120 / 91, 141 / 114 ms | 489 / 1 293, 398 / 939, 471 / 1 065 ms |
| v11 voie CPU (`802811`, feuilles 16) : mur | 314 / 255 / 313 ms | — |
| v11 voie CPU : `domain` / forêts | 200 / 113, 163 / 92, 195 / 116 ms | — |
| v11 à froid, voie GPU / voie CPU | 335 / 301 / 345 ; 343 / 272 / 329 ms | 1 824 / 1 395 / 1 602 ms (GPU) |
| v10 (`777406b82`, CPU, une passe chaude, u18) : mur | 252 / 204 / 254 ms | 1 125 / 861 / 1 024 ms |
| v10 : catalogue / tour | 164 / 89, 137 / 67, 164 / 89 ms | 653 / 472, 528 / 334, 618 / 406 ms |
| meilleur mur v11 jamais mesuré | 196,8 ms (ng01, voie GPU, réglage glibc `@tas`) | — |

Source : session `claudeg1` (reçu `developpement_20261007/filtre_g1_avx2/`) ; référence v10 :
`audit_deep_20261004/performance/context/TABLE_VERITE_G4.md`. « À froid » est la médiane haute de six processus (trois
à K10). La comparaison v10/v11 est descriptive (sessions et régimes distincts).

### 3.2 Répartition du `domain` de la v11 (ng00, K5, une passe chaude)

| Sous-étage | Voie GPU | Voie CPU |
| --- | ---: | ---: |
| frontière | 20,7 ms | 19,9 ms |
| passe unique (parcours ; et feuilles en voie CPU) | 37,5 ms | 152,1 ms |
| lot de feuilles (GPU + hôte) | 51,8 ms | — |
| tri / balayage / assemblage / compactage | 10,0 / 4,4 / 3,4 / 3,2 ms | 10,0 / 4,4 / 3,3 / 2,8 ms |
| frontière + fin, hors parcours et feuilles | **48,6 ms** | **46,9 ms** |

### 3.3 Volumes de travail (ng00)

| Grandeur | K5 | K10 |
| --- | --- | --- |
| boules du catalogue (par site) | 1 306 696 (32,8) | 5 512 670 (138,2) |
| incidences (par boule) | 6 097 121 (4,67) | 45 383 538 (8,23) |
| feuilles | 353 456 (feuilles 16) ; 123 581 (feuilles 24) | 530 259 (feuilles 24) |
| tests du filtre G1 | 379 M (feuilles 16) | 1 229 M (feuilles 24) |
| naissances ; nœuds des ordres 1..K | 897 776 ; 1 541 750 | — |
| pas de descente (dont succès de table) | 4,80 M (3,62 M) | ordre 10 seul : 6,18 M |
| plus petites boules ; présentations de supports | 1,175 M ; 3,79 M | ordre 10 seul : 2,35 M ; 174,4 M (74,2 par boule) |
| census | 291 515 | ordre 10 seul : 1,16 M |
| CPU·s par FULL (voie GPU / voie CPU) | 9,36 / 12,17 | 52,3 (voie GPU) |
| mémoire de l'appareil | 243 Mo | 1,42 Go |

Les volumes sont fixés par l'objet : la v10 et la v11 font le même travail logique à quelques pour cent près. Sur nuages
uniformes 3D, 75 à 79 boules par site à K5 : la géométrie surfacique du LiDAR divise le volume par 2,4.

## 4. Empreintes de référence (différentiel de la v12)

SHA-256 des vidages `MHGP11FUL1` de la sonde `mhgp11_full_bench` de la v11 (moteur `ac081a06f`, profil u21), **reproduits
à l'octet** pour l'audit du 7 octobre (voie CPU, K10 compris) :

| Entrée | K | SHA-256 |
| --- | --- | --- |
| ng00 | 5 | `3a2bfb4f9f48b4b0cc5b0318d9fcf4887906638e034b2c97dda1918e3170a6fe` |
| ng01 | 5 | `5212a2ced81bf69bd2abfb935a3b14a35158df25339285a0d25a9d5d5c09e091` |
| ng02 | 5 | `78feb765e21c8e4582762a25bd0dd36a455747d3f0df80523e19f7ba4be0e207` |
| ng00 | 10 | `61a4245b91d9a4fdad012f0a2e26a63c3e48db1d46a180db756f2c4e4aa77295` |
| ng01 | 10 | `838a447e0b92e13e42f7f69a84fd536d5d46d26463d3f4cc7c688475878fe0de` |
| ng02 | 10 | `81f89995eaccb497cfe92abb81213bebd0d4ce5076210570d86aa9456cf7ff2e` |
| uniforme 8 000 | 5 | `f87dbb19dd928311fcb65d5a98d30b4fb50eddf7de1d26708bbc278f46dcc7cf` |
| uniforme 16 000 | 5 | `141bc7d6523154cff1dd22b288e4155259e3d97a82877205887642bad8286889` |
| uniforme 32 000 | 5 | `a7563907a5c9f1b273776b811d8a559eb80713eb161460edd948664f5433811b` |

Commande : `mhgp11_full_bench <cas>.u32le <cas>.ids.u32le <vidage> <K> <feuille> 256 0 4294967295 8589934592 <fils> <masque>`
(feuilles 16 à K5, 24 à K10 ; le résultat ne dépend ni de la voie ni de la taille de feuille). L'empreinte **sémantique**
(lecteur strict `bench/full_semantic.py`) est identique aux profils u18, u21 et u24 : c'est elle que la v12 doit
reproduire si son format change. Signature `tree_k_sha256` de ng00 K5 : `a4425cb5cedd255d…` (rapport G).

## 5. Protocole statistique

Tiré des données brutes des bancs de la v11 (rapport H). Écarts-types des log-rapports appariés par prise, prise
d'échauffement exclue :

| Métrique | voie CPU | voie GPU |
| --- | ---: | ---: |
| mur | 0,070 | 0,037 |
| `domain` | 0,096 | 0,034 |
| forêts | 0,082 | 0,079 |
| frontière | 0,044 | 0,051 |
| cellules du publieur de l'ordre 5 | 0,095 | 0,096 |

Nombre total de paires nécessaires (trois trames) pour un effet minimal détectable donné (bilatéral 5 %, puissance 80 %) :

| Métrique | 2 % | 3 % | 5 % |
| --- | ---: | ---: | ---: |
| mur CPU / GPU | 96 / 26 | 42 / 12 | 15 / 4 |
| `domain` CPU / GPU | 176 / 23 | 78 / 10 | 28 / 4 |
| forêts | 122–129 | 54–57 | 19–20 |
| phases du publieur | 172–277 | 76–122 | 27–43 |

**Règles proposées.**
1. Unité de réplication : le **processus**, à froid comme à chaud ; premier processus GPU jeté (environ 300 ms
   d'initialisation contre environ 70 ensuite) ; mode persistance du GPU activé et relevé au reçu.
2. À froid, K5 : 20 processus par bras, par trame et par voie, en ordre entrelacé (carré de Williams) ; environ cinq
   minutes de G4. À K10 : 10 processus.
3. À chaud : au moins 5 processus par bras et par trame, 10 passes chacun, ordre entrelacé (la variance entre processus
   vaut 10 à 27 fois celle des passes).
4. Statistique : moyenne ou estimateur de Hodges–Lehmann des log-rapports appariés ; intervalle par bootstrap stratifié
   ou permutation par inversion de signe ; rapport aussi par cellule (trame, voie).
5. Adoption : borne haute de l'intervalle sous 1 sur l'étage visé, avec un nombre de paires choisi pour 80 % de
   puissance à l'effet **attendu** (3 à 10 %) ; non-infériorité de 1 % sur le mur et les autres étages ; effet de phase
   significatif comme contrôle de mécanisme ; correction de Holm si plusieurs métriques décident.
6. Un bras A/A dans chaque session, fenêtre d'environ ±1,5 % sur le mur.
7. Leviers algorithmiques jugés à un fil ou sur compteurs déterministes (bruit A/A d'environ 0,5 % à un fil).
8. Petits gains cumulables : en lot, jugés en bloc contre la base, puis ablation, sur prises neuves.
9. Un juge unique en bibliothèque, couvert par CTest, haché au lancement et inscrit au reçu ; vraie médiane ; refus des
   cellules sans paire ; modes nommés et versionnés.

**Rappel** : la v11 a retiré cinq leviers sur des seuils placés 2 à 6 fois au-delà de l'effet réel ; rejugés sur les
données, un seul l'avait été clairement à tort (filtre G1 en AVX2 : `domain` −3,6 %, mur −1,7 %).
