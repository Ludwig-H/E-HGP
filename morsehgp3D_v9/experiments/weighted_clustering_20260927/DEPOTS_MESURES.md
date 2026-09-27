# Comptage capturé des dépôts datés — 27 septembre 2026

Deux cas **déjà clos**, G2/δ8/graine1, 1 200 points, K5 et K10, z1.
Ce sont des comptages postérieurs à la capture, pas des résultats de la
campagne complète, une implémentation d'EOM à dépôts ou un benchmark de gain.
Aucun payload ni module gelé n'a été modifié. Voir la
[preuve et ses conditions](DEPOTS_DATES.md).

## Source et reçus privés

Le nouveau [measure_deposit_groups.py](measure_deposit_groups.py) est épinglé
par SHA256 `17383b3dfe0b37b06a50d1d313cf4bbdb50c4c2eddee1487fab239c0ba35a281`.
Les deux rapports JSON neufs sont sous
`/tmp/mhgp9-deposit-group-census-20260927.2eQBDHxG/` :

| Rapport | SHA256 |
|---|---|
| `k5.json` | `2f9e0e49842bdc839955eb8eed4aea774ca3f81e0561fcb79ee134114ca8bc30` |
| `k10.json` | `94b38cadbf152798fbac2a9923789d40472c56a0800250aa9f9d875737764ec1` |

Chaque rapport ferme six fichiers avant/après : `native.json`,
`measure_z1.json.gz`, `command.json`, le dernier payload `weighted_m50_z2.json.gz`,
le script et l'exécutable Python. La sortie native est reliée au hash de sa
commande réussie. Les facettes, attaches exactes et dates du measure sont
recoupées avec le natif. Les six pins ont été vérifiés LIVE après capture.
Les compteurs capturés égalent les premiers comptages exploratoires.
Les chemins privés d'origine sont conservés, même derrière un déplacement
en lien symbolique coordonné par le responsable du pilote.

## Tailles observées

F = facettes ; D = couples `(nœud FULL fermé, β MEB exacte)` ;
J = couples `(dépôt, PointId)` non nuls. La positivité des scores permet
de compter J par union des IDs, sans annulation numérique.

| Quantité | K5 | K10 |
|---|---:|---:|
| Cofaces C | 28 678 | 78 532 |
| Facettes F | 108 909 | 689 531 |
| Dépôts D | 55 915 | 174 331 |
| Incidences K·F | 544 545 | 6 895 310 |
| Incidences J | 309 138 | 1 828 667 |
| Nœuds FULL | 35 068 | 111 819 |
| Internes de l'arbre pondéré | 34 925 | 106 889 |
| Nœuds pondérés, feuilles comprises | 143 834 | 796 420 |
| F/D | 1,948 | 3,955 |
| K·F/J | 1,761 | 3,771 |

La réduction du nombre d'incidences n'est pas la même chose qu'une réduction
des octets. À titre **illustratif**, IDs uint32 et scores/masses binary64 :
`4KF+16F` octets pour les IDs, scores et masses par facette contre
`12J+8D` pour IDs/scores ponctuels et masses par dépôt donnent
3 920 724 → 4 156 976 octets à K5, et
38 613 736 → 23 338 652 octets à K10. Ce sous-total exclut topologie,
dates, conteneurs Python, alignement, preuves et travail transitoire ;
aucune économie mémoire totale n'est mesurée ici.

## Groupes par coface et travail combinatoire

Pour chaque coface σ, le script choisit un support Q **contenu dans σ**
parmi les masques natifs. Il vérifie avec Fraction que toutes les omissions
hors Q ont la même naissance MEB et le même nœud FULL fermé, égal à l'ancre
de σ normalisée à cette date. Les données natives de support sont
consommées comme provenance ; ce script ne recalcule pas leur MEB.

| Compteur | K5 | K10 |
|---|---:|---:|
| Cofaces avec g=3 | 3 536 | 3 151 |
| Cofaces avec g=4 | 16 399 | 31 173 |
| Cofaces avec g=5 | 8 743 | 44 208 |
| Maximum g | 5 | 5 |
| Omissions hors Q vérifiées | 80 827 | 587 199 |
| Expansion naïve K(K+1)C | 860 340 | 8 638 520 |
| Accumulation groupée Σ(K+1)g | 719 514 | 3 907 035 |
| Recherches de groupes (K+1)C | 172 068 | 863 852 |
| Groupée + recherches | 891 582 | 4 770 887 |
| Référence factorisée : (K+1)C + KF | 716 613 | 7 759 162 |

Dans ces données, g=|Q|+1 à chaque coface. La formule groupée réduit
l'expansion **naïve** d'un facteur 1,196 / 2,211. Le code actuel ne fait
pas cette expansion : il calcule d'abord les scores de facettes puis
parcourt KF. Le tableau conserve donc aussi son compte factorisé.
Ces comptes ne pondèrent ni les recherches, ni les allocations, tris,
calculs d'attaches, masses, sorties ou votes ; **aucun gain de temps réel
n'en découle**. Les 5,25 / 31,57 secondes du lecteur sont uniquement son
coût de diagnostic, jamais une durée de construction HGP.

Les masses maximales de dépôt sont environ 0,19547 / 0,07002. Ces cas
ne testent donc pas le danger d'un dépôt de masse supérieure au seuil
qui deviendrait à tort une branche admissible.

## Petit contrôle exact et rejeu

`--selftest` passe en normal et `-O` : 42 fixtures, 838 cofaces et
11 148 contrôles de multiplicité entière. Il compare, par deux écritures
indépendantes, l'expansion coface→facette→point et la formule groupée,
avec poids entiers puis Fraction ; les numérateurs, T_x, masses et leur
conservation sont identiques. Les affectations d'événements y sont
combinatoires, pas une nouvelle qualification géométrique. Ni arrondis
binary64, ni stabilité, ni EOM, ni labels ne sont qualifiés par ce test.

Depuis la racine du worktree, choisir deux **nouveaux** chemins de rapport :

```sh
python3 -B morsehgp3D_v9/experiments/weighted_clustering_20260927/measure_deposit_groups.py --selftest
python3 -B -O morsehgp3D_v9/experiments/weighted_clustering_20260927/measure_deposit_groups.py --selftest
python3 -B morsehgp3D_v9/experiments/weighted_clustering_20260927/measure_deposit_groups.py \
  --case /workspaces/E-HGP/build/v9-weighted-full-gaussian-pilot-20260927-r1/spherical_g2_d8_s1_k5 \
  --output /chemin/prive/neuf-k5.json
python3 -B morsehgp3D_v9/experiments/weighted_clustering_20260927/measure_deposit_groups.py \
  --case /workspaces/E-HGP/build/v9-weighted-full-gaussian-pilot-20260927-r1/spherical_g2_d8_s1_k10 \
  --output /chemin/prive/neuf-k10.json
```

Les durées murales et le chemin de sortie changent au rejeu ; comparer les
compteurs et les pins plutôt que prétendre à une identité byte à byte des
rapports. Les anciennes preuves ne sont ni écrasées ni requalifiées.
