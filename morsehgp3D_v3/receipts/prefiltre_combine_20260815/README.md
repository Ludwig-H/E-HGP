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
| graine | `3` pour les trente-six configurations |
| brut | [`rampe_brute.txt`](rampe_brute.txt), trente-six lignes |

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

| famille | `s` | `K` | `n=8 000` | `n=16 000` | `n=32 000` | monotone |
| --- | ---: | ---: | ---: | ---: | ---: | :---: |
| `uniform` | `6` | `5` | `97,55 %` | `98,69 %` | `99,31 %` | oui |
| `uniform` | `6` | `10` | `90,98 %` | `94,89 %` | `97,23 %` | oui |
| `uniform` | `8` | `5` | `98,50 %` | `99,22 %` | `99,59 %` | oui |
| `uniform` | `8` | `10` | `95,41 %` | `97,54 %` | `98,69 %` | oui |
| `eight_clusters` | `6` | `5` | `91,17 %` | `94,38 %` | `97,32 %` | oui |
| `eight_clusters` | `6` | `10` | `76,52 %` | `85,00 %` | `92,44 %` | oui |
| `eight_clusters` | `8` | `5` | `93,77 %` | `96,18 %` | `97,51 %` | oui |
| `eight_clusters` | `8` | `10` | `84,13 %` | `89,66 %` | `93,59 %` | oui |
| `terrain` | `6` | `5` | `99,02 %` | `98,99 %` | `88,70 %` | **non** |
| `terrain` | `6` | `10` | `97,22 %` | `97,98 %` | `88,13 %` | **non** |
| `terrain` | `8` | `5` | `99,33 %` | `99,58 %` | `98,39 %` | **non** |
| `terrain` | `8` | `10` | `98,30 %` | `98,99 %` | `98,04 %` | **non** |

Les colonnes q2 et q3 sont dans le brut. q2 reste au-dessus de `99 %` partout
sauf sur `terrain,n=32000,s=6`, où il tombe à `89,0 %` avec les autres.

## Les quatre faits

**1. La fermeture croît avec `n` sur huit séries sur douze — et pas sur
`terrain`.** Les quatre séries `uniform` et les quatre séries `eight_clusters`
sont strictement croissantes ; en q4 à `s=6, K=10`, `eight_clusters` fait
`76,52 -> 85,00 -> 92,44 %`. C'est notable, parce que c'est la famille qui a
résisté au cœur commun, au fuseau sur rectangle et aux cinq certificats de bloc,
et parce qu'aucun certificat antérieur du dossier ne se renforçait avec la
taille du nuage.

Mais **les quatre séries `terrain` déclinent à `n=32 000`**, et l'énoncé
« la fermeture croît avec `n` » est donc **faux en général**. Trois points ne
font de toute façon pas une pente.

**2. `terrain` à `s=6, n=32 000` s'effondre, et ce n'est pas un artefact.**
`88,70 %` contre `98,99 %` à `n=16 000`. Le cap n'est pas en cause :
`cellule_max = 482` reste sous `512`, donc toutes les paires sont jugées. C'est
la séparation qui devient insuffisante pour ce nuage à cette densité — une
cellule large affaiblit tous les tests uniformes à la fois, puisque `H` doit
rester positif sur **toute** la boîte.

**3. `s = 8` domine `s = 6` partout**, et l'écart devient critique à grande
taille : sur `terrain,n=32000`, `98,04 %` contre `88,13 %`, soit un facteur
**six** sur le résiduel — `10,0` millions d'ancres contre `60,8` millions.
L'arbitrage n'était pas évident : augmenter `s` rétrécit `A` et `B`, donc
appauvrit `h_a` et `h_b` ; mais il resserre les bornes uniformes plus vite qu'il
ne les appauvrit.

**4. `K = 5` ferme davantage que `K = 10`**, mécaniquement, les seuils passant
de `10/9/8` à `5/4/3`. L'écart est le plus grand là où la fermeture est la plus
faible : `+14,7` points sur `eight_clusters,n=8000,s=6`.

## Coût, et pourquoi `uniform` est le plus lent

Le coût par million de rectangles est quasi constant d'une famille à l'autre —
`16` à `32` secondes. Toute la différence de temps vient du **nombre de
rectangles**, à `n=32 000` :

| famille | `s` | rectangles | par point | cellule max | secondes (`K=10`) |
| --- | ---: | ---: | ---: | ---: | ---: |
| `terrain` | `6` | `3,64 M` | `113,6` | `482` | `113` |
| `terrain` | `8` | `5,60 M` | `175,0` | `478` | `181` |
| `eight_clusters` | `6` | `8,02 M` | `250,7` | `492` | `178` |
| `eight_clusters` | `8` | `12,69 M` | `396,7` | `381` | `275` |
| `uniform` | `6` | `12,71 M` | `397,3` | `270` | `393` |
| `uniform` | `8` | `22,41 M` | `700,4` | `146` | `710` |

La taille d'une WSPD est `O(s^3 n)`, mais **sa constante dépend de la dimension
de doublement du nuage**. `terrain` est quasi-surfacique, de dimension
intrinsèque proche de deux : peu de nœuds réellement tridimensionnels, et la
séparation se satisfait vite. `eight_clusters` concentre la masse en huit amas,
donc la plupart des paires de nœuds sont séparées d'emblée. `uniform` remplit le
volume : l'octree se subdivise dans les trois directions à chaque niveau et il
faut descendre plus profond.

La signature se lit dans la colonne `cellule max` : `uniform` à `s=8` a des
cellules de `146` points quand `terrain` en a `478`. Plus de rectangles, plus
petits — la WSPD a dû subdiviser beaucoup plus. C'est cohérent avec le reçu
`wspd_front_lineaire_20260813` : « la famille qui porte `n^2/4` paires
sémantiques est cinq fois moins chère en rectangles que l'uniforme ».

Second effet, mineur et inverse : `terrain` paie un peu plus **par** rectangle,
ses cellules étant plus grosses et `h_a` étant en `O(|A|^2)` avec sortie
anticipée. Le premier effet domine largement.

Le passage de `s=6` à `s=8` coûte `1,76x` en rectangles sur `uniform` à
`n=32 000`. À `n=50 000` la même famille en produirait de l'ordre de `35`
millions, et l'arbitrage devra être refait.

## Ce que ce reçu ne dit pas

Il ne mesure aucun support, aucun census, aucun débit. `98,69 %` de fermeture à
`uniform,n=32000,s=8,K=10` laisse encore `6,68` millions d'ancres, soit `209`
par point ; et `93,59 %` sur `eight_clusters` dans la même configuration en
laisse `32,8` millions, soit `1 026` par point — à comparer aux `428` supports
par point mesurés à `n=50 000`. **Le préfiltre ne rend donc pas le producteur
output-sensitive** : il retire une part importante du travail, pas l'ordre de
grandeur.

Aucune pente n'est publiée. La règle du plan de test exige trois exposants
successifs par arité avant toute conclusion d'échelle, et ce reçu ne les calcule
pas.
