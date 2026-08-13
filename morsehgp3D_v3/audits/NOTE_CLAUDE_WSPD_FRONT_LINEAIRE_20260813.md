# Note de Claude — le front factorisé est linéaire par théorème, pas par mesure

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=proposition_math_non_recue`,
`public_status=not_claimed`. GCP non utilisé.

Suite de
[`NOTE_CLAUDE_LZ_FERME_LA_CONTRE_FAMILLE_20260813.md`](NOTE_CLAUDE_LZ_FERME_LA_CONTRE_FAMILLE_20260813.md).

## 1. Le point de départ : pourquoi q3/q4 ne créditent jamais

Le certificat de nœud q3/q4 se met sous forme entièrement séparable. Avec
`e=z-a`, `t=b-z`, l'identité de Lagrange donne
`R=\lVert(b-a)\times(z-a)\rVert^2=E_2X_2-H^2` où `E_2=\lVert z-a\rVert^2` et
`X_2=\lVert b-z\rVert^2`. La condition « `cH^2>R` » devient donc
« `(c+1)H^2>E_2X_2` », soit `4H^2>E_2X_2` en q3 et `3H^2>E_2X_2` en q4.

Or `E_2` et `X_2` sont des sommes de carrés par coordonnée : leurs maxima sur
des boîtes produit sont séparables, quatre couples d'extrémités par coordonnée.
Le certificat de nœud est donc

$$\Lambda(A,B,C)>0\quad\text{et}\quad c'\,\Lambda(A,B,C)^2>E_2^{\max}X_2^{\max},$$

quarante-huit produits entiers, sans produit vectoriel ni énumération de coins.

Il ne se déclenche jamais, et j'ai la constante. Pour `A`, `B` de rayon `r`
séparés de `D` et un nœud témoin `C` de rayon `\rho` au milieu, en posant
`u=(\rho+r)/(D/2)`, la condition q3 s'écrit
`4\left(\frac{1-u}{1+u}\right)^4>1`, c'est-à-dire

$$\rho+r<0{,}0858\,D.$$

Le certificat de nœud q3/q4 exige donc que **les trois boîtes soient petites
devant la séparation**. C'est mot pour mot la définition d'une **paire bien
séparée**.

## 2. Ce que cela ouvre : la décomposition de Callahan–Kosaraju

Si la condition du certificat est la séparation, alors l'ordonnance naturelle du
front n'est pas un découpage dyadique quelconque mais la **WSPD** : tout nuage de
`\mathbb{R}^3` admet une famille de rectangles `A\times B` bien séparés qui
partitionne **exactement** les `\binom{n}{2}` paires, de cardinal `O(s^3n)` et
calculable en `O(n\log n)`.

Mesuré, arbre à découpe équitable, `s=2` :

| `n` | rectangles | par point | pente |
| ---: | ---: | ---: | :---: |
| `2 000` | `56 921` | `28,46` | |
| `8 000` | `299 260` | `37,41` | `1,197` |
| `32 000` | `1 396 568` | `43,64` | `1,111` |
| `128 000` | `6 173 359` | `48,23` | `1,072` |

La masse couverte vaut exactement `n(n-1)/2` à chaque `n` : c'est une
**partition**, chaque paire dans un rectangle et un seul. C'est précisément
l'identité canonique que vous exigez — un `RectId` dérivé de `ANodeKey` et
`BNodeKey`, jamais du chemin de split — et elle est ici vérifiée par une
identité de cardinal, pas inférée.

La pente décroît de façon monotone vers `1`. Le régime transitoire vient de la
profondeur de l'arbre ; la borne `O(n)` est un théorème, pas une extrapolation.

## 3. Le retournement, chiffré

Sur **votre** contre-famille à deux plans, celle qui laisse `n^2/4` paires
sémantiques :

| `n` | rectangles WSPD | par point | masse couverte |
| ---: | ---: | ---: | ---: |
| `12 500` | `111 027` | `8,88` | `78 118 750` |
| `50 000` | `476 743` | `9,53` | `1 249 975 000` |

**`9,53` rectangles par point contre `48,23` pour l'uniforme.** La famille
adverse est cinq fois moins chère que le cas moyen, et sa pente est plate. Votre
point 2 — « le pire cas de la masse sémantique est le meilleur cas de la
représentation compacte » — cesse d'être un argument pour devenir un nombre.

## 4. Ce que j'en conclus, et ce que je n'en conclus pas

Ce qui est acquis : **`front_records = O(n)` par théorème**, indépendamment de
la masse sémantique. Aucune contre-famille ne peut violer la règle des deux
pentes sur ce compteur, parce que ce n'est pas une mesure. La masse quadratique
subsiste — Chazelle et al. construisent `n^2` arêtes de Gabriel croisées dans
`\mathbb{R}^3`, et l'ordre `k` de Voronoï y est en `O(k^2n^2)` — mais elle est
désormais portée par `O(n)` enregistrements.

Ce qui ne l'est pas, et que je ne masque pas :

1. **q3/q4 restent à `0,01 %`** même sur les rectangles bien séparés, à `s=2`.
   La condition `\rho+r<0{,}0858\,D` demande `s\gtrsim 12`, donc environ
   `s^3=1728` fois plus de rectangles. Le compromis n'est pas tranché.
2. Ce n'est pas seulement un défaut de borne. La lentille q3 — l'ensemble des
   `z` voyant `[a,b]` sous plus de `120^\circ` — a un volume d'environ `0,3`
   fois celui de la boule diamétrale. Il faut donc environ trois fois plus de
   points dans la boule pour trouver neuf témoins q3 que dix témoins q2. q3/q4
   sont intrinsèquement plus chers, et aucune écriture ne l'effacera.
3. **Mon coût de parcours est faux, et je le dis avant qu'on me le trouve** :
   ma DFS repart de la racine pour chaque rectangle, d'où `46` millions
   d'évaluations à `n=1 000`. Il faut une descente jointe qui suive les trois
   nœuds ensemble. C'est un défaut d'implémentation de la mesure, pas de la
   structure — mais tant qu'il est là, je ne publie aucune pente d'évaluations.

## 5. La question que cela pose à la place de la précédente

Si le front est `O(n)` par construction, la gate ne porte plus sur la masse
mais sur deux choses : le **nombre de rectangles résiduels** et le **coût de la
source générative par rectangle**. Confirmez-vous que c'est bien là que doit se
placer la règle des deux pentes, et que la masse sémantique en sort
définitivement ?

Et sur q3/q4 : préférez-vous une WSPD à `s` élevé qui crédite les trois lanes
d'un coup, ou une WSPD à `s=2` où q2 se ferme par nœud et où q3/q4 sont
délégués à la source générative sur un domaine déjà `O(n)` fois plus petit ?

## 6. Non-claims

Aucune pente physique, aucun octet, aucun high-water. Le programme est hors
dépôt, flottant pour le seul test de séparation, sans porte ni mutant ; il n'a
aucune autorité. Les quatre familles contractuelles ne sont pas mesurées ici.
Le contrat `50 000` reste entièrement ouvert et G4 reste NO-GO.
