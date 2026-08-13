# Reçu du 13 août 2026 — le front WSPD est linéaire, et partitionne exactement

Cadre : `phase=exploration_v3_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=proposition_math_non_recue`,
`public_status=not_claimed`. GCP non utilisé.

Programme hors dépôt : [`Wspd_hors_depot.cpp.txt`](Wspd_hors_depot.cpp.txt).
Découpe équitable au milieu de l'axe le plus long, test de séparation
`\lVert c_A-c_B\rVert-r_A-r_B\ge s\max(r_A,r_B)`.

## Cardinal du front, nuage uniforme `u16`, `s=2`

| `n` | rectangles | par point | pente |
| ---: | ---: | ---: | :---: |
| `2 000` | `56 921` | `28,46` | |
| `8 000` | `299 260` | `37,41` | `1,197` |
| `32 000` | `1 396 568` | `43,64` | `1,111` |
| `128 000` | `6 173 359` | `48,23` | `1,072` |

À `s=4` : `73,21`, `108,87`, `137,25`, `159,38` par point, pentes
`1,287 / 1,167 / 1,108`.

## Partition exacte

La masse couverte vaut `1 999 000`, `31 996 000`, `511 984 000`,
`8 191 936 000`, soit exactement `n(n-1)/2` à chaque taille. Chaque paire est
dans un rectangle et un seul.

## Contre-famille à deux plans

| `n` | rectangles | par point | masse |
| ---: | ---: | ---: | ---: |
| `12 500` | `111 027` | `8,88` | `78 118 750` |
| `50 000` | `476 743` | `9,53` | `1 249 975 000` |

La famille qui porte `n^2/4` paires sémantiques est **cinq fois moins chère**
en rectangles que l'uniforme.

## Ce que ce reçu NE mesure pas

Aucune pente d'évaluations : la descente témoin repart de la racine pour chaque
rectangle et coûte `46` millions d'évaluations à `n=1 000`, ce qui est un défaut
de la mesure et non de la structure. Aucun octet, aucun high-water, aucune
famille contractuelle autre que l'uniforme et la contre-famille.
