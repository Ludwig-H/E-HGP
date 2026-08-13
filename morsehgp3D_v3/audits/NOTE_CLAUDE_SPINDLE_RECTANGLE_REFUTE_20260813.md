# Note de Claude — le cœur anisotrope ne se factorise pas sur un rectangle

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=proposition_math_non_recue`,
`public_status=not_claimed`. GCP non utilisé.

Résultat négatif, mesuré. Il réfute le levier numéro un de mon étape 1.

## 1. Ce que j'attendais

Le certificat central en production teste `209 V^2 \le 56 D^2`, qui est la boule
**inscrite** dans le vrai cœur : la réduction supprime le terme favorable
`-4 (d\cdot v)^2`, maximal sur l'axe de l'arête. Mesuré **par paire** sur
`eight_clusters`, le cœur exact porte `5,6` fois plus de témoins, fait tomber la
fraction de cœur vide de `13,8 %` à `0,9 %` et rend `95 %` du résiduel fermable.

Votre `§6` donnait le passage au rectangle : l'identité
`T = d\cdot v = \lVert z-a\rVert^2 - \lVert z-b\rVert^2`, séparable par axe,
avec les candidats exacts par extrémités, ruptures et milieux. Je l'ai
implémentée dans `rect_front.hpp` — `rect_t_axis`, `rect_t_abs`,
`rect_spindle_all` — et vérifiée contre une énumération exhaustive de
`A \times B \times C` sur trois mille triplets de boîtes, zéro désaccord.

Le certificat sûr est alors, en disjonction avec le central :

```text
Dlo > Vhi   et   (Dlo-Vhi)^2 > 2 (Dhi Vhi - Tabs^2),
```

avec `Tabs = 0` si l'intervalle de `T` enjambe zéro.

## 2. Ce que la mesure donne

`n=6 000`, `s=8`, boîte serrée, budget `512`, masse q4 ouverte :

| famille | central seul | central `OU` spindle |
| :--- | ---: | ---: |
| `uniform` | `13,303 %` | `13,278 %` |
| `terrain` | `7,920 %` | `7,906 %` |
| `eight_clusters` | `84,102 %` | `84,085 %` |

**Le gain est nul.** Trois centièmes de point sur la famille où le certificat
par paire gagnait un facteur `5,6`.

## 3. Le mécanisme, mesuré et non supposé

Ma première hypothèse était que l'intervalle de `T` enjambe presque toujours
zéro, donc que `Tabs` serait presque toujours nul. **Elle est fausse** :

| famille | essais du spindle | `Tabs > 0` | `ALL` rendus |
| :--- | ---: | ---: | ---: |
| `uniform` | `260 153 528` | `77 161 036` (`29,7 %`) | `318 961` (`0,12 %`) |
| `eight_clusters` | `88 692 954` | `26 738 956` (`30,1 %`) | `113 470` (`0,13 %`) |

Le terme directionnel est donc disponible dans près d'un tiers des blocs, et il
ne suffit pourtant jamais. La raison est celle que vous aviez écrite et que je
n'avais pas prise au sérieux : **`D`, `V` et `T` sont corrélés, et leurs extrema
séparés perdent le rappel.** `Tabs` est le **minimum** de `\lvert T\rvert` sur
tout le bloc, alors que le gain par paire vient des sites où `\lvert T\rvert`
est **maximal** — ceux de l'axe. Un bloc qui contient un site axial contient
aussi des sites hors axe, et c'est le pire d'entre eux qui fixe `Tabs`.

Autrement dit : le cœur anisotrope est fortement **anisotrope**, et une boîte
alignée ne peut pas en capturer la partie utile sans capturer aussi la partie
qui la détruit. Ce n'est pas un défaut de la borne d'intervalle — elle est
exacte, je l'ai vérifiée exhaustivement — c'est que la question « quel est le
minimum de `\lvert T\rvert` sur ce bloc » n'est pas celle qui décide.

## 4. Ce que cela change au plan

Le levier numéro un de mon étape 1 est **réfuté comme certificat de rectangle**.
Il reste vrai par paire, et donc il reste vrai à granularité assez fine : la
seule façon de le récupérer est de rétrécir les boîtes jusqu'à ce que
l'intervalle de `T` cesse d'être dominé par sa propre largeur. C'est exactement
le **raffinement local** que vous prescriviez, et ma mesure lui donne maintenant
sa raison d'être — ce n'est pas une optimisation de confort, c'est la seule voie
qui reste vers le cœur exact.

L'ordre de l'étape 1 devient donc :

1. raffinement local des seuls terminaux ouverts, avec tâches, octets et HWM
   bloquants ;
2. le spindle **à l'intérieur** du raffinement, là où les boîtes sont assez
   petites pour que `Tabs` cesse d'être le minimum d'un large intervalle ;
3. `s` par lane, choisi par coût composé.

Je conserve `rect_t_axis` et `rect_spindle_all` : ils sont exacts, gardés par
leur fixture, et ils redeviendront utiles à granularité fine. Ce que je retire,
c'est le claim qu'ils réparent le rectangle tel quel.

## 5. Non-claims

Une graine, `n=6 000`, trois familles, un seul `s`. La disjonction est **sûre**
— deux certificats suffisants restent suffisants — donc ce résultat ne peut pas
avoir perdu de support ; il ne peut avoir que sur-estimé le gain, et il l'estime
à zéro. Aucun raffinement local n'est encore implémenté, donc le point 4 est une
direction, pas une mesure.
