# Réponse Claude — la machinerie de patches vaut 0,4 à 3,7 % : un certificat UNIQUE en capte 92 à 98 % (29 août 2026)

```text
phase=exploration_v5_hors_registre
backend=cpu_reference
profile=quantized_u16_input_only
mode=audit_independant_math_and_architecture
public_status=not_claimed
```

Pin de mesure : `1ff39ab9`, `worktree_modifie=non`, quatre familles, $n = 8000$,
3 000 blocs à pas constant.

## 1. La distinction que votre rétractation avait introduite

En retirant mon `tb` de la v1, vous écriviez : « plusieurs patches peuvent être
tués par neuf ensembles **incompatibles** alors que leur intersection globale
contient moins de neuf sites ». C'est exact, et c'est ce qui justifie toute
votre machinerie de patches — 64 patches par rectangle, antichaînes d'identités
par patch, bornes `L32/U32`, médiatrices `AB/AC/BC`, `CappedWitnessSet` par
source, interdiction d'additionner `parent_count + fresh_count`.

**Cette distinction est mesurable, et je viens de la mesurer.** Sur les blocs du
gain marginal — ceux dont tous les supports sont profonds mais que $W_3$ ne tue
pas, c'est-à-dire exactement la cible du center-cover :

| famille | **certificat UNIQUE suffit** ($\ge h_3$ témoins communs) | **patches nécessaires** |
|---|---|---|
| `scanline` | 684 blocs, 8 648 918 appels — **98,2 %** | 56 blocs, 162 726 — 1,8 % |
| `terrain` | 722 blocs, 2 305 996 appels — **98,9 %** | 37 blocs, 25 577 — 1,1 % |
| `uniform` | 867 blocs, 1 219 444 appels — **96,3 %** | 31 blocs, 47 508 — 3,7 % |
| `eight_clusters` | 728 blocs, 78 819 354 appels — **99,6 %** | 43 blocs, 343 610 — 0,4 % |

**De 96,3 % à 99,6 % du gain marginal est atteignable par un certificat GLOBAL
unique**, c'est-à-dire par une simple intersection de toutes les boules du bloc.
La machinerie de patches ne capte que le reste : **0,4 % à 3,7 %**.

## 2. Ce que cela donne bout à bout

En composant avec le plafond du § précédent — la part du travail de profondeur
que la production paie encore et qui vit dans des blocs entièrement profonds :

| famille | résiduel réellement payé | capté par un certificat UNIQUE | part |
|---|---|---|---|
| `scanline` | 8 962 419 | 8 648 918 | **96,5 %** |
| `terrain` | 2 389 991 | 2 305 996 | **96,5 %** |
| `uniform` | 1 327 216 | 1 219 444 | **91,9 %** |
| `eight_clusters` | 80 115 357 | 78 819 354 | **98,4 %** |

**Un center-cover global, sans patches, plafonne à 92–98 % du travail résiduel.**

## 3. Ce que je propose d'en conclure — et ce que je ne conclus pas

**Ce que je propose :** construire d'abord le **certificat global unique** —
l'intersection des boules du bloc, créditée par témoins avec seuil $h_3$ — et
traiter les patches comme un **raffinement mesuré ensuite**, pas comme le
premier incrément. Votre construction reste juste ; la mesure dit seulement que
sa partie lourde est, dans ces quatre régimes, une optimisation de 0,4 à 3,7 %
et non le mécanisme principal.

Cela rejoint la remarque du § précédent : le seau « inhérent » ne pèse que 0,4 à
2,2 %, donc les blocs **mixtes** sont rares. Un bloc à supports valides est
presque toujours **entièrement** profond, et presque toujours par des témoins
**communs**. Les deux mesures disent la même chose sous deux angles.

**Ce que je ne conclus pas :**

- **Ce n'est pas un certificat de boîtes.** Le « certificat unique » mesuré ici
  connaît les triplets réels du bloc ; un certificat implémentable ne connaît
  que les boîtes et captera strictement moins. Le rapport simple/patches
  pourrait bouger — bien qu'il n'y ait pas de raison qu'il s'inverse, puisqu'un
  certificat de boîtes plus lâche perdra des blocs des deux catégories.
- **Une taille, quatre familles, un échantillon, pas de reçu.** Le rapport
  simple/patches pourrait dépendre de $n$ : à grande échelle, des blocs plus
  gros porteraient des supports plus divers, donc peut-être plus de patches.
  C'est la première chose que je mesurerais avant de figer la conception.
- **Rien ici ne dit qu'un certificat global est bon marché.** Le plafond ne dit
  pas le coût.

## 4. Questions

- **V79.** Acceptez-vous de faire du certificat **global unique** le premier
  incrément, les patches devenant un raffinement conditionné à une mesure ?
  C'est un allègement considérable de la conception que vous avez transmise, et
  je ne le prendrais pas sans votre accord.
- **V80.** Le rapport simple/patches doit-il être vérifié à $n = 16\,000$ et
  $32\,000$ avant de figer, ou la stabilité sur quatre familles très
  différentes (0,4 % à 3,7 %) vous suffit-elle à ce stade ?
- **V81.** Si le certificat global devient le premier incrément, faut-il tout
  de même conserver la nomenclature de fates et le ledger de provenance, qui
  n'ont pas de valeur de gain mais fixent la comptabilité des rôles ? Je
  plaiderais pour oui : c'est ce qui a permis de mesurer tout ce qui précède.
