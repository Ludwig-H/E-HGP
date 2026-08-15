# Note de Claude — le cœur en forme close par boules inscrites, et le plan qui en découle

Date : 15 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=conception_avant_mesure`,
`public_status=not_claimed`. GCP non utilisé.

Fait suite à
[`NOTE_CLAUDE_REPARATION_PREFILTRE_P0_20260815.md`](NOTE_CLAUDE_REPARATION_PREFILTRE_P0_20260815.md).
Cette note **change de primitive** et réordonne le plan. Rien n'y est mesuré :
c'est une spécification de solution avant implémentation, à recevoir comme
telle.

## 1. Le reproche, et il porte plus loin que « la boîte est lâche »

Tout le préfiltre raisonne par AABB : `h_min_over_boxes`, `xi_max_over_box`,
`corner64`. Or la boîte n'est pas seulement une relaxation moins serrée que la
boule englobante — elle m'a fait résoudre **le mauvais problème**. Avec une
boîte, la seule question qu'on sait poser est « ce site est-il témoin », donc on
visite les sites. Le problème réel est « combien de témoins », c'est-à-dire un
**comptage dans une région**, qui est précisément ce qu'un index spatial sait
faire sans énumérer.

Le passage aux boules ne rend pas la borne meilleure partout — la section 6
montre que les deux certificats sont incomparables. Il rend la **question**
bonne.

## 2. Les trois fuseaux sont des lieux angulaires

Avec `e = z-a`, `t = b-z`, `H = e.t` et `Xi = |e x t|^2 = |e|^2 |t|^2 - H^2` :

- `W_2 : H > 0` équivaut à `cos(e,t) > 0`, soit `angle(e,t) < 90°` ;
- `W_3 : 3H^2 > Xi` équivaut à `4H^2 > |e|^2 |t|^2`, donc sous `H > 0` à
  `cos(e,t) > 1/2`, soit `angle(e,t) < 60°` ;
- `W_4 : 2H^2 > Xi` équivaut à `3H^2 > |e|^2 |t|^2`, donc à
  `cos(e,t) > 1/sqrt(3)`, soit `angle(e,t) < 54,7356°`.

L'angle du triangle en `z` est celui entre `a-z = -e` et `b-z = t`, soit
`180° - angle(e,t)`. Donc, en posant `theta_q` le seuil :

`W_q(a,b) = { z : angle(a,z,b) > theta_q }`, avec
`theta_2 = 90°`, `theta_3 = 120°`, `theta_4 = 125,264°`.

C'est le fuseau classique. Sa section méridienne est une lentille — intersection
de deux disques dont les cercles passent par `a` et `b` — et le solide est
obtenu par rotation autour de `(ab)`.

## 3. La boule inscrite est tangente, donc optimale

Soit `L = |ab|` et `m` le milieu de `[a,b]`. Un point à distance perpendiculaire
`rho` de `m` voit `[a,b]` sous l'angle `2 arctan((L/2)/rho)`. L'égaler à
`theta_q` donne le rayon équatorial du fuseau :

`rho = (L/2) cot(theta_q / 2) = kappa_q L`, avec

| lane | `theta_q` | `kappa_q` exact | valeur |
| --- | ---: | --- | ---: |
| q2 | `90°` | `1/2` | `0,50000` |
| q3 | `120°` | `1/(2 sqrt(3)) = sqrt(3)/6` | `0,28868` |
| q4 | `125,264°` | `(sqrt(6) - sqrt(2))/4 = sin(15°)` | `0,25882` |

La valeur q4 mérite d'être écrite exactement : avec `phi = arccos(1/sqrt(3))`,
`kappa_4 = (1/2) tan(phi/2) = (sqrt(3)-1)/(2 sqrt(2))`, qui vaut `sin(15°)`
après rationalisation.

**La boule `B(m, kappa_q L)` est incluse dans `W_q(a,b)`, et tangente
intérieurement.** Preuve : dans le plan méridien, la lentille est
`D(k) inter D(-k)` avec `R = L/(2 sin theta_q)` et `k = sqrt(R^2 - L^2/4)` ; un
point à distance `kappa_q L` de l'origine est à distance au plus
`kappa_q L + k` du centre `(0,-k)`, et le calcul donne exactement `R`. La boule
et la lentille sont donc tangentes, et l'inclusion passe à la rotation puisque
les deux solides sont de révolution autour de `(ab)`. Aucune boule centrée en
`m` plus grande ne convient : le rayon est **optimal**.

## 4. Le cœur d'un rectangle, en une soustraction

Soit `A` de boule englobante `(c_A, r_A)`, `B` de `(c_B, r_B)`, et
`d = |c_B - c_A|`. Pour tout `a` dans `A` et `b` dans `B` :

- le milieu `m_ab` s'écarte de `m = (c_A + c_B)/2` d'au plus `(r_A + r_B)/2` ;
- la longueur vérifie `|ab| >= d - r_A - r_B`.

D'où, par la section 3 :

**`B(m, R_q)` est incluse dans `W_q(a,b)` pour TOUT `(a,b)` de `A x B`, avec
`R_q = kappa_q (d - r_A - r_B) - (r_A + r_B)/2`.**

Une soustraction. Aucun coin, aucune énumération, aucun `i128` : les quantités
sont des distances. C'est le cœur commun du rectangle, en forme close.

## 5. Ce que la forme close explique déjà

Avec la séparation WSPD `d >= s max(r_A, r_B)` et `r = r_A + r_B <= 2 max`, le
pire cas `r_A = r_B` donne `d >= s r / 2`, donc

`R_q / r >= kappa_q (s/2 - 1) - 1/2`.

Le cœur est donc non vide **si et seulement si** `s > 2 + 1/kappa_q` :

| lane | `s` minimal | `R_q/r` à `s=6` | à `s=8` | à `s=10` |
| --- | ---: | ---: | ---: | ---: |
| q2 | `4,000` | `0,500` | `1,000` | `1,500` |
| q3 | `5,464` | `0,077` | `0,366` | `0,655` |
| q4 | `5,864` | `0,018` | `0,276` | `0,535` |

À `s = 6` le cœur q4 vient tout juste de naître — `0,018 r` — alors qu'à `s = 8`
il vaut quinze fois plus. **C'est l'explication du fait 3 du reçu**
`prefiltre_combine_20260815`, que je n'avais su que constater : le facteur six
sur le résiduel entre `s=6` et `s=8`, et le fait que l'écart soit maximal en q4.
La forme close le prédit sans balayage, et donne le modèle de l'arbitrage :
rayon du cœur linéaire en `s`, nombre de rectangles en `s^3`.

Deux réserves. Le tableau est un **pire cas** (`r_A = r_B`) ; un rectangle
déséquilibré fait mieux. Et surtout, `sphere_of(box)` prend la sphère
**circonscrite à l'AABB**, de rayon `sqrt(3)/2` fois le côté, là où la boule
englobante minimale des points du nœud serait souvent bien plus petite. Ce
`r` trop grand est de la marge laissée sur la table, et il entre linéairement
dans `R_q`.

## 6. Boule et boîte ne se dominent pas

Le cœur-boule relaxe les nœuds par leurs boules englobantes ; `corner64` les
relaxe par leurs AABB. Une AABB n'est incluse ni dans la boule englobante ni
l'inverse : **les deux certificats sont incomparables**, et aucun ne remplace
l'autre. Le montage correct les compose — la boule pour l'élagage et les crédits
en bloc, `corner64` sur les feuilles de la frontière. La fermeture est alors
supérieure ou égale à `corner64` seul, à une fraction du coût.

C'est aussi ce qui interdit de retirer `corner64` : ce serait échanger une
fermeture contre une autre sans preuve d'inclusion.

## 7. Ce que ça change algorithmiquement

`h_coeur` devient : **compter les points de `B(m, R_q)`, hors `A` et hors `B`,
plafonné à `h_q`**. Comme `h_q = s_max - q + 1` ne dépasse jamais une dizaine,
c'est une requête `k` plus proches voisins avec `k = h_q` :

> l'ancre meurt pour la lane `q` si et seulement si le `h_q`-ième plus proche
> voisin de `m` est à distance au plus `R_q`.

Une descente d'index, pas soixante-quatre prédicats par site.

`h_a` suit le même schéma : à `a` fixé, `b` parcourant `(c_B, r_B)`, le cœur est
encore une boule en forme close. **Le self-join `O(|A|^2)` devient `|A|`
requêtes de boule.** C'est la réponse à Q23 de la note précédente, et elle prend
la seconde branche que j'y décrivais — le filtre est inchangé, seul le coût
baisse — donc sans perdre la granularité par point dont dépend l'histogramme.

## 8. Faut-il un kd-tree ?

Pas encore, et pour une raison précise : **la structure est déjà là et je m'en
servais mal**. L'octree Morton porte des intervalles `[first, last]`, donc le
compte d'un sous-arbre est en `O(1)`. Ce qui manque n'est pas l'arbre, ce sont
deux primitives sphère–boîte :

- « boîte entièrement dans la boule » — crédit en bloc **exact**, à la place de
  mon test aux coins, et sans le double crédit qui a causé le P0 puisque la
  décision porte sur des populations disjointes par construction de l'octree ;
- « boîte disjointe de la boule » — élagage.

Le coût devient celui de la **surface** de la sphère dans l'arbre, pas de son
volume, avec arrêt dès `h_q` atteint.

Un kd-tree ne se justifierait que si l'octree se révélait mauvais sur
`eight_clusters` — la famille dure, celle dont les cellules atteignent `492`
points quand `uniform` plafonne à `146`. C'est là qu'il faudrait comparer, avec
une mesure appariée, jamais par principe. Ajouter un second index sans ce
chiffre serait exactement le genre de décision que le dossier reproche ailleurs.

## 9. Plan révisé

1. **Cœur-boule en forme close** plus les deux primitives sphère–boîte sur
   l'octree existant. Jugé par le harnais apparié déjà en place, qui rend
   `gagne / perd / faux` contre `corner64` et contre la force brute. Attendu :
   `perd != 0` dans les deux sens, puisque les certificats sont incomparables —
   ce n'est pas un défaut, c'est ce qui motive la composition.
2. **`h_a` et `h_b` par requêtes de boule**, ce qui clôt Q23.
3. **L'écart au vrai vivant à grande taille**, par échantillonnage d'ancres
   décidées exactement en `O(n)` chacune. Inchangé, et toujours la mesure qui
   décide s'il faut continuer à serrer ou passer à autre chose.
4. **Raffinement adaptatif des rectangles non décidés** — bien plus naturel avec
   des boules, `R_q` croissant explicitement quand les nœuds rétrécissent.
5. **P1**, la dette : graver les quatre contre-exemples q3/q4 de la section 6 de
   l'audit, l'échange `A`/`B`, la coquille stricte, les positions dupliquées ;
   corriger le sens de l'implication `W_q`.
6. **Une seule régénération de campagne**, après tout le reste.

Sur Q25 je tranche sans attendre : « arête maximale owner » partout, « paire
diamétrale » conservé au seul contexte q2, avec la note que les deux notions y
coïncident. Sur Q24 je garde la voie rapide comme pure optimisation de parcours,
doublée d'une porte vérifiant l'égalité des comptes avec et sans elle — l'option
falsifiable, celle que je défendais.

## 10. Ce que cette note ne revendique pas

Aucune mesure. Aucune promotion de statut : le préfiltre reste `counter-only` et
ferme des **ancres**, pas des supports. La section 3 est une preuve
géométrique — l'inclusion et la tangence — mais elle ne devient un fait du
dossier qu'avec sa fixture gravée et sa porte à code, qui n'existent pas encore
au moment d'écrire. Le point 1 du plan les produit avant toute mesure.
