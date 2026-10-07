# Microbanc MES-M2 : la feuille du catalogue, data-parallèle sur un warp

7 octobre 2026. Microbanc de la tranche T0 de la v12 (`morsehgp3D_v12/docs/PLAN.md`, `MES-M2`), **hors produit**.

```text
phase=exploration_v12_hors_registre
backend=cpu_reference (identité, warp simulé) ; cuda_g4 (banc, à jouer sur G4)
quantification=quantized_u21_input_only
public_status=not_claimed
GCP non utilisé
```

Question posée par `ARCHITECTURE.md` § 4.1 : la feuille du catalogue doit-elle, sur le GPU, prendre la forme **J3 par
phases** ou la forme **cohérente** (tout le warp sur un même préfixe) ? **Jamais un fil par feuille** (3 fils actifs sur
32 dans la v11). Règle d'adoption écrite d'avance (`PLAN.md`) : temps de comptage au plus **1/3** de celui du noyau un-fil
de la v11 (témoin), et **identité** avec la feuille de référence `leaf.cpp`.

## 1. Carte du dossier

| Chemin | Rôle |
| --- | --- |
| `vidage/leaf_dump.cpp` | vidage des feuilles de la v11 et de leur sortie de référence `leaf.cpp` (lié à `libmhgp11.a`) |
| `vidage/leaf_timing.cpp` | chrono local indicatif à un fil : `leaf.cpp`, feuille v11 sur l'hôte, formes v12 simulées |
| `include/mhgp12/leaf/dump_format.hpp` | format binaire `MHGP12LF` v1 (en-tête versionné, sections, FNV-1a) |
| `include/mhgp12/leaf/simt.hpp` | warp en source unique : appareil réel ou warp simulé déterministe sur l'hôte |
| `include/mhgp12/leaf/predicates.hpp` | prédicats exacts, port explicite de `leaf_device_predicates.hpp` (v11) |
| `include/mhgp12/leaf/leaf_common.hpp` | préparation, census par vote du warp, support canonique, compteurs logiques |
| `include/mhgp12/leaf/leaf_j3.hpp` | forme A, phases à la J3 |
| `include/mhgp12/leaf/leaf_coherent.hpp` | forme B, cohérente |
| `include/mhgp12/leaf/arena.hpp`, `compare.hpp` | arène d'émissions (16 o par boule) et vérification contre le vidage |
| `host/leaf_identity.cpp` | identité sur l'hôte des deux formes, toutes les feuilles |
| `host/arena_selftest.cpp` | auto-test de la vérification d'arène du banc (arrivées mélangées, six mutants) |
| `host/dump_admission_selftest.cpp` | porte d'admission des vidages (`CST-0215`) : vidage synthétique valide admis, 46 mutants à empreinte juste refusés |
| `host/mes_s.cpp` | mesure `MES-S` du contrat numérique (étendues locales) |
| `cuda/leaf_bench.cu` | banc CUDA : témoin v11, formes J3 et cohérente, variantes de registres, chrono, identité |
| `scripts/g4_leaf_bench.py` | session G4 complète (bibliothèque standard) : construit, vide, admet, vérifie, mesure, juge |
| `tests/test_juge_m2.py` | porte du juge et du pilote (`CST-0018`, `CST-0215`) : 32 scénarios simulés (dont les six injections du reçu `audit_socle_microbancs_20261007` et les quatre du reçu `audit_cd_corrections_20261007`), juge pur, rejugement des 45 prises réelles du reçu G4 et cohérence des compteurs de leurs 333 formes |
| `tests/test_admission_m2.py` | porte native de l'admission : mutants rejoués par les vrais outils (code 2, aucun noyau), vidages réels admis |
| `scripts/make_dumps_local.sh` | vidages locaux (hors dépôt) |
| `CMakeLists.txt` | CMake ≥ 3.20 (3.22.1 de la VM), C++20, `-Wall -Wextra -Wpedantic -Werror` côté hôte |
| `RAPPORT.md` | résultats du 7 octobre |

Les vidages (`dumps/`, environ 940 Mo pour neuf cas) sont dérivés de SemanticKITTI : **jamais dans le dépôt**. Seuls
leurs comptes et leurs empreintes sont publiés.

## 2. Vidage des feuilles de la v11

`mhgp12_leaf_dump <xyz.u32le> <ids.u32le> <K> <feuille> <sortie.bin>` lit l'entrée comme `mhgp11_full_bench`, prépare le
`Cloud` de la v11 (profil u21), puis :

1. **parcours** : `catalogue_detail::walk` séquentiel, avec une file différée branchée sur `Run::deferred` ; chaque
   feuille admissible à la voie lot (graphe de paires, $1\leq m\leq 32$) est copiée (boîte T0 demi-ouverte, `SiteIdx`
   croissants) ; la partition est celle de toutes les voies (même filtre G1, même ajustement, même bissection) ;
2. **référence** : `enumerate_leaf` de `leaf.cpp` (voie graphe de paires, cache J2 actif comme le produit), feuille par
   feuille ; émissions dans l'ordre de `leaf.cpp` et quinze compteurs logiques lus comme différence du grand livre ;
3. **témoin** : la feuille `leaf_device.hpp` de la v11 jouée sur l'hôte ; son statut est noté, et ses émissions et
   compteurs doivent égaler ceux de `leaf.cpp` quand elle est résolue (sinon code 3).

Paramètres du produit v11 : `max_leaf` 256, `cache_center_lines` et `pair_graph` actifs. Le vidage est déterministe :
reconstruit depuis une bibliothèque v11 neuve, il est identique à l'octet (§ 3 du rapport).

**Format `MHGP12LF` v1** (`dump_format.hpp`) : en-tête de 128 octets (magie, version, profil, K, taille de feuille,
drapeaux, comptes), puis nuage (x, y, z), feuilles (64 octets, disposition de `LeafJob`), sites, compteurs (15 × u32 par
feuille), statuts, débuts et enregistrements (8 octets, disposition de `LeafRecord` : $S^{*}$ en rangs locaux, p, m,
$q_{\min}$), débuts et populations (rangs locaux, I puis U) ; FNV-1a 64 final ; lecteur strict.

**Admission** (`CST-0215`, contre-audit Codex) : une empreinte FNV-1a juste protège les octets, pas le domaine des
noyaux. `dump::read` refuse donc, jamais en silence : avant toute allocation dépendant des comptes, un en-tête hors
domaine (profil différent de celui du binaire, K hors de 1..12, taille de feuille hors de K+3..256, `max_leaf` hors de
taille..256, drapeaux inconnus ou sans graphe de paires, comptes au-delà des indices `u32` du banc, vidage sans
feuille, champ réservé non nul) ou dont les tailles de sections ne donnent pas exactement la taille du fichier ; après
l'empreinte, toute coordonnée hors de $[0,2^{B})$, toute feuille qui ne pave pas la liste des sites dans l'ordre (d'où
aucun débordement de début + m), toute boîte hors de $0\leq lo<hi\leq 2^{B}$, tout site hors du nuage, non croissant ou
répété, tout statut sans le bit de `leaf.cpp` ou avec l'écart du témoin v11, tout début d'enregistrements ou
d'incidences faux, tout compteur `emitted`/`incidences` différent de sa plage, tout enregistrement hors domaine
($q_{\min}$, S* croissant dans la feuille, $p+m$, $p+q_{\min}\leq K+1$) et toute population non croissante, hors de la
feuille, non disjointe ou sans S* dans la coquille. Les outils (`mhgp12_leaf_identity`, `mhgp12_mes_s`, le banc) admettent
**tous** leurs vidages avant le premier noyau, relisent chacun avec la même empreinte et la citent (`dump_fnv1a`) ;
`mhgp12_leaf_identity --admission` admet sans calculer. Les neuf vidages réels (ng00–02) sont admis.

## 3. Sémantique de référence et compteurs logiques

La référence est la voie graphe de paires de `leaf.cpp` (et son port fidèle `leaf_device.hpp`), $m\leq 32$. On la décrit
par **ensembles**, ce qui la rend indépendante de l'ordre de visite :

- $\mathrm{cnt}(P)$ est le cardinal de l'union des dominateurs des sites de $P$ ; `live[t][x]` contient les voisins $y$
  de $x$ dans le graphe de paires avec $\mathrm{cnt}(\lbrace x,y\rbrace)\leq K-1-t$ ($t=0,1,2$) ;
- $V_1$ est l'ensemble des sites ; un préfixe visité $P=(i_0<\dots<i_{q-1})$ est **développé** s'il passe G3
  ($\mathrm{cnt}(P)\leq K+1-q$), la droite J2 si $q=3$, et si $q<4$ et $K\geq q$ ;
- ses enfants logiques sont $\mathrm{CE}(P)$ : les sites au-dessus de $i_{q-1}$ voisins de tous les sites de $P$ ; ses
  enfants visités (si de plus $\mathrm{cnt}(P)\leq K-q$) sont les sites au-dessus de $i_{q-1}$ présents dans toutes les
  lignes `live[q-1][p]`.

Les compteurs de `leaf.cpp` ont alors une forme close, que les deux formes calculent :

| Compteur | Valeur |
| --- | --- |
| `dominance_tests` | $m(m-1)/2$ |
| `prefixes` | $m$ plus la somme des $\lvert\mathrm{CE}(P)\rvert$ sur les préfixes développés (les visites y sont déjà comptées : la boucle DFS ajoute 1 par visite et la différence logique moins visités) |
| `judged`, `census_tests`, `emitted`, `incidences`, `q4_candidates`, `q4_levels` | sommes sur les présentations, indépendantes de l'ordre |
| `region_line_tests` | 1 par triplet visité qui passe G3, puis, par quadruplet visité qui passe G3, les faces $(i_0,i_1,i_3)$, $(i_0,i_2,i_3)$, $(i_1,i_2,i_3)$ jusqu'au premier échec |
| `region_line_evaluations`, `cache_hits` | avec le cache J2 : nombre de triplets visités qui passent G3, et le reste ; sans cache : tous les tests, et zéro |
| `region_pair_*`, `region_line_fallbacks` | nuls (graphe de paires, $m\leq 32$) |

**Lemme des faces.** Toute face d'un quadruplet visité $(i_0,i_1,i_2,i_3)$ qui passe G3 est un triplet visité qui passe
G3. Preuve : $\mathrm{cnt}$ est croissant pour l'inclusion, donc chaque sous-préfixe a un compte au plus
$\mathrm{cnt}(P)\leq K-3$ et passe G3 et les seuils de développement de son cardinal ; $i_3$ est dans
`live[2]` de $i_0,i_1,i_2$, donc dans `live[1]` et `live[0]` (les seuils décroissent) ; $i_2$ est dans `live[1]` de
$i_0$ et $i_1$ ; d'où $(i_0,i_1,i_3)$, $(i_0,i_2,i_3)$ et $(i_1,i_2,i_3)$ visités par la même règle. Les évaluations du
cache J2 (rangs distincts consultés) sont donc exactement les triplets visités qui passent G3, quel que soit l'ordre.
Ce lemme fonde aussi la table H de la forme J3 : les faces d'un quadruplet se lisent dans les triplets déjà jugés.

Résultat : **même contrat de compteurs que la v11, champ par champ** ; aucun compteur nouveau dans l'empreinte. Le travail
physique (pas de warp, voies utiles, census) est publié à part (`diag`), jamais dans une empreinte.

## 4. Source unique et warp simulé

`simt.hpp` impose un style « super-pas » :

- le code **uniforme** (hors `MHGP12_LANES`) est exécuté par les 32 voies sur l'appareil, une fois sur l'hôte ; il ne lit
  que des valeurs identiques sur les voies (paramètres, collectives, mémoire partagée relue après `sync`) ;
- `MHGP12_LANES(l) { ... }` est le corps d'**une** voie : la voie courante sur l'appareil, une boucle $l=0..31$ sur l'hôte ;
  ni collective, ni `break`, ni `return` dedans ;
- `Lanes<T>` porte une valeur par voie (un registre sur l'appareil, 32 cases sur l'hôte) ;
- collectives à masque plein en code uniforme : `ballot`, `shfl` (structures comprises), `sum`, `exclusive_scan` ;
  `atomic_or` en mémoire partagée ; `sync` (`__syncwarp`) entre écriture partagée et lecture par une autre voie.

Sur l'hôte la simulation est séquentielle et déterministe ; sur l'appareil, les collectives synchronisent les voies et
`__syncwarp` publie les écritures partagées. Le même texte est compilé par `g++` (hôte) et `nvcc` (appareil).

## 5. Forme A : phases à la J3 (`leaf_j3.hpp`)

1. **D** (commun) : coordonnées en mémoire partagée ; une ligne de dominance par voie (chaque couple évalué par ses deux
   voies, toujours dans l'orientation canonique de la v11) ; graphe de paires ; lignes vivantes.
2. **P** paires visitées, **T** triplets visités, **Q** quadruplets visités : chaque voie $i$ engendre les items de la ligne
   $i$ par un itérateur persistant ; un préfixe exclusif sur les voies les range dans une file partagée de 512 cases,
   traitée par paquets de 32 (une voie par item) ; une file plus longue passe par tranches, chaque item n'étant engendré
   qu'une fois.
3. **T** calcule la droite J2 **une fois par triplet** (aucun cache à consulter) et pose le bit $k$ de la ligne $(i,j)$
   de la **table H** (triplets vivants : visités, G3, droite qui rencontre la boîte) ; candidats q3.
4. **Q** n'engendre ses quadruplets que depuis les triplets vivants qui se développent ; les trois autres faces sont
   lues dans trois lignes de H (**ET des trois bits**), les compteurs suivent l'ordre séquentiel des faces ; candidats q4.
5. Chaque présentation jugée d'un paquet est recensée par **tout le warp** (§ 7), en ordre croissant des voies.

Mémoire partagée par warp : 5 184 octets (coordonnées, masques, H de 496 mots, file de 512 mots).

## 6. Forme B : cohérente (`leaf_coherent.hpp`)

Parcours en profondeur **uniforme** des préfixes (pas de pile locale, pas de divergence de parcours). À chaque préfixe
développé, tous ses enfants visités sont évalués ensemble, la voie $x$ jugeant l'enfant $P\cup\lbrace x\rbrace$ (G3,
droites, fabrique du candidat, comptes logiques) ; chaque présentation jugée est recensée par tout le warp ; puis le warp
descend dans chaque enfant développé, en ordre croissant. Réutilisation locale : au niveau des quadruplets sous
$(i_0,i_1,i_2)$, la face $(i_0,i_1,x)$ est lue dans le vote des droites fait sous $(i_0,i_1)$ ; les deux autres faces
sont recalculées. Mémoire partagée par warp : 1 152 octets.

## 7. Census par vote, support canonique, émission, arithmétique

- **Census** (lemme R puis vote) : chaque voie $s<m$ classe son site (générateur : contact ; dominateur d'un générateur :
  intérieur ; dominé : extérieur ; sinon côté exact certifié). Trois votes donnent les masques intérieur $I$, coquille $C$
  et non certifié $U$. **Premier événement de l'ordre séquentiel** (contrat Q1 de l'auditeur v11) : arrêt au
  $(\theta+1)$-ième intérieur, $\theta=K+1-q$, ou au premier site non certifié ; `census_tests` compte jusqu'au site
  d'arrêt inclus, ou $m$. Un site à la fois dominateur et dominé des générateurs (conflit des deux masques du lemme R)
  rend la feuille non résolue, comme la v11 (refus d'invariant de `leaf.cpp`).
- **Support canonique** (coquilles étendues, 0,02 à 0,04 % des boules) : code uniforme séquentiel, hors ligne (chemin
  froid), même ordre que `support.cpp` (paires, triangles strictement aigus coplanaires, tétraèdres), premier succès ou
  refus non certifié.
- **Émission** : seul $S^{*}$, s'il est la présentation génératrice, dans la boîte propriétaire (centre dans la boîte
  demi-ouverte, testé avant le census), et si $p+q_{\min}\leq K+1$. Populations écrites par rang de bit.
- **Arithmétique** : celle de la v11 (contrat R7). Feuille d'étendue au plus $2^{20}$ (levier C) : droites J2,
  orientation q4 et $t$ du q3 en `i32`/`i64` ; centres, appartenance à la boîte et côté q3/q4 en `i128` sous les
  certificats de la v11 ; sinon la feuille rend « non résolue » (rejouée en exact par `leaf.cpp` avant admission).
  **Seul ajout** : le côté q2 en `i64` (sous l'étendue $2^{20}$, $\lvert v\rvert^{2}$ et $(b-a)\cdot v$ sont au plus
  $3\cdot 2^{40}$ ; même signe que le chemin `i128`, facteur 2 près). Aucune décision flottante.

## 8. Banc CUDA (`cuda/leaf_bench.cu`)

- **witness** : copie de `count_kernel` de la v11 (`leaf_batch_cuda.cu`, l. 52–75) : même `leaf_device.hpp`, même puits
  `ScratchSink` à cases de 2 Kio et réservoir chaîné (en-têtes v11 inclus tels quels), blocs d'un warp, fils par $m$
  décroissant ;
- **j3**, **coherent** : un warp par feuille, 4 warps par bloc, mêmes feuilles dans le même ordre ; trois bornes de
  registres nommées : libre, `_r168` (12 warps par SM), `_r128` (16 warps par SM) ;
- chrono : événements CUDA autour du seul noyau, remises à zéro hors chrono, échauffement, formes entrelacées à chaque
  répétition (ordre tournant) ;
- vérification après la dernière prise : statuts, compteurs par feuille et **ensemble des émissions** par feuille
  (arène regroupée par feuille, triée par $S^{*}$) pour les formes v12 ; totaux des compteurs, boules et incidences par
  feuille pour le témoin. La vérification d'arène est partagée avec `host/arena_selftest.cpp` ;
- `--leaves N` restreint le banc aux N premières feuilles (contrôles sous Compute Sanitizer) ;
- preuves de la prise (`CST-0018`) : `--nonce` répète le jeton de la session dans le JSON ; chaque cas cite l'empreinte
  du vidage lu, son profil et ses comptes ; le JSON est écrit dans un fichier temporaire puis renommé, et une écriture
  incomplète rend le code 2 (jamais un code 0 sans résultat).

Ressources (ptxas 12.9, sm_120) :

| Noyau | Registres | Débordements (o) | Pile (o) | Mémoire partagée par bloc | Warps par SM |
| --- | ---: | ---: | ---: | ---: | ---: |
| witness | 166 | 0 | 4 144 | 0 | 12 |
| j3 / j3_r168 / j3_r128 | 178 / 168 / 128 | 0 / 4 / 612 | 272 / 400 / 528 | 20 736 | 8 / 12 / 16 |
| coherent / coherent_r168 / coherent_r128 | 194 / 168 / 128 | 0 / 0 / 328 | 272 / 272 / 400 | 4 608 | 8 / 12 / 16 |

## 9. Règle écrite d'avance et juge

Écrite avant toute mesure GPU, jamais réécrite après les données (`PLAN.md` § 0, `MESURE.md` § 5) :

1. cas : ng00, ng01, ng02 à K5/16, K5/24, K10/24 (neuf vidages). **Décident** les six cas à feuilles 24 (K5/24 et
   K10/24), configuration GPU de la v11 (`868347:400`) qu'il s'agit de battre ; les trois cas K5/16 sont mesurés et
   publiés (choix de la taille de feuille du noyau par warp), et l'identité y est exigée aussi ;
2. par cas, par variante et par processus : rapport des médianes (variante sur témoin) sur les répétitions du
   processus ; les variantes sont entrelacées dans chaque répétition ; un premier processus d'échauffement est jeté ;
3. agrégat : moyenne géométrique des rapports sur au moins 5 processus, intervalle à 95 % par bootstrap sur les
   processus (10 000 tirages, graine fixe) ;
4. **adopté** : identité sur toutes les feuilles résolues de tous les cas, et borne haute de l'intervalle au plus 1/3
   sur **chaque** cas qui décide ; **rejeté** : identité en défaut sur un cas, ou borne haute au-dessus de 1/3 sur un cas
   qui décide ; **refusé** : banc invalide (vidage ou processus manquant, binaire non construit ou non haché, témoin
   différent de la référence, auto-test d'arène en échec, isolation GPU non certifiée) ; seul « adopté » permet
   l'adoption ;
5. choix : parmi les variantes adoptées, la plus petite moyenne géométrique sur les six cas qui décident ; les feuilles
   non résolues sont publiées (attendu : zéro sur ces trames), ainsi que les rapports des cas K5/16.

**Preuves exigées** (`CST-0018`, `CST-0215` ; la règle et sa statistique ne changent pas, le juge rejugeant les 45
prises du reçu G4 redonne exactement ses verdicts et intervalles). « Adopté » exige toutes les preuves présentes et
fraîches ; toute absence rend « refusé », jamais « adopté » ni « rejeté » :

- contrat : trames ng00–02, configurations 5:16, 5:24, 10:24, cas qui décident 5:24 et 10:24, au moins 5 processus,
  identité hôte et sanitizers joués, banc CUDA joué ; une option qui s'en écarte publie la mesure et refuse l'adoption ;
- binaires construits et hachés, inchangés en fin de session ; sources inchangées ; porte d'admission conforme ;
- vidages : construits dans la session (cible effacée avant, code 0) ou fournis, en-tête conforme au cas, empreintes
  (sha256 et FNV-1a finale) relevées, admis par le lecteur C++ avant tout noyau, inchangés en fin de session ;
- identité hôte de code 0, une ligne par cas et par forme de base, identité, couverture complète et empreinte du vidage ;
- auto-test d'arène de code 0 sur le vidage cité, aucun mutant vivant ;
- Compute Sanitizer trouvé et joué (memcheck, racecheck, synccheck) : aucun défaut (code 0, ou 1 si le banc constate
  lui-même un écart d'identité), prise neuve de la session sur le bon vidage, validée comme une prise du banc :
  exactement les formes et la répétition de la commande, couverture des `--sanitizer-leaves` premières feuilles
  (comptes de référence bornés par ceux du vidage), compteurs cohérents, code et identité concordants ;
- prise d'échauffement et chaque prise du banc : fichier neuf (cible effacée avant), jeton de la session, vidage cité
  par son chemin, son empreinte et ses comptes, exactement les formes et répétitions demandées, durées finies
  strictement positives, code cohérent avec l'identité, témoin identique à la référence ; une forme qui se déclare
  identique n'a ni écart de comptes ou d'émissions ni débordement, et son arène égale celle de la référence quand
  toutes ses feuilles sont résolues (reçu `audit_cd_corrections_20261007/m2`) ;
- chaque cas attendu présent avec ses N processus valides ; isolation du GPU relevée au début, avant et après le banc.

Le rapport cite ses preuves (`evidence` : jeton, empreintes des vidages, prises et journaux).

## 10. Session G4

Depuis la racine du dépôt où ce dossier est intégré (`<chemin>` ci-dessous), sur la VM :

```text
python3 <chemin>/scripts/g4_leaf_bench.py --out <sortie> --data <dossier des trames lidar_ng0X.*>
```

Le script trouve `nvcc` comme les sessions v11 (PATH, `CUDA_HOME`, `/usr/local/cuda`, `/usr/local/cuda-12.*`),
construit la bibliothèque v11 (`-DMHGP11_MODULES=catalogue`, cible `mhgp11`), l'outil de vidage, le banc et les outils
hôte, vide les neuf cas en parallèle, joue l'identité hôte, l'auto-test d'arène, `MES-S`, Compute Sanitizer
(`memcheck`, `racecheck`, `synccheck` sur les 3 000 premières feuilles de ng00 K5/24, formes v12 ; un défaut refuse le
banc), puis 1 + 5 processus de banc par cas (15 répétitions, 3 d'échauffement), et écrit `<sortie>/report.json`
(versions, empreintes des sources, des binaires et des vidages, journaux, verdicts). Option `--dumps` pour des vidages déjà faits. Durée estimée : 15 à
25 minutes. Codes : 0 rapport écrit (quel que soit le verdict), 2 refus avant toute mesure.
Portes locales (sans GPU) : `python3 -S -O tests/test_juge_m2.py` (juge, pilote, reçu G4) et
`python3 -S -O tests/test_admission_m2.py --binaires <hôte>` (admission native).

## 11. Limites et risques

- **Rien n'est mesuré sur GPU ici** : aucun GPU local ; la compilation `sm_120` et l'identité sur l'hôte sont faites, le
  temps reste à mesurer sur G4.
- La forme cohérente n'a que 3,1 à 4,0 voies utiles par pas sur les trames (189 pas par feuille à K5/24) ; la forme J3
  remplit ses paquets à 94 % (80 % à K5/16) mais paie la génération des files et la table H.
- Registres : les deux formes tiennent 8 warps par SM sans débordement ; les bornes à 168 et 128 registres échangent
  occupation contre débordements. Le meilleur réglage est une mesure de G4.
- Le census occupe tout le warp à chaque présentation jugée (43 par feuille à K5/24, 22 à K10/24, 9 à K5/16) : c'est
  le poste qui reste séquentiel par présentation.
- La variante SIMD sur l'hôte « même source » n'existe pas par simple recompilation (§ 5 du rapport).
- Le départage canonique par coordonnées de la v12 (`CONTRAT_NUMERIQUE.md`, `CST-0113`) n'est pas porté : le microbanc
  reproduit la v11 à l'identique ($S^{*}$ lexicographique en `SiteIdx`).
