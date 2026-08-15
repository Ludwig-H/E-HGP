# Note de Claude — le cœur en forme close par boules inscrites, et le plan qui en découle

Date : 15 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=conception_avant_mesure`,
`public_status=not_claimed`. GCP non utilisé.

> [!CAUTION]
> **Ré-audit au `HEAD=66b4f0c` : ne pas prolonger cette note telle quelle.**
> La boule tangentielle doit être **ouverte** ; la convention WSPD effective
> est `d-r_A-r_B >= s max(r_A,r_B)` ; avec `sphere_of(box)` le cœur-boule est
> dominé par Corner64, et le plan ball-only diminue `h_a/h_b`. Les formules
> corrigées, une borne couplée plus forte, l'autorité exacte cône--boule et
> l'auto-jointure qui conserve les comptes ponctuels sont données dans
> [`AUDIT_REAUDIT_PREFILTRE_COMBINE_COEUR_BOULE_41DFD2C_20260815.md`](AUDIT_REAUDIT_PREFILTRE_COMBINE_COEUR_BOULE_41DFD2C_20260815.md).
> La primitive ouverte ajoutée au `6220ea3` est reçue comme sous-certificat ;
> les claims architecturaux de la note ne sont pas une autorité active. En
> particulier, le « non-mutant » rayon arrondi d'une unité vers le haut est
> faux pour une norme de grille irrationnelle ; le chemin nominal par plancher
> reste sûr. Pour ne pas perdre la fraction, employer le test fixe Q30 par
> carrés décrit dans le ré-audit, jamais un plafond entier.
>
> La section Q23 ajoutée au `66b4f0c` n'est pas reçue non plus. Son cône est un
> sous-certificat obtenu par `|e|<=2r_A`, pas la région exacte de `h_a` ;
> l'auto-jointure reste quadratique au pire cas. Surtout, `apex_sin2` met le
> sinus au carré sans vérifier `gamma_q>0` : à `separation=1`, une fixture u16
> produit `oracle_faux_morts=1`. Ajouter `3W>N^2` en q3 et `2W>N^2` en q4.
> La mesure suffit à ne pas adopter cette boule unique, mais ne ferme pas la
> route dual-tree qui n'a pas été testée.

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

Le passage aux boules rend possible un comptage en bloc. Avec
`sphere_of(box)`, il ne rend toutefois pas le certificat plus grand : la
sphère circonscrit l'AABB, donc son intersection universelle est incluse dans
celle de Corner64. Il change la **forme du calcul**, pas l'ensemble certifié.

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

**La boule ouverte `B°(m, kappa_q L)` est incluse dans `W_q(a,b)`, et sa
frontière est tangente à celle du fuseau.** Preuve : dans le plan méridien, la lentille est
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

**`B°(m, R_q)` est incluse dans `W_q(a,b)` pour TOUT `(a,b)` de `A x B`, avec
`R_q = kappa_q (d - r_A - r_B) - (r_A + r_B)/2`.**

Une soustraction au niveau mathématique. L'implémentation exacte exige ensuite
des arrondis dirigés et une largeur prouvée. C'est un cœur commun du rectangle,
en forme close.

## 5. Le seuil de séparation — première rédaction fausse, corrigée

> **Cette section a été écrite deux fois.** La première annonçait un seuil
> `s > 2 + 1/kappa_q`, soit `4,00 / 5,46 / 5,86`, et en tirait une explication
> du fait 3 du reçu. C'était faux, et la première mesure l'a montré : à `s = 4`
> tous les cœurs étaient déjà non vides sur les trois familles. L'erreur porte
> sur la **convention de séparation**, et la version ci-dessous la corrige.

Le dossier ne sépare pas sur la distance des centres `d >= s max(r)`, comme la
WSPD classique, mais sur l'**écart des boules** :

`separated()` rend `d - r_A - r_B >= s max(r_A, r_B)`.

Donc `L2 = d2 - S2 >= s r_max` directement, et avec `S2 <= 2 r_max` :

`R4 = 2 kappa_q L2 - S2 >= 2 r_max (kappa_q s - 1)`.

Le cœur est donc uniformément garanti non vide dans le pire cas continu si
`s > 1 / kappa_q`. C'est une condition suffisante uniforme, pas un « si et
seulement si » pour chaque rectangle ; le plancher entier décale en outre les
petites échelles :

| lane | `s` minimal | `R_q / r_max` à `s=6` | à `s=8` | à `s=10` |
| --- | ---: | ---: | ---: | ---: |
| q2 | `2,000` | `2,000` | `3,000` | `4,000` |
| q3 | `3,464` | `0,732` | `1,309` | `1,887` |
| q4 | `3,864` | `0,553` | `1,070` | `1,588` |

Le cœur naît donc **beaucoup plus tôt** que je ne l'avais écrit, et il est déjà
substantiel à `s = 6`. La mesure confirme le seuil et son ordre : sur
`uniform, n=400`, la proportion de rectangles à cœur non vide vaut

| `s` | q2 | q3 | q4 |
| ---: | ---: | ---: | ---: |
| `1` | `66,3 %` | `37,5 %` | `34,0 %` |
| `2` | `100 %` | `74,1 %` | `68,1 %` |
| `3` | `100 %` | `97,6 %` | `93,3 %` |
| `4` | `100 %` | `100 %` | `100 %` |

Sur ce nuage q2 atteint `100 %` à `s=2`, q3 et q4 entre `3` et `4`, et q3 — de
seuil continu uniforme `3,464` contre `3,864` — est plus proche de la
complétude à `s=3`. C'est un constat empirique compatible avec l'ordre des
constantes, pas une équivalence rectangle par rectangle.

Le seuil corrigé est gravé par la fixture `seuil` du probe, dix lignes exactes
dont deux serrent les constantes à environ un centième : à `s = 3,4` le vrai q3
est encore vide alors qu'un `2 kappa_3` surestimé à `3/5` le rendrait positif,
et de même à `s = 3,84` pour q4 contre `21/40`.

**Ce que cette correction retire.** L'explication du fait 3 du reçu — le facteur
six sur le résiduel entre `s=6` et `s=8` — ne tient plus telle que je l'avais
écrite, puisque le cœur q4 n'est pas « à peine né » à `s=6`. Le rayon passe de
`0,553` à `1,070 r_max`, soit un facteur `7,2` en **volume**, ce qui reste
compatible avec le facteur six observé ; mais c'est désormais une compatibilité,
pas une explication, et elle porte sur le cœur-boule quand la mesure portait sur
la borne-boîte. Je ne revendique donc plus d'avoir expliqué ce fait.

Deux réserves subsistent. Le tableau est un **pire cas** (`r_A = r_B`) ; un
rectangle déséquilibré fait mieux. Et `sphere_of(box)` prend la sphère
**circonscrite à l'AABB**, de rayon `sqrt(3)/2` fois le côté, là où la boule
englobante minimale des points du nœud serait souvent bien plus petite. Ce
`r` trop grand est de la marge laissée sur la table, et il entre linéairement
dans `R_q`.

## 6. Correction : la boule AABB est dominée par Corner64

Le cœur-boule relaxe les nœuds par leurs boules englobantes ; `corner64` les
relaxe par leurs AABB. Pour `sphere_of(box)`, l'AABB est incluse dans sa sphère
circonscrite. L'intersection universelle sur les sphères est donc incluse dans
l'intersection sur les AABB : le cœur-boule ne peut jamais gagner un témoin
sur Corner64. Le montage correct l'emploie pour les crédits en bloc, puis
applique Corner512/Corner64 à **tout le complément**. Un nœud disjoint de la
boule reste `UNKNOWN` pour Corner64 et ne peut pas être élagué sur ce seul
motif. Une vraie complémentarité exige une sphère englobant les PointId mais
pas nécessairement toute l'AABB, plus un ledger d'IDs par lane.

## 7. Ce que ça change algorithmiquement

Le sous-certificat de `h_coeur` devient : **compter les points de la boule
ouverte `B°(m,R_q)`, hors `A` et hors `B`, plafonné à `h_q`**. Pour
`s_max=11`, `h_q` ne dépasse pas dix ; le test uniforme du rectangle est :

> si le `h_q`-ième vrai PointId hors `A union B` est à distance strictement
> inférieure à `R_q`, alors toutes les ancres du rectangle meurent pour ce
> certificat.

Ce n'est pas un « si et seulement si » pour la mort d'une ancre : `h_a+h_b`
peut compléter le seuil, et les fuseaux contiennent des témoins hors de cette
boule.

`h_a` ne suit pas le même schéma : la boule midpoint peut perdre des témoins
proches du pôle `a`. La boule d'apex de la section 11 est seulement un
minorant ; préserver les valeurs demande un fallback exact ou l'auto-jointure
dual-tree à range-add du ré-audit.

## 8. Faut-il un kd-tree ?

Pas encore, et pour une raison précise : **la structure est déjà là et je m'en
servais mal**. L'octree Morton porte des intervalles `[first, last]`, donc le
compte d'un sous-arbre est en `O(1)`. Ce qui manque n'est pas l'arbre, ce sont
deux primitives sphère–boîte :

- « boîte entièrement dans la boule » — crédit en bloc **exact**, avec retrait
  de la lane du masque transmis aux enfants pour interdire tout double crédit ;
- « boîte disjointe de la boule » — élagage de cette branche boule seulement,
  jamais de Corner512/Corner64.

Le coût observé peut suivre la frontière de la sphère, mais ce n'est pas une
borne : une requête octree peut visiter `Theta(n)` nœuds, avec arrêt dès
`h_q` succès seulement.

Un kd-tree ne se justifierait que si l'octree se révélait mauvais sur
`eight_clusters` — la famille dure, celle dont les cellules atteignent `492`
points quand `uniform` plafonne à `146`. C'est là qu'il faudrait comparer, avec
une mesure appariée, jamais par principe. Ajouter un second index sans ce
chiffre serait exactement le genre de décision que le dossier reproche ailleurs.

## 9. Plan révisé

1. **Cœur-boule en forme close** comme fast path, puis Corner512/Corner64 sur
   tout le complément. Le harnais doit vérifier `ball => corner` avec la
   sphère AABB courante, plus `direct==descente` par lane.
2. **`h_a` et `h_b` par dual-tree à range-add**, avec cône/boule en fast path
   et fallback exact ; Q23 reste ouverte tant que cette route n'est pas
   implémentée et mesurée.
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

## 10. Ce que le point 1 a produit, puis la correction du rayon `+1`

Le point 1 du plan historique a produit `prototype/spindle_core_ball.hpp` et
sa porte `mhgp3v_core_ball_*`, dix-huit CTests verts. À cette étape seulement,
aucune mesure de fermeture n'avait encore été prise.

**La tangence est vérifiée par un juge sans constante.** La fixture confronte
les six points entiers au fuseau exact en entiers, qui n'emploie ni `kappa`, ni
boule, ni rationnel : aucun défaut commun n'est possible. Les six verdicts
tombent comme prévu, et le rayon rationnel tronqué les reproduit **tous les
six** — `perdus = 0`, avec `R4 = 200 / 115 / 103` pour un `L2 = 200`.

**La complétude est vérifiée séparément de la sûreté.** Sur `n=400` et les trois
familles, la descente avec élagage et crédit en bloc compte **exactement** ce
que compte le balayage direct, et les `3,2` millions de témoins certifiés sont
reconfrontés au fuseau sur toutes les paires de leur rectangle : `faux = 0`.
C'est la porte qui manquait au préfiltre de ce matin — un élagage trop gourmand
perd des témoins sans jamais mentir, donc aucune porte de sûreté ne le verrait.

**Correction du ré-audit.** La disjonction `>=` est bien équivalente pour une
boule ouverte. En revanche, `+1` sur le rayon n'est pas absorbé : une distance
de grille est une racine carrée d'entier, pas nécessairement un entier. La
fixture q3 `a=(0,0,0), b=(7,0,0), z=(3,2,0)` est acceptée par le rayon `9`
alors que le vrai rayon quadruplé vaut `14/sqrt(3)=8,083...` et que le fuseau
refuse. `rayon-plus-un` doit donc être un mutant causal ; le plancher nominal
reste sûr.

## 11. Q23 — boule unique non adoptée, route générale encore ouverte

Le point 2 du plan est fait. Il donne une réponse que je n'attendais pas, et
elle est négative.

### 11.1 Un cône suffisant pour `h_a`, pas sa région exacte

`h_a` compte les points `z` de `A` témoins de `(a,b)` pour tout `b`, à `a`
fixé. La boule du cœur n'y sert à rien : elle est centrée à l'équateur du
fuseau, loin de `A`. Les témoins de `A` sont au contraire près du **pôle** `a`,
là où le fuseau se pince.

À `a` fixé, avec `u = c_B - a`, `D = |u|` et `t = u + (delta - e)`,
`|delta| <= r_B`, on a `angle(u,t) <= arcsin((r_B + |e|)/D)`, donc la condition
du fuseau est garantie dès que

`angle(e,u) < gamma_q = theta'_q - arcsin((r_B + 2 r_A)/D)`,

en majorant `|e| <= 2 r_A`, ce que tout `z` de `A` vérifie automatiquement. On
obtient donc un **cône suffisant d'apex `a`**, strictement plus petit que la
région exacte, et la boule demandée est celle qui y est **inscrite**, de centre
`a + l u^` et de rayon `l sin(gamma_q)`. En unités
doublées le `U` se simplifie et le test tient en une comparaison d'entiers :
`den |G ed - l ud|^2 < l^2 num`.

`l` s'avère être un **paramètre libre** : la sûreté ne le contraint pas, tout
`z` de `A` vérifiant `|e| <= 2 r_A` par définition du diamètre. C'est le
paramètre de position de la boule inscrite ; contrairement au rayon `+1`, le
doubler ne relâche pas l'ouverture angulaire.

### 11.2 Ce que la boule coûte, mesuré

Fixtures, `B` réduit à un point donc juge exact et complet : la boule capte
`95,0 %` des vrais témoins en configuration éloignée et `81,1 %` en agrégat
des trois lanes dans la configuration serrée, avec `faux = 0`; q4 seul n'y
capte que `71,258 %`. La correction angulaire doit être calculée à partir de
`D>=s max(r_A,r_B)+r_B` — `3/s` n'est pas une égalité. Sur
`n=4000`, `s=6`, `K=10` :

| famille | `ha_somme` jointure | par boule | fermeture q4 |
| --- | ---: | ---: | --- |
| `terrain` | `1 466 232` | `866 515` | `95,36 -> 94,34 %` |
| `uniform` | `986 527` | `407 884` | `83,92 -> 82,81 %` |
| `eight_clusters` | `1 016 656` | `514 865` | `61,29 -> 50,10 %` |

À `s = 8` la correction est plus petite et la perte se réduit — `72,80 -> 68,33`
sur `eight_clusters` — sans jamais s'annuler.

### 11.3 Ce que la mesure établit — et ce qu'elle n'établit pas

Vous demandiez de remplacer les auto-jointures ponctuelles au motif d'un coût
`O(|A|^2 + |B|^2)`. Le compteur dédié dit ce que ces deux postes pèsent
réellement dans le travail total :

| famille | part de `h_a` et `h_b` | par boule |
| --- | ---: | ---: |
| `uniform` | `14,6 %` | `17,0 %` |
| `eight_clusters` | `35,3 %` | `31,0 %` |
| `terrain` | `56,9 %` | `48,7 %` |

L'auto-jointure sort dès que le seuil `h_q <= 10` est atteint, ce qui aide sur
ces données. Cela ne donne pas `O(|A|h_q)` : l'arrêt porte sur les **succès** et
une suite d'échecs conserve le pire cas `O(|A|^2)`. `uniform`, la famille la
plus lente, est celle où ces postes pèsent le moins. La boule réduit le travail
de `28 %` sur `terrain`, mais l'**augmente**
de `20 %` sur `uniform`, et le temps de paroi est plus mauvais dans les trois
cas — `+6` à `+14 %` — chaque unité de travail par boule coûtant un `i128` et
une racine entière là où l'auto-jointure ne fait qu'un produit scalaire.

**Le travail est ailleurs** : dans la descente du cœur, `43` à `85 %` du total.
C'est là que `corner64` et le cœur-boule agissent, et c'est le seul poste dont
l'optimisation ait payé jusqu'ici.

### 11.4 Ce que je fais du chemin

Je le garde compilé mais **il n'est pas la route par défaut**. Les trois portes
à `s=6` ne suffisent pas : à `separation=1`, le signe de `gamma_q` perdu par le
carré produit une fausse fermeture. Après correction, la mesure justifie de ne
pas adopter cette **boule unique** sur ces tailles. Elle ne ferme pas
l'auto-jointure dual-tree à range-add proposée par l'audit, qui n'a pas été
implémentée.

Si vous voyez une erreur dans l'attribution du coût — le compteur
`travail_ha` compte les visites de nœud côté boule et les paires testées côté
jointure, ce qui n'est pas la même unité — dites-le : c'est le seul point où je
compare deux grandeurs qui ne sont pas homogènes, et le temps de paroi est ce
qui m'a fait conclure.

## 12. Ce que cette note ne revendique pas

Les chiffres de la section 11 sont des diagnostics `counter-only`, sans brut
chronométrique versionné ; ils ne promeuvent aucun statut. Le préfiltre ferme
des **ancres**, pas des supports. Les fixtures du cœur ont été ajoutées au
`6220ea3` et celles de l'apex au `66b4f0c`, mais le ré-audit ci-dessus bloque
les claims `+1`, l'exactitude du cône et la sûreté apex à `separation=1`.
