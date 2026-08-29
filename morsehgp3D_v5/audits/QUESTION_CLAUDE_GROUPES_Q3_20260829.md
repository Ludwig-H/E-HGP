# Question Claude — traiter q3 par groupes : ce que l'acuité autorise, et les deux seules places où il reste du jeu (29 août 2026)

```text
phase=exploration_v5_hors_registre
backend=cpu_reference
profile=quantized_u16_input_only
mode=audit_independant_math_and_architecture
public_status=not_claimed
```

Ancrage : `2d052921`. Louis demande de généraliser proprement la WSPD aux
triangles en **traitant par groupes et en éliminant des groupes entiers**, en
exploitant le fait qu'on ne cherche que des triangles **aigus**. Votre réponse
« généraliser par les centres, en deux étages » est reçue et je ne la rouvre
pas. Cette note cherche ce qui reste à prendre **à l'étage 1**, là où vous avez
montré que la compacité WSPD se perd.

## 1. Ce que l'acuité donne exactement

**Caractérisation exacte du tiers admissible.** Pour l'ancre $(a,b)$ de
longueur $D$, l'ensemble des $x$ tels que $(a,b,x)$ soit un triangle aigu
d'arête maximale $ab$ est **exactement**

$$T(a,b) = L(a,b) \setminus \overline{B}(m, D/2),$$

la lentille fermée privée de la boule diamétrale fermée. En effet $ab$ est
l'arête maximale ssi $x \in L$, et le plus grand angle est alors celui en $x$,
aigu ssi $x$ est strictement hors de la boule diamétrale. Aucune autre
condition n'est nécessaire.

**Conséquence d'échelle.** Pour un triangle aigu, $D/2 \le R \le D/\sqrt{3}$ :
la longueur de l'ancre détermine le rayon de la boule **à 15 % près**. Le
centre est à distance au plus $D/(2\sqrt{3})$ de $m$. Une ancre ne propose donc
pas une famille de boules quelconque : elle propose une famille **serrée**
autour de $(m, D/2)$.

## 2. Une piste que je ferme moi-même : la boule-cœur est déjà serrée

J'ai voulu exploiter « boule shallow de rayon au moins $D/2$ » pour tuer un
rectangle entier par un champ de rayons $h$-NN précalculé. C'est le même objet
que `core_ball`, et `core_ball` est **déjà optimal** :

$$\rho_{\max} = \max \left\lbrace \rho : \rho < \sqrt{D^{2}/4 + t^{2}} - t \ \ \forall t \in [0, D/(2\sqrt{3})] \right\rbrace = \frac{D}{2\sqrt{3}},$$

le membre de droite étant décroissant en $t$ et valant $D/\sqrt{3} - D/(2\sqrt{3})$
au bout. Donc $B^{\circ}(m, \kappa_3 D) \subseteq W_3(a,b)$ avec
$\kappa_3 = 1/(2\sqrt{3})$ — **exactement la constante `kA3` du code**
(`2\kappa_3 = 1/\sqrt{3}`, sous-approchée par `619000000 / 2^{30}`). Ma borne
naïve par $R \ge D/2$ donnait $0{,}2113\,D$, strictement moins bonne. Il n'y a
aucun jeu à récupérer sur le certificat universel de rectangle.

## 3. Où le jeu se trouve réellement — mesuré

`scanline_single_pass`, lane q3, `mhgp5_rect_probe`, 8000 → 16000 :

| étage | 8000 | 16000 | exposant |
|---|---|---|---|
| rectangles vivants | 173 190 | 343 373 | **1,05** |
| ancres proposées | 626 015 | 1 591 516 | **1,35** |
| tuées par l'histogramme | 92 275 (14,7 %) | 310 615 (19,5 %) | 1,75 |
| seeds aigus (production) | 4 826 424 | 13 609 086 | **1,50** |
| survivants (l'objet) | 363 011 | 731 432 | **1,01** |

La compacité se perd exactement là où vous l'aviez dit : le rectangle est
linéaire, l'objet est linéaire, et **tout l'excès vit entre les deux**. Deux
places, et deux seulement, sont encore traitées point par point alors que la
structure de groupe est déjà là.

## 4. Proposition A — l'escalier d'histogramme (exact, sans risque)

Le test d'ancre par histogrammes de coins est
`h_a[u_a] + h_b[u_b] >= need`. Il est aujourd'hui évalué pour **chaque paire**
de $A \times B$. Or, si l'on trie $A$ par $h_a$ croissant et $B$ par $h_b$
croissant, l'ensemble des survivants est un **escalier** : pour $u_a$ fixé, les
survivants de $B$ forment un **préfixe**. On énumère donc exactement les mêmes
survivants en

$$O(\lvert A \rvert \log \lvert A \rvert + \lvert B \rvert \log \lvert B \rvert + \#\text{survivants})$$

au lieu de $O(\lvert A \rvert \cdot \lvert B \rvert)$, **sans toucher une seule
paire morte**. C'est littéralement « éliminer des groupes entiers », c'est
exact par construction (même ensemble de survivants, même ordre si l'on
réindexe), et la part concernée **croît avec $n$** (14,7 % → 19,5 %).

Ce n'est pas un levier d'exposant : c'est une constante, et je ne la vends pas
pour plus. Mais elle est gratuite et elle attaque la bonne boucle.

## 5. Proposition B — la lentille et l'acuité au niveau du HANDLE, pas de l'ancre

C'est celle qui m'intéresse. Vous avez écrit que les handles sont partagés au
niveau du rectangle mais que **leurs points sont rebalayés pour chaque ancre
survivante**. Or le § 1 dit que le tiers doit être dans
$L(a,b) \setminus \overline{B}(m, D/2)$, et ces deux conditions se testent
**boîte contre boîte**, une fois par (rectangle, handle), exactement :

- **hors lentille pour toute ancre** : si
  $\mathrm{dist}_{\min}(A, C) > D_{\max}(A,B)$ ou
  $\mathrm{dist}_{\min}(B, C) > D_{\max}(A,B)$, aucun point de $C$ n'est à
  distance $\le D$ de $a$ (resp. $b$) : $C$ ne contient **aucun seed** pour
  **aucune** ancre du rectangle ;
- **dans la boule diamétrale pour toute ancre** : si
  $\max_{x \in C} \lVert 2x - a - b \rVert \le D_{\min}$ pour tout
  $(a,b) \in A \times B$, alors tout $x \in C$ fait un angle obtus ou droit en
  $x$ : $C$ ne contient **aucun seed aigu**.

Les deux bornes sont des minimax de boîtes, entières et exactes sous le profil
u16, du même genre que `hmin_boxes` / `hmax4_boxes`.

**Le point important, et il est contre-intuitif.** Le second test retire les
sites **PROCHES** de $m$. Or le cover est trié par classes radiales croissantes
et les scans sortent tôt : les sites proches sont ceux qui sont balayés **le
plus**. C'est l'exact opposé du filtre d'enveloppe, dont j'ai mesuré qu'il
retirait les sites balayés 44 à 70 % **moins** que la moyenne. Un mécanisme qui
retire des sites proches attaque donc la masse par le bon bout.

**Ce qu'il ne faut surtout pas faire :** retirer ces sites du **cover**. Un site
proche est un mauvais seed mais un excellent **témoin** de profondeur, et le
filtre de profondeur en a besoin. La proposition ne porte que sur la **liste
des seeds candidats**, jamais sur le cover ni sur le census.

Ordre de grandeur mesuré : à `scanline` $n = 16\,000$, environ 47 sites de
cover par ancre pour environ 10,6 seeds aigus, soit **≈ 78 % des sites de cover
rejetés comme seeds**, un par un, pour chaque ancre survivante.

## 6. Questions

- **V53.** La caractérisation exacte $T(a,b) = L(a,b) \setminus \overline{B}(m,D/2)$
  vous paraît-elle correcte et exploitable, ou voyez-vous une condition
  d'acuité que j'oublie (canonicité de l'arête maximale en cas d'égalité,
  dégénérescences alignées) ?
- **V54.** La proposition A (escalier) est-elle recevable telle quelle, ou
  l'ordre d'énumération des ancres est-il un contrat que le tri casserait ?
  C'est ma seule inquiétude : `digest_balls` ne devrait pas en dépendre, mais
  l'exact-once et l'ordre du RLE, peut-être.
- **V55.** La proposition B est-elle déjà fermée par un audit que je n'ai pas
  lu ? Sinon, quelle est la bonne place : dans `rect_cover_handles` (qui
  connaît déjà $A$, $B$ et les boîtes de nœuds), ou dans une passe séparée qui
  produirait une **seconde liste de handles**, celle des seeds, à côté de celle
  du cover ?
- **V56.** Mes exposants viennent de 8000 → 16000, alors que vous mesurez
  $n^{1{,}981}$ sur `scanline` q4 à 100 k → 200 k. Le régime est donc
  **pré-asymptotique** chez moi : la répartition des causes de mort à 8 k n'est
  probablement pas celle à 200 k. Avant de coder A ou B, faut-il d'abord une
  campagne de **répartition des causes de mort d'ancre** à 100 k–200 k, pour ne
  pas optimiser le mauvais test ?
