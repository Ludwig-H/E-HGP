# Contre-vérification adverse : bibliothèque produit `morsehgp3d/`

4 octobre 2026, 13 h 13 UTC (`date -u`). Contre-vérificateur de la fouille
[`../fouille/produit.md`](../fouille/produit.md). Contexte : [`../CONTEXTE.md`](../CONTEXTE.md). Contrat rappelé par
l'utilisateur : **100 ms** sur G4 à K = 5 (le jalon de 200 ms n'est pas le but).

```text
phase=exploration_v11_hors_registre (contre-vérification, lecture seule)
backend=cpu_reference
profile=quantized_u21_input_only
public_status=not_claimed
GCP non utilisé ; aucune construction ni exécution native ; aucune commande git qui écrit
```

Base lue : `origin/main` = `17514012b` (worktree `build/v11-claude-20261003`), postérieur de dix minutes à la fouille
(12 h 56). **Fait nouveau qui change la lecture de la première idée** : le commit `17514012b` (13 h 06) apporte la
réponse de l'auditeur aux sept doctrines de vitesse, dont la doctrine 7 « GPU »
(`morsehgp3D_v11/audits/AUDIT_CONTRATS_NUMERIQUES_ET_CAPACITE_20261002.md` l. 127–140).

Étiquettes : **M** mesuré (reçu nommé), **L** établi par lecture, **E** estimé, **C** conjecturé.

## Synthèse

| id | verdict | en une ligne |
| --- | --- | --- |
| produit-gpu-lecons | **garder, réduite et corrigée** | chiffres G4 exacts ; la doctrine 7 de l'auditeur couvre déjà G1 et G3 ; restent quatre compléments (taux de replis CPU compté, count–scan–emit device, contexte chaud déclaré, lanceurs factices hostiles) ; deux erreurs de la fouille corrigées (i64/i128 faux, capacité « constante » fausse a priori) |
| produit-ci-v11 | **garder** | incident `claudequal1` vérifié, aucun workflow v10/v11, Clang jamais qualifié ; v11 sans Boost, CI faisable ; l'incident `claudepts5` n'aurait pas été attrapé |
| produit-empreintes-points | **garder** (priorité basse, hors contrat 100 ms) | lacune vérifiée mot pour mot ; aucun haché de la hiérarchie ni de la sortie plate ; permutation testée seulement sur 5 points côté HGP |

Aucune des trois ne gagne une milliseconde vers les 100 ms. Elles évitent des pertes (sessions G4, conception GPU) ou
qualifient un futur port natif.

---

## 1. produit-gpu-lecons

**(2) La preuve existe-t-elle et dit-elle ce qu'on lui fait dire ?** Oui pour les chiffres, relus un par un.

- [RJ] `docs/validation/phase15_rng_jung_g4_20260808/RESULTATS.md` § 3 : `frontier_ns` 2 395,883 ms, total froid
  3 927,585 ms, 40 lancements / 66 synchronisations, 13 reprises sur capacité de 640 candidats par ancre, arène
  1 408,926 Mo allouée dans le premier `advance`, ≈ 17,198 Go remis à zéro, 26 `cudaMemGetInfo`, enveloppe froide non
  ventilée 1 422,344 ms « plausible et probablement dominante » (sans mesure séparée). « Un passage qualifié
  antérieur au même rang ajoutait environ 8,447 s de recertification CPU » (§ 3, dernier paragraphe). § 6 : « fusionner
  proposition, classification exacte bornée et consommateur afin de ne jamais copier puis recertifier ». **L, M**.
- [WF] relu par script sur les JSON (`rank_results[0]`) : `cpu_recertification_ns` 4 800,6 / 5 377,5 / 5 330,9 ms,
  `launcher_ns` 1 224,4 / 1 053,5 / 1 004,6 ms, `qualification_output_copy_ns` 971,7 / 983,0 / 1 001,4 ms,
  `build_ns` 18,2 / 18,5 / 18,1 ms, `total_ns` 7 054,2 / 7 473,8 / 7 397,4 ms, 4 500 332 records. **M**. Correction
  de citation : les fichiers s'appellent `phase15_device_frontier_50k_kmax5_warm_g4_f39ab07.json`,
  `…_warm_cache_g4_a444de9.json` et `…_warm_occupancy_g4_4bf3cc0.json` (le motif `warm_g4_{…}` de la fouille n'en
  désigne qu'un).
- [P2B] `docs/validation/PHASE2B_PROGRESS.md` l. 139–153 : p50 107,27 / 158,77 / 180,77 ms pour 65 536 cas ;
  677 195 replis sur 2 031 616 entrées = 33,3 %. **Nuance que la fouille omet** : « les 677 195 replis sont aussi
  677 195 zéros exacts » ; le corpus est construit avec un tiers de cas dégénérés. Le taux d'un tiers est donc un
  artefact du corpus adverse, pas un taux naturel ; ce chiffre montre le coût par cas du repli, pas sa fréquence.
- Limbs : `phase15_exact_higher_support_product_fixed.cuh` l. 15–22 : 16 limbs, 1 024 bits, coordonnées alignées
  ≤ 124 bits. **L**. Lanceurs factices : `morsehgp3d/tests/cuda/fake_gpu_*_launchers.cpp` existent ; [GPU] § 11
  étape 1 « fake-launcher hostile ». **L**. Pistes fermées [AB] l. 12–13 conformes.

**(1) Déjà dans la v11 ?** En grande partie depuis `17514012b`. La doctrine 7 de l'auditeur exige : exact sur le
device, ou `unresolved` repris exactement sur CPU **avant admission** ; un débordement n'est jamais un rejet ;
sentinelles, baux/epochs, refus transactionnels ; budget **host/pinned/device** et leurs coexistences ; vrai nvcc,
portes CPU/device puis FULL identique ; chronométrer préparation, transferts, retour, canonicalisation. Cela couvre G1
(décision exacte), G3 (epochs, budget) et l'essentiel de G5 (mémoire épinglée). La v6 (C6) avait déjà ses portes
« stub » et son anneau de lots épinglés (contre-vérification v6-I5, `verif/v6.md` l. 214–230). Aucun code device en
v11 (`git ls-tree origin/main morsehgp3D_v11` : aucun `.cu`). **L**.

**Deux erreurs de la fouille.**

1. **G1 « u21 : i64/i128, Wide en file rare » est faux.** L'auditeur : « Centres i128 ne signifient pas catalogue
   i128 : q3 conserve checked/Wide, puissance 134/152 bits et niveaux jusqu'à 180/134 ou 204/152 en u21/u24 »
   (doctrine 7, l. 127–129). Un port device des feuilles doit porter une arithmétique de 134 à 204 bits pour la
   puissance et les niveaux, pas seulement i128. La mise en garde contre les 1 024 bits du produit reste juste.
2. **G2 « capacité constante, aucune reprise » est faux a priori.** Dans `src/catalogue/boxes.cpp` l. 143–167, une
   boîte n'est plus coupée si `count <= leaf_size` **ou** `width <= 1` ; une feuille peut alors compter jusqu'à
   `max_leaf` sites (256 dans le réglage mesuré, plafond 1 024, `catalogue.hpp` l. 27–28) avant le refus `wide_leaf`.
   « Au plus 16 sites » est un fait mesuré sur trois trames (`max_leaf` observé = 16, CARTE_V11 § 1), pas une borne.
   À 256 sites la borne combinatoire devient $\binom{256}{4}\approx 1{,}7\cdot 10^{8}$ ; en outre chaque boule émise
   porte $p+m$ incidences variables (`leaf.cpp` l. 199–203). La capacité doit donc être **comptée** (count–scan–emit
   en deux passes, ou repli hôte des feuilles larges), non supposée constante. L'arithmétique
   $\binom{16}{2}+\binom{16}{3}+\binom{16}{4}=2500$ est juste, mais ne vaut que pour `leaf_size` = 16 et `width > 1`.

**(3) Gain pour la v11 à K = 5 sur LiDAR sans sol.** 0 ms par lui-même. Le seul étage assez gros pour un device est la
passe unique (167–180 ms à W48, 4,75 s CPU à W1, CARTE_V11 § 2.1–2.2, **M**) ; ce qu'un device en ferait reste **C**.
Les échecs du produit ont été mesurés sur une architecture (paires ancrées Morton–Yao48) étrangère aux boîtes de
centres v11 ; leur valeur est d'ordre de grandeur : un étage device qui recopie puis recertifie, ou que l'hôte pilote
tuile par tuile, coûte des secondes, pas des millisecondes, sur la même machine.

**(4) Doctrine.** Compatible : décision exacte, sorties identiques octet pour octet, budget mémoire. Le passage à
`backend=cuda_g4` reste une déclaration explicite. **(5) Piste fermée ?** Les architectures du produit, oui
([AB] l. 12–13) ; un catalogue device v11, non.

**Verdict : garder, réduite à quatre compléments de la doctrine 7** (ce que l'auditeur n'a pas écrit) :

- C1 (de G1) : le taux de replis `unresolved` vers le CPU est un **compteur publié et borné par une porte** ; la
  mesure [WF] (4,8–5,4 s de recertification sur 7,0–7,5 s) montre qu'un repli de masse détruit l'étage ; le corpus
  [P2B] donne le coût unitaire d'un repli (≈ 50 ns par cas de lot, 65 536 cas en ≈ 107 ms, contexte chaud).
- C2 (de G2) : capacités **comptées sur le device** (count–scan–emit), sans retour hôte par tuile ; feuilles larges
  (`width <= 1`, plus de `leaf_size` sites) traitées à part et comptées.
- C3 (de G4) : contexte CUDA et processus chauds ; l'initialisation (≈ 1,4 s froide dans [RJ], non ventilée) est
  hors du chronomètre FULL **et déclarée** comme telle, jamais cachée.
- C4 (de G6) : lanceurs factices hostiles (capacité épuisée, lot vide, ordre permuté, `unresolved` injecté) pour
  tester l'orchestration sans GPU, avant toute session payante.

Coût : nul (texte). Mesure qui tranchera le gain lui-même, si la voie est ouverte : prototype device de
`enumerate_leaf` sur G4, chronométré bout en bout (préparation, H2D, noyau, D2H, canonicalisation), contre la passe
unique CPU à W48 sur ng00/ng01/ng02, sorties identiques.

---

## 2. produit-ci-v11

**(2) Preuve.** Vérifiée.

- Message de `eb036dbe2` : « the first qualification of P1/P2 (session claudequal1) failed only on this gate
  (ModuleNotFoundError), which also left the tower mutant campaign without a green witness ». Confirmé par
  `2b1abb6a5` (reçu `qualification_p1p2`) : `claudequal1` n'a échoué que sur la porte de largeur important numpy ;
  `claudequal2` conforme (686/686, 338/338 mutants). **M**. Coût réel : une session G4 complète perdue.
- `HIERARCHIE_POINTS.md` (tableau des sessions) : `claudepts5` « arrêtée après sa boucle principale : le dossier
  d'export des mutants n'était pas créé ». **M**. **Mais** cette campagne lit des données LiDAR et des exports de
  points ; une CI sans données (la fouille l'exclut elle-même) ne l'aurait **pas** attrapée. Un seul des deux incidents
  cités est évitable par la CI proposée.
- `DEVELOPPEMENT.md` « Résultats clos » : « Clang absent » ; `ARCHITECTURE.md` règle 11 : « GCC et Clang ». **L**.
- `.github/workflows/` sur `origin/main` : `ci.yml`, `gcp.yml`, `hgp-szeged-presentation.yml`, v7, v8 (audit LiDAR),
  v9 ; ni v10 ni v11. **L**.

**(1) Déjà dans la v11 ?** Non. `cmake/gates.cmake` déclare bien les labels `fast`/`long` et les jumelles `_opt` sous
`python3 -O` (l. 22–25, 40–41), mais rien ne rejoue les portes sous un Python nu hors G4 ; la discipline repose sur la
mémoire de l'assistant. Faisabilité vérifiée : `CMakeLists.txt` v11 ne demande que `Threads` et `Python3 3.10`
(l. 148, 251), pas Boost ; Clang est disponible sur les exécuteurs GitHub. **L**.

**(3) Gain.** 0 ms. Évite une session G4 perdue par faute de portabilité (une mesurée en deux jours, pas deux) et fait
respecter la règle 11. L'A/B de performance Clang est **C** (les variantes `-march` étaient dans le bruit, CARTE_V11
§ 5.3) et ne justifie pas l'idée.

**(4) Doctrine.** Neutre : CPU seulement, aucune donnée, aucune VM, `public_status=not_claimed` ; le contrôle
`tools/check_gcp_workflows.py` garde la CI hors GCP. **(5)** Non fermée.

**Verdict : garder.** Restriction : la valeur sûre est (c) Python 3.10 nu sans numpy sur les portes `fast` et (b)
Clang ; (a) GCC duplique ce que la qualification G4 fait déjà ; (d) ASan est facultatif. Durée sur un exécuteur à deux
cœurs à mesurer au premier passage (666 portes `fast` sur G4) ; si elle dépasse le budget GitHub, ne garder que les
portes Python et la compilation Clang.

---

## 3. produit-empreintes-points

**(2) Preuve du besoin.** Vérifiée mot pour mot : `receipts/pts4_review_20261003/README.md` l. 22–23 « Aucun dump
complet des dates/propriétaires ou FULL canonique ne prouve toutes les décisions internes » ; `HIERARCHIE_POINTS.md`
sous le tableau des sessions : « aucun dump canonique de toutes les dates et de tous les propriétaires internes ne
l'établit ». Côté produit : numérotation canonique d'un lot par tri (enfants, puis sources) dans
`point_hierarchy.cpp` l. 910–943 ; rejeu des invariants l. 1990 et suivantes ; 2,83 ms de rejeu et 144,3 ms de
scellement à 50 000 points (`docs/PERFORMANCE_MORSEHGP3D.md`, mesure **locale** du 3 août, deux CPU logiques). La cause
« texte décimal » des 144 ms est plausible (`builder.update(std::to_string(…))`, l. 275–277) mais non isolée par une
mesure. **L, M local**.

**(1) Déjà dans la v11 ?** Partiellement, et la fouille le dit correctement :

- `bench/points_flat.py` : `PointTree.finish` contrôle « chaque site entre une fois, enfants avant parents »
  (l. 389–404) ; `labels` canonise par plus petit `PointId` (l. 868–888) ; `U = 2.0 ** -53` (l. 42).
- `bench/points_flat_campaign.py` l. 172–173 (`tree_to_labels_equal`, côté sklearn) et l. 229–240 (D2, D5).
- Les `sha256` présents dans `bench/points_*.py` portent sur les **entrées** (sites, labels de vérité, binaires,
  générateurs), jamais sur l'arbre de points ni sur la sortie plate (`git grep` sur `origin/main`). **L**.
- Permutation de l'entrée : à l'échelle seulement pour le fit sklearn (`R0p`, `points_flat_campaign.py` l. 182–186) ;
  côté HGP, seulement la fixture de cinq points sous 120 permutations (`points_flat_gate.py` l. 252–265). **L**.
- `tower_point_tree` numérote les blocs dans l'ordre du balayage (l. 462 et suivantes) : une empreinte brute
  dépendrait de cet ordre ; il faut la canonisation par plateau du produit. **L**.

**(3) Gain.** 0 ms, hors du contrat de 100 ms (qui porte sur FULL). Utile au port natif de `points`/`head`, ouvert
mais non engagé : différentiel exact Python ↔ natif sur trames entières, identité W1/W48, équivariance par
permutation à 30–60 k sites. Coût faible (O(n + blocs) par k, ≈ 100–150 lignes, **E**).

**(4) Doctrine.** Conforme si les niveaux hachés sont exacts (fractions réduites) et si les exports à niveaux
flottants bornés sont marqués non canoniques ; une égalité d'empreintes ne remplace ni l'oracle ni les mutants. La
remarque $u=2^{-52}$ pour le port C++ sous tout mode d'arrondi est juste (Python arrondit toujours au plus proche).
**(5)** Non fermée.

**Verdict : garder, priorité basse** : à faire avec l'ouverture du port natif de la tête, pas avant les leviers
FULL. Ajouter dès maintenant le seul élément qui ne coûte rien et qui manque à l'échelle : un test de permutation de
l'entrée sur l'arbre de points HGP d'une trame entière (labels canoniques et nombre de blocs par plateau).

---

## Limites

- Non vérifié : la durée de la suite `fast` sur un exécuteur GitHub ; l'acceptation par nvcc 12.9 d'une arithmétique
  device à 134–204 bits sans spill ; les chiffres v6 (7 ns par boule, 2,16 Go en 75–79 ms) repris de l'auditeur v6 et
  revérifiés par son contre-vérificateur (`verif/v6.md` l. 214–220), pas ici.
- Aucune mesure nouvelle ; aucune transposition n'a de gain en millisecondes établi.

FIN
