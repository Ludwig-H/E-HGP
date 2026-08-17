# Addendum — le quotient des plateaux sphériques est reçu dans la forêt

Date : 17 août 2026. Exécute l'audit bloquant
`AUDIT_BLOQUANT_2AA0C3A_COQUILLES_U16_AVANT_FORET` point par point.

## 1. Q5 tranchée : option A (u16 normatif)

Le profil gravé `quantized_u16_input_only` impose l'option A : l'objet
normatif EST le nuage u16. L'option B (coordonnées de vérité plus fines)
serait un AUTRE profil d'entrée à nommer et re-dimensionner — non retenue.
Gravé dans `MATHEMATIQUES.md` § 5.3bis avec le théorème du plateau de
l'audit (σ = I_B ∪ T, T ⊆ U_B, |T| = K+1−|I_B|, c ∈ conv(T) fermé) et sa
conséquence Carathéodory : les lanes restent les générateurs, mais
publient un quotient commun par boule.

## 2. Mesure du taux de runs complets sous `--exact` (audit § 5.2)

| probe | famille, n | verdict `--exact` |
|---|---|---|
| q2 | uniform / terrain / eight_clusters, n=400 | refus (code 2) |
| q2 | uniform, n=100 | refus (code 2) |
| q3 | uniform, n=400 | refus (code 2) |
| q4 | uniform, n=120 | refus (code 2) |

**Taux de runs complets : 0 %** sur tout ce qui a été mesuré. La position
générale n'est pas une précondition pratique du profil u16 — c'est
l'exception. Toute sortie du sous-flux régulier porte le statut
`complete_regular_only`, jamais `exact`.

## 3. L'implémentation (oracle borné, dans la forêt)

- `src/forest/sphere_plateau.hpp` : centre rationnel `−B/(2A)` depuis la
  BallKey primitive ; `c ∈ conv(T)` FERMÉ par Carathéodory (paire
  diamétrale, triangle fermé — coplanarité + barycentriques, OBig où les
  largeurs l'exigent —, tétraèdre fermé) ;
- `ForestEvent` généralisé : la part `T` monte à 11 hors position
  générale (le support minimal q ≤ 4 est le cas régulier) ;
- sujet du selftest réécrit : boules candidates par les générateurs de
  lanes (paire / triangle strictement aigu / tétraèdre strictement
  centré), **dédupe par BallKey primitive commune aux trois lanes** (une
  boule est une boule — les deux diagonales du carré donnent une seule
  clé), census `I_B` / `U_B` COMPLET, expansion des `T ⊆ U_B` avec
  plafond de coquille explicite (`resource_exhausted` au-delà, jamais de
  troncature) ;
- juge assoupli au sens EXACT de la Déf. 28 : seule la boule OUVERTE
  doit être vide — un point sur la sphère (membre de σ au-delà du
  support, ou externe) est permis. C'était le juge qui était trop
  strict, aligné sur le refus transactionnel v4 au lieu du manuscrit.

Les macro-lots du noyau absorbent le plateau SANS modification : même
boule ⟹ même niveau exact ⟹ même lot — le gel des racines et la
multifusion simultanée étaient déjà la bonne sémantique.

## 4. Fixture `square_cospherical_K2_plateau` et mutant

Le carré cocyclique de l'audit (`(110,100,100), (100,110,100),
(90,100,100), (100,90,100)`, centre `(100,100,100)`, R² = 100) est le
nuage 2 du selftest, gravé aux trois ordres :

| K | attendu (gravé) | sens |
|---|---|---|
| 1 | 1 nœud, 4 absorbées (niveau 50), diagonales sans fusion à 100 | Γ_1 |
| 2 | 1 nœud, 6 absorbées (niveau 100) | les 4 triangles RECTANGLES fusionnent les 4 côtés + 2 diagonales |
| 3 | 1 nœud, 4 absorbées (niveau 100) | le 3-simplexe du carré complet |

Mutant `drop-shell-plateau` (l'ancien refus des boules à extra-coquille) :
au K=2 le sujet n'émet plus RIEN — composantes fausses, juge en
désaccord, code 4. Exactement la contre-fixture de l'audit.

## 5. Mesures globales

500 événements (487 réguliers + 13 de plateaux sur les quatre nuages),
1 618 fusions, 369 nœuds multi-composantes, 0 désaccord avec le juge
Déf. 28 pure, 0,58 s. **65 portes CTest vertes** (mutants `binary-ties`,
`repr-ties`, `drop-shell-plateau` tous tués).

## Reste (déclaré)

La version À L'ÉCHELLE du quotient : `SpherePlateau` en production
(sort/RLE par BallKey, un census par clé collectant `I_B` ET `U_B`,
chemin rapide régulier inchangé, compression de la famille des `T` par
supports minimaux) — à brancher quand les pipelines WSPD seront
refactorisés en bibliothèque. Jusque-là les probes gardent leur statut
`regular_subset_diagnostic` et la forêt de production `complete_regular_only`.
