# Note de Claude — je retire ma réfutation de `s=2`, et je mesure `s` contre `K`

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=proposition_math_non_recue`,
`public_status=not_claimed`. GCP non utilisé pour cette note.

## 1. Rétractation — mon refus de `s=2` portait sur le certificat, pas sur `s`

J'ai publié que `s=2` était réfuté par la fenêtre — pentes `1,423 / 1,686` — et
que `s=3` devait devenir la baseline **contre** votre recommandation. C'était
faux, et la cause est entièrement de mon côté : je mesurais la fenêtre avec le
**masque central seul**, qui est suffisant mais très incomplet.

Le masque central et le repli `H_{\min} / E_2^{\max}X_2^{\max}` sont deux
certificats suffisants **non comparables** ; leur disjonction reste suffisante.
Avec les deux, `uniform`, boîtes serrées :

| `s` | masque central seul | **disjonction des deux** |
| ---: | :---: | :---: |
| `2` | `1,423 / 1,686` — REFUSÉ | **`1,272 / 1,006`** — VERT |
| `3` | `1,012 / 1,134` | `1,015 / 1,273` |

Et la fenêtre moyenne à `s=2` passe de `2 003 / 2 685 / 4 319` à
`814 / 982 / 986` — elle **sature** au lieu de croître. La masse q2 fermée y
monte à `79,65 / 87,72 / 93,84 %`.

**Votre baseline `s=2` tient.** Je retire ma correction, et je note contre moi
que j'ai contredit une recommandation sur la foi d'une mesure faite avec un
certificat amputé — exactement le défaut que vous m'aviez signalé quand je
choisissais `s` sur la seule lane q2.

## 2. La dépendance en `K`, mesurée et non modélisée

Vous aviez réfuté ma loi volumique : sans hypothèse de densité, aucun rapport de
volumes ne relie les populations, donc il n'existe ni loi `K\lambda(s)` ni loi
`s(K)`. J'ai donc dérivé les seuils de `smax` — `h_q = smax+1-q` — et je mesure.

Premiers points, `s=2`, disjonction, fenêtre moyenne à `n = 4 000 / 8 000 / 16 000` :

| `smax` | `h_2` | moyennes | pentes |
| ---: | ---: | --- | :---: |
| `7` | `6` | `534,1 / 589,9 / 602,3` | `1,143 / 1,030` |
| `11` | `10` | `813,8 / 982,4 / 986,2` | `1,272 / 1,006` |

Le rapport des seuils vaut `10/6 = 1,67` ; celui des fenêtres asymptotiques
`986,2/602,3 = 1,64`. **La fenêtre semble proportionnelle à `K`, et sa pente en
`n` reste voisine de un dans les deux cas.**

Si cela se confirme sur la grille complète, la réponse à la question posée est
nette et rassurante : **`s` n'a pas besoin de dépendre de `K`**. Seule la
constante de la fenêtre y est proportionnelle, et l'exposant en `n` ne bouge pas.
Autrement dit `\lvert N_q(a)\rvert = \Theta(K)` à `s` fixé, donc
`\sum_a\lvert N_q(a)\rvert = \Theta(Kn)`.

Je publie ce résultat comme **partiel** : deux valeurs de `smax`, une famille,
trois tailles. La grille `smax \in \lbrace 7,11,15,19,25\rbrace` fois
`s \in \lbrace 2,3,4\rbrace` tourne.

## 3. Ce que cela changerait, si cela tient

Le contrat fixe `smax=11`. Si la fenêtre est `\Theta(K)`, alors :

- augmenter `smax` coûte **linéairement**, pas cubiquement — c'est une bonne
  nouvelle pour les profils plus riches ;
- le choix de `s` se fait **indépendamment** de `smax`, donc une seule ablation
  suffit et elle vaut pour tous les profils ;
- et la borne `\Theta(Kn)` de la fenêtre devient la borne naturelle du travail
  de la source, puisque c'est elle que le moteur consomme comme `kept`.

## 4. Deux questions

1. Confirmez-vous que la **disjonction** de deux certificats suffisants est
   recevable telle quelle — je ne vois pas d'obstacle, chacun étant fail-open et
   aucun ne prononçant `NONE` sur la foi de l'autre, mais c'est exactement le
   genre de raisonnement où vous m'avez déjà repris.
2. `\Theta(K)` est mesuré sur `uniform`. Sur `eight_clusters`, où la densité est
   très inhomogène, attendez-vous la même proportionnalité, ou faut-il la
   mesurer avant d'en faire une règle ?

## 5. Non-claims

Aucun temps, aucun octet. La grille `s \times smax` est partielle. Le résultat
`\Theta(K)` est une lecture de deux points et n'est pas une loi. Le contrat
`50 000` reste entièrement ouvert et G4 reste NO-GO.

## 6. Correction — mon « recoupement » avec le `kept` du moteur était trop affirmatif

J'ai écrit que le `kept` du moteur — `446` puis `474` — et ma fenêtre WSPD
« concordent », et que c'était « la vérification croisée qui me manquait ».
C'était trop fort, et la grille le montre.

Ma fenêtre dépend de **deux** paramètres que je n'avais pas fixés en annonçant
la concordance :

| configuration | fenêtre asymptotique |
| --- | ---: |
| `s=3`, masque central seul | `528,6` |
| `s=3`, disjonction | `354,5` |
| `s=2`, disjonction | `986,2` |
| `kept` du moteur (sa propre construction) | `446` à `474` |

Le `kept` du moteur tombe **entre** mes valeurs à `s=2` et `s=3`, et le chiffre
que j'avais cité — `477,6 / 481,6` — était celui d'une configuration précise
que je n'avais pas nommée. Le bon énoncé est donc : **les deux quantités sont
du même ordre de grandeur**, ce qui est déjà instructif puisqu'elles viennent de
deux constructions sans primitive commune ; ce n'est **pas** une concordance
numérique, et je n'aurais pas dû l'appeler ainsi.

Ce qui reste vrai et utile : les deux saturent, et aucune des deux ne croît avec
`n`. C'est cela qui fait passer votre critère, pas leur égalité.

## 7. La grille, en cours

`uniform`, disjonction, fenêtre moyenne à `n = 4 000 / 8 000 / 16 000` :

| `smax` | `s` | moyennes | pentes |
| ---: | ---: | --- | :---: |
| `7` | `2` | `534,1 / 589,9 / 602,3` | `1,143 / 1,030` |
| `7` | `3` | `157,2 / 171,9 / 198,2` | `1,129 / 1,206` |
| `7` | `4` | `93,8 / 103,0 / 103,3` | `1,135 / 1,004` |
| `11` | `2` | `813,8 / 982,4 / 986,2` | `1,272 / 1,006` |

**Aucune pente ne refuse, et la fenêtre sature dans les six cas.** La
proportionnalité en `K` se vérifie aussi à `s=3` : `198,2` à `smax=7` prédit
`331` à `smax=11`, et la mesure indépendante donne `354,5` — sept pour cent
d'écart.
