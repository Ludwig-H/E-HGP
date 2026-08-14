# Questions de Claude : la séparation d'ordre quatre et le signe de l'orientation

Date : 14 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

> [!CAUTION]
> Ce fichier conserve les questions et mesures locales de Claude au commit
> `069d903`; ce n'est pas une autorité. Les réponses Q10--Q13 et les
> contre-fixtures sont dans
> [`AUDIT_CONTRE_RECEPTION_SUPPORT_COMPLET_CORNER8_WST34_22D1CB0_20260814.md`](AUDIT_CONTRE_RECEPTION_SUPPORT_COMPLET_CORNER8_WST34_22D1CB0_20260814.md).
> La séparation améliore parfois une décision uniforme, mais n'est ni une
> condition nécessaire d'un support q4, ni une permission de supprimer un
> couple non séparé.

Ces questions sont mathématiques, pas d'ingénierie. Elles viennent d'une mesure
qui bute sur un plancher que je ne sais pas expliquer.

## Le contexte en trois lignes

`Corner8BallDepth` ferme un bloc de supports q4 en huit tests de coin, mais il
exige d'abord que le **signe de l'orientation** `O = det3(b-a, x-a, y-a)` soit
constant sur tout le bloc `A×B×C×D`. Sans ce signe, la convexité de `sigma*J` en
`z` n'a pas de direction et le verdict reste `MIXED` sans qu'aucun coin ne soit
testé.

## La mesure

Sur `uniform, n=500`, l'option `echelle=256/1` du code impose le seuil effectif
`diag^2<=rayon^2/256` — l'inverse de son contrat textuel — et soumet les vingt
mille premiers blocs :

| `s` | `(C,D)` non séparés / soumis | orientation indécise / séparés | fermetures / orientés |
|---:|---:|---:|---:|
| 2 | 21,0 % | 80,6 % | 30,9 % |
| 8 | 28,9 % | 50,8 % | 54,7 % |
| 16 | 28,1 % | 54,9 % | 61,1 % |

Deux constats.

**La séparation est corrélée à la décidabilité sur ce préfixe.** En passant de
`s=2` à `s=8`, l'indécision d'orientation baisse et la part de fermetures parmi
les blocs orientés monte. Les trois colonnes n'ont cependant pas le même
dénominateur et les vingt mille premiers blocs changent avec `s`; cette table
n'est donc pas une ablation causale. Une WSPD ne sépare que `(A,B)`. Séparer
`(C,D)` peut aider Corner8, mais un échec doit raffiner `Sym2(C)` ou rester
`PENDING`, jamais supprimer une complétion q4.

**Mais un plancher résiste.** Autour de `50 %`, augmenter `s` ne fait plus
rien : `s=16` est même légèrement pire que `s=8`. Le coût, lui, continue de
monter — les rectangles passent de `10 177` à `97 951` entre `s=2` et `s=16`.

## Ce que je crois comprendre, et où je bute

Quatre boîtes peuvent être **deux à deux parfaitement séparées** et pourtant
porter des tétraèdres presque **coplanaires**. Le déterminant est alors petit
devant l'incertitude de position, et aucune séparation par paires ne le corrige.
La séparation contrôle les distances ; elle ne contrôle pas l'aplatissement.

J'ai essayé de gagner par l'arithmétique plutôt que par la géométrie, en
remplaçant l'évaluation par intervalles de `det3` par une forme centrée. Sa borne
contient explicitement un terme `e1` linéaire dans les rayons, puis des termes
quadratique et cubique; elle n'a donc pas l'ordre annoncé initialement. Le rejeu
local donne `82,6 %` d'indécision au lieu de `77,3 %`, sans gain reçu.

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

La prémisse initiale « presque coplanaire implique grand circumrayon » est
fausse, même pour un q4 positif. Les quatre points
`(3000,2000,2001),(2000,3000,1999),(1000,2000,2001),(2000,1000,1999)` ont le
centre `(2000,2000,2000)`, les poids `1/4`, `R^2=1000001` et une orientation
normalisée qui tend vers zéro lorsqu'on augmente l'échelle horizontale à
hauteur fixée. Une petite orientation et un rayon, même grand, ne garantissent
de toute façon aucun des huit `PointId` intérieurs d'un nuage arbitraire.

La question corrigée est : peut-on fermer séparément les supports `O>0` avec
huit témoins universels `J<0` et les supports `O<0` avec huit témoins universels
`J>0`, sans fixer un signe commun au bloc ? Ce certificat bisigne est
potentiellement sûr, mais exige deux ledgers authentifiés et un oracle de
jonction; il ne découle pas du seul rayon.

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
