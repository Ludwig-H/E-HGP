# MorseHGP3D v4 — Plan de tests

Hérite du plan racine (`docs/TEST_PLAN_MORSEHGP3D.md`, § 3.1–3.2) et des
conventions v3 ; ce fichier fixe leur application v4.

## 1. Tailles et régimes

- **Tailles d'intérêt : n = 8000, 16000, 32000** (et 64000 en extension —
  dernière taille observable du profil u16 côté grille `uniform`). Toute
  conclusion de coût, sélectivité, mémoire ou échelle s'y mesure. Les petites
  tailles (n ≤ 2000) ont UN rôle : oracle de correction — jamais une pente.
  Aucune pente sous n = 8000 (plafond `C(n,2)` mesuré en v3 : à n = 1000 la
  WSPD couvre 40 % de toutes les paires).
- **Familles** : `uniform` (régime dur volumique), `terrain` (cible LiDAR
  2,5D), `eight_clusters` (adversariale témoins), `scanline_single_pass` /
  `scanline_overlap_multiecho` (LiDAR anisotrope) — générateurs **bit à bit
  identiques v3** (mêmes graines ⟹ mêmes nuages ⟹ mesures confrontables aux
  reçus v3). Contre-familles gravées : `two_lines`, `collinear_seven`
  (réfutations déterministes, jamais des régimes).
- **Séparations WSPD : s = 6, 8, 10.** Acquis v3 à confronter : s = 8 domine
  s = 6 partout ; s = 10 retire 9–27 % du résiduel pour 1,38–1,55× de
  rectangles ; l'arbitrage final dépend du coût d'instruction d'une ancre.
- **Graine par défaut : 3** (continuité v3) ; l'équivariance par permutation
  est une porte, pas une option.

## 2. Interdits et obligations

- **Jamais de vérification exhaustive** : ce qu'un théorème garantit est
  invoqué ; on grave ses fixtures d'égalité. Ce qui reste à tester est la
  faute d'implémentation : invariant global, juge d'échantillon, mutant.
  Exception : les oracles bornés (n ≤ 12–14 pour la forêt complète ;
  n ≤ ~4000 pour les comptes q2 par force brute au milieu) *établissent* la
  vérité.
- **Planchers de couverture** (`--min-*`) contre le vert-par-vacuité, sur
  toute porte.
- **Mutants tués** (`--inject=...`, portes à code 4) pour chaque défaut connu
  ou historique.
- **Codes de sortie exacts** : 0 conforme, 1 désaccords du juge, 2 refus
  avant calcul, 3 plancher/invariant violé, 4 mutant tué. Les crashs par
  signal sont refusés partout (`cmake/run_expect.cmake`). Un CTest à
  `PASS_REGULAR_EXPRESSION` est doublé d'une porte à code.
- À l'échelle : invariants globaux et juges d'échantillon, jamais un juge
  `O(n³)` ni un tableau indexé par paire.

## 3. Invariants globaux par étage

| Étage | Invariant | Porte |
|---|---|---|
| arbre | clés strictement croissantes ; plages enfants = partition du parent ; boîte ⊆ cellule alignée ; poids parent = somme enfants ; équivariance permutation | `mhgp4_tree_selftest` |
| WSPD | ledger `Σ|A||B| = C(n,2) − Σ C(μ_u,2)` exact en 128 bits ; plancher de rectangles ; digest invariant par permutation | `mhgp4_wspd_ledger_*` |
| WSPD mutants | rectangle perdu → ledger (code 4) ; cap dans le critère terminal → discrimination appariée (code 4, sur `two_lines` — « le cap ne mord pas » sur uniform n=2000, constat v3 reproduit) | `mhgp4_wspd_mutant_*` |
| descente ternaire | `masse_morte + masse_vivante = masse_totale` exact ; fail-open : `vivantes_probe ⊇ vivantes_vraies` (juge d'échantillon par requête au milieu) ; vrai vivant invariant de la partition (même compte aux trois s) | à venir |
| h_a/h_b | porte métamorphique `direct == dual-tree` par ancre et lane ; masque de lanes (mutant `dual-sans-masque` en régime tendu s_max=32, séparation=1) | à venir |
| événements | `ballkeys_uniques == evenements` (régime régulier) ; niveau exact > 0 ; fixtures q3/q4 gravées (v3 § 10) | `mhgp4_q3_events_judge_*` |
| oracle q3 | accord total sujet/oracle sur tous les triangles (acuité, profondeur, coquille, niveau exact ET niveau public) ; fixtures u16 extrêmes ; plancher de limbes (produit du niveau ≥ limbe 3) ; débordement = statut fail-closed, jamais un signal | `mhgp4_q3_oracle_*` |
| arithmétique oracle | le juge du juge : `OBig` contre `cpp_int` (troisième autorité, si Boost présent) — paires add/sub/mul/cmp, distributivité (plancher 1 000 triplets), frontière exacte du 7e limbe en statut | `mhgp4_obig_*` |
| ordre des niveaux | `compare_level` U192 (produits croisés < 2 puissance 171) contre l'oracle 384 bits sur toutes les paires récoltées ; antisymétrie ; canonicité (égalité ⟺ mêmes fractions réduites) ; plancher de plateaux (cas des macro-lots) ; fixture de largeur mot-haut | `mhgp4_q3_level_cmp_*` |
| source q4 | fixture bloquante 13 points : ancre q3-morte (n3=9) mais q4-vivante (n4=0), tétraèdre q4 de profondeur 0, owner EdgeKey(0,1) — le découplage des lanes est gravé ; mutant `q4-seeds-from-q3-live` tué | `mhgp4_q4_source_*` |
| forêts | K=1 ≡ single-linkage (MST de référence) ; juge `gamma_forest` indépendant sur n ≤ 12–14 ; égalité Théorème 2 (second chemin par L_K(r) + couverture) | à venir |

## 4. Fixtures permanentes reprises de la v3

À graver au fur et à mesure que l'étage correspondant existe (liste source :
PROPOSITION v3 § 10) — notamment : owner équilatéral/isocèle sous
permutation ; tétraèdre entier aux six arêtes égales (tie-break EdgeKey) ;
q3 aigu/droit/obtus et fixtures de seuil huit/neuf ; « dix témoins q2 dans la
boule diamétrale qui ne ferment pas q4 » (a=(100,100,100), b=(200,100,100),
x=(150,30,120), y=(150,30,80), z_i=(150+i,140,100)) ; la contre-fixture kNN à
50 000 points (4 sommets + 4×12499 distracteurs) ; `collinear_seven`
(profondeur exactement 7, crédit de groupe interdit) ; `two_lines` (masse
universelle quadratique, zéro porteur aigu) ; fixtures d'arrondi q3
a=(0,0,0), b=(14,0,0), z=(7,1,4) et q4 a=(0,0,0), b=(8,0,0), z=(4,1,2)
(plancher du rayon cœur-boule).

## 5. Mesures d'échelle et reçus

Campagnes `counter-only` aux 36 configurations (4 familles × 3 tailles ×
3 s), graine 3 : rectangles/point, fermeture par lane, résiduel absolu par
point, mou (survivantes/vivantes), temps par étape. Le « vrai vivant » est
recalculé par le juge d'échantillon, jamais par la structure mesurée.
Trois exposants successifs par arité avant toute phrase sur une pente.
Les temps G4 exigent un reçu (commande, HEAD, hashes, codes, arrêt certifié) ;
un OK CPU ou une extrapolation de bande passante ne qualifie aucun SLO.
