# Réponse à `fe57d29` : un préfiltre exact q4 en seules longueurs carrées existe

Date : 19 août 2026.  
Pins relus : `b1f7eb16b7c1b62f02572bd939f7c8fea01b2ebb` puis `fe57d29d7f13dd069a3e48a4c96d8bcd0c50771a`.

## Verdict

Les deux avancées sont reçues positivement.

- La parallélisation par tranches ordonnées de `wspd_alive` est structurellement saine : le traitement des rectangles d'une vague est pur et la concaténation dans l'ordre des tranches préserve l'ordre déterministe. Les nouveaux compteurs montrent surtout que l'ancien diagnostic « q3 dominant » était faux : le scan q3 s'arrête après environ dix sites par seed et ne justifie pas l'index par couches convexes dans le régime mesuré.
- L'identité `(-V,+V,-V,+V)` de `fe57d29` est correcte. Avec `V = det(b-a,x-a,y-a)`, les quatre orientations du sommet opposé ont bien ces signes. Le calcul unique du volume est donc reçu.

Claude pose ensuite la bonne question : peut-on éliminer une partie des 41 % de complétions rejetées par `q4_center_strictly_inside` **avant** de construire `q4_form`, avec un prédicat nécessaire bon marché en longueurs carrées ?

La réponse est **oui**. Il existe même une caractérisation exacte classique du bien-centrage par les boules équatoriales des faces. Dans notre pipeline, elle est particulièrement intéressante parce que les six longueurs carrées du tétraèdre sont déjà disponibles au moment où l'on arrive au test du centre.

---

## 1. Lemme : puissance du sommet opposé à la boule équatoriale d'une face

Considérons une face `abc` non dégénérée et le sommet opposé `d`. Soient

```text
p = b-a,
q = c-a,
r = d-a.
```

Notons `o_F` le centre du cercle circonscrit à `abc` dans son plan et `R_F` son rayon. Le centre circonscrit `o` du tétraèdre appartient à la droite orthogonale au plan `abc` passant par `o_F` :

```text
o = o_F + t n,
d = pi_F(d) + h n,
```

avec `h != 0`. L'égalité `|o-d|² = |o-a|² = R_F²+t²` donne immédiatement

```text
2 h t = |d-o_F|² - R_F².
```

Par conséquent :

> `o` et `d` sont strictement du même côté de la face `abc` si et seulement si
> 
> `Pow_F(d) := |d-o_F|² - R_F² > 0`.

Or le centre circonscrit est strictement intérieur au tétraèdre si et seulement s'il est, pour chacune des quatre faces, du même côté que le sommet opposé. Donc :

> **Caractérisation exacte.** Un tétraèdre non dégénéré est strictement bien centré si et seulement si chacun de ses quatre sommets est strictement extérieur à la boule équatoriale de la face opposée.

Cela ne dit absolument pas que les quatre faces sont aiguës. La confusion interdite par les fixtures v3 disparaît : on teste la **puissance du sommet opposé à la boule équatoriale**, pas l'acuité de la face.

---

## 2. Formule entière sans centre ni division

Posons

```text
A = |p|²,
B = |q|²,
C = p·q,
D = p·r,
E = q·r,
F = |r|².
```

Le Gram de la face vaut

```text
Delta = A B - C² > 0.
```

Écrivons `o_F-a = alpha p + beta q`. Les équations de cercle donnent

```text
alpha = B(A-C)/(2 Delta),
beta  = A(B-C)/(2 Delta).
```

Comme `|o_F-a|² = R_F²`, la puissance de `d` est simplement

```text
Pow_F(d) = F - 2 r·(o_F-a).
```

Son signe est donc celui de l'entier

```text
N_F(d)
  = Delta F
    - B(A-C)D
    - A(B-C)E.
```

Ainsi :

```text
N_F(d) <= 0  => rejet CERTAIN avant q4_form,
N_F(d) > 0   => cette face est compatible avec un centre intérieur.
```

Tous les scalaires peuvent être reconstruits depuis les six longueurs carrées déjà calculées :

```text
2C = |ab|² + |ac|² - |bc|²,
2D = |ab|² + |ad|² - |bd|²,
2E = |ac|² + |ad|² - |cd|².
```

Pour éviter les moitiés, poser directement

```text
C2 = A + B - |bc|²,
D2 = A + F - |bd|²,
E2 = B + F - |cd|².
```

Alors `4 N_F(d)` s'écrit

```text
(4AB - C2²) F
- B (2A - C2) D2
- A (2B - C2) E2.
```

Le signe est identique. C'est donc un prédicat **pur i64/i128 sur longueurs carrées**, sans `q4_form`, sans Cramer, sans division et sans coordonnées du centre.

Avec les coordonnées u16 du projet, les longueurs carrées sont de l'ordre de `2^34`; chaque terme ci-dessus reste de l'ordre de `2^102`, donc `i128` suffit largement. Il faut garder la borne exacte déjà utilisée par le projet plutôt que coder ce chiffre comme nouvelle autorité.

---

## 3. Pourquoi c'est exploitable dans la boucle actuelle

Au point où `fe57d29` atteint le test du centre, le pipeline a déjà payé les tests de lentille/owner et connaît les six distances du quadruplet `a,b,x,y` ou peut les conserver sans nouveau `dist2`.

La première expérience à faire n'est surtout **pas** de remplacer immédiatement `q4_center_strictly_inside` par quatre puissances. Il faut transformer l'entonnoir en sous-entonnoir :

```text
atteint_centre
  -> puissance face abx, sommet y
  -> puissance face aby, sommet x
  -> puissance face axy, sommet b
  -> puissance face bxy, sommet a
  -> q4_form seulement si les tests choisis passent
```

et mesurer cumulativement combien de rejets supplémentaires chaque face capture.

### Ordre que je testerais d'abord

Commencer par la face `abx`, sommet `y`.

Raison d'implémentation, pas théorème de distribution : `a,b,x` est fixé pendant toute la boucle des complétions d'un seed. On peut donc pré-calculer une fois par seed les coefficients dépendant de la face :

```text
A, B, C2,
H = 4AB-C2²,
U = B(2A-C2),
V = A(2B-C2).
```

Pour chaque `y`, il ne reste essentiellement que la formation de `F,D2,E2` depuis des distances déjà disponibles puis

```text
H*F - U*D2 - V*E2 > 0.
```

C'est la meilleure chance d'obtenir un rejet utile avec très peu de travail par paire. Si ce premier test capture déjà une fraction notable des 41 %, il n'est peut-être jamais rentable d'évaluer les trois autres avant Cramer.

La face `aby` possède également `ab` fixe mais dépend de `y`; les deux dernières faces changent davantage. Leur intérêt doit être décidé par le ratio

```text
paires tuées supplémentaires / coût CPU supplémentaire,
```

pas par la beauté de la caractérisation complète.

---

## 4. Porte causale proposée

La porte mathématique est petite et indépendante du pipeline massif.

Pour des tétraèdres entiers non coplanaires :

1. calculer les quatre signes `N_F(opposite)` par la formule de longueurs ;
2. calculer `q4_form` puis le test actuel des quatre orientations du centre ;
3. exiger l'équivalence stricte

```text
all_four_equatorial_powers_positive
== q4_center_strictly_inside.
```

Il faut inclure :

```text
centre strictement intérieur,
centre extérieur par chacune des quatre faces,
centre exactement sur une face (puissance nulle),
faces obtuses mais tétraèdre bien centré si la fixture existe,
tétraèdre à faces aiguës mais non bien centré,
permutations des quatre sommets.
```

Mutants causaux :

```text
q4-equatorial-nonstrict       // >= 0 au lieu de > 0
q4-equatorial-drop-cross      // supprimer C2² dans H
q4-equatorial-opposite-sign   // inverser une puissance
```

Le premier est important : la frontière `N=0` correspond exactement à un centre circonscrit situé dans le plan de la face, donc elle doit être rejetée par le contrat strict actuel.

---

## 5. Décision recommandée

Je proposerais à Claude l'ordre suivant :

1. **conserver** l'identité de volume unique de `fe57d29` ;
2. ajouter la primitive pure `equatorial_power4(face, opposite)` et sa porte contre l'autorité Cramer actuelle ;
3. instrumenter uniquement le premier test `face abx / opposite y`, pré-calculé par seed ;
4. mesurer sur `uniform` et `eight_clusters` la fraction des `q4_rej_center` capturée **avant** `q4_form` et son coût apparié intra-processus ;
5. n'ajouter une deuxième/troisième/quatrième face que si le rendement marginal le justifie.

Si le premier test ne capture presque rien, abandonner immédiatement cette piste : la caractérisation est mathématiquement exacte mais le Cramer actuel restera probablement meilleur. Si elle capture une part substantielle des 41 %, on tient précisément le préfiltre nécessaire demandé dans le commit, sans aucune approximation géométrique.

## Conclusion

Le verrou posé dans `fe57d29` admet donc une réponse mathématique concrète : **la puissance du sommet opposé à la boule équatoriale d'une face est un test nécessaire, et les quatre tests ensemble sont équivalents au bien-centrage strict**. Pour le pipeline, la version intéressante est probablement le premier test sur la face `abx`, dont les coefficients sont amortis une fois par seed. C'est assez bon marché et assez exact pour mériter une mesure avant toute nouvelle sophistication q4.
