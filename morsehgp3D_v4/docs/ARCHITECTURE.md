# MorseHGP3D v4 — Architecture

Date d'ouverture : 17 août 2026 UTC. Autorité mathématique :
[`MATHEMATIQUES.md`](MATHEMATIQUES.md). Plan de mesure :
[`PLAN_DE_TESTS.md`](PLAN_DE_TESTS.md).

## 0. Contrats

- **Objet** : la forêt HGP complète (K = 1..K_max), niveaux ρ exacts,
  événements exacts (MATHEMATIQUES § 1–2). Une optimisation ne modifie ni
  l'objet, ni les niveaux, ni les inclusions.
- **Cibles** : K_max = 10 en < 100 ms sur une G4 (objectif principal) ;
  K_max = 5 en < 1 s (objectif secondaire). Nuages jusqu'à des dizaines de
  millions de points. Méthode GPU-friendly avec fallback CPU bien parallélisé.
- **Interdits d'architecture** (hérités, non négociables) : aucune structure
  de Delaunay d'aucun ordre, aucun arrangement global, aucune matrice globale
  de cofaces, aucun catalogue résident de tous les supports, aucun tableau
  indexé par toutes les paires/triplets/quadruplets (`∝ C(n,k)` interdit).
- **Profil d'entrée** : u16 quantifié seulement (grille `[0,65536)³`),
  `PointId` u32 (dizaines de millions de points), positions dupliquées
  bucketisées avec multiplicité conservée, dégénérescences → refus explicite
  (`unsupported_degeneracy`), jamais de jitter.
- **Statuts transactionnels** (hérités v3) : toute exécution termine dans
  `complete_regular | unsupported_degeneracy | resource_exhausted |
  numeric_failure | incomplete_continuation | invalid_input` ; séquence
  `count → preflight → fill → validate → publish` ; jamais un préfixe de
  payload publié.

## 1. Les structures — et pourquoi si peu

**Décision : UNE seule structure spatiale.** L'arbre radix binaire de Karras
construit sur les clés de Morton 48 bits des **positions uniques**, avec par
nœud : la cellule alignée (borne de packing), la boîte serrée (certificats),
la plage `[first,last]` dans l'ordre trié (layout mémoire), le poids
(multiplicités par somme préfixe). Réponses aux questions de structure :

- **« Un Morton en plus d'un octree ? » — Non.** Morton n'est pas une
  structure : c'est la *clé de tri* qui construit l'arbre (tri radix, GPU) et
  le *layout* qui rend chaque nœud contigu. L'arbre radix EST le déroulé
  binaire de l'octree comprimé (un nœud par bit de préfixe ; seuls les
  préfixes multiples de 3 sont des cubes, les autres ont un rapport d'aspect
  2 ou 4 — dégradation d'un facteur borné de l'argument d'empilement, jamais
  une pente). Il n'y a donc ni octree séparé, ni « Morton » persistant : un
  tri, un arbre.
- **Pas de second arbre.** La v3 a payé cher la coexistence de deux arbres
  (médiane de rang dans un probe, radix dans un autre) : une réfutation
  invalide, rétractée, est née de leur confrontation. En v4, le MÊME arbre
  sert les trois consommateurs : source WSPD, comptage de témoins dual-tree
  (h_coeur, h_a, h_b), requêtes de lentille des ancres survivantes.
- **Pas de fair split tree.** La borne CK vaut sur l'octree comprimé à
  constante près ; la construction Karras est un kernel plat (un thread par
  nœud interne), le fair split ne l'est pas. Le front mesuré v3 est conforme
  à sa théorie — c'est le *critère terminal* qui était faux, pas l'arbre.
- **Positions uniques d'abord.** La bucketisation des doublons AVANT l'arbre
  rend les clés distinctes : plus aucun tie-break par index (le défaut qui
  bornait la v3 à n ≤ 65535). Les identités (u32) et multiplicités vivent
  dans des buckets CSR triés par PointId — déterminisme sous permutation
  d'entrée, équivariance testée.

Coûts mémoire (SoA partout) : 6 octets/point de coordonnées + 4 octets/point
d'identité + ~96 octets/position unique d'arbre. 30 M de points ≈ 3,2 Go —
compatible G4 ; le format de nœud sera compacté (u16 par axe) au moment du
portage CUDA si nécessaire.

## 2. Le pipeline

```text
0. entrée u16 + PointId  →  tri Morton  →  buckets uniques  →  arbre radix
1. WSPD par vagues TERNAIRE (par lane q2/q3/q4, masques de lanes) :
     chaque paire active  →  MORTE (h_coeur ≥ h_q, sans descente)
                          |  TERMINALE (séparée, à instruire)
                          |  SCINDÉE (facteur de plus grand diamètre)
2. par rectangle terminal vivant : h_a/h_b (dual-tree à cutoff, histogramme)
     →  ancres survivantes par arité
3. instruction des ancres : lentille B(m,D) via l'arbre
     q2 : profondeur de la boule diamétrale ;
     q3 : porteurs aigus (V² > D²), owner, circumcentre Cramer ;
     q4 : seed aigu + théorème axial (≤ 16 groupes), positivité Cramer
4. census : BallKey → count/sort/RLE → range-count → census par clé unique
     → I_B/U_B  →  événements (S, depth, ρ, facettes actives)
5. dix forêts : hachage des facettes actives, tri par ρ, union-find/Borůvka
     → K-MST par K, niveaux, payload § 9.1 (S_τ, T_x, m_τ, vote)
```

Chaque étape est un kernel plat ou une vague `count → scan → fill` ; le
fallback CPU exécute les mêmes passes en parallèle par plages (les nœuds et
rectangles sont des intervalles contigus, le vol de travail est trivial).

### 2.1 L'étape 1 en détail : la descente qui tue

C'est la réponse au constat final de la v3 (« le WSPD est correct et aveugle
à la sortie : 10⁷ rectangles pour 6,6·10⁵ arêtes vivantes ») : le certificat
de mort est évalué PENDANT la récursion, au niveau du bloc, pas après.

- Test de mort d'un couple de nœuds (A,B) pour la lane q : compter (borne
  inférieure) les témoins universels du cœur — descente sur le witness-tree Z
  (le même arbre), `Hmin(A,B,Z) > 0` (exact, séparable par axe) crédite
  `poids(Z ∖ A∪B)` d'un coup ; cœur-boule `R_coup` comme voie ALL bon
  marché ; arrêt dès `h_q` atteint. Un rectangle mort ne descend plus, sa
  masse est comptée morte : le ledger `morte + vivante = C(n,2) − paires
  co-positionnelles` reste exact à l'unité.
- Fail-open par construction (MATHEMATIQUES § 2.5) : aucune ancre vivante
  n'est jamais fermée ; le coût du certificat est borné par nœud et
  early-exit.
- Les lanes partagent la descente (fuseaux emboîtés, une évaluation (H,Ξ)
  pour trois compteurs) avec masque par lane créditée.
- Le critère terminal reste : séparé ⟺ terminal (aucun cap) ; le cap
  d'ordonnancement aval est une continuation, jamais une redéfinition du
  rectangle.

### 2.2 Ce que la v4 ne refait pas (leçons v3 gravées)

- Pas de cap de masse dans le critère terminal ; pas de scission par
  population (les deux bugs du 16 août 2026).
- Pas de source kNN à petit préfixe comme condition de complétude (fixture
  50 000 points).
- Pas de crédit de groupe sans disjonction d'identités (fixture
  `collinear_seven`).
- Pas de dual-tree sans cutoff ponctuel (~256), sans masque de lanes, ni avec
  suppression de la diagonale au niveau nœud (mesuré : tout tombe à zéro).
- Pas de `double` non encadré dans une décision ; `ceil_sqrt` est le vrai
  plafond ; arrondi dirigé partout où une racine est approchée.
- Pas de deuxième arbre, pas de « pente » mesurée sous n = 8000, pas de
  conclusion d'échelle sans emprise contrôlée.

## 3. GPU et fallback CPU

- **Référence CPU d'abord** : chaque kernel existe en version CPU exacte,
  parallèle (plages d'intervalles), qui EST la spécification. Le port CUDA
  (opt-in, comme v3) reproduit bit à bit les décisions entières ; toute
  divergence est une faute, pas une tolérance.
- Primitives : tri radix (clé 48 bits), Karras (un thread par nœud), remontée
  de boîtes par compteurs atomiques, vagues `count → scan → fill`
  (rectangles), radix/RLE (BallKeys), union-find par sauts (forêts) —
  toutes standard côté GPU.
- Les mesures de temps sur G4 passent par les scripts gardés de
  `gcp-migration/` (VM SPOT, double coupe-circuit) et produisent des reçus ;
  aucun chiffre G4 n'est publié sans reçu.

## 4. Identités et clés

Reprises v3 (PROPOSITION § 2.1), inchangées : `PointId` stable ≠ index dense
≠ rang de Morton ; `SupportKey` = tuple trié des vrais PointId ;
owner = arête maximale + tie-break EdgeKey ; `BallKey` formée AVANT le census
(cinq coefficients primitifs de `A‖z‖² + B·z + C`, pgcd, signe A > 0, jamais
un champ issu du census) ; `I_B/U_B` appartiennent à l'événement.

## 5. État d'implémentation (mis à jour au fil des commits)

- [x] `src/core/` types, Morton (§ 1)
- [x] `src/cloud/` familles v3 portées bit à bit (+ contre-familles gravées)
- [x] `src/tree/` arbre radix sur positions uniques, cellules + boîtes
      serrées + poids ; selftest d'invariants (plages, alignement, poids,
      équivariance par permutation)
- [x] `src/wspd/` front par vagues binaire (séparé/scindé), ledger de masse
      exact, mutants `drop-rect` et `cap-terminal` tués
- [x] descente ternaire q2 (boule-cœur dirigée + Hmin exact + borne minimax),
      juge fail-open par équivalence `r_{h_2}(m) <= ‖ab‖/2`, mutant
      `radius-ceil` tué sur fixture gravée ; reçus 12 configurations
- [x] trois lanes q2/q3/q4 sur une seule vague, masque de lanes, boule-cœur
      par arité (constantes point-fixe prouvées par static_assert), autorité
      64 coins aux feuilles (sens ALL seulement), juge `in_spindle`
- [ ] boule-cœur : passer au rayon `max(R_dec, R_coup)` (la borne couplée
      donne +71 % de rayon à s=6 q4, mesure v3) — R_dec seul est implémenté
- [ ] h_a/h_b dual-tree à range-add (cutoff ~256, coins distincts, masques)
      + histogramme de fermeture ; piste « dominance directionnelle 432 »
      (seul certificat v3 dont la fermeture croît avec n) à re-mesurer
- [ ] instruction q3 (lentille + acuité) et q4 (seed + axial)
- [ ] BallKey/RLE/census, événements
- [ ] dix forêts + juge `gamma_forest` indépendant
- [ ] port CUDA des étapes 0–2, campagne G4 sous reçus
