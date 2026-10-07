# Repère des boîtes et profondeur du catalogue

Audit du 7 octobre 2026, sur les contrats v12 `fc1f913ce` (numérique `e264de6f2`) et le moteur v11 `ac081a06f`.
Ces témoins portent sur la préparation du port ; aucune implantation native v12 n'est testée.

## Borne supérieure de boîte : s peut valoir 33

Le catalogue v11 représente une boîte de centres par `[lo,hi)` ; `src/catalogue/boxes.cpp:95` calcule
`hi = max(site) + 1`. À l'extrémité du domaine u32, cette borne vaut donc `2^32`, tout en gardant les sites valides.
Pour les sites `(0,0,0)` et `(2^32−1,0,0)`, la fermeture de leur boîte, incluse dans `NUM-COUVERTURE`, exige `s=33`
selon la définition stricte de `NUM-REPERE`. Aux profils 21 et 24, le même témoin exige respectivement 22 et 25 bits.

Ce n'est pas un motif de refus d'une entrée valide. Il faut garder les bornes de boîtes dans un type élargi et
qualifier les budgets pour le domaine de **chaque objet** : sites, boîtes fermées et garde. En particulier, ne pas
calculer `max+1` en u32, ni confondre `B<=32` pour les sites avec `s<=32` pour toute boîte. L'étendue nulle doit être
définie (`s=0` suffit). Le témoin vérifie aussi qu'un site extérieur à une petite boîte participe au repère.

La fenêtre de garde, prise toute entière, peut avoir une étendue de `s+3` bits. Pour le support `{0,3}`, les deux
requêtes `−7` et `11` passent le pavé proposé `(-8,12)` ; chaque requête réunie au support tient sur 4 bits,
mais leur union exige 5 bits. La preuve par requête à `s+2` ne construit donc pas, à elle seule, un unique repère
minimum-sur-tous-les-points de cette largeur. Une preuve avec différences relatives au support reste possible.

## La borne de 38 niveaux est fausse

`ARCHITECTURE.md` § 4.1 annonce au plus 38 niveaux. Deux petits nuages synthétiques produisent davantage de niveaux
sur le parcours v11 qu'il est prévu de porter, avec une seule voie séquentielle, en Release u21 :

| Nuage | K / feuille | Profondeur maximale | Nœuds | Feuilles |
| --- | --- | ---: | ---: | ---: |
| 24 permutations signées de `(1,2,2)`, échelle `2^18`, translation `(2^19)^3` | 2 / 16 | 60 | 927 | 44 |
| 48 permutations signées de `(1,2,3)`, échelle `2^18`, translation `(3·2^18)^3` | 5 / 24 | 63 | 951 | 328 |

Le second témoin porte directement sur K5 et une feuille de taille 24. Les points sont distincts, entiers et
dans `[0,2^21)`. Le catalogue retourne `ok`; les budgets mémoire sont libérés. Les chiffres sont des comptes
discrets ; les durées locales dans les JSON bruts ne jugent aucun objectif de temps.

La preuve existante de `boxes.cpp:182` utilise le potentiel `Σ ceil(log2 largeur_i)`, au plus `3B`.
`internal.hpp:15` fixe d'ailleurs `kMaxDepth=3*kCoordBits`. Avec les mêmes règles, une profondeur maximale de
63 / 72 / 96 est une borne sûre pour les profils 21 / 24 / 32 ; un tableau de niveaux inclut en plus la racine.
Cela ne borne pas le nombre total de nœuds, qui exige son propre budget. Le repère fermé de 33 bits ne change pas
cette preuve : la largeur demi-ouverte reste au plus `2^32`.

## Reproduction et portée

```
python check.py --native /chemin/mhgp11_catalogue_probe
python -O check.py --native /chemin/mhgp11_catalogue_probe
```

Les sorties structurées normal/`-O` sont identiques à `result.json`. Les requêtes complètes et réponses sont dans
`raw/`. Le binaire est celui du [contre-audit précédent](../../../../morsehgp3D_v11/receipts/audit_independant_v12_20261007/verification/summary.json),
sans recompilation ; son SHA-256 figure dans `result.json`. Les 644 fichiers du manifeste source de ce build ont
été revérifiés, sans changement. Le build est GCC 13.3, Release, u21, sans CUDA.
GCP non utilisé ; ni matrice, ni sanitizer, ni performance v12 nouvellement qualifiés.
