# HGP-FM — un modèle de fondation 3D pour le LiDAR extérieur

26 septembre 2026. Conception seulement. Aucune expérience apprise, aucun code,
aucun chiffre d'apprentissage revendiqué.

```text
phase=conception_modele_fondation_hors_registre
backend=reference_cpu_et_cuda (producteur morsehgp3D_v9)
profile=quantized_u18_input_only
mode=conception_et_falsification
public_status=not_claimed
```

## L'idée

Le nuage de points est un artefact du capteur. À grande portée, une voiture
donne moins de retours, des trous différents et des facettes plus grossières ;
un modèle point à point doit donc apprendre en même temps la géométrie, le
capteur, la densité, l'occultation et la sémantique. L'hypothèse du poster
3IA 2026 est que la **surface**, elle, varierait beaucoup moins.

On remplace donc l'unité de calcul :

```text
points / voxels / superpoints
              |
              v
   pieces polyedriques HGP, et leur hierarchie de fusion
```

GPT découpe le texte en jetons et apprend comment ils s'assemblent ; ici on
découpe la scène en **pièces géométriques** et on apprend comment elles se
ressemblent, croissent et fusionnent. La différence avec un tokenizer appris
est que celui-ci est défini par un théorème : les jetons sont exactement les
amas de forte densité de l'estimateur $K$-NN, à tous les niveaux de la
hiérarchie (manuscrit, Théorème 2).

## Les cinq documents

| document | ce qu'on y trouve |
| --- | --- |
| [`OBJET.md`](OBJET.md) | ce que la tour v9 fournit vraiment, avec ses chiffres mesurés et ses trous |
| [`ARCHITECTURE.md`](ARCHITECTURE.md) | la pile en cinq étages, les conceptions écartées et pourquoi |
| [`JETON.md`](JETON.md) | le descripteur : cinq familles de canaux et la règle de normalisation |
| [`PROTOCOLE.md`](PROTOCOLE.md) | l'échelle de portes G0 à G7, dont la sonde XGBoost |
| [`RISQUES.md`](RISQUES.md) | les trois risques majeurs, les pistes déjà fermées |
| [`GLOSSAIRE.md`](GLOSSAIRE.md) | les termes du manuscrit, de la v9 et du modèle |

## Le résumé en dix lignes

1. La tour FULL de `morsehgp3D_v9` calcule exactement, en entiers, la
   hiérarchie $K$-NN d'une trame SemanticKITTI : **0,76 à 0,98 s à $K \leq 5$**
   sans sol, 1,81 à 2,03 s en brut (reçu G4 R22, 26 septembre 2026).
2. Elle produit de **1,3 M à 16,3 M nœuds par trame**. Un Transformer en veut
   $10^3$ à $10^4$ : la sélection est donc le premier choix d'architecture, pas
   un détail.
3. La réponse existe déjà dans la thèse et dans `morsehgp3d/` : condensation par
   les masses de § 9.1, puis sélection par excès de masse, le tout exact.
4. Le tokenizer est **déterministe** : il se calcule une fois et se met en
   cache. Aucun tokenizer appris n'a cette propriété.
5. Le jeton porte cinq familles de canaux ; **une seule est normalisée** (la
   forme), les autres gardent leurs unités.
6. Le backbone est un Transformer ordinaire à **trois canaux d'attention** :
   latérale, verticale, et **d'ordre** — ce dernier étant la structure qu'aucun
   concurrent ne possède.
7. Le retour aux points n'est pas inventé : c'est le vote pondéré de § 9.1,
   démontré (Proposition 7) et déjà implémenté exactement.
8. Le pré-entraînement a un objectif propre, la **modélisation de filtration** :
   prédire les niveaux de mort, les partenaires de fusion et les arités
   masqués. Cibles exactes et gratuites.
9. Les **deux portes qui peuvent tuer le projet** — l'invariance en portée
   (G1) et le plafond d'oracle par classe (G2) — se franchissent **sans une
   heure de GPU d'entraînement**.
10. Le chaînon manquant est un seul objet : un exportateur
    `mhgp9_tower_export` reliant la v9 au réducteur produit. C'est par là qu'il
    faut commencer.

## Sources

- Manuscrit de thèse, parties I–II :
  [`docs/references/MANUSCRIT_THESE_HAUSEUX.pdf`](../../docs/references/MANUSCRIT_THESE_HAUSEUX.pdf),
  Déf. 20–31, Théorèmes 2 à 7, et § 9.1 pour le passage aux points.
- Poster 3IA Côte d'Azur Days, 24–25 septembre 2026, *Higher-order clustering
  for 3D point clouds* : sections 4 (LiDAR, anomalies) et 5 (perspective,
  jetons géométriques).
- Présentation Inria / SZTE du 16 septembre 2026 :
  [`../PolyhedralEncoding/`](../PolyhedralEncoding/).
- Tour et reçus :
  [`morsehgp3D_v9/`](../../morsehgp3D_v9/).
