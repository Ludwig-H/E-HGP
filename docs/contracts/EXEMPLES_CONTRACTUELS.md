# Cinq exemples contractuels de MorseHGP3D

Chaque exemple utilise des identifiants entiers, des coordonnées 3D entières et des niveaux exacts en rayon carré. Une coupe écrite « avant $a$ » signifie un seuil strictement inférieur à $a$ et supérieur au niveau distinct précédent. Une coupe « à $a$ » utilise le sous-niveau fermé.

Dans chaque table, les identifiants sont déjà réassignés selon l'ordre canonique lexicographique des triplets IEEE-754 exacts. Les indices source originaux, lorsqu'ils diffèrent, ne participent pas à l'identité scientifique.

Les arbres ci-dessous sont sémantiques : ils ne prescrivent ni représentant DSU, ni pivot d'étoile. Les listes d'enfants sont canoniquement triées par identifiant de nœud dans la sérialisation.

## E1 — ancre fondamentale à l'ordre un

Entrée : $K_{\max}=1$ et trois points collinéaires distincts.

| identifiant | coordonnées |
|---:|---|
| `0` | `(0, 0, 0)` |
| `1` | `(2, 0, 0)` |
| `2` | `(8, 0, 0)` |

Les deux arêtes de l'EMST sont `01` et `12`. Leurs longueurs carrées valent respectivement $4$ et $36$, donc leurs niveaux HGP valent $1$ et $9$ après division par quatre. La paire `02` a une miniball de niveau $16$ contenant `1` strictement; elle ne fournit pas une selle de rang deux utile lorsque $s_{\max}=2$.

```text
M012 @ 9
├── M01 @ 1
│   ├── B0 @ 0
│   └── B1 @ 0
└── B2 @ 0
```

Sorties attendues pour `hgp_reduced` et `full_pi0` :

| coupe de $T_1$ | composantes |
|---|---|
| $0\leq a<1$ | `0`, `1`, `2` |
| $1\leq a<9$ | `01`, `2` |
| $9\leq a$ | `012` |

Le catalogue utile contient trois minima de rang un au niveau zéro et deux événements de rang deux aux niveaux $1$ et $9$. Il n'existe aucune application verticale puisque $K_{\mathrm{eff}}=1$.

## E2 — naissance isolée à l'ordre deux

Entrée : $K_{\max}=2$ et deux points.

| identifiant | coordonnées |
|---:|---|
| `0` | `(-1, 0, 0)` |
| `1` | `(1, 0, 0)` |

La sphère de diamètre `01` a pour centre `(0, 0, 0)` et niveau exact $1$. Son rang fermé vaut deux. Le même événement est une selle à l'ordre un et un minimum à l'ordre deux.

```text
ordre 1                    ordre 2, full_pi0
M01 @ 1                    B01 @ 1
├── B0 @ 0                    │
└── B1 @ 0                    └── v₂→₁ à 1 vers M01
```

Sorties attendues :

- `full_pi0` contient la naissance isolée `B01` dans $T_2$; la coupe est vide avant $1$, puis contient l'unique composante `01` au niveau fermé $1$;
- `hgp_reduced` laisse $T_2$ vide, car `01` reste l'unique facette de la composante terminale et n'est jamais non triviale;
- dans `full_pi0`, l'application verticale au niveau fermé $1$ envoie `B01` vers la composante post-lot `M01` de $T_1$.

Ce cas interdit de confondre l'absence correcte de nœud réduit avec l'absence d'une composante de $\pi_0(L_2)$.

## E3 — fusion binaire avec une facette simultanée

Entrée : $K_{\max}=2$ et trois points collinéaires.

| identifiant | coordonnées |
|---:|---|
| `0` | `(-2, 0, 0)` |
| `1` | `(0, 0, 0)` |
| `2` | `(2, 0, 0)` |

Les facettes `01` et `12` naissent au niveau $1$. La miniball de `02` est centrée sur `1`, a le niveau $4$, contient `1` strictement et définit l'événement

$$I=\lbrace1\rbrace,\qquad U=\lbrace0,2\rbrace,\qquad S=\lbrace0,1,2\rbrace.$$

À l'ordre deux, ses deux bras stricts sont `12` et `01`. Ils appartiennent à deux composantes globales distinctes juste avant $4$. La facette `02` naît au niveau $4$ et est activée dans le lot, mais n'est pas un bras strict et ne devient pas une branche de persistance positive.

```text
full_pi0, ordre 2

M012 @ 4
├── B01 @ 1
└── B12 @ 1

delta de couverture au niveau 4 : facette 02, aucun nouvel identifiant de point
```

Le shell a deux points, donc la multiplicité locale d'indice un vaut $\Delta_1=1$. Le nombre de racines strictement antérieures touchées vaut $q=2$ et une classe de $H_0$ est tuée. Les coupes attendues sont :

| coupe de $T_2$ | composantes `full_pi0` |
|---|---|
| $1\leq a<4$ | `01`, `12` |
| $4\leq a$ | une composante de facettes `01`, `02`, `12`, couvrant `012` |

Dans `hgp_reduced`, les deux facettes antérieures sont omises. Le même lot a donc $q=0$ au sens réduit et crée une naissance `R012` au niveau $4$, sans prétendre qu'il s'agit d'une fusion de deux racines réduites.

À l'ordre un, les événements `01` et `12` ont tous deux le niveau $1$ et sont contractés dans un lot unique : les trois singletons ont un parent multifurqué commun au niveau $1$.

## E4 — multifusion portée par un seul centre

Entrée : $K_{\max}=2$ et un triangle aigu scalène.

| identifiant | coordonnées |
|---:|---|
| `0` | `(0, 0, 0)` |
| `1` | `(1, 3, 0)` |
| `2` | `(4, 0, 0)` |

Les niveaux de naissance des trois facettes sont

$$\beta(01)=\frac{5}{2},\qquad\beta(02)=4,\qquad\beta(12)=\frac{9}{2}.$$

Le cercle circonscrit est centré en `(2, 1, 0)` et son niveau vaut $5$. Ses coordonnées barycentriques relativement à `0`, `1`, `2` valent respectivement $\frac{1}{4}$, $\frac{1}{3}$ et $\frac{5}{12}$; elles sont toutes strictement positives. L'événement a $I=\varnothing$, $U=S=\lbrace0,1,2\rbrace$ et trois bras stricts.

```text
full_pi0, ordre 2

M012 @ 5
├── B01 @ 5/2
├── B02 @ 4
└── B12 @ 9/2
```

Juste avant $5$, les trois facettes forment trois composantes distinctes. Au niveau fermé $5$, elles forment une seule composante couvrant `012`. Ainsi $q=3$, deux classes de $H_0$ sont tuées et $\Delta_1=\lvert U\rvert-1=2$. Le nœud `M012` est une multifusion ternaire unique; aucune sérialisation canonique ne peut le remplacer par deux fusions binaires ordonnées.

Dans `hgp_reduced`, aucune des trois facettes isolées n'est une racine antérieure. Le lot crée donc une unique naissance réduite `R012` au niveau $5$.

## E5 — deux composantes d'ordre deux qui se recouvrent

Entrée : $K_{\max}=2$ et cinq points coplanaires.

| identifiant | coordonnées |
|---:|---|
| `0` | `(-2, -1, 0)` |
| `1` | `(-2, 1, 0)` |
| `2` | `(0, 0, 0)` |
| `3` | `(3, 2, 0)` |
| `4` | `(4, -1, 0)` |

Les triangles `012` et `234` sont aigus. Leurs centres sont `(-5/4, 0, 0)` et `(47/22, 1/22, 0)`, et leurs niveaux exacts valent respectivement $25/16$ et $1105/242$. Tous les couples croisés gauche–droite ont un niveau au moins égal à $13/2$, donc aucune coface croisée n'est active au niveau $1105/242$.

Après fermeture du second événement, les deux composantes d'hypergraphe sont indépendantes :

```text
composante gauche             composante droite
facettes 01, 02, 12           facettes 23, 24, 34
union de points 012           union de points 234
             \                 /
              recouvrement : 2
```

Sortie réduite attendue à l'ordre deux :

- avant $25/16$, aucune composante n'est représentée;
- au niveau fermé $25/16$, la naissance réduite gauche apparaît;
- au niveau fermé $1105/242$, la naissance réduite droite apparaît et les deux racines restent distinctes;
- leurs unions de points sont `012` et `234`;
- leur intersection est exactement `2` et ne provoque aucune union DSU, car la connectivité porte sur les facettes de cardinal deux, pas sur l'intersection des unions de points.

Ce recouvrement persiste pour $1105/242\leq a<13/2$. Au niveau $13/2$, l'événement de rang trois $I=\lbrace2\rbrace$, $U=\lbrace1,3\rbrace$ relie les bras `12` et `23`; il crée alors la fusion réelle des deux racines et active la facette simultanée `13`. L'événement ultérieur $I=\lbrace2\rbrace$, $U=\lbrace0,4\rbrace$ au niveau $9$ est globalement redondant, mais son delta de couverture active encore `04`.

Dans `full_pi0`, les facettes `01`, `02` et `12` naissent aux niveaux $1$, $5/4$ et $5/4$. Les facettes `34`, `23` et `24` naissent aux niveaux $5/2$, $13/4$ et $17/4$. À leurs niveaux circonscrits respectifs, les événements `012` puis `234` contractent chacun trois composantes de facettes. Aucune coface croisée ne les relie avant $13/2$.

À l'ordre un, tous les points appartiennent déjà à la même composante au niveau $13/4$. Les deux composantes réduites d'ordre deux ont donc la même cible verticale au niveau $1105/242$. Une application verticale est une fonction, mais elle n'est pas nécessairement injective.

## Tableau récapitulatif

| fixture | propriété discriminante | profil sérialisé | sémantique et statut | ordre ciblé |
|---|---|---|---|---:|
| `k1-emst.json` | facteur EMST $1/4$ et feuilles singleton | `hgp_reduced` | `exact` | 1 |
| `isolated-birth-k2.json` | facette terminale présente seulement dans le profil complet | `full_pi0` | `partial_refinement`, `conditional` | 2 |
| `binary-merge-k2.json` | deux bras stricts, facette simultanée et fusion binaire | `full_pi0` | `partial_refinement`, `conditional` | 2 |
| `multifusion-k2.json` | trois bras et multifusion ternaire en un centre | `full_pi0` | `partial_refinement`, `conditional` | 2 |
| `overlap-k2.json` | couverture recouvrante, jamais partition arbitraire | `hgp_reduced` | `exact` | 2 |
