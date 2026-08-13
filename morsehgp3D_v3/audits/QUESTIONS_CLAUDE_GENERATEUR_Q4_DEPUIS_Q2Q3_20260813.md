# Questions de Claude — la propriété qui engendrerait q4 depuis q2 et q3

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=proposition_math_non_recue`,
`public_status=not_claimed`. GCP non utilisé.

Question posée : existe-t-il une propriété mathématique permettant d'engendrer
q4 depuis q2 et q3 sans payer `\lvert lens\rvert^2` ? Je crois l'avoir
identifiée, et je crois surtout qu'elle est **exactement** ce que vous nommiez
depuis le début sans que je le comprenne.

## 1. Ce qui est FAUX, et qu'il faut écarter d'abord

Un q4 positif d'arête maximale `ab` possède **au moins une** face aiguë, pas
deux. Votre contre-fixture, que j'ai revérifiée :

```text
a=(0,3,2)  b=(6,3,2)  x=(1,0,1)  y=(3,5,0)
H_x = (1,-3,-1).(5,3,1) = -5   aigu, porteur
H_y = (3,2,-2).(3,-2,2) = +1   obtus, NON porteur
```

Et même une face aiguë n'est pas un support q3 : la boule de `abx` et celle de
`abxy` passent par le même cercle, donc appartiennent au même **pinceau**, et
**aucune ne contient l'autre** — le point `O-rn` de la petite est à distance
`\lvert t\rvert+r>\sqrt{r^2+t^2}` du centre de la grande. Le rang ne s'hérite
donc pas. Apparier des supports q3 pour fabriquer q4 en perdrait par les deux
bouts.

## 2. La propriété que je propose

Fixons la paire `(a,b)`, `m=(a+b)/2`, `D=\lVert b-a\rVert`. Toute boule dont la
sphère passe par `a` et `b` a son centre en `c=m+t` avec `t\perp(b-a)`, donc
`t` parcourt un **plan**, soit **deux paramètres**.

Le calcul est immédiat :

$$\lVert z-c\rVert^2-\lVert a-c\rVert^2 \;=\; -H(z)-2\,t\cdot(z-m),$$

où `H(z)=(z-a)\cdot(b-z)`. Donc

$$z \text{ est intérieur} \iff 2\,t\cdot(z-m)+H(z)\;>\;0.$$

**C'est linéaire en `t`.** Chaque site `z` définit donc un **demi-plan** de
l'espace des paramètres, et :

- **q2** est le point `t=0` : `\lvert I\rvert=\#\lbrace z: H(z)>0\rbrace` ;
- **q3** vit sur les **droites** `2t\cdot(x-m)+H(x)=0`, une par porteur `x` ;
- **q4** vit aux **sommets** — l'intersection de deux droites, c'est-à-dire la
  sphère passant par `a,b,x,y` ;
- et la condition de rang `\lvert I_B\rvert\le smax-q` est exactement une
  condition de **niveau** dans cet arrangement.

## 3. Pourquoi cela casserait le mur

Le mur mesuré est `\lvert lens\rvert^2/2` par paire, avec `\lvert lens\rvert`
non borné sur les nappes — `7 811` à `terrain, n=25 000`, soit
`3{,}05\cdot 10^{7}` couples pour **une seule** paire.

Or les supports q4 ne sont pas tous les sommets de l'arrangement : ce sont les
sommets de **niveau au plus `smax-4=7`**. Et la borne classique du `\le k`-niveau
d'un arrangement de `m` droites est

$$O\bigl(m\,(k+1)\bigr),$$

donc **linéaire en `m` à `k` fixé**. Avec `m=\lvert lens\rvert=7\,811` et
`k=7`, cela donne de l'ordre de `6\cdot 10^{4}` sommets au lieu de
`3\cdot 10^{7}` couples — **un facteur `500`**, et il croît avec
`\lvert lens\rvert`.

C'est, je crois, exactement ce que vous désigniez en écrivant qu'il manque « les
niveaux `0..k` d'un arrangement de lignes », « les segments actifs
`P-P/N-N/P-N` », « le shallow cutting et la liste de conflits ». Je ne
comprenais pas de quel arrangement il s'agissait ; c'est celui-ci, dans le plan
des centres admissibles de la paire `(a,b)`.

## 4. Ce qui rend la chose sûre, et ce qui reste à prouver

Ce qui me paraît acquis :

- la linéarité en `t` est exacte et entière — `2t\cdot(z-m)+H(z)` avec `t`
  rationnel, donc un prédicat de signe sur des entiers après dénominateur
  commun ;
- q3 et q4 vivent dans le **même** arrangement, donc une seule construction
  sert les deux lanes, ce qui répond aussi à votre exigence de masque commun ;
- l'orientation canonique et l'arête maximale sont des contraintes sur `t` — la
  borne `R\le D/\sqrt2` en q4 est un **disque** de rayon `D/(2\sqrt2)` dans le
  plan des paramètres — donc un simple domaine borné.

Mes trois questions, et je ne les tranche pas :

1. **La borne `O(m(k+1))` s'applique-t-elle ici ?** Les droites ne sont pas en
   position générale : elles proviennent de points d'un nuage `u16`, avec
   coplanarités massives sur `terrain` et `scanline`. La borne du `\le k`-niveau
   vaut-elle sans position générale, ou faut-il la version dégénérée ?
2. **Est-ce bien votre `LocalShallowBall` ?** Si oui, je n'ai rien à inventer :
   il s'agit de construire l'arrangement et d'en extraire le `\le 7`-niveau, et
   ma proposition de triplets aigus devient superflue — ou au mieux une manière
   de borner `m` avant de construire l'arrangement.
3. **Où placez-vous `q2` là-dedans ?** `t=0` est un point de ce plan, donc q2
   est le niveau en un point fixe. La chaîne « q2 puis q3 puis q4 » serait alors
   « le point, puis les droites, puis les sommets » d'un **seul** objet, ce qui
   serait une simplification considérable de l'architecture — mais c'est une
   lecture, et je préfère que vous la validiez ou la refusiez.

## 5. Non-claims

Rien n'est implémenté. La borne `O(m(k+1))` est citée de mémoire et n'est pas
vérifiée dans ce dépôt. Le facteur `500` est une conséquence arithmétique de
cette borne, pas une mesure. Le contrat `50 000` reste entièrement ouvert et G4
reste NO-GO.
