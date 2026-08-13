# Questions de Claude — `s` dépend-il de `K`, et la banque est-elle le vrai goulet

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=proposition_math_non_recue`,
`public_status=not_claimed`. GCP non utilisé pour cette note.

## 1. La question qu'on m'a posée, et que je n'avais pas mesurée

*Le `s` de la décomposition doit-il dépendre du seuil `K` ?*

Théoriquement, oui, et par une **inflation multiplicative**. Pour une paire, la
région des témoins q2 est la boule diamétrale de rayon `D/2`. Pour un
**rectangle**, la région commune à toutes ses paires est un cœur de rayon
`\rho \le (d-2S)/2` avec `S=r_A+r_B`. En posant `u = S/d \le 2/(s+2)` :

$$\lambda(s)=\left(\frac{1+u}{1-2u}\right)^{3}.$$

Le certificat ne ferme donc une paire que si elle possède `K\lambda(s)` témoins,
et non `K`. Les paires dont le compte tombe entre `K` et `K\lambda(s)` sont
**faussement résiduelles**.

| `s` | `2` | `4` | `6` | `8` | `18` | `38` |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `\lambda(s)` | `\infty` | `64` | `15,6` | `8,0` | `2,6` | `1,59` |

La dépendance en `K` est **multiplicative**, donc pour une inflation *relative*
donnée, `s` n'en dépend pas. Mais pour une marge **absolue** de `j` témoins,
`\lambda = 1 + j/K` : à `j=2`, `K=8` demande `s \approx 54` et `K=10` demande
`s \approx 65`. **Un seuil plus grand exige une séparation plus grande**, et le
coût est en `s^3`.

## 2. Mais la mesure dit que ce n'est PAS le facteur limitant

J'ai compté les **vrais** témoins universels q2 des paires appartenant aux
rectangles que la banque laisse **ouverts** — balayage exhaustif du nuage,
`uniform`, `n=8 000`, `W=64`, `L=32` :

| `s` | front/pt | q2 fermé (records) | témoins moyens des ouverts | **faussement résiduels** |
| ---: | ---: | ---: | ---: | ---: |
| `2` | `51,1` | `4,5 %` | `89,8` | **`69,5 %`** |
| `4` | `140,5` | `33,3 %` | `89,8` | **`76,0 %`** |
| `8` | `433,6` | `66,7 %` | `102,7` | **`78,8 %`** |

Deux faits que je ne sais pas concilier avec la théorie ci-dessus :

1. **Les rectangles ouverts contiennent en moyenne `90` à `103` témoins**, soit
   **neuf à dix fois le seuil de dix**. Ce ne sont pas des paires marginales :
   le certificat les rate largement.
2. **Le taux de faux résiduels EMPIRE avec `s`** — `69,5 %` puis `76,0 %` puis
   `78,8 %` — alors que `\lambda(s)` prédit l'inverse.

La seule explication que je voie est que le goulet n'est pas la séparation mais
la **banque** : elle ne regarde que `L=32` candidats pris dans une fenêtre
Morton de `W=64` autour du centre. Si moins de dix d'entre eux sont des témoins
universels, le rectangle reste ouvert — quels que soient les quatre-vingt-dix
autres témoins qui existent ailleurs. C'est exactement votre avertissement :
« la discontinuité de Morton peut faire manquer tous les bons témoins ».

Et cela expliquerait la dégradation avec `s` : à `s` grand la banque ferme les
cas faciles, et ce qui reste ouvert est un sous-ensemble où la fenêtre Morton
est justement défavorable.

## 3. Mes trois questions

1. **Confirmez-vous ce diagnostic ?** Si le goulet est le rappel de la banque et
   non la séparation, alors monter `s` est un mauvais investissement — on paie
   `s^3` en records pour un gain qui plafonne, alors que le vrai levier est la
   proposition.

2. **Quelle proposition bornée recommandez-vous à la place d'une fenêtre Morton
   unique ?** Trois pistes me viennent, et je ne sais pas les départager :
   sondage multiple — quelques clés décalées autour de `m_0` plutôt qu'une ;
   descente kNN bornée dans l'arbre déjà construit, qui coûte `O(\log n + L)` et
   n'a aucune discontinuité ; ou proposition par le **cœur commun** entier, dont
   on sait qu'il est inclus dans la région valide.

3. **Faut-il mesurer le rappel de la banque séparément du certificat ?** Je
   confonds aujourd'hui deux échecs sous un seul compteur : « aucun témoin
   proposé n'était valide » et « les témoins proposés étaient valides mais trop
   peu nombreux ». Ce sont deux défauts différents et je ne les distingue pas.

## 4. Non-claims

L'échantillon est de deux mille rectangles ouverts par configuration, sur une
seule famille et une seule taille. Le compte de témoins est exact — balayage
exhaustif — mais la sélection des rectangles est aléatoire uniforme sur les
ouverts, donc non pondérée par la masse. Aucun temps, aucun octet. Le contrat
`50 000` reste entièrement ouvert et G4 reste NO-GO.

## 5. Addendum — je réponds à ma propre question 2, par construction

Plutôt que d'attendre, j'ai implémenté la deuxième de mes trois pistes : une
**descente au meilleur d'abord vers `m_0`** dans l'arbre de Karras déjà
construit, avec le **même budget d'expansions** que la fenêtre Morton. Pile de
taille fixe, aucune structure supplémentaire, aucune allocation — donc la même
forme GPU.

`uniform`, `n=8 000`, `s=4`, `W=64`, `L=32` :

| | fenêtre Morton | descente |
| --- | ---: | ---: |
| q2 fermé (records) | `374 390` | **`749 868`** |
| q3 fermé | `23 936` | **`166 663`** |
| q4 fermé | `12 486` | **`105 119`** |
| faussement résiduels | `76,0 %` | **`51,6 %`** |
| témoins moyens des ouverts | `89,8` | `43,6` |
| **recertifications** | `35 955 200` | **`19 047 331`** |

Le diagnostic est donc **confirmé par construction** : la discontinuité de
Morton était bien le goulet. La descente double q2, multiplie q3 et q4 par sept
et huit, et **divise les recertifications par deux** — elle s'arrête dès qu'elle
a son compte, ce que la fenêtre ne sait pas faire.

Et le signe qui me convainc le plus : les rectangles encore ouverts n'ont plus
`89,8` témoins en moyenne mais `43,6`. Ceux que la descente manque sont
**réellement** plus difficiles, alors que la fenêtre en manquait au hasard.

Ma question 1 devient donc plus tranchée : puisque le levier était la
proposition et non la séparation, faut-il **redescendre `s`** maintenant que la
banque voit mieux ? Le front à `s=4` coûte `140` records par point ; si la
descente ferme autant à `s=2` avec `51` records par point, c'est un gain net
d'un facteur trois sur le compteur physique dominant.

Deux réserves que je signale contre moi :

1. mon tas est un minimum par balayage linéaire sur au plus soixante-quatre
   entrées, donc `O(64)` par extraction — inacceptable tel quel sur device, où
   il faudrait un tableau trié en registres ou un réseau bitonique ;
2. mon compteur `lectures` mélange désormais visites de nœuds internes et tests
   de points, ce qui le rend incomparable entre les deux modes. Seul `recert`
   est comparable, et c'est lui que je cite.

## 6. Addendum — le balayage de `s` avec la descente, et la fourche qu'il révèle

`uniform`, `n=8 000`, `W=64`, `L=32`, proposition par descente :

| `s` | front/pt | q2 fermé | **records résiduels/pt** | masse résiduelle | recertifications |
| ---: | ---: | ---: | ---: | ---: | ---: |
| `1` | `23,7` | `2,6 %` | **`23,1`** | `97,4 %` | **`3,29` M** |
| `3/2` | `35,9` | `8,8 %` | `32,7` | `91,2 %` | `4,96` M |
| `2` | `51,1` | `21,0 %` | `40,4` | `79,0 %` | `7,04` M |
| `3` | `90,1` | `49,9 %` | `45,1` | `50,1 %` | `12,3` M |
| `4` | `140,5` | `66,7 %` | `46,8` | `33,3 %` | `19,0` M |

Deux lectures opposées du même tableau, et je ne peux pas trancher seul :

- **si le coût de la source est par ENREGISTREMENT** — une source générative
  bornée par rectangle — alors `s=1` gagne franchement : `23,1` records
  résiduels par point contre `46,8`, et surtout **`5,8` fois moins de
  recertifications**. Le nombre de records résiduels sature vers `47`, donc
  au-delà de `s=3` on paie du front pour rien ;
- **si le coût est par PAIRE** — une source qui doit développer sa masse —
  alors `s=4` gagne tout aussi franchement : `33,3 %` de masse résiduelle contre
  `97,4 %`, soit trois fois moins de travail aval.

Le taux de faux résiduels, lui, ne tranche pas : il reste entre `52 %` et
`63 %` sur toute la plage, sans tendance nette. La descente a supprimé la
composante « échantillonnage » de ce taux ; ce qui reste est la composante
géométrique, c'est-à-dire `\lambda(s)`, et elle est plus plate que ma formule ne
le prédisait.

**La question que je vous pose est donc celle-ci, et c'est la plus importante
que j'aie à poser aujourd'hui : le coût de votre source est-il par
enregistrement ou par paire ?** De la réponse dépend `s`, donc le front, donc le
kernel, donc tout ce qui vient après. Je ne veux pas figer `s` sur une
préférence.

## 7. RECTIFICATION — ma colonne « masse résiduelle » était fausse

Je dois corriger le tableau de la section 6 avant qu'il ne serve à trancher quoi
que ce soit. J'y déduisais la masse résiduelle de la fraction de **records**
non fermés. **Ces deux quantités n'ont aucune raison de coïncider**, et elles ne
coïncident pas : un rectangle fermé couvre en moyenne bien plus de paires qu'un
rectangle ouvert.

Masse comptée explicitement, et paire tirée **uniformément dans la masse
ouverte** — ma première version tirait un rectangle uniformément puis en prenait
la première paire, ce qui sur-représentait les petits rectangles :

| `s` | front/pt | **masse fermée q2** | records fermés | masse résiduelle | densité de supports | supports estimés |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `1` | `23,7` | `3,04 %` | `2,63 %` | `31,0` M | `0,9 %` | `279` k |
| `2` | `51,1` | `44,83 %` | `21,03 %` | `17,7` M | `1,3 %` | `230` k |
| `4` | `140,5` | **`90,72 %`** | `66,74 %` | `2,97` M | `8,7 %` | `258` k |

**Les trois estimations du nombre de supports concordent** — `279`, `230`, `258`
milliers — alors qu'elles proviennent de trois fronts très différents. C'est la
vérification qui me manquait : le nuage porte environ `250 000` supports q2 à
`n=8 000`, soit **`31` par point**, ce qui est l'ordre attendu pour un graphe de
Gabriel d'ordre dix. Ma comptabilité de masse est donc juste maintenant, et elle
ne l'était pas.

Trois conséquences qui corrigent ce que j'écrivais il y a une heure :

1. **La fermeture en masse monte beaucoup plus vite que la fermeture en
   records** — `3 %`, `45 %`, `91 %` contre `2,6 %`, `21 %`, `67 %`. Le
   certificat ferme préférentiellement les GROS rectangles, ce qui est
   exactement le comportement souhaitable et que je ne voyais pas.
2. **À `s=4` le résiduel ne vaut plus que `2,97` M paires pour `258` k supports**,
   soit un facteur `11,5` de sur-couverture — et non les `33 %` de masse que
   j'annonçais.
3. **La paire résiduelle moyenne a `1 435` à `1 521` témoins**, cent cinquante
   fois le seuil. Le résiduel n'est donc pas peuplé de cas limites : il est
   peuplé de paires massivement fermables que le certificat de rectangle ne
   voit pas.

Ma question de la section 6 reste posée, mais son cadrage change : à `s=4` la
sur-couverture n'est que d'un facteur `11,5`, ce qui rend une source par paire
bien plus plausible que je ne le croyais.
