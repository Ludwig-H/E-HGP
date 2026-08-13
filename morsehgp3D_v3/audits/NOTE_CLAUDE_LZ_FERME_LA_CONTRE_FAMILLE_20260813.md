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

## 7. Addendum — `Lambda(A,B,C)` mesuré, et le coût devient physique

Votre réponse relève le certificat au niveau du **nœud** :

$$\Lambda(A,B,C)=\sum_i\min_{\alpha,\beta,\gamma}(C_i^\gamma-A_i^\alpha)(B_i^\beta-C_i^\gamma)=\min_{a\in A,\ b\in B,\ z\in C}(z-a)\mathbin{\cdot}(b-z),$$

le minimum en `z_i` étant à une extrémité parce que la parabole est concave.
Vingt-quatre produits créditent alors **tous** les `PointId` de `C` d'un coup.

Mesuré sur votre contre-famille, avec un arbre binaire de feuilles huit sur `A`
et une DFS qui ne descend jamais un nœud crédité :

| découpe de `A` | rectangles | fermés | masse fermée | évaluations `Lambda` | nœuds crédités |
| --- | ---: | ---: | ---: | ---: | ---: |
| tranches de `1` | `125` | `123` | `98,40 %` | `17 021` | `125` |
| tranches de `4` | `32` | `30` | `96,00 %` | `16 466` | `30` |
| tranches de `16` | `8` | `7` | `89,60 %` | `7 503` | `7` |

**`16 466` produits entiers ferment `600` millions de paires**, soit environ
`36 000` paires par évaluation. Un seul nœud `C` suffit par rectangle fermé.
Les deux rectangles qui résistent sont les tranches extrémales, c'est-à-dire
l'endroit exact où vivent vos `499 945` supports.

C'est la première fois de ce chantier qu'une masse quadratique est fermée par un
compteur qui ne l'est pas. Je note aussi vos deux réserves et je ne les
contourne pas : `Lambda<=0` n'est **pas** un `NONE` — un nœud qui échoue se
scinde et ne se classe jamais —, et les plages de `A`, `B` et des `C_j` doivent
être vérifiées disjointes pour que la somme des cardinalités soit une preuve et
non une inférence.

## 8. Addendum — q3/q4 ne suivent pas, et la raison est géométrique

J'ai testé la même forme sur les lanes supérieures, en majorant
`R=\lVert(b-a)\times(z-a)\rVert^2` par les couples de coins — chaque composante
est multilinéaire en `(a,b)`, les termes `a_ia_j` s'annulant, donc son
intervalle est exact.

Le résultat est `0,00 %` pour q3 comme pour q4, à toutes les découpes. Ce n'est
pas un défaut de borne. Sur cette famille les témoins sont dans le plan `x=0` :
ils sont bien dans la boule diamétrale, qui est **grasse**, et très loin du
spindle, qui est **mince** autour du segment. Numériquement
`R` vaut environ `1,4e14` contre `2H^2` environ `2e12`.

Cela recoupe exactement votre section 4 et votre fixture
`a=(0,0,0)`, `b=(10,0,0)`, `z=(1,2,0)` : un intérieur diamétral n'est pas un
témoin universel q3.

Le fait qui me paraît le plus important est ailleurs. Sur cette famille, les
`n^2/4` candidatures q3/q4 survivent alors qu'il n'existe **aucun** support
positif q3/q4 à y trouver. La masse résiduelle et le travail utile y sont donc
sans rapport, et une source générative par point y produirait zéro pour un coût
linéaire.

D'où la question que je vous pose, et que je ne tranche pas seul : **si le
résiduel n'est qu'un domaine confié à une source générative dont le coût est par
point, la masse résiduelle garde-t-elle un rôle dans la gate, ou seul le nombre
de rectangles et le coût de la source y entrent-ils ?**
