# Questions de Claude : la séparation d'ordre quatre et le signe de l'orientation

Date : 14 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles`,
`profile=quantized_u16_input_only`,
`mode=question_mathematique`,
`public_status=not_claimed`.

Ces questions sont mathématiques, pas d'ingénierie. Elles viennent d'une mesure
qui bute sur un plancher que je ne sais pas expliquer.

## Le contexte en trois lignes

`Corner8BallDepth` ferme un bloc de supports q4 en huit tests de coin, mais il
exige d'abord que le **signe de l'orientation** `O = det3(b-a, x-a, y-a)` soit
constant sur tout le bloc `A×B×C×D`. Sans ce signe, la convexité de `sigma*J` en
`z` n'a pas de direction et le verdict reste `MIXED` sans qu'aucun coin ne soit
testé.

## La mesure

Sur `uniform, n=500`, cellules témoins très fines (`echelle=256/1`), vingt mille
blocs soumis :

| `s` | couples `(C,D)` non séparés | orientation indécise | fermetures |
|---:|---:|---:|---:|
| 2 | 21,0 % | 80,6 % | 30,9 % |
| 8 | 28,9 % | 50,8 % | 54,7 % |
| 16 | 28,1 % | 54,9 % | 61,1 % |

Deux constats.

**La séparation gouverne la décidabilité.** Louis avait raison de suspecter `s`.
En passant de `s=2` à `s=8`, l'indécision d'orientation tombe de `80,6 %` à
`50,8 %` et les fermetures montent de `30,9 %` à `54,7 %`. J'ai en outre dû
étendre la séparation au couple `(C,D)` lui-même : une WSPD ne sépare que
`(A,B)`, alors que l'orientation dépend des quatre sommets — et sur la diagonale
`C=D` elle est structurellement indécidable.

**Mais un plancher résiste.** Autour de `50 %`, augmenter `s` ne fait plus
rien : `s=16` est même légèrement pire que `s=8`. Le coût, lui, continue de
monter — les rectangles passent de `10 177` à `97 951` entre `s=2` et `s=16`.

## Ce que je crois comprendre, et où je bute

Quatre boîtes peuvent être **deux à deux parfaitement séparées** et pourtant
porter des tétraèdres presque **coplanaires**. Le déterminant est alors petit
devant l'incertitude de position, et aucune séparation par paires ne le corrige.
La séparation contrôle les distances ; elle ne contrôle pas l'aplatissement.

J'ai essayé de gagner par l'arithmétique plutôt que par la géométrie, en
remplaçant l'évaluation par intervalles de `det3` par une forme centrée dont
l'erreur est quadratique en le rayon. Sans effet mesurable : `82,6 %`
d'indécision au lieu de `77,3 %`, donc légèrement pire. Le problème n'est donc
pas la finesse de la borne, il est géométrique.

## Q10 — Existe-t-il une décomposition qui borne l'aplatissement ?

Y a-t-il une décomposition d'ordre quatre — analogue à Callahan--Kosaraju mais
sur les quadruplets — qui garantisse, pour chaque bloc, que **tous** ses
tétraèdres ont un signe d'orientation constant ? Autrement dit, une condition de
« bonne forme » de bloc, du type

```text
|det3| >= c * diam(A) * diam(B) * diam(C) * ... ?
```

et si oui, quel est le nombre de blocs qu'elle produit ? Je crains qu'elle ne
soit pas atteignable à nombre de blocs linéaire, puisque les quadruplets
coplanaires forment une variété de codimension un que toute décomposition doit
raffiner indéfiniment le long de sa trace.

## Q11 — Les tétraèdres plats peuvent-ils être traités *sans* leur orientation ?

Un tétraèdre presque coplanaire a un circumcentre très éloigné, donc une
circumsphère énorme, donc beaucoup d'intérieurs — il devrait sortir du contrat
de rang de lui-même. Existe-t-il un certificat qui exploite directement cela,
c'est-à-dire qui ferme un bloc **parce que** son orientation est petite, sans
jamais décider son signe ?

Une piste : borner par le bas le rayon de la circumsphère par
`R >= produit des longueurs d'arêtes / (6 * |det3|)` — ou une forme entière
équivalente — puis montrer qu'une boule de ce rayon contient nécessairement huit
points sous une hypothèse de densité locale mesurable. Cela remplacerait la
dichotomie « orientation décidée ou rien » par une trichotomie
« orientation décidée / orientation petite donc sphère grosse / MIXED ».

Est-ce que cette borne est exacte et calculable en entiers sous u16, et
peut-elle porter un verdict `ALL` sûr plutôt qu'une heuristique ?

## Q12 — Le bon `s` est-il différent par lane ?

`s` a deux rôles contradictoires : il rend les prédicats décidables et il
multiplie les rectangles. Les mesures suggèrent un optimum vers `s=8`, mais je
n'ai pas de raison de croire que ce soit le même pour la couverture q2, le
carrier q3 et l'orientation q4. Faut-il une décomposition unique à `s` élevé, ou
une décomposition coarse raffinée localement seulement là où un prédicat reste
indécis — et dans ce cas, le raffinement local casse-t-il l'exact-once ?

## Q13 — La diagonale

Sur la diagonale `C=D`, l'orientation est indécidable par construction, et ces
couples portent pourtant les paires internes à une cellule. Faut-il les router
vers un moteur différent — la sweep d'axe, ou le shallow edge-local que vous
proposiez — plutôt que de les scinder jusqu'à séparation ?

GCP non utilisé pour ces questions. Les mesures sont locales, mono-thread, sur
une machine partagée.
