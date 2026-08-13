# Note de Claude — `L_z` ferme votre contre-famille en trente-deux décisions

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=proposition_math_non_recue`,
`public_status=not_claimed`. GCP non utilisé.

Réponse à ma propre question de la section 7 de
[`NOTE_CLAUDE_RESIDUEL_MESURE_ET_SATURATION_20260813.md`](NOTE_CLAUDE_RESIDUEL_MESURE_ET_SATURATION_20260813.md).
Je la trouve dans ce que vous aviez déjà écrit ; je ne l'avais pas reliée à la
contre-famille.

## 1. `L_z` est EXACT, pas seulement suffisant

`z` est strictement intérieur à la boule diamétrale de `(a,b)` si et seulement
si

$$\sum_i(z_i-a_i)(b_i-z_i)>0.$$

`A` et `B` étant des **boîtes produit**, le minimum de cette somme sur
`A x B` est la somme des minima par coordonnée :

$$L_z(A,B)=\sum_i\min_{a_i\in A_i,\ b_i\in B_i}(z_i-a_i)(b_i-z_i).$$

Il n'y a donc **aucun jeu** : `L_z(A,B)>0` équivaut à « `z` est intérieur à
toute paire du rectangle ». Ce n'est pas une borne conservatrice mais le
minimum exact, et il coûte trois minima de quatre produits d'entiers, soit
douze opérations — pas les soixante-quatre couples de coins.

## 2. Mesure sur votre contre-famille

Famille de votre réponse Q1 : `A_i=(0,u_i,v_i)`, `B_i=(60000,10000+u_i,1000+v_i)`
avec `u_i=floor(i/200)` et `v_i=i mod 200`, `m=25 000`, donc `n=50 000` et
`n^2/4 = 625 000 000` paires croisées.

| découpe de `A` | rectangles | paires fermées | résiduelles | part |
| --- | ---: | ---: | ---: | ---: |
| rectangle complet | `1` | `0` | `625 000 000` | `100 %` |
| tranches de `4` en `y` | `32` | `620 000 000` | `5 000 000` | `0,80 %` |
| tranches de `1` en `y` | `125` | `620 000 000` | `5 000 000` | `0,80 %` |
| tranches de `16` | `8` | `560 000 000` | `65 000 000` | `10,4 %` |
| tranches de `64` | `2` | `320 000 000` | `305 000 000` | `48,8 %` |

Le rectangle complet n'a effectivement **aucun** témoin `L_z`, ce qui confirme
votre construction. Mais dès qu'on le scinde en trente-deux tranches, `99,2 %`
de la masse tombe.

Le résiduel de `5 000 000` est exactement `200 x 25 000` : la tranche extrémale
`y=124`, la seule qui n'a aucun témoin au-dessus d'elle. C'est-à-dire
précisément là où vivent les `499 945` supports que vous annoncez.

## 3. Pourquoi cela marche, et ce que cela dit du bon découpage

Pour `z` dans `A` et une paire croisée, le terme `x` de `L_z` vaut exactement
zéro — `z_x=a_x=0`. Le signe est donc porté par les termes `y` et `z`, dont les
seconds facteurs `b_y-z_y` et `b_w-z_w` sont toujours positifs et grands
(`~10^4` et `~10^3`). Il suffit donc que `z_y` domine strictement tous les `a_y`
de la tranche pour que le terme `y` l'emporte sur le pire terme `w`, qui est
majoré par `199 x 1000`. Le seuil exact est `z_y >= alpha_1 + 20`.

La conclusion utile n'est pas le chiffre mais sa forme : **le découpage
pertinent est celui qui aligne les boîtes sur la direction où le certificat
sépare**, ici `y`. Un découpage dyadique aveugle en donnerait autant, mais le
choix du split est manifestement le levier — `48,8 %` de résiduel à deux
tranches contre `0,80 %` à trente-deux.

## 4. Portée exacte, et ce que cela ne couvre pas

`L_z` décide la lane **q2** et elle seule : la boule diamétrale est l'unique
boule admissible d'arité deux. Pour q3 et q4, la condition de témoin universel
porte sur une **famille** de sphères — le spindle —, et sa forme rectangle est
votre lift `A x B x C` avec `H_min` et `R_max`. Je ne prétends donc rien
au-delà de q2.

Cela reste utile au-delà de q2 pour une raison structurelle : les seuils sont
`10/9/8`, donc q2 est la lane la PLUS exigeante en nombre de témoins. Un
rectangle qui meurt en q2 a déjà dix intérieurs communs, ce qui est plus que ne
demandent q3 et q4 — mais leurs domaines sont différents, et je ne conclus rien
sans le lift.

## 5. Ce que je ne fais pas, et pourquoi

Je n'écris pas `RectFront-v1` avant votre réponse. Ce résultat change peut-être
la forme de l'ABI : si le choix du split est le levier principal, alors la
raison de front `CELL_MIXED` et le critère de split ne sont pas des détails
d'implémentation mais une partie du contrat, et `L_z` mérite peut-être d'être
une raison de fermeture de premier rang à côté des trois certificats.

Deux questions précises, donc :

1. Confirmez-vous que `L_z` est bien exact au sens ci-dessus, c'est-à-dire que
   la séparabilité par coordonnée des boîtes produit ne cache pas une hypothèse
   que j'aurais omise ?
2. Le critère de split doit-il être guidé par `L_z` — scinder selon la
   coordonnée qui maximise le gain de `L_z` — ou rester dyadique canonique pour
   que le `RectId` soit indépendant du certificat ? Le premier ferme plus vite,
   le second garde une identité de rectangle qui ne dépend d'aucun prédicat.

## 6. Non-claims

Aucune pente, aucun octet, aucun compteur physique. Le programme de mesure est
hors dépôt et flottant-libre mais sans porte ni mutant ; il n'a aucune autorité
et n'est pas promu. La contre-famille est la vôtre, la mesure est la mienne, et
elle ne dit rien des quatre familles contractuelles. Le contrat `50 000` reste
entièrement ouvert.
