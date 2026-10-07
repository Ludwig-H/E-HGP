# Rapport E — Sorties, hiérarchies dérivées, HGP contre HDBSCAN, applications (Zoltan), polyèdres d'ordre k

7 octobre 2026. Lecteur E sur huit, en lecture seule du dépôt à `e968aba8d` (moteur v11 gelé à `ac081a06f`).

```text
phase=exploration_v11_hors_registre (close)
backend=cpu_reference
profile=quantized_u21_input_only
public_status=not_claimed
```

**GCP non utilisé.** Aucune compilation ni mesure. Je me suis limité à des recomptes en mémoire de reçus existants, avec deux scripts du bloc-notes qui n'écrivent rien : `recompute_points_g4.py` (tableaux de `HIERARCHIE_POINTS.md` § 7) et `recount_bouts_e1.py` (bouts, étude E1, antichaîne oracle). Aucune donnée KITTI n'a été recopiée.

**Légende.**
- **[V]** : vérifié dans le code, dans un reçu du dépôt, ou recalculé depuis un reçu du dépôt.
- **[V-hd]** : vérifié sur une pièce hors dépôt (`build/v11-persist/`, non versionnée).
- **[D]** : déclaré par un document du dépôt, non recalculé.
- **[M]** : note de mémoire du développeur.
- **[I]** : inférence.

Le `PASSATION.md` et l'`AUDIT_FINAL_V11.md` sont traités comme des affirmations à vérifier. Le § 7 liste les corrections et compléments que je leur apporte.

---

## 0. Verdict en dix points

1. **Le socle de sortie est solide.** La chaîne `io`/`api`/`cli` fournit :
   - un dossier transactionnel avec manifeste écrit en dernier et `renameat2(RENAME_NOREPLACE)` ;
   - les codes 0, 2 et 3 ;
   - la signature `tree_k_sha256` v2, commune aux quatre sorties.

   Elle est qualifiée à `98a009550` [V].

   **Mais les trois sorties dérivées sont moins qualifiées que `full`.**
   - SPv2 (`supports` de Kruskal) n'a passé que 15/15 portes, en Release u21 seulement [V].
   - Les différentiels de `points` et `plat` n'ont pas été rejoués depuis la garde `b0f2a0a9e` [V].
   - Aucune des trois n'a de coût mesuré sur G4 [V].
   - `points` et `plat` passent encore par la voie lente `build_order` (masque 7035), et non par L2b [V].
2. **HGP bat HDBSCAN au niveau B (meilleur bloc) sur synthétique.** C'est établi et reproductible : je recalcule à l'identique +0,008 / +0,026 / +0,050 / +0,078 à n = 8 000 [V].
   - Ce gain vient de l'entrée par couverture de la tour, pas de la marge.
   - Il n'est pas attribué à la connexité d'ordre supérieur, faute de bras MR_k-bord.
3. **Sur LiDAR, l'avantage au niveau B est petit et concentré.**
   - Il porte sur les vélos et les piétons, dans des cohortes enrichies en échecs de HDBSCAN.
   - Sur les trames témoins, les deux méthodes sont à parité [V].
   - Sur un échantillon représentatif (v10), l'estimateur pondéré vaut +0,0002 à K5 [V].
   - Le criblage borne la marge : HDBSCAN n'échoue au niveau B que pour environ 1,1 % des instances à K5 [V].
4. **Au niveau de la sélection plate, la supériorité de HGP n'est pas établie.**
   - Synthétique : le gain vient de la tête certifiée, qui s'applique aussi à l'arbre de `sklearn` [V-hd].
   - LiDAR : le « 68,5 % contre 56,6 % » est vrai [V], mais il est mesuré sur une population conditionnée au succès de la hiérarchie HGP, et son écart est porté à 87 % par des découpes de voitures, surtout à k = 2 et 3 [V-hd].
   - Aucun test préenregistré décisif n'a été mené jusqu'au bout.
5. **Plusieurs chiffres de l'audit final sont à corriger ou à nuancer** (§ 7) :
   - « cibles Q2–Q4 perdues : 70 cellules sur 125 » : c'est l'inverse, 70 tenues ;
   - « trois vélos sauvés » : ce sont deux vélos physiques ;
   - « 13 gains / 3 pertes » : la règle de priorité gonfle les gains ;
   - le seuil relatif « demandé » par Zoltan : la doctrine Zoltan est en fait neutre.
6. **La v11 ne livre aucun des deux exports dont le pilote Zoltan a besoin**, `coverage_v1` et `weighted_gabriel_v1` [V].
   - Avec SPv2, la masse du § 9.1 est incalculable.
   - S\* dépend du rang de Morton, donc il n'est pas invariant par translation [V].
   - L'arbre d'ordre K brut ne fait pas un jeton : 14,45 nœuds par site, et 61 à 67 % des nœuds vivent moins de 2δ.
7. **Le polyèdre d'ordre k est un objet mathématiquement fixé et vérifié** : A_k(r), filtré par d_k, sans aucun désaccord avec FULL.
   - Il coûte environ 1 000 faces par site à K5, soit 62 à 82 fois la tour.
   - Il relève donc de l'aval, à la demande. Huit questions à l'auditeur restent ouvertes.
8. **Le contrat de 100 ms est ambigu pour les applications.**
   - Pour la segmentation, le produit utile coûte à lui seul, hors tour et en local à W8, environ 0,4 s pour `plat` et 0,75 s pour `points` [D].
   - Pour Zoltan, ce sont le débit et le stockage qui comptent, pas la latence [I].
9. **Le registre d'événements unique est la bonne architecture de sortie** pour la v12 [I], appuyé sur ce que la v11 a payé : SPv1 puis SPv2, export de masse manquant, chemins de calcul divergents.
   - Son budget de 10 ms n'est soutenu par aucune mesure.
10. **Les décisions de l'utilisateur en attente conditionnent la v12 des sorties.** Elles sont listées au § 6.4.

---

## 1. Applications et enjeux

### 1.1 À quoi sert la tour

**(a) Clustering hiérarchique robuste et multi-échelle (HGP-Clusterer).**
- La thèse (Théorème 2) identifie les K-polyèdres aux amas discrets de forte densité de l'estimateur K-NN.
- Le § 9.1 (manuscrit, p. 96–97) prescrit, sur un recouvrement :
  - une condensation « comme HDBSCAN » au seuil `min_cluster_size` pris sur la masse m_τ = S_τ Σ_{x∈τ} 1/T_x ;
  - puis un vote pondéré.
- Lu dans le PDF [V] : « C'est ce poids m_τ, et non le simple comptage des faces, qui est utilisé par le seuil min_cluster_size dans l'arbre condensé » (p. 97).
- L'Algorithme 1 (p. 100) condense le Kruskal T₂ avec ce seuil.
- La comparaison de la thèse à HDBSCAN (SIPU, p. 101) garde mcs = √n, ρ̂ = 1/r et ψ(t) = 1/t : « la seule différence est le type de connexité ».

**(b) Segmentation d'instances LiDAR (SemanticKITTI sans sol).**
- C'est la vitrine des démos (`Zoltan/demos/`).
- Consignes de l'utilisateur du 4 octobre [M] : « découper plutôt que fusionner » sur LiDAR, « retrouver exactement les classes » sur synthétique (`docs/SORTIE_PLATE.md:90-96`).

**(c) Jetons et enseignant d'un modèle de fondation 3D (Zoltan, HGP-FM).**
- La thèse : substituer l'échelle canonique de la tour à l'échelle métrique codée en dur des encodeurs (voxel, rayons, patch), un composant à la fois (`Zoltan/FoundationModel/README.md`, `MESURE.md` § 1).
- Deux pilotes indépendants (`PLAN.md:9-17`) :
  - **FULL enseignant** : PTv3 inchangé, perte de connexité K1 à des rayons interrogés ;
  - **FULL tokenizer** : HGP-UNet, pooling de filtration, matrices § 9.1.

**(d) Forme.** Polyèdres reconnaissables (roue, vélo, piéton), demandés le 6 octobre pour le jeton polyédrique (§ 5).

### 1.2 Ce que chaque application exige de l'objet

| Usage | Sortie consommée | Exigences | Ce que la v11 fournit |
|---|---|---|---|
| (a) Clustering | hiérarchie laminaire de points, condensation, sélection ; ou recouvrement et vote § 9.1 | fidélité à FULL ; multifusions N-aires jamais binarisées ; masse § 9.1 pour mcs ; latence non critique | `points` ($H^{r}_{K+1}$) et `plat`, mais condensation au **comptage entier** (critère A), pas à la masse § 9.1 (`src/head/head.hpp:19-23`) [V] |
| (b) Segmentation | partition plate | PQ ; découpe plutôt que fusion ; 10 Hz ; contacts (sol, murs) ; objets minces à petit K | `plat` (EOM z = 1, mcs 20 par défaut) ; coût aval non mesuré sur G4 ; contacts avec un mur non séparables (plafond FULL < 1/2, démos 02 et 03) [V] |
| (c) Enseignant (A0G1) | `coverage_v1` : états datés `(tower_digest, K, segment, niveau, côté de coupe…)`, cartes à la coupe, unions de sites, censure (`CONTRAT_COUPES_ET_MASSES_20260926.md:17-35`, `PLAN.md:19-33`) | exact ; déterministe ; recalculé **par vue augmentée** (la quantification casse l'équivariance) ; mis en cache par empreinte | **non livré.** `MHGP11FUL1` contient la tour, mais pas les populations par état daté ; la fixture growth_ABCZ n'est pas couverte [V] |
| (c) Tokenizer (A1) | `weighted_gabriel_v1` et CutBundle : incidences coface–facette, S_τ, T_x, masses, réserves, condensation N-aire, verticales naturelles, départage par clé canonique et jamais par PointId (`SPECIFICATION.md` § 2–3) | environ 160 000 unités par branche (`ARCHITECTURE.md:38-57`) ; composition P_g = P_f Q | **non livré** ; impossible depuis SPv2, qui n'a ni I_b, ni U_b, ni boules internes [V] |
| (d) Forme | A_k(r) par événements (propriétaire, date) | topologie certifiée, petit, robuste | prototypes Python seulement (§ 5) |

`coverage_v1` et `weighted_gabriel_v1` n'apparaissent nulle part dans `morsehgp3D_v11/docs` ni dans `src/` (grep) [V]. Il n'existe pas non plus d'exportateur vers `CertifiedTowerInput` de `morsehgp3d/`, que `CLAUDE.md:134` désigne comme le chaînon manquant [V].

### 1.3 Pourquoi 100 ms, et pour quoi faire

**Le contrat.** C'est un contrat de l'utilisateur : « 100 ms sur nuages LiDAR sans sol, K = 5 et si possible K = 10 » (`AGENTS.md:9-10`, `README.md`). Il est hérité de la v9 : « nuages de 30 000 à 60 000 points » (`AGENTS.md:53-56`).

**Sa justification.** Aucun document ne la donne. [I] 100 ms est la période d'une trame d'un LiDAR tournant à 10 Hz, comme celui de KITTI. La passation pose bien la question froid/chaud « à 10 Hz » (`PASSATION.md:195-197`), et une note v10 dit : « pour un flux LiDAR à 10 Hz, la session résidente est le cas d'usage » (`receipts/notes_hors_depot_20261007/audit_transpositions/fouille/v10_moteur.md:288`).

Trois remarques que la passation ne fait pas :

1. **Latence ou débit.** [I] Un flux à 10 Hz exige un débit de 10 trames par seconde, pas une latence de 100 ms. On pourrait traiter plusieurs trames à la fois avec une latence plus grande, si les ressources le permettent. Les mesures à W48 consacrent toute la machine à une trame : la question est à poser à l'utilisateur, à côté de froid/chaud.
2. **Le produit utile à la segmentation n'est pas FULL.** Mesures locales du 6 octobre (codespace, W8, ng00, K5 ; `receipts/notes_hors_depot_20261007/gpu_optim/carte_aval.md`, [D], une prise par configuration), **hors tour** :
   - `points` : `attach` 102–109 ms, `output` 171–177 ms, `write` 478–496 ms ;
   - `plat` : `attach` 101–118 ms, `output` 226–239 ms, `write` 80–83 ms, ce dernier presque entièrement fait du calcul de `tree_k_sha256`.

   Sur G4 [V] (`qualification_sorties/claudequalmesure/sorties_g4.json`, W48, ng00 K5), l'écriture est strictement sérielle :
   - `attach` 75,4 ms ;
   - écriture de `full` : 2,16 s pour 300,9 Mo ;
   - écriture de SPv1 : 0,22 s pour 30,8 Mo.

   Même avec une tour gratuite, `plat` ne tient pas en 100 ms. Le périmètre du contrat (`PASSATION.md:199-200`) est donc décisif.
3. **Pour Zoltan, la latence n'est pas l'enjeu.** La tour se met en cache (`OBJET.md:154`) et se recalcule par vue augmentée. Ce qui compte, ce sont le débit et le stockage.
   - [I] À 300 883 482 octets par trame en `MHGP11FUL1` à K5 (ng00), une passe sur les 19 130 scans d'entraînement de SemanticKITTI pèserait environ 5,8 To ; à K10 (1,46 Go par trame [V]), environ 28 To.
   - Le format, et non le temps de la tour, est le verrou de cette application.

### 1.4 Ce que le criblage dit de l'enjeu LiDAR

Sur les 299 trames du criblage 1 sur 8 de la séquence 08, HDBSCAN (meilleur nœud, `min_samples` = K) ne trouve aucun nœud d'IoU supérieur à 1/2 pour (`Zoltan/demos/recherche/README.md:60-71`, [V]) :

| Classe | Instances | Échecs à K5 | Échecs à K10 |
|---|---:|---:|---:|
| voitures | 2 188 | 0 | 0 |
| vélos | 138 | 22 (16 %) | 36 (26 %) |
| piétons | 231 | 7 | 6 |
| **toutes classes** | **2 918** | **33 (1,1 %)** | **47 (1,6 %)** |

[I] Le gain possible au niveau B est donc borné à quelques pour cent des instances, sur une classe surtout : les vélos garés. L'essentiel de l'enjeu pratique est dans la **sélection**, et dans les objets en contact, que la densité ne sépare pas.

---

## 2. Les quatre sorties

**Socle commun** [V] :
- `cli/mhgp11.cpp` n'inclut que `api/api.hpp`, avec `--sortie` obligatoire et 1 ≤ K ≤ 12 ;
- dossier transactionnel (`src/io/directory.cpp`) ;
- manifeste canonique (`src/api/manifest.cpp`) ;
- une seule ligne JSON par exécution ;
- `tree_k_sha256` v2 (`docs/SORTIES.md:640-669`), égal entre les sorties d'une même entrée et d'un même K.

**Paramètres moteur de l'API** (`src/api/compute.cpp:23-61`) [V] :
- feuilles de 16 et `max_leaf` 256 à tout K ;
- masque 278 523 : base 16 379 plus placement O1 ;
- sans GPU ni cache de blocs.

Le commentaire d'en-tête du fichier, « masque 16379 », est périmé.

**Les deux voies de calcul de l'arbre d'ordre K** (`compute.cpp:122-140`, `163-206`) [V] :
- `supports` utilise `build_order_full` : voie L2b, journal des graines posé sur FULL ;
- **`points` et `plat` utilisent encore `build_order`** au masque 7 035, sans ordres concurrents.

Or la règle de L2 a mesuré `build_order` plus lent que FULL à W48 : rapports de 1,21 / 1,18 / 1,09 sur l'étage `tree` (`receipts/developpement_20261005/qualification_sorties/README.md`). L2b n'a été appliquée qu'aux supports. Ce point manque à l'audit final.

### 2.1 `full` : tour FULL, format `MHGP11FUL1`

**Objet.** Pour k = 1..K :
- T_k est l'arbre de fusion de π₀(L_k(a)), avec a le rayon carré exact non réduit, comparé par produits croisés ;
- les fusions sont N-aires, à plateau atomique, avec une racine unique ;
- la numérotation est canonique :
  - naissances par (niveau, centre exact) ;
  - fusions par (niveau, plus petite naissance) ;
- la verticale envoie chaque nœud d'ordre k sur le nœud d'ordre k−1 vivant à la coupe fermée.

**Format** (`SORTIES.md:276-293`). C'est le vidage `serialize` de la sonde, inchangé. Il contient :
- les sites, avec leurs PointId ;
- par ordre : le parent, les enfants, le niveau exact, le centre exact de chaque naissance et la verticale.

Il ne contient **ni population par nœud, ni S\***, si bien que `tree_k_sha256` ne s'y recalcule pas (`SORTIES.md:656`).

**Garanties.**
- Exacte ; octets identiques quel que soit W ; invariante par permutation et réétiquetage.
- Porte `mhgp11_cli_full_identity` : le CLI rend le même SHA-256 que le banc.

**Qualification** : `qualification_finale` à `98a009550` [V].

**Coûts** [V] (`sorties_g4.json` de `claudequalmesure`, source `b319efc84`, ancienne voie 16 379) :

| ng00, W48 | Fichier | Écriture | Total |
|---|---:|---:|---:|
| K5 | 300,9 Mo | 2,16 s | 2,56 s |
| K10 | 1,46 Go | 10,9 s | 14,4 s |

- Le débit d'écriture apparent est constant, de 134 à 150 Mo/s. La cause probable est un SHA-256 logiciel sériel, sans SHA-NI (`carte_aval.md`, [D] ; mesure locale du SHA-256 maison : 246–262 Mo/s, contre 1 527 Mo/s pour OpenSSL avec SHA-NI).
- Le « tampon fixe de 64 Kio » de `SORTIES.md` § 9 est inexact sous glibc : `setvbuf(file_, nullptr, _IOFBF, kBufferBytes)` (`src/io/writer.cpp:69`) laisse glibc choisir un tampon de `st_blksize`. On observe 73 460 appels `write` de 4 096 octets [D].

### 2.2 `supports` : arbre couvrant d'ordre K par Kruskal, format `MHGP11SP` v2

**Objet** (`MATHEMATIQUES.md:918-940`, `SORTIES.md:295-324`) [V] :
- toutes les naissances ;
- parmi les boules de fusion d'un nœud, parcourues dans l'ordre `BallIdx` (rang, puis S\*), celles qui réalisent au moins une union dans un DSU des enfants (`src/supports/hierarchy.cpp:89-134`) ;
- un seul support par boule, S\*, de plus petite arité, départagé par les `SiteIdx` de Morton.

Une hyper-arête qui réunit c ≥ 3 branches est gardée une fois, avec son `ant(b)`. Les lemmes A–H, prouvés sans position générale, établissent que naissances et fusions suffisent à reconstruire T_K.

**Format.** Sections `SITES`, `NODES`, `BALLS` (rang, `prior_count`, rôle, p, m), `SUPPORTS` (arité et sites de S\*) et `PRIOR`. Seul plafond : m ≤ 255.

**Comptes** [V] :
- ng00 à K5 : 576 388 boules pour 576 371 nœuds, contre 789 886 boules et 789 889 supports en v1 ;
- uniforme 32 000 : boules = nœuds = 1 163 756 (`receipts/developpement_20261006/supports_kruskal/README.md`).

Le sélecteur de Kruskal retire 94 boules sur ng00 par rapport au filtre de rôle : 576 482 → 576 388 (`audits/REPONSE_CLAUDE_SUPPORTS_20261004.md`, sections R et S) [V]. Les chiffres 90 et 523 avancés pour ng01 et ng02 n'ont pas de source retrouvée.

**Ce que la sortie n'est pas** (déclarations obligatoires, `MATHEMATIQUES.md:840-865`) [V] :
- elle est instable aux cosphéricités : sur le cercle à quatre points, le saut de Hausdorff vaut au moins 1/4 ;
- ce n'est pas le K-polyèdre ;
- elle omet les sites intérieurs ;
- elle ne dessine pas l'objet : 6 ouvertures gardées sur 44, et 2 441 simplexes par nœud d'objet, d'après `notes_hors_depot_20261007/polyedres_reconnaissables/SYNTHESE.md` [D].

Compléments :
- **S\* n'est pas invariant par translation.** Son départage passe par le rang de Morton (`SORTIES.md:764-768`). [I] Un départage lexicographique sur les coordonnées le serait.
- `JETON.md:50-56` avertit qu'« un support représentant n'est pas une géométrie canonique » : les deux diagonales du carré certifient la même boule. SPv2 est donc un **témoin de connexité** (H₀), pas une forme.

**Qualification** [V] : 15/15 en Release u21 (session `claudesupkr`, `07428324e`).
- Les quatre mutants `sp_*` n'ont pas été joués sur G4 : la session `claudesupkr` ne joue que les 15 portes, et seule la note S les déclare tués.
- Pas de profils u18 ni u24 sur G4, pas de sanitizer dédié, pas de porte K10 du CLI.
- Il existe en revanche des portes K10 de la bibliothèque, en sélection `all` (v1) : `mhgp11_supports_hierarchy_lidar_ng00_k10` (`tests/supports/tests.cmake:169-170`).

**Coût.**
- SPv2 n'est pas mesuré sur G4.
- SPv1 sur ng00 à K5, W48 : 0,75 s au total, dont `attach` 74 ms (sériel), assemblage 51 ms et écriture 220 ms [V].
- Après L2b, l'étage `tree` vaut 1,045 / 0,999 / 1,055 fois celui de FULL (`qualification_finale/README.md`) [V].

### 2.3 `points` : $H^{r}_{K+1}$, format `MHGP11PT` v1

**Objet** (`HIERARCHIE_POINTS.md:20-45`, `98-110`) : P₁∘Π_{K+1}, avec :
- m(K) = K+1 et m(1) = 1, κ = 1 (`src/points/points.hpp:31-33`) [V] ;
- un site entre à e_i = t_i + D_i, dans le propriétaire o_i, ancêtre vivant à e_i en coupe fermée ;
- des dates de la forme √t + √M − √Q, décidées exactement : classes de carrés, puis encadrements entiers, sinon refus `radical_sign_budget`.

**Format** (`SORTIES.md:517-555`) : `LEVELS` exacts non réduits, `NODES`, `HANGING` (t, M, Q, propriétaire, plancher, drapeau strict), `TREE` (plateaux, blocs, `site_block`, `site_plateau`).

**Garanties H1–H7, prouvées** (`HIERARCHIE_POINTS.md:126-134`) :
- fidèle, laminaire, indépendante de mcs, liaison simple à k = 1 ;
- stable en 3ε, dates et hauteurs, sous perturbation appariée ;
- au plus 2,28ε mesuré sur 502 488 paires [D].

**Prix**, également prouvés :
- les entrées sont dans [α_{K+1}, α_{K+1} + d_K/2] ;
- le cœur n'est pas respecté à k = 2 ;
- une structure isolée de k sites n'est jamais un bloc ;
- aucune stabilité par insertion ;
- les blocs de deux ordres peuvent se croiser.

**Qualification** [V] :
- porte stricte `claudepts6` : 2 854 nuages, 215 974 sites comparés exactement, 12/12 fixtures, 4/4 mutants ;
- natif égal à Python et à l'oracle (S9, `claudefinp9`).

Rien n'a été rejoué après `b0f2a0a9e`, qui ajoute des gardes de chronologie dans le lecteur et dans `head::check_shape` (`git show b0f2a0a9e`).

**Coût** : non mesuré sur G4 ; local W8 au § 1.3.

### 2.4 `plat` : EOM exacte, format `MHGP11ET` v1

**Objet** (`SORTIE_PLATE.md:17-25`, `176-192` ; `src/head/head.hpp:1-45`) [V] :
- **condensation au critère A** : un bloc est gros s'il compte au moins mcs sites engagés ;
- EOM N-aire à φ(r) = r^{−z}, avec z ∈ {1, 2, 3}, ou sélection des feuilles ; racine exclue ;
- le parent l'emporte seulement sur égalité **certifiée** ;
- encadrements entiers sur 384 bits, puis repli exact par sommes de radicaux ; au-delà de 4 096 termes, refus de l'appel entier ;
- étiquettes dans l'ordre d'entrée : plus petit PointId du cluster, ou −1.

**Valeurs par défaut** : EOM, z = 1, mcs 20. C'est la ligne LiDAR choisie sur des exemples de développement (§ 3.4).

**Qualification** [V] :
- différentiels S10 (`claudefinp10`) : natif égal à Python sur 120 cas, 1 920 appels et 16 474 clusters ;
- les trois trames ;
- mutants `head` 10/10 en u18.

Gardes de `b0f2a0a9e` non rejouées.

**Coût** : local seulement ; 0,2 à 0,27 s pour `output` en W4 (`SORTIE_PLATE.md:204-206`) [D].

**Ce que la v11 n'a pas fait** : la masse § 9.1, l'arbre `condense` publié, la vue à trois niveaux (jetons, groupes, parties).

---

## 3. HGP contre HDBSCAN : chiffres vérifiés, portée, jugement

### 3.1 Vérification chiffre par chiffre

| Affirmation (audit final ou mémoire) | Source | Statut | Nuance indispensable |
|---|---|---|---|
| Niveau B synthétique, n = 8 000 : +0,008, +0,026, +0,050, +0,078 (IC > 0) | `developpement_20261003/points_g4`, sessions `claudepts4` et `claudepts6` | **[V]**, recalcul identique, intervalles compris | 64 scènes par k ; seulement 41/64 scènes ≥ HDBSCAN à k = 2. `first`, sans marge, fait mieux : +0,010 / +0,030 / +0,055 / +0,084. Niveaux medium et hard **calibrés sur l'ARI de HDBSCAN** (`receipts/full_points_20261003/experiment/vendor_scenes.py:21-47`). À k = 5 : medium +0,044, hard +0,056 [V] |
| Trames voisines, 71 scènes, 859 instances, sauvetages/pertes +19/−8, +22/−7, +27/−4, +35/−1 | `claudepts6` | **[V]** | Voisines des trames d'échec, donc corrélées et enrichies. **Témoins** (20 trames) : 0 sauvetage, 0 perte ; IoU 0,973 contre 0,977 en faveur de HDBSCAN à k = 2, à peu près égal ensuite. v10 pondéré sur 299 trames : +0,0002 à K5, intervalle contenant 0 (`morsehgp3D_v10/receipts/audit_independant_20261002/battery_review/README.md:7`) [V] |
| EOM z = 1 : 68,5 % contre 56,6 % (`sklearn`) des couples (scène, k) | `developpement_20261004/e1_sortie_plate/etude/etude_table.json` | **[V]** (0,6854 contre 0,5663 ; A = 0,5636) | Population **conditionnée au succès de $H^{r}$** : tous les objets ont un bloc d'IoU > 1/2 (`bench/points_flat_study.py:16-20`). Les lignes `sklearn` y sont **descriptives**. Par k : 0,604/0,418, 0,650/0,448, 0,709/0,636, **0,779/0,765**. 74 % des couples sont des découpes de **voitures** seules (1 080 sur 1 462), qui portent 152 des 174 couples d'écart ; à k = 10, `sklearn` y fait mieux (0,785 contre 0,774). Vélos et piétons à k = 5 : 0,714 contre 0,704. **31 bouts comptés deux fois** (lots 1 et 2) [V-hd, `build/v11-persist/e1_s0/study_all.json`, dont l'agrégat égale le reçu] |
| Antichaîne oracle 252/258, EOM z = 1 198 | `etude/oracle_lot2.json` | **[V]** (mcs 20 ; 254 à mcs 10, et EOM 150) | Unité : couples (bout, k) sur 31 bouts organisés, pas des objets distincts ; démos : 244/246 contre 194 |
| Vélo 3 séparé seulement entre 107,4 et 107,8 mm ; union jusqu'à 456,9 mm | `SORTIE_PLATE.md:111-117` | [D] | Rapport d'échelle 1,004 [M] |
| 13 gains / 3 pertes sur 360 bouts | `Zoltan/demos/bouts_evalues.json` | **[V]** | Catégorie « ∃ k », avec la priorité gain > perte. **Par ordre** : k = 2 : 4/2 ; k = 3 : 5/2 ; k = 5 : 10/1 ; k = 10 : 7/0. Deux « gains » ont aussi une perte à k = 2 (`b00_001466`, `b08_001182`). Bouts réduits aux seuls points des instances. Trois gains viennent de la même scène (06/000800), et 00/001466, 001470 et 001472 voient la même rangée de vélos |
| Vidéos, 97 groupes : instances 10/1 (k5), 7/0 (k10) ; sans sol 8/9 (k5), 25/0 (k10) | `Zoltan/demos/videos_hgp_hdbscan/README.md:56-70` | **[V]** | La variante **sans sol est calculée en local seulement** ; seules les instances sont recoupées à G4 (388/388). Sans sol à k = 5 : réussites HDBSCAN 9 + 45 = 54, contre 53 pour HGP. Les 9 pertes viennent d'une seule rangée (08/001180–001182). 31 groupes gagnants pour 12 scènes |
| « Trois vélos sauvés dans les démos » | `HIERARCHIE_POINTS.md:288-296` | **[V]** avec correction | L'instance 55 est comptée dans 01 et 04, qui sont la **même trame** (08/001176) sans puis avec sol : ce sont **deux vélos physiques**. Au niveau de chaque démo, 01 à 04 restent **« les deux échouent »** à tous les ordres (`Zoltan/demos/hgp_echoue_hdbscan_echoue/*/resultats_hgp.json`) |
| Test synthétique préenregistré S2b : T_eom2 − R0 = +0,041, +0,045, +0,045, −0,002 | `build/v11-persist/e1_s2/summary_test_8k16k.json` | **[V-hd]**, pas de reçu | Terme de hiérarchie T − A : +0,0015, +0,0012, −0,0042, **−0,037**. Terme de sélection A − R0 : +0,039 à +0,049. Garde PQ significativement pire à k = 5 (−0,018, p = 4·10⁻⁵) et à k = 10 (−0,066). R1(5) et R1(10) échouent selon la règle écrite |
| 520 configurations sur 2 400 où la tête N-aire appliquée à l'arbre de `sklearn` diffère de `sklearn` | `receipts/flat_selection_evidence_20261004/README.md:9` | **[V]** | `sklearn` 1.9.1 local, 150 petits arbres de 30 à 200 points, ε = 0. `sklearn` peut publier des clusters absents de tout niveau atomique (`flat_evidence_followup_20261004/README.md:20`) |

### 3.2 Niveau A (tour) et niveau B (hiérarchie)

**Synthétique** [V].
- L'avantage au niveau B croît avec k : +0,008 à k = 2, +0,078 à k = 10, à n = 8 000. Même profil à n = 2 000.
- Il est **commun à toutes les règles fidèles** sauf `core` : `first` +0,084, `cover` +0,083 à k = 10.
- La marge (κ = 1) coûte de 0,002 à 0,006 au meilleur bloc : c'est le prix de la stabilité en 3ε.
- [M] La v10 avait mesuré l'écart B − A (perte par rapport à la tour) : cover −0,012, HDBSCAN −0,063 à K5.

**LiDAR** [V].
- Gain modeste dans les cohortes choisies pour leurs échecs : voisines +0,011 en IoU moyen à k = 5 ; échecs du criblage +0,009.
- Nul sur les témoins.
- Les vélos contre une façade (démo 02) et le piéton de la démo 03 ne sont rattrapés par aucune règle : le plafond de FULL reste sous 1/2. C'est la limite de densité (`ARCHITECTURE.md:266-318`), pas un défaut de projection.

**Attribution** (`HIERARCHIE_POINTS.md:303-307`) : le gain vient de **l'entrée par couverture**.
- MR₂-bord, sans tour, rattrape aussi le vélo C.
- [M] La v10 trouvait la tour égale à MR₂-bord, à même entrée et même tête, sur la famille « objet ».
- [I] La contribution propre de la connexité d'ordre supérieur n'est donc **pas isolée** : le bras MR_k-bord prévu par E1 n'a jamais tourné.

### 3.3 Niveau C (sélection)

**Synthétique.**
- Choix sur le dev (192 scènes, `e1_sortie_plate/README.md`) [V] : z = 2 (mIoU 0,618), devant z = 3 (0,588) et z = 1 (0,567).
- Écarts du dev contre `sklearn` : +0,039, +0,058, +0,039, −0,008. La même tête sur l'arbre de `sklearn` gagne +0,033 à +0,051, et la hiérarchie de la tour **perd** à k = 10 (−0,043).
- Le test S2b le confirme [V-hd] : **le gain plat vient de la tête certifiée (z = 2, EOM N-aire, égalités exactes), qui se transpose à HDBSCAN.**

**LiDAR.**
- Sur la population conditionnée, T_eom1 bat `sklearn` surtout aux petits k, où `sklearn`, avec `min_samples` = 2 ou 3 et mcs 20, ne rend pas les voitures entières : 0,326 contre 0,567 à k = 2 sur les voitures [V-hd].
- À k = 10, quasi-parité : 0,779 contre 0,765.
- Sur les vélos, les fusions baissent nettement à k = 5 et 10 : 16 contre 26, et 14 contre 24 [V-hd].
- Sur trames entières (19 objets), d'après `polyedres_reconnaissables/SYNTHESE.md` § 3.1 [D] :

  | PQ | K5 | K10 |
  |---|---:|---:|
  | tête plate v11 | 0,845 | **0,769** |
  | HDBSCAN | 0,756 | **0,781** |

  Échantillon minuscule, absent de l'audit final.

**Plafond.** L'antichaîne oracle (252/258 contre 198) montre que la hiérarchie contient presque tout. La perte est dans la sélection : séparations fugaces.

### 3.4 État du préenregistrement E1 (`plans/e1_prereg_*_20261004.json`)

**Synthétique** (écrit le 4 octobre à 11 h 17 Z). Ligne primaire T_eom2, règle R1/R2 de Holm, conditions de gardes et de tailles, prédictions PS1–PS8.

| Session | Contenu | État |
|---|---|---|
| S1a, S1b (dev) | dev 8 000 et 16 000 points | faits, reçu `e1_sortie_plate` [V] |
| S2b (`claudeflat2b`, test 8k et 16k) | test préenregistré | fait, résumé seulement hors dépôt [V-hd], **sans reçu** |
| S3a (32k, moitié) | test préenregistré | dans `claudeab8`, **sans reçu** [M] ; `build/v11-persist/e1_s3a` ne contient que `data` et `plan.json` [V-hd] |
| S3b | test préenregistré | **jamais lancée** (`e1_s3b` : données et plan seulement ; `e1_s3b2` vide) [V-hd] |

La condition 3 de R1, « estimation > 0 à chacune des trois tailles », est donc **inévaluable**.

**LiDAR P08** (écrit le 4 octobre à 10 h 53 Z).
- Population : 141 trames de la séquence 08 au pas de 16, hors criblage et voisines.
- Hypothèses :
  - H_L1 : T_eom2 fusionne moins que R0 ;
  - H_L2 : T_eom1 non inférieur à R0, marge 0,02, IC en grappes.
- Holm sur {H_L1, H_L2} × {k = 5, 10}.
- Les sessions S6 et S7 ne contiennent que des données et un plan (`build/v11-persist/e1_p08/{s6,s7}`) : **jamais lancées** [V-hd].
- Le corrigendum du préenregistrement manque (rapport F) : la règle H_L2 a été durcie en `723cf6e43`, p de Holm **et** borne d'IC.

**Conclusion.** Les chiffres « HGP contre HDBSCAN » de la v11 sont des **exemples de développement** ou des **analyses descriptives à critères écrits d'avance** : points_g4, étude E1, bouts, vidéos. Le seul test préenregistré exécuté, S2b, est synthétique, sans reçu, et favorable à la tête plus qu'à la hiérarchie.

### 3.5 Fragilités du banc HDBSCAN

- `np.argsort` est instable dans `_process_mst` : les étiquettes dépendent de la machine. [M] La v10 voyait 28,5 à 52,3 % d'étiquetages différents ; les moyennes changeaient de moins de 0,003.
- `sklearn` binarise les plateaux ; la tête N-aire sur son arbre en diffère sur 520 configurations sur 2 400 (§ 3.1).
- Les versions 1.7.2 (G4) et 1.9.1 (codespace) n'ont jamais été qualifiées sur un même banc.
  - Les campagnes G4 (points_g4, bouts, `claudeflat0`) utilisent 1.7.2.
  - L'étude E1 recalcule les R0 de mcs ≠ 20 en local : « jamais une sortie officielle » (`points_flat_study.py:33`).
  - Les vidéos sans sol sont entièrement locales.
- **IoU arrondi avant le seuil de 1/2.** La correction (`38faaf272`, `bench/points_flat_study.py`) est **postérieure** à `etude_table.json`, qui date du 4 octobre. [I] L'impact se limite aux IoU à moins de 5·10⁻⁵ de 1/2.
- [I] La convention de niveau (facteur 2 entre rayon HGP et atteignabilité HDBSCAN) ne change ni le niveau B (maximum sur tous les blocs) ni une EOM à φ = r^{−z} : une homothétie globale multiplie tous les scores par une même constante. Elle n'affecte que ε et l'alignement visuel des vidéos.
- **Séquence 08.** La v11 a choisi z, mcs et la ligne LiDAR sur des trames et des découpes de la séquence 08. Or `Zoltan/FoundationModel/MESURE.md:45-52` réserve la 08 au bilan. P08 exclut les trames du criblage, mais reste dans la même séquence.

### 3.6 Jugement

- **Au niveau de la tour et du meilleur bloc, la supériorité est établie sur synthétique.** Écarts appariés positifs, intervalles par bootstrap hors de 0, reproduits par deux sessions G4.
  - Elle n'est pas attribuée à la connexité d'ordre supérieur : entrée par couverture contre cœur.
  - Elle est mesurée sur des niveaux de difficulté définis par l'échec de HDBSCAN.
- **Sur LiDAR, elle est faible et locale.**
  - Les « sauvetages » portent sur une minorité d'instances, des vélos, dans des cohortes enrichies.
  - Parité sur les témoins et sur l'échantillon pondéré de la v10.
- **Au niveau de la sélection, elle n'est pas établie.**
  - Synthétique : la tête fait le gain, et la hiérarchie perd à k = 10.
  - LiDAR : population biaisée et gain porté par les petits k ; quasi-parité à k = 10 ; PQ inférieur à HDBSCAN à K10 sur trames entières.
  - Les tests décisifs (P08, S3b) n'ont jamais été menés.
- [M] La directive « HDBSCAN ne peut pas battre la tour » reste **compatible** avec ces données au niveau A/B. Les données v11 montrent pourtant HDBSCAN égal ou devant dans plusieurs réglages : témoins, sans sol à k = 5, PQ K10, terme de hiérarchie à k = 10.

---

## 4. Ce qui n'a pas marché, et pourquoi

| Essai | Constat | Cause | Source |
|---|---|---|---|
| Supports v1 (W_K entière et tous les Q_b, plafond de 24 sites) | instables ; plafonnés ; ne dessinent pas l'objet | Q_b change sous une perturbation qui tend vers 0 (cercle, saut ≥ 1/4) ; conv(S\*) vaut la cellule critique agrandie K fois et retournée (364 mm contre environ 73 mm à K5) | `MATHEMATIQUES.md:840-858` [V] ; `QUESTION_CLAUDE_POLYEDRE_ORDRE_K_20261006.md` § 2.4 [V] |
| `kparties_reliees` = C(p+m, K), compte mis en avant par décision de l'utilisateur du 4 octobre | instable | sortir un site de la coquille fait passer le compte de 3 à 1 sans changer ni la boule ni Q_b | `MATHEMATIQUES.md:859-864` [V] ; `build/v11-persist/sortie_supports/DECISIONS_UTILISATEUR.md` § 1.5 [V-hd] |
| Filtre de rôle (0cc9cbec4) | cycle de plateau | trois sites équidistants à K = 1 : trois boules de fusion au même plateau | `be8085ec1`, corrigé en `07428324e` [V] |
| Arbre d'ordre K brut comme jetons | 14,45 nœuds par site à K5, 41,08 à K10 ; 67,8 % (K5) et 85,5 % (K10) des nœuds meurent à moins de 1 % au-dessus de leur rayon de naissance | aucune masse ni sélection ; nœuds de passage | `polyedres_reconnaissables/SYNTHESE.md:72-82` [D] ; 576 371 / 39 885 = 14,45 [V] |
| $H^{r}_{K+1}$ face aux cibles de l'utilisateur | **70 jugements tenus sur 125**, et non 70 perdus : T0 40/40, Q1 et Q1bis 20/20, Q2 0/5, Q3 0/25, Q-Π2 0/5, Q4 10/30 | la qualification (≥ K+1 sites) rend un pont ou une petite structure inactifs ; `first` fait aussi 70/125 ; ER0h fait 125/125, mais sans constante uniforme | `receipts/developpement_20261003/points_math/SYNTHESE.md:241-254` [V] |
| Stabilité par insertion | absente | {0,2,4} contre {0,η,2,4} à k = 2 : l'entrée et la réunion sautent de 2 à 1 ; obstruction de Palm (conditionnelle) | `HIERARCHIE_POINTS.md:309-324` [D] |
| Sélection plate EOM | séparations fugaces perdues (198 contre l'oracle 252) ; z = 2 déchiquette les voitures en lignes de balayage | EOM intègre la persistance : une séparation de 0,4 mm contre une union de 350 mm ne pèse rien, à tout z testé | `SORTIE_PLATE.md:108-122` [V] |
| Seuil relatif au parent (REL20, α = 1/10) | 9 objets sur 19 (rappel oracle 0,47 et 0,53), 0 vélo sur 3 contre le mur | **structurel** : un objet de masse m qui fusionne avec une composante de masse > 9m est jugé léger, quelle que soit sa persistance. La règle de décision écrite d'avance le rejette | `polyedres_reconnaissables/SYNTHESE.md:176-177` [D] ; comptage entier, pas masse § 9.1 |
| Polyèdres | environ 1 000 faces par site à K5 ; trois réfutations ; dessin instable à une coupe critique ; vélo réel 08/002852 illisible | voir § 5 | `receipts/polyedre_ordre_k_20261007` [V] |

**Sur le seuil relatif**, la passation (`PASSATION.md:202-203`) et l'audit (`AUDIT_FINAL_V11.md:663-666`) disent que `CLAUDE.md` et `SPECIFICATION.md` « demandent » un seuil relatif. **C'est exact pour `CLAUDE.md:134` seulement** :
- ce passage attribue de plus le seuil relatif au § 9.1, qui emploie en fait `min_cluster_size`, donc un seuil absolu (thèse, p. 97) [V] ;
- `SPECIFICATION.md:67-86` fait du seuil relatif la règle **à tester**, et garde le seuil absolu comme témoin ;
- `ARCHITECTURE.md:207-216` : « aucun n'est disqualifié uniquement par son unité » ;
- `PLAN.md:50-53` : « N'en retenir aucun par principe avant mesure ».

La décision attendue consiste donc surtout à **amender `CLAUDE.md`**.

**Sur la masse du § 9.1** : `HIERARCHIE_POINTS.md:175-178` [V] montre qu'avec les masses fractionnaires, chaque triangle de la fixture T0 (les deux triangles de la thèse) pèse 8/3 < 3 et cesse d'être un cluster à mcs 3. Adopter la masse du § 9.1, comme le recommandent la passation et Zoltan, change le comportement sur la cible la plus chère à l'utilisateur. Cette tension n'est pas relevée par la passation.

---

## 5. Le polyèdre d'ordre k

### 5.1 L'objet, fixé avec l'auditeur

Pièces : `d2be6bdc7`, reçu `audit_hartigan_delaunay_20261006` ; réponse `ee2df0362`.

$$a_\sigma=\min_{y\in F_\sigma}d_k(y)^{2},\qquad A_k(r)=\lbrace\sigma : a_\sigma\le r^{2}\rbrace$$

C'est :
- la mosaïque de Delaunay d'ordre k, c'est-à-dire la tranche de profondeur k du pavage rhomboïdal et le Delaunay pondéré des barycentres (BCY, Th. 4.8) ;
- **datée par d_k**, et non par la puissance (DTM) ;
- le complexe alpha pour k = 1 ;
- le prolongement du nerf des régions témoins W_Q(r), dont le 1-squelette est Γ_K.

Ses propriétés :
- les cellules se rattachent par leurs étiquettes (K-parties), jamais par la position d'un barycentre ;
- la DTM n'est qu'un attribut ;
- pas de Wrap par analogie.

**Garanties [P]** (`polyedre_ordre_k_20261007/SYNTHESE.md:21-58`) :
- même type d'homotopie que Ω_k(r) ;
- π₀ = nœuds de FULL ;
- les étiquettes donnent exactement P ∩ (C ⊕ B_r) ;
- Hausdorff ≤ r.

### 5.2 Résultats du workflow `wf_cd387a02-4b0`

Reçu `polyedre_ordre_k_20261007`, note `REPONSE_CLAUDE_POLYEDRE_ORDRE_K_RESULTATS_20261007.md` [V].

**Correction et robustesse.**

| Constat | Mesure |
|---|---|
| Oracle borné (n ≤ 60, k ≤ 8) contre FULL | 0 désaccord sur 91 ordres et 37 993 coupes |
| Filtration sous bruit apparié | robuste, d_B ≤ δ dans 64 cas sur 64 |
| Inclusions décalées en k | 0 violation sur 209 406 |
| Dessin à une coupe critique | instable : 44 nœuds sur 141 sautent, jusqu'à 1 426 mm |
| Ajout d'un seul site proche | crée une composante (33/33) |
| Identité des nœuds | stable seulement au-delà de 2δ (δ = √3/2 mm) ; or **61 à 67 % des nœuds K5 vivent moins de 2δ** sur les trois trames |

**Taille et coût.**

| Constat | Mesure |
|---|---|
| Faces par site de la mosaïque | 1 003–1 086 à K5, 132–144 à K2 [D], soit 62–82 fois (K5) et 28–30 fois (K2) la tour |
| A_5 d'une trame en natif | 6 à 8 s, extrapolé sous une hypothèse non vérifiée [D] |
| Réduction certifiée à sommets protégés | ×2 à ×7, sans alléger le dessin |

**Reconnaissance** : c'est le **rayon le long de la chaîne d'ancêtres** qui rend un objet lisible, pas le rendu. Sur le vélo synthétique à K5 :
- la roue est un anneau à 46,6 mm ;
- le vélo est lisible à 77–89 mm, sur des nœuds qui vivent 0,1 à 0,4 mm ;
- le vélo réel 08/002852 n'est lisible à aucun K.

**Défaut de prototype** : un trou dans la mosaïque d'ordre 2 de 08/000100, dû à qhull et localisé à deux cellules. Ce n'est pas une contradiction mathématique.

**Trois réfutations gravées** (`1fbeea5b8`) [V] :
- moins de k ajouts ne créent aucune composante : faux ;
- les chaînes δ-contractées gardent leur identité : faux ;
- les réalisations sont emboîtées entre ordres : faux.

Elles reposent sur la fixture `tests/fixtures/regressions/polyhedron_order_k_counterexamples.json`, le vérificateur `tools/check_polyhedron_order_k_counterexamples.py` et les entrées `false_in_general` de `docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md:154`.

**Deux cibles géométriques coexistent** [V] :
- la **surface d'acquisition observée**, regroupée par HGP (audit `7d41562c3` ; `polyedres_reconnaissables/SYNTHESE.md` § 2, qui recommandait aussi une condensation ABS à seuil absolu) ;
- **A_k(r)**, la région dense, sur laquelle l'utilisateur a recentré la question le 6 octobre au soir.

La v12 doit dire laquelle sert au jeton de forme.

### 5.3 Les huit questions ouvertes à l'auditeur

Source : `REPONSE_CLAUDE_POLYEDRE_ORDRE_K_RESULTATS_20261007.md` § 4.

1. Le représentant causal (« certifier au sommet de la chaîne, puis restreindre ») vaut-il certificat sur toute la plage ?
2. Quelle identité donner à un jeton porté par des nœuds de vie < 2δ : (chaîne, rayon, marges), entrelacement d'arbres, ou sélection du § 9.1 ?
3. Le certificat κ corrigé est-il valide, et quelle est la référence du lemme de déformation non lisse ?
4. Sous le plancher des sommets : retrait avec registre (r + λ), ou annulation de paires H1/H2 de persistance < ε avec π₀ exact ?
5. Existe-t-il un appariement optimal restreint aux intervalles de même naissance, ou une borne du forçage ?
6. Les six points du § 6.1 de la thèse, à K = 2 : deux trous sur [√(2+√3), 2) ?
7. D_v daté par max(α, g_k) et le dual de SCov sont-ils acceptables comme approximations déclarées ?
8. Retours fusionnés : multiplicités dans le prédicat, ou excès e(ρ) publié par le contrat d'entrée ?

Les cinq questions précédentes (`ee2df0362`, § 3) ont reçu une réponse en `28d70f8ab` (trois sens de « robuste », effondrements à sommets protégés, ombre acceptée comme rendu déclaré). Les huit nouvelles sont restées sans réponse jusqu'au gel [V].

---

## 6. Recommandations pour la v12 (sorties et applications)

### 6.1 Architecture et contrats

1. **Un registre d'événements unique par ordre, produit par le publieur et non par un balayage a posteriori.** L'`attach` sériel coûte 75 ms à W48 [V]. Le registre contient :
   - les nœuds : rang exact, parent, genre, K-partie de naissance (S\* et population I_b ∪ U_b, ou référence de boule), mort ;
   - les hyper-arêtes de fusion (ant(b) et la boule retenue par Kruskal) ;
   - les verticales ;
   - la pendaison par site (t, M, Q, propriétaire) ;
   - les sites couverts à d_v^− ;
   - le flux d'incidences coface–facette pour le § 9.1 (séquence count → preflight → fill).

   Toutes les sorties en sont des vues, chacune avec son lecteur en bibliothèque standard et `tree_k_sha256` : `full` compact, `squelette` (SPv2), `points`, `condense`, `plat`, **`coverage_v1`** et **`weighted_gabriel_v1`** (pilotes Zoltan 0A et 0B), polyèdres A_k à la demande.

   **Ne pas annoncer « ≤ 10 ms » avant un microbanc** : aucune mesure ne soutient ce chiffre (`PASSATION.md:230`) [I].
2. **Écriture.**
   - SHA-256 avec SHA-NI, ou haché hors du chemin critique.
   - Tampons réels (corriger le `setvbuf`).
   - Colonnes écrites en bloc (l'écrivain `MHGP11PT` écrit élément par élément).
   - Signature calculée une fois.
   - Un `full` compact est indispensable pour Zoltan : sinon 5,8 To par passe à K5 [I].
3. **Une seule voie d'arbre pour toutes les sorties.** Prendre L2b, ou S11 (pipeline à un ordre, estimé de 65 à 90 ms contre 138 à 184 ms pour `tree`, estimation [D] de `receipts/notes_hors_depot_20261007/gpu_optim/carte_forets.md:55-57`), pour `points` et `plat`. Mesurer les étages `attach`, `output` et `write` sur G4.
4. **S\* canonique et invariant par translation** : départage lexicographique sur les coordonnées. Déclarer SPv2 comme témoin H₀, jamais comme forme. Publier Q_b sur demande pour les usages de forme.
5. **Masses du § 9.1 exactes ou certifiées.**
   - [I] Avec des niveaux carrés a = r², ψ = r^{−p} n'est rationnel que pour p pair. ψ = r^{−3} (densité ambiante, `SPECIFICATION.md:54`) demande des encadrements certifiés, comme la tête plate. Décider avant le code.
   - Graver la fixture T0 à mcs 3 avec la masse § 9.1, et trancher la sémantique de mcs.
6. **Sélection enfichable** : EOM z, feuilles, masse, critère A. Ajouter la vue à trois niveaux (jetons, groupes, parties). Ouvrir une piste de recherche sur les séparations fugaces (cohérence sur l'axe K), bornée par l'antichaîne oracle. Ne pas revendiquer de règle par défaut universelle : z = 1 sur LiDAR, z = 2 sur synthétique.
7. **Points.** Garder $H^{r}_{K+1}$ (prouvé) comme règle de référence et publier `first` comme témoin : même score de 70/125, meilleur niveau B, pas de constante de stabilité. La stabilité par insertion et le critère d'existence (Q-Π2) restent de la recherche.

### 6.2 Protocole HGP contre HDBSCAN (T5), préenregistré et tenu jusqu'au bout

- **Trois niveaux séparés** : A (tour, meilleur amas discret), B (meilleur bloc de la hiérarchie), C (partition), chacun avec son oracle (antichaîne).
- **Bras** :
  - `sklearn` épinglé (version, BLAS, machine), sur G4 ;
  - `sklearn` atomique, plateaux normalisés ;
  - **même tête sur l'arbre de `sklearn`** (attribution T − A, A − R0) ;
  - **MR_k-bord**, pour isoler la connexité d'ordre supérieur ;
  - témoins de `MESURE.md` § 0.1 (voxels, DBSCAN, superpoints SPT) et ALPINE pour l'instance.
- **Populations** :
  - **représentatives** : tirage systématique de trames de plusieurs séquences, pas seulement des trames d'échec ;
  - dev sur 00–07 et 09–10, bilan sur 08 (`MESURE.md:45-52`) ;
  - témoins publiés à part ;
  - aucune population conditionnée au succès d'un bras ;
  - pas de double comptage entre lots.
- **Rapport par ordre**, jamais en catégorie « ∃ k ». Bootstrap en grappes par instance physique et par séquence. Analyse de puissance tirée des effets v11 (niveau B vélos +0,01 à +0,03 ; fusions −0,01).
- **Exécuter** P08 et S3b, ou leurs successeurs, en entier. Verser S2b et S3a en reçus. Rédiger le corrigendum du préenregistrement.
- **Métriques** : PQ « things » au sens de SemanticKITTI, IoU un-à-un, parts intact / fusionné / découpé / bruit. Convention de niveau déclarée.

### 6.3 Polyèdres

Hors du chemin des 100 ms : un module aval natif à la demande (K = 2 d'abord), alimenté par le registre (K-partie de naissance, sites couverts à d_v^−, vies, verticales). Les portes et la feuille de route sont dans `polyedre_ordre_k_20261007/SYNTHESE.md` § 7.1–7.3 ; obtenir d'abord la réponse aux huit questions.

### 6.4 Décisions de l'utilisateur en attente qui conditionnent ces choix

1. **Seuil de condensation**, absolu (thèse, p. 97) ou relatif, et amendement de `CLAUDE.md:134`.
   - [I] La v11 penche pour l'absolu : rejet structurel du seuil relatif, fixture du vélo contre le mur.
   - La doctrine Zoltan est neutre.
2. **Sémantique de mcs** : masse du § 9.1 ou comptage entier, compte tenu de la tension avec T0.
3. **Multiplicités** : refus (`multiplicity_unsupported`, `compute.cpp:85`) ou modèle par copies.
   - À 1 mm, les trois trames n'ont aucune fusion de retours (`polyedre_ordre_k_20261007/SYNTHESE.md` § 3.3, [D] ; cohérent avec le passage des portes LiDAR, qui refuseraient toute multiplicité) : ce n'est pas bloquant pour KITTI à 1 mm.
   - C'est bloquant pour une grille plus grossière ou un capteur plus dense.
4. **Périmètre et régime du contrat de 100 ms** : froid ou chaud ; latence ou débit ; FULL seul ou aussi `plat`/`points` ; K10 contrat ou objectif.
5. **Application prioritaire** : segmentation en temps réel, ou tokenizer et enseignant hors ligne. Ce choix fixe la cible, latence ou débit et stockage, et donc l'ordre de livraison des vues.
6. **Géométrie du jeton de forme** : surface d'acquisition observée ou A_k(r) ; polyèdres dans la v12 ou en aval.
7. **SPv2 et S\* seul** : réalisation finale, ou témoin seulement.
8. **Rôle de la séquence 08** (dev ou bilan).
9. Verser `build/v11-persist/sortie_supports/DECISIONS_UTILISATEUR.md` dans le dépôt, et y inscrire la décision du 6 octobre (Kruskal, S\* seul), qui en est absente [V-hd].

---

## 7. Corrections et compléments à `PASSATION.md` et `AUDIT_FINAL_V11.md`

1. **`AUDIT_FINAL_V11.md:651`** : « Les cibles Q2–Q4 sont perdues : 70 cellules sur 125 ». **Faux** : $H^{r}_{K+1}$ en **tient** 70 sur 125 et en perd 55 (Q2 0/5, Q3 0/25, Q-Π2 0/5, Q4 20/30) (`points_math/SYNTHESE.md:241-254`). La phrase source, `HIERARCHIE_POINTS.md:34`, est ambiguë. Le rapport D disait juste.
2. **`AUDIT_FINAL_V11.md:629-630`** (68,5 % contre 56,6 %) :
   - la population est conditionnée au succès de la hiérarchie HGP ;
   - les lignes `sklearn` y sont descriptives ;
   - 74 % des couples sont des découpes de voitures ;
   - l'écart vient surtout de k = 2 et 3 ;
   - 31 bouts sont comptés deux fois.
3. **`AUDIT_FINAL_V11.md:622`** (« Trois vélos sauvés ») : deux vélos physiques ; les démos 01 à 04 restent toutes « les deux échouent ».
4. **`AUDIT_FINAL_V11.md:632`** (13/3) : règle « ∃ k » avec priorité au gain ; par ordre, 4/2, 5/2, 10/1 et 7/0 ; deux « gains » contiennent une perte.
5. **Vidéos** : la variante sans sol est locale seulement ; à k = 5 sans sol, HDBSCAN fait 54 réussites contre 53 pour HGP.
6. **Témoins LiDAR** (parité) et **estimateur pondéré de la v10** (+0,0002 à K5) : absents de l'audit, ils sont indispensables pour lire les « sauvetages ».
7. **PQ sur trames entières** : à K10, HDBSCAN (0,781) devance la tête plate (0,769), sur 19 objets. Absent de l'audit.
8. **Calibrage synthétique** : les niveaux medium et hard sont définis par l'ARI de HDBSCAN.
9. **Seuil relatif** (`PASSATION.md:202-203`, `AUDIT_FINAL_V11.md:665`) : la doctrine Zoltan est neutre ; seul `CLAUDE.md:134` est catégorique, et il cite à tort le § 9.1.
10. **Voie de calcul** : `points` et `plat` utilisent `build_order` (7 035), pas L2b. Le commentaire d'en-tête de `compute.cpp` (« masque 16379 ») est périmé.
11. **Errata à ajouter** (`PASSATION.md` § 9) :
    - `SORTIES.md` § 9 annonce un « tampon fixe de 64 Kio », inexact sous glibc (`writer.cpp:69`) ;
    - `SORTIES.md` § 11, tableau « État au 4 octobre 2026 », périmé ;
    - `etude_table.json` est antérieur à la correction de l'arrondi des IoU (`38faaf272`).
12. **« Aucune porte K10 pour `supports` »** (`AUDIT_FINAL_V11.md:683`) : il en existe pour la bibliothèque en sélection `all` (v1), aucune pour SPv2 ni pour `points`.
13. **« Kruskal retire 94, 90 et 523 boules »** (rapport D) : seul 94 est traçable (576 482 → 576 388).
14. **Manques de la passation** :
    - les exports Zoltan `coverage_v1` et `weighted_gabriel_v1` ;
    - l'exportateur `CertifiedTowerInput` ;
    - la non-invariance de S\* par translation ;
    - la tension entre la masse § 9.1 et la fixture T0 ;
    - le criblage (environ 1,1 % d'échecs HDBSCAN à K5) comme borne de l'enjeu LiDAR ;
    - l'usage de la séquence 08 pour le dev, contre `MESURE.md`, règle 7 ;
    - la distinction entre latence et débit dans le contrat de 100 ms ;
    - le coût de `plat` et `points` hors tour (environ 0,4 s et 0,75 s en local à W8).
15. **Volume des bancs** : « plus de 9 000 lignes » se lit 7 727 lignes pour `bench/points_*.py`, et 9 312 en ajoutant `points_export.cpp` et `mhgp11_formats.py`. Écart mineur.

---

## Annexe : rejouer les recomptes

```bash
python3 -I /tmp/claude-1000/-workspaces-E-HGP/6acaaf62-b44a-41e7-b5d6-81e4c7b6b7f1/scratchpad/agent_E/recompute_points_g4.py claudepts6 claudepts4
python3 -I /tmp/claude-1000/-workspaces-E-HGP/6acaaf62-b44a-41e7-b5d6-81e4c7b6b7f1/scratchpad/agent_E/recount_bouts_e1.py
```

Le premier lit les archives `results.tar.gz` de `receipts/developpement_20261003/points_g4` en mémoire. Le second lit `Zoltan/demos/bouts_evalues.json`, `receipts/developpement_20261004/e1_sortie_plate/etude/` et, s'il est présent, `build/v11-persist/e1_s0/study_all.json` (hors dépôt). Aucun des deux n'écrit de fichier.
