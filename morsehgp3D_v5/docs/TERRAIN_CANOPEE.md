# La pente super-quadratique de `terrain` vient de la canopée, pas de l'algorithme

Document durable, 30 août 2026, pin `e6ed85df`. Toutes les mesures sont à
$s=8$ — le seul profil admis — avec le vrai prédicat `is_acute_seed` et le vrai
test $W_3$.

## Le fait

`seeds` par **ancre survivante** — l'analogue exact de `seeds[0]` du pipeline —
sur `terrain`, échantillon de 1 200 rectangles vivants tirés par hachage :

| $n$ | canopée du dépôt ($\mathrm{lift} \in [1,\ \mathrm{coord}/8]$) | canopée absolue ($\mathrm{lift} \in [1,3]$) |
|---:|---:|---:|
| 2 000 | 5,97 | **5,41** |
| 8 000 | 12,62 | **5,45** |
| 32 000 | **53,21** | **5,72** |

Avec une canopée dont la hauteur ne dépend pas de l'emprise, la charge par ancre
est **constante à $6\,\%$ près sur un facteur $16$ en $n$** — le même régime que
`uniform` ($11{,}15$ aux cinq tailles). Avec la canopée du dépôt, elle est
multipliée par neuf.

## Le mécanisme, isolé

Survie des ancres par octave de longueur (unité : $1$-NN $= 2{,}83$),
`terrain` $n = 32\,000$, 8 000 rectangles échantillonnés :

| octave $\lvert ab\rvert$ | canopée $\mathrm{coord}/8$ | canopée $=28$ | canopée $=3$ |
|---|---:|---:|---:|
| [4, 8) | 94,7 % | 94,6 % | 93,9 % |
| [8, 16) | 44,0 % | 45,0 % | 42,9 % |
| **[16, 32)** | **67,7 %** (1 388) | 16,9 % (236) | **2,3 %** (175) |
| **[32, 64)** | **30,8 %** (3 995) | 0,0 % (931) | **0,0 %** (743) |
| **[64, 128)** | 1,3 % (4 029) | 0,0 % (2 731) | **0,0 %** (2 301) |

Les quatre premières octaves sont **invariantes d'échelle** : à $n=2000$ elles
donnent $100 / 100 / 95{,}0 / 43{,}7\,\%$, à $n=32\,000$
$100 / 100 / 94{,}7 / 44{,}0\,\%$. Toute la pathologie est dans les octaves
longues, et elle disparaît quand la canopée cesse de croître.

La signature qui l'explique : dans l'octave [16, 32) à $n=32\,000$, le cover
utile vaut $41{,}2$ points — trois fois celui de l'octave inférieure — pour
seulement $3{,}88$ témoins $W_3$, **moins** que les $6{,}36$ de l'octave
inférieure. Les points sont là, mais **hors du citron** : la corde ne passe pas
près d'eux. C'est une ancre entre un point suspendu et le sol, dont le fuseau
traverse de l'air. **Aucun certificat fondé sur des témoins ne peut la tuer**, et
elle porte ensuite des centaines de seeds.

## Ce qui est en cause, et ce qui ne l'est pas

**N'est pas en cause :** le générateur est correct — points rendus complets,
densité locale rigoureusement invariante ($1$-NN $2{,}83$, $8$-NN $8{,}3$,
$64$-NN $23{,}4$ aux trois tailles), dimension intrinsèque locale
$2{,}01 / 2{,}00 / 1{,}99$, et le prédicat de propriété d'un triplet est exact —
**zéro double comptage** sur $20{,}7$ M de triplets malgré $0{,}07$–$0{,}11\,\%$
d'arêtes égales.

**N'est pas en cause non plus :** le phénomène géométrique. Un point isolé
au-dessus d'une surface a réellement des ancres au fuseau vide, et un LiDAR
aérien renvoie réellement de la végétation au-dessus du sol nu.

**Est en cause :** l'échelle. `terrain_cloud` tire
`canopy_lift(1, max(2, coord/8))`, donc une hauteur d'arbre **proportionnelle à
l'emprise du levé** — $28$ unités à $n=2000$, $112$ à $n=32\,000$. Un arbre
mesure la même chose que l'on survole $1$ ou $100\ \mathrm{km}^2$. C'est le seul
terme non auto-similaire du générateur, et il transforme un phénomène **borné**
en un phénomène **croissant**.

## Conséquence sur la question de la sous-quadraticité

La pente locale des seeds de `terrain` valait $2{,}093$ [$2{,}045$ ; $2{,}191$]
sur trois graines — le seul régime où la sous-quadraticité était contredite. Avec
une canopée d'échelle physique, `seeds/ancre` devient constant et la charge suit
le nombre d'ancres, quasi linéaire : **le même régime que les trois autres
cohortes.**

Autrement dit : **la lane q3 n'a pas de mur sur une surface dense de densité
constante.** Le mur mesuré était celui d'une famille dont un paramètre croît
avec $n$ sans raison physique.

## Ce que je ne fais pas

Je **ne modifie pas** `families.hpp`. Changer `canopy_lift` changerait tous les
digests et invaliderait tous les reçus, y compris les campagnes
`masses_q3_seed{3,4,5}_20260829` et les conformités v4. C'est une décision de
spécification, pas une correction de bug : le générateur fait exactement ce que
sa spécification dit.

Ce qui est proposé, et rien de plus : que la spécification de `terrain` borne la
hauteur de canopée indépendamment de `coord`, ou qu'une **contre-famille**
`terrain_canopee_bornee` soit ajoutée à côté, pour que la mesure d'échelle ne
soit plus portée par un paramètre non physique.

## Reproduction

Sondes `octave2.cpp` et `canopee.cpp` (copie exacte de `terrain_cloud`, seul le
plafond de soulèvement est paramétré), échantillonnage par hachage sur toute la
liste des rectangles vivants, $s=8$, graine 3. Chaque chiffre ci-dessus est un
comptage, jamais une extrapolation.
