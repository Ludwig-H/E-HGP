# Note de solution — source complète par ancre maximale et enveloppe mobile

Date : 12 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=proposition_math_non_recue`,
`public_status=not_claimed`.

Cette note est une **spécification de solution** écrite avant implémentation.
Elle ne mesure rien, ne reçoit aucune porte et ne modifie aucun statut public.
Elle demande un contre-audit sur les points listés en section 8.

## 0. Décision d'orientation

La lane « cellules de centres » cesse d'être poursuivie comme chemin produit :
au point gelé `uniform,n=50 000` elle publie `839 582 666` géométries pour
`21 395 212` supports, soit `39,242` occurrences par support. Elle reste
comparateur d'identités et falsificateur borné.

La route retenue est celle des théorèmes 4 et 5 de
[`AUDIT_VERROU_MATHEMATIQUE_FRONT_JUNG_H0_GPU_20260812.md`](AUDIT_VERROU_MATHEMATIQUE_FRONT_JUNG_H0_GPU_20260812.md),
avec une différence d'ordonnance : **aucun front n'est matérialisé avant
l'extension**. Le producteur est un balayage par ancre maximale dont le
prédicat de survie et l'extension partagent la même liste de voisins triée. Le
but est `occurrences = SupportKey_unique` par construction, sans RLE ni lift.

## 1. Objet produit

`X` est un nuage de `n` `PointId` distincts, coordonnées `u16`. Un **support**
est `S subset X`, `q=|S| in {2,3,4}`, tel que la miniboule `B(c,R)` de `S`
porte tout `S` sur sa frontière et `c in relint conv(S)`. On pose
`I_B={x in X setminus S : |x-c|<R}`, `p=|I_B|`, `U_B={x in X : |x-c|=R}`.
Le support est **pertinent** si `p+q<=smax=11`.

Source S est l'ensemble des supports propres positifs pertinents.

## 2. Ancre maximale : réduction exacte

### Lemme A — disque de Jung de l'arête maximale

Soit `S` un support propre positif, `(a,b)` une arête de longueur maximale
`D`, `m` le milieu, `w=2(c-m)`. Alors `w cdot (b-a)=0` et

$$\left\Vert w\right\Vert^{2}\leq\frac{D^{2}}{3}\ (q=3),\qquad\left\Vert w\right\Vert^{2}\leq\frac{D^{2}}{2}\ (q=4).$$

Preuve : Jung en dimension deux et trois donne `R<=D/\sqrt{3}` pour un triangle
et `R<=D\sqrt{3/8}` pour un tétraèdre, de diamètre `D`; puis
`|c-m|^2=R^2-D^2/4`. Les deux bornes sont atteintes (triangle équilatéral,
tétraèdre régulier).

### Lemme B — boule de milieu universelle

Pour tout centre `c` du disque de Jung, `B(m,\rho_q)\subset B(c,|c-a|)` avec
`\rho_3=D/\sqrt{12}` et `\rho_4=D/\sqrt{15}`.

Preuve : pour `s=|c-m|` et `|z-m|=t`, la condition suffisante est
`t+s<\sqrt{D^{2}/4+s^{2}}`, dont le membre droit moins `s` décroît en `s`. En
`s=D/(2\sqrt{3})` la valeur vaut exactement `D/\sqrt{12}`; en `s=D/(2\sqrt{2})`
elle vaut `D(\sqrt{3/8}-1/(2\sqrt{2}))=0{,}258819D>D/\sqrt{15}=0{,}258199D`.

Les trois tests de mort de lane sont donc entiers et sans division, avec
`u=2x-a-b` et `D^{2}=(b-a)\cdot(b-a)` :

| lane | témoin strict | seuil de mort |
| --- | --- | ---: |
| q2 | `u\cdot u<D^{2}` | 10 |
| q3 | `3(u\cdot u)<D^{2}` | 9 |
| q4 | `15(u\cdot u)<4D^{2}` | 8 |

Sur `u16`, `u\cdot u<=3\cdot131070^{2}` et `15\cdot4D^{2}<=180\cdot65535^{2}\cdot3`
tiennent en `i64` signé. Aucune boule témoin ne contient `a` ni `b`, puisque
`|a-m|=D/2>D/\sqrt{12}`.

### Lemme C — cône de préfixe et rayon de coupure certifié par chambre

Le profil du spindle universel `W_q(a,b)`, en unités de `D` et en notant `u`
la distance axiale depuis `a`, vaut

$$r_q(u)=\frac{\sqrt{a_q^{2}+4u-4u^{2}}-a_q}{2},\qquad a_3=\frac{1}{\sqrt{3}},\quad a_4=\frac{1}{\sqrt{2}}.$$

Le rapport `r_q(u)/u` est décroissant. Donc le cône de sommet `a`, d'axe
`b-a`, de demi-angle `\theta_q` et tronqué à `u<=1/2` est contenu dans
`W_q(a,b)` dès que `\tan\theta_q<=r_q(1/2)/(1/2)`, ce qui donne
`\theta_3=30` degrés et `\theta_4=27{,}368` degrés.

Corollaire : soit une banque de chambres directionnelles de demi-angle `\gamma`
autour de `a`. Si le sous-cône d'axe la chambre et de demi-angle
`\theta_q-\gamma` contient `h_q` `PointId` à distance au plus `d`, alors toute
paire `(a,b)` avec `b` dans la chambre et `D>=2d` a au moins `h_q` témoins
universels, donc n'ancre aucun support pertinent d'arité `q`. Le rayon de
coupure `2d` est un **certificat exact par (point, chambre)**, jamais une
heuristique de densité, et il vaut `+\infty` tant que la chambre n'a pas
accumulé ses `h_q` témoins — auquel cas la chambre reste ouverte et son univers
est balayé exactement.

C'est la seule réponse au régime `eight_clusters` : une ancre inter-amas dont
le sous-cône est vide reste énumérée; elle n'est ni prunée à tort, ni
silencieusement tronquée.

## 3. Enveloppe mobile et complétude du census

Pour une ancre `(a,b)` fixée, `d=b-a`, et un centre `c` du plan médiateur,
poser `w=2c-a-b`, `U_z=2z-a-b`, `g_z=D^{2}-U_z\cdot U_z`. La marge entière

$$F_z(w)=g_z+2U_z\mathbin{\cdot}w=4\left(R(c)^{2}-\left\Vert z-c\right\Vert^{2}\right)$$

a le signe de l'appartenance de `z` à l'intérieur, au shell ou à l'extérieur, et
`F_a\equiv F_b\equiv0`.

### Théorème D — filtre `theta` sûr et census complet

Soit `K` le disque de Jung, `L_z=\min_K F_z`, `U^{*}_z=\max_K F_z`, et soit
`\theta` la neuvième plus grande valeur de `L_z` sur `z in X setminus\lbrace a,b\rbrace`
(`-\infty` s'il y a moins de neuf sites). Si `U^{*}_z<\theta` strictement, alors
`z` est écarté.

Alors, pour tout support pertinent d'arité trois ou quatre ancré par `(a,b)` :

1. aucun site écarté n'est intérieur ni sur le shell;
2. `I_B` et `U_B` se lisent exactement sur les sites conservés.

Preuve : soit `z` écarté et `c` le centre du support. Les neuf sites réalisant
`L\geq\theta` vérifient `F(c)\geq\theta>U^{*}_z\geq F_z(c)`. Si `F_z(c)\geq0`,
ces neuf sites ont `F>0`, donc `p\geq9` et `p+q\geq12`. Le support n'est pas
pertinent. Aucun de ces neuf sites n'est écarté, donc le rejet est visible sur
les sites conservés seuls. Les extrémités `a,b` sont exclues de la sélection
parce que `F\equiv0` les rendrait indistinguables du shell.

Ce théorème remplace la sentinelle terminale `top-(12-q)` par un census
directement fourni par le producteur : `envelope_certified` doit valoir 100 %
des supports acceptés, `knn_fallback` zéro, et l'identité `(SupportKey,I,E)`
doit être comparée à l'oracle borné.

### Théorème E — carriers dans l'enveloppe

Un support q3 `abx` pertinent a `F_x(c)=0` et au plus huit sites `F>0`; un q4
`abxy` a `F_x(c)=F_y(c)=0` et au plus sept sites `F>0`. Dans les deux cas les
carriers ne sont pas écartés par le filtre `theta`. La recherche q3 est donc le
minimum auto-centré sur chaque droite conservée, et la recherche q4 est
l'ensemble des intersections de deux droites conservées.

Le centre q3 est exactement le point de `\ell_x` le plus proche de `m`, parce
que `\ell_x` est l'axe du triangle `abx` et que `|c-a|^{2}=|c-m|^{2}+D^{2}/4`.

## 4. Émission exacte-une-fois

Chaque support propre positif possède au moins une arête de longueur maximale.
L'owner est l'arête maximale de plus petit `PairId` canonique. Le producteur
n'émet un support que depuis cette arête. Donc

`occurrences = SupportKey_unique`

par construction, sans RLE ni lift, sans owner de cellule et sans point
location. Les mutants obligatoires sont : suppression du tie-break entre arêtes
maximales, choix de l'arête minimale, et émission depuis toutes les arêtes.

## 5. Ordonnance complète

```text
points u16
  -> LBVH exact + banque directionnelle de chambres
  -> par point a : liste de voisins triee par distance, bucketee par chambre,
     etendue jusqu'au rayon de coupure certifie de chaque chambre (Lemme C)
  -> par paire candidate (a,b) canonique : masque de lane par boules de milieu
     (Lemme B), sortie anticipee au seuil
  -> par ancre survivante : enveloppe mobile (Theoreme D) sur le prefixe de la
     liste de a a distance <= 1,2248 D
       |-> q2 : census direct de la boule diametrale
       |-> q3 : minimum auto-centre par droite conservee
       `-> q4 : intersections de droites conservees
  -> tests exacts : positivite, arete maximale, owner, pertinence, census (I,E)
  -> evenements H0 compacts -> dix forets -> payload
```

Aucune étape ne matérialise de mosaïque d'ordre supérieur, ni de catalogue de
paires, ni de CSR de droites : la liste de voisins par point est la seule
structure large, et son high-water est préflighté.

## 6. Bornes de travail annoncées comme mesurables, pas comme preuves

En espérance Poisson bulk, les quantités suivantes sont attendues et devront
être **mesurées**, pas admises :

- candidats de paires par point : de l'ordre de `(4\pi/3)\lambda^{3}` avec
  `\lambda` le rayon de coupure normalisé du Lemme C, soit environ `1 100` à
  `1 700` selon la finesse de la banque;
- sites évalués par ancre : la somme des `\#\lbrace x : |x-a|\leq1{,}2248D\rbrace`,
  dont l'audit du verrou donne la baseline `13 831,22 n`;
- supports émis : environ `440 n`, soit `22` millions à `50 000` points.

La gate reste deux pentes au plus `1,35` sur `uniform` **et**
`eight_clusters` à `12 500/25 000/50 000`, sur : candidats de paires, tests de
boule de milieu, sites évalués, intersections testées, supports émis.

## 7. Ce que cette note ne dit pas

Elle ne prouve aucune complexité, ne mesure aucun temps, ne reçoit aucune
porte, et ne ferme ni la dégénérescence `u16`, ni le cas terminal `k=n`, ni le
payload officiel. Les Lemmes A, B, C et les Théorèmes D, E sont des énoncés
géométriques élémentaires; leur exactitude ne dit rien du coût du producteur.

## 8. Questions à l'auditeur

1. Le Théorème D est-il correct sur le point suivant : le filtre `theta` est
   calculé avec `L_z` sur le disque de Jung **q4** (le plus grand des deux),
   afin de servir les deux lanes q3 et q4 avec une seule enveloppe. Y a-t-il une
   perte d'exactitude à mutualiser ainsi le disque ?
2. Le Lemme C suppose que le sous-cône de demi-angle `\theta_q-\gamma` autour de
   l'axe de chambre est inclus dans le cône de demi-angle `\theta_q` autour de
   `b-a` pour tout `b` de la chambre. Est-ce bien la seule hypothèse, et la
   troncature `u<=1/2` est-elle correctement traduite par « distance au plus
   `D/2` », donc rayon de coupure `2d` ?
3. L'émission exacte-une-fois par arête maximale canonique rend-elle inutile
   toute la lane `SupportKey`/RLE/owner de cellule, ou l'auditeur voit-il un
   support propre positif sans arête maximale bien définie sur `u16` ?
4. Sur `eight_clusters`, la chambre ouverte du Lemme C énumère l'univers de la
   chambre. Existe-t-il un certificat collectif reçu qui ferme ces paires avant
   énumération, ou faut-il accepter ce coût et le mesurer ?
5. Le contrat visé pour la cible « une seconde » reste-t-il
   `hgp_reduced_normalized_h0_v3`, ou faut-il produire `BenchmarkOutputContract-v1`
   complet — dix forêts, verticales, lots, certificat minimal — dans le même
   chronomètre ?

GCP non utilisé pour cette note.
