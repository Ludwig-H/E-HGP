# Audit final de la v11

7 octobre 2026. Cadre :

```text
phase=exploration_v11_hors_registre (close le 7 octobre 2026)
backend=cpu_reference ; voie de banc cuda_g4 pour le lot de feuilles du catalogue
profile=quantized_u21_input_only
public_status=not_claimed
```

Demande de l'utilisateur (7 octobre 2026) : « On va organiser plutôt une passation et tout reconstruire à neuf pour
une v12 de Morse HGP 3D. Je veux que tu fasses un audit géant de cette v11 ; toutes les bonnes idées qui ont marché,
celles qui n'ont pas marché, etc. » Ce document est cet audit. La [passation](../PASSATION.md) en tire l'état exact au
gel, les décisions attendues et le plan proposé pour la v12.

## Méthode

- **Instantané lu** : `ac081a06f` (7 octobre, 03 h 43 UTC), dernier commit moteur de la v11.
- **Six lectures indépendantes en lecture seule**, une par domaine :
  - A : objet mathématique et exactitude numérique ;
  - B : catalogue (étage `domain`) et voie GPU ;
  - C : tour FULL et forêts ;
  - D : sorties, hiérarchies dérivées, comparaison à HDBSCAN ;
  - E : tests, portes, mutants, mesure et sessions G4 ;
  - F : canal d'audit et dialogue avec le développeur.

  Les six rapports bruts sont archivés dans `receipts/passation_20261007/rapports/`.
- **Autres sources** : les notes du développeur (registre des leviers et leçons des 5 au 7 octobre) et des
  vérifications ciblées dans le code, les reçus et l'historique Git. Le § 16 liste les corrections que ces
  vérifications ont apportées aux rapports.
- **Aucune compilation ni mesure nouvelle. GCP non utilisé.** Tous les temps viennent de reçus G4 existants.
- **Légende** : sans marque, un énoncé est vérifié dans le reçu, le code ou le commit cité ; **[I]** marque une
  inférence.

**Données.** Les trames sont les trois captures sans sol de la séquence SemanticKITTI 08 : 000000, 000100 et 000200,
notées ng00, ng01 et ng02, de 39 885, 35 551 et 45 845 sites, sur une grille de 1 mm. Elles ne sont pas dans le
dépôt. Les nuages uniformes de 8 000, 16 000 et 32 000 sites servent d'échelle synthétique.

**Machine G4.** VM `g4-standard-48` : AMD EPYC 9B45 (24 cœurs physiques × 2 SMT, trois L3 de 32 Mio) et RTX PRO
6000 Blackwell (sm_120). W48 désigne 48 fils.

**Régimes de mesure.** « À froid » : médiane de 6 processus neufs. « À chaud » : médiane des passes 2 à 10 d'un même
processus.

## 1. Verdict

1. **L'objet calculé est juste.**
   - Aucun résultat FULL faux n'a été établi en six jours : 126 sessions G4 au moins, dont 22 relues par les
     auditeurs, et aucun défaut d'exactitude du moteur.
   - La voie GPU a rendu les mêmes octets que le CPU sur 372 prises à froid et 84 à chaud.
   - Les empreintes FULL K5 des trois trames n'ont pas changé du 3 au 7 octobre, à travers toutes les optimisations
     (§ 3.4).
2. **Le contrat de temps n'est pas tenu.**
   - 100 ms : non.
   - Jalon de 200 ms : approché sur ng01 à chaud seulement (212 ms, meilleure passe 207 ms).
   - K10 : 1,3 à 1,8 s.
3. **La v11 n'a pas dépassé la v10 en vitesse** (comparaison entre captures distinctes, pas un A/B ; § 3.2).
   - À K5, sa voie GPU égale la v10 CPU, et sa voie CPU reste environ ×1,24 plus lente.
   - À K10, elle est ×1,5 à 1,6 plus lente.
   - Son catalogue GPU est plus rapide que celui de la v10 (×0,84 à 0,88 à K5, ×0,75 à K10). Ses forêts sont plus
     lentes : ×1,28 à 1,35 à K5, et ×2,6 à 2,8 à K10.
4. **Les mathématiques sont le meilleur acquis.**
   - Le contrat est démontré sans position générale.
   - Les témoins exacts sont gravés en fixtures.
   - L'oracle borné est indépendant.
   - Les lemmes courts sont devenus des gains moteur.
5. **Le socle d'ingénierie est solide et se réutilise** : portes à code exact, mutants causaux, session G4 gardée,
   budget mémoire transactionnel, dossier de sortie atomique, refus plutôt que dégradation.
6. **Les sorties sont livrées et partiellement qualifiées.** `supports` (arbre couvrant de Kruskal), `points`
   ($H^{r}_{K+1}$) et `plat` (EOM exacte) l'ont été les 5 et 6 octobre. La sélection plate reste le goulot de la
   comparaison à HDBSCAN, et la hiérarchie de points n'atteint ni ses cibles ni la stabilité par insertion.
7. **La voie GPU est exacte mais sous-employée** :
   - un fil par feuille ;
   - environ 3 fils actifs sur 32 par warp ;
   - environ 50 ms de travail sur les 138 ms de l'étage `domain`.
8. **Le protocole de mesure était honnête mais mal calibré.**
   - La règle était écrite d'avance et jamais réécrite après les données.
   - Les seuils ont été posés sans calcul de puissance, face à un bruit de ±9 à 12 % à W48 : cinq leviers positifs, de
     3 à 15 % sur leur phase, ont été retirés.
9. **La qualification complète date du 5 octobre** (`98a009550`). Le HEAD n'a eu depuis que des qualifications
   partielles par session.
10. **Les décisions structurelles nécessaires aux 100 ms n'ont pas été prises en v11** :
    - catalogue résident sur le GPU et produit en flux ;
    - forêts traitées comme un problème d'arbre couvrant parallèle, et non comme un pipeline de publieurs en série ;
    - descentes ramenées au coût de la v10.

    C'est l'objet de la v12.

## 2. Chronologie

| Jour | Faits marquants | Temps FULL K5 W48 (ng00 / ng01 / ng02) |
|---|---|---|
| 2 oct. | Ouverture (`52687f8e5`, 06 h 11 UTC). Audit de la v10, fondations (`core`, `sched`, `num`, `cloud`, `io`, `reference`), catalogue séquentiel puis parallèle, première tour FULL. L'auditeur réfute F3 25 min après l'ouverture. | Catalogue seul : 26,0 / 20,7 / 24,1 s, puis 4,5 / 3,0 / 4,0 s. Forêts : 16,9 s (ng00). |
| 3 oct. | Frontière adaptative, passe unique, mémo daté, lots réguliers, réemploi vertical, mode 16379 (table de populations, ordres concurrents), tranche 3 (pipeline futex), hiérarchie de points $H^{r}_{K+1}$ | 6,16 s (ng00, matin) ; 489 / 345 / 432 ms (`c40f40798`) ; 412 / 352 / 381 ms (voie `b87285378`) |
| 4 oct. | Voie GPU à un fil par feuille, q3 différé, lemme R, sortie plate E1, contrat des sorties (S0–S4), contrats R1–R7 de l'auditeur | à chaud : CPU 275–343 ms, GPU 323–394 ms |
| 5 oct. | Sorties S5 à S10 (`full`, `supports` v1, `points`, `plat`), qualification finale (12 sessions, 485/485 mutants) | — |
| 6 oct. | Supports = Kruskal (SPv2) ; N1, L4 et feuille coopérative rejetés ; J2 mémorisé, réservoir chaîné, feuilles de 24 sur GPU, levier C ; O1, V3, constantes, O2 ; exécuteur partagé ; polyèdres d'ordre K | à chaud, voie GPU : 234–301 ms (soir) |
| 7 oct. | Cohortes par tranches et cache de blocs gardés ; THP, frontière par tranches, annonces, préchargements et G1 AVX2 retirés ; clôture | à chaud : GPU 251 / 212 / 255 ms, CPU 314 / 255 / 313 ms |

360 commits touchent `morsehgp3D_v11/` du 2 au 7 octobre : 58, 82, 58, 60, 74 et 28 par jour, dont environ 92
d'auditeurs.

## 3. Bilan chiffré

### 3.1 Temps au gel

Session `claudeg1`, variante de base, archive de `733912e65`, moteur identique à `ac081a06f`. Reçu
`receipts/developpement_20261007/filtre_g1_avx2/`.

| Mode de banc | À froid | À chaud | À chaud : `domain` / forêts |
|---|---|---|---|
| K5 GPU `868347:400` (feuilles 24, partage 400 ‰, cache de blocs) | 335 / 301 / 345 | 251 / 212 / 255 (meilleures passes : 250 / 207 / 251) | 138 / 114, 120 / 91, 141 / 114 |
| K5 CPU `802811` (feuilles 16, cache de blocs) | 343 / 272 / 329 | 314 / 255 / 313 | 200 / 113, 163 / 92, 195 / 116 |
| K10 GPU `868347` (feuilles 24, cache de blocs) | 1 824 / 1 395 / 1 602 | 1 782 / 1 336 / 1 536 | 489 / 1 293, 398 / 939, 471 / 1 065 |

Temps en ms, ng00 / ng01 / ng02. L'exécutable `mhgp11` et l'API ne jouent aucun de ces modes : ils prennent la voie
CPU à feuilles de 16 à tout K, sans GPU ni cache de blocs (`src/api/compute.cpp`).

### 3.2 Comparaison à la v10, étage par étage

**Référence v10.** Commit `777406b82`, session 4 : CPU W48, profil u18, troisième passe chaude d'un même processus,
« catalogue + tour 1..K sans attaches ». Source : `receipts/audit_deep_20261004/performance/context/TABLE_VERITE_G4.md`.

**Comparabilité.** Les XYZ sont identiques à l'octet, et les cardinalités du catalogue et des forêts sont égales. Mais
les sessions et les régimes diffèrent, et le différentiel canonique intégral n'a jamais été fermé. Ces rapports sont
**descriptifs**.

| Étage, à chaud | v10 CPU | v11 GPU `868347:400` | Rapport | v11 CPU `802811` | Rapport |
|---|---|---|---|---|---|
| K5 total | 252,0 / 204,2 / 253,6 | 251 / 212 / 255 | 1,00 / 1,04 / 1,01 | 314 / 255 / 313 | 1,25 / 1,25 / 1,23 |
| K5 catalogue (`domain` v11) | 163,5 / 136,9 / 164,3 | 138 / 120 / 141 | 0,84 / 0,88 / 0,86 | 200 / 163 / 195 | 1,22 / 1,19 / 1,19 |
| K5 tour (forêts et verticales) | 88,5 / 67,3 / 89,3 | 114 / 91 / 114 | 1,29 / 1,35 / 1,28 | 113 / 92 / 116 | 1,28 / 1,37 / 1,30 |
| K10 total | 1 124,6 / 861,4 / 1 024,3 | 1 782 / 1 336 / 1 536 | 1,58 / 1,55 / 1,50 | — | — |
| K10 catalogue | 652,6 / 527,5 / 618,1 | 489 / 398 / 471 | 0,75 / 0,75 / 0,76 | — | — |
| K10 tour | 472,0 / 333,9 / 406,2 | 1 293 / 939 / 1 065 | 2,74 / 2,81 / 2,62 | — | — |

**Lecture.**
- **Catalogue.** La voie GPU de la v11 bat le catalogue de la v10. Sa voie CPU reste ×1,2 plus lente.
- **Tour.** La v10 est plus rapide, et l'écart se creuse à K10.
  - Descentes K10 de la v10 : 242,9 / 179,3 / 203,4 ms à W48.
  - Résolveurs K10 de la v11 : 1 214 / 894 / 988 ms, sur 29 voies, pour 25 à 35 CPU·s.
- **[I] Causes possibles de l'écart des descentes**, d'après `audit_deep_20261004/performance` et le rapport A :
  - la v10 choisit les k plus proches dans un intérieur surchargé ; la v11 prend les k premiers de la liste
    intérieure, ce qui peut allonger les descentes ;
  - le mémo cellulaire daté de la v10 (−9 % de pas à K10) n'a pas été porté ;
  - la v10 subdivise les boîtes à 1/64 de maille (partition T6, T > 0) ; la v11 s'arrête à la largeur entière 1 ;
  - le pipeline v11 ne donne que 29 voies de résolution à K10.

  Mesurer l'histogramme des pas par descente des deux moteurs, sur les mêmes trames, est la première question de la
  v12 sur la tour.

### 3.3 Trajectoire

| Date | Commit, reçu | Mesure | ng00 / ng01 / ng02 |
|---|---|---|---|
| 2 oct. | `e6fe34cb0`, `catalogue_20261002` | catalogue séquentiel, deux passes | 26 018 / 20 741 / 24 093 ms |
| 2 oct. | `c1046dfc7`, `catalogue_parallel_20261002` | catalogue, Pool, frontière fixe | 4 460 / 3 002 / 4 028 ms |
| 2 oct. | `c6ca345e0`, `full_20261002` (full3) | forêts et verticales, un processus | 16 885 ms (ng00) |
| 3 oct. | `3dbfd1c32`, `full_parallel_20261003/forest3` | FULL, lots réguliers | 14,79 → 6,16 s (ng00) |
| 3 oct. | `c40f40798`, `qualification_performance_20261003` | FULL CPU, mode 16379, à froid (médiane de 3) | 489 / 345 / 432 ms |
| 3 oct. | `b87285378`, `developpement_20261003/pipeline_g4` (session claudeab7) | FULL CPU, médiane de 5 | 412 / 352 / 381 ms |
| 4 oct. | `22a6af6aa`, `developpement_20261004/gpu_g4` | FULL K5 à chaud, CPU / GPU | 275–343 / 323–394 ms |
| 7 oct. | `733912e65`, `developpement_20261007/filtre_g1_avx2` | FULL K5 à chaud, GPU / CPU | 251 / 212 / 255 et 314 / 255 / 313 ms |

À K10 :
- le 2 octobre, toute prise expirait au plafond de 15 s ;
- le 4 octobre, le mur à chaud valait 1,76 à 2,35 s sur GPU ;
- au gel, 1,34 à 1,78 s.

**Bruit.** D'une session à l'autre, la même base dérive de ±5 à 10 %. Seuls les rapports appariés dans une même
session décident.

### 3.4 Empreintes de référence (pour le différentiel de la v12)

Les vidages FULL de la sonde (`MHGP11FUL1`, u21) ont des empreintes stables depuis le 3 octobre :
- pour les trames, mêmes valeurs dans `receipts/developpement_20261003/ecart_v10_v11/records.json` (`895680ff8`) et
  dans les rapports `gpu_ab` de `claudeg1` (voies CPU et GPU) ;
- pour les nuages uniformes, valeurs de `records.json` seulement (non rejouées au gel).

| Entrée | K | SHA-256 du vidage |
|---|---|---|
| ng00 | 5 | `3a2bfb4f9f48b4b0cc5b0318d9fcf4887906638e034b2c97dda1918e3170a6fe` |
| ng01 | 5 | `5212a2ced81bf69bd2abfb935a3b14a35158df25339285a0d25a9d5d5c09e091` |
| ng02 | 5 | `78feb765e21c8e4582762a25bd0dd36a455747d3f0df80523e19f7ba4be0e207` |
| ng00 | 10 | `61a4245b91d9a4fdad012f0a2e26a63c3e48db1d46a180db756f2c4e4aa77295` |
| ng01 | 10 | `838a447e0b92e13e42f7f69a84fd536d5d46d26463d3f4cc7c688475878fe0de` |
| ng02 | 10 | `81f89995eaccb497cfe92abb81213bebd0d4ce5076210570d86aa9456cf7ff2e` |
| uniforme u18, 8 000 | 5 | `f87dbb19dd928311fcb65d5a98d30b4fb50eddf7de1d26708bbc278f46dcc7cf` |
| uniforme u18, 16 000 | 5 | `141bc7d6523154cff1dd22b288e4155259e3d97a82877205887642bad8286889` |
| uniforme u18, 32 000 | 5 | `a7563907a5c9f1b273776b811d8a559eb80713eb161460edd948664f5433811b` |

## 4. Mathématiques et exactitude

### 4.1 L'objet

Entrée :
- n sites distincts, de poids 1, sur la grille entière $[0,2^{B})^{3}$ ;
- B = 21 par défaut ; 18 et 24 sont compilables ;
- K ≤ 12 et K ≤ n.

Niveaux :
- un niveau est un **rayon carré** rationnel exact ;
- il est stocké non réduit et comparé par produits croisés ;
- il est indexé par des rangs denses (`LevelRank`).

Forêts :
- la forêt $T_k$ est l'arbre de fusion de $\pi_0(L_k(a))$ ;
- ses fusions sont N-aires, à plateau atomique, avec une racine unique ;
- sa numérotation est canonique : naissances par (niveau, centre exact), fusions par (niveau, plus petite
  naissance) ;
- coupes fermées et coupes ouvertes sont distinctes.

Verticales : elles envoient chaque nœud d'ordre k sur le nœud d'ordre k−1 vivant à la coupe fermée.

Catalogue : $\mathrm{Cat}_K$ réunit les boules critiques telles que p+q ≤ K+1, avec S*, p, m, q, I et U.

### 4.2 Ce qui a marché

**Un contrat complet sans position générale** (`docs/MATHEMATIQUES.md`, § 1 à 8 et § 10) :
- M1/M2, G1–G4 (complétude conditionnelle du catalogue), T1–T6, P1–P5 et J1–J3 sont démontrés ;
- T2 (une trace est stricte si et seulement si elle est séparable) remplace partout la position générale ;
- les lemmes A, P, W, B à H du § 10 et la suffisance de Kruskal sont au registre racine avec le statut
  `proved_here` ;
- le théorème 4 de la thèse est étendu sans position générale (lemme W.4).

**Contre-épreuve de l'auditeur** (`audit_geant_20261005/math`) : 65 ordres, 17 276 unions arbitraires, 1 453 coupes et
15 925 faces verticales comparées au nerf des régions témoins, sans aucun défaut.

**Juge global d'Euler J3.**
- Cat_{K+2} suffit pour juger tous les ordres jusqu'à K.
- Il tourne à l'échelle (8 000, 16 000, 32 000 sites et les trois trames, K5 et K10).
- Les nombres de boules égalent exactement ceux de la v10.
- C'est un filet, pas un certificat : des compensations existent.

**Oracle borné** (`reference/`).
- Deux étages et un juge : l'étage A, définition exhaustive de Γ_k en `Fraction` (n ≤ 14, K ≤ 10) ; l'étage B,
  constructif. Le juge exige B = A champ par champ, et un oracle d'intervalles n'importe rien du paquet.
- Couverture : 1 362 ordres, 48 234 coupes, 13 029 nœuds, 22 mutants tués. Vidages v10 identiques sur 190 nuages.
- Oracle des supports S1 : 210 nuages, 951 ordres, 13 mutants.

**Doctrine numérique.**
- Budgets de bits typés et calculés en `constexpr` (`src/num/budgets.hpp`) : un débordement est une erreur de
  compilation.
- Trois voies (native, contrôlée, `Wide`), avec des certificats globaux calculés par les fabriques.
- Seul q3 est dur : 216M⁶.
- « Le flottant propose, l'entier décide » :
  - racines certifiées par isqrt ;
  - tri F3/F4 rejoué en exact ;
  - égalités de sommes de radicaux décidées par classes de carrés ;
  - tête plate décidée par encadrements entiers.
- Le coût des profils est faible : u21/u18 ≈ 1,05–1,06 et u24/u21 ≈ 1,00.

**Déterminisme** : sorties identiques à W1, W8 et W48, sous permutation et sous réétiquetage.

**Lemmes courts devenus des gains moteur.**
- Table de populations : une égalité exacte de population signale un pas terminal. Les présentations MEB passent de
  15,7 / 12,6 / 15,6 M à 3,8 / 2,9 / 3,3 M.
- MEB par diamètre et zonogone de la droite des centres.
- Bornes entières sur sites et arbre radix de Morton (V3) : tests de points du census ×0,26.
- Lemme R (census par masques).
- Niveaux q3 et q4 différés.

### 4.3 Ce qui n'a pas marché

| Élément | Cause, preuve |
|---|---|
| Borne flottante F3 « par nombre d'instructions » | Témoin N = 2^53+1 : erreur ≈ 8u > 7u. Remplacée par une propagation par expression (`audit_independant_20261002/floating_bounds`). |
| Borne F2 « par degré » | x = 32767, A = 512x³ : (A+1)−A−1 donne −1 en binary64. |
| MEB « support + extérieur » (idée v10) | Plus de présentations (341 → 392) et de tests (828 → 1 294) sur 108 fixtures. Écartée. |
| Euler comme certificat | Compensation sur 5 sites ; défauts invisibles à 13 sites (K5) et à 23 sites (K10). Gardé comme filet seulement. |
| Supports v1 (tous les $\mathcal{Q}_b$) | Porteur instable (cercle à quatre points, saut de Hausdorff ≥ 1/4). Remplacés par Kruskal avec S* seul. |
| Marge en niveau carré (Q₁ v10, ER0h) | Aucune constante uniforme (rapport 31,8 à L = 10⁴). |
| Polyèdre d'ordre k comme jeton | Trois affirmations réfutées ; environ 1 000 faces par site à K5 ; prototypes Python seulement. |

### 4.4 Pièges

- **Une borne sur le résultat final ne borne pas les intermédiaires.**
  - Triangle aigu avec s = 2^21−1 : le premier produit vaut 6s⁶ ≥ 2^127, alors que la puissance finale tient dans
    un i128.
  - La garde i64 du test cubique de région était dépassée dès u21.
  - Export u24 sur trois mots : niveau de 196/148 bits.
  - `LevelSource` : borne en u128, calcul en i128, et le +1 déborde.
  - Racine 2^127−1.
- **Contrat sur les sites, pas sur la boîte.** Le minimum sur les points entiers d'une boîte n'est pas le minimum
  continu. Le contrat V3 est donc réservé aux sites.
- **Coupes et dates.**
  - Témoin D2 (41 < 64 < 1681/25) : seuls les ensembles de nœuds coïncident entre coupe ouverte et coupe fermée de
    rang inférieur, pas les composantes.
  - Un mémo n'est valide qu'à partir de la date initiale, jamais de la terminale (X = {0, 2, 4, 6}).
- **Contradictions gravées** :
  - E5 : la proposition 6 de la thèse est fausse ;
  - sept sites {0, 10, 11, 26, 27, 45, 46} : les ordres se croisent ;
  - {0, 2, 4} : le plus petit ancêtre commun est discontinu ;
  - {0, 2, 5} : cover ≠ MR₂-bord ;
  - cercle : supports instables ;
  - trois réfutations sur le polyèdre (`1fbeea5b8`).
- **Invariants supposés faux.** « Feuilles ≤ sites » est faux : 353 456 feuilles pour 39 885 sites. Toutes les trames
  étaient refusées (session claudegpu2).
- **Profils.** Des mutants compilés en u18 étaient vides : leurs branches sont éliminées par `if constexpr` pour
  B ≤ 20.
- **Oracles décimaux insuffisants aux plateaux exacts** : (2, 162, 50) et (8, 98, 32) valent tous deux 5√2.
- **Quantification.** 61 à 67 % des nœuds K5 vivent moins de 2δ (δ = √3/2 mm). Seuls les nœuds de vie > 2δ ont une
  identité stable.

### 4.5 Dettes

**Objet et preuves.**
- FULL pondéré : refusé (multiplicités) ; le modèle par copies n'est ni relu ni implanté.
- Coquilles étendues : seule l'énumération bornée existe. Une coquille cosphérique de 270 sites, entrée u18 valide,
  est refusée.
- Aucune borne de travail ni de terminaison générale pour la subdivision.
- [I] Sur LiDAR, le catalogue croît comme n·K² environ : 31 à 33 boules par site à Cat₅, 120 à 140 à Cat₁₀.
- Points : aucune règle connue n'est à la fois stable par insertion, locale au profil et conforme à T0 et Q1–Q4.

**Validation.**
- **Le différentiel canonique v10/v11 sur trames entières n'a jamais été fermé.** Seules les cardinalités sont égales,
  alors que `docs/ARCHITECTURE.md` (§ 6) en fait une condition de conformité.
- L'oracle des supports s'arrête à K ≤ 5 et aux coquilles ≤ 12 sites.
- Portes natives demandées le 4 octobre : q3 extrême, q4 au seuil 2^20 et 2^20+1, préfixe obtus, coquille qmin = 2.
  Elles ne sont couvertes qu'en partie, côté hôte (`mhgp11_catalogue_leaf_narrow`).

## 5. Catalogue (étage `domain`)

### 5.1 Algorithme au gel

`prepare_full_domain` construit Cat_K, puis la table support → boule.

**Frontière** (`adaptive_frontier.cpp`, `adaptive_prepare.cpp`).
- La racine est filtrée en série.
- Viennent ensuite des rondes « lourds d'abord » : seules les listes de population ≥ max/2 sont coupées.
- Le plan compte au plus 1 024 feuilles : 1 023 tâches et 27 à 28 rondes sur les trames, réclamées dans l'ordre LPT.

**Passe unique** (`single_pass.cpp`). Chaque tâche poursuit le DFS de boîtes :
- filtre G1 sur un réservoir de 3K témoins ;
- ajustement de la boîte, puis coupe médiane ;
- sorties en pages par ordinal, puis compactage.

**Feuille** (`leaf.cpp`, m ≤ 32).
- Dominances en O(m²) sous forme de masques, graphe de paires et lignes vivantes.
- DFS des préfixes q2 à q4, sous les filtres G3, J2, M3 et E4.
- Census par masques (lemme R), puis support canonique.
- Seul S* est émis ; les niveaux q3 et q4 sont différés.

**Fin.** Tri indirect (clés binary64 F3/F4 avec repli exact), balayage des niveaux, assemblage par blocs, table à
sondage linéaire remplie par CAS.

**Répartition** (ng00, K5, une prise à chaud, `developpement_20261007/diagnostic_domaine`) :

| Sous-étage (ms) | GPU `868347:400`, feuilles 24 | CPU `802811`, feuilles 16 |
|---|---:|---:|
| Frontière (racine / rondes / sélection / publication) | 20,7 (0,9 / 14,4 / 0,7 / 4,6) | 19,9 |
| Passe unique | 37,5 (Σ des tâches 1,78 CPU·s) | 152,1 (Σ 7,0 CPU·s) |
| Lot de feuilles (hôte 31 902 feuilles en parallèle de GPU 91 679) | 51,8 | — |
| Level sur l'hôte et rassemblement | 3,2 | — |
| Tri / balayage / assemblage / compactage | 10,0 / 4,4 / 3,4 / 3,2 | 10,0 / 4,4 / 3,3 / 2,8 |
| Restitution / table support → boule | 0,8 / 2,9 | 3,6 / 2,9 |
| **`domain`** | **138,0** | **199,2** |

**Lecture.**
- Ces sous-étages s'enchaînent strictement en série.
- En voie GPU, la passe unique est presque entièrement faite des filtres G1 des nœuds internes.
- La frontière est faite de rondes étroites, d'environ 0,5 ms chacune ; elle n'a pas bougé depuis le 3 octobre.
- À K10 :
  - passe unique 116 à 141 ms ;
  - exécuteur 167 à 207 ms ;
  - puis 88 à 112 ms de petits étages en série : tri 32–46, balayage 15–19,5, assemblage 12–15, compactage
    9,5–11,5, Level 11–14, table 8–10.

### 5.2 Ce qui a marché

| Levier | Effet mesuré | Source |
|---|---|---|
| Frontière possédée et Pool | 26 s → 4,5 s, sorties égales au séquentiel | `catalogue_parallel_20261002` |
| Tri indirect, puis clés F3/F4 | tri 0,9–1,4 s → 9–13 ms | `catalogue_optimizations_20261002`, `qualification_performance_20261003` |
| Assemblage par blocs | 167–250 ms → 4,0–5,2 ms | `catalogue_assembly_20261003` |
| Frontière adaptative | catalogue ×1,25 à 1,61 | `assembly1` |
| Passe unique | `domain` ×1,65 à 1,80 | `catalogue_single_full_20261003` |
| `ef75dafac` : lourds d'abord, LPT, termes G1 précalculés, G3 avant J2, lignes vivantes | plus longue tâche 0,885 → 0,045 s (en local) ; `domain` 708 → 265 ms (ng00, paquet) | `developpement_20261003/ecart_v10_v11` |
| Graphe de paires | 265 → 227, 217 → 181, 249 → 239 ms | `qualification_performance_20261003` |
| Table support → boule remplie par CAS | ~30 → 3 ms | `src/tower/full_domain.cpp` |
| Niveaux q4 différés | 99,7 % des niveaux q4 évités | `catalogue_q4_20261002` |
| q3 différé | −1,0 à −1,3 % du mur (W1) | `mesures_g4_ab8_diag1` |
| Cache de blocs du budget | restitution 14 → 0,8 ms ; mur à chaud ×0,923, à froid ×0,974 | `developpement_20261007/cache_blocs` |

### 5.3 Ce qui n'a pas marché

| Levier | Cause, chiffre | Source |
|---|---|---|
| Frontière fixe de profondeur 8 | une tâche de 1,527 s dans une phase de 1,533 s | `catalogue_parallel_20261002` |
| Règle adaptative initiale | plan épuisé vers la profondeur 11, zones denses en tâches de milliers de sites | `docs/CATALOGUE_FRONTIERE_ADAPTATIVE.md` |
| Filtre flottant F6 de `power` | environ 1 % du CPU ; retiré | `developpement_20261003/pipeline_g4` |
| Enveloppes M3/E4 sur la voie CPU | +1,2 à 1,4 % de passe unique à W1. Retrait approuvé par l'auditeur, jamais fait : elles sont toujours dans `leaf.cpp`. | `mesures_g4_ab8_diag1` |
| N1, arène de pile du parcours | +2,4 à 8,5 ms (cause probable : SMT ; non établie comme exclusive) ; retiré `c1675e4c9` | `developpement_20261006/n1_ab` |
| Frontière par tranches | moyenne géométrique 0,859 pour un seuil de 0,80 ; retiré `64746f985`. Les rondes coûtent par leur nombre, pas par leur largeur. | `developpement_20261007/frontiere_tranches` |
| Filtre G1 en AVX2 | 0,895 pour un seuil de 0,85 (−12 à −17 % en voie GPU ; en voie CPU, la statistique était diluée par les feuilles) ; retiré `23b759dfe` | `developpement_20261007/filtre_g1_avx2` |
| Pages de 2 Mio (THP) | −17 % sur les forêts en local, mais ×1,064 sur le mur à chaud sur G4 ; fermé | `developpement_20261007/thp_exploration` |
| Feuilles de 24 sur CPU à K5 | +17 à 21 ms | `reservoir3_chemin_chaud`, `domain_diagnostic` |

### 5.4 Pistes jamais faites ou jamais portées

- **Feuille J3 de la v10**, ×1,37 sur l'étage des boîtes (mesure locale) : portée en partie seulement (lemme R, M3/E4).
- **Partition T > 0** (sous-maille à 1/64 de la v10).
  - Jamais mesurée comme port.
  - L'auditeur a proposé une reformulation compatible avec u24 : comparer QE à Dr, avec un budget 4B+5+T ≤ 127.
- **Extrema q2 couplés.**
- **Repli `unresolved` parallèle (recette U2).** Le repli reste en série sur le pilote
  (`src/catalogue/single_pass_batch.cpp`, l. 142).

## 6. Voie GPU

### 6.1 Ce qui a marché

**Exactitude tenue de bout en bout.**
- 372 prises à froid et 84 processus à chaud rendent les mêmes vidages et le même registre que le CPU (`gpu_g4`).
- Chaque banc suivant est « conforme ».
- Compute Sanitizer ne relève aucune erreur.

**Contrat R7.** L'appareil est exact sur des chemins i128 certifiés ; une feuille hors de ces chemins rend
`kUnresolved` et est rejouée sur l'hôte avant admission. Aucun écart de sortie n'a été observé.

**Source unique hôte/CUDA** : `leaf_device.hpp`.

**Leviers gardés.**

| Levier | Effet |
|---|---|
| Cases par feuille, rassemblement et copie par le Pool, pages pré-touchées, enregistrements compacts (`b74f9ea3a`, `16b482169`, `4ec33e3d7`) | écriture 35 → 19 ms, rassemblement 10,5 → 2,4, retour 22 → 2,3, Level 15 → 5,6 |
| J2 mémorisé et rangs locaux directs (`34a8a561d`) | exécuteur ×0,82 à K5, ×0,76 à K10 |
| Réservoir chaîné (`79fa5e9f7`) | écriture 13 → 0,3 ms à K5, 75 → 1,7 à K10 ; gardé alors que son critère n'était pas atteint (comptage ×1,28–1,32, à cause d'un `CALL`) |
| Feuilles de 24 à K5 sur GPU | première victoire du GPU sur le CPU (`domain` 192 contre 210 ms) |
| Arithmétique étroite, levier C (`5861c223f`) | comptage ×0,83 à 0,86 sur les feuilles d'étendue ≤ 2^20 ; bornes refaites (6D³ < 2^63), avis favorable de l'auditeur |
| Exécuteur partagé hôte/GPU à 400 ‰ (`2045ec27c`) | `domain` à chaud ×0,873 à 0,907 ; règle à froid manquée, confirmation à chaud 0,893 |
| Feuilles de 24 au lieu de 16 à K10 | −24 à −27 % du mur FULL |

### 6.2 Ce qui n'a pas marché

| Levier | Cause, chiffre | Source |
|---|---|---|
| Lot sur l'hôte seul | jamais meilleur que la voie CPU (374 contre 233 ms en S4) | `gpu_g4` |
| Fils triés par taille de feuille | sans effet : m prédit mal le travail (Spearman 0,55 ; triplets candidats 0,91) | `gpu_g4`, `wfgpu1_leviers` |
| Blocs d'un warp, barrière retirée | attente mémoire 1,19 → 3,03 ; noyau −3 % seulement | `gpu_g4` |
| L4, 16 sous-lots recouverts | ×1,6 à 3,5 plus lent : chaque sous-lot paie sa queue ; retiré `830473218` | `l4_recouvrement` |
| Feuille coopérative par paires (un warp par feuille) | exécuteur ×1,15 à 1,38 à K5, divergence intacte, 198 registres ; retirée `d4228f5e5` | `coop1`, `coop2`, `coop3` |
| Levier A, réservoir sans appel | ×1,11 à K5/16, malgré le meilleur SASS statique | `wfgpu1_leviers` |
| Leviers B (feuilles lourdes au CPU) et D (ordre par travail) | prédicteur à 2–9 µs par feuille sur le chemin critique ; équilibrage parfait borné à ×0,86–0,93 | `wfgpu1_leviers/workflow` |
| Partage à froid, partage à K10 | contexte CUDA ≈ 75 ms à froid ; 250 ‰ donne 0,96 à K10, sans règle écrite | `lot_partage`, `lot_partage_confirmation` |

### 6.3 Limites structurelles

- **Occupation.** Un fil par feuille :
  - 3,1 à 3,4 fils actifs sur 32 par warp ;
  - occupation de 17 à 22 % ;
  - pile locale de 3,2 Kio lue à 2,2 octets utiles par secteur ;
  - ALU à 24 %.
- **Temps de travail du GPU.** Il ne travaille que pendant environ 50 des 138 ms de `domain`, et attend le join de la
  passe unique.
- **Ouverture à froid.** L'ouverture CUDA coûte 78 ms sous Nsight, 124 à 151 ms sous charge. À froid, la voie GPU
  perd donc : 224/205/231 ms de `domain` contre 208/190/209 pour la voie CPU.
- **Mémoire de l'appareil.**
  - 196 à 243 Mo à K5, 1,15 à 1,42 Go à K10.
  - Ce sont surtout les cases de 2 Kio par feuille ; or les feuilles se recouvrent (13 par site à K10/24).
  - [I] C'est un risque pour les nuages de plusieurs millions de points.
- **Équilibrage.** Le poids m³ corrèle mal au travail réel. Un petit Pool et un `std::thread` sont recréés à chaque
  lot.
- **Régression de reconvergence.**
  - L'appel à `extend_one` faisait passer `BSSY` de 85 à 107 et doublait l'écriture.
  - Trouvée par lecture du SASS, corrigée en `9eee2ed4b`.
  - Compter `BSSY` et `CALL` avant chaque session : le SASS statique signale ces régressions, mais il ne prédit pas le
    temps.

## 7. Tour FULL et forêts

### 7.1 Algorithme au gel

**Entrée.** Un `FullDomain` immuable : index, catalogue trié par (niveau exact, support), table support → boule. Les
descentes ne lisent jamais le DSU, si bien que la résolution peut être calculée d'avance.

**Cinq étapes par ordre k :**
- **A. Classification** des cellules.
- **B. Naissances.** Les cohortes de même rang sont triées par centre exact.
- **C. Résolution des graines** :
  - par la table de populations « liée » ;
  - sinon par `descend_each_step` : MEB borné, census sur l'index, support canonique ;
  - garde : la date **initiale** doit être strictement inférieure au niveau de la cellule.
- **D. Publication par plateau de rang.** `find`, `touch`, `unite_roots`, puis `close` : une seule multifusion N-aire
  par plateau.
- **E. Verticales** par `ClosedAncestorSweep`, avec réemploi des graines régulières : 857 771 descentes évitées sur
  857 891 (ng00).

**Parallélisme.** Voie `concurrent_orders` :
- A par blocs ;
- B en trois distributions, dont les cohortes en (K−1)×32 tranches ;
- puis une seule distribution « pipeline » de W tâches : L = W−(2K−1) résolveurs, K publieurs (un par ordre) et
  K−1 suiveurs verticaux, synchronisés par époques et futex.

Le placement O1 épingle P_K, P_{K−1}, V_K et V_{K−1} sur des cœurs physiques, pour K ≤ 5 seulement.

**Garanties.**
- Plateau atomique (T4) : le mutant `forest_plateau_sequentiel` le garde.
- Descente datée (T5).
- Numérotation canonique, donc octets identiques à tout W.
- Contrôles de naturalité dans le produit.
- Admission au `MemoryBudget` avant toute allocation.
- Portes `pipeline_decisions` (4 000 flux) et `pipeline_equivalence` (W48 ×6), TSan, 163 mutants `tower`.

**Chemin critique K5** :
- préambule de 13 à 16 ms (table de populations de 44 Mo reconstruite à chaque appel, classification 2 ms,
  naissances 4 à 5 ms) ;
- puis max(R ≈ 63–83 ms, P5 ≈ 80–102 ms, V5 qui suit P5 de 0 à 6 ms).

**Publieur de l'ordre 5** (profil échantillonné) :
- cellules : environ 65 ms, à 145 ns chacune ;
- clôtures : environ 29 ms, à 66 ns ;
- reste : environ 13 ms.

**Plateaux presque singletons** : 438 011 clôtures pour 448 698 cellules à l'ordre 5. Une barrière par plateau est
sans espoir.

**K10.** Les résolveurs sont critiques : 1 214 / 894 / 988 ms, 25 à 35 CPU·s sur 29 voies. Les 19 publieurs et
suiveurs attendent environ 1 s sur 1,22 s.

### 7.2 Ce qui a marché

| Levier | Source | Effet |
|---|---|---|
| Mémo de descente daté | `c2c3e0323` | FULL ×1,27–1,32 ; supplanté ensuite |
| Lots réguliers par lanes | `3dbfd1c32` | ng00 FULL 14,79 → 6,16 s |
| Census réutilisé, verticales parallèles | `cc93360a3` | forêt ng00 750 ms |
| Réemploi vertical régulier, table dense des naissances | `ae817d09e` | 857 771 / 857 891 descentes verticales évitées ; forêt ng00 658 ms |
| Mode 16379 : table de populations, ordres concurrents, sans mémo | `c40f40798` | forêts 240,8 / 163,6 / 197,2 ms ; remplace environ 600 barrières |
| Tranche 3 : voie liée, naissances par blocs, pipeline futex | `b87285378` | forêts 209 → 171 / 133 / 158 ms |
| O1, placement des tâches lourdes | `9d10de213` → `12ce8f8f0` | moyenne géométrique 0,917, pire rapport 0,967 (troisième essai) |
| V3, census : bornes entières et arbre radix | `a841d8c4b` | forêts ×0,925 ; tests de points ×0,26 ; instructions de résolution ×0,565 |
| Constantes du pas de descente | `751686868` | ×0,966 |
| O2 partie 1 : parents DSU denses, préchargement des parents et des états | `13a4a0a4c` | ×0,904 |
| Tri des cohortes par tranches | `5734ca6e8` | naissances ×0,529 ; forêts ×0,942 |
| Chronos par tâche, profil échantillonné des publieurs | `e49ea4690`, `d5b1d0179`, `0ff64512a` | diagnostics qui ont localisé la queue (publieur 5) |

### 7.3 Ce qui n'a pas marché

| Essai | Cause, chiffres | Source |
|---|---|---|
| Mémos de lane | 2,6 % de succès ; incompatibles avec le pipeline | note d'audit du 3 oct. |
| Lots ordre par ordre | environ 600 barrières et 330 ms de pilote seul | `forest_concurrent.cpp` |
| O1, premiers essais | option perdue au déplacement de `ForestParallel` (A/A involontaire), puis critère manqué (ng02 à chaud 1,038) | claudeo1place, claudeo1place2 |
| Annonces tous les 1 024 plateaux | publieur 5 ×0,966 pour un seuil de 0,90 ; retiré `06fdf8013` | `annonces_publieurs` |
| Préchargement du nœud des graines | 0,865 pour un seuil de 0,85 ; retiré `caca9d8d7` | `prechargement_graines` |
| Préchargements combinés | 0,950 pour 0,90 ; clôtures inchangées ; retiré `4de17f141` | `prechargements_publieurs` |
| Forêt parallèle (V4/O7) | jamais construite ; précédents négatifs (Borůvka v9 à 1 264,7 ms ; 31–45 % des fusions à la couture) | notes hors dépôt `build/v11-persist/gpu_optim/carte_forets.md` et `build/v11-persist/conception/PISTES_DE_RUPTURE.md` |

### 7.4 Pièges

- **Cohabitation.**
  - Le publieur 5 coûte 35 à 46 ms seul sur un fil, mais 81 à 151 ms de CPU dans le pipeline (95 à 99 ms après O1 et
    O2).
  - Le balayage V5 passe de 28–40 ms à 68–88 ms.
  - Les parts du SMT, du L3 et des réveils futex ne sont pas mesurées.
- **Défaut de concurrence trouvé par l'auditeur.**
  - `ForestProgress::finish` publiait `closed = kNone` après `abandoned`, si bien qu'au réveil le contrôle d'abandon
    était sauté.
  - Corrigé en `3bd4d734e`, avec porte et mutant.
  - Une sentinelle qui sert à la fois de fin et d'abandon se revérifie après chaque attente.
- **Le MST seul ne transporte pas les compteurs logiques** (triangle (0,0), (3,0), (1,2)). Le contrat des compteurs
  doit être écrit avant tout port de forêt parallèle.
- **Rigidité du Pool** : une tâche par fil, pas de vol de travail. À K10, seuls 29 fils résolvent.
- **Fixtures de couverture inatteignables.** Des nuages de ≤ 5 sites ne pouvaient pas saturer un plancher
  (`vertical1`). Un mutant qui ne compile pas n'est pas tué.

### 7.5 Pistes jamais faites

- **O2 partie 2.** Flux compact par ordre : chaque publieur balaie encore les 1,31 M fiches de boules.
- **Mémo de cellule daté (V7).** La v10 en tirait −7 % de pas à K5 et −9 % à K10.
- **Autres** :
  - S11, un pipeline à un seul ordre pour `points` et `plat` (estimé à 65–90 ms contre 138–184 ms) ;
  - sur-souscription des fils libres (O4) ;
  - placement conscient des CCD ;
  - vagues GPU de descentes (O8).
- **Prérequis de la forêt parallèle.** La « contraction des plateaux par composantes fortement connexes » reste une
  `proof_obligation` au registre ; le lemme MST/contraction de l'auditeur n'y est pas inscrit.

## 8. Sorties, hiérarchies dérivées, HDBSCAN, polyèdres

### 8.1 Le socle de sortie

Un seul exécutable, `mhgp11`, avec `--sortie` obligatoire et 1 ≤ K ≤ 12.
- **Façade `api`** : `Session` (budget et Pool uniques), `compute`, `publish`, `finish`, `withdraw`.
- **Module `io`** : dossier transactionnel. Le dossier `D.pending` reçoit son manifeste en dernier, puis
  `renameat2(RENAME_NOREPLACE)` le publie.
- **Codes de sortie** : 0, 2 (refus) et 3 (invariant violé) ; une ligne JSON par exécution.
- **Signature** : `tree_k_sha256` v2, commune aux quatre sorties.

| Sortie | Objet | Format | Dernière qualification G4 |
|---|---|---|---|
| `full` | tour FULL, ordres 1..K, verticales | `MHGP11FUL1` | `qualification_finale` (`98a009550`) |
| `supports` | arbre couvrant d'ordre K : naissances et fusions retenues par Kruskal au plateau, S* seul | `MHGP11SP` v2 | `supports_kruskal` (`07428324e`) : 15/15, Release u21 seulement |
| `points` | $H^{r}_{K+1}=P_1\circ\Pi_{K+1}$, avec m(1) = 1, m(K) = K+1 et κ = 1 | `MHGP11PT` v1 | `qualification_finale` (différentiels S9) |
| `plat` | critère A, puis EOM N-aire à $\varphi=r^{-z}$ ou feuilles ; défaut EOM, z = 1, mcs 20 | `MHGP11ET` v1 | `qualification_finale` (différentiels S10, mutants `head` 10/10) |

### 8.2 Ce qui a marché

**Supports réduits à l'arbre couvrant de Kruskal.**
- Les lemmes A à H sont prouvés sans position générale.
- Sur ng00 à K5, on publie 576 388 boules pour 576 371 nœuds, contre 789 886 boules en v1.
- Le plafond de coquille de 24 sites disparaît.
- Un différentiel exact contre l'oracle S1 et un lecteur couvrant les contrôlent.

**Une règle écrite d'avance a fixé la voie de calcul.**
- Rapport supports/FULL de l'étage `tree` : 1,21 / 1,18 / 1,09, d'où la livraison de L2b.
- Après L2b : 1,045 / 0,999 / 1,055.

**La hiérarchie $H^{r}_{K+1}$.**
- Fidèle à FULL, laminaire, indépendante de mcs.
- Stable en 3ε pour les dates et les hauteurs.
- Dates décidées exactement.
- Porte stricte G4 `claudepts6` : 2 854 nuages, 215 974 sites.
- Port natif S9 identique à Python sur synthétique et sur les trois trames.

**Gain au niveau B (meilleur bloc) contre HDBSCAN.**
- Synthétique, n = 8 000 : +0,008, +0,026, +0,050 et +0,078 à k = 2, 3, 5 et 10.
- Trames voisines (859 instances) : sauvetages/pertes +19/−8, +22/−7, +27/−4 et +35/−1.
- Trois vélos sauvés dans les démos.

**Tête plate exacte.**
- Aucune décision flottante ; le parent ne l'emporte que sur égalité certifiée.
- Au-delà du budget, refus entier de l'appel.
- Natif égal à Python sur 1 920 appels.

**Choix LiDAR fixé par un critère écrit d'avance** : l'EOM à z = 1 retrouve tous les objets dans 68,5 % des couples
(scène, k), contre 56,6 % pour `sklearn`.

**Démos et vidéos** (`Zoltan/demos/`) : sur 360 bouts de scène, HGP gagne 13 fois et perd 3 fois.

**Polyèdres : objet fixé avec l'auditeur.** C'est A_k(r), la mosaïque d'ordre k filtrée par d_k. L'oracle exact ne
montre aucun désaccord avec FULL sur 91 ordres et 37 993 coupes.

### 8.3 Ce qui n'a pas marché

**Supports v1.**
- Instables, plafonnés à 24 sites, et ils ne dessinent pas l'objet.
- `kparties_reliees` n'est pas stable : sortir un site de la coquille le fait passer de 3 à 1.

**Un filtre de rôle n'est pas Kruskal.** Le triangle équidistant ferme un cycle de plateau (`be8085ec1`) ; corrigé en
`07428324e`.

**L'arbre d'ordre K brut n'est pas utilisable comme jetons.**
- 14,45 nœuds par site à K5, 41,08 à K10.
- 67,8 % des nœuds K5 meurent à moins de 1 % au-dessus de leur rayon de naissance.

**Points.**
- Les cibles Q2–Q4 sont perdues : 70 cellules sur 125.
- Aucune stabilité par insertion.
- Le gain au niveau B vient de FULL, pas de la marge : MR₂-bord, sans tour, rattrape aussi le vélo C.

**Sélection plate, le goulot.**
- Une antichaîne oracle retrouverait 252 objets sur 258 ; l'EOM à z = 1 en retrouve 198.
- Cause : les séparations fugaces. Le vélo 3 n'est séparé qu'entre 107,4 et 107,8 mm.

**Préenregistrement inachevé.**
- S3b et P08 n'ont jamais été lancées ; S2b et S3a n'ont pas de reçu.
- Le choix z = 1 sur LiDAR repose sur les exemples de développement.

**Seuil de condensation relatif au parent.**
- Il ne garde que 9 objets sur 19, et aucun vélo contre un mur. La thèse prescrit un seuil absolu.
- `CLAUDE.md` et `Zoltan/FoundationModel/SPECIFICATION.md` demandent pourtant un seuil relatif ; la décision de
  l'utilisateur est en attente.

**Polyèdres, à l'échelle.**
- Environ 1 000 faces par site à K5, soit 62 à 82 fois la tour.
- Un seul site proche crée une composante.
- Le vélo réel 08/002852 n'est lisible à aucun K.

**Comparaisons fragiles à HDBSCAN.**
- `np.argsort` n'est pas stable dans `_process_mst`.
- La tête N-aire appliquée à l'arbre de `sklearn` diffère de ses étiquettes sur 520 configurations sur 2 400.
- `sklearn` 1.7.2 sur G4, 1.9.1 sur le codespace : jamais qualifiés sur un même banc.

### 8.4 Dettes

**Qualification.**
- SPv2 n'est qualifiée qu'en Release u21 : pas de mutants sur G4, pas d'u18/u24, pas de jumelles `_opt`, pas de
  triangle à W48.
- Aucune porte K10 pour `supports` ni pour `points`.
- Les différentiels S9 et S10 n'ont pas été rejoués après la garde de chronologie (`b0f2a0a9e`).
- Coût de `supports`, `points` et `plat` non mesuré sur G4.

**Documents périmés** : voir le tableau d'errata de la passation (§ 9).

## 9. Tests, portes, mutants, qualification

### 9.1 Ce qui a marché

**Porte à code exact et registre fermé** (`cmake/run_expect.cmake`, `cmake/gates.cmake`).
- Codes 0 à 4 exacts ; un signal est un échec ; 126/127 signifie « lancement impossible », jamais « mutant tué ».
- Sont refusés : `add_test` direct, `PASS_REGULAR_EXPRESSION`, `WILL_FAIL`, `DISABLED`.
- Le harnais a ses propres portes, par injection de fautes.

**Anti-vacuité à chaque étage.**
- `ctest --no-tests=error`.
- `tools/g4_matrix.py` classe une configuration `vacuous`, `incomplete` ou `floor_violated`, et recoupe JUnit et
  sortie standard.
- Ces gardes ont mordu : des matrices coupées sont restées rouges.

**Mutants causaux.**
- Patch sur copie, témoin joué d'abord, « tué » seulement avec la cause donnée par le juge.
- R3 : 485/485, recomptés par l'auditeur dans `LastTest.log`.
- Ils ont révélé de vrais défauts de câblage.
- 530 mutants déclarés au gel, dans 13 modules.

**Volumes** : environ 21 900 lignes sous `src/`, 48 700 sous `tests/`, 124 scripts Python de porte, 385 tests C++.

**Campagnes complètes.**
- `c40f40798` : 4 073/4 073 portes, 326 mutants.
- `98a009550` (reprises R1 à R4) : 3 695 portes, 485/485 mutants, 80 portes ASan u24 d'échelle.
- `38faaf272` (qualification V3) : Release u18 890/890, u21 et u24 800/800, ASan u24 800/800.

**Portes Python reproductibles** : Python 3.10 nu, jumelle `-O`, `assert` interdit.

### 9.2 Ce qui n'a pas marché

**G4 a servi de boucle de compilation le 2 octobre.**
- 26 sessions sur 38 ont fini en `failed_remote`, souvent sur des fautes qu'une compilation locale aurait vues
  (`-Werror=misleading-indentation`, `maybe-uninitialized`, attendu faux).
- La pratique a changé le 5 octobre (construction locale ciblée).

**La matrice dépassait la session.**
- `maxRunDuration` vaut 4 200 s pour un budget de matrice de 2 100 à 2 200 s.
- Causes : jumelles `_opt` des portes d'échelle ; `-L ^long$` qui aspirait les campagnes de mutants ; un profil
  entier par configuration.
- Bilan : 19 sessions le 5 octobre pour qualifier S8, S9, L2b et S10.

**Attendus gravés pour un seul profil.** Des empreintes u21 jouées en u18 et u24 ont donné six échecs, et 23 mutants
n'ont pas été jugés.

**Mutants partiellement couverts.**
- Profil de base u18 (453 joués en u18, 15 en u21, 17 en u24) ; aucune variante TSan.
- `--check` ne vérifie pas que la porte citée existe (claudesplit1 n'a jugé aucun mutant).

**Aucune qualification complète au HEAD.** Depuis le 5 octobre, seulement des mutants ciblés et des matrices ASan/TSan
courtes par session.

## 10. Mesure et protocole statistique

### 10.1 Ce qui a marché

**Identité à l'octet, condition de toute mesure.** `gpu_ab.py` exige, à chaque prise, l'empreinte du vidage et le
registre du catalogue.

**Règle écrite avant les données, jamais réécrite.** Les 31 `plan.json` des 6 et 7 octobre égalent l'empreinte
certifiée au lancement, et les échecs sont publiés tels quels.

**Bras A/A.** Exemple avec une fenêtre de validité [0,97 ; 1,03] : moyenne géométrique A/A 0,994 (`o1_placement`).

**G4 seul juge des temps.**
- THP : −17 % en local, ×1,064 sur G4.
- Levier A : meilleur SASS statique, ×1,11 sur G4.
- V3 : instructions ×0,565, temps ×0,925.

**Un fil pour trancher les leviers algorithmiques** : bruit A/A d'environ ±0,5 % à W1, contre 9 % à W48.

### 10.2 Ce qui n'a pas marché

**Seuils sans calcul de puissance.**

| Levier retiré | Mesuré | Seuil |
|---|---:|---:|
| Préchargement des graines | 0,865 | 0,85 |
| Filtre G1 AVX2 | 0,895 | 0,85 |
| Frontière par tranches | 0,859 | 0,80 |
| Annonces des publieurs | 0,966 | 0,90 |
| Préchargements combinés | 0,950 | 0,90 |
| Lot partagé, à froid | 0,916 à 0,977 | 0,90 |

**Statistique mal ciblée.**
- Une mesure à froid pour juger un levier GPU, alors que l'ouverture CUDA coûte environ 75 ms.
- La passe unique CPU pour juger G1 (effet dilué par les feuilles).
- K5/16 pour juger le levier A, alors que K5/24 était la configuration de référence GPU.

**À chaud, un seul processus par (trame, mode), dans un ordre fixe.** [I] La faible dispersion des passes masque la
variance entre processus.

**Première prise biaisée.** La première prise GPU à froid vaut 599,8 ms, contre 343 à 357 pour les autres, et elle
tombe toujours dans le bras de tête.

**Juges ad hoc.**
- Un `judge.py` par session, non haché au reçu.
- Règle en texte libre.
- Médiane haute pour un nombre pair de prises.

**Chemins bifurquants.**
- La confirmation du lot partagé est passée du froid au chaud, et d'un seuil de 0,90 à 0,95.
- reservoir3 a été gardé alors que son critère n'était pas atteint.

**Bancs de décision sans porte.**
- Les auditeurs ont trouvé, dans `gpu_ab.py` et `ab_g4.py` :
  - un banc vert avec `reps=0` ;
  - un cache de variantes non lié au SHA ;
  - `None == None` accepté ;
  - une IoU arrondie avant le seuil strict.
- Corrigés sans rétroqualification.

**Modes opaques.**
- Les modes sont des masques entiers : 16379, 278523, 802811, `868347:400`.
- Un même masque (212987) a désigné trois variantes différentes le même jour.
- La sonde prend 10 à 13 arguments positionnels.
- 9 portes sur 9 passaient avec `placements=0`.

## 11. Sessions G4 et exploitation

**Le script de session est mûr** (`gcp-migration/v11_session.py`, environ 2 300 lignes) :
- 126 sessions distinctes au moins, dont 124 avec arrêt ciblé certifié ;
- les deux autres sont des échecs de démarrage par épuisement de capacité de la zone, `TERMINATED` constaté ensuite ;
- `--recover` a fermé une session après la perte de son contrôleur.

**Pièges payés.**
- TSan exige `setarch -R` : 6 démarrages sur 40 sans, 40 sur 40 avec.
- CMake 3.22 de la VM ne connaît pas le dialecte CUDA20.
- Préemption SPOT au démarrage.
- Capacité épuisée en us-central1-b ; nouvelle VM en c sans g++ ni CMake.
- Redémarrages du codespace hors calendrier : `/tmp` vidé, contrôleur tué, et `--recover` certifie l'arrêt sans
  rapatrier les résultats.
- Garde disque d'environ 1,15 Go, d'où des plans plafonnés à 32 Mio de résultats.
- `rsync -a` après une rejouée locale de mutants garde l'objet muté.
- `pkill -f` tue son propre shell.
- Éditer un script bash en cours d'exécution le corrompt.
- Le lanceur de session n'accepte que `./mhgp11*`, `ctest` et `python3 {src}/morsehgp3D_v11/<script>.py`.

**Hygiène des reçus.**
- `receipts/` pèse environ 190 Mo pour 9 774 fichiers, dont environ 2 000 copies de sources C++.
- L'identité du compte GCP (adresse électronique) figure dans 143 fichiers de reçus : champs `user`,
  `recovery_command` et `gcloud_account`.
- Une porte (`full_paired_protocol`) lit un reçu, et devient rouge en checkout partiel.

## 12. Canal d'audit

**Acteurs.**
- Deux auditeurs, l'« indépendant » et le « continu ».
- Le second est devenu développeur sur instruction de l'utilisateur dès le 2 octobre (sa note est gelée depuis le
  3 octobre). [I] Un seul auditeur a donc vraiment relu pendant les deux premiers jours.
- Aucune pièce n'est signée : tous les commits portent l'auteur Git `Ludwig-H`.

**Ce qui a été le plus utile.**
- Des témoins exacts minimaux gravés en fixtures : triangle K1, D2, 2^127−1, 17 entrées, sphère x²+y²+z² = 5,
  9 sites pour 80 feuilles.
- Des patches prêts, intégrés à l'octet.
- La relecture des brouillons avant commit.
- Le comptage nominatif PASS / Failed / sans résultat, mutant par mutant.
- Les revues de conception avant écriture.
- Les propositions prouvées avant le code : q3 différé, EOM exacte, bornes sur sites entiers devenues V3.

**Constats majeurs corrigés.**
- Borne F3.
- Faux verts d'outillage.
- Lecture d'un ordre abandonné après réveil.
- Export u24 trop étroit.
- Garde « feuilles ≤ sites ».
- Refus au milieu d'un `std::sort`.
- Débordement de la racine 2^127−1.
- Cycle de Kruskal au plateau.
- Placement perdu, donc A/A involontaire (vu par le développeur sur G4, témoin proposé par l'auditeur).
- Juge GPU acceptant un registre absent.
- IoU arrondie.

Aucun résultat FULL faux.

**Ce qui a coûté.**
- Des matrices plus longues que le budget.
- Des audits approfondis de variantes retirées peu après (feuille coopérative, N1, L4).
- Des notes géantes réécrites en place, où l'état courant se perd : 89 et 74 versions, ~17 500 et ~9 500 mots.
- Des captures de brouillons périmées dans l'heure.
- La moitié du volume de `receipts/` (audits).

**Adoption presque totale des constats.** [I] Elle relève davantage de la déférence que du débat.

**Arriéré.**
- Environ 40 commits du développeur, après le 6 octobre à 22 h 08, n'ont pas été relus. Parmi eux, deux changements
  sensibles : le contrat mémoire du cache de blocs (`ccdd4db75`) et le publieur O2 (`13a4a0a4c`).
- Questions sans réponse : sections T, V, X et Y de `REPONSE_CLAUDE_SUPPORTS_20261004.md`, et huit questions sur le
  polyèdre.

## 13. Registre des leviers

Le registre complet, avec effet mesuré et statut, est réparti dans les tableaux des § 5.2–5.4, 6.1–6.2 et 7.2–7.5.
Le tableau suivant en donne la synthèse.

| Domaine | Gardés (effet) | Retirés ou rejetés (cause) | Jamais faits |
|---|---|---|---|
| Catalogue CPU | frontière possédée, tri F3/F4, assemblage par blocs, frontière adaptative, passe unique, `ef75dafac`, graphe de paires, table par CAS, q3/q4 différés, cache de blocs | frontière fixe, F6, N1, frontière par tranches, G1 AVX2 (sous le seuil), THP | J3 complète, partition T > 0, extrema q2 couplés, repli U2 parallèle ; retrait M3/E4 du CPU approuvé mais non fait |
| GPU | source unique exacte, cases et copie par le Pool, J2 mémorisé, réservoir chaîné, feuilles 24 à K5, levier C, partage 400 ‰ | lot hôte seul, tri par taille, blocs d'un warp, L4, feuille coopérative, levier A, B et D | feuille cohérente par warp, `domain` résident en flux, contexte ouvert une fois par Session |
| Forêts | mémo daté (supplanté), lots réguliers, census réutilisé, réemploi vertical, table de populations, ordres concurrents, pipeline futex, O1, V3, constantes, O2 partie 1, cohortes par tranches | mémos de lane, lots ordre par ordre, annonces 1 024, préchargements (graines, combinés) | O2 partie 2, mémo de cellule daté V7, forêt parallèle (V4/O7), S11, O4, CCD, vagues GPU O8 |
| Sorties | transaction de dossier, `supports` Kruskal, `points`, `plat` exacte | supports v1, filtre de rôle | export de masse § 9.1, `condense`, polyèdre natif |
| Outillage | portes à code exact, mutants causaux, session gardée, identité par prise, règle écrite d'avance | G4 comme boucle de compilation, matrice trop longue, seuils sans puissance | juge unique en bibliothèque, modes nommés, protocole séquentiel |

## 14. Causes racines

### 14.1 Pourquoi 100 ms n'est pas atteint

1. **Trop de travail sur un seul nœud de 24 cœurs.**
   - Avant les derniers leviers, la v11 dépensait 10 à 14 CPU·s par FULL K5 (voie `b87285378`).
   - Avec 24 cœurs physiques, le SMT n'apportant qu'une fraction, le plancher de mur reste de plusieurs centaines de
     ms. [I] Les leviers mesurés ensuite ont retiré des constantes, pas un ordre de grandeur.
2. **Deux étages en série, chacun au-dessus du budget.**
   - `domain` vaut 120 à 200 ms et les forêts 91 à 116 ms à K5. Les deux se suivent sans recouvrement.
   - Pour 100 ms, il faudrait environ 40 à 50 ms chacun (condition U2 du plan GPU,
     `receipts/audit_plan_gpu_20261006/PLAN_GPU_FINAL.snapshot.md`).
3. **Des chemins critiques séquentiels par construction.**
   - Le publieur de l'ordre K publie environ 450 000 cellules et 440 000 clôtures en série (environ 100 ms).
   - La frontière fait 27 rondes étroites.
   - Les petits étages de fin du catalogue (tri, balayage, assemblage, compactage) s'enchaînent.
   - Le lot GPU attend le join de la passe unique.
4. **Un GPU employé comme un CPU de plus** : un fil par feuille, une occupation de quelques pour cent du temps, et un
   contexte ouvert à chaque processus.
5. **À K10, des descentes beaucoup plus coûteuses qu'en v10** (§ 3.2).

### 14.2 Pourquoi la reconstruction n'a pas dépassé la v10

- **La v11 est repartie de zéro sans porter d'emblée les mécanismes qui faisaient la vitesse de la v10.**
  - [I] Mécanismes concernés : partition T > 0, choix des k plus proches dans les descentes, mémo cellulaire daté,
    feuille J3 complète.
  - Elle a redécouvert certains d'entre eux en route : semis par population, réemploi vertical, q3 différé.
  - Le différentiel v10/v11 sur trames entières, qui aurait localisé l'écart tôt, n'a jamais été fermé.
- **Le temps de développement est allé à la qualification et aux sorties.** Fondations et portes ; mutants ; quatre
  sorties ; supports v1 puis v2 ; hiérarchie de points ; tête plate ; polyèdres. Ce sont des acquis réels, mais la
  vitesse n'est revenue sur le devant que le 4 octobre.
- **Les leviers de constante ont été jugés un par un.** Avec des seuils trop sévères pour leur taille d'effet et un
  bruit de ±10 % à W48, des gains cumulables de 3 à 15 % ont été perdus.

## 15. Leçons transversales

1. **Le contrat avant le code.** Les tranches qui ont posé contrat et oracle borné avant le natif (L0, S0–S1, V3, EOM
   exacte) ont été les plus sûres. Celles qui ont écrit le natif avant la réponse sur le contrat ont été reprises.
2. **Les témoins exacts minimaux valent plus que les campagnes.** Chaque défaut grave s'est vu sur un témoin de
   quelques sites, ou à une borne exacte (2^127−1, 2^20±1).
3. **Un invariant supposé est un défaut en attente** (« feuilles ≤ sites », bornes intermédiaires, profil unique).
   Toute garde doit être prouvée ou testée à sa borne.
4. **Le local ne prédit pas G4, et les instructions ne prédisent pas le temps.** Les compteurs déterministes servent à
   trier ; le temps se mesure sur la machine cible, en apparié.
5. **Le bruit à W48 impose un protocole statistique.** Plusieurs processus par bras, un ordre entrelacé, un bras A/A,
   des seuils tirés de la variance et de l'effet attendu, et une confirmation pour les petits gains cumulables.
6. **Une option qui disparaît en route fausse une mesure.** Il faut un témoin d'activation par prise (placements,
   GPU, cache).
7. **Un seul profil produit.** Trois profils compilés ont multiplié les défauts de qualification (références gravées,
   branches éliminées, largeurs d'export) pour un coût de vitesse de 5 % au plus.
8. **La qualification se dimensionne d'avance**, en lots de durée mesurée, sans jamais retirer une porte pour tenir
   l'échéance. Sa dette se tient commit par commit.
9. **Le GPU se conçoit de bout en bout.** Un noyau exact branché sur un pilote CPU qui l'attend ne rachète ni
   l'ouverture du contexte, ni le join, ni la sous-occupation.
10. **La sortie pivot doit exister tôt.** Le cycle SPv1 → SPv2 et l'export de masse manquant viennent de sorties
    conçues une par une, au lieu de vues d'un même registre d'événements.
11. **Le canal d'audit doit rester lisible** : un registre de constats à identifiants et à états, des notes vivantes
    courtes, des rôles stables, et une relecture avant adoption de tout changement de budget, de concurrence ou de
    format.

## 16. Corrections apportées aux rapports

- **Enveloppes M3/E4.** Le rapport A les dit retirées de la voie CPU. Elles sont toujours appliquées dans
  `src/catalogue/leaf.cpp` (`q3_of`, `q4_of`) : le retrait, approuvé par l'auditeur, n'a pas été fait (rapport B
  exact).
- **Réservoir chaîné.** Les notes du développeur le rangeaient parmi les leviers fermés. Il est gardé dans le code
  (`src/catalogue/leaf_batch.hpp`) et le reçu `reservoir3_chemin_chaud` le garde explicitement, critère non atteint
  (rapports B et E exacts).
- **Temps.**
  - Les rapports A et F citent K5 à 240–290 ms et K10 à 1,8–3,3 s, d'après une section antérieure du canal.
  - Les temps au gel sont ceux du § 3.1 : K5 à chaud 212 à 314 ms selon la trame et la voie, K10 à chaud 1,34 à
    1,78 s.
- **Comparaison à la v10.** Elle est refaite ici étage par étage depuis la table de vérité G4 de la v10 (§ 3.2). Les
  rapports ne comparaient que les totaux K5.
- **Constats vérifiés dans l'instantané** :
  - deux § 10.10 dans `docs/MATHEMATIQUES.md` (lignes 866 et 918) ;
  - formule `8C⌈C/64⌉` dans `docs/CATALOGUE.md` (l. 158), que l'auditeur veut à 16C ;
  - lien mort vers `audits/REPONSE_CLAUDE_POINTS_20261003.md` dans `receipts/audit_dialogues_20261004/README.md` ;
  - feuilles de 16 à tout K dans l'API (`src/api/compute.cpp`, l. 26).

## 17. Références

**Documents** (`docs/`) :
- [MATHEMATIQUES.md](MATHEMATIQUES.md), [ARCHITECTURE.md](ARCHITECTURE.md), [CONCEPTION_MOTEUR.md](CONCEPTION_MOTEUR.md),
  [PROVENANCE.md](PROVENANCE.md), [AUDIT_V10_SYNTHESE.md](AUDIT_V10_SYNTHESE.md) ;
- [CATALOGUE.md](CATALOGUE.md), [CATALOGUE_SINGLE_PASS.md](CATALOGUE_SINGLE_PASS.md),
  [CATALOGUE_FRONTIERE_ADAPTATIVE.md](CATALOGUE_FRONTIERE_ADAPTATIVE.md) ;
- [FULL_FORESTS.md](FULL_FORESTS.md), [PERFORMANCE_FULL.md](PERFORMANCE_FULL.md) ;
- [SORTIES.md](SORTIES.md), [SORTIE_PLATE.md](SORTIE_PLATE.md), [HIERARCHIE_POINTS.md](HIERARCHIE_POINTS.md).

**Reçus clés** (`receipts/`) :
- performance : `qualification_performance_20261003`, `developpement_20261003/pipeline_g4`,
  `developpement_20261004/gpu_g4`, `developpement_20261004/mesures_g4_ab8_diag1` ;
- qualification : `developpement_20261005/qualification_finale`, `developpement_20261006/v3_qualification` ;
- leviers : `developpement_20261006/*` et `developpement_20261007/*` ;
- audits : `audit_deep_20261004/performance`, `audit_plan_gpu_20261006`.

**Rapports bruts de cet audit** : `receipts/passation_20261007/`. Les notes de travail restées hors dépôt sont
listées au § 10 de la [passation](../PASSATION.md).

**Canal d'audit** : `audits/`, en particulier `AUDIT_CONTRATS_NUMERIQUES_ET_CAPACITE_20261002.md`,
`AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md` et `REPONSE_CLAUDE_SUPPORTS_20261004.md` (sections A à Y).
