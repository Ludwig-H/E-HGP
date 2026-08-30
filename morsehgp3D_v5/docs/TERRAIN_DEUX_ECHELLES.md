# `terrain` a deux échelles de hauteur qui croissent en $\sqrt{n}$

`phase=exploration_v5_hors_registre` `backend=cpu_reference`
`profile=quantized_u16_input_only` `mode=audit_independant_math_and_architecture`
`public_status=not_claimed`

Le mur super-quadratique de la lane q4 sur `terrain` **n'est pas algorithmique**.
Il vient du générateur de la famille, qui fait croître deux hauteurs comme
$\sqrt{n}$ alors que l'espacement au sol reste constant : le nuage devient de
plus en plus volumique à mesure qu'il grandit. Gelées, q4 redevient linéaire sur
les trois graines, à $2$ % près.

## 1. Le générateur

`src/cloud/families.hpp`, `terrain_cloud`. L'emprise vaut
$\mathrm{coord} = \sqrt{25 n}$, donc l'espacement au sol est
$\delta = \mathrm{coord}/\sqrt{n} = 5$ unités, **constant**. Mais :

- le **saut de canopée** (2 % des points) est tiré dans
  $[1, \mathrm{coord}/8]$ ;
- l'**amplitude des six calottes** est tirée dans
  $[\mathrm{coord}/16, \mathrm{coord}/8]$.

Les deux sont $\propto \mathrm{coord} \propto \sqrt{n}$. Mesuré sur les nuages
eux-mêmes, le nombre d'altitudes distinctes passe de $137$ à $284$ entre
$n = 8000$ et $n = 32000$ (graine 3), et $z_{\max}$ double quand $n$ quadruple.

## 2. Pourquoi cela coûte cher à q4

Une ancre reliant un point du sol à un point haut a $\lvert ab \rvert \approx H$
avec $H \propto \sqrt{n}$. Sa boule diamétrale a un rayon $H/2$ et couvre une
aire que le sol peuple à raison d'un point par $\delta^2 = 25$ unités² :

$\dfrac{\pi H^2}{4 \cdot 25} = \dfrac{\pi n}{256} \approx 0{,}0123\,n,$

soit $98$ points de cover à $n = 8000$ et $393$ à $n = 32000$ — donc
$\propto n$. Multiplié par $\Theta(n)$ telles ancres, cela donne $\Theta(n^2)$.
L'exposant mesuré des seeds q3 vaut $1{,}89$–$1{,}96$ sur trois graines, un peu
sous $2$ parce que toutes ces ancres ne survivent pas. **Le mécanisme prédit le
chiffre observé sans paramètre ajusté.**

## 3. Borner la seule canopée ne suffit pas

Plafond de canopée à $3$. Exposants locaux, les deux pas
($8000 \to 16000$ puis $16000 \to 32000$), reçu
`receipts/canopee_q4` (18 runs, `statut=complete`) :

| métrique | dépôt, 3 graines | canopée bornée, graines 4 et 5 | canopée bornée, graine 3 |
|---|---|---|---|
| `covers` | 1,49–2,34 | 0,99 – 1,04 | **1,11 · 1,21** |
| `visites_cover` | 1,39–1,68 | 1,00 – 1,02 | **1,07 · 1,14** |
| `seeds_q4` | 1,62–1,80 | 1,00 – 1,01 | **1,30 · 1,39** |
| `tests_coeur` | 1,74–2,48 | 0,99 – 1,01 | **2,63 · 2,96** |

Pour deux graines sur trois, les quatre métriques tombent à $1{,}00$ : borner la
canopée suffit. Mais la graine 3 reste super-quadratique sur l'unité qui paie,
et **s'aggrave** d'un pas à l'autre ($2{,}63$ puis $2{,}96$). Sa grille de
cellules tue $1\,715\,367$ seeds là où les graines 4 et 5 en tuent $4\,244$ et
$0$. Une conclusion tirée des seules graines 4 et 5 aurait donc été fausse.

**Hypothèse réfutée.** J'ai d'abord supposé que la graine 3 gardait une part
plate plus grande, donc plus de configurations cosphériques. Mesure directe sur
les nuages : la part de points à $z \leq 2$ vaut $52$ % pour la graine 3, mais
$55$ % pour la graine 4 (linéaire) et $31$ % pour la graine 5 (linéaire aussi).
La planéité ne discrimine rien.

**Ce qui discrimine** est $z_{\max}$ : $130$ puis $262$ pour la graine 3, contre
$77/154$ et $87/175$. Son relief est $1{,}7$ fois plus haut — la seconde échelle,
celle des calottes, que le plafond de canopée ne touche pas.

## 4. Geler les deux : q4 redevient linéaire

Canopée plafonnée à $3$ **et** amplitude des calottes gelée à $30$ ;
`tests_coeur` :

| graine | $n = 8000$ | $n = 16000$ | $n = 32000$ | exposants locaux |
|---|---:|---:|---:|---|
| 3 | 16 167 387 | 32 616 414 | 65 877 575 | 1,012 · 1,014 |
| 4 | 16 267 910 | 32 676 463 | 65 840 396 | 1,006 · 1,011 |
| 5 | 16 532 213 | 33 239 694 | 66 623 370 | 1,008 · 1,003 |

Les trois graines sont linéaires ($1{,}003$–$1{,}014$) et leurs valeurs absolues
coïncident à $2$ % près. **L'écart de $\times 26$ entre graines s'évanouit** : il
ne venait pas des nuages, il venait de la hauteur qui grandit.

## 5. Ce que cette note ne dit pas

- Elle **ne change pas la famille du dépôt**. `terrain` reste porté bit à bit
  depuis la v3/v4 (`mhgp5_families_fixture`, douze digests calculés par la v4,
  vert) ; les plafonds sont des paramètres de mesure inertes par défaut, et
  aucun reçu de conformité ne les emploie.
- Elle **ne dit pas que q4 est sous-quadratique en général**. Elle dit que sur
  `terrain`, la super-quadraticité mesurée est imputable à une anisotropie du
  générateur, pas à la lane. Un nuage LiDAR réel a une épaisseur bornée par la
  physique, pas par $\sqrt{n}$ ; c'est le régime gelé qui lui ressemble.
- Elle **ne dit rien de `scanline`**, dont les exposants restent non établis
  (étendue $0{,}36$ sur les seeds q3, $0{,}86$ sur q4 à trois graines).
- Une seconde échelle du même type existe dans les familles `scanline_*`
  (`terrain_field`, même tirage $[\mathrm{coord}/16, \mathrm{coord}/8]$) et n'a
  pas été testée ici.
