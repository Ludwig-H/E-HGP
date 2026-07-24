# Oracles CPU exhaustifs de MorseHGP3D

Ce répertoire contient les oracles indépendants de petite taille. Le chemin historique de phase 1 énumère directement les sous-ensembles et cofaces utiles, calcule leurs miniballs en arithmétique rationnelle, construit les filtrations Gamma et Gabriel, puis rejoue les forêts et les applications verticales à chaque coupe exacte. Le module `ordinary_diagram` ajoute l'atlas affine borné de Phase 8 pour confronter le diagramme ordinaire de production à une construction algorithmique indépendante.

Cet oracle est une vérité terrain de petite taille, pas un backend de production. Son coût est exponentiel : la suite courante vise les nuages jusqu'à douze points et réserve quatorze points à des cas sélectionnés ou nocturnes. Aucun module de `HGP-old`, aucun noyau CUDA et aucun futur code de réduction de production n'est importé.

L'atlas 8.4 possède sa propre borne plus stricte $n\leq8$. Il décode directement les mots binary64, construit chaque intersection de cellules $K_Q$ par RREF et énumération de frontières en `fractions.Fraction`, puis compare seulement une projection sémantique du producteur C++. Il n'importe aucune primitive géométrique de production et ne confère aucun statut public.

Cette implémentation est gelée comme oracle de preuve borné. Toute baseline de Voronoï plus large doit d'abord évaluer un adaptateur versionné vers Geogram ou une bibliothèque mature équivalente au lieu d'étendre ce module; aucune de ces voies de test ne remplace le chemin produit GPU ni sa recertification exacte.

Cette séparation protège les deux cibles produit : passage complet d'environ 50 000 points pour $K_{\max}\leq10$ avec un p95 `warm_e2e` strictement inférieur à 100 ms comme objectif principal et strictement inférieur à une seconde comme objectif secondaire, puis streaming transactionnel à dix millions de points ou davantage. Le catalogue exhaustif peut contrôler l'extraction 8.5 dans les tests; il n'entre jamais dans son producteur.

## Garanties et profils

- Les entiers, fractions et coordonnées `binary64` finies sont interprétés exactement. Un `float` devient le dyadique exact porté par ses bits IEEE 754, sans conversion décimale intermédiaire.
- Les doublons et les coordonnées non finies sont refusés sans perturbation.
- `hgp_reduced` rejoue toutes les cofaces Gamma et conserve exactement les composantes Gamma non triviales au-dessus de l'ordre un. Sa sérialisation v2 porte `proof_basis=gamma_exhaustive_reference`, `forest_semantics=exact` et `public_status=exact` uniquement sur `reference_cpu` après rejeu déterministe complet.
- `full_pi0` est calculé exhaustivement en interne, mais sa sérialisation contractuelle reste `partial_refinement` et `public_status=conditional` tant que l'obligation M.1 n'est pas démontrée.
- Avant d'émettre un certificat de complétude, `serialize_oracle_result` relance l'oracle de référence depuis les points et exige l'égalité structurelle du catalogue, de Gamma, des coupes, forêts, lots, attaches, journaux, verticales et compteurs. Cette frontière volontairement coûteuse refuse une provenance simplement réétiquetée ou un objet cohérent mais amputé.
- Une coquille extérieure exacte pertinente ou une autre dégénérescence hors du premier domaine générique provoque un échec explicite. Aucun jitter n'est appliqué.
- Les cartes verticales Gamma restent l'oracle exact. Le flot Gabriel brut est exposé séparément par `build_gabriel_partial_forest`; il ne construit aucune carte verticale normative et son écart à Gamma est un diagnostic attendu. Une inclusion d'unions de points ne sert jamais à choisir une cible, car ces unions peuvent se recouvrir.

La [note du paquet](morsehgp3d_oracle/README.md) précise pourquoi l'énumération directe ferme l'univers des supports sans prétendre avoir construit des cellules canoniques de production.

## API minimale

```python
from reference.morsehgp3d_oracle import run_oracle, serialize_oracle_result

points = [(0.0, 0.0, 0.0), (2.0, 0.0, 0.0), (8.0, 0.0, 0.0)]
result = run_oracle(points, 2, profile="hgp_reduced")
contract = serialize_oracle_result(result)
```

Le résultat Python conserve aussi toutes les coupes ouvertes et fermées. `serialize_oracle_cuts` produit leur artefact canonique séparé pour le différentiel détaillé; le contrat v2 principal conserve une forêt rejouable plutôt que d'ajouter un champ non gelé.

## Validation

```bash
PYTHONDONTWRITEBYTECODE=1 python -m unittest discover -s tests/oracle -p 'test_*.py'
PYTHONDONTWRITEBYTECODE=1 python tools/run_oracle_campaign.py --ci
```

Le runner de campagne compare les forêts aux filtrations exhaustives, les couvertures réduites aux composantes Gamma non triviales, les cartes verticales des deux profils, les préfixes en `K_max` et les transformations métamorphiques exactes. Un écart peut produire une fixture minimisée uniquement lorsqu'un répertoire d'échec est fourni explicitement.
