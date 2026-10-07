# Fouille de la bibliothèque produit `morsehgp3d/` : ce qui se transpose dans la v11

4 octobre 2026, 12 h 56 UTC (heure lue par `date -u`). Auditeur de la source « bibliothèque produit ». Contexte
commun : [`../CONTEXTE.md`](../CONTEXTE.md) ; cartes lues d'abord : [`../cartes/CARTE_V11.md`](../cartes/CARTE_V11.md)
et [`../cartes/CARTE_V10_VITESSE.md`](../cartes/CARTE_V10_VITESSE.md).

```text
phase=exploration_v11_hors_registre (audit des transpositions, lecture seule)
backend=cpu_reference
profile=quantized_u21_input_only
public_status=not_claimed
GCP non utilisé ; aucune construction ni exécution native ; aucune commande git qui écrit
```

Rappel du contrat (demande de l'utilisateur) : tour HGP FULL des trames SemanticKITTI sans sol, grille 1 mm, moteur
entier exact, **100 ms sur G4 à K = 5**, si possible K = 10. Le jalon de 200 ms n'est pas le but.

Étiquettes : **M** mesuré (reçu nommé, G4 sauf mention « local »), **E** estimé (arithmétique sur des mesures,
méthode donnée), **C** conjecturé, **L** établi par lecture du code ou des documents.

## Sources

Les fichiers de `morsehgp3d/`, `docs/PERFORMANCE_MORSEHGP3D.md`, `docs/GPU_G4_ARCHITECTURE.md`,
`docs/math/HIERARCHIE_DE_POINTS_MULTI_ORDRES.md`, `docs/validation/` et `.github/workflows/` sont identiques entre
l'extraction principale (`origin/main` = `63a91d2f1`) et `origin/main` = `eb036dbe2` du worktree v11 (`git diff --stat`
vide). Le dernier commit qui touche `morsehgp3d/` est **`95dd8036a` du 9 août 2026** : la ligne produit est dormante
depuis deux mois.

| Abréviation | Chemin (relatif à `/workspaces/E-HGP/` sauf mention) |
| --- | --- |
| [PH] | `morsehgp3d/src/cpu/api/point_hierarchy.cpp` (2 182 lignes) |
| [PHH] | `morsehgp3d/include/morsehgp3d/api/point_hierarchy.hpp` (272 lignes) |
| [CID] | `morsehgp3d/include/morsehgp3d/contract/canonical_id.hpp` |
| [FP] | `morsehgp3d/include/morsehgp3d/exact/fp64_interval.hpp`, `expansion.hpp`, `predicate.hpp` |
| [MO] | `docs/math/HIERARCHIE_DE_POINTS_MULTI_ORDRES.md` (spécification du réducteur) |
| [PERF] | `docs/PERFORMANCE_MORSEHGP3D.md` |
| [GPU] | `docs/GPU_G4_ARCHITECTURE.md` |
| [RJ] | `docs/validation/phase15_rng_jung_g4_20260808/RESULTATS.md` (diagnostic G4 du 8 août) |
| [WF] | `docs/validation/phase15_device_frontier_50k_kmax5_warm_g4_{f39ab07,a444de9,4bf3cc0}.json` |
| [P2B] | `docs/validation/PHASE2B_PROGRESS.md` (qualification `warm_context_e2e`) |
| [AB] | `docs/archive/abandoned/README.md` (pistes abandonnées) |
| [REG] | `docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md` (sur `origin/main` `eb036dbe2`) |
| [CI], [CI9] | `.github/workflows/ci.yml`, `.github/workflows/morsehgp3d-v9.yml` |
| [HP], [SP], [DEV], [ARCH] | `morsehgp3D_v11/docs/HIERARCHIE_POINTS.md`, `SORTIE_PLATE.md`, `DEVELOPPEMENT.md`, `ARCHITECTURE.md` |
| [Q7] | `morsehgp3D_v11/audits/QUESTION_CLAUDE_VITESSE_100MS_20261004.md` (sept verrous avant les leviers 100 ms) |
| [PTS4] | `morsehgp3D_v11/receipts/pts4_review_20261003/README.md` |
| [CL2], [CV10] | `morsehgp3D_v10/docs/conception/CLUSTER_v2.md`, `CONCEPTION_V10.md` |
| [ZC] | `Zoltan/FoundationModel/CONTRAT_COUPES_ET_MASSES_20260926.md` |
| [V6] | `build/v11-persist/audit_transpositions/fouille/v6.md` (lecture de l'auditeur v6, citée telle quelle) |
| [CARTE_V11], [CARTE_V10] | `build/v11-persist/audit_transpositions/cartes/CARTE_V11.md`, `CARTE_V10_VITESSE.md` |

---

## 0. Verdict

1. **Vers les 100 ms sur CPU, la bibliothèque produit n'apporte aucun levier.** Elle n'a jamais construit la tour
   depuis un nuage brut (README l. 3 ; [PERF] l. 5) ; sa seule tentative 50 000 points K = 10 `reference_cpu` est
   censurée au-delà de 300 s ([PERF] l. 165, **M**) ; son code CPU est mono-fil, en `cpp_int`, `std::map` et
   `std::set` (**L**, [PH]) ; son unique chiffre sous 100 ms est le point-MST (p95 95,8 ms à 50 000 points sur G4),
   surrogate qui calcule un autre objet et piste fermée ([PERF] l. 166 et 189 ; [AB] l. 8).
2. **Sa ligne GPU, mesurée sur la même G4, est la mise en garde la plus coûteuse du dépôt** : frontière de paires
   seule 2 396 ms à 50 000 points et K = 10 ([RJ] l. 31) ; frontière chaude à Kmax = 5 : 7,05–7,47 s dont 4,8–5,4 s de
   recertification CPU ([WF]) ; prédicats GPU en contexte chaud : 107–181 ms pour 65 536 cas ([P2B] l. 147–151). Ces
   échecs mesurés donnent des exigences précises pour le verrou 7 de la v11 (« GPU », [Q7] l. 49–53) : idée
   **produit-gpu-lecons**.
3. Deux outils de test manquent à la v11 et le produit en montre la forme : une **CI GitHub** (GCC + Clang + portes
   `fast` + Python nu ; idée **produit-ci-v11**) et une **chaîne d'empreintes canoniques** tour → hiérarchie de points
   → sortie plate avec rejeu O(n) des invariants (idée **produit-empreintes-points**).
4. **Pour la sortie plate**, le réducteur n'apporte rien à la mathématique de la sélection : la tête v11 est en avance
   (plateaux N-aires avec entrées, critère A, sommes de radicaux certifiées), son routage par vote est rejeté par la
   v11, et son arbre multi-ordres n'a jamais été mesuré tandis que deux têtes multi-ordres voisines ont perdu en
   développement v10 (§ 3). Il apporte la forme du contrat (hiérarchie immuable, reçus liés, invariants).
5. Douze fausses bonnes idées sont écartées au § 4.

---

## 1. La source en une page

| Composant | Où | Ce que c'est | Mesures |
| --- | --- | --- | --- |
| Réducteur public | [PHH], [PH], spécification [MO] | arbre de fusion de la tour multi-ordres ordonnée par $\lambda=k/r^{z}$ (arêtes verticales activées au niveau de l'ordre inférieur, [MO] § 3–5), routage descendant irréversible par vote $S_\tau/T_x$ ([PH] l. 1170–1390), rendus λ-coupe, DBSCAN par rayon, EOM ([PH] l. 1756–1988), reçus liés ([PH] l. 270–390, 1516–1580, 1637–1667), rejeu des invariants ([PH] l. 1990–2042) | 50 000 points, tour synthétique d'ordre 1, local (2 CPU logiques, 3 août) : construction 2 792,5 ms, EOM 16,7 ms, DBSCAN 2,2 ms, rejeu des invariants 2,8 ms, scellement canonique 144,3 ms, RSS 124,7 Mio ([PERF] l. 27–52, **M** local) |
| Flottant certifié | [FP] | cascade fp32 → intervalle fp64 → expansions (`std::vector`) → `cpp_int` ; exige l'arrondi au plus proche et un MXCSR sans FTZ/DAZ, vérifiés à l'exécution ([FP] `fp64_interval.hpp` l. 22–47) | qualification CPU 2A/2B ; aucune mesure de vitesse comparable à la v11 |
| GPU (CUDA, `sm_120`) | `morsehgp3d/src/cuda/` (60 fichiers), [GPU] | frontière Morton–Yao48 de paires, supports 3–4 tuilés, prédicats à limbs fixes jusqu'à 1 024 bits (`phase15_exact_higher_support_product_fixed.cuh` l. 18–21), lanceurs factices hostiles (`morsehgp3d/tests/cuda/fake_gpu_*_launchers.cpp`) | tableau ci-dessous |
| Contrats | [CID], `schemas/morsehgp3d-contract-v{1,2}.schema.json` | SHA-256 incrémental à séparateurs de domaine `MorseHGP3D/v1|v2/…` ([CID] l. 39–71) | — |
| CI | [CI] l. 28–59 | GCC Release + test du paquet installé, Clang Release, ASan/UBSan Debug | — |

Mesures G4 de la ligne GPU (toutes `component_only` / `profile_only`, aucune revendication) :

| Mesure | Valeur | Source |
| --- | --- | --- |
| Frontière de paires device, 50 000 points, rang fermé 11 | `frontier_ns` 2 395,9 ms ; total froid 3 927,6 ms ; 40 lancements, 66 synchronisations ; 13 reprises de tuile ; arène de 1,41 Go allouée dans la fenêtre ; ≈ 17,2 Go de capacité remise à zéro | [RJ] § 3, l. 22–50 |
| Frontière device chaude, 50 000 points, Kmax = 5, trois sessions | total 7 054 / 7 474 / 7 397 ms ; lanceur 1 224 / 1 053 / 1 005 ms ; copie de sortie 972 / 983 / 1 001 ms ; **recertification CPU 4 801 / 5 377 / 5 331 ms** ; LBVH 18,1–18,5 ms ; 4 500 332 candidats | [WF], champs `rank_results[0]` |
| Prédicats GPU, contexte chaud, lots de 65 536 cas | p50 107,3 / 158,8 / 180,8 ms (distance, orientation, puissance) ; un tiers des cas renvoyé au CPU | [P2B] l. 139–153 |

---

## 2. Idées retenues

### produit-gpu-lecons — exigences d'un port GPU des feuilles du catalogue, tirées des échecs mesurés du produit

**Catégorie** : architecture (vitesse conditionnelle). **Statut** : **M** pour les échecs ; tout gain positif reste **C**.

**Source.** [RJ] § 1 (l. 6–15), § 3 (l. 22–50) et § 6 (l. 89–93) ; [WF] ; [P2B] l. 139–153 ; [GPU] § 6 (l. 93–103),
§ 10 (l. 153–157) et § 11 étape 1 (l. 172, « fake-launcher hostile ») ; `morsehgp3d/tests/cuda/fake_gpu_*_launchers.cpp` ;
`morsehgp3d/src/cuda/phase15_exact_higher_support_product_fixed.cuh` l. 18–21.

**Contexte v11.** Le développeur demande, avant tout port GPU des feuilles du catalogue, « arithmétique exacte sur le
device, sentinelles, identité octet pour octet avec le CPU, budget mémoire device » ([Q7] l. 49–53), avec la seule
leçon v6 « l'hôte avait mangé le gain ». L'auditeur v6 couvre ce mode d'échec hôte ([V6] l. 262–290, idée v6-I5). Le
produit en documente **cinq autres**, chacun mesuré :

| # | Exigence pour la v11 | Échec mesuré dans le produit |
| --- | --- | --- |
| G1 | **Décider exactement sur le device**, dans les budgets `constexpr` de la v11 (u21 : i64/i128 ; repli `Wide` en file rare) ; jamais « proposition flottante GPU puis recertification CPU » | recertification CPU 4,8–5,4 s sur 7,0–7,5 s ([WF]) ; un passage qualifié antérieur ajoutait 8,447 s de recertification ([RJ] l. 50) ; prédicats chauds 107–181 ms pour 65 536 cas avec un tiers de replis CPU ([P2B] l. 147–151) ; le produit conclut lui-même « fusionner proposition, classification exacte bornée et consommateur afin de ne jamais copier puis recertifier » ([RJ] l. 91) |
| G2 | **Capacités de sortie prouvées a priori** par tâche, puis `count → scan → emit` sur le device ; aucun pilotage hôte par tuile | capacité de 640 candidats par ancre : 13 reprises, 40 lancements, 66 synchronisations ([RJ] l. 36, 42–44). Pour la v11, une feuille compte au plus 16 sites (mesuré, [CARTE_V11] l. 65), donc au plus $\binom{16}{2}+\binom{16}{3}+\binom{16}{4}=2500$ candidats par feuille (**E**, borne combinatoire) : la capacité par feuille est une constante, aucune reprise n'est nécessaire |
| G3 | **Arènes persistantes** réservées une fois et comptées dans le `MemoryBudget`, époques au lieu de remises à zéro, aucune télémétrie dans le chemin chaud | arène de 1,41 Go allouée dans `frontier_ns`, ≈ 17,2 Go remis à zéro sur 13 tuiles, 26 appels `cudaMemGetInfo` dans la fenêtre ([RJ] l. 38, 44) |
| G4 | **Contexte et processus chauds** ; chronométrer `warm_e2e` comme le fait [GPU] § 10 | enveloppe froide non instrumentée de 1 422 ms, initialisation du contexte « plausible et probablement dominante » ([RJ] l. 46) |
| G5 | **Sortie compacte** (SoA, mémoire épinglée, records denses), répartition du travail dans le warp | copie de sortie ≈ 1,0 s pour 4,5 M records ([WF]) alors que la v6 copiait 2,16 Go en 75–79 ms ([V6] l. 266–267) : c'est la forme des records, pas le bus ; décisions concentrées sur la lane zéro ([RJ] l. 40) |
| G6 | **Tester l'orchestration sans GPU** par des lanceurs factices hostiles (capacités épuisées, lots vides, ordres permutés) | patron du produit : `morsehgp3d/tests/cuda/fake_gpu_*_launchers.cpp`, [GPU] l. 172 ; la v6 a ses « portes stub » ([V6] l. 272) |

À ne **pas** reprendre : l'arithmétique device à 16 limbs (1 024 bits) du produit, dimensionnée pour des coordonnées
binary64 arbitraires alignées sur 124 bits (`…_fixed.cuh` l. 18–21) ; la v11 u21 tient en i64/i128 par ses budgets, et
la v6 aurait mesuré l'exact `__int128` sur le device à ≈ 7 ns par boule ([V6] l. 264–265, lecture d'un autre auditeur,
non revérifiée ici).

**Dans la v11 ?** Non : aucun code device ; verrou 7 ouvert, en attente de la synthèse de cet audit ([Q7] l. 49–53,
dernière phrase). **Doctrine** : compatible et même exigée (F1 « aucune décision en flottant », sorties identiques
octet pour octet CPU/GPU, budget mémoire) ; changer de `backend` (`cuda_g4`) demande une déclaration explicite ; feu
vert de l'utilisateur sur G4 ([Q7] l. 49). **Piste fermée ?** Les architectures du produit le sont (subdivision
`prune-only` pilotée par l'hôte, parcours relancé par paire : [AB] l. 12–13) ; un catalogue device v11 ne l'est pas.

**Gain.** Aucun gain en millisecondes par lui-même. Il évite de reproduire une conception mesurée à 2,4–7,5 s pour un
seul étage sur la même machine. Base d'un gain éventuel : la passe unique vaut 4,75 s de CPU à W1 et 167–180 ms à W48
sur ng00 ([CARTE_V11] § 2.1–2.2, **M**) ; ses 353 456 feuilles sont indépendantes ([CARTE_V11] § 2.5, **M**) ; ce qu'un
device en ferait reste **C**. **Coût** : la liste est gratuite ; le port lui-même est élevé (DFS des boîtes irrégulier,
filtre G1 à 379 M tests par trame). **Intérêt** : élevé si la voie GPU est ouverte, nul sinon.

### produit-ci-v11 — une CI GitHub pour la v11 : GCC et Clang, portes `fast`, Python nu

**Catégorie** : outillage_tests. **Statut** : **M** pour les incidents évitables ; gain **E**.

**Source.** [CI] l. 28–59 (produit : GCC Release puis test du paquet installé, Clang Release, ASan/UBSan Debug) ;
[CI9] (v9 : construction, `check_docs`, `ctest -L '^gate$'`, auto-tests hors ligne du protocole G4 sous `python3 -B`
et `python3 -B -O`, déclaration « CPU gates only; public_status=not_claimed »).

**Mécanisme.** Un workflow `morsehgp3d-v11.yml` déclenché sur `morsehgp3D_v11/**` : (a) GCC Release u21,
`-Werror`, `ctest -L fast` (les jumelles `_opt` sous `python3 -O` sont déjà déclarées par `cmake/gates.cmake`) ;
(b) **Clang Release u21** et les portes `unit` ; (c) chaque porte Python enregistrée rejouée sous un **Python 3.10
nu, sans numpy** — la condition exacte de la matrice G4 ; (d) facultatif : ASan/UBSan sur les portes `fast` de
`num`, `index`, `tower`, et compilation seule en u18/u24. Jamais de GCP (déjà contrôlé par
`tools/check_gcp_workflows.py`), jamais de données LiDAR, aucune qualification.

**Preuve.** Session `claudequal1` (4 octobre) : « failed only on this gate (ModuleNotFoundError), which also left the
tower mutant campaign without a green witness » (message du commit `eb036dbe2`, **M**) ; session `claudepts5` arrêtée
parce que « le dossier d'export des mutants n'était pas créé » ([HP] l. 242, **M**) ; qualification v11 « Clang
absent » ([DEV] l. 39) alors que la règle 11 exige « GCC et Clang » ([ARCH] l. 31) ; aucun workflow v10 ni v11 dans
`.github/workflows/` (v7, v8, v9 seulement, **L**).

**Dans la v11 ?** Non. La discipline « Python nu » n'existe que par la correction du 4 octobre et la mémoire de
l'assistant. **Doctrine** : neutre. **Piste fermée** : non.

**Gain.** 0 ms. Il épargne des sessions G4 perdues avant même le premier chronomètre (deux mesurées en deux jours) et
fait respecter la règle GCC + Clang. **C** : la compilation Clang offre aussi un A/B de performance bon marché sur la
passe unique (non mesuré ; les variantes `-march` étaient dans le bruit, [CARTE_V11] § 5.3). **Coût** : faible
(≈ 80 lignes YAML) ; durée sur un exécuteur à deux cœurs inconnue (la suite `fast` compte 666 portes sur G4,
[CARTE_V11] l. 25). **Intérêt** : moyen.

### produit-empreintes-points — empreintes canoniques chaînées tour → hiérarchie de points → sortie plate

**Catégorie** : outillage_tests (hiérarchie de points et sortie plate). **Statut** : lacune **L** (relevée par
l'auditeur) ; gain **E**.

**Source.** [PH] l. 270–390 (identifiant canonique du payload, invariant par permutation : tri par identifiants),
l. 910–943 (numérotation canonique des nouveaux nœuds d'un lot de même niveau : tri par enfants puis par sources,
indépendante de l'ordre de traitement), l. 1516–1580 (reçu de réduction : chaque nœud avec niveau, enfants, sources,
puis chaque point → terminal), l. 1637–1667 (reçu de sélection : méthode, mcs, paramètres, nœuds retenus),
l. 1990–2042 (rejeu O(n) des invariants : un terminal par point, comptes directs, intervalles DFS contigus, tailles de
sous-arbres) ; [PHH] l. 161–199 (`FlatClustering`, `ExactPointHierarchyReceipt`) ; [CID] l. 39–71. Coûts mesurés
(local, 50 000 points) : rejeu des invariants 2,8 ms, scellement en chaînes décimales 144,3 ms ([PERF] l. 41–47).

**Mécanisme pour la v11.** Par trame et par ordre k, une empreinte SHA-256 sur des **enregistrements binaires** (pas
de texte décimal, cause des 144 ms du produit), chaînée : empreinte du dump FULL (déjà calculée,
`bench/verify_full_captures.py` l. 50–58) → arbre de points de $H^{r}_{k+1}$ (niveaux exacts $(t,m,q)$ en fractions
réduites, blocs numérotés canoniquement à l'intérieur de chaque plateau, entrée de chaque site indexée par `PointId`)
→ sortie plate (règle, z, mcs, labels canoniques que `points_flat.labels` fournit déjà, l. 868–888). Plus le rejeu
O(n) des invariants : chaque site entre une fois, masse d'un bloc = somme de ses enfants et de ses entrées,
sélection = antichaîne.

**Preuve du besoin.** « Aucun dump complet des dates/propriétaires ou FULL canonique ne prouve toutes les décisions
internes » ([PTS4]) ; « aucun dump canonique de toutes les dates et de tous les propriétaires internes ne l'établit »
([HP] l. 245–248). Les sessions D et F ne sont « identiques » que sur les observables publiés.

**Dans la v11 ?** En partie : dumps FULL hachés ; labels canoniques ; invariants D2, D5 et `tree_to_labels` à
l'échelle (`bench/points_flat_campaign.py` l. 172–173, 229–240) ; contrôles de forme de `PointTree.finish`
(`bench/points_flat.py` l. 389–404) ; arbres exportés en `.npz` par `points_flat_dump.py`, sans empreinte comparée.
Absents : empreinte de la hiérarchie et de la sortie plate, numérotation canonique indépendante de l'ordre de
traitement (`tower_point_tree` numérote les blocs dans l'ordre du balayage, `points_flat.py` l. 462–540), test de
permutation des points à l'échelle.

**Usages.** (1) Différentiel exact Python ↔ port natif des modules `points` et `head` (prévus, [ARCH] § 2) sur les
trames entières, en complément de l'oracle des petits nuages ; (2) identité W1/W48 du futur natif ; (3) équivariance
par permutation de l'entrée à 30–60 k sites (propriété prouvée H2, [HP] l. 129, à tester sur l'implantation) ;
(4) comparaison de sessions sur tout l'état interne.

**Doctrine.** Niveaux exacts seulement ; les exports à niveaux flottants bornés (`FloatLevel`) sont marqués non
canoniques ; une égalité d'empreintes ne remplace ni l'oracle ni les mutants. Note pour le port natif : les bornes
flottantes de `points_flat.py` prennent `U = 2.0 ** -53` (l. 42), juste en Python où l'arrondi est toujours au plus
proche ; en C++ sous la doctrine F3, il faut $u=2^{-52}$ pour tout mode d'arrondi ([ARCH] § 4). **Piste fermée** :
non.

**Gain.** 0 ms ; rend exacts à l'échelle des différentiels aujourd'hui impossibles. **Coût** : faible (≈ 100–150
lignes Python, puis l'équivalent C++), O(n + blocs) par k. **Intérêt** : moyen ; c'est un préalable de qualification
du port natif de la tête, aujourd'hui ouvert.

---

## 3. Ce que le réducteur peut apporter à la sortie plate v11

| Aspect | Produit (`point_hierarchy`) | v11 (`points_radius.py`, `points_flat.py`) | Lecture |
| --- | --- | --- | --- |
| Projection sur les points | arbre multi-ordres $\lambda=k/r^{z}$, routage descendant figé par vote $S_\tau/T_x$ ([PH] l. 1170–1390 ; [MO] § 6–8) | $H^{r}_{k+1}=P_1\circ\Pi_{k+1}$ à k fixé, dates et hauteurs stables en $3\varepsilon$ ([HP] § 3) | la v11 est en avance ; le vote figé est rejeté ([HP] l. 183–195) |
| Plateaux | lot atomique par niveau exact ([MO] § 5 ; [PH] l. 842–965) | plateaux atomiques avec entrées ([SP] l. 19–25) | déjà dans la v11 |
| Masse de condensation | $m_x=1$ ([MO] § 11) | masses entières, critère A ([SP] l. 19–25) | déjà |
| Arithmétique EOM | intervalles rationnels par racine entière, symbole $\Lambda_\infty$ pour le rayon nul ([PH] l. 1463–1514) | sommes de radicaux, classes de carrés, encadrements entiers ([`points_flat.py`] l. 86–140) | la v11 est plus générale (dates à trois radicaux) ; $\Lambda_\infty$ inutile (mcs ≥ 2, k ≥ 2, doublons retirés) |
| Égalité EOM, racine | parent sur égalité ([PH] l. 1952) ; racine seule interdite par défaut ([PH] l. 1967) | parent sur égalité certifiée, racine exclue ([SP] l. 25) | identiques |
| λ-coupe, DBSCAN par rayon | oui ([PH] l. 1756–1814) | non | sans intérêt pour E1 (§ 4, n° 12) |
| Reçus liés, rejeu des invariants | oui | en partie | idée produit-empreintes-points |
| Vitesse | `cpp_int`, `std::map` : construction 2,79 s, EOM 16,7 ms à 50 000 points d'ordre 1 (local) | Python | aucun des deux n'est un modèle ; la v10 estimait une tête native à ≈ 3–5 ms à K5 ([CL2] § 11.1, **E**, autre règle) |

**Conclusion.** Le problème ouvert de la sortie plate v11 — « une règle sans vérité qui attrape les séparations
fugaces sans morceler » ([SP] l. 117–119) — n'est pas traité par le produit : son EOM est celui de la v11, et son arbre
multi-ordres n'est ni mesuré ni stable par un théorème connu (§ 4, n° 2). Le réducteur apporte la **forme du
contrat** (hiérarchie immuable, un terminal par point, intervalles DFS, labels tirés d'une antichaîne, reçus de
réduction et de sélection, budgets refusés avant calcul) au futur port natif de `points` et `head`, et c'est la partie
reçus et invariants qui a une valeur de test réelle (§ 2).

---

## 4. Fausses bonnes idées écartées

1. **Prendre le routage descendant par vote du produit comme projection v11.** Le vote figé est discontinu, dépend
   des faces construites et de l'exposant, et retourne la décision sur les deux triangles ([HP] l. 183–195) ; un
   routage glouton n'est pas un vote global (contre-exemple à trois masses 3/10, 3/10, 4/10, [ZC] l. 131) ; « Routage
   par votes » est déjà retiré en v10 ([CL2] l. 625).
2. **Arbre de fusion multi-ordres $\lambda=k/r^{z}$ (et son EOM) pour la synthèse des ordres (P6) ou les séparations
   fugaces.** Jamais mesuré ; deux têtes multi-ordres voisines ont perdu en développement v10 : EOM intégrée sur K,
   0,722–0,777 contre 0,795 (`measured_negative`, [CV10] l. 844, sonde `h2_proxy` à n = 2 000 sur `mreach`) et tranche
   γ T2 à $\lambda=k(s)/s^{z}$, −0,016 à −0,037 contre T1 ([CL2] l. 596–605) ; z entrerait dans la topologie ; aucun
   théorème de stabilité de la projection. Ne se rouvre qu'en recherche (ancrage $P_1$ sur cet arbre et preuve de
   stabilité), pas comme transposition.
3. **Porter la frontière GPU Morton–Yao48 ou le « catalogue GPU » du produit.** 2,4 s pour les seules paires
   ([RJ] l. 31), 7,0–7,5 s à chaud à Kmax = 5 ([WF]) ; architecture par paires ancrées étrangère aux boîtes de centres
   v11 ; ses variantes `prune-only` et relance par paire sont des pistes fermées ([AB] l. 12–13).
4. **Cascade flottante certifiée du produit pour les prédicats chauds v11.** Elle suppose l'arrondi au plus proche et
   un MXCSR sans FTZ/DAZ vérifiés à l'exécution ([FP] `fp64_interval.hpp` l. 22–47), contraire à F1–F6 « sous tout
   mode » ([ARCH] § 4) ; elle est plus lente (`volatile`, `nextafter` par opération, `std::vector` dans
   `expansion.hpp`) ; la v11 a mesuré qu'un filtre F6 statique ne gagne que ≈ 1 % du CPU ([CARTE_V11] § 5.3).
5. **Le point-MST, seul chiffre produit sous 100 ms** (p50 88,8 ms, p95 95,8 ms à 50 000 points, G4) : surrogate qui
   oublie les simplexes, facettes et cofaces dès l'ordre deux ; piste fermée ([AB] l. 8 ; [PERF] l. 189).
6. **Bâtir le port natif de la tête sur le code du réducteur.** 2,79 s à 50 000 points d'ordre 1 (local) ; tout est
   `cpp_int`, `std::map`, `std::set`, mono-fil ; ses plafonds par défaut (2 M nœuds, 4 M arêtes, 8 M simplexes) sont
   déjà sous des sorties v9 réelles (`Zoltan/FoundationModel/AUDIT_V9_ET_ARCHITECTURE_20260926.md` l. 33). On reprend
   la forme du contrat, pas le code.
7. **Arithmétique device à 1 024 bits du produit** : taillée pour du binary64 arbitraire ; la v11 u21 n'en a pas
   besoin (voir G1).
8. **Germination, théorème de Jung ou critère RNG-HGP pour élaguer les candidats v11.** Autre architecture (paires
   ancrées) mesurée hors contrat : 37 897 paires par seconde, arité quatre jamais démarrée ([RJ] § 4) ; Jung ne
   rejette aucun support bien centré (il le vérifie par théorème, `docs/math/OPTIMISATIONS_JUNG_SUPPORTS_3_4.md` § 0)
   et n'ajoute rien au test d'acuité d'une feuille ; les cascades RNG sont `false_in_general` ([REG] l. 75) ; un
   préfiltre RNG des cellules demanderait une table facettes → cellules et devrait coûter moins qu'un pas de descente
   (597–714 ns à W1, [CARTE_V10] § 3.4), rien de mesuré.
9. **Borůvka (`relative_morse_boruvka`) pour paralléliser la publication par ordre.** `architecture_only`, aucune
   mesure ; il rend un arbre couvrant, pas l'arbre de fusion à plateaux et parents ; la publication v11 est déjà
   recouverte par le pipeline (queue de 24–36 ms, [CARTE_V11] § 2.1).
10. **Budget d'ingénierie par étage** ([GPU] l. 157 : 25/45/20/10 ms) : outil de pilotage seulement ; la carte v11
    § 6 fait déjà ce découpage, avec des mesures.
11. **Schémas JSON v1/v2 du produit pour les reçus v11** : chaque reçu v11 a déjà son lecteur `check.py` rejouable ;
    gain faible.
12. **Rendu DBSCAN par rayon de référence** : un paramètre ε de plus ; ne traite ni les séparations fugaces ni le
    critère d'existence des clusters (P4, [HP] § 8).

---

## 5. Limites de cette fouille

- Lecture ciblée : API de points, exactitude, flottant certifié, GPU mesuré, CI, contrats ; les quelque 550 000
  lignes de recherche Phase 10–15 (`src/cpu/hierarchy/direct_*`, `src/gpu/`) n'ont été que parcourues par en-têtes et
  `grep` ; elles n'ont jamais produit de tour complète à 50 000 points ([PERF] l. 165).
- Non vérifié : la durée de la suite `fast` sur un exécuteur GitHub ; l'acceptation de `__int128` en code device par
  le CUDA 12.9 de la G4 (le produit utilise des limbs u64 ; l'usage v6 est une lecture de l'auditeur v6).
- Aucun chiffre de ce rapport n'est une mesure de la v11 sous une transposition : chacune exige ses portes, ses
  mutants et un A/B G4 à sorties identiques.

FIN
