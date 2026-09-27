# Contre-audit mathématique après capture

Ce complément a été ajouté **après** la clôture de la campagne gaussienne
du 27 septembre 2026 : 96 exports natifs, 1 920 lignes, aucun échec.
Il ne figure pas dans les sources de la campagne et n'en est ni un gate
hérité ni un nouveau lecteur complet de provenance. Aucune source gelée,
entrée, capture, géométrie ou sélection n'a été modifiée. GCP non utilisé.

[audit_math.py](audit_math.py) lit la capture privée close et le validateur
gelé, puis crée seulement un rapport neuf distinct. Les rapports normal et
`-O` sont identiques octet pour octet ; leurs chemins et SHA-256 figurent
dans [summary.json](summary.json). Le rapport détaillé conserve les hashes
avant/après des 1 536 sorties inspectées et les suites de comptes.

## Résultat

- 1 536 arbres condensés validés ; 1 843 200 sorties individuelles de points
  conservées, sans retrait de points de l'entrée.
- 768 paires `expZ=1/2` : parents, enfants, masses et IDs/CSR de sorties
  identiques ; 948 562 dates rejouées exactement par `lambda ** 2`.
- 384 suites `m=10,20,50,100` : nombre de clusters condensés non croissant.
  Les 576 transitions distinctes conservent aussi une sous-famille des
  ensembles de naissance. Cela ne promet **aucune monotonie des labels
  ou de la sélection EOM**.
- 8 764 ensembles sélectionnés recoupés avec leurs labels ; 128 coupes sur
  32 arbres vérifient partition complète et raffinement lorsque lambda
  augmente. Les points inactifs sont des singletons distincts, pas un
  cluster commun de bruit. Ce contrôle de coupes est échantillonné.
- Les 1 536 arbres respectent la borne de taille ci-dessous ; 4 368
  diagnostics de classes sous le seuil respectent exactement la borne F1.

En binary64, le code gelé calcule `(1/r) ** exp_z`. Pour 879 dates,
`lambda * lambda` diffère de `lambda ** 2` d'une ULP au maximum ; ce n'est
pas une modification topologique. Le rejeu conserve l'opération originale.

## Bornes et objets à ne pas confondre

Pour une vraie classe de taille `s < m`, tout cluster admissible de taille
`t >= m` satisfait `F1 = 2|A ∩ B|/(s+t) <= 2s/(s+m)`.
Ainsi les classes de 75 points à `m=100` sont bornées par `6/7`, et celles
de 60 points par `3/4`. Leur récupération exacte est impossible dans ces
sorties à seuil, même si l'arbre brut les représente parfaitement.
Ces bornes ont été vérifiées par fractions exactes sur les diagnostics
de labels appariés, d'arbre brut filtré par taille et d'arbre condensé.

Avec `C` clusters condensés, racine comprise, et `L` feuilles-clusters,
aucun nœud n'a un seul enfant. Si `C > 1`, les ensembles de naissance des
feuilles sont disjoints et ont chacun au moins `m` points. Donc
`L <= floor(n/m)` et `C-1 >= 2(C-L)`, d'où
`C <= max(1, 2*floor(n/m)-1)`. Le cas `C=1` est la racine structurelle,
y compris quand `n<m`. Les `n` sorties de points sont toujours conservées :
le stockage est **O(n+C)**, pas O(n/m) seul.

L'arbre brut, les ensembles de naissance des clusters condensés, leurs
ensembles actifs à une coupe et les clusters finalement sélectionnés par
EOM sont quatre objets distincts. Le meilleur F1 par classe est un oracle
supervisé de représentabilité : ses choix peuvent se recouvrir et ne
constituent pas une coupe ni une sélection conjointe. L'exposant modifie
les durées/stabilités et peut donc modifier EOM sans changer la topologie.
Cet audit ne requalifie pas le producteur géométrique et ne démontre pas
une supériorité statistique générale des méthodes.

## Rejeu local

Depuis la racine du worktree qui a produit la capture, choisir deux chemins
de rapports encore inexistants :

```sh
python3 -B morsehgp3D_v9/audits/b_gaussian_point_clustering_20260927/post_audit/audit_math.py /workspaces/E-HGP/build/v9-gaussian-clustering-benchmark-20260927-r1 /tmp/gaussian-math-replay-normal.json
python3 -O -B morsehgp3D_v9/audits/b_gaussian_point_clustering_20260927/post_audit/audit_math.py /workspaces/E-HGP/build/v9-gaussian-clustering-benchmark-20260927-r1 /tmp/gaussian-math-replay-optimized.json
cmp /tmp/gaussian-math-replay-normal.json /tmp/gaussian-math-replay-optimized.json
```

Le rejeu dépend de la capture et du manifeste privés ainsi que des chemins
de sources fermés par le reçu ; ce dossier public n'est pas une archive
autonome des données.
