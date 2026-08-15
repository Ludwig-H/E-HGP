# Note de Claude — la borne couplée encodée, et sa saturation gravée

Date : 15 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=diagnostic_counter_only`,
`public_status=not_claimed`. GCP non utilisé.

Point P1.7 de
[`AUDIT_REAUDIT_PREFILTRE_COMBINE_COEUR_BOULE_41DFD2C_20260815.md`](AUDIT_REAUDIT_PREFILTRE_COMBINE_COEUR_BOULE_41DFD2C_20260815.md).
Votre borne couplée est juste, je l'ai vérifiée avant de l'encoder, et elle est
en place. Deux choses méritent d'être dites : ce qu'elle apporte réellement, et
ce que j'avais surestimé.

## 1. Elle tient, et la vérification a porté sur les trois points sensibles

L'identité `|p|^2 + |w|^2 = (|u|^2 + |v|^2)/2` est exacte, sans hypothèse
d'orthogonalité. Le point qui pouvait clocher — les deux termes sont-ils liés
par la même contrainte ? — est bien traité : ils le sont, par **une seule**
contrainte quadratique, donc Cauchy s'applique sans relâchement illégitime.
Relâcher l'ensemble atteignable des couples `(|w|,|p|)` au quart de disque ne
peut que sur-estimer la pénalité, donc sous-estimer le rayon : le sens de
l'erreur est conservateur.

Elle se garde aussi toute seule : `(r_A+r_B)^2 <= 2(r_A^2+r_B^2)` donne
`R_coup <= 0` dès que `d <= r_A + r_B`. Aucun piège de signe comme celui que
vous aviez trouvé dans ma boule d'apex.

## 2. La forme entière, et la saturation

En unités doublées, avec `S = rA2^2 + rB2^2` :

`R4_coup = 2 kappa_q d2 - sqrt(2(4 kappa_q^2 + 1) S)`,

et le facteur sous la racine vaut **exactement** `4` en q2, `8/3` en q3 et
`6 - 2 sqrt(3) = 2,5358983849` en q4. Arrondis suivant vos directions : le
terme positif sous-approché deux fois, la racine soustraite sur-approchée, et
`ceil_sqrt` au **vrai** plafond, pas `isqrt + 1`.

**La saturation est gravée, pas affirmée.** La fixture `couple-sature`
construit l'adversaire où Cauchy est une égalité — `w` antiparallèle à l'axe,
`p` orthogonale, dans le rapport `(2 kappa, 1)` — en q2 où `2 kappa = 1` rend
l'arithmétique entière :

```text
c_A = (2000,5000,5000)   c_B = (8000,5000,5000)   d = 6000
u = (700,700,0)   v = (-700,700,0)   donc |w| = |p| = 700
a = (2700,5700,5000)   b = (7300,5700,5000)   |ab| = 4600
m = (5000,5000,5000)   m_ab = (5000,5700,5000)
```

`R_coup = 3000 - 700 sqrt(2) sqrt(2) = 1600` exactement, et le point
`z = (5000,3400,5000)` est à `1600` de `m` et `2300` de `m_ab`, donc **sur** la
sphère diamétrale : `H = 0`, hors du fuseau ouvert. L'implémentation rend
`1599,75` et le refuse ; le mutant qui sous-estime le facteur rend `1617,5`,
l'accepte, et meurt au code 1. C'est une porte de **sûreté**, pas de seuil.

Les bascules réelles de l'implémentation sont `L2 = 830 / 2003 / 2355` contre
`828,43 / 2000,00 / 2350,66` en continu : les arrondis dirigés coûtent deux
dixièmes de pour cent. Elles sont gravées telles quelles — c'est le code que la
fixture juge, pas la géométrie.

## 3. Ce qu'elle apporte, et ce que j'avais surestimé

Au régime équilibré et **au seuil**, le gain est spectaculaire : à `s=6` en q4,
`R_coup = 0,944 rho` contre `R_dec = 0,553 rho`, soit `+71 %` de rayon. La
couplée naît d'ailleurs à `39 %` de la séparation qu'exige la décorrélée
(`L2 = 2355` contre `3866`).

**En régime, c'est beaucoup moins.** Témoins du cœur trouvés, lane q4,
`n=400` :

| famille | `s` | `R_dec` | `max(R_dec,R_coup)` | facteur |
| --- | ---: | ---: | ---: | ---: |
| `terrain` | `6` | `50 647` | `61 454` | `1,21` |
| `terrain` | `8` | `110 316` | `124 142` | `1,12` |
| `eight_clusters` | `6` | `7 575` | `8 120` | `1,07` |
| `eight_clusters` | `8` | `16 557` | `17 716` | `1,07` |
| `uniform` | `6` | `104 716` | `113 673` | `1,08` |
| `uniform` | `8` | `215 908` | `225 463` | `1,04` |

La raison est arithmétique et j'aurais dû la voir avant d'annoncer un facteur
cinq en volume : l'écart `R_coup - R_dec` vaut `kappa r + r/2 - rho sqrt(4
kappa^2+1)`, donc il est en `O(r)` et non en `O(d)`. Il ne dépend pas de la
séparation, alors que les deux rayons croissent avec elle : le gain **relatif**
s'estompe dès qu'on dépasse le seuil. Mon `x5` valait au seuil, pas en régime.

Cela reste `+4` à `+21 %` de témoins pour une soustraction de plus, sans
descente supplémentaire, et le maximum des deux est pris lane par lane puisque
aucune ne domine partout.

## 4. Point 11 : les sphères des points ne rendent presque rien

J'ai essayé, parce que vous en faites la condition d'un gain au-delà de
Corner64. Le résultat est négatif, et la raison est mesurable.

**Premier piège, que j'ai commencé par y tomber : deux usages, deux sphères.**
La sphère sert au test de **séparation**, donc à la partition, et au **rayon**
du cœur. Serrer celle de séparation rend la séparation plus facile, donc produit
des rectangles moins séparés, donc des cœurs plus **petits** : mesurée ainsi, la
sphère des points paraissait perdre `3 %`. La partition doit rester sur la
sphère de l'AABB.

**Une fois découplé, le gain est réel mais minuscule** : `+0,1 %` à `+1,4 %` de
témoins, le maximum sur `terrain`. Et la mesure dit pourquoi : à centre fixé,

`rayon serré / rayon AABB = 0,969` (`uniform`, `eight_clusters`), `0,960`
(`terrain`).

La sphère circonscrite à une AABB **serrée** est donc déjà à quatre pour cent du
meilleur rayon possible **à ce centre**. Il n'y avait presque rien à prendre.

**Ce qui reste, et sa borne.** Le levier n'est pas le rayon mais le **centre**.
Un minorant du rayon de la boule englobante minimale — la moitié du diamètre,
estimé par la première passe de Ritter — vaut

`0,848` (`uniform`), `0,849` (`eight_clusters`), `0,806` (`terrain`)

fois le rayon AABB. Une vraie boule englobante minimale ne pourrait donc gagner
que `15` à `19 %` de rayon, dont j'ai déjà `4 %`. Le reste demanderait une MEB
exacte en arithmétique rationnelle, pour un gain de fermeture que la pente
mesurée ici situe autour de `+5 %`. Je ne l'ouvre pas sans que vous jugiez ce
rapport acceptable.

**Conséquence sur l'autorité cône robuste.** Avec `sphere_of(box)` elle est
dominée par l'autorité aux coins, puisque la sphère circonscrit la boîte. Avec
la sphère des points elle cesse de l'être — mais de `4 %` de rayon seulement.
Je ne l'implémente donc pas : elle coûterait trois racines dirigées par couple
pour un gain que la mesure ci-dessus borne très bas.

## 5. Ce qui reste de votre plan

Non fait, et non prétendu : l'autorité cône robuste du § 6.2 — dont je note
qu'avec `sphere_of(box)` elle serait **dominée** par l'autorité aux coins,
puisque la sphère circonscrit la boîte, donc que son intérêt dépend entièrement
du point 11 de votre plan ; les sphères englobant les **points** ; le test fixe
Q30 ; le mode `--no-bulk` ; la porte `direct == tree` pour l'apex.

La campagne n'est pas régénérée et le reçu garde son bandeau q2 invalide.

Suite complète : `831/832`. Le seul échec est `mhgp3v_arith_selftest`, faute
d'en-têtes GMP dans ce conteneur.
