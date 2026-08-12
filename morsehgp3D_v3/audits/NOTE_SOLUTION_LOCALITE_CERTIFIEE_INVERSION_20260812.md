# Note de solution — localité certifiée par inversion : la source par ancre

Date : 12 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Cette note est une **spécification de solution**. Elle ne certifie rien, ne
revendique aucune complexité et ne qualifie aucune mesure 50 k. Le verdict live
reste [`AUDIT_ETAT_COURANT.md`](AUDIT_ETAT_COURANT.md). Les réponses de
l'auditeur qui la corrigent sont dans
[`AUDIT_REPONSES_LOCALITE_INVERSION_20260812.md`](AUDIT_REPONSES_LOCALITE_INVERSION_20260812.md).

## 1. Ce que la coupe de Yao48 laisse sur la table

Une première version de cette note attribuait un facteur 384 à la coupe Yao48.
**C'était faux et l'audit l'a réfuté** : le facteur huit y était compté deux
fois. Le calcul correct, sous un modèle homogène isotrope :

- la région d'intérêt exacte d'une cible à distance `D` est la boule diamétrale,
  de volume `pi D^3/6` — elle vaut déjà un huitième de la boule de rayon `D`
  centrée sur l'ancre ;
- la région Yao simplifiée « la chambre de la cible, rayon `D/2` » est un
  secteur de solide angle `4 pi/48` de cette même boule de rayon `D/2`, donc
  exactement **un quarante-huitième de la boule diamétrale**.

Le rapport modélisé est donc **48**, pas 384. Et ce 48 est une intuition de
volume sous homogénéité : ce n'est ni une borne sur les visites LBVH, ni une
explication des familles anisotropes, ni une preuve de cause racine. En
particulier la traversée duale du dossier utilise **toutes** les directions :
ses pentes rouges ne peuvent pas lui être imputées. Cette section motive une
expérience, elle ne réfute aucune route.

## 2. Le théorème

Soit `P` le nuage u16 à positions deux à deux distinctes, `x` une ancre,
`K = 10`.

**Lemme de l'antipode.** Toute boule non dégénérée `B` de centre `c` et de rayon
`R` dont `x` appartient à la sphère possède l'antipode unique `x + D u`, avec
`D = 2R` et `u = (c-x)/R`. Elle est donc la boule de diamètre `[x, x + D u]`,
même quand l'antipode n'est pas une observation. Pour `s = z - x`,
`d = ||s||`, `v = s/d` :

$$z\in\mathrm{int}(B)\iff\left\Vert s\right\Vert^{2}<D\,(u\mathbin{\cdot}s)\iff d<D\cos\angle(u,v).$$

Le rayon nul et les positions colocalisées ne sont pas couverts : le probe les
refuse explicitement avant toute géométrie.

**Lecture par inversion.** En posant $\zeta(z)=s/\left\Vert s\right\Vert^{2}$,
la condition devient l'inégalité **linéaire**
$u\mathbin{\cdot}\zeta(z)>1/D$. Les boules passant par `x` sont exactement les
demi-espaces de l'espace inversé, et leurs intérieurs exactement les points
inversés au-delà du plan. À ancre fixée, le catalogue des boules à au plus
`K-1` intérieurs est donc le `<=(K-1)`-niveau du nuage inversé local.

**Lecture par calotte.** Le point `z` interdit exactement les directions

$$C_z(D)=\left\lbrace u\in S^{2}\ :\ u\mathbin{\cdot}v>d/D\right\rbrace,$$

calotte sphérique centrée en `v`, de rayon angulaire `arccos(d/D)`, qui
**croît** avec `D`. D'où :

> **Théorème (localité certifiée).** Soit `r > 0`. Si toute direction de la
> sphère appartient à au moins `K` calottes `C_z(r)`, alors toute boule `B`
> ayant `x` sur son bord et au plus `K-1` points intérieurs vérifie
> `diam(B) <= r`.

*Preuve.* Les calottes croissent avec `D`. Si `D > r`, la profondeur de `u` dans
la famille `C_z(D)` majore sa profondeur dans `C_z(r)`, donc au moins `K` points
sont strictement intérieurs. Contraposée. ∎

**Le seuil dix est sûr pour les trois arités, sans être optimal.** Dix
intérieurs donnent `p + q >= 12` pour toute arité `q >= 2`. Lorsque l'arité du
support propre positif est déjà certifiée, les seuils exacts d'inertie H0 sont
`10 / 9 / 8` ; une banque directionnelle peut les conserver dans le même ordre
statistique sans confondre les décisions des trois lanes.

## 3. Le raffinement directionnel

Un rayon **global** par ancre est correct mais inutilisable au bord : une
direction ouverte le rend infini et emporte toute l'ancre. Le théorème est
pourtant directionnel. Pour une cellule `c` et un point `z`, la calotte `C_z(r)`
contient `c` dès que `r > rho(c,z)`, avec

$$\rho(c,z)=\max_{g\in c}\ \frac{\left\Vert g\right\Vert\left\Vert s\right\Vert^{2}}{g\mathbin{\cdot}s},$$

le maximum portant sur les trois sommets `g`, et `rho = +infini` dès qu'un
sommet vérifie `g . s <= 0`. En posant `r_c` la `K`-ième plus petite valeur de
`rho(c, .)` : toute boule dont l'antipode pointe dans `c` vérifie
`diam(B) <= r_c`, ou possède au moins `K` intérieurs stricts.

L'inégalité est **large des deux côtés**, et c'est le bon côté fail-open : au
seuil `D = r_c` un témoin peut être *sur* la sphère et ne pas compter comme
intérieur, donc une activation vérifie seulement `D <= r_c`. Le filtre de
candidats emploie `d_y^2 <= r_c^2`, comparé exactement par
`d_y^2 * den <= num`.

Le localisateur de cellule doit inclure **toutes** les cellules réellement
incidentes à une direction : en oublier une au bord du réseau produirait une
omission fausse. Le mutant `locate-drop-boundary` grave ce cas.

## 4. Le prédicat, en entiers u16 exacts

Les directions sont discrétisées par la subdivision de l'octaèdre : sommets =
vecteurs **entiers** `g` avec `|g_x| + |g_y| + |g_z| = m`, `8 m^2` triangles
géodésiques, `4 m^2 + 2` sommets.

Une calotte stricte de rayon inférieur à 90 degrés s'écrit `a . u > t` avec
`t > 0` ; si ses trois sommets la vérifient, toute combinaison sphérique courte
la vérifie aussi, le numérateur dépassant strictement `t sum(lambda)` alors que
le dénominateur vaut au plus `sum(lambda)`. La couverture d'une cellule se
teste donc exactement sur trois sommets entiers :

$$g\mathbin{\cdot}s>0\quad\text{et}\quad(g\mathbin{\cdot}s)^{2}r^{2}>\left\Vert g\right\Vert^{2}\left(\left\Vert s\right\Vert^{2}\right)^{2}.$$

Aucune garde `||s|| < r` séparée n'est nécessaire : par Cauchy--Schwarz,
`||s|| >= r` fait échouer l'inégalité quadratique complète. Le seul test
`g . s > 0` ne suffirait pas.

Aucun flottant, aucun rationnel, aucune racine n'entre dans une décision. Les
amplitudes tiennent dans `__int128` : `||s||^2 < 2^34`, `||g||^2 <= m^2`, et la
comparaison croisée de deux `rho` reste sous `2^118`.

## 5. Le corollaire de Jung, qui ferme q3 et q4 sans nouvelle coupe

Soit une activation de support propre positif `S` contenant `x`. Le centre est
dans l'intérieur relatif de `conv(S)`, donc **`B` est la miniboule de `S`**. Par
le théorème de Jung en dimension trois,

$$R\leq\mathrm{diam}(S)\sqrt{\tfrac{3}{8}},\qquad\text{donc}\qquad D\leq\sqrt{\tfrac{3}{2}}\ \mathrm{diam}(S).$$

Par ailleurs, pour toute paire `(y,z)` de `S`, la boule diamétrale de `[y,z]`
est incluse dans `B`, donc a au plus `p <= 9` intérieurs : **toute arête de
support d'une activation d'arité quelconque est une activation q2**. Tous les
points de `S` sont donc à distance au plus `rho_x`, le plus long arc
d'activation q2 issu de `x`, d'où `diam(S) <= 2 rho_x` et

$$D\leq 2\sqrt{\tfrac{3}{2}}\ \rho_x<2{,}4495\ \rho_x .$$

Comme `x` est sur le bord de `B`, **toute** l'activation — support et intérieurs
— tient dans `B(x, 2.4495 rho_x)`. La lane q3/q4 hérite donc entièrement de la
borne q2 : il n'y a pas de second verrou de localité à lever, et aucune
couverture de cellule n'est requise dans les directions vides. C'est la réponse
à l'objection Q4 de l'audit pour les arités supérieures ; elle ne dit rien du
coût de la requête, qui reste à compter.

## 6. Mesures obtenues

Sujet : [`prototype/certified_locality_probe.cpp`](../prototype/certified_locality_probe.cpp).
Machine de mesure : codespace 2 vCPU, 7 Gio. **Ces secondes ne sont pas un
benchmark.**

### 6.1 Juges indépendants

L'arithmétique du juge est écrite séparément : déterminants entiers et Cramer
explicite, aucune calotte, aucune inversion.

- **Juge du census** — le census directionnel doit égaler l'énumération
  exhaustive de `C(n,2)`. `terrain n = 900` : `17 023 = 17 023`, écart nul. Ce
  juge tue quatre mutants du chemin par cellule (`rho-min-corner`,
  `rho-first-corner`, `rho-kth-short`, `locate-drop-boundary`) au code 4.
- **Juge A** — pour toute ancre certifiée au rayon `r` et tout point à distance
  au moins `r`, la boule diamétrale doit contenir `K` intérieurs stricts.
  `uniform n = 2 000`, `K = 10` : 1 639 064 paires lointaines, 0 violation.
- **Juge B** — énumération exhaustive des trois arités à support propre positif.
  `uniform n = 70`, `K = 4` : `q2 = 681`, `q3 = 884`, `q4 = 202`, 187
  vérifications de support, 0 violation.

### 6.2 Contradiction devenue fixture permanente

Le déterminant InSphere développé sur la colonne des normes est de signe
**opposé** à `orient3d` pour un point strictement intérieur. La règle `==` a
été livrée une fois : elle faisait tomber `q4` de 202 à 3 activations à
`n = 70`. Le contre-exemple minimal — tétraèdre de circumcentre l'origine, son
centre intérieur, un point lointain extérieur — est gravé, exercé à chaque
exécution, et tue son mutant `insphere-sign-flip` au code 4. Un plancher
`--min-q4` double la garde.

### 6.3 Masse de travail et taille de sortie

Grille `m = 4`, `K = 10`, census complet, localisation de cellule exacte.

| famille | n | candidats par ancre | p50 | p95 | census q2 | activations/point |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| terrain | 2 000 | 56,97 | 59 | 70 | 38 831 | 19,4155 |
| terrain | 4 000 | 58,89 | 60 | 71 | 79 264 | 19,8160 |
| terrain | 8 000 | 61,29 | 61 | 72 | 160 566 | 20,0708 |
| terrain | 16 000 | 68,13 | 61 | 73 | 325 071 | 20,3169 |
| scanline_single_pass | 16 000 | 81,11 | 60 | 79 | 324 041 | 20,2526 |
| scanline_overlap_multiecho | 8 000 | 98,63 | 75 | 238 | 187 526 | 23,4408 |

Le census q2 est **invariant** sous la finesse de grille (`m = 3, 4, 6, 8`) et
sous la fenêtre de voisinage (64, 256, 512, complète) : le filtre est
conservateur et la lane est close. À `terrain n = 2 000`, 68 % des paires
candidates sont de vraies activations.

Ces densités reproduisent, par une route entièrement différente, celles des
reçus `2e49dcf` : 20,58 activations par point pour `terrain` à 50 k, 20,61 pour
`scanline_single_pass`, 24,56 pour `scanline_overlap_multiecho`, 37,48 pour
`uniform`. C'est une concordance, pas une admission.

## 7. Ce qui n'est pas prouvé et ce qui manque

1. **La linéarité n'est pas prouvée.** La médiane des candidats est constante
   (59 à 61 sur un facteur huit en `n`), mais la queue croît : p99 de 125 à 554
   et maximum de 157 à 4 856 sur `scanline_single_pass`. Ces ancres sont celles
   dont des cellules restent ouvertes. `sum_x work(x)`, la mémoire et le
   high-water ne sont pas encore publiés.
2. **Le balayage d'univers complet par ancre n'est pas une architecture.** Il
   ferme la lane dans le probe de mesure ; le chemin produit doit employer le
   minorant de nœud proposé par l'audit,
   `LB_C(W) = max_g ||g||^2 d2_min^2 / dot_max(g,W)^2`, dans un best-first
   `LBVH x masque de cellules`, en comptant visites, tests, octets et
   high-water. Le calcul doit en outre être **cible-aware** : une cellule sans
   cible ne demande aucune couverture.
3. **L'énumérateur local des trois arités n'est pas écrit.** La bijection exacte
   entre plans inversés, supports propres positifs, niveaux stricts et fermés et
   `BallKey` reste à établir, avec coplanarités, coquilles multiples,
   orientation et owner exact-once. Le pinceau de
   [`order_k_bfs.hpp`](../prototype/order_k_bfs.hpp) **ne s'applique pas tel
   quel** : ses trois énoncés sont faux hors position simple et son germe est
   réfuté par une fixture gravée. C'est
   [`order_k_flats.hpp`](../prototype/order_k_flats.hpp) — théorème de
   propriétaire, connexité de l'ensemble des niveaux au plus `k`, flats fermés
   de rang trois — qui porte la complétude, comme oracle borné et instrument de
   mesure, jamais comme backend.
4. **La taille de sortie q3/q4 est presque inconnue.** Le seul instrument du
   dépôt qui la fende par arité est `prototype/scale_profile.cpp`, qu'aucun
   CTest n'exerce. À `n = 200`, profil nappe, `smax = 11`, il publie
   `1,00 / 23,66 / 99,99 / 86,13` enregistrements par point pour les arités
   1/2/3/4 : q3 vaut environ `4,2` fois q2 et q4 environ `3,6` fois q2. La
   sortie totale a 50 k serait donc de l ordre de `1e7` a `2e7`
   enregistrements. Un budget d une seconde sur 48 coeurs laisse alors environ
   `2,4` microsecondes de temps-coeur par enregistrement emis : **la taille de
   sortie ne contredit pas la cible**, seul le travail par unite de sortie le
   peut. Ce chiffre est une extrapolation depuis `n = 200`, pas une mesure
   d echelle.
5. **Aucun port CUDA**, aucun census q3/q4, aucun resolver, aucun fold, aucun
   payload officiel.

## 8. Portes tenues aujourd'hui

Vingt-deux CTests `mhgp3v_locality_*` : trois juges, invariance de grille et de
fenêtre, six mutants au code 4, cinq refus de CLI ou d'injection hors mode au
code 2, trois planchers au code 3.

GCP non utilisé pour cette note.
