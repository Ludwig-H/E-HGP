# Note de l'auditeur — nombre exact de vrais supports q4 sous Poisson

Date : 15 août 2026 UTC.

Compléments liés :

- [`NOTE_AUDITEUR_POISSON_DIMENSION_INTRINSEQUE_20260815.md`](NOTE_AUDITEUR_POISSON_DIMENSION_INTRINSEQUE_20260815.md) ;
- [`NOTE_AUDITEUR_POISSON_CHARGE_CARRIERS_AIGUS_20260815.md`](NOTE_AUDITEUR_POISSON_CHARGE_CARRIERS_AIGUS_20260815.md) ;
- [`CONTRE_AUDIT_POSITIF_Q4_PROPOSITIONS_EB42B574_20260815.md`](CONTRE_AUDIT_POSITIF_Q4_PROPOSITIONS_EB42B574_20260815.md).

Référence primaire : H. Edelsbrunner, A. Nikitenko et M. Reitzner,
*Expected Sizes of Poisson–Delaunay Mosaics and Their Discrete Morse
Functions*, Advances in Applied Probability 49 (2017), 745–767,
doi:`10.1017/apr.2017.20`, arXiv:`1607.05915`.

Cadre : `phase=exploration_v3_hors_registre`, `backend=math_reference`,
`profile=stationary_poisson_R3_general_position`,
`mode=slivnyak_mecke_blaschke_petkantschin`, `public_status=not_claimed`.

> [!IMPORTANT]
> Sous un processus de Poisson homogène dans `R^3`, le nombre attendu de vrais
> supports q4 positifs ayant moins de `h` points intérieurs est **exactement
> linéaire** :
>
> ```text
> E[W4_positive(<h)] / E[n]
>   = (3*pi^2/16) * C(h+2,3).
> ```
>
> Pour le seuil courant `h=r4=8` :
>
> ```text
> E[W4_positive] / E[n]
>   = 45*pi^2/2
>   = 222,066099...
> ```
>
> Le q4 positif n'est donc ni quadratique en régime homogène, ni petit. À
> `n=30 M`, le modèle prédit environ `6,66 milliards` de supports. Même à seize
> octets par support, cela dépasse `106 Go`. Une architecture exacte doit
> produire, trier/RLE et replier les événements en flux ; elle ne peut pas
> conserver un catalogue résident de tétraèdres, même lorsque la théorie donne
> le rassurant symbole `O(n)`.

## 1. Objet compté

Soit `X` un processus de Poisson stationnaire d'intensité `rho` dans `R^3`.
Pour quatre points affinement indépendants, leur sphère circonscrite est unique.
Notons :

```text
N_k^+(Omega)
```

le nombre de quadruplets non ordonnés tels que :

1. leur circumcentre appartient à une région `Omega` ;
2. le circumcentre appartient à l'intérieur strict du tétraèdre ;
3. la boule ouverte circonscrite contient exactement `k` autres points de `X`.

Sous Poisson, la position générale tient presque sûrement :

```text
shell = exactement les quatre sommets,
affine_rank = 3,
aucun tie géométrique non forcé.
```

Ainsi `N_k^+` est précisément le nombre de supports q4 positifs de profondeur
intérieure `k`, avant toute convention purement combinatoire d'owner/primary qui
ne doit changer que la multiplicité d'émission, pas l'objet géométrique.

## 2. Niveau vide : constante critique exacte

Pour `k=0`, ces tétraèdres sont les tétraèdres de Delaunay dont le circumcentre
est strictement intérieur. Ils sont exactement les 3-simplexes critiques de la
fonction rayon du complexe de Poisson–Delaunay.

La constante d'intensité donnée par Edelsbrunner--Nikitenko--Reitzner est :

```text
C^3_{3,3} = 3*pi^2/16 = 1,850550825...
```

Donc :

```text
E[N_0^+(Omega)]
  = (3*pi^2/16) * rho * |Omega|.
```

À titre de contrôle, l'intensité de **tous** les tétraèdres de Delaunay vaut

```text
D^3_3 = 24*pi^2/35 = 6,767728732...
```

La fraction des tétraèdres de Poisson–Delaunay qui sont bien centrés est donc
exactement :

```text
(C^3_{3,3})/(D^3_3) = 35/128 = 27,34375 %.
```

Cette fraction ne dépend ni de `rho`, ni du rayon : c'est une constante de la
loi angulaire du simplex inscrit.

## 3. Facteur exact du niveau intérieur `k`

La formule sphérique de Blaschke–Petkantschin pour quatre points en dimension
3 sépare :

```text
centre c,
rayon r,
forme angulaire u=(u0,u1,u2,u3).
```

Son jacobien radial est proportionnel à :

```text
r^8 dr.
```

L'indicateur « circumcentre dans l'intérieur du tétraèdre » dépend seulement de
la forme angulaire `u`, pas de `r` ni du nombre de points intérieurs.

Conditionnellement à `r`, le nombre de points dans la boule ouverte est
poissonnien de paramètre :

```text
t = rho * nu_3 * r^3,
nu_3 = 4*pi/3.
```

La partie radiale du niveau `k` est donc :

```text
I_k = int_0^infty r^8 exp(-t) t^k/k! dr.
```

Avec `t=rho*nu_3*r^3` :

```text
r^8 dr = [1/(3(rho*nu_3)^3)] t^2 dt,
```

et :

```text
I_k
 = [1/(3(rho*nu_3)^3)] Gamma(k+3)/k!
 = [1/(3(rho*nu_3)^3)] (k+1)(k+2).
```

Au niveau vide :

```text
I_0 = 2/[3(rho*nu_3)^3].
```

Le rapport est donc exactement :

```text
I_k/I_0
 = (k+1)(k+2)/2
 = C(k+2,2).
```

Tous les autres facteurs de Slivnyak--Mecke et de Blaschke–Petkantschin étant
identiques, on obtient :

```text
E[N_k^+(Omega)]
 = (3*pi^2/16) * C(k+2,2) * rho * |Omega|.
```

## 4. Somme shallow `<h`

En sommant `k=0,...,h-1` :

```text
sum_{k=0}^{h-1} C(k+2,2) = C(h+2,3).
```

D'où le théorème :

```text
E[N_<h^+(Omega)]
 = (3*pi^2/16) * C(h+2,3) * rho * |Omega|.
```

Comme :

```text
E[|X intersect Omega|] = rho * |Omega|,
```

la constante par point vaut :

```text
c4_positive(h)
 = (3*pi^2/16) * C(h+2,3).
```

### Seuil courant `h=8`

```text
C(10,3)=120,

c4_positive(8)
 = 120 * 3*pi^2/16
 = 45*pi^2/2
 = 222,0660990245...
```

Décomposition par profondeur :

| `k` intérieur | multiplicateur `C(k+2,2)` | supports positifs / point |
| ---: | ---: | ---: |
| 0 | 1 | 1,85055 |
| 1 | 3 | 5,55165 |
| 2 | 6 | 11,1033 |
| 3 | 10 | 18,5055 |
| 4 | 15 | 27,7583 |
| 5 | 21 | 38,8616 |
| 6 | 28 | 51,8154 |
| 7 | 36 | 66,6198 |
| **somme** | **120** | **222,0661** |

La majorité de la masse shallow vient donc des dernières profondeurs admises.
Passer de `h=8` à `h=7` ne retire pas un huitième, mais exactement le niveau
`k=7`, soit `36/120=30 %` de l'espérance q4.

## 5. Mise en rapport avec les étages déjà prédits

Pour q4, les notes précédentes donnent sous Poisson volumique :

```text
V4_pair_walive / n      ~ 139,0696,
C4_carrier / n          ~ 4 079,607,
W4_positive(<8) / n     ~   222,066.
```

Ainsi, en moyenne homogène :

```text
supports positifs / paire W4-vivante ~ 1,5968,
carriers aigus / support positif      ~ 18,37.
```

Ces rapports ne sont pas des invariants de chaque nuage. Ils donnent en revanche
un oracle statistique indépendant pour les compteurs de stage :

- si `W4_positive/n` converge vers plusieurs milliers sur un volume homogène,
  il existe probablement une duplication d'owner/primary ;
- s'il converge vers zéro, le gateway positif perd des supports ;
- si `C4_carrier/W4_positive` est proche de un, la fenêtre carrier est
  probablement incomplète ou le compteur ne mesure que les seeds retenus ;
- si la masse de `BallKey` distinctes dépasse la masse de `SupportKey`, les
  unités sont inversées.

## 6. Conséquence mémoire à la cible

Pour `n=30 000 000` :

```text
E[W4_positive]
  = 30 M * 45*pi^2/2
  ~ 6,662 milliards.
```

Empreinte minimale purement illustrative :

| octets par support | mémoire |
| ---: | ---: |
| 16 | 106,6 Go |
| 32 | 213,2 Go |
| 48 | 319,8 Go |
| 64 | 426,4 Go |

Un vrai enregistrement `SupportKey + BallKey + niveau + provenance` dépasse
probablement seize octets. La production q4 doit donc être :

```text
bloc -> count/preflight -> sélection shallow ->
BallKey/SupportKey -> radix/RLE en vague -> fold -> libération,
```

et non :

```text
énumération globale des tétraèdres -> tri global ultérieur.
```

La hiérarchie finale peut être beaucoup plus petite que la liste des supports,
mais cela n'aide que si le fold est effectué pendant le flux. Stocker d'abord
les milliards d'objets pour découvrir ensuite qu'ils fusionnent serait une
interprétation particulièrement littérale de « output-sensitive ».

## 7. Portes expérimentales

### 7.1 Processus Poisson périodique ou fenêtre intérieure

Un cube fini sans points extérieurs n'est pas le processus stationnaire : ses
sphères proches du bord sont artificiellement vides. Pour tester la constante :

- utiliser un tore périodique avec géométrie adaptée ; ou
- générer dans une grande fenêtre et ne compter que les centres dans une fenêtre
  intérieure éloignée du bord ;
- publier la correction en aire/volume lorsque la marge varie.

### 7.2 Rampe

Sur au moins quatre tailles et plusieurs graines :

```text
W4_positive/n -> 45*pi^2/2,
N_k^+/n -> (3*pi^2/16) C(k+2,2),
owner_duplicates = 0,
primary_duplicates = 0,
pending = 0,
```

avec intervalles de confiance empiriques entre graines.

### 7.3 Fixtures déterministes

Conserver en parallèle :

- `regular_tetra_q4_positive` : un support positif exact ;
- `one_acute_incident_face_q4` : complétude avec un seul seed aigu ;
- `cube8_shell_plateau` : dégénérescence cosphérique typée ;
- `two_lines` : masse pair-level quadratique et zéro support positif.

Le modèle Poisson teste la moyenne ; ces fixtures testent les bords logiques que
la moyenne a précisément la mauvaise habitude de ne jamais visiter.

## 8. Portée et limites

Ce résultat vaut pour un Poisson homogène volumique en position générale. Il ne
prédit pas directement :

- une surface LiDAR quasi 2D, où les tétraèdres deviennent presque dégénérés ;
- les scanlines et les multi-échos ;
- les grands vides et les mélanges de nappes ;
- les plateaux exacts de la quantification u16 ;
- le nombre final de fusions distinctes après fold HGP.

Il établit néanmoins un fait théorique net : la sortie q4 positive est linéaire
en espérance dans le régime volumique homogène, avec une constante exacte et
suffisamment grande pour imposer une architecture de streaming.
