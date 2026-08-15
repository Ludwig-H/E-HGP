# Reçu — préfiltre d'ancre combiné, 15 août 2026

Cadre : `phase=exploration_v3_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=diagnostic_counter_only`,
`public_status=not_claimed`. GCP non utilisé.

## Provenance

| | |
| --- | --- |
| commit | `2c3eee3761cd89755ab33784007beaab28d7c834` |
| worktree | propre avant la campagne |
| source | `prototype/combined_prefilter_probe.cpp`, sha256 `3e135334dc3a47416c8c3be76f04fbc334216fea5691e4bbafd91b3397afe1ff` |
| ELF | sha256 `b1472270bac38645f8237131d8025c6d89dff9886e7c5dda76179936e61c0898` |
| build | `cmake -DCMAKE_BUILD_TYPE=Release -G Ninja`, gcc 13.3.0, `-O3 -Werror` |
| graine | `3` pour toutes les configurations |
| brut | [`rampe_brute.txt`](rampe_brute.txt) |

Rejeu d'une ligne :

```bash
./build/v3/mhgp3v_combined_prefilter_probe \
  --points=32000 --family=eight_clusters --seed=3 --separation=8 --smax=11
```

## Ce que la colonne mesure

Le pourcentage d'**ancres fermées**, rapporté à `C(n,2)`. Une ancre est une
paire `(a,b)` candidate à être l'arête diamétrale d'un support. Ce n'est **pas**
un nombre de supports, ni un débit, ni une pente reçue.

Le ledger boucle exactement sur `C(n,2)` à chaque ligne — la partition
Callahan--Kosaraju est vérifiée, pas supposée. Un rectangle dont une extrémité
dépasse le cap n'est pas décidé et toutes ses paires sont comptées
**survivantes** : la mesure majore donc toujours le résiduel.

`K = s_max - 1`. Les seuils de mort sont `h_q = s_max - q + 1`, soit `10/9/8` à
`K=10` et `5/4/3` à `K=5`.

## Fermeture q4

| famille | `s` | `K` | `n=8 000` | `n=16 000` | `n=32 000` |
| --- | ---: | ---: | ---: | ---: | ---: |
| `uniform` | `6` | `5` | `97,55 %` | `98,69 %` | `99,31 %` |
| `uniform` | `6` | `10` | `90,98 %` | `94,90 %` | `97,23 %` |
| `uniform` | `8` | `5` | `98,50 %` | `99,22 %` | `99,59 %` |
| `uniform` | `8` | `10` | `95,41 %` | `97,54 %` | *en cours* |
| `eight_clusters` | `6` | `5` | `91,17 %` | `94,38 %` | `97,32 %` |
| `eight_clusters` | `6` | `10` | `76,52 %` | `85,00 %` | `92,44 %` |
| `eight_clusters` | `8` | `5` | `93,77 %` | `96,18 %` | `97,51 %` |
| `eight_clusters` | `8` | `10` | `84,13 %` | `89,66 %` | `93,59 %` |
| `terrain` | `6` | `5` | `99,02 %` | `98,99 %` | `88,70 %` |
| `terrain` | `6` | `10` | `97,22 %` | `97,98 %` | `88,13 %` |
| `terrain` | `8` | `5` | `99,33 %` | `99,58 %` | `98,39 %` |
| `terrain` | `8` | `10` | `98,30 %` | `98,99 %` | `98,04 %` |

Les colonnes q2 et q3 sont dans le brut. q2 reste au-dessus de `99 %` partout
sauf sur `terrain,n=32000,s=6`, où il tombe à `89,0 %` avec les autres.

## Les quatre faits

**1. La fermeture croît avec `n` sur la famille difficile.** Sur
`eight_clusters`, les quatre séries sont monotones croissantes : en q4 à
`s=6, K=10`, `76,52 → 85,00 → 92,44 %`. C'est la famille qui a résisté au cœur
commun, au fuseau sur rectangle et aux cinq certificats de bloc. Trois points ne
sont pas une preuve d'asymptotique, mais aucun certificat antérieur du dossier
ne se renforçait avec la taille.

**2. `s = 8` domine `s = 6` partout**, et l'écart devient critique à grande
taille. Sur `terrain,n=32000` il vaut `88,13 %` contre `98,04 %`, soit un
facteur **six** sur le résiduel (`60,8` millions d'ancres contre `10,0`
millions). Le mécanisme est visible dans le brut : à `s=6` les cellules
atteignent `482` points, et une cellule large affaiblit tous les tests
uniformes à la fois, puisque `H` doit rester positif sur toute la boîte.

**3. L'effondrement de `terrain` à `s=6, n=32000` est un signal, pas un
artefact.** Il ne s'agit pas de rectangles non décidés — `cellule_max = 482`
reste sous le cap `512`, donc toutes les paires sont jugées. C'est la
séparation elle-même qui devient insuffisante pour ce nuage à cette densité.

**4. `K=5` ferme davantage que `K=10`**, mécaniquement, les seuils passant de
`10/9/8` à `5/4/3`. L'écart est le plus grand là où la fermeture est la plus
faible : `+14,7` points sur `eight_clusters,n=8000,s=6`.

## Ce que ce reçu ne dit pas

Il ne mesure aucun support, aucun census, aucun débit. `93,59 %` de fermeture à
`eight_clusters,n=32000,s=8,K=10` laisse encore `32,8` millions d'ancres, soit
`1 026` par point — à comparer aux `428` supports par point mesurés à
`n=50 000`. **Le préfiltre ne rend donc pas le producteur output-sensitive** ;
il retire une part importante du travail, pas l'ordre de grandeur.

Aucune pente n'est publiée. La règle du plan de test exige trois exposants
successifs par arité avant toute conclusion d'échelle, et ce reçu ne les
calcule pas.

La configuration `uniform,n=32000,s=8,K=10` n'a pas rendu au moment de la
rédaction ; elle est marquée *en cours* et sera ajoutée au brut sans réécrire
les autres lignes.
