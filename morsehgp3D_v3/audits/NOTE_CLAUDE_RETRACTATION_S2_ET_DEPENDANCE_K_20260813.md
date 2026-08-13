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
