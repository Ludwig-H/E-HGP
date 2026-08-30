# Réponse de Claude — V143 : le nombre de seeds q3 est un invariant du nuage, et cela ferme toute une classe d'attaques

- **Ancrage :** pin `d9503a87`. Deux mesures sur la cible **produit** `mhgp5`,
  `terrain` $n=8000$, graine 3, `--digest`.
- **Cadre :** `phase=exploration_v5_hors_registre`, `backend=cpu_reference`,
  `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
  `public_status=not_claimed`. GCP non utilisé.

## Le raffinement post-séparation est inerte sur la masse q3

`--postsep=L` existe dans la v5 et était resté à $L=0$ dans toute la campagne
(`tues=0/0/0`). Activé :

| $L$ | rectangles q3 | ancres | **seeds** | candidats | digest |
|---:|---:|---:|---:|---:|---|
| 0 | 207 772 | 436 323 | **3 679 817** | 374 500 | `7edc25dd80efc103` |
| 1 | 264 311 | 359 967 | **3 679 817** | 374 500 | identique |
| 2 | 290 545 | 323 427 | **3 679 817** | 374 500 | identique |
| 3 | 298 415 | 309 601 | **3 679 817** | 374 500 | identique |

Il retire $29\,\%$ des ancres et **zéro seed**. Les ancres qu'il tue sont
exactement celles qui ne coûtaient rien.

## Le nombre de seeds ne dépend pas non plus de la séparation

| $s$ | rectangles q3 | ancres | $\lvert A\rvert\lvert B\rvert$ | **seeds** | temps |
|---:|---:|---:|---:|---:|---:|
| 2 | 101 380 | 10 345 546 | **102,05** | **3 679 817** | 31,0 s |
| 3 | 128 824 | 2 549 862 | 19,79 | **3 679 817** | 23,6 s |
| 4 | 149 574 | 1 017 108 | 6,80 | **3 679 817** | 22,5 s |
| 6 | 184 038 | 545 684 | 2,97 | **3 679 817** | 22,9 s |
| 8 | 207 772 | 436 323 | 2,10 | **3 679 817** | 25,2 s |
| 10 | 225 272 | 390 471 | 1,73 | **3 679 817** | 24,2 s |

Digest identique aux six séparations. Le compte d'ancres varie d'un facteur
$26$, la masse d'amortissement $\lvert A\rvert\lvert B\rvert$ d'un facteur $59$,
et **le nombre de seeds ne bouge pas d'une unité**.

## Ce que cela établit

Le nombre de seeds q3 est
$\sum_{(a,b)} \lvert\lbrace x : (a,b,x) \text{ aigu}, ab \text{ arête maximale}\rbrace\rvert$,
somme sur **toutes** les paires du nuage. C'est une **quantité intrinsèque du
nuage**, pas un artefact de la WSPD : celle-ci partitionne les paires, donc
quelle que soit sa granularité elle énumère exactement les mêmes triplets. Une
ancre ajoutée par une séparation plus faible est une ancre **sans seed**.

Conséquence, qui ferme une classe entière d'attaques : **aucun réglage de la
WSPD — séparation, raffinement post-séparation, critère de terminaison — ne peut
réduire le nombre de propositions q3.** Ma question V142 (« scinder selon la
charge prévue plutôt que selon la séparation ») est donc mal posée, et je la
retire : scinder ne change pas l'ensemble des paires, donc pas la masse.

Il reste que $s$ est un vrai levier d'**amortissement** : à $s=4$,
$\lvert A\rvert\lvert B\rvert = 6{,}80$ contre $2{,}10$ à $s=8$, pour un temps
total *inférieur* ($22{,}5$ s contre $25{,}2$ s). Tout certificat au niveau du
rectangle a donc trois fois plus à amortir à $s=4$. Cela ne sauve pas le
center-cover (il lui manquait un facteur $3{,}4$ au meilleur point **par seed**,
pas par rectangle), mais c'est un fait à retenir pour tout futur certificat par
rectangle.

## Ce qu'il reste, et pourquoi

Le coût de la lane vaut $\text{ancres} \times \text{seeds/ancre} \times 13$. Le
facteur $13$ est optimal (mesuré, plat en $n$). Le nombre de seeds est
intrinsèque. Donc **la seule attaque possible est un certificat qui tue un
GROUPE de seeds sans les énumérer**, et la seule question est : quel groupe ?

Toutes les granularités mesurées aujourd'hui ont une base d'amortissement qui
**stagne ou rétrécit** :

| granularité | base d'amortissement | évolution avec $n$ |
|---|---:|---|
| rectangle WSPD | $\lvert A\rvert\lvert B\rvert = 2{,}10$ (à $s=8$) | stagne |
| patch de rectangle, $K$ utile | $\sim 1$ seed par patch | rétrécit avec $K$ |
| **ancre** | **seeds/ancre $= 4{,}51 \to 25{,}57$** | **croît** |

**L'ancre est la seule unité dont la base d'amortissement croisse avec $n$, et
elle croît exactement sur la cohorte pathologique.** C'est pourquoi $W_3$ — le
certificat par ancre — est le seul mécanisme q3 dont le taux de mort croisse
($19{,}7 \to 32{,}0\,\%$), et c'est là que le prochain incrément doit porter.

## Question

- **V143.** Le certificat de patch a été mesuré **par rectangle**, où il n'avait
  que $2{,}10$ ancres à amortir. Posé **par ancre**, sur le disque des centres de
  cette ancre seule, il aurait $25{,}57$ seeds à amortir à $n=32\,000$ sur
  `terrain` — et ses patches seraient bien plus serrés, donc son crédit bien plus
  fort : rappel, seuls $2{,}2$ à $3{,}3\,\%$ des sites sont crédités pour un patch
  de rectangle, contre $\sim 70\,\%$ de sites intérieurs pour une boule unique.
  Voyez-vous une objection de principe à poser $K=2$ (huit secteurs-patches du
  disque de l'ancre, sommets entiers à l'échelle $2N$ par
  $\hat q = N(a+b) + 2(iu+jv)$) **entre `anchor_kill_cumulated` et la boucle des
  seeds**, c'est-à-dire exactement là où $W_3$ échoue déjà et où les $25$ seeds
  sont sur le point d'être énumérés ?
