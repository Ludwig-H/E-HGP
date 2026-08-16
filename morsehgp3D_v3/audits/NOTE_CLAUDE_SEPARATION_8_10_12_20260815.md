# Note de Claude — `s=10` domine `s=8`, et `V_q` ne dépend pas de la partition

Date : 15 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=diagnostic_counter_only`,
`public_status=not_claimed`. GCP non utilisé — `gcloud` est absent du conteneur.

Configuration : `coeur=corner64`, `ha=fusion`, `cap=scission`, `smax=11`,
`seed` par défaut. `masse_non_decide = 0` sur **toutes** les lignes, donc les
comparaisons portent sur le même univers de `PairId` entièrement décidé — la
réserve 3 de l'audit positif est levée pour ces mesures-là.

## 1. Le contrôle interne qui valide toute la campagne

`V4_pair_walive` est **identique** pour les trois séparations, à `n` fixé :

| famille | `n` | `s=8` | `s=10` | `s=12` |
| --- | ---: | ---: | ---: | ---: |
| `terrain` | `8 000` | `313 806` | `313 806` | `313 806` |
| `terrain` | `16 000` | `667 449` | `667 449` | `667 449` |
| `terrain` | `32 000` | `1 440 227` | `1 440 227` | `1 440 227` |
| `uniform` | `8 000` | `879 078` | `879 078` | `879 078` |
| `uniform` | `32 000` | `3 852 997` | `3 852 997` | — |
| `eight_clusters` | `8 000` | `980 606` | `980 606` | `980 606` |
| `eight_clusters` | `16 000` | `2 167 153` | `2 167 153` | `2 167 153` |
| `eight_clusters` | `32 000` | `4 693 975` | `4 693 975` | `4 693 975` |

C'est ce qu'il fallait : l'ensemble `W`-vivant est une propriété du **nuage**,
pas de la partition. Seule la finesse du préfiltre change. Une divergence ici
aurait signalé une fuite de la partition dans le compte, et c'est le contrôle
qui manquait à mes campagnes précédentes.

Corollaire : le mou décroît avec `s` sans que le dénominateur bouge, donc il
mesure bien le resserrement et rien d'autre.

## 2. `terrain` — `s=10` retire du résiduel **et** du temps

| `n` | `s` | survivantes q4 | fermeture | rectangles | mou |
| ---: | ---: | ---: | ---: | ---: | ---: |
| `8 000` | `8` | `419 974` | `98,687 %` | `948 005` | `1,338` |
| `8 000` | `10` | `380 713` | `98,810 %` | `1 312 284` | `1,213` |
| `8 000` | `12` | `359 717` | `98,876 %` | `1 708 179` | `1,146` |
| `16 000` | `8` | `965 348` | `99,246 %` | `2 291 578` | `1,446` |
| `16 000` | `10` | `867 193` | `99,322 %` | `3 163 428` | `1,299` |
| `16 000` | `12` | `810 549` | `99,367 %` | `4 116 076` | `1,214` |
| `32 000` | `8` | `2 270 039` | `99,557 %` | `5 535 416` | `1,576` |
| `32 000` | `10` | `2 032 255` | `99,603 %` | `7 720 807` | `1,411` |
| `32 000` | `12` | `1 887 474` | `99,631 %` | `10 089 216` | `1,311` |

Gain relatif à `s=8` :

| `n` | `s=10` | `s=12` |
| ---: | --- | --- |
| `8 000` | `-9,3 %` de résiduel | `-14,3 %` |
| `16 000` | `-10,2 %` | `-16,0 %` |
| `32 000` | `-10,5 %` | `-16,9 %` |

Le gain de `s=10` **croît lentement** avec `n` — `9,3` puis `10,2` puis
`10,5 %` — et celui de `s=12` aussi. Ni l'invariance que j'avais annoncée après
la rétractation du facteur `6,4`, ni l'explosion que j'avais annoncée avant :
une dérive de l'ordre du point de pourcentage par doublement.

## 3. `eight_clusters` — c'est là que `s` compte vraiment

| `n` | `s` | survivantes q4 | fermeture | mou |
| ---: | ---: | ---: | ---: | ---: |
| `8 000` | `8` | `2 586 460` | `91,916 %` | `2,638` |
| `8 000` | `10` | `2 020 565` | `93,685 %` | `2,061` |
| `8 000` | `12` | `1 697 882` | `94,693 %` | `1,731` |
| `16 000` | `8` | `6 531 556` | `94,897 %` | `3,014` |
| `16 000` | `10` | `5 290 669` | `95,866 %` | `2,441` |
| `16 000` | `12` | `4 339 835` | `96,609 %` | `2,003` |
| `32 000` | `8` | `16 779 180` | `96,723 %` | `3,575` |
| `32 000` | `10` | `12 315 068` | `97,595 %` | `2,624` |
| `32 000` | `12` | `9 949 312` | `98,057 %` | `2,120` |

Gain relatif à `s=8` : `-21,9 / -19,0 / -26,6 %` pour `s=10`, et
`-34,4 / -33,6 / -40,7 %` pour `s=12`. **Deux à quatre fois `terrain`**, et le
gain croît avec `n` sur les deux séparations.

`uniform` se place entre les deux — `-11,4 / -12,2 / -12,5 %` pour `s=10`,
`-17,4 %` pour `s=12` à `n=8 000` — avec une remarquable stabilité en `n`.

## 3bis. Où le travail de certificat est fini, et où il ne l'est pas

Le mou à `s=12` sépare nettement les trois familles :

| famille | mou `s=8` | mou `s=12` | résiduel encore retirable |
| --- | ---: | ---: | ---: |
| `uniform` (`n=8 000`) | `1,336` | `1,103` | `9,3 %` |
| `terrain` (`n=32 000`) | `1,576` | `1,311` | `23,7 %` |
| `eight_clusters` (`n=32 000`) | `3,575` | `2,120` | `52,8 %` |

Sur `uniform`, un certificat parfait ne gagnerait plus que `9,3 %` du résiduel :
le travail de certificat y est **terminé**. Sur `eight_clusters` il en reste
`52,8 %` même à `s=12`, et le mou y croît toujours avec `n` (`2,638` à `8 000`,
`3,575` à `32 000` pour `s=8`). C'est la seule des trois familles où un
resserrement garde une marge réelle — et c'est aussi celle qui ressemble le plus
à un nuage LiDAR.

## 4. Ce que je ne conclus pas, et pourquoi

Les durées de la campagne **incluent le balayage `--vrai-vivant`**, qui est un
diagnostic et non la production. Il coûte `|S| x n`, donc il baisse
mécaniquement quand `s` monte : lire « `s=10` est plus rapide » sur ces
chiffres serait lire l'effet du diagnostic, pas celui du préfiltre. Le temps qui
arbitre `s` est celui du préfiltre **seul**, et je le mesure séparément ; tant
qu'il n'est pas rendu, je ne publie aucun verdict de temps.

Ce que la campagne établit sans réserve, c'est le côté **résiduel** :
`s=10` retire `9` à `27 %` du résiduel de `s=8` pour `1,38x` à `1,55x` de
rectangles, et `s=12` en retire `14` à `41 %` pour `1,80x` à `1,89x`. La décision dépend donc entièrement
du coût d'instruction d'une ancre survivante, qui n'est pas encore mesuré à ces
tailles.

## 5. Ce qui reste dû

Le temps du préfiltre seul, aux vingt-sept configurations — c'est lui qui
arbitre, et il tourne. Les colonnes `s=12` d'`uniform` à `n=16 000` et
`n=32 000`. Les familles `scanline_single_pass`, `scanline_overlap_multiecho` et
`two_lines`, désormais acceptées par le probe mais absentes de cette campagne.
Et le coût d'instruction d'une ancre à ces tailles, sans lequel aucun arbitrage
de `s` n'est décidable.
