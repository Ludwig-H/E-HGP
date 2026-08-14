
---

## Addendum du même jour : la localité mesurée, et la question qui en découle

Les questions ci-dessus portaient sur l'orientation. Les mesures qui ont suivi
déplacent le sujet, et j'ajoute donc une question de plus.

### Ce qui est mesuré

Le filtre manquant est écrit : la stricte positivité des quatre poids du
circumcentre, par Cramer et sans jamais former ce centre — `c = N/(2 O)`, le
signe de `orient3d(face, c)` valant celui de `det3(e1, e2, N - 2 O q0)` fois
celui de `O`, le tout sous `2^109` donc dans `i128`. Quatre fixtures le
séparent, dont votre tétraèdre plat, qui est bien centré, et la fixture de shell
où `ab` est un diamètre, qui ne l'est pas.

Appliqué à l'énumération exhaustive, il ramène la sortie à `31,4` supports par
point sur les amas et `61,2` sur `uniform` à `n=120` — enfin cohérent avec
l'extrapolation du brute-force du dépôt.

Sur la localité de ces supports, `uniform`, `s=2` :

| `n` | rang max moyen | pire | part sous `k=64` |
|---:|---:|---:|---:|
| 60 | 34,6 | 57 | 100 % |
| 80 | 39,5 | 71 | 97,4 % |
| 120 | 43,5 | 99 | 88,8 % |
| 160 | 47,4 | 127 | 80,7 % |

Le rang **moyen** croît en `n^0,30` environ, et sa part de `n` décroît de `0,58`
à `0,30`. Le **pire** croît en `n^0,82`, sa part passant de `0,95` à `0,79`. À
ces tailles les effets de bord dominent complètement, et je ne prétends aucune
loi asymptotique.

### Q14 — la Delaunay d'ordre un est-elle un squelette autorisé ?

Le fait mesuré est qu'aucun préfixe kNN n'est exact, ce que votre fixture au rang
4380 affirmait déjà. Mais un tétraèdre de Delaunay a ses sommets reliés dans le
graphe de Delaunay, dont le nombre d'arêtes est linéaire en pratique même quand
le degré maximal ne l'est pas. Le bon voisinage ne serait donc pas métrique mais
combinatoire.

Votre réponse Q2 interdit de « construire d'abord la mosaïque de Delaunay
d'ordre supérieur pour en extraire ses arêtes ». La question est de savoir si
l'interdit couvre la Delaunay d'**ordre un** — qui n'est pas d'ordre supérieur,
dont la taille est linéaire en pratique et qui se calcule en `O(n log n)` — ou
seulement les ordres `k >= 2` et leurs cofaces.

Si l'ordre un est autorisé comme squelette de proximité, alors la route change :
on ne filtre plus une masse quartique, on part d'un graphe linéaire et on monte
en ordre par voisinage combinatoire, avec continuation exacte là où le voisinage
ne suffit pas. Si l'ordre un est lui aussi interdit, quelle structure de
proximité l'invariant autorise-t-il, sachant que ni le kNN ni la lentille ne
donnent une source exacte à coût acceptable ?
