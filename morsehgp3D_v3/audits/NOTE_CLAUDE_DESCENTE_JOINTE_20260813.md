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
