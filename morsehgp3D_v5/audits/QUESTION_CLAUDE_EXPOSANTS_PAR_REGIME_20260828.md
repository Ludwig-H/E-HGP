# Question Claude — la sous-quadraticité **par régime**, et ce qu'elle exige (28 août 2026)

Ancrage : mesures de `MESURE_CLAUDE_OU_EST_LA_QUADRATICITE_20260828.md`
(reçus du pin `839cf1ec` et `bench/mhgp5_rect_probe` au HEAD `ff5931fd`).
Cadre : `phase=exploration_v5_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

L'utilisateur a reformulé l'objectif, et cette reformulation change tout :

> « Un algorithme sous-quadratique n'est peut-être pas possible dans le pire
> des cas ; il faudrait au moins que ce soit le cas pour les différents
> régimes considérés. »

C'est un objectif **mesurable**, donc gardable par une porte. Ce document
propose de le transformer en contrat chiffré, et pose les verrous.

## 1. Où en est chaque régime, en un chiffre

Exposant local mesuré entre $n = 32\,000$ et $n = 50\,000$ (reçus appariés,
même graine, même binaire) sur le compteur d'évaluations Jung de la lane q4 —
le poste qui explose :

| régime | exposant mesuré | exposant admissible pour 10 M en 8 h | verdict |
|---|---|---|---|
| `uniform` | **1,06** | 2,89 | tient largement |
| `eight_clusters` | **1,06** | 2,80 | tient largement |
| `scanline_single_pass` | **2,21** | 2,52 | tient de justesse |
| `terrain` | **3,14** | 2,28 | **ne tient pas, il manque 0,86** |

Extrapolation à débit constant ($4{,}8 \times 10^{10}$ évaluations/s, 48 fils —
un **ordre de grandeur**, jamais un temps citable) :

| régime | 1 M | 10 M | 30 M |
|---|---|---|---|
| `uniform` | 0,2 s | 1,8 s | 5,6 s |
| `eight_clusters` | 0,2 s | 2,8 s | 9,1 s |
| `scanline_single_pass` | 33,6 s | 1,5 h | 17,2 h |
| `terrain` | 32,4 min | **31 jours** | **979 jours** |

**La conclusion tient en une phrase : trois régimes sur quatre tiennent déjà
10 M ; le seul qui ne tienne pas est `terrain`, et il lui manque exactement
0,86 d'exposant.** L'objectif n'est donc pas « rendre la génération
sous-quadratique » — c'est **ramener l'exposant de `terrain` sous 2,28**, avec
une marge, et empêcher `scanline` de dériver au-dessus de 2,52.

Réserves, à charge : l'exposant est une pente locale sur moins d'une décade ;
le supposer constant est une hypothèse forte et probablement fausse — sur
`terrain` il **croît** (2,82 → 2,83 → 3,14), ce qui rend l'extrapolation
optimiste, pas pessimiste. Les évaluations Jung sont un compteur d'instrument,
pas un temps. Et la mémoire est un problème **séparé** (≈ 0,35 Mo par point au
pic du fold, soit ≈ 3,5 To à 10 M : c'est L2–L4, pas la génération).

## 2. Pourquoi la coupe par rayon, l'idée naturelle, est réfutée

Le travail est dans les grands rayons et le résultat dans les petits — mais
**pas partout**. Par classe $D_{\max}$ du rectangle, $n = 16\,000$ :

| famille / lane | $D_{\max} < 32$ : travail | $D_{\max} < 32$ : survivants | $D_{\max} \ge 64$ : travail | $D_{\max} \ge 64$ : survivants |
|---|---|---|---|---|
| `uniform` q3 | 12,7 % | 97,8 % | 2,8 % | **0,0 %** |
| `uniform` q4 | 20,2 % | 97,7 % | 0,1 % | **0,0 %** |
| `scanline` q3 | 2,1 % | 93,6 % | 97,0 % | 5,1 % |
| `scanline` q4 | 6,4 % | 57,5 % | 92,3 % | **36,8 %** |
| `terrain` q4 | 13,9 % | 53,3 % | 77,4 % | **24,4 %** |

Sur `uniform`, une coupe par rayon serait exacte — et ne gagnerait rien
(2,8 % du travail). Sur `scanline` q4 et `terrain` q4, les grands rayons
portent **24 % à 37 % des survivants** : une coupe y **changerait l'objet**,
ce qui est interdit. La piste « ignorer les grandes ancres » est donc
**fermée par la mesure**, et il faut le dire avant que quelqu'un ne la
propose.

Ce qui reste licite est un test de rectangle **exact** : ne tuer un rectangle
que si l'on prouve qu'aucune de ses ancres ne peut produire de survivant. Son
gain maximal est donc borné par le travail porté par les rectangles dont
**toutes** les ancres sont déjà tuées par le test d'ancre exact. J'ai
instrumenté ce plafond (`plafond_test_rectangle` dans `bench/rect_probe.cpp`) ;
la mesure est en cours et sera versée avant toute conception.

## 3. Ce que je propose comme contrat

**Contrat d'exposant par régime.** Pour chaque famille de mesure $F$ et chaque
grandeur instrumentée $Q$ (ancres q3/q4, seeds q3/q4, complétions q4,
évaluations Jung), l'exposant local entre deux tailles consécutives de
$\lbrace 8000, 16000, 32000, 50000 \rbrace$ doit vérifier
$e_{F,Q} \le e^{*}_{F}$, avec $e^{*}$ gravé par famille et **vérifié par une
porte** qui refuse (code 3) si le plancher est franchi. Valeurs de départ
proposées, choisies au-dessus des mesures actuelles sauf pour `terrain` :

| famille | $e^{*}$ proposé | mesure actuelle |
|---|---|---|
| `uniform` | 1,20 | 1,06 |
| `eight_clusters` | 1,20 | 1,06 |
| `scanline_single_pass` | 2,30 | 2,21 |
| `terrain` | **2,20** | 3,14 (échec assumé, c'est la cible) |

Cette porte a trois vertus : elle rend l'objectif **falsifiable** ; elle
détecte une régression d'exposant que les temps absolus masquent ; et elle
dit, famille par famille, si le contrat 10–30 M est encore atteignable.

## 4. Verrous

- **V36** — acceptez-vous le **contrat d'exposant par régime** comme critère
  d'avancement (porte à code 3 sur les exposants locaux des compteurs de
  génération, seuils gravés par famille), plutôt qu'un objectif de
  sous-quadraticité au pire cas ?
- **V37** — la coupe par rayon est réfutée par la mesure (24 à 37 % des
  survivants q4 vivent à $D_{\max} \ge 64$ sur `terrain` et `scanline`).
  Confirmez-vous qu'elle doit entrer dans `PISTES_FERMEES.md` **avec cette
  mesure comme cause**, avant que quelqu'un ne la repropose ?
- **V38** — existe-t-il un **minorant du nombre de points strictement
  intérieurs valable pour toutes les ancres d'un rectangle** $A \times B$, et
  serré ? Formellement : une fonction $\underline{I}(A, B)$ calculable en
  $O(\text{polylog})$ telle que $\underline{I}(A, B) \le \min_{(a,b) \in A \times B} \lvert I_{B(a,b)} \rvert$
  et qui atteigne le seuil $h_q$ aussi souvent que le test d'ancre. Les
  histogrammes de coins actuels en sont un, mais lâche (ils ne tuent **rien**
  dans les classes $c \le 3$ et 75 % en $c = 6$ là où le test d'ancre en tue
  96 %). Est-ce un problème ouvert, ou connaissez-vous la construction ?
- **V39** — sur `terrain` q4, l'exposant est de **3,14** alors que les ancres
  ne croissent qu'en $n^{1{,}41}$ : le coût par ancre croît donc en
  $n^{1{,}73}$. Avant de concevoir, faut-il d'abord instrumenter **ce qui
  grossit dans une ancre `terrain`** (taille du cover ? nombre de seeds par
  ancre ? profondeur de la complétion q4 ?) — je penche pour oui, et je ne
  proposerai rien avant cette mesure.
- **V40** — l'asymétrie (médiane 5 seeds par rectangle contre un maximum de
  849 452) rend toute affectation « un rectangle par bloc GPU » inutilisable.
  Le découpage d'un rectangle **vivant** en sous-rectangles, à seule fin de
  répartir le travail, ne change ni les paires énumérées ni l'objet — il ne
  touche pas au critère terminal de la WSPD, donc pas à la piste fermée « cap
  de population dans le critère terminal ». Confirmez-vous cette lecture ?

## 5. Ce que je ne propose pas encore, et pourquoi

Aucune conception. Trois mesures manquent, et chacune peut invalider une
famille entière de solutions : le plafond d'un test de rectangle exact (en
cours), la décomposition du coût par ancre sur `terrain` (V39), et le
comportement des exposants au-delà de 50 000 points — sans quoi toute
extrapolation à 10 M reste une pente sur moins d'une décade.
