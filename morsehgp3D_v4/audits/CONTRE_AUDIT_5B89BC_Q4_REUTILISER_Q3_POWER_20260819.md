# Contre-audit après `5b89bc6` — le premier filtre équatorial existe déjà dans le moteur

Date : 19 août 2026.  
Pins relus : `fe57d29d7f13dd069a3e48a4c96d8bcd0c50771a` et audit `5b89bc6a5d7e54313a5f7f45deb9a98cb82743ff`.

## Verdict

L'audit `5b89bc6` est mathématiquement correct : pour une face non dégénérée `abc` et son sommet opposé `d`, le centre circonscrit du tétraèdre est du même côté de `abc` que `d` si et seulement si la puissance de `d` par rapport à la sphère équatoriale de `abc` est strictement positive. Les quatre signes donnent donc exactement le bien-centrage strict.

La remarque utile pour l'implémentation est plus forte : **pour la première face `abx`, ce prédicat n'est pas une nouvelle primitive. C'est exactement le signe de `q3_power(f3s, y)` déjà calculable par le kernel affine certifié du moteur.**

Il faut donc commencer par réutiliser cette primitive, et non introduire une seconde formule de puissance en longueurs qui ferait la même chose par un autre chemin.

---

## 1. Identité géométrique avec une coordonnée barycentrique du centre

Fixons la face seed `F = abx`, et notons :

- `o_F` son centre circonscrit dans le plan de `F` ;
- `R_F` son rayon ;
- `y` le sommet opposé ;
- `n` une normale unitaire à `F` ;
- `y = y_0 + h n`, avec `h != 0` ;
- `o = o_F + t n` le centre circonscrit du tétraèdre.

L'égalité des distances de `o` à `a` et à `y` donne

\[
|y-o_F|^2-R_F^2 = 2ht.
\]

Mais la coordonnée barycentrique du centre `o` associée au sommet `y` est exactement

\[
\lambda_y=\frac{t}{h}.
\]

Donc

\[
\operatorname{Pow}_{abx}(y)=2h^2\lambda_y.
\]

Ainsi :

\[
\lambda_y>0
\iff
\operatorname{Pow}_{abx}(y)>0.
\]

Autrement dit, le préfiltre proposé ne teste pas vaguement une condition nécessaire : il teste **exactement l'une des quatre coordonnées barycentriques dont `q4_center_strictly_inside` vérifie les signes**.

La frontière est également exacte :

```text
Pow = 0  <=>  lambda_y = 0  <=> centre dans le plan de la face
```

et doit donc être rejetée par le contrat strict.

---

## 2. Dans le code actuel, cette puissance est `q3_power(f3s, py)`

Pour chaque seed aigu `(a,b,x)`, le moteur construit déjà

```cpp
const Q3Form f3s = q3_form(pa, pb, px);
```

avant la boucle de complétion. La forme `f3s` est précisément la sphère équatoriale de la face `abx`, et le contrat de `q3_power` est :

```text
q3_power(f3s,z) < 0 : intérieur strict
q3_power(f3s,z) = 0 : coquille
q3_power(f3s,z) > 0 : extérieur
```

Le premier filtre équatorial est donc simplement :

```cpp
if (q3_power(f3s, py) <= 0)
    reject_completion();
```

placé **avant** `q4_form`.

Il n'y a aucune approximation et aucune hypothèse d'acuité supplémentaire : l'acuité de `abx` sert déjà à définir le seed q3, mais l'équivalence de signe ci-dessus ne dépend pas d'une affirmation fausse du type « tétraèdre bien centré => toutes les faces aiguës ».

---

## 3. Encore mieux : le filtre flottant certifié existe déjà

Le moteur a déjà gravé l'identité

\[
L(z,x)=4\,q3\_power(f3s,z)
\]

avec :

```text
affine_l_hat
+ affine_l_bound
+ repli exact i128
```

et les coefficients seed-locaux `G,N` sont déjà construits pour le cœur de seed.

La première expérience devrait donc avoir **deux variantes**, dans cet ordre :

```text
A. exact simple : q3_power(f3s,py) <= 0
B. affine certifié : signe de L, repli exact seulement si intervalle indécis
```

La variante B a une chance d'être pratiquement gratuite, puisque l'étage flottant et sa preuve existent déjà. Ne créons pas un second mini-système d'arithmétique certifiée pour découvrir ensuite qu'il calcule le même signe. Les logiciels géométriques ont déjà suffisamment de religions concurrentes.

Si l'accès de `py` à la SoA seed-locale n'est pas direct dans `valid_completion`, il vaut mieux transporter l'indice du site/lentille jusqu'à la lambda que recalculer une nouvelle représentation géométrique.

---

## 4. Formule en longueurs : à garder comme oracle croisé, pas comme premier chemin produit

La formule de `5b89bc6`

\[
(4AB-C_2^2)F
-B(2A-C_2)D_2
-A(2B-C_2)E_2
\]

est correcte et son signe est celui de la puissance équatoriale. Elle est très utile comme **troisième expression indépendante** du même prédicat dans une porte :

```text
sign(equatorial_length_formula)
== sign(q3_power(f3s,py))
== sign(lambda_y from q4/Cramer)
```

C'est même une meilleure utilisation immédiate de cette formule que de l'insérer directement dans la boucle chaude : elle donne un oracle de structure purement métrique, indépendant du kernel affine et du Cramer.

---

## 5. Expérience minimale avant toute extension aux quatre faces

Ajouter un compteur entre `exact_once` et `q4_form` :

```text
q4_rej_seed_face_power
```

et mesurer, sur `uniform` et `eight_clusters` :

```text
capture = q4_rej_seed_face_power / ancien q4_rej_center
```

avec le temps apparié intra-processus de la complétion q4.

La porte doit vérifier sur un ensemble non vide de tétraèdres non coplanaires :

```text
q3_power(f3s,y) > 0
<=> coordonnée barycentrique lambda_y > 0
```

et tuer au minimum :

```text
seed-face-power-nonstrict   // >= 0 accepté à tort
seed-face-power-sign        // signe inversé
```

Seulement si ce premier signe capture une fraction substantielle des 41 %, mesurer l'apport marginal d'une deuxième face. Les quatre puissances donnent une caractérisation exacte, mais quatre prédicats bon marché peuvent finir plus chers que les quatre signes du Cramer qu'ils remplacent. La mesure doit donc décider face par face.

## Conclusion

Le résultat mathématique de `5b89bc6` est reçu. Le raccord optimal au moteur est toutefois plus simple que proposé : **la puissance équatoriale de la face seed `abx` est déjà `q3_power(f3s,y)`, et son signe dispose déjà d'un étage affine flottant certifié.** C'est ce chemin qu'il faut tester avant d'ajouter une nouvelle primitive de production. La formule purement métrique reste excellente comme oracle croisé indépendant.
