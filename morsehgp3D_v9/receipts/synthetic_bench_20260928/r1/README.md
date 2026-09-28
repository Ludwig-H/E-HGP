# Reçu — banc synthétique calibré, références du 28 septembre 2026

```text
phase=exploration_v9_hors_registre
profile=synthetic_float64_no_engine
mode=benchmark_only
public_status=not_claimed
```

Campagne des **références seules** (HDBSCAN et témoin négatif) sur le banc calibré
du 28 septembre. Aucun moteur v9 n'intervient : ce reçu établit la cible à battre,
pas un résultat de la tour.

- Code du banc : `morsehgp3D_v9/experiments/synthetic_bench_20260928/`, commit
  `f4d74af7e8ed`, empreinte des quatre modules dans `SUMMARY.json`.
- 51 scènes, 255 mesures par méthode, 772 500 points.
- Commande : `python3 run_baselines.py --out baselines.csv`.

## Rejouabilité, vérifiée

L'audit du 28 septembre a établi que les campagnes du 27 septembre n'étaient pas
rejouables (captures et manifestes sous `/tmp`, disparus). Ce reçu est vérifié :

- le plan reconstruit par le module est **identique** aux 255 couples
  scène-graine du CSV ;
- 45 scènes tirées au hasard dans le CSV ont été **régénérées octet pour octet**,
  digest SHA-256 des coordonnées compris : **zéro écart** ;
- rien ne dépend d'un chemin temporaire.

## Ce que HDBSCAN obtient

ARI moyen, toutes familles et toutes tailles :

| Difficulté | défaut | oracle | liaison simple |
| --- | --- | --- | --- |
| `easy` | 0,970 | 0,982 | 0,717 |
| `medium` | 0,592 | 0,759 | 0,176 |
| `hard` | 0,382 | 0,576 | 0,174 |
| `extreme` | 0,199 | 0,387 | 0,242 |

L'« oracle » est le meilleur `min_cluster_size` par scène, choisi **contre la
vérité terrain** sur huit valeurs. C'est une borne supérieure inatteignable en
usage réel ; c'est elle qu'il faut battre.

## Trois ouvertures mesurées

**Le nombre de groupes casse le réglage figé.** Sphérique, n = 2000, `medium` :

| Groupes | défaut | oracle |
| --- | --- | --- |
| 2 | 0,610 | 0,646 |
| 4 | 0,704 | 0,736 |
| 8 | 0,527 | 0,683 |
| 16 | 0,636 | 0,797 |
| 32 | **0,032** | 0,491 |

À 32 groupes HDBSCAN s'effondre à 0,032 tandis que son propre oracle tient 0,491 :
l'écart n'est pas de la difficulté, c'est un paramètre qui n'a plus de bonne valeur
unique.

**Le coût du réglage figé est maximal sur les familles structurées.** Marge de
l'oracle sur le défaut, par famille : `shells` +0,411, `hierarchical` +0,291,
`spherical` +0,164, puis tout le reste sous +0,09.

**Sur les surfaces, la géométrie est facile et c'est le réglage qui échoue.**
Famille `shells`, support de dimension intrinsèque 2 : la liaison simple au bon
nombre de groupes obtient **1,000**, l'oracle **1,000**, et HDBSCAN par défaut
**0,589**. Le banc y isole donc le problème du paramètre, débarrassé du problème
géométrique. C'est le régime LiDAR, et c'est l'exposant `z = 2`.

## Ce que ce reçu ne dit pas

Il ne compare rien à la tour HGP : aucune méthode de ce dépôt n'y figure. Il ne
promeut aucun statut public. La couverture moyenne de HDBSCAN par défaut est 0,819,
donc près d'un cinquième des points sont rejetés en bruit ; toute comparaison
ultérieure doit publier sa couverture à côté de son ARI, sous peine de comparer un
score obtenu sur des populations différentes.
