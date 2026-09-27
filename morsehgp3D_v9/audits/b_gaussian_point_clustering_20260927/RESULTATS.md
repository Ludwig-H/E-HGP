# Gaussiennes 3D : résultats et condensation de l'arbre de points

27 septembre 2026. **48 scènes complètes, 96 exports natifs, 384 fits
HDBSCAN et 1 920 lignes de résultats ; aucun échec.** Calcul CPU local,
sans GCP, sans modification du moteur géométrique. Le seul arbre T_K
alimente la projection de points « première couverture » du lot précédent.

**Correction de portée après relecture de la thèse et de HGP-old :** dans
tout ce bilan et ses tableaux, « HGP » désigne **HGP + projection première
couverture**, pas le clusterer pondéré historique. Celui-ci condense les
facettes pondérées avant de voter vers les points ; ici, les points sont
affectés avant condensation. `expZ` n'a donc pas non plus sa portée complète
historique. Voir l'[audit de sources](../RELECTURE_THESE_ET_HGP_OLD_20260927.md).
Les captures figées, labels et valeurs numériques restent inchangés.

Résultat principal : les groupes bien séparés sont presque parfaitement
retrouvés. HGP est nettement meilleur sur les gaussiennes sphériques à
séparation intermédiaire de ce lot, mais les deux méthodes échouent quand
les composantes se recouvrent fortement. Les gaussiennes allongées révèlent
une vraie faiblesse de la sélection HGP, malgré un arbre prometteur.

Les [tableaux détaillés](results_r1/TABLES.md), la [carte des scores](results_r1/primary.svg),
les [1 920 lignes CSV](results_r1/scores.csv) et les [agrégats](results_r1/aggregates.csv)
conservent tous les paramètres, sans choisir le meilleur après coup.

## Ce qui a été testé

Chaque scène contient **1 200 points**, sans bruit artificiel ajouté.
La vérité terrain est la composante gaussienne génératrice de chaque point.

- 36 scènes sphériques équilibrées : 2, 4, 8 ou 16 communautés,
  séparation minimale des moyennes δ=8, 4 ou 2 écarts-types, trois graines.
- 12 scènes de stress à huit communautés : gaussiennes allongées
  d'écarts-types principaux (2, 1, 0,5), ou tailles déséquilibrées 4:1 ;
  trois séparations, deux graines par cellule.
- K=5 et K=10 ; `min_cluster_size`=10, 20, 50 et 100 ; expZ=1 et 2.
  Référence fixée avant les scores : **K5, taille minimale20, expZ1**.

HGP et HDBSCAN reçoivent les mêmes coordonnées entières u18 préparées
isotropiquement ; aucun doublon ni fusion. Ce sont des unités synthétiques,
pas une qualification de précision LiDAR. `min_samples=K` reste fixé chez
HDBSCAN lorsque le seuil de taille change. Même condensation, EOM, masses
unitaires, traitement simultané des exæquos et racine non sélectionnable.
Les labels HDBSCAN standards sont aussi conservés, séparément.

Le générateur, les paramètres vrais, les tirages appariés entre séparations
et les empreintes des entrées sont documentés dans [DATASETS.md](DATASETS.md).
Le diagnostic MAP à paramètres connus n'intervient jamais dans les clusters
ou le choix des paramètres.

## `min_cluster_size` nettoie maintenant un arbre explicite

Le seuil existait dans EOM ; la nouvelle [API](CONDENSATION.md) expose aussi
l'arbre condensé : parents/enfants des clusters, naissances, décès, masses,
stabilités et sorties datées de **tous** les points.

```python
from condensed import point_clusterer_from_tree

result = point_clusterer_from_tree(tree, min_cluster_size=20, exp_z=1)
cluster_tree = result["condensed_tree"]
labels = result["selection"]["labels"]
```

`tree` est l'arbre de points déjà projeté depuis T_K, avec `n`, `children`
et `heights` en rayons. Changer le seuil ne recalcule pas la géométrie.
Il ne change pas K.

En allant vers les zones plus denses, une branche trop petite disparaît
comme cluster ; un seul grand enfant prolonge son parent ; plusieurs grands
enfants créent une bifurcation. On perd volontairement les petites
sous-branches, **pas les identifiants des points**. Les points sortis sont
des singletons distincts pour compléter une coupe en partition, jamais un
unique « cluster bruit ». Les partitions restent emboîtées.

Exemple réel, deux gaussiennes très séparées, première graine, K5 :
952 nœuds internes de points donnent 55, 19, 7 puis 3 nœuds de clusters
pour les seuils 10, 20, 50, 100. Les 1 200 sorties de points restent présentes.
EOM retrouve les deux communautés au seuil20.

Sur les 36 scènes sphériques, K5/expZ1 :

| Seuil | Nœuds condensés HGP, moyenne | Nœuds condensés HDBSCAN, moyenne | ARI moyen HGP / HDB commun |
|---:|---:|---:|---:|
| 10 | 59,31 | 23,81 | 0,480 / 0,399 |
| 20 | 22,25 | 13,00 | 0,527 / 0,412 |
| 50 | 11,36 | 9,06 | 0,514 / 0,403 |
| 100 | 5,56 | 4,83 | 0,401 / 0,346 |

Avant condensation : 938,17 nœuds internes HGP en moyenne, 1 199 chez
HDBSCAN. Les colonnes condensées incluent la racine et **n'incluent pas**
les 1 200 sorties individuelles. Réduire les branches n'implique pas une
amélioration monotone des scores ni une sélection EOM monotone.

Une borne structurelle simple vaut pour cette représentation :
`C <= max(1, 2*floor(n/m)-1)`. Hors racine seule, les feuilles-clusters ont
des ensembles de naissance disjoints de taille au moins m, et chaque nœud
interne a au moins deux enfants. D'où au plus `floor(n/m)` feuilles et
`2*floor(n/m)-1` nœuds. Le stockage complet reste **O(n+C)**, pas O(n/m) :
on conserve les points. La validation implémentée coûte au pire O(n log n)
à cause des tris locaux de dates. Cette borne ne porte ni sur le générateur
géométrique ni sur les diagnostics supervisés de qualité.

## Qualité contre la vérité terrain

Référence K5/taille20/expZ1. Chaque ligne sphérique ci-dessous moyenne
12 scènes : quatre nombres de communautés, trois graines chacun.
ARI=1 indique la partition vraie ; près de zéro, pas de récupération utile.
La couverture est la proportion de points auxquels EOM affecte un cluster.

| Séparation sphérique | ARI HGP | ARI HDB commun | ARI HDB standard | Couverture HGP / HDB commun |
|---|---:|---:|---:|---:|
| Forte, δ8 | 0,9997 | 0,9989 | 0,9988 | 100,0 % / 99,9 % |
| Intermédiaire, δ4 | 0,5633 | 0,2228 | 0,2201 | 82,4 % / 73,3 % |
| Faible, δ2 | 0,0175 | 0,0139 | 0,0140 | 65,2 % / 40,6 % |

Les moyennes/écarts-types par G figurent dans les tableaux détaillés ;
trois graines ne sont pas une démonstration de supériorité générale.
À δ4, HGP gagne 11 comparaisons appariées sur12 contre l'EOM HDB commun ;
à δ2, six sur12, avec des scores très faibles des deux côtés.

L'ARI usuel considère tous les labels -1 comme un même groupe. Le CSV
publie donc aussi une variante où **chaque point rejeté est un singleton** :
à δ4, elle donne 0,5929 HGP contre 0,2735 HDB commun ; à δ2,
0,0165 contre 0,0138. Les rejets ne sont pas des détections de bruit vrai :
tous les points ont une communauté génératrice.

### Allongement et déséquilibre : ne pas masquer les échecs

À huit communautés et δ4, deux graines :

| Régime | ARI HGP / HDB commun | ARI avec rejets singletons HGP / HDB | Couverture HGP / HDB |
|---|---:|---:|---:|
| Allongé | 0,1970 / 0,1856 | **0,1990 / 0,2859** | 95,0 % / 70,2 % |
| Déséquilibré 4:1 | 0,5359 / 0,2686 | 0,5680 / 0,2762 | 86,0 % / 89,7 % |

Sur l'allongé intermédiaire, le petit avantage en ARI usuel serait trompeur :
le F1 moyen des classes après appariement vaut **0,194 HGP contre 0,441 HDB**.
HGP fusionne trop largement : seulement 2,5 clusters en moyenne pour huit
composantes. Ce n'est pas corrigé par une meilleure couverture.

### Arbre et sélection ne racontent pas la même chose

À δ4 sphérique, pureté du dendrogramme brut : 0,7190 HGP / 0,5314 HDB.
Le meilleur F1 disponible par classe dans les branches condensées vaut
0,8443 / 0,7131. Pour l'allongé δ4, ce dernier diagnostic reste favorable
à HGP, 0,8069 / 0,6750, malgré sa mauvaise sélection finale.

Ces meilleurs F1 sont des **oracles supervisés de diagnostic** : chaque
classe peut choisir sa meilleure branche, y compris une branche qui
chevauche celle choisie pour une autre classe. Ce n'est ni une partition
réalisable simultanément, ni une nouvelle méthode de sélection, ni un score
de clustering non supervisé. Cela motive l'étude de la sélection, sans
prouver qu'un meilleur EOM suffirait à retrouver les classes.

À δ2, l'arbre HGP lui-même se dégrade nettement : pureté0,2764 et meilleur
F1 condensé0,3825. Modifier seulement l'extraction ne résoudra pas tout.
Même le MAP à paramètres vrais ne classe correctement que62,7–84,6 % des
points sphériques selon G. À δ4, ce diagnostic reste à94,5–97,9 % : on ne
peut donc pas attribuer tous les échecs intermédiaires au seul recouvrement.
Ces plages portent sur les moyennes des trois graines pour chaque G,
pas sur les extrema des douze scènes individuelles.
Le MAP connaît le générateur ; ce n'est ni un concurrent équitable ni une
borne supérieure de l'ARI. Des composantes recouvrantes ne sont pas
nécessairement des modes de densité distincts.

## Effet de K, expZ et du seuil

Sur les 36 scènes sphériques, taille20, mêmes scènes pour chaque ligne :

| K | expZ | ARI HGP | ARI HDB commun |
|---:|---:|---:|---:|
| 5 | 1 | 0,5268 | 0,4119 |
| 5 | 2 | 0,5144 | 0,4314 |
| 10 | 1 | 0,5369 | 0,3947 |
| 10 | 2 | 0,5447 | 0,3947 |

Dans ce prototype, expZ2 change les stabilités et la sélection, **pas les
branches du même arbre condensé**. Dans HGP-old, il change aussi les masses
de facettes : ne pas transférer cette invariance. Son effet mesuré dépend
du régime et de K ; ce n'est pas une
amélioration universelle. La référence reste le réglage fixé avant les scores.

Le seuil100 illustre un piège important : à G16, chaque vraie classe contient
75 points. Aucune ne peut être exactement un cluster admissible. Même le
meilleur recouvrement possible avec un ensemble de taille au moins100 est
borné par `2*75/(75+100)=6/7`. Dans le stress déséquilibré, les quatre classes
de60 points ont une borne de0,75. Ces cas sont conservés, avec leurs
indicateurs d'inéligibilité, pas supprimés pour améliorer la moyenne.
Ces bornes concernent le seuil cardinal du prototype. Un seuil sur les
masses fractionnaires de facettes n'impose pas la même cardinalité finale
après vote ; elles ne s'appliquent donc pas automatiquement à HGP-old.

## Comparateur HDBSCAN et garanties de la campagne

La comparaison principale utilise un EOM commun avec exæquos simultanés.
Les labels diffèrent du HDBSCAN standard dans217des384configurations
expZ1. Sans atomisation des exæquos, l'adaptateur reproduit les partitions
standards dans384sur384. Les deux sorties sont archivées.

L'écart absolu moyen d'ARI commun/standard est0,00124, mais le maximum
atteint0,23859 : allongé, δ4, deuxième graine, K5/taille20. Ici l'EOM commun
donne huit clusters et ARI0,30489, le standard deux et ARI0,06629.
Ne pas présenter les écarts comme toujours négligeables ; le résultat HDB
commun n'est pas une exécution du sélecteur standard inchangé.

Qualification préalable : neuf commandes passent, normal et Python−O,
incluant génération, condensation, métriques, pipeline et porte native.
Le [reçu](results_r1/receipt.json) ferme sources, exécutable et entrées.
Les [contrôles statistiques indépendants](AUDIT_RESULTATS.md) et le
[contre-audit structurel](post_audit/README.md) complètent cette preuve.
Sources précédentes inchangées ; gros exports et données préparées privés,
scripts, tableaux et empreintes publics. Ce n'est pas une archive autonome
des payloads : leurs relectures complètes nécessitent les fichiers privés
ou une reproduction explicite.

## Conclusion pratique et limites

Garder la première couverture comme baseline, l'API de condensation explicite
et un seuil configurable indépendant de K. Dans ce lot,20 est un compromis
utile, pas un seuil universel. Un seuil élevé nettoie fortement mais peut
effacer les communautés que l'on cherche précisément à détecter.

La relecture ultérieure donne priorité à une restitution fidèle des masses
et du vote historiques, puis à une qualification séparée du routage emboîté.
Ne pas modifier seulement EOM avant d'avoir isolé l'effet de la projection.
Tester ensuite sur d'autres graines sans sélectionner les paramètres à la
vérité terrain. Les résultats ne
justifient ni une nouvelle géométrie coûteuse ni une proclamation de victoire
générale sur HDBSCAN.

À n fixé, augmenter G diminue aussi l'effectif par communauté : l'effet G
n'est pas isolé. Les agrégats réunissent toutes les graines, y compris celles
marquées développement ; ce sont des résultats descriptifs, pas une seule
évaluation tenue à l'écart. L'ordre des points est commun aux méthodes et
groupé par composante ; pas de nouveau test global de permutation dans ce lot.

Le rattachement HGP initial est rationnel ; condensation/EOM restent un
prototype Python binary64, non certifié numériquement. Les temps incluent
exports, JSON et contrôles, et les calculs natifs amont1..K : **aucune
comparaison de vitesse équitable ni qualification100ms/G4**. Avec une seule
taille n=1200, aucune nouvelle mesure de croissance sous-quadratique du moteur.
