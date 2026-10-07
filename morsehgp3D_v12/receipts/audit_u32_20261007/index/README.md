# Socle u32 : garde, census, identité et requêtes entières

Pin audité : `c3de9d73d8999f2f1e31a0f592b829efc6e7a4da` (socle numérique de `6a38`).
Profil compilé : `MHGP12_COORD_BITS=32`, CPU Release, GCC 13.3.0.
Aucune exécution CUDA/G4, aucune donnée réelle, aucun résultat FULL ou chrono contractuel.

`check.py` compile une petite sonde sur treize unités du produit, sans CMake ni matrice globale.
Les sources des quatre modules `core`, `num`, `cloud`, `index` doivent être identiques au pin Git et sont
hachées avant/après. L'oracle Python reconstruit les centres par élimination rationnelle de Gram, les
signes de puissance, les extrema entiers des boîtes, les 96 bits de Morton et les populations exactes.
Il n'importe pas les calculs du produit. Les conditions de contrôle utilisent des exceptions explicites,
y compris sous `python -O`. Les fichiers temporaires et l'exécutable sont supprimés en sortie.

## Résultats

Les **2 439 requêtes** passent en normal et `-O`, avec captures identiques :

- 52 distances entières, dont les coins opposés du cube u32 : `3 (2^32−1)^2`, au-delà de `u64` ;
- 128 clés Morton, dont chaque bit des trois coordonnées et le coin maximal ; six préparations Cloud
  avec doublons, permutations et positions distinguées seulement par des bits élevés (21, 24, 31).
  Les positions restent distinctes et chaque liste de PointId est intégralement conservée ;
- 13 requêtes de réservoir/repère, étendues 0, 1, 16, 17, 24, 25, 29, 30, 31, 32 et 33, dont le
  contre-exemple `3 (2^31−3)^2 > 2^63` et la fermeture à `2^32` ; site non couvert et boîte inversée refusés ;
- 2 144 confrontations de support, site et boîte sur 90 supports, dont 62 certifiés et 28 non certifiés.
  Translations vers `UINT32_MAX`, petits et grands supports, demi-centres, triangle droit/obtus et
  tétraèdres sont confrontés aux fractions exactes. `CertifiedBall` et `GuardedSphere` ne sont pas
  constructibles depuis une simple `Sphere` (assertions statiques de la sonde) ;
- 96 census, sur de petits nuages, seuils 1, 2 et `UINT32_MAX`, feuilles 1, 3 et 8 : résultats génériques,
  certifiés et empruntés conformes à l'oracle, y compris saturation et abandon de la coquille à saturation.

La garde exerce 316 boîtes disjointes, 323 boîtes partielles et 460 sites extérieurs ;
les quatre voies sont parcourues (1 036 natives, 875 certifiées, 143 contrôlées, 666 larges).
Ces compteurs correspondent aux requêtes directes de garde, sans additionner les census.

## Portée pour le registre

- **CST-0108** : preuve du type non forgeable depuis `Sphere` et de refus de certificats non stricts,
  puis census générique correct pour ces propositions. Le mutant demandé dans le constat initial
  n'est pas compilé dans cette tranche légère.
- **CST-0109** : le nouveau census local gardé passe sur les boîtes partielles, disjointes, contenues
  et aux limites u32. Cela qualifie ces interfaces du socle ; aucun raccord catalogue n'est exercé.
- **CST-0110** : la primitive de distance u32, ses valeurs et ses voies sont contrôlées ;
  le futur calcul complet des k plus proches n'est pas présent dans cette sonde.
- **CST-0202** : la clé complète et la préparation Cloud u32 passent les témoins de collision et
  d'intercalation ; le régime D8 de refus par défaut des doublons reste une responsabilité d'entrée.
- **CST-0204/0208** : repère s33 et primitive de réservoir corrects ici, sans transfert au catalogue
  ni à ses noyaux GPU. L'emploi du repère parent par le filtre futur n'est pas testé.

Aucun défaut nouveau établi. Cette porte bornée complète la lecture du code ; elle ne démontre pas
la correction de tous les supports u32, les garanties de concurrence, la sûreté sous sanitizer ou
les objectifs de coût et de performance de la v12.

## Rejeu

Depuis la racine du worktree épinglé :

```sh
python3 -B morsehgp3D_v12/receipts/audit_u32_20261007/index/check.py
python3 -B -O morsehgp3D_v12/receipts/audit_u32_20261007/index/check.py
```

`normal.json` et `optimized.json` conservent les résultats agrégés et les empreintes des sources,
de la sonde, du flux complet généré, des réponses natives et de l'exécutable. Les deux flux complets
sont reproductibles par le script ; ils ne sont pas dupliqués dans le dépôt.
