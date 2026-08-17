# HGP-PolyFM — vers un modèle de fondation sur espace d'échelle polyédrique

> **Question centrale.** Une hiérarchie fine de surfaces polyédriques, construite par niveaux de densité, peut-elle devenir un meilleur espace natif d'apprentissage que les points, voxels et superpoints pour la perception 3D ?

## Changement de paradigme

Le modèle n'apprend pas d'abord sur les retours LiDAR. Il apprend sur les **polyèdres HGP** : des recollements de facettes décrivant les surfaces effectivement observées par le capteur, organisés par leur croissance et leurs fusions le long de la filtration de densité.

```text
retours LiDAR
    │
    └─ hiérarchie HGP supposée disponible
          │
          ├─ surfaces polyédriques à chaque nœud
          ├─ trajectoires de croissance le long des branches
          ├─ événements de fusion
          └─ voisinages spatiaux entre branches
                    │
                    ▼
              HGP-PolyFM
                    │
          ├─ segmentation sémantique
          ├─ segmentation d'instance / panoptique
          ├─ détection et localisation
          ├─ recherche de régions
          └─ transfert cross-capteur
```

Les points restent nécessaires pour construire la géométrie et pour la sortie officielle, mais ils ne sont pas les tokens du backbone principal.

## Décision de représentation

Une fonction radiale monocouche est trop restrictive : un polyèdre peut être ouvert, concave ou multicouche depuis un centre donné. L'objet principal est donc la **mesure surfacique attribuée normalisée** du polyèdre. Sa discrétisation compacte utilise une grille sphéro-radiale douce, qui conserve toute la masse de surface, y compris plusieurs intersections dans une même direction.

La connectivité des facettes reste un canal séparé. Des résumés topologiques légers complètent le code compact ; une branche de graphe surfacique, puis un encodeur natif du maillage, servent de corrections et de plafonds si la compression détruit une information utile.

Voir [REPRESENTATION_POLYEDRIQUE.md](REPRESENTATION_POLYEDRIQUE.md).

## Architecture cible

Le modèle cible n'est pas une simple adaptation de Superpoint Transformer.

1. **Surface encoder** : un code fixe par polyèdre à partir de la mesure surfacique normalisée, de sa grille sphéro-radiale et d'un résumé de connectivité, avec branche maillée sous condition.
2. **Branch encoder** : évolution de la forme le long d'une branche, indexée par les écarts de niveau de densité.
3. **Merge-event encoder** : agrégation permutation-invariante des enfants et de la géométrie nouvellement apparue.
4. **Lateral context** : échanges entre polyèdres spatialement voisins appartenant à des branches différentes.
5. **Facet decoder** : les facettes reçoivent le contexte de leurs ancêtres puis produisent les logits reprojetés vers les points.

SPT-nano, Sequoia et HSA restent des comparateurs utiles ; aucun n'est supposé être le cœur naturel du nouveau paradigme.

## Ce qui doit être démontré

Le projet n'est confirmé que si les résultats établissent successivement :

1. **fidélité** : les surfaces sont représentées avec une faible distorsion à budget fixe ;
2. **stabilité** : le code varie moins que les représentations points/superpoints sous portée, thinning, remeshing et changement de capteur ;
3. **suffisance sémantique** : les polyèdres conservent les frontières et les petites classes ;
4. **apport de la hiérarchie** : l'arbre réel bat les contrôles plats, aléatoires et concurrents ;
5. **transfert** : le pré-entraînement améliore linear probing, faible supervision et plusieurs tâches sur plusieurs capteurs.

Un bon score sur SemanticKITTI seul établit un backbone spécialisé. Le terme **modèle de fondation** n'est utilisé qu'après pré-entraînement multi-datasets et transfert multi-tâches.

## Ordre des travaux

```text
P0  audit géométrique du tokenizer
P1  benchmark des représentations de surface
P2  modèle plat sur polyèdres
P3  modèle complet sur l'espace d'échelle HGP
P4  pré-entraînement géométrique et cross-range
P5  pré-entraînement temporel et multi-capteurs
P6  distillation 2D / langage sous condition
P7  évaluation fondation multi-tâches
```

Les portes et règles d'arrêt sont définies dans [PROTOCOLE.md](PROTOCOLE.md) et [VOIES.md](VOIES.md).

## Lecture du dossier

| Document | Rôle |
|---|---|
| [GUIDE.md](GUIDE.md) | vue pédagogique du changement de paradigme |
| [REPRESENTATION_POLYEDRIQUE.md](REPRESENTATION_POLYEDRIQUE.md) | radialité, alternatives et tokenizer retenu |
| [ARCHITECTURE.md](ARCHITECTURE.md) | modèle natif surface × hiérarchie |
| [ENTRAINEMENT.md](ENTRAINEMENT.md) | pré-entraînement géométrique, temporel et cross-capteur |
| [PROTOCOLE.md](PROTOCOLE.md) | quantification, baselines et critères de décision |
| [RISQUES.md](RISQUES.md) | réfutations possibles et plans de repli |
| [CONCURRENCE.md](CONCURRENCE.md) | état de l'art et revendication défendable |
| [VOIES.md](VOIES.md) | feuille de route de recherche |
| [REFERENCES.md](REFERENCES.md) | sources primaires |
| [GLOSSAIRE.md](GLOSSAIRE.md) | définitions normatives |

Le dossier [`archive/`](archive/) conserve les formulations antérieures. Il n'est plus normatif.

## Statut

```text
research_status = representation_falsification
foundation_claim = not_yet_earned
```

La première victoire attendue n'est pas un gros Transformer. C'est la démonstration qu'une surface polyédrique HGP fournit un **meilleur alphabet géométrique** qu'un échantillon ponctuel à budget comparable.
