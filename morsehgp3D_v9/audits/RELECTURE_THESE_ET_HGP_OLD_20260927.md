# Relecture des parties I–II et de HGP-Clusterer historique

27 septembre 2026. Audit de sources, après les deux campagnes de clustering
de points. Parties I et II du manuscrit relues intégralement, puis chemins
actifs de `HGP-old` confrontés au prototype v9 ; contrelectures indépendantes.
Moteur inchangé, aucune nouvelle campagne G4. Ce document corrige
l'interprétation des expériences, **pas leurs données ni leurs scores**.

## Conclusion principale

Le prototype `first_coverage` n'est pas une reproduction de HGP-Clusterer.
Il impose une affectation exclusive des points **avant** condensation.
La thèse et l'ancien code conservent d'abord les incidences pondérées des
facettes, condensent cette hiérarchie, puis attribuent les points par vote.
Les deux opérations ne commutent pas. Les défaites comme les victoires
récentes ne qualifient donc que cette nouvelle projection.

La priorité devient une référence fidèle de la pondération du §9.1,
puis une qualification séparée de l'emboîtement des partitions de points.
Ni retoucher EOM pour gagner sur les exemples connus, ni changer de
géométrie avant d'avoir isolé cet écart ne serait justifié par ces résultats.

## 1. Ce que dit réellement le manuscrit

Source : [manuscrit](../../docs/references/MANUSCRIT_THESE_HAUSEUX.pdf).
Les pages suivantes sont les pages **imprimées** ; ajouter 26 pour la page PDF.

| Passage | Résultat pertinent | Limite à conserver |
|---|---|---|
| Définition 8, p.21 | Les points d'un amas sont `X ∩ δ_r(C)` | Le texte rejette explicitement `C ∩ X` ; les couvertures peuvent se chevaucher |
| Théorème 2, p.60–61 | Correspondance exacte des K-polyèdres avec les amas discrets de surdensité K-NN | Pas un théorème sur l'ARI d'une sélection EOM |
| Théorème 3, p.71–72 | Fraction récupérable asymptotique dans un modèle à deux densités | Hypothèses et modèle particuliers, pas domination de tous les échantillons finis |
| §7.4, p.73–80 | Comparaison des vitesses de percolation, puis discussion gaussienne | Comparaison empirique à K fixé ; certains passages gaussiens sont admis ou conjecturaux |
| Chapitre 8 | Réductions Gabriel/MST préservant les composantes pertinentes | Préserver la connexité ne conserve pas automatiquement une mesure d'incidences |
| §9.1, p.96–97 | Masses normalisées, condensation et vote | La proposition 7 garantit une partition pour une sélection fixée, pas toutes les coupes emboîtées |
| Algorithme 1, p.100 | Les scores de facettes sont calculés avant le MST | Il faut déclarer le catalogue de cofaces contributives |

La piste `C ∩ X` proposée dans la précédente passation est donc **retirée
comme restitution fidèle de l'objet de la thèse**. Elle définirait un autre
modèle, limité aux points appartenant eux-mêmes à l'ensemble de surdensité.
Les anciens documents/captures restent historiques ; cette correction prévaut.

### L'attente « HDBSCAN ne doit jamais faire mieux »

Le manuscrit donne une forte motivation et de nombreux résultats favorables
à HGP, mais ne démontre pas cette affirmation universelle. Il publie même
une exception explicite dans le tableau 9.3, p.102 :

| `birch2`, 100 000 points, 100 groupes vrais | ARI | Groupes trouvés | Points classés |
|---|---:|---:|---:|
| HDBSCAN | 0,996 | 100 | 99,7 % |
| HGP-Clusterer | 0,441 | 84 | 83,9 % |

La p.103 explique les fusions intempestives sur ces groupes filiformes et
rapporte qu'un exposant 2 corrige ce cas, avec un nombre de groupes presque
parfait. Il ne faut ni effacer cette exception, ni en déduire un échec de
la géométrie. EOM optimise une stabilité interne, pas les labels générateurs.
Inversement, cette limite théorique n'excuse pas notre écart d'implémentation.

## 2. La mesure manquante dans le prototype récent

Fixons le catalogue contributif `C`, son univers de facettes `F = ∂C`,
et `z = expZ`. Une facette τ contient K points, une coface σ en contient K+1.
Pour des rayons strictement positifs :

\[
S_\tau=\sum_{\sigma\in C,\ \sigma\supset\tau}\rho_\sigma^{-z},\qquad
T_x=\sum_{\tau\in F,\ x\in\tau}S_\tau,
\]
\[
w_{x\tau}=S_\tau/T_x,\qquad
m_\tau=\sum_{x\in\tau}w_{x\tau}.
\]

Tout point couvert distribue une masse totale de 1 entre ses facettes.
La somme des masses de toutes les facettes vaut le nombre de points couverts.
Une somme sur une coupe partielle peut être plus petite : les facettes
non encore présentes exigent une réserve, pas une renormalisation implicite.

La condensation travaille avec `m_τ`. Après sélection, le vote du point x
pour le cluster c est la somme des `w_{xτ}` des facettes étiquetées c.
Le dénominateur `T_x` étant commun, l'argmax peut sommer les `S_τ`.
Il faut déclarer séparément bruit, égalités, points non couverts et éventuelle
propagation 1-NN ; cette dernière n'est pas active par défaut dans `HGP-old`.

| Étape | HGP-old actif | Prototype `first_coverage` |
|---|---|---|
| Recouvrement | Incidences de facettes conservées | Une seule ancre par point, report au LCA en cas de conflit simultané |
| Masse avant condensation | Masse fractionnaire `m_τ` des facettes | Masse unitaire des points déjà affectés |
| `expZ` | Modifie `S_τ`, masses, votes et densités | Modifie les dates/stabilités EOM, pas la projection ni ses masses |
| Seuil minimal | Masse pondérée des facettes | Nombre de points dans l'arbre projeté |
| Sortie ponctuelle | Vote après sélection | Affectation fixée avant sélection |
| Racine EOM | Peut être sélectionnée | Exclue dans le protocole commun |

Sources actives : [core.py](../../HGP-old/src/hgp_clusterer/core.py), lignes
202–215, 255–263, 297–324 ; [hypergraph.py](../../HGP-old/src/hgp_clusterer/hypergraph.py),
lignes 119–128 ; [_cython.pyx](../../HGP-old/src/hgp_clusterer/_cython.pyx),
accumulation des scores vers 838 et condensation à partir de 481.
La voie récente annonce d'ailleurs explicitement
`facet_weighted_thesis_votes=False` dans
[projection.py](b_point_hierarchy_k_20260927/projection.py).

Le `min_cluster_size` pondéré ne garantit **pas** la même cardinalité des
labels durs après compétition des votes. Les bornes de F1 fondées sur
« chaque cluster final contient au moins m points » dans le lot gaussien
restent correctes pour son prototype, pas automatiquement pour l'ancien vote.
De même, l'invariance de son arbre condensé lorsque seul z change n'est pas
une propriété de la méthode historique avec ses masses dépendant de z.

`HGP-old` n'est pas un nouvel oracle exact : float32, régularisations,
plafonnement de l'inverse des rayons très petits et voies géométriques
optionnelles doivent rester visibles. `weight_face` est stocké mais ne
sélectionne pas les anciennes branches commentées dans le chemin actif.

## 3. Deux contrôles concrets, sans changer le moteur

### Perte par première couverture

Sur les trois points entiers `(0,0,0)`, `(2,2,0)`, `(2,0,2)`, à K=2 :

- FULL contient trois minima de niveau `r²=2`, fusionnant à `r²=8/3`.
- Le contrôle natif rejoue les sept coupes et confirme les couvertures.
- Les trois points ont chacun deux premières couvertures simultanées.
  `first_coverage` les reporte tous au parent ; il ne reste qu'un nœud
  interne dans l'arbre de points.
- Avec m=2 et racine exclue, EOM rejette alors les trois points.

Capture locale neuve, pas un reçu fabriqué après un essai non archivé :
`build/v9-point-projection-audit-20260927/r1/receipt.json`, SHA256
`7849a8d7c8097cf5b40b5cc4b20a4067140b72c83e75d52c1d474dc935970038`.
Commande native : `native_export --input equilateral.u32le --k 2 --workers 1 --verify-coverage`.
Le reçu conserve les chemins complets, hashes avant/après et code retour 0.
Ces fichiers privés ne constituent pas une archive publique autonome.

Cela démontre une perte dans la projection, **pas** que l'ancien EOM gagne
sur ce jouet : il faudrait aussi aligner ses masses, seuil et politique racine.
Les trois ensembles `{A,B}`, `{A,C}`, `{B,C}` ne peuvent de toute façon pas
être simultanément des blocs d'une hiérarchie stricte de points.

### Une réduction topologique ne suffit pas aux votes

Rejeu en lecture seule, normal et `python -O`, de `run_cases()` dans
[masses_vote_probe.py](../../morsehgp3D_v7/audits/masses_vote_probe.py).
Les deux appels passent ; aucun appel à son `main` qui écrirait l'archive v7.
La fixture rationnelle comporte 7 points, 35 cofaces Čech et 21 facettes.
Deux retraits différents d'une coface redondante laissent les mêmes tokens
de fusion et toutes les mêmes coupes, mais donnent des vainqueurs opposés
au vote du point 1. Les quatre corruptions mathématiques sont rejetées.

Cette fixture compare des catalogues contributifs, **pas** deux catalogues
Gabriel complets distincts du même nuage. Elle prouve qu'un quotient de
connexité n'est pas à lui seul un accumulateur de poids. Avec les coordonnées,
on peut envisager de recalculer les contributions : ce serait un travail
supplémentaire, pas une information déjà contenue dans les seuls parents.

Voir le [contrat des masses](../../morsehgp3D_v7/audits/CONTRAT_MASSES_VOTE_COURANT.md).
Le catalogue doit être nommé avant toute reproduction : catalogue de Gabriel
de l'algorithme 1 (triangles obtus compris), ou catalogue réellement produit
par une voie historique Delaunay d'ordre supérieur. Même lorsqu'une
équivalence topologique est établie, elle ne rend pas les sommes d'incidences
identiques. Pour z fixé, des `S_τ` agrégés peuvent suffire ;
un histogramme rayon/multiplicité garde la liberté de changer de fonction ψ.
Il faut aussi préserver l'affectation datée des facettes à l'arbre T_K.

## 4. L'embryon utile pour des partitions réellement emboîtées

Dans [clustering.py](../../HGP-old/src/hgp_clusterer/clustering.py),
`GetClusters(..., whole_tree=True)` agrège les votes puis distribue les points
**du parent vers un seul enfant**, par score maximal (lignes 398–561).
C'est une meilleure piste de départ que de retarder automatiquement chaque
point partagé jusqu'au LCA. Ce n'est cependant pas le chemin `fit()` usuel.

Deux précautions sont indispensables :

1. Le code démarre séparément de chaque racine sélectionnée avec tous ses
   points ; des racines peuvent partager des points. Il manque une attribution
   globale exclusive initiale pour en faire une partition de tout le nuage.
2. Le vote plat de la proposition 7, répété indépendamment aux coupes, n'est
   pas emboîté : des votes `(A,B,C)=(4,3,3)` font choisir A, puis B∪C après
   fusion de B et C. Un autre point votant seulement pour A y reste ; l'ancien
   bloc A se trouve donc scindé. Le routage descendant impose une autre règle.

Proposition à qualifier, pas méthode déjà mesurée : une racine globale,
des scores agrégés non négatifs, un routage descendant exclusif et des
résidus datés conservés comme singletons. L'emboîtement découle du fait
qu'un enfant ne reçoit que des points de son parent. Il faut encore fixer
le traitement des attaches tardives et le sens du seuil après routage.
Cette conversion reste distincte du vote plat historique ; aucun théorème
ne garantit qu'elle améliore tous les scores.

Ne pas copier aveuglément l'implémentation `whole_tree` : ses tableaux de
points répétés aux ancêtres et sa matrice points×enfants peuvent coûter
très cher. Un futur port doit exploiter des incidences creuses et publier
son travail total, pas seulement le temps d'EOM.

## 5. Suite de qualification recommandée

1. Fixer le catalogue de cofaces, la masse et les dates d'attachement à T_K.
   Commencer par des petits oracles exhaustifs, sans utiliser les autres
   niveaux de la tour pour décider les labels.
2. Reproduire le §9.1 : scores et masses avant condensation, même fonction
   de stabilité EOM, puis vote. Pour z=1 **et** z=2, recalculer également les
   masses ; ne pas seulement élever les niveaux de densité au carré.
3. Réutiliser les mêmes entrées gaussiennes et scènes 3D. Fixer avant les
   scores racine, bruit, exæquos, propagation éventuelle et convention
   `min_samples` HDBSCAN (inclut-il le point lui-même ?). Le K géométrique
   et ce paramètre de lissage ne sont pas des synonymes implicites.
4. Comparer séparément la version historique pondérée, la baseline première
   couverture, et le futur routage emboîté. Publier aussi les cardinalités
   finales après vote, pas seulement les masses au seuil.
5. Garder les défaites et les victoires, puis tester de nouvelles graines.
   Aucun réglage choisi sur la vérité terrain du lot d'évaluation.

Pas de nouveau grand chantier ni de dépense GCP engagé par cette relecture.
Elle établit un défaut de fidélité méthodologique, **pas** une preuve que
sa correction inversera chaque comparaison.

## Empreintes des sources relues

Base Git : `01dc72e006746c08be746fa77fabf5a533b16139`.

```text
579f83671ebca34cd810f350820074eb42672411713160f9c9c2a458ff4f4fef  manuscrit PDF
b8d2763b3c51541b4e86d75384a53be115034fc092678f4218fb74e12f88cf0c  HGP-old/core.py
0d1a888229191b9be8405ceaf78e55fcb4fbc02af2aca42f0e3daa534182d60d  HGP-old/hypergraph.py
3cc4488327357b49eac0bcf5ce3acffbd4d6609e0ff618d71ffca46f8fe416ce  HGP-old/clustering.py
d3ba8b97a0a54d59f280f70db08ae53842c0e661690d8b644fdafc97ac322722  HGP-old/_cython.pyx
dfd95efe5fba6136410526952759e1d370e0bc89ab9bdd1c07118747ddf7fc41  masses_vote_probe.py
```
