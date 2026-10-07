# Rapports G-c : agrégations et frontières des chronomètres

Lecture du tableau local **22:50 UTC** du prototype, main observé `0377684ec`. Le rapport annonce ng00 K5, trois fils, `taskset 5-7`, quatre tours de trois passes, machine partagée chargée. Les fragments, sources et hashes capturés sans écriture du prototype figurent dans `capture.json`. Aucun nouveau benchmark, build ou appel GCP.

## Les ratios ne sont pas les quotients des médianes

Pour chaque bras `b` et tour `r`, `ab_local.py` retient `m[b,r] = min_p(wall[b,r,p])`. Puis il affiche :

- « min » : minimum des quatre `m[b,r]` ;
- « médiane » : médiane des quatre `m[b,r]` ;
- « ratio med » : médiane des quatre rapports **appariés par tour** `m[b,r] / m[base,r]`.

L'ordre des bras est inversé un tour sur deux. **Toutes les passes, y compris la passe 0, participent au minimum.** Ces chiffres ne sont donc pas les médianes chaudes de la session G4 H.

| Bras du tableau | Minimum publié (ms) | Médiane des minima publiée (ms) | Ratio médian apparié annoncé | Quotient des médianes arrondies, recalculé |
| --- | ---: | ---: | ---: | ---: |
| base (`build_base`) | 1 119,4 | 1 653,8 | 1 | 1 |
| index + G-L5 (`build_new`) | 1 166,4 | 1 505,0 | 0,965 | 0,910025… |
| index + G-L7 (`build_p`) | 910,7 | 1 295,2 | 0,914 | 0,783166… |

Le fichier `runs/ab_3fils_gl5_gl7.txt` concorde avec le tableau du rapport. **Aucune incohérence arithmétique n'est démontrée** par la différence entre ces deux sortes de ratios. En revanche, ce fichier ne contient ni les douze minima individuels ni les trente-six passes annoncées, et le script ne les écrit pas : **0,965 et 0,914 ne sont pas recalculables indépendamment depuis ces seules preuves conservées**. Leurs valeurs restent celles de la sortie agrégée, sans nouvel intervalle d'incertitude ni qualification de gain G4.

`check.py` relit ces agrégats et recalcule seulement le quotient des médianes arrondies. Il vérifie aussi un exemple algébrique **sans unité et sans rapport avec ce banc** : `a=[1,100,101]`, `b=[1,2,100]` donnent médiane(a/b)=1,01 mais médiane(a)/médiane(b)=50. Aucun temps individuel manquant n'est inventé.

## Portée exacte des compteurs

Les CMakeCache pointent les trois bras ci-dessus vers `base`, `repo` et `repo_p`. Les sources capturées entourent toutes `resolve_tower` par `wall_ns`, avec nuage/index/catalogue préparés avant ce chronomètre. L'instrumentation détaillée `MHGP12_TOWER_PROFILE` est absente des fichiers de compilation capturés. La correspondance historique binaire/source au moment de chaque processus n'est toutefois pas réétablie par cette lecture des sources locales.

La colonne **« resolve min » change de périmètre** :

- base : `stage.cpp` SHA `2682f239…`, tables construites avant le départ du chronomètre `resolve_ns` ;
- G-L5 : `stage.cpp` SHA `ac184501…`, construction de l'index et jointure dans `resolve_orders`, donc comprises dans `resolve_ns` ;
- G-L7 : `stage.cpp` SHA `f0d45467…`, construction de l'index dans `resolve_orders`, comprise dans `resolve_ns`.

Comparer directement 1 039,3 / 1 126,2 / 873,0 ms comme des coûts de résolution seuls mélangerait donc les frontières. Le script choisit d'abord la passe au wall minimal de chaque processus ; son « resolve min » est ensuite le minimum des compteurs resolve associés à ces quatre passes, pas le minimum indépendant de toutes les passes.

**Cette attribution concerne les bras historiques du tableau 22:50.** Le nouveau `build_r2 → repo2`, dont les chronos ont été réorganisés, n'est pas l'un de ces trois bras et ne reçoit pas leurs anciennes frontières. Conserver les passes et minima individuels, ainsi que les pins exécutés, permettrait de vérifier les futurs ratios appariés sans relancer ni extrapoler ces mesures locales.

Rejeu : `python check.py` puis `python -O check.py` depuis ce dossier ; sorties identiques lors de la fermeture. `SHA256SUMS` contrôle les quatre fichiers. Ce reçu reste une lecture du rapport de développement local, pas une nouvelle mesure publiée de G4 ni un temps FULL intégré.
