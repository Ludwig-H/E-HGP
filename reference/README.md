# Oracle CPU exhaustif de MorseHGP3D

Ce répertoire contient l'oracle indépendant de la phase 1. Il énumère directement les sous-ensembles et cofaces utiles, calcule leurs miniballs en arithmétique rationnelle, construit les filtrations Gamma et Gabriel, puis rejoue les forêts et les applications verticales à chaque coupe exacte.

Cet oracle est une vérité terrain de petite taille, pas un backend de production. Son coût est exponentiel : la suite courante vise les nuages jusqu'à douze points et réserve quatorze points à des cas sélectionnés ou nocturnes. Aucun module de `HGP-old`, aucun noyau CUDA et aucun futur code de réduction de production n'est importé.

## Garanties et profils

- Les entiers, fractions et coordonnées `binary64` finies sont interprétés exactement. Un `float` devient le dyadique exact porté par ses bits IEEE 754, sans conversion décimale intermédiaire.
- Les doublons et les coordonnées non finies sont refusés sans perturbation.
- `hgp_reduced` conserve toutes les composantes à l'ordre un puis les composantes Gamma non triviales représentées par Gabriel. La sérialisation peut porter `public_status=exact` seulement après fermeture de tous les contrôles du certificat.
- `full_pi0` est calculé exhaustivement en interne, mais sa sérialisation contractuelle reste `partial_refinement` et `public_status=conditional` tant que l'obligation M.1 n'est pas démontrée.
- Une coquille extérieure exacte pertinente ou une autre dégénérescence hors du premier domaine générique provoque un échec explicite. Aucun jitter n'est appliqué.
- Les cartes verticales du profil réduit sont obtenues par projection certifiée de l'oracle Gamma sur la bijection des composantes non triviales. Une inclusion d'unions de points ne sert jamais à choisir une cible, car ces unions peuvent se recouvrir.

La [note du paquet](morsehgp3d_oracle/README.md) précise pourquoi l'énumération directe ferme l'univers des supports sans prétendre avoir construit des cellules canoniques de production.

## API minimale

```python
from reference.morsehgp3d_oracle import run_oracle, serialize_oracle_result

points = [(0.0, 0.0, 0.0), (2.0, 0.0, 0.0), (8.0, 0.0, 0.0)]
result = run_oracle(points, 2, profile="hgp_reduced")
contract = serialize_oracle_result(result)
```

Le résultat Python conserve aussi toutes les coupes ouvertes et fermées. `serialize_oracle_cuts` produit leur artefact canonique séparé pour le différentiel détaillé; le contrat v1 principal conserve une forêt rejouable plutôt que d'ajouter un champ non gelé.

## Validation

```bash
PYTHONDONTWRITEBYTECODE=1 python -m unittest discover -s tests/oracle -p 'test_*.py'
PYTHONDONTWRITEBYTECODE=1 python tools/run_oracle_campaign.py --ci
```

Le runner de campagne compare les forêts aux filtrations exhaustives, les couvertures réduites aux composantes Gamma non triviales, les cartes verticales des deux profils, les préfixes en `K_max` et les transformations métamorphiques exactes. Un écart peut produire une fixture minimisée uniquement lorsqu'un répertoire d'échec est fourni explicitement.
