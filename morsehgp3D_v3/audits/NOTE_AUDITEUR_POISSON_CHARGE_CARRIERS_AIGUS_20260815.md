# Note de l'auditeur — charge poissonnienne des carriers aigus

Date : 15 août 2026 UTC.

Complément à :

- [`NOTE_AUDITEUR_POISSON_DIMENSION_INTRINSEQUE_20260815.md`](NOTE_AUDITEUR_POISSON_DIMENSION_INTRINSEQUE_20260815.md) ;
- [`NOTE_AUDITEUR_ACUTE_BOX24_GATEWAY_20260815.md`](NOTE_AUDITEUR_ACUTE_BOX24_GATEWAY_20260815.md).

Cadre : `phase=exploration_v3_hors_registre`, `backend=math_reference`,
`profile=stationary_poisson_intrinsic_dimension`,
`mode=acute_carrier_load_model`, `public_status=not_claimed`.

> [!IMPORTANT]
> La linéarité attendue du `W`-vivant ne suffit pas à autoriser une
> matérialisation. Sous Poisson homogène, une paire q4 vivante possède en
> moyenne environ `5,49` carriers aigus sur une surface et `29,33` dans un
> volume. Le compte total attendu vaut respectivement environ `190 n` et
> `4 080 n` incidences `(owner edge, carrier)`.
>
> À `n=30 M`, cela représente `5,7 milliards` ou `122 milliards` d'incidences
> avant même la sélection de l'apex. Le gateway aigu doit donc conserver des
> blocs et alimenter directement la sweep shallow ; un tableau de faces reste
> exclu, même dans un régime théoriquement linéaire. La notation `O(n)` a, comme
> souvent, omis de joindre la facture.

## 1. Région géométrique d'un carrier

Fixons une paire `e={a,b}` de longueur `r`. Un troisième site `x` est un carrier
strictement aigu dont `ab` est l'arête maximale lorsque

```text
||x-a|| <= r,
||x-b|| <= r,
(a-x) dot (b-x) > 0,
```

les égalités de longueur étant de mesure nulle sous Poisson continu.

La région est donc l'intersection des deux boules de rayon `r` centrées en
`a,b`, moins la boule diamétrale de rayon `r/2`. La boule diamétrale est incluse
dans la lentille.

Son volume intrinsèque s'écrit

```text
|A_e| = c_d r^d.
```

Pour les deux dimensions utiles :

```text
c_2 = 5*pi/12 - sqrt(3)/2 = 0,4429715352,
c_3 = pi/4                 = 0,7853981634.
```

En dimension deux, la lentille de deux disques unité à distance un a pour aire
`2*pi/3-sqrt(3)/2`, dont on retire `pi/4`. En dimension trois, la lentille de
deux boules unité vaut `5*pi/12`, dont on retire `pi/6`.

## 2. Indépendance avec le `W`-vivant

Pour une lane `q`, écrivons

```text
|W_q(a,b)| = v_{q,d} r^d,
h = h_q.
```

Le fuseau `W_q` est inclus dans la boule diamétrale, tandis que la région aiguë
`A_e` est strictement à l'extérieur de cette boule. Les deux régions sont donc
disjointes.

Sous un processus de Poisson homogène d'intensité `lambda`, leurs nombres de
points sont indépendants conditionnellement à la paire et à `r`.

Posons

```text
u = lambda v_{q,d} r^d.
```

Parmi les paires `W_q`-vivantes, la densité de `u` est proportionnelle à

```text
P(Poisson(u)<h).
```

Les identités gamma donnent

```text
int P(Poisson(u)<h) du       = h,
int u P(Poisson(u)<h) du     = h(h+1)/2,
E[u | paire W-vivante]       = (h+1)/2.
```

Or, à `u` fixé, le nombre moyen de carriers vaut

```text
lambda c_d r^d = (c_d/v_{q,d}) u.
```

Donc :

```text
E[#carriers | paire W_q-vivante]
  = (c_d/v_{q,d}) * (h+1)/2.
```

## 3. Nombre total par point

La note de dimension intrinsèque donne

```text
E|V_q|/E|P| -> s_{d-1} h / (2 d v_{q,d}).
```

En multipliant par le nombre moyen de carriers par paire :

```text
E[C_q]/E|P|
  -> s_{d-1} c_d h(h+1) / (4 d v_{q,d}^2).
```

Ici `C_q` compte les incidences géométriques

```text
(owner edge, acute carrier).
```

Sous Poisson continu, les ties de plus longue arête ont probabilité nulle ; un
triangle aigu possède donc presque sûrement un owner unique.

## 4. Constantes q3 et q4

Avec `smax=11`, donc `h_3=9` et `h_4=8` :

| dimension intrinsèque | lane | `W`-vivantes / point | carriers / paire vivante | incidences carrier / point |
| ---: | ---: | ---: | ---: | ---: |
| `d=2` | q3 | `34,5267` | `5,4093` | `186,764` |
| `d=2` | q4 | `34,6244` | `5,4924` | `190,170` |
| `d=3` | q3 | `123,796` | `25,7909` | `3 192,815` |
| `d=3` | q4 | `139,070` | `29,3350` | `4 079,607` |

Ces comptes précèdent :

- l'indépendance affine du q4 ;
- la sélection de l'apex shallow ;
- la positivité tétraédrique ;
- le rang/census ;
- le RLE des `BallKey`.

Sur une nappe exactement plane, le q4 final est vide par dépendance affine,
mais les triangles carriers existent malgré tout. Le rang doit donc être
branché suffisamment tôt pour que le régime surfacique ne paie pas une sweep
quaternaire absurde.

## 5. Ordres de grandeur mémoire

Pour q4 :

| `n` | surface `d=2` | volume `d=3` |
| ---: | ---: | ---: |
| `50 000` | `9,51 M` incidences | `204,0 M` incidences |
| `30 M` | `5,71 G` incidences | `122,4 G` incidences |

À seulement seize octets par incidence :

```text
surface, 30 M : environ 91 Go,
volume, 30 M  : environ 1,96 To.
```

Cela interdit de transformer `AcuteCarrierBlock` en catalogue résident de
faces. La bonne unité de scheduling reste le bloc, avec

```text
count -> preflight -> continuation/sweep -> émission finale,
```

et non

```text
bloc -> expansion de toutes les faces -> tri ultérieur.
```

## 6. Portes expérimentales proposées

Sur les familles homogènes ou quasi homogènes, publier :

```text
C3_carrier / V3,
C4_carrier / V4,
C3_carrier / n,
C4_carrier / n,
AcuteCarrierBlocks physiques,
masse logique couverte,
faces effectivement matérialisées,
touches site-bloc,
octets et HWM.
```

Références q4 :

```text
terrain/surface : C4/V4 ~ 5,49 et C4/n ~ 190,
uniform/volume  : C4/V4 ~ 29,33 et C4/n ~ 4 080.
```

Les bords, la grille, l'inhomogénéité et les trous déplacent ces constantes ;
ce ne sont pas des seuils de correction. Elles fournissent néanmoins un ordre
de grandeur indépendant qui permet de détecter :

- un compteur confondant blocs physiques et masse logique ;
- un owner compté plusieurs fois ;
- une fenêtre de carrier incomplète ;
- une expansion dont le coût est déjà condamné avant l'apex.

La contre-famille `two_lines` demeure l'autre extrême : elle possède
`Theta(n^2)` ancres q4 vivantes mais exactement zéro carrier. Les deux modèles
doivent rester simultanément verts. Le premier interdit de matérialiser une
sortie linéaire à énorme constante ; le second interdit d'énumérer les ancres
avant de constater que leur sortie est vide.
