# PolyTreeFormer — segmentation LiDAR sur hiérarchie polyédrique

> **Question de recherche.** Peut-on remplacer les points comme unités apprises par la hiérarchie fine de facettes et de polyèdres issue de la filtration de densité, afin d'obtenir une représentation plus stable à la portée du capteur tout en conservant une sortie sémantique par point ?

## Décision scientifique

La voie principale est désormais **polyèdre-only** :

- les points servent au prétraitement géométrique et à la reprojection finale ;
- aucun point n'est un token du réseau ;
- les tokens élémentaires sont les facettes sérialisées ;
- les tokens internes sont les nœuds de l'arbre de fusion ;
- l'apprentissage combine le graphe d'incidence local, l'évolution le long des branches et les fusions multi-échelles.

Le nom de travail du modèle est **PolyTreeFormer**. Il ne constitue pas une revendication de nouveauté.

## Hypothèse centrale

Sous une dilution approximativement multiplicative de l'échantillonnage LiDAR, une hiérarchie de densité idéale est préservée à une reparamétrisation monotone du niveau près. Le réseau doit donc privilégier :

- les différences de niveaux logarithmiques ;
- la persistance et les rapports de masse ;
- la forme normalisée ;
- la combinatoire d'incidence et de fusion.

La taille physique, la hauteur, la pose, la portée et la rémission restent disponibles dans des canaux séparés. L'objectif n'est pas de rendre le modèle aveugle au monde physique, seulement moins dépendant du nombre accidentel de retours.

Cette invariance est une **hypothèse à falsifier**, pas un résultat acquis : occultations, angle d'incidence, anneaux et retours manquants produisent une dilution non homogène.

## Architecture retenue

La première implémentation ne part pas d'un Transformer inventé pour l'occasion. Elle adapte le mode **`nano` de Superpoint Transformer**, qui supprime déjà l'étage point-wise et traite uniquement une hiérarchie de régions.

1. **Encodeur local sparse** sur le graphe dual des facettes.
2. **Encodeur-décodeur hiérarchique** parent–enfants avec arêtes latérales.
3. **Prédiction par facette**, puis reprojection massique vers les points.
4. **Pré-entraînement teacher–student** entre scan complet et scan physiquement aminci.
5. **HSA** et l'attention de type Sequoia sont des opérateurs comparatifs, ouverts seulement après validation de la tokenisation et des canaux.

La description précise est dans [ARCHITECTURE.md](ARCHITECTURE.md) et [ENTRAINEMENT.md](ENTRAINEMENT.md).

## Ordre des travaux

| Porte | Question | Décision |
|---|---|---|
| G0 | la sérialisation conserve-t-elle masse, incidences et ordre des points ? | sinon arrêt |
| G1 | l'oracle facette permet-il encore largement le score visé ? | sinon raffiner les feuilles |
| G2 | l'arbre reste-t-il stable sous thinning réaliste ? | sinon l'argument d'invariance tombe |
| G3 | un SPT-nano adapté apprend-il utilement sans tokens points ? | sinon abandon du polyèdre-only strict |
| G4 | la hiérarchie réelle bat-elle arbres aléatoires, octree et HDBSCAN à budget égal ? | sinon la structure spécifique n'aide pas |
| G5 | les canaux de filtration apportent-ils plus que les raccourcis capteur ? | sinon réduire le claim |
| G6 | le pré-entraînement de portée améliore-t-il probing et fine-tuning sur trois graines ? | sinon ne pas complexifier |
| G7 | le gain se transfère-t-il à un second capteur ? | requis pour une revendication générale |

Les seuils, contrôles et règles d'arrêt sont détaillés dans [PROTOCOLE.md](PROTOCOLE.md).

## Lecture du dossier

| Document | Rôle |
|---|---|
| [GUIDE.md](GUIDE.md) | vue pédagogique de bout en bout |
| [ARCHITECTURE.md](ARCHITECTURE.md) | tokens, canaux, graphes et Transformers |
| [ENTRAINEMENT.md](ENTRAINEMENT.md) | supervision, pré-entraînement et optimisation |
| [PROTOCOLE.md](PROTOCOLE.md) | expériences, métriques, contrôles et portes |
| [RISQUES.md](RISQUES.md) | difficultés techniques et scientifiques |
| [CONCURRENCE.md](CONCURRENCE.md) | positionnement par rapport aux travaux existants |
| [VOIES.md](VOIES.md) | feuille de route exécutable |
| [REFERENCES.md](REFERENCES.md) | sources primaires retenues |
| [GLOSSAIRE.md](GLOSSAIRE.md) | vocabulaire ML et implémentation |

Le dossier [`archive/`](archive/) conserve les formulations antérieures. Il n'est plus normatif.

## Périmètre expérimental principal

- SemanticKITTI, segmentation sémantique **mono-scan** ;
- LiDAR seul à l'inférence ;
- entrée brute : `(x, y, z, remission)` ;
- entraînement : séquences `00–07, 09, 10` ;
- validation : séquence `08` ;
- sans TTA ni ensemble pour le résultat principal ;
- sortie : 19 logits dans l'ordre exact des points du fichier `.bin`.

Les chiffres de concurrence changent plus vite que la géométrie. Aucun score n'est déclaré « état de l'art » sans réaudit du régime, du split et des données utilisées.

## Statut

`research_status = design_and_falsification`

Aucun résultat appris n'est revendiqué à ce stade. La première contribution attendue est une réponse expérimentale nette à la question suivante :

> Une hiérarchie polyédrique fine conserve-t-elle suffisamment d'information sémantique et suffisamment de stabilité sous changement de densité pour devenir l'espace principal d'apprentissage ?
