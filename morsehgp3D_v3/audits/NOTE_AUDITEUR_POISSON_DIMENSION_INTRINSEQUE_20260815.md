# Note de l'auditeur — loi du `W`-vivant en dimension intrinsèque

Date : 15 août 2026 UTC.

Complément à
[`AUDIT_SUIVI_POSITIF_P05_7493DECA_POISSON_20260815.md`](AUDIT_SUIVI_POSITIF_P05_7493DECA_POISSON_20260815.md).

Cadre : `phase=exploration_v3_hors_registre`, `backend=math_reference`,
`profile=stationary_poisson_intrinsic_dimension`, `public_status=not_claimed`.

> [!IMPORTANT]
> Le régime `terrain` ne doit pas être comparé en premier lieu au Poisson
> volumique 3D. Un nuage LiDAR de sol est principalement un processus de
> dimension intrinsèque deux plongé dans `R^3`.
>
> La loi de Campbell--Mecke se généralise exactement à toute dimension
> intrinsèque `d`. Elle prédit, pour q4, `34,624` ancres `W`-vivantes par point
> sur un plan, contre `139,070` dans un volume. Les mesures `terrain` commencent
> à `35,321` par point. Pour la lentille, la prédiction planaire vaut `14,230`
> candidats ; la mesure donne `16,60`.
>
> Ces constantes expliquent donc simultanément l'écart `terrain/uniform` et le
> coût d'instruction. Elles donnent aussi une raison théorique pour laquelle une
> baisse de densité LiDAR ne doit pas, au premier ordre, faire exploser le nombre
> d'ancres par point : l'intensité s'élimine du calcul.

## 1. Théorème en dimension intrinsèque `d`

Soit un processus de Poisson homogène d'intensité `lambda` sur un espace
euclidien de dimension `d`, ou sur un `d`-plan affine plongé dans `R^3`. Pour
une paire à distance `r`, supposons

```text
volume_d(W_q(a,b)) = v_{q,d} r^d.
```

Notons `s_{d-1}` l'aire de la sphère unité de dimension `d-1` et

```text
V_q = {(a,b) : N(W_q(a,b)) < h_q}.
```

Dans une fenêtre croissante,

```text
E|V_q| / E|P|  ->  C_{q,d}
C_{q,d} = s_{d-1} h_q / (2 d v_{q,d}).
```

La preuve est la même qu'en dimension trois :

```text
(lambda^2/2) s_{d-1} int r^(d-1)
  P(Poisson(lambda v_{q,d} r^d)<h_q) dr,
```

puis `u=lambda v_{q,d}r^d` et

```text
int_0^infty P(Poisson(u)<h) du = h.
```

L'intensité `lambda` disparaît après division par le nombre moyen de points.
C'est un résultat de **complexité moyenne par point**, pas une borne de pire
cas.

## 2. Constantes pour les trois fuseaux

### Dimension un

Sur une droite contenant les deux endpoints, `Xi=0`; les trois fuseaux ont la
même intersection, l'intervalle ouvert entre `a` et `b`. Ainsi

```text
v_{q,1}=1,
s_0=2,
C_{q,1}=h_q.
```

Pour `h_2/h_3/h_4=10/9/8`, les constantes valent `10/9/8`.

### Dimension deux

Dans le plan des endpoints, avec `|ab|=1`, `x` le long de l'ancre et `r` la
distance signée à son axe, les fuseaux satisfont

```text
x^2+r^2+alpha_q |r| < 1/4,
alpha_2=0,
alpha_3=1/sqrt(3),
alpha_4=1/sqrt(2).
```

Leurs aires normalisées sont

```text
v_{2,2} = pi/4,
v_{3,2} = -sqrt(3)/6 + 2pi/9,
v_{4,2} = 3pi/8 - sqrt(2)/4 - 3 asin(1/sqrt(3))/4.
```

### Dimension trois

Les volumes normalisés sont

```text
v_{2,3} = pi/6,
v_{3,3} = pi(27-4 sqrt(3) pi)/108,
v_{4,3} = pi(28-9 sqrt(2) pi
                 +18 sqrt(2) asin(1/sqrt(3)))/96.
```

### Tableau numérique

| dimension intrinsèque | q2 | q3 | q4 |
| ---: | ---: | ---: | ---: |
| `d=1` | `10,000` | `9,000` | `8,000` |
| `d=2` | `20,000` | `34,5267` | `34,6244` |
| `d=3` | `40,000` | `123,7962` | `139,0696` |

Le saut q4 entre une surface et un volume est un facteur `4,02`. Il est donc
normal que `terrain` porte beaucoup moins d'ancres que `uniform`, même à nombre
de points égal.

## 3. Application aux comptes actuels

Les comptes q4 publiés donnent :

| famille | `n=2000` | `n=4000` | `n=8000` | `n=16000` |
| --- | ---: | ---: | ---: | ---: |
| `terrain`, `|V_4|/n` | `35,321` | `37,019` | `39,226` | `41,716` |
| `uniform`, `|V_4|/n` | `94,884` | `103,033` | `109,885` | `115,551` |

Références homogènes :

```text
plan 2D : 34,624,
volume 3D : 139,070.
```

Le premier point `terrain` est à `2,0 %` de la constante planaire. La dérive
ultérieure n'est pas surprenante : le générateur contient courbure, relief
macroscopique, bords francs et `2 %` de points hauts ; ce n'est pas un Poisson
stationnaire sur un plan. Elle doit être interprétée comme une déviation au
modèle intrinsèque, non comme une puissance abstraite à deviner sur quatre
points.

Pour `uniform`, l'ajustement de bord en `n^(-1/3)` donne une limite empirique
`136,019`, à `2,2 %` de la constante volumique. Cela renforce l'interprétation
linéaire avec transitoire de bord et de grille.

## 4. Lentille en dimension deux et trois

La même correction conditionnelle s'applique dans toute dimension. Si

```text
R_{q,d} = volume_d(L)/volume_d(W_q),
```

alors, parmi les paires `W_q`-vivantes,

```text
E[N_L] = (R_{q,d} h_q + R_{q,d} - 2)/2.
```

### Plan 2D

L'intersection de deux disques de rayon `D` dont les centres sont à distance
`D` a pour aire

```text
ell_2 D^2,
ell_2 = 2pi/3-sqrt(3)/2 = 1,228369699.
```

Pour q4 :

```text
R_{4,2} = ell_2/v_{4,2} = 3,384553256,
E[N_L | W4-vivante] = 14,23049.
```

La mesure `terrain = 16,60` n'est qu'à environ `17 %` de cette prédiction,
malgré la courbure, les points hauts et les bords.

### Volume 3D

```text
ell_3 = 5pi/12,
R_{4,3} = 10,864814574,
E[N_L | W4-vivante] = 47,89167.
```

La mesure `uniform = 38,94` est du bon ordre. La mesure
`eight_clusters = 87,82` est au contraire `1,83` fois la prédiction homogène :
elle quantifie l'inhomogénéité et les grands vides, pas une confirmation du
modèle uniforme.

## 5. Conséquence pour le LiDAR et l'architecture

Dans un régime localement homogène sur une surface lisse, la densité spatiale
varie avec la distance au capteur mais s'élimine au premier ordre dans
`C_{q,2}`. Le nombre de `W`-vivantes **par point** dépend surtout de :

1. la dimension intrinsèque ;
2. la géométrie locale et les bords ;
3. les trous, occultations et mélanges de nappes ;
4. les paires inter-composantes.

Cela va dans le sens recherché pour MorseHGP : réduire la densité par dix ne
doit pas, à forme locale identique, changer radicalement le broad phase. En
revanche, une seconde nappe, un mur vertical, une multi-captation ou un grand
vide peut créer des paires croisées que le modèle stationnaire ne voit pas.

Le protocole expérimental naturel est donc :

- une nappe plane Poisson 2D, avec constante q4 `34,6244` ;
- un volume Poisson 3D, avec constante q4 `139,0696` ;
- des lignes Poisson 1D, avec constante q4 `8` ;
- deux nappes, deux lignes et scanlines avec trous, pour mesurer la masse
  croisée hors modèle ;
- un diagnostic par point de dimension locale `1/2/3`, puis comparaison du
  compte observé à la somme des constantes de référence.

Ces tests donnent un sens géométrique aux compteurs. Une pente log-log seule ne
peut pas distinguer une surface, un volume, une correction de bord et un amas
séparé ; elle réussit surtout à donner un nombre décimal à notre ignorance.
