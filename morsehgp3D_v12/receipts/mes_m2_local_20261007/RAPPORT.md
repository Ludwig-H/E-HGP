# Rapport MES-M2 : la feuille du catalogue, data-parallèle sur un warp

7 octobre 2026, 09 h 41 à 10 h 50 UTC (heures lues par `date -u`). Microbanc hors produit de la tranche T0 de la v12.

```text
phase=exploration_v12_hors_registre
backend=cpu_reference (identité, warp simulé) ; cuda_g4 (banc compilé, à jouer)
quantification=quantized_u21_input_only
public_status=not_claimed
GCP non utilisé ; rien écrit sous /workspaces/E-HGP ; aucune commande git qui écrit
```

Sources lues : `morsehgp3D_v12/README.md`, `docs/ARCHITECTURE.md` (§ 4.1), `docs/PLAN.md` (`MES-M2`), `docs/MESURE.md`,
`docs/CONTRAT_NUMERIQUE.md` (§ 2, § 8, pour `MES-S`) ; `morsehgp3D_v11/docs/AUDIT_GEANT_V11.md` (§ 7.2, § 7.3) ; rapport C
de l'audit (§ 2, § 4) ; feuille v11 (`leaf.cpp`, `leaf_device.hpp`, `leaf_device_predicates.hpp`, `leaf_batch*.hpp/.cpp`,
`leaf_batch_cuda.cu`, `small_pair_graph.hpp`, `support.cpp`, `center_line_cache.hpp`, `boxes.cpp`, docs `CATALOGUE*.md`,
`CENTER_REGION.md`) ; prototype J3 de la v10 (`leaf.hpp`, `J3_feuille_v3.patch`) ; conception du générateur v11 (§ 4–5) ;
contrat de la feuille cohérente de l'auditeur v11 (`audit_coherent_leaf_design_20261006`). Moteur v11 épinglé :
`morsehgp3D_v11/src` à `HEAD` = `ac081a06f` (aucune différence).

## 0. Résumé

1. **Vidage** (`vidage/leaf_dump.cpp`, lié à `libmhgp11.a` u21) : neuf cas, ng00, ng01, ng02 à K5/16, K5/24, K10/24, soit
   **2 748 544 feuilles** et leur sortie de référence `leaf.cpp`. Feuilles, boules et incidences égales aux chiffres
   connus de la v11 (ng00 : 353 456 / 123 581 / 530 259 feuilles ; 1 306 696 et 5 512 670 boules ; 6 097 121 et
   45 383 538 incidences ; nœuds 783 071 et 1 137 395). Feuille v11 sur l'hôte : 0 non résolue, 0 écart avec `leaf.cpp`.
   Vidage déterministe : reconstruit depuis une bibliothèque v11 neuve, identique à l'octet.
2. **Deux formes en source unique** `__host__ __device__`, un warp par feuille : **J3 par phases** (paires, triplets avec
   table H, quadruplets par ET de trois lignes de H, census par vote) et **cohérente** (tout le warp sur un même
   préfixe). Mêmes filtres G1–G3, mêmes prédicats (port explicite), même canonisation, **mêmes quinze compteurs
   logiques que `leaf.cpp`, sans changement de contrat**.
3. **Identité sur l'hôte** (warp simulé de façon déterministe) : **les deux formes rendent exactement la sortie de
   `leaf.cpp` sur toutes les feuilles des neuf cas** (2 748 544 feuilles par forme) : 15 compteurs par feuille et
   ensemble des émissions (S*, p, m, qmin, I, U) ; **0 feuille non résolue**. La canonisation des coquilles étendues
   est exercée (135 à 1 301 boules émises à coquille étendue selon le cas).
4. **CUDA** : le banc compile pour `sm_120` (nvcc 12.9 local) par CMake 3.28 et **CMake 3.22.1** (celle de la VM) ;
   témoin v11 (`count_kernel` copié, `leaf_device.hpp` et `ScratchSink` inclus tels quels) et six variantes v12 (deux
   formes × trois bornes de registres). **Rien n'est exécuté sur GPU** (aucun GPU local) : le temps reste à mesurer.
5. **Script G4** (`scripts/g4_leaf_bench.py`, bibliothèque standard, testé sous `python3 -S -O`) : construit, vide,
   vérifie, passe Compute Sanitizer, joue 1 + 5 processus par cas, juge (trois verdicts) et écrit `report.json`.
   Essais locaux sans GPU : chaîne complète jusqu'au verdict « refusé » attendu.
6. **Prédiction écrite avant G4** : la forme J3 a 94 % de voies utiles par paquet, la forme cohérente 3 à 4 voies sur
   32 par pas ; J3 est la candidate sérieuse (rapport estimé 0,1 à 0,4 du témoin), la cohérente probablement rejetée.
7. **`MES-S`** (ajout du coordinateur) : toutes les feuilles ont une étendue **s ≤ 17** (au plus 13 feuilles sur
   430 579 à s = 17, aucune au-delà) ; tous les supports **s ≤ 15** ; aucune part au-delà de 17, 19, 20 ou 24 ;
   minorité (le site le plus lointain fixe au moins deux bits) : **0,002 à 0,008 %** des feuilles.

## 1. Livrable

Dossier `/tmp/claude-1000/-workspaces-E-HGP/6acaaf62-b44a-41e7-b5d6-81e4c7b6b7f1/scratchpad/v12_feuille/` :

| Chemin | Rôle |
| --- | --- |
| `README.md` | conception : vidage, sémantique par ensembles et preuve des compteurs, discipline du warp, deux formes, census, banc, règle et juge, session G4, limites |
| `RAPPORT.md` | ce rapport |
| `CMakeLists.txt` | CMake ≥ 3.20 ; C++20 ; `-Wall -Wextra -Wpedantic -Werror` côté hôte ; dialecte CUDA20 déclaré comme la v11 |
| `include/mhgp12/leaf/simt.hpp` | warp en source unique (appareil réel ; warp simulé déterministe sur l'hôte) |
| `include/mhgp12/leaf/predicates.hpp` | prédicats exacts (port de `leaf_device_predicates.hpp`, sha256 `c638996b…`) |
| `include/mhgp12/leaf/leaf_common.hpp` | préparation, census par vote, support canonique (hors ligne), compteurs |
| `include/mhgp12/leaf/leaf_j3.hpp`, `leaf_coherent.hpp` | les deux formes |
| `include/mhgp12/leaf/dump_format.hpp`, `arena.hpp`, `compare.hpp` | format `MHGP12LF` v1, arène, vérification |
| `vidage/leaf_dump.cpp`, `vidage/leaf_timing.cpp` | vidage ; chrono local indicatif (liés à la v11) |
| `host/leaf_identity.cpp`, `host/arena_selftest.cpp`, `host/mes_s.cpp` | identité, auto-test de la vérification d'arène, `MES-S` |
| `cuda/leaf_bench.cu` | banc CUDA |
| `scripts/g4_leaf_bench.py`, `scripts/make_dumps_local.sh` | session G4 ; vidages locaux |
| `results/` | sorties locales (JSON des identités, `MES-S`, chronos, `MES_S.md`, `SHA256SUMS.sources`) |
| `dumps/` | neuf vidages, 942 Mo, **dérivés de SemanticKITTI : jamais dans le dépôt** |

4 300 lignes environ (sources, scripts et documents). Intégration : copier le dossier sans `build/`, `dumps/` ni
`results/` (ou garder `results/` sans les vidages : il ne contient que des comptes et des empreintes).

## 2. Vidages

Paramètres du produit v11 (`max_leaf` 256, cache J2 et graphe de paires actifs). Un fil, codespace chargé.

| Cas | Sites | Nœuds | Feuilles | Σm | m max | Émettrices | Boules | Incidences | Préfixes | Jugées | Tests de droites (éval. / cache) | `leaf.cpp` (s) | v11 hôte (s) |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- | ---: | ---: |
| ng00_k5_l16 | 39885 | 783071 | 353456 | 4928075 | 16 | 138490 | 1306696 | 6097121 | 120449590 | 3269620 | 75015824 (31280290 / 43735534) | 6.6 | 4.6 |
| ng00_k5_l24 | 39885 | 272249 | 123581 | 2419980 | 24 | 60858 | 1306696 | 6097121 | 174386434 | 5302831 | 133118853 (38074969 / 95043884) | 8.9 | 5.9 |
| ng00_k10_l24 | 39885 | 1137395 | 530259 | 11455500 | 24 | 228759 | 5512670 | 45383538 | 510964834 | 11509072 | 498008963 (146177271 / 351831692) | 30.4 | 20.2 |
| ng01_k5_l16 | 35551 | 637505 | 284835 | 3968155 | 16 | 105821 | 1095926 | 5085683 | 96036723 | 2654824 | 59707762 (24888969 / 34818793) | 4.7 | 3.4 |
| ng01_k5_l24 | 35551 | 222371 | 99768 | 1952848 | 24 | 46464 | 1095926 | 5085683 | 139512609 | 4260939 | 107027053 (30449805 / 76577248) | 7.2 | 4.5 |
| ng01_k10_l24 | 35551 | 932305 | 430579 | 9295715 | 24 | 169253 | 4383302 | 35706993 | 406477041 | 8861327 | 393347267 (116304828 / 277042439) | 24.1 | 16.3 |
| ng02_k5_l16 | 45845 | 735601 | 323879 | 4519093 | 16 | 121054 | 1407885 | 6514697 | 110289640 | 3305664 | 68651309 (28669257 / 39982052) | 6.1 | 4.2 |
| ng02_k5_l24 | 45845 | 262845 | 115657 | 2263204 | 24 | 51790 | 1407885 | 6514697 | 161933669 | 5191891 | 123357164 (35452918 / 87904246) | 8.0 | 5.2 |
| ng02_k10_l24 | 45845 | 1066917 | 486530 | 10502291 | 24 | 183476 | 5483320 | 44413313 | 466202866 | 10819740 | 453348834 (133640010 / 319708824) | 28.0 | 18.4 |

Aucune feuille hors lot (toutes $m\leq 24$) ; feuille v11 sur l'hôte : 0 non résolue, 0 écart, sur les neuf cas.

Empreintes sha256 des vidages (comptes et empreintes seulement, aucune coordonnée publiée) :

| Cas | sha256 |
| --- | --- |
| ng00_k5_l16 | `a2c3ec7717bffef7a87bc111eb24f17eb2d9fdbed2023d46d8cedd3d3640227b` |
| ng00_k5_l24 | `a241d2f34caef928a6376cddee214f2c4050003789e16c7c29d770bf2122401b` |
| ng00_k10_l24 | `9ca54f31f32a7cf69891b95ea042c1ac9028172479d6c77f30673a8652ccde57` |
| ng01_k5_l16 | `19f7df4c6aaf58593f1cbd543c4868d8b5a7fc5e481876a74d0e64c9bf093505` |
| ng01_k5_l24 | `48d605dba1cc83592564c747d4e5c10448b895eceed741bd0b9027940ad188e9` |
| ng01_k10_l24 | `39db7ad61dfbd1a0b9a83c176ee8eb4450369ca59414fe49163198da7ed090e6` |
| ng02_k5_l16 | `c9531cdcd9f7daa8778a5d5558e9645074f8f7fb2b078076be8e5d70229f3756` |
| ng02_k5_l24 | `53625cd9240ff9433a8b5f40611c318f0aae13e281d3b97789ed15c0e4ef9ed5` |
| ng02_k10_l24 | `e5568cbbd04a0885b7e3dafe85c5a2e618ec0d076aa3b0c3f866ac15759e4752` |

**Déterminisme** : le script G4 a reconstruit en local la bibliothèque v11 (`-DMHGP11_MODULES=catalogue`, 33 s) et
l'outil, puis revidé ng00 K5/24 : même sha256 `a241d2f3…`.

## 3. Conception des deux formes

Détail et preuves dans `README.md`. L'essentiel :

- **Sémantique par ensembles** (§ 3 du README). La boucle DFS de `leaf.cpp` visite un ensemble de préfixes défini sans
  ordre (G3, droite J2, lignes vivantes, seuils de développement), et ses quinze compteurs ont une forme close :
  `prefixes` vaut $m$ plus la somme des enfants logiques $\lvert\mathrm{CE}(P)\rvert$ des préfixes développés (les
  visites y sont déjà comptées) ; avec le cache J2, les évaluations de droites sont les triplets visités qui passent G3,
  par le **lemme des faces** (toute face d'un quadruplet visité qui passe G3 est un tel triplet). Ce lemme fonde aussi
  la table H de J3. Contrat de compteurs : **inchangé**, champ par champ.
- **Source unique** (`simt.hpp`) : code uniforme et corps de voie (`MHGP12_LANES`), `Lanes<T>`, collectives à masque
  plein (`ballot`, `shfl` de structures, `sum`, `exclusive_scan`), `atomic_or` partagé, `__syncwarp`. Sur l'hôte, voies
  jouées l'une après l'autre ; le même texte compile en `g++` et `nvcc`.
- **Forme A, J3** : D (une ligne de dominance par voie), P, T, Q par files partagées de 512 cases remplies par prefixe
  exclusif sur les voies (itérateurs persistants, tranches) et traitées par paquets de 32 ; T calcule chaque droite une
  fois et remplit H ; Q n'engendre que des quadruplets de triplets vivants et lit ses faces par ET de trois lignes de H.
- **Forme B, cohérente** : DFS uniforme ; à chaque préfixe, les enfants visités sont évalués ensemble (une voie par
  enfant) ; face $(i_0,i_1,x)$ des quadruplets lue dans le vote des droites du niveau précédent.
- **Census** (commun) : une voie par site ; lemme R ; trois votes (intérieur, coquille, non certifié) ; premier
  événement de l'ordre séquentiel (contrat Q1 de l'auditeur v11) : arrêt au $(\theta+1)$-ième intérieur ou au premier
  site non certifié. **Support canonique** : séquentiel uniforme, hors ligne (chemin froid). **Émission** du seul S*,
  dans la boîte propriétaire, avec $p+q_{\min}\leq K+1$.
- **Arithmétique** : celle de la v11 (R7, levier C : étendue ≤ $2^{20}$, sinon non résolue) ; seul ajout, le côté q2 en
  `i64` (exact sous l'étendue $2^{20}$, même signe). Aucune décision flottante.

## 4. Identité sur l'hôte

`mhgp12_leaf_identity --threads 3` sur les neuf vidages, après la dernière modification des en-têtes :

| Cas | Feuilles | Forme | Non résolues | Écarts de compteurs | Écarts d'émissions | Identité |
| --- | ---: | --- | ---: | ---: | ---: | --- |
| ng00_k5_l16 | 353 456 | j3 / coherent | 0 / 0 | 0 / 0 | 0 / 0 | oui / oui |
| ng00_k5_l24 | 123 581 | j3 / coherent | 0 / 0 | 0 / 0 | 0 / 0 | oui / oui |
| ng00_k10_l24 | 530 259 | j3 / coherent | 0 / 0 | 0 / 0 | 0 / 0 | oui / oui |
| ng01_k5_l16 | 284 835 | j3 / coherent | 0 / 0 | 0 / 0 | 0 / 0 | oui / oui |
| ng01_k5_l24 | 99 768 | j3 / coherent | 0 / 0 | 0 / 0 | 0 / 0 | oui / oui |
| ng01_k10_l24 | 430 579 | j3 / coherent | 0 / 0 | 0 / 0 | 0 / 0 | oui / oui |
| ng02_k5_l16 | 323 879 | j3 / coherent | 0 / 0 | 0 / 0 | 0 / 0 | oui / oui |
| ng02_k5_l24 | 115 657 | j3 / coherent | 0 / 0 | 0 / 0 | 0 / 0 | oui / oui |
| ng02_k10_l24 | 486 530 | j3 / coherent | 0 / 0 | 0 / 0 | 0 / 0 | oui / oui |

Émissions égales en nombre aux boules du catalogue dans chaque cas. Coquilles étendues émises (canonisation jouée) :
227, 135, 572 (K5) et 444, 280, 1 301 (K10) pour ng00, ng01, ng02 ; coquilles d'au plus 5 sites.

Première prise (avant correction) : émissions déjà identiques, seul `prefixes` différait d'exactement le nombre
d'enfants visités ; c'est ce qui a fixé la forme close de `prefixes` (§ 3).

**Auto-test de la vérification d'arène du banc** (`mhgp12_arena_selftest`, 20 000 feuilles de ng00 K5/24 et K10/24,
enregistrements mélangés) : identité, puis six mutants tués sur six (population altérée, support altéré, boule perdue,
boule doublée, feuille échangée, compteur altéré).

## 5. Diagnostics physiques (déterministes, hors empreinte)

| Cas | J3 : paquets / feuille | J3 : items P / T / Q par feuille | J3 : remplissage des paquets | Census / feuille | Cohérente : pas / feuille | Cohérente : voies utiles par pas |
| --- | ---: | --- | ---: | ---: | ---: | ---: |
| ng00 K5/16 | 7,7 | 58 / 89 / 50 | 80,5 % | 9,3 | 63,8 | 3,10 |
| ng00 K5/24 | 24,8 | 121 / 313 / 313 | 94,1 % | 42,9 | 188,9 | 3,96 |
| ng00 K10/24 | 22,5 | 119 / 281 / 272 | 93,5 % | 21,7 | 185,9 | 3,62 |
| ng01 K5/24 | 24,7 | 120 / 310 / 312 | 94,0 % | 42,7 | 187,5 | 3,96 |
| ng02 K5/24 | 24,6 | 121 / 312 / 309 | 94,1 % | 44,9 | 188,0 | 3,94 |
| ng01 / ng02 K10/24 | 22,0 / 22,3 | 118 / 275 / 264 ; 119 / 280 / 269 | 93,3 / 93,4 % | 20,6 / 22,2 | 181,9 / 185,1 | 3,61 / 3,61 |

Lecture : à K5/24, J3 traite une feuille en environ 25 paquets pleins et 43 census ; la forme cohérente en 189 pas à
4 voies utiles sur 32, plus les mêmes 43 census. La file de 512 cases déborde rarement (1,16 tranche par phase).

## 6. Compilation CUDA

nvcc 12.9.86 (`build/cuda-redist`), hôte g++ 13.3, `-std=c++20 -O3 -arch=sm_120 --expt-relaxed-constexpr -lineinfo`,
4 warps par bloc pour les formes v12 ; construction propre sans avertissement ; CMake 3.28 et 3.22.1.

| Noyau | Registres | Débordements (o) | Pile (o) | Mémoire partagée par bloc | Warps par SM (registres) |
| --- | ---: | ---: | ---: | ---: | ---: |
| witness (v11, un fil par feuille, blocs de 32) | 166 | 0 | 4 144 | 0 | 12 |
| j3 / j3_r168 / j3_r128 | 178 / 168 / 128 | 0 / 4 / 612 | 272 / 400 / 528 | 20 736 | 8 / 12 / 16 |
| coherent / coherent_r168 / coherent_r128 | 194 / 168 / 128 | 0 / 0 / 328 | 272 / 272 / 400 | 4 608 | 8 / 12 / 16 |

La pile de 4 Kio du témoin est la pile locale déjà relevée par Nsight en v11 (3,2 Kio lue à 2,2 o utiles par
secteur). Les formes v12 n'ont pas de tableau local par fil : leur état de feuille est en mémoire partagée par warp.
Sortir la canonisation du chemin chaud (`__noinline__`) a fait passer j3 de 224 à 178 registres et supprimé les
débordements à 168 ; la rendre « par valeur » les aggravait (essai mesuré, retiré).

Essai sans GPU : le binaire échoue proprement (`cudaGetDevice` : pilote absent), le script le note comme processus en
échec et rend « refusé ».

## 7. Mesures locales indicatives (un fil, codespace AMD EPYC 7763 chargé, charge 10 à 12 sur 8 cœurs)

`mhgp12_leaf_timing`, répétitions entrelacées, minimum (secondes) :

| Cas | `leaf.cpp` (avec niveaux) | feuille v11 sur l'hôte | j3 simulée | cohérente simulée |
| --- | ---: | ---: | ---: | ---: |
| ng00 K5/24 (123 581 feuilles, 3 prises) | 12,11 | 6,82 | 10,33 | 10,07 |
| ng00 K10/24 (150 000 premières feuilles, 2 prises) | 11,73 | 6,89 | 9,50 | 10,67 |
| ng00 K5/16 (353 456 feuilles, 2 prises) | 8,68 | 4,68 | 6,81 | 8,89 |

Le warp simulé coûte 1,4 à 1,9 fois la feuille v11 sur l'hôte : il paie 32 voies par census sans arrêt anticipé et
chaque dominance deux fois. **Variante SIMD sur l'hôte** : recompilée avec `-march=native` (AVX2), la même source ne
vectorise que des boucles triviales (votes, sommes, remises à zéro), ni les corps de voie (branches, boucles internes
de longueur variable, `i128`) ; aucun gain. Une feuille SIMD sur l'hôte exige des étages sans branchement (décision
G08 de la conception d'origine), que la file d'items de J3 prépare mais ne fournit pas. Conséquence pour
`ARCHITECTURE.md` § 1 (« une seule implantation par noyau, SIMD sur l'hôte et CUDA ») : la référence CPU d'une feuille
data-parallèle sera soit la même source simulée (exacte, 1,4 à 1,9 fois la feuille v11), soit un noyau d'étages propre ;
c'est à décider, ce n'est pas acquis. Aucun temps local ne prédit G4.

## 8. MES-S : étendues locales (ajout du coordinateur)

Définition (`CONTRAT_NUMERIQUE.md`, `NUM-REPERE`) : $s$ est le plus petit entier tel que la plus grande différence de
coordonnées dans l'ensemble, sur chaque axe, soit inférieure à $2^{s}$. Feuille : fermeture $[lo,hi]$ de la boîte de
centres et **tous** les sites de sa liste (y compris hors boîte) ; support : $S^{*}$ de chaque boule émise. Nuage
entier : 18 bits (157 × 155 × 15 m environ). Travail : environ dix minutes, fait en entier.

Feuilles (nombre de feuilles par valeur de $s$) :

| Cas | s=6 | s=7 | s=8 | s=9 | s=10 | s=11 | s=12 | s=13 | s=14 | s=15 | s=16 | s=17 | > 16 | > 17 | > 19 | > 20 | > 24 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| ng00_k5_l16 | 0 | 6144 | 18770 | 87537 | 104279 | 63913 | 40751 | 23471 | 7932 | 601 | 58 | 0 | 0 | 0 | 0 | 0 | 0 |
| ng00_k5_l24 | 0 | 373 | 5574 | 21420 | 39120 | 26213 | 17010 | 9782 | 3720 | 337 | 31 | 1 | 1 | 0 | 0 | 0 | 0 |
| ng00_k10_l24 | 0 | 418 | 20060 | 78060 | 162143 | 114649 | 80329 | 52660 | 19839 | 1950 | 149 | 2 | 2 | 0 | 0 | 0 | 0 |
| ng01_k5_l16 | 0 | 1429 | 24834 | 42866 | 57898 | 70669 | 48359 | 30141 | 7864 | 757 | 12 | 6 | 6 | 0 | 0 | 0 | 0 |
| ng01_k5_l24 | 0 | 71 | 4923 | 14415 | 17527 | 26256 | 19835 | 12781 | 3631 | 318 | 8 | 3 | 3 | 0 | 0 | 0 | 0 |
| ng01_k10_l24 | 0 | 105 | 15230 | 56248 | 68419 | 109087 | 90847 | 68012 | 20510 | 2091 | 17 | 13 | 13 | 0 | 0 | 0 | 0 |
| ng02_k5_l16 | 4438 | 18825 | 44075 | 63211 | 57159 | 64550 | 47818 | 19031 | 4510 | 262 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| ng02_k5_l24 | 202 | 6547 | 9140 | 24003 | 20108 | 25177 | 19611 | 8667 | 2047 | 155 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |
| ng02_k10_l24 | 287 | 26212 | 27792 | 93242 | 78161 | 108780 | 94329 | 46623 | 10438 | 666 | 0 | 0 | 0 | 0 | 0 | 0 | 0 |

Parts au-delà de $s=16$ : au plus 0,003 % (13 feuilles sur 430 579, ng01 K10/24) ; au-delà de 17, 19, 20 et 24 : **zéro**
partout. Médiane 10 à 11, 99e centile 14. La voie étroite de la v11 (étendue ≤ $2^{20}$) couvre 100 % des feuilles. La
boîte seule a une étendue médiane de 7 à 9 bits (au plus 15) : les sites hors boîte, présents dans **toutes** les
feuilles (97,9 à 99,6 % des sites d'une liste sont hors de la fermeture de la boîte), ajoutent 2 à 3 bits en médiane
(jusqu'à 9).

Supports des boules émises (nombre de boules par valeur de $s$ ; mêmes boules à K5/16 et K5/24) :

| Trame, K, arité | Boules | s=2 | s=3 | s=4 | s=5 | s=6 | s=7 | s=8 | s=9 | s=10 | s=11 | s=12 | s=13 | s=14 | s=15 | > 16 |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| ng00_k5 q2 | 456919 | 0 | 7 | 300 | 3687 | 24159 | 77223 | 139310 | 118567 | 54494 | 21726 | 10070 | 5469 | 1788 | 119 | 0 |
| ng00_k5 q3 | 691283 | 0 | 0 | 0 | 175 | 16087 | 82030 | 207691 | 214527 | 102904 | 39166 | 16417 | 8861 | 3239 | 186 | 0 |
| ng00_k5 q4 | 158494 | 0 | 0 | 0 | 2 | 1522 | 13737 | 44783 | 54596 | 27767 | 10265 | 3713 | 1561 | 528 | 20 | 0 |
| ng00_k10 q2 | 881908 | 0 | 7 | 300 | 3687 | 24695 | 102329 | 209364 | 269865 | 150353 | 64129 | 31925 | 17949 | 6877 | 428 | 0 |
| ng00_k10 q3 | 2898218 | 0 | 0 | 0 | 175 | 17213 | 210432 | 545163 | 1046839 | 625600 | 250574 | 109909 | 62776 | 27878 | 1659 | 0 |
| ng00_k10 q4 | 1732544 | 0 | 0 | 0 | 2 | 1774 | 86635 | 253774 | 679975 | 438697 | 168793 | 62447 | 28240 | 11788 | 419 | 0 |
| ng01_k5 q2 | 396629 | 0 | 2 | 151 | 2171 | 15368 | 68217 | 99356 | 92513 | 67715 | 29583 | 13190 | 6855 | 1370 | 138 | 0 |
| ng01_k5 q3 | 577994 | 0 | 0 | 0 | 34 | 6030 | 78607 | 147712 | 134806 | 116309 | 56709 | 22527 | 12154 | 2865 | 241 | 0 |
| ng01_k5 q4 | 121303 | 0 | 0 | 0 | 0 | 375 | 11792 | 32308 | 26419 | 26725 | 15446 | 5342 | 2362 | 517 | 17 | 0 |
| ng01_k10 q2 | 755068 | 0 | 2 | 151 | 2171 | 15501 | 80971 | 172979 | 164872 | 160740 | 88670 | 40429 | 22684 | 5421 | 477 | 0 |
| ng01_k10 q3 | 2357638 | 0 | 0 | 0 | 34 | 6329 | 137634 | 527191 | 482841 | 577152 | 362159 | 151397 | 86999 | 23924 | 1978 | 0 |
| ng01_k10 q4 | 1270596 | 0 | 0 | 0 | 0 | 434 | 47950 | 261514 | 230839 | 326361 | 250155 | 96311 | 45518 | 10992 | 522 | 0 |
| ng02_k5 q2 | 518233 | 1 | 165 | 2208 | 13452 | 45301 | 128827 | 157015 | 83115 | 50357 | 20951 | 11645 | 4387 | 686 | 123 | 0 |
| ng02_k5 q3 | 746547 | 0 | 0 | 118 | 8303 | 52747 | 153960 | 255891 | 122015 | 86255 | 38082 | 19813 | 7701 | 1454 | 208 | 0 |
| ng02_k5 q4 | 143105 | 0 | 0 | 3 | 1085 | 12261 | 22399 | 51815 | 20329 | 19508 | 9564 | 4334 | 1577 | 218 | 12 | 0 |
| ng02_k10 q2 | 977534 | 1 | 165 | 2208 | 13635 | 65452 | 162131 | 303342 | 187793 | 125605 | 62928 | 36043 | 14705 | 3090 | 436 | 0 |
| ng02_k10 q3 | 3019193 | 0 | 0 | 118 | 8777 | 170257 | 328915 | 1028156 | 581338 | 443746 | 249179 | 137833 | 56117 | 13206 | 1551 | 0 |
| ng02_k10 q4 | 1486593 | 0 | 0 | 3 | 1263 | 97215 | 154839 | 491950 | 229038 | 233571 | 164720 | 80455 | 28348 | 4981 | 210 | 0 |

Tous les supports ont $s\leq 15$ : aucune part au-delà de 16 (donc de 17, 19, 20, 24). Médiane 8 à 10.

Feuilles dont l'étendue est fixée par une minorité de sites :

| Cas | s − s(sans le site le plus lointain) = 0 / 1 / 2 | ≥ 2 (part) | meilleur retrait d'un site : = 0 / 1 / 2 | s − s(boîte seule) : médiane, max |
| --- | --- | ---: | --- | --- |
| ng00_k5_l16 | 328933 / 24493 / 30 | 30 (0,0085 %) | 304376 / 49050 / 30 | 2, 8 |
| ng00_k5_l24 | 116588 / 6987 / 6 | 6 (0,0049 %) | 109701 / 13874 / 6 | 2, 7 |
| ng00_k10_l24 | 505844 / 24394 / 21 | 21 (0,0040 %) | 477725 / 52513 / 21 | 3, 9 |
| ng01_k5_l16 | 265541 / 19276 / 18 | 18 (0,0063 %) | 246312 / 38505 / 18 | 2, 9 |
| ng01_k5_l24 | 93706 / 6057 / 5 | 5 (0,0050 %) | 88063 / 11700 / 5 | 2, 7 |
| ng01_k10_l24 | 410386 / 20173 / 20 | 20 (0,0046 %) | 386609 / 43950 / 20 | 3, 9 |
| ng02_k5_l16 | 301677 / 22182 / 20 | 20 (0,0062 %) | 277008 / 46851 / 20 | 2, 8 |
| ng02_k5_l24 | 108706 / 6944 / 7 | 7 (0,0061 %) | 101425 / 14225 / 7 | 2, 7 |
| ng02_k10_l24 | 464774 / 21745 / 11 | 11 (0,0023 %) | 433045 / 53474 / 11 | 3, 9 |

« Le plus lointain » : le site de plus grande distance L∞ à la fermeture de la boîte. Le retirer abaisse $s$ d'un bit
dans 4,5 à 6,9 % des feuilles, de deux bits dans 0,002 à 0,008 % ; le meilleur retrait d'un seul site abaisse $s$ d'un
bit dans 9,9 à 14,5 %. Lecture pour le contrat : l'hypothèse « presque toujours sous $2^{17}$ » est vérifiée (toujours,
sur ces trames) ; un palier étroit $s\leq 16$ couvre au moins 99,997 % des feuilles et tous les supports ; un palier
$s\leq 14$ (numérateurs q4 en `i64`, table du § 3 du contrat) en couvrirait 99,5 à 99,9 % ; l'étirement par un seul site est
marginal. Limites : trames de la séquence 08 seulement, au millimètre ; ni dixième de millimètre, ni parties de
descente, ni scènes de plusieurs millions de points (non vidées ici).

## 9. Prédictions écrites avant G4

Estimations, pas des mesures. Témoin (v11, G4) : comptage d'environ 60 ms à K5/24 et 196 ms à K10/24 sur ng00
(rapports `claudediag1` et `cache_blocs`), un fil sur 32 actif sur 3,2 et pile locale. J3 : environ 25 paquets pleins
et 43 census par feuille à K5/24, sans pile locale ; la feuille v11 coûte 1,4 à 1,9 fois moins que J3 en travail
scalaire total, mais sur 3,2 voies contre environ 30. **Attendu** : J3 entre 0,1 et 0,4 du témoin aux cas qui décident,
moins favorable à K5/16 (feuilles de 16 sites : la moitié des voies inoccupées au census) ; la forme cohérente entre
0,3 et 0,9 (189 pas à 4 voies utiles), probablement rejetée. Le réglage de registres le plus rapide est imprévisible
(8, 12 ou 16 warps par SM contre débordements).

## 10. Ce que le passage sur G4 doit mesurer

Une session gardée, environ 20 minutes (limite de 30 minutes par lot) :

```text
python3 <dépôt>/<chemin intégré>/scripts/g4_leaf_bench.py --out <sortie hors /tmp> --data <trames lidar_ng0X.*>
```

1. **Le verdict** de chaque variante (adopté, rejeté, refusé) et le choix, par la règle du README § 9 : rapport des
   médianes au témoin par processus, moyenne géométrique et intervalle bootstrap sur 5 processus, borne haute au plus
   1/3 sur les six cas à feuilles 24, identité exigée sur les neuf cas.
2. **L'identité sur l'appareil** de toutes les feuilles (statuts, quinze compteurs, ensembles d'émissions) et le nombre
   de feuilles non résolues (attendu 0) ; Compute Sanitizer `memcheck`, `racecheck`, `synccheck` sur 3 000 feuilles
   (attendu : aucune erreur ; un défaut refuse le banc).
3. **La fidélité du témoin** : son temps doit retrouver le comptage connu de la v11 (environ 60 ms et 196 ms sur ng00) ;
   un écart important met en cause le témoin, pas les formes.
4. **Les temps absolus** de la variante choisie à K5/24 et K10/24, à rapprocher du budget du catalogue
   (`ARCHITECTURE.md` § 3 : C entre 35 et 45 ms à K5, feuilles comprises), et les rapports K5/16 (choix de la taille de
   feuille d'un noyau par warp).
5. Publiés avec : versions (nvcc, pilote, CMake, compilateur), mode de persistance, horloges, processus GPU présents,
   empreintes des sources, des binaires et des vidages, journaux de chaque étape.

Suites si J3 est adoptée (chacune avec sa propre règle) : profil Nsight Compute de la variante choisie (raisons
d'attente, occupation atteinte, conflits de banques) ; census groupés pour $m\leq 16$ (deux feuilles ou deux
présentations par warp) ; taille de feuille 32 ; paliers étroits du contrat numérique ($s\leq 14$ ou 16, `MES-S`
ci-dessus) ; intégration en flux dans le parcours des boîtes (`MES-M5`). Si la forme cohérente est rejetée, la fermer.

## 11. Risques et limites

- **Aucune exécution GPU** : des fautes propres à l'appareil (ordre mémoire, convergence, registres non initialisés
  dans les échanges) ne se voient pas dans la simulation ; elles se verraient à l'identité sur l'appareil et sous
  Compute Sanitizer, prévus dans la session.
- **Occupation** : 8 warps par SM sans débordement, 12 ou 16 avec ; l'arithmétique `i128` (centres, côté q3/q4) pèse
  sur les registres.
- **Census** : un pas de warp par présentation jugée (43 par feuille à K5/24) ; c'est la part qui ne se parallélise
  qu'à l'intérieur d'une présentation.
- **Feuilles de 16 sites** : la moitié des voies inoccupées au census et à la dominance.
- **Témoin** : copie du comptage de la v11 sans les passes de préfixes, copie et écriture ; la règle ne porte que sur le
  comptage, comme le plan l'écrit.
- **Portée** : noyau seul ; transferts, contexte et fin d'étage (tri, assemblage) hors mesure ; arène aux totaux de
  référence (un débordement rend la forme non identique).
- **Départage de S*** : celui de la v11 (ordre des `SiteIdx`) ; le départage par coordonnées du contrat v12
  (`CST-0113`) n'est pas porté. Les paliers d'étendue du contrat v12 non plus : les certificats sont ceux de la v11,
  globaux au profil (le constat `CST-0201` porte sur la garde v12, qui n'est pas en jeu ici : la feuille ne confronte que
  ses propres sites).
- **Compatibilité VM** : CMake 3.22.1 testée ; GCC 11.4 non disponible localement (code C++20 sans fonctionnalité
  propre à GCC 13 ; contrôlé aussi avec clang 18 en `-Werror`).
- **Données** : vidages hors dépôt (licence SemanticKITTI) ; seuls comptes et empreintes ci-dessus.

## 12. Reproduction locale

```text
cmake -S v12_feuille -B b -DCMAKE_BUILD_TYPE=Release -DMHGP11_SOURCE_DIR=<dépôt>/morsehgp3D_v11 \
      -DMHGP11_LIBRARY=<construction v11 u21>/libmhgp11.a [-DMHGP12_FEUILLE_CUDA=ON -DCMAKE_CUDA_COMPILER=<nvcc>]
cmake --build b -j3
MHGP12_DUMP_TOOL=b/mhgp12_leaf_dump v12_feuille/scripts/make_dumps_local.sh
b/mhgp12_leaf_identity --threads 3 dumps/*.bin
b/mhgp12_arena_selftest dumps/ng00_k5_l24.bin
b/mhgp12_mes_s dumps/*.bin
b/mhgp12_leaf_timing dumps/ng00_k5_l24.bin 3
```
