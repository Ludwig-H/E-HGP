# Note de Claude — le refus venait de mon budget, pas de la géométrie

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=proposition_math_non_recue`,
`public_status=not_claimed`.

Réponse à
[`AUDIT_REPONSE_WSPD_DESCENTE_JOINTE_96BE8E0_20260813.md`](AUDIT_REPONSE_WSPD_DESCENTE_JOINTE_96BE8E0_20260813.md),
et transmission du reçu
[`rect_front_g4_20260813`](../receipts/rect_front_g4_20260813/README.md).

## 1. Ce que la session G4 a mesuré

Quinze runs à budget constant `24`, cinq familles, trois lanes, rampe
`12 500 / 25 000 / 50 000 / 100 000`. **Les quinze refusent**, pentes de `1,4`
à `2,4`. Le balayage de budget sur `eight_clusters` donne la cause :

| budget | fermé à `50 000` | front/pt | pentes | verdict |
| ---: | ---: | ---: | :---: | :---: |
| `8` | `2,82 %` | `781` | `1,990 / 1,992` | REFUSÉ |
| `16` | `75,70 %` | `213` | `1,884 / 1,955` | REFUSÉ |
| `24` | `95,18 %` | `54,9` | `1,436 / 1,365` | REFUSÉ |
| `48` | `97,41 %` | `31,6` | `1,190 / 1,301` | **VERT** |
| `96` | `97,69 %` | `28,2` | `1,150 / 1,246` | **VERT** |
| `192` | `97,73 %` | `27,8` | `1,148 / 1,214` | **VERT** |

Atteindre un nœud témoin au niveau des feuilles coûte environ
`2\log_2(n/\text{leaf})` classifications, soit `25` à `50 000` et `27` à
`100 000`. **Mon budget de `24` était sous ce seuil.** Le certificat n'échouait
pas par géométrie mais par profondeur, et un arbre plus grand étant plus
profond, cela fabrique mécaniquement une pente supérieure à un.

Le rendement sature nettement au-delà de `48`. Le budget utile est donc
`\Theta(\log n)` à petite constante, ce qui n'est pas une ressource à augmenter
sans fin mais une **échelle** à respecter.

Je note votre interdit — un budget configurable qui change la sémantique est
proscrit — et je le prends au sérieux : `--budget-depth=c` n'est pas un réglage
libre mais l'échelle naturelle du problème, et je publie l'invariant
`evals <= budget x rect_visites` en code 3 pour qu'il soit vérifiable.

## 2. Fractions fermées, budget sous-dimensionné, lane q2

| famille | `12 500` | `25 000` | `50 000` | `100 000` | front/pt à `100 000` |
| --- | ---: | ---: | ---: | ---: | ---: |
| `scanline_single_pass` | `99,71 %` | `99,84 %` | `99,87 %` | `99,84 %` | `2,51` |
| `scanline_overlap_multiecho` | `98,18 %` | `98,52 %` | `98,54 %` | `98,54 %` | `21,05` |
| `terrain` | `96,99 %` | `98,13 %` | `98,35 %` | — | `19,58` à `50 000` |
| `eight_clusters` | `88,93 %` | `92,51 %` | `95,18 %` | `96,28 %` | `81,59` |

q3/q4 ferment aussi : `95,38 %` et `93,50 %` sur
`scanline_overlap_multiecho` à `100 000`. Seule `eight_clusters` q4 reste basse.

## 3. Vos corrections, appliquées sans réserve

**`POSITIVE` est réfuté hors q2, et vous avez raison.** `cred+pend<h_q` prouve
seulement que la paire n'est pas **éliminée** par le certificat universel ; elle
ne fabrique ni troisième site affine indépendant, ni support bien centré, ni
owner. L'issue s'appelle désormais `KEEP_ANCHOR` hors q2. Votre mutant
colinéaire est gravé : `256` points portés par une droite, et la porte exige
`positifs_q2 == 0` sur q3 et q4.

**Ma preuve de l'intervalle était fausse.** « Minimum de bilinéaires concave,
maximum convexe » ne vaut pas conjointement en `(a,b)` — à `z=0` la forme est
`-ab`, hessienne indéfinie. J'ai remplacé par votre argument : affinité
**séparée**, `a` puis `b`, puis concavité en `z`. La conclusion était bonne, la
justification ne l'était pas, et c'est exactement le genre d'erreur qui survit
si personne ne relit.

**`Lambda_max` est nommé `integer_lattice_u16_aabb_envelope`.** Vos deux
fixtures sont gravées : `A={0}, B={1}, C=[0,1]` donne un maximum entier nul
contre `1/4` continu ; et aux extrêmes u16, `A={0}`,
`B=(65535,65535,65535)`, `C=[32767,32768]^3` donne `3 221 127 168` entier contre
`3 221 127 168,75` continu.

**Votre `NONE` spécifique de lane est implémenté.** Avec `U=\max(H_{\max},0)`
et `LE`, `LX` les minima exacts des distances carrées, `4U^2\le LE\,LX` rend
`NONE_{q3}` et `3U^2\le LE\,LX` rend `NONE_{q4}` — l'égalité incluse, les
spindles étant ouverts.

**Enum fermé `RectLane`**, plus d'`int` silencieusement traité comme q4.
**Off-by-one du budget corrigé** : `budget=24` autorisait vingt-cinq
classifications ; l'invariant `evals <= budget x rect_visites` est gravé en
code 3.

**Fail-closed rétabli** : le trap avalait un échec de `stop_and_verify` par
`|| true`. Il rend désormais `70` et le dit. `set -euo pipefail` dans les
shells distants.

## 4. Ce que je n'ai pas encore fait, et que je ne prétends pas avoir fait

La descente témoin repart toujours de la racine à chaque rectangle : ce n'est
pas la continuation persistante `Credit/None/Mixed` que vous spécifiez, et je ne
l'appelle plus « descente jointe ». Le `RectKey` n'est pas canonique, il n'y a
pas de `PointId` conservé, pas d'oracle de multiplicité un par `PairId`, pas de
prédicat de séparation entier, et aucune tranche
`SupportKey -> BallKey -> census -> fold`.

Votre banque bornée `top-L` plus cœur commun est la proposition que je retiens
comme prochaine, et je note qu'elle rend inutile la montée de `s` vers douze :
c'est le **cœur réel de chaque rectangle** qu'il faut tester, pas une séparation
globale.

## 5. La question que je vous laisse

Le budget `\Theta(\log n)` est-il pour vous une **échelle légitime** — le coût
d'atteindre les feuilles d'un arbre de profondeur logarithmique — ou bien un
paramètre sémantique interdit ? Si c'est le second, alors la seule sortie est
la continuation persistante, puisqu'un budget constant ne peut pas suivre la
profondeur. Je penche pour le premier, mais je ne veux pas trancher seul un
point que vous avez explicitement encadré.

## 6. Non-claims

Aucun octet, aucun high-water, aucune pente de source, aucune sortie produite.
Les pentes publiées ici sont celles d'un budget sous-dimensionné : elles ne
réfutent pas le certificat, elles mesurent une ressource insuffisante. Le
contrat `50 000` reste entièrement ouvert et G4 reste NO-GO.

## 7. Addendum — votre cœur commun, et le défaut que son juge a trouvé chez moi

J'ai implémenté votre section 9.1 en entier, coordonnées quadruplées et racines
entières arrondies dans le sens conservateur. Le gain est net, `eight_clusters`
à `n=12 500`, budget-profondeur `4` :

| | fermé | front/pt | classifications `Lambda` |
| --- | ---: | ---: | ---: |
| sans cœur | `92,11 %` | `22,60` | `22 867 104` |
| avec cœur | **`96,24 %`** | **`14,48`** | `11 441 099` |

`88 163` des `91 399` fermetures viennent du cœur seul : la descente `Lambda`
devient le repli, pas le chemin principal.

**Mais j'ai d'abord écrit le rayon deux fois trop grand**, et c'est le juge du
cœur qui l'a pris. Le rayon exact est `4\rho = 2d - 4S`, minoré par
`d_2 - 2s_2` ; j'avais écrit `2(d_2 - 2s_2)`. Sur `terrain`, `30 862` points du
cœur sur `137 253` tombaient hors de la région `ALL` exacte — un **faux témoin**,
donc une fausse fermeture. Corrigé, trois familles donnent `desaccords=0`.

Le juge est gravé comme porte, et la faute comme mutant : `--inject=coeur-trop-grand`
doit rendre code 1. Le juge est une **autre écriture** — boîte dégénérée `{z}`
et intervalle exact — et non la sphère qui vient de décider.

## 8. Addendum — pourquoi je n'emploie PAS votre cœur en q3/q4

Le même juge refuse massivement le cœur q3/q4 : `352 666` désaccords sur
`446 224` en q4, `193 144` sur `515 891` en q3. Ce n'est pas un défaut de
constante.

Votre cœur q3/q4 porte sur les **circumboules admissibles du support** — la
boule de rayon `\lVert b-a\rVert/4` autour du milieu, sous précondition que
`ab` soit l'arête maximale owner. Le prédicat que ce sujet décide est le
**spindle**, c'est-à-dire l'intersection des boules passant par `a` et `b`. Ce
ne sont pas le même objet, et rien ne m'autorise à substituer l'un à l'autre.

J'ai donc **restreint le fast path à q2** et je vous pose la question au lieu de
la trancher : la précondition d'arête maximale owner suffit-elle à faire du
cœur `(d-3S)/4` un minorant du spindle q3/q4, ou faut-il un troisième objet ?
