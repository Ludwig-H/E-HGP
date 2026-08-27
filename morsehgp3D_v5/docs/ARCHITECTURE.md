# MorseHGP3D v5 — Architecture

Ce document décrit le pipeline **réel** (celui de `src/pipeline/run.hpp`), pas
un plan. Autorité mathématique : [`MATHEMATIQUES.md`](MATHEMATIQUES.md). Plan
de mesure : [`PLAN_DE_TESTS.md`](PLAN_DE_TESTS.md). Provenance module par
module : [`PROVENANCE.md`](PROVENANCE.md).

## 0. Contrats

- **Objet** : la forêt HGP complète (K = 1..K_max, K_max = 10 au profil),
  niveaux $\rho^2$ exacts, événements exacts (MATHEMATIQUES § 1–2). Une
  optimisation ne modifie ni l'objet, ni les niveaux, ni les inclusions ; la
  porte de conformité v4 ≡ v5 (digests canoniques au format v4) le grave.
- **Profil d'entrée** : u16 quantifié seulement (grille $[0, 65536)^3$),
  `PointId` u32 arbitraires (≠ index dense ≠ rang Morton), dégénérescences →
  refus explicite, jamais de jitter. Les positions dupliquées sont refusées
  (`unsupported_degeneracy`) tant qu'un HGP pondéré n'est pas défini et prouvé
  (question ouverte à l'auditeur).
- **Interdits d'architecture** : aucune structure de Delaunay d'aucun ordre,
  aucun arrangement global, aucune matrice globale de cofaces, aucun catalogue
  résident de tous les supports, aucun tableau indexé par toutes les paires /
  triplets / quadruplets ($\propto \binom{n}{k}$ interdit).
- **Statuts transactionnels** : toute exécution termine dans
  `complete_regular | unsupported_degeneracy | resource_exhausted | invalid_input | invariant_violated` ;
  aucun préfixe de payload n'est publié sur un refus.
- **Cibles** : portes d'invariants et de mesure à n = 8000, 16000, 32000 sur
  la machine de développement (8 cœurs, 31 Go) ; puis 50 000 points sur G4 ;
  puis des dizaines de millions de points. Aucun claim de temps ni de
  capacité sans reçu.

## 1. Une seule structure spatiale

`src/tree/cloud_index.hpp` : tri par clé de Morton 48 bits, bucketisation des
positions dupliquées (géométrie sur positions UNIQUES, identités et
multiplicités dans un CSR), arbre radix binaire de Karras sur les clés
uniques, cellules alignées (borne d'empilement) et boîtes serrées
(certificats). Le même arbre sert la source WSPD, le comptage de témoins et
les requêtes de cover. Il n'existe ni octree séparé ni second arbre.

## 2. Le pipeline

```text
0  entrée (PointId, u16³)  →  index (§ 1)                         tree/
1  pour chaque lane q ∈ {2,3,4} :
   1a  descente WSPD ternaire : paire MORTE (h_coeur ≥ h_q, sans      wspd/, spindle/
       descente) | TERMINALE (séparée, instruite) | SCINDÉE
   1b  par rectangle vivant : h_a/h_b (8 coins, exact), ancres        generate.hpp
       survivantes (h_coeur + h_a + h_b < h_q)
   1c  par ancre : cover (coef 3, ou 4 pour les intérieurs q4) ;      lanes/, float_filter.hpp
       q2 : boule diamétrale ; q3 : seeds aigus + filtre de
       profondeur certifié ; q4 : W_4, seed, cœur de Jung,
       complétions (owner, exact-once, préfiltres, Cramer, centre)
2  RLE par BallKey : arité minimale puis plus petite représentation    candidates.hpp
3  préfiltre count-only : mort à |I_B| ≥ h_qmin                         census.hpp, expand.hpp
4  census I_B / U_B complets des survivantes (plafond de coquille)
5  expansion des plateaux → événements par K, en PointId externes      forest/plateau.hpp
6  pour K = 1..K_max, STREAMÉ : fold (macro-lots, deltas, partition     forest/fold.hpp
   dense) → signature → compteurs → libération
```

Chaque étape parallèle découpe par tranches d'index et fusionne **en ordre de
tranche** : la sortie est bit-identique au séquentiel quel que soit le nombre
de fils, et le nombre d'ouvriers réellement créés est retourné, jamais
déclaré (`src/parallel/pool.hpp`).

## 3. Doctrine d'exactitude

Toute décision est entière et dimensionnée (i64 sous $2^{34}$, i128 au-delà,
U192/U320 pour les niveaux) ; les largeurs sont déclarées en tête de chaque
fichier de `src/lanes/`. Le flottant n'existe que comme **filtre certifié à
repli exact** (`src/pipeline/float_filter.hpp`) : séquence FMA figée, borne
d'erreur prouvée par seed, garde d'arrondi (`__FAST_MATH__`, `fegetround`).
Une boule est une boule : la `BallKey` primitive $(A, B, C)$ identifie la
boule quelle que soit la lane génératrice ; le niveau est un rayon **au
carré** en fraction non réduite, comparé par produits croisés.

## 4. Frontières mémoire (la faute de fond de la v4)

La v4 gardait les dix forêts résidentes et annonçait un « pic projeté » qui
oubliait les résultats terminés. La v5 nomme quatre rôles et ne confond
jamais deux d'entre eux :

| rôle | contenu | durée de vie |
|---|---|---|
| amont | index, candidats post-RLE, boules censusées | libérés dès que l'étape suivante n'en a plus besoin (`run.hpp` : `swap` explicites) |
| en construction | événements du K courant, table d'internement, union-find, tables à époque | un seul K à la fois |
| sortie persistante | ce que le consommateur **choisit** de garder dans `on_forest` (digest, cardinalités, ou la forêt) | décision de l'appelant, jamais du pipeline |
| temporaires | brouillons par ouvrier (cover, sites affines, histogrammes) | par tâche |

Le pipeline ne promet aucun « pic » ; il publie ce qu'il libère et quand. Un
plafond, s'il est demandé, se calcule sur des majorants comptés **par rôle**
et refuse avant allocation — chantier ouvert, voir `../audits/ETAT_COURANT.md`.

## 5. Ce que la v5 ne construit pas

Aucune mosaïque de Delaunay d'ordre supérieur, aucune liste globale de
cellules ou de cofaces, aucun tableau indexé par paire. Les seuls objets
globaux sont l'index spatial ($O(n)$), les rectangles WSPD vivants d'une lane
($O(s^3 n)$, consommés par vague), les candidats post-RLE et, par K, les
facettes du K-graphe. Les oracles exhaustifs (`oracle/`, `tests/`) restent
bornés et hors du chemin produit.

## 6. Mutants

Registre unique `src/core/mutants.hpp` : chaque défaut connu a un nom dans
`kMutants`, exactement un point d'injection `MHGP5_MUTANT("nom")` dans `src/`,
et une porte CTest à code 4. Les mutants ne sont jamais des options ; le
registre est vide dans tout chemin nominal.
