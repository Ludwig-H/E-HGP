# Note de Claude — l'arbitrage `s=6` contre `s=8` n'est pas invariant d'échelle

Date : 15 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=diagnostic_counter_only`,
`public_status=not_claimed`. GCP non utilisé.

Mesure faite à la demande de l'utilisateur, qui soutenait que `s=8` est le bon
compromis pour des nuages de plusieurs dizaines de milliers de points. Il a
raison, et le chiffre est beaucoup plus net que je ne l'attendais.

> [!CAUTION]
> **Le facteur `6,4` est rétracté : il vient du cap, pas des certificats.**
> Le probe traite tout rectangle dont une extrémité dépasse `cap-cellule=512`
> comme indécidé et ajoute **toute** sa masse aux survivantes. À
> `terrain, n=32 000`, la baisse de masse hors cap vaut `49 096 900` ancres sur
> les `49 566 930` gagnées, soit **`99,052 %` du gain**. Sur la seule masse
> jugée, `s=8` gagne `17,238 %` — contre `17,503 %` à `n=8 000`, où aucun
> rectangle n'est hors cap. **L'effet intrinsèque est donc presque invariant en
> `n`**, et la mesure agrégée dit surtout que `s=8` raffine assez la WSPD pour
> contourner le cap.
>
> J'avais exclu ce biais en invoquant `cellule_max = 482 < 512`. Ce champ était
> mis à jour **après** le `continue` des rectangles hors cap : il ne pouvait
> structurellement jamais dépasser le cap. Le compteur est corrigé — le vrai
> maximum vaut `677` — et le reçu du 15 août reposait sur le même raisonnement
> circulaire. Détail :
> [`AUDIT_REAUDIT_DUAL_TREE_COEUR_BOULE_SEPARATION_EB1B52A_20260815.md`](AUDIT_REAUDIT_DUAL_TREE_COEUR_BOULE_SEPARATION_EB1B52A_20260815.md)
> section 5.

## 1. Le fait

`terrain`, `K=10`, chemin courant — cœur `corner64`, `h_a` par dual-tree :

| `n` | `s` | ancres survivantes | fermeture q4 | temps |
| ---: | ---: | ---: | ---: | ---: |
| `8 000` | `6` | `504 885` | `98,422 %` | `12,2 s` |
| `8 000` | `8` | `416 516` | `98,698 %` | `19,2 s` |
| `32 000` | `6` | `58 684 461` | `88,538 %` | `173,5 s` |
| `32 000` | `8` | `9 117 531` | `98,219 %` | `290,2 s` |

À `n=8 000`, `s=8` retire `17,5 %` du résiduel pour `+57 %` de temps. À
`n=32 000`, il en retire **`84,5 %`** — un facteur `6,4` — pour `+67 %` de
temps. **L'avantage de `s=8` croît brutalement avec `n`**, et l'arbitrage n'est
donc pas invariant d'échelle.

## 2. Le point de bascule, qui rend la décision factuelle

`s=8` est rentable dès que le coût d'instruction d'une ancre survivante dépasse
le rapport (temps supplémentaire) / (ancres évitées) :

| famille | `n` | seuil de rentabilité |
| --- | ---: | ---: |
| `terrain` | `8 000` | `79 us` par ancre |
| `eight_clusters` | `8 000` | `17 us` |
| `uniform` | `8 000` | `130 us` |
| `terrain` | `32 000` | **`2,4 us`** |

Instruire une ancre, c'est retrouver ses supports — au minimum un balayage de
voisinage. Le seuil de `2,4 us` à `n=32 000` est donc franchi de très loin :
à cette échelle la question ne se pose plus.

Ce raisonnement suppose que le coût d'instruction par ancre ne dépend pas de
`s`, ce qui est vrai puisqu'une ancre survivante est la même paire dans les deux
régimes ; seul leur nombre change.

## 3. Ce que cela dit du reçu, et de mes améliorations

Le reçu `prefiltre_combine_20260815` notait déjà cet effondrement — fait 2,
`terrain` à `s=6, n=32 000` tombant à `88,70 %` — et l'attribuait à une
séparation devenue insuffisante pour ce nuage à cette densité. **Ce diagnostic
tient**, et mes améliorations de la journée ne l'ont pas changé : `88,538 %`
aujourd'hui contre `88,13 %` alors.

C'est un point important pour lire les mesures de cette journée. À `n=8 000`,
`corner64`, l'autorité à huit coins et la borne couplée avaient presque comblé
l'écart entre `s=6` et `s=8` — `98,42 %` contre `98,70 %`, soit trois dixièmes
de point. J'aurais pu en conclure que le choix de `s` cessait d'importer. À
`n=32 000` l'écart est de **dix points**. Les améliorations resserrent les
certificats à séparation donnée ; elles ne compensent pas une séparation
insuffisante.

## 4. Conséquence opérationnelle

Les dix-huit lignes à `s=6` de la campagne des trente-six configurations sont
peu informatives pour la cible `50 000` : elles mesurent un régime que personne
ne choisirait. Quand la campagne sera régénérée — elle ne l'est pas, le reçu
garde son bandeau q2 invalide — il faudra soit la recentrer sur `s in {8,10}`,
soit assumer explicitement que la moitié des lignes documente un mauvais
réglage.

Reste à mesurer, et non prétendu ici : le même arbitrage sur `uniform` et
`eight_clusters` à `n=32 000`, et le comportement à `s=10`, où le nombre de
rectangles croît en `s^3` alors que le rayon du cœur ne croît que linéairement.
Le point de retournement existe donc quelque part au-dessus de `s=8` ; je ne
sais pas encore où.
