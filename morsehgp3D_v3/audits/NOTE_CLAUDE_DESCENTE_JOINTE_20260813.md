# Note de Claude — la descente jointe, un faux `NONE` que j'avais écrit, et le coût réel

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=proposition_math_non_recue`,
`public_status=not_claimed`. GCP non utilisé.

Suite de
[`NOTE_CLAUDE_WSPD_FRONT_LINEAIRE_20260813.md`](NOTE_CLAUDE_WSPD_FRONT_LINEAIRE_20260813.md).
J'y annonçais la descente jointe comme seul obstacle restant à une mesure de
coût honnête. La voici, avec ce qu'elle m'a appris contre moi.

## 1. Un seul `Lambda` rend vos trois verdicts

La quantité `\sum_i(z_i-a_i)(b_i-z_i)` admet sur des boîtes produit un
**intervalle exact**, et ses deux bornes portent les deux classifications :

- `Lambda_{\min}(A,B,C)>0` : tout point de `C` est témoin de toute paire de
  `A\times B`. C'est `ALL` — on crédite `\lvert C\rvert` et on retire le nœud.
- `Lambda_{\max}(A,B,C)\le 0` : aucun point de `C` n'est témoin d'aucune paire.
  C'est `NONE` — on retire le nœud **définitivement**, sans le raffiner.
- sinon `MIXED`.

`ALL` n'a besoin d'aucun test d'exclusion de `a` et de `b` : un point
strictement intérieur à sa propre boule diamétrale n'existe pas, donc
`Lambda_{\min}>0` implique déjà `z\neq a` et `z\neq b`. J'avais d'abord écrit un
test d'index qui jetait tout nœud chevauchant `A` ou `B` ; à la racine
`A\cup B` est le nuage entier, et **aucun témoin ne survivait**. C'était une
garde inutile et destructrice.

## 2. Le faux `NONE` que j'avais écrit, et pourquoi il était faux

J'ai d'abord calculé `Lambda_{\max}` comme `Lambda_{\min}`, par les seules
extrémités des boîtes. C'est **faux**, et l'asymétrie est instructive :

- en `z_i`, la parabole `(z-a)(b-z)` est **concave**. Son **minimum** sur un
  intervalle est donc à une extrémité — c'est ce qui rend `Lambda_{\min}` exact
  et bon marché. Son **maximum** est au sommet `z=(a+b)/2`, à l'intérieur.
- en `(a,b)`, `\min_z` est un minimum de fonctions bilinéaires donc concave, et
  `\max_z` un maximum de fonctions bilinéaires donc convexe : les deux sont bien
  atteints à un coin.

La forme exacte du maximum est donc : pour chacun des quatre coins `(a,b)`,
évaluer au sommet `(a+b)/2` **écrêté** à la boîte de `C`, et aux deux
extrémités. En n'évaluant que les extrémités, je sous-estimais le maximum et je
prononçais `NONE` là où des témoins existaient — une **fausse classification**,
pas une simple perte.

Vérifié : `20 000` triplets de boîtes aléatoires, `Lambda_{\min}` et
`Lambda_{\max}` comparés aux minimum et maximum exhaustifs sur les points
entiers des trois boîtes. **Zéro désaccord.**

## 3. Le budget plat ne marche pas, le meilleur d'abord oui

Traiter la liste de témoins dans l'ordre de l'arbre épuise le budget sur les
nœuds `MIXED` de la **frontière** de la boule sans jamais atteindre le centre :
à budget `32`, la fermeture est nulle.

Ordonner la file par `Lambda_{\max}` décroissant — du plus intérieur vers la
frontière — change le régime. À budget `24` évaluations par rectangle :

| `n` | rectangles fermés | résiduels | masse fermée q2 |
| ---: | ---: | ---: | ---: |
| `2 000` | `11 078` | `28 280` | `53,72 %` |
| `8 000` | `99 145` | `183 860` | `81,31 %` |

**La fermeture croît avec `n`** à budget constant, ce qui est la propriété que
je cherchais depuis le début.

## 4. Ce qui n'est pas acquis, et que je publie tel quel

Le nombre de rectangles **visités** croît en `n^{1,42}` sur cette plage :
`19,7` par point à `2 000`, `35,4` à `8 000`. Il ne s'agit pas du cardinal du
front — la partition WSPD est `O(n)` par théorème — mais de tous les rectangles
intermédiaires que ma récursion traverse avant d'atteindre les résiduels
feuille. Tant que ce compteur n'est pas linéaire, **je ne publie aucune pente de
travail**, et la règle des deux pentes n'est pas satisfaite par cette
implémentation.

q3/q4 restent proches de zéro, pour la raison géométrique déjà donnée : la
lentille à `120^\circ` vaut environ `0,3` fois le volume de la boule diamétrale,
donc neuf témoins q3 coûtent environ trois fois plus de points que dix témoins
q2.

## 5. Ce que je vous demande

Le budget par rectangle est bien votre raison de front `RESOURCE_CAP`, et il
rend le coût borné par construction. Mais un rectangle capé sort **résiduel**
sans être scindé, donc le budget échange de la couverture contre une borne.

Confirmez-vous que c'est le bon échange — un `RESOURCE_CAP` terminal qui délègue
à la source générative — ou exigez-vous qu'un rectangle capé soit **scindé** et
réessayé, ce qui rétablit la couverture mais rend le coût à nouveau non borné ?

## 6. Non-claims

Aucune pente physique publiée, aucun octet, aucun high-water. Les programmes
sont hors dépôt, sans porte ni mutant, et n'ont aucune autorité. Seules
l'uniforme et la contre-famille sont mesurées. Le contrat `50 000` reste
entièrement ouvert et G4 reste NO-GO.

## 7. Addendum — le front doit s'arrêter à « bien séparé », pas à « fermé »

Une identité de mes propres compteurs corrige la note ci-dessus. À `n=8 000` :
`564 438` rectangles visités contre `2 x 282 925` terminaux. La récursion étant
binaire, **le compteur de visites n'est rien d'autre que le cardinal du front**,
au facteur deux près. Il n'y a donc pas deux quantités à surveiller mais une.

Et ce front-là n'est **pas** la WSPD. Ma récursion scinde tant que le certificat
ne ferme pas, donc elle continue bien au-delà du seuil de bonne séparation et
descend jusqu'aux feuilles sur le résiduel. D'où le `n^{1,40}` : ce n'est pas la
borne de Callahan–Kosaraju qui est fausse, c'est mon critère d'arrêt qui n'est
pas le sien.

La correction est structurelle et tient en une ligne : un rectangle **bien
séparé** qui n'a pas fermé sort **terminal** et part à la source générative, au
lieu d'être scindé. Alors :

- `front_records = O(s^3 n)` par théorème, quelle que soit la famille ;
- `evaluations <= budget x front_records = O(n)` par construction.

Les deux pentes de la règle deviennent des théorèmes et non des mesures. Ce que
la mesure décide n'est plus la pente mais la **fraction fermée** à `s` et budget
donnés, et le coût de la source sur le résiduel.

C'est aussi la réponse à ma question de la section 5, et elle est meilleure que
les deux branches que je vous proposais : un rectangle capé n'est ni terminal ni
scindé indéfiniment — il est scindé **jusqu'à la bonne séparation**, puis
terminal. Le budget borne le travail par rectangle, la séparation borne le
nombre de rectangles.

Je le mesure, et je vous transmettrai la fraction fermée en fonction de `s`.

## 8. Addendum — la file porte aussi un MAJORANT, donc un certificat POSITIF

La file de priorité ne sert pas qu'à trouver des témoins. À tout instant

$$\text{cred}+\sum_{C\in\text{file}}\lvert C\rvert+\text{bloques}$$

**majore** ce que le rectangle pourra jamais créditer, puisque tout point non
encore classé est soit dans un nœud de la file, soit déjà `NONE`. Il y a donc
trois issues et non deux :

- `cred >= h` : **FERMÉ** — aucune paire du rectangle n'est un support ;
- `cred + pend < h` : **POSITIF** — **toute** paire du rectangle est un support,
  produite en bloc sans qu'aucun témoin ne soit énuméré ;
- sinon : **RÉSIDUEL**.

Le deuxième cas est la source générative que vous demandiez, et il sort du même
`Lambda` que les deux autres. C'est aussi la réponse à mon propre constat de la
section 4 : sur votre contre-famille, `n^2/4` candidatures survivent sans qu'il
y ait un seul support q3/q4 à trouver — un certificat qui ne sait que fermer ne
peut rien y faire, un certificat qui sait aussi **conclure positivement** décide
le rectangle entier.

Une correction m'a été nécessaire pour qu'il se déclenche : la classification
doit se faire **à l'insertion** et non au dépilage. La file étant ordonnée du
plus intérieur au plus extérieur, les nœuds `NONE` en sont dépilés en dernier,
et le majorant ne baisse jamais avant que le budget ne soit épuisé.

Mesuré, `uniform`, `n=1 500`, budget `32`, feuilles unitaires :

| feuille | fermé | positif | résiduel | rectangles positifs |
| ---: | ---: | ---: | ---: | ---: |
| `1` | `85,46 %` | `0,64 %` | `13,90 %` | `5 238` |
| `2` | `81,01 %` | `0,29 %` | `18,70 %` | `1 384` |
| `4` | `71,01 %` | `0,02 %` | `28,97 %` | `62` |

`7 154` paires sont **certifiées supports** sans qu'un seul témoin ait été
énuméré, et l'invariant de partition tient exactement :
`960 740 + 7 154 + 156 356 = \binom{1500}{2}`.

Le certificat positif exige des feuilles fines, et la raison est nette : un
nœud `MIXED` bloqué contribue `\lvert C\rvert` au majorant même si un seul de
ses points est réellement témoin. À feuille `8` et seuil `10`, deux feuilles
bloquées suffisent à le tuer. C'est un compromis mesurable, pas un obstacle.
