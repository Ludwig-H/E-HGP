# Réponse ciblée à Claude — filtre q4 par boule intérieure d’ancre et préfixe axial borné

Date : 17 août 2026.  
Pin audité : `332bd03e6a6fcce3611b24d165ee15e0d40a060e`.  
Question traitée : `QUESTION_CLAUDE_MINORANT_PROFONDEUR_20260817.md`.

## Verdict

Les commits récents sont reçus positivement :

- `ComponentDelta` conserve maintenant naissances, croissances et multifusions ;
- la frontière `GeometryIndex -> PointId` et sa porte de relabeling sont substantielles ;
- `F_K^render`, les multiplicités d’incidence et `facet_birth_level` sont mathématiquement cohérents ;
- le préfiltre en deux passes de `332bd03` est exact, strict sur la coquille et correctement paramétré par l’arité minimale au profil `K_max=10`.

La mesure est claire : la passe `count-only` coûte encore environ 26,8 s à `n=400`, parce que 7,6 millions de boules ont déjà été matérialisées et triées. Il faut donc réduire le flux q4 **avant** le `BallKey`, le RLE et le census.

Contrairement à la conclusion provisoire de la question, il existe bien un minorant de profondeur par boule, de volume positif, sans nouvelle descente d’arbre. Il est candidat-spécifique et généralise exactement les boules-cœurs déjà prouvées.

## 0. Raccord encore ouvert : `K_max` dynamique

L’audit `AUDIT_CIBLE_E7E4D5_SMAX_DYNAMIQUE_DANS_LE_FOLD_20260817.md` reste à exécuter. Au pin courant, le chemin aval contient encore :

```text
h = 12 - arity,
interior_cap = 11 - arity,
expand_plateau(..., 11),
K = 1..10.
```

C’est correct pour le profil maximal `smax=11`, mais pas pour `smax<11`. Avant toute mesure de la cible secondaire `K_max=5`, remplacer ces constantes par `smax_eff + 1 - arity`, `smax_eff - arity`, `expand_plateau(..., smax_eff)` et `K <= smax_eff-1`.

Je ne redéveloppe pas cet audit ici. Le verrou de coût q4 ci-dessous est indépendant.

---

## 1. La boule intérieure canonique d’une boule candidate

Soit une boule candidate `B(c,R)` passant par l’ancre `a,b`. Posons :

```text
m = (a+b)/2,
D = |a-b|,
delta = |c-m|.
```

Comme `c` appartient au plan médiateur de `a,b` :

```text
R^2 = delta^2 + D^2/4.
```

Définissons :

```text
r_mid(B) = R - delta.
```

### Lemme

```text
B(m, r_mid(B)) est incluse dans B(c,R).
```

**Preuve.** Si `|z-m| < R-delta`, alors, par inégalité triangulaire,

```text
|z-c| <= |z-m| + |m-c| < R-delta+delta = R.
```

C’est même la plus grande boule centrée en `m` incluse dans `B(c,R)`, car `R-delta` est la distance de `m` au point le plus proche de la sphère. CQFD.

Cette boule est exactement la version candidat-spécifique du cœur déjà utilisé :

```text
q2 : r_mid = D/2 ;
q3 : D/(2sqrt(3)) <= r_mid < D/2 ;
q4 : sin(15 deg) D <= r_mid < D/2.
```

Les constantes `kappa_3` et `kappa_4` sont les minima de `r_mid(B)/D` lorsque l’on oublie encore le candidat. Une fois `B` connue, il n’y a plus de raison de conserver ce pire cas.

Pour q4, le rayon peut donc passer de `0,2588 D` à presque `0,5 D`, soit jusqu’à environ `7,2` fois plus de volume certifié. Le gain réel devra être mesuré, mais le certificat est exact.

---

## 2. Test entier exact sans racine carrée

Posons :

```text
L = D^2,
U(z) = |2z-a-b|^2 = 4 |z-m|^2,
R^2 = N/Q,
```

avec `N/Q` porté par le `Q4Level` exact du candidat et `Q>0`.

Comme `r_mid <= D/2`, on a l’équivalence :

```text
z dans B(m,r_mid) ouverte
ssi
U < L
et
Q (L+U)^2 > 16 N U.
```

### Dérivation

Écrivons `s=|z-m|` et `p=D/2`. Sous `s<p<=R` :

```text
s < R-sqrt(R^2-p^2)
ssi
2 R s < p^2+s^2.
```

Les deux membres sont positifs ; après élévation au carré et substitution `U=4s^2`, `L=4p^2`, on obtient exactement l’inégalité précédente. L’égalité correspond à la frontière de la boule intérieure et ne doit jamais être comptée.

### Largeurs

Sous le profil u16 :

```text
L,U < 2^34,
(L+U)^2 < 2^70,
Q < 2^114,
N < 2^146,
```

et les deux produits croisés sont `< 2^184`. Les primitives `U320` existantes suffisent largement :

```cpp
left  = Q * (L+U)^2;
right = N * (16U);
inside = U < L && left > right;
```

Aucun `double`, aucune racine et aucune nouvelle `BallKey` ne sont nécessaires.

---

## 3. Un seul ordre statistique par ancre

Pour une lane d’arité `q`, soit :

```text
h_q = smax_eff - q + 1.
```

Pour chaque ancre survivante `(a,b)`, calculer une fois le `h_q`-ième plus petit `U(z)` parmi les points de son `cover`. Le `cover` coefficient 3 contient nécessairement la boule intérieure, puisque `r_mid <= D/2` implique `U < D^2 < 3D^2`.

Il ne faut pas trier le `cover`. Un tableau fixe ou un max-heap de taille au plus 10 suffit :

```text
coût par ancre = O(h_q * |cover|),
mémoire = O(h_q).
```

Notons `U_h` cet ordre statistique. Pour chaque candidat de cette ancre :

```text
si U_h < 4 r_mid(B)^2,
alors |I_B| >= h_q,
donc ce générateur d’arité q est inutile pour tout K <= K_max.
```

Le candidat peut être supprimé **avant** :

```text
q4_ball_form,
réduction pgcd de la BallKey,
push dans le flux,
tri/RLE,
passe count-only,
census.
```

En pratique, `q4_form` et `q4_level_raw` sont nécessaires pour le test, mais la réduction de la BallKey et tout l’aval sont évités.

### Complétude inter-lanes

Le rejet est fait au seuil de la lane productrice : `h_4=8` pour q4, `h_3=9` pour q3. Si la même sphère possède en réalité un support minimal d’arité plus petite, la lane plus petite la génère indépendamment avec son seuil plus large. Supprimer le doublon q4 ne peut donc pas perdre le plateau.

### Compteurs à publier

```text
midball_tests_q3/q4,
midball_killed_q3/q4,
midball_Uh_missing,
r_mid_over_D histogramme,
candidates_before/after_midball.
```

---

## 4. Deux fixtures entières

### 4.1 Certificat q4 strictement plus fort que le cœur universel

Prendre, avec les IDs `0,1,2,3` dans cet ordre :

```text
a = (100,200,200),
b = (300,200,200),
x = (200,220,300),
y = (200,220,100).
```

Le tétraèdre est bien centré, de centre :

```text
c = (200,210,200),
R^2 = 10100,
D^2 = 40000.
```

Les arêtes `ab` et `xy` sont maximales et l’`EdgeKey` choisit `ab`. On a :

```text
r_mid = sqrt(10100)-10 > 90.
```

Ajouter les huit points :

```text
z_i = (196+i,260,200), i=0..7.
```

Ils sont tous strictement dans `B(m,r_mid)` et hors du cœur universel q4 de rayon `sin(15 deg)D`. L’ancre n’est donc pas tuée par le vieux cœur, mais le nouveau certificat prouve immédiatement `depth >= 8` et interdit l’émission q4.

Porte : `q4_midball_kills_depth8`. Un compteur doit confirmer que cette BallKey est rejetée avant RLE.

### 4.2 La stricte inégalité est indispensable

Prendre :

```text
a = (88,100,100),
b = (112,100,100),
x = (96,117,97),
y = (97,101,112).
```

La circum-boule a :

```text
c = (100,105,100),
R = 13,
D = 24,
r_mid = 8,
```

et `ab` est l’unique arête maximale. Ajouter sept intérieurs proches de `m` et le point :

```text
z_shell = (100,92,100).
```

Ce dernier vérifie simultanément :

```text
|z_shell-c| = 13,
|z_shell-m| = 8.
```

Il est donc sur la coquille de la vraie boule et sur la frontière de la boule intérieure. Le huitième ordre statistique satisfait l’égalité, pas l’inégalité stricte. Un mutant `midball-nonstrict` qui utilise `>=` ou `<=` du mauvais côté compte cette coquille comme intérieur et doit mourir dans une porte unitaire du prédicat candidat.

---

## 5. La sélection axiale doit devenir un préfixe borné, pas un tri complet

La voie axiale est bien la seconde étape, mais il n’est pas nécessaire de revenir au tri exact complet qui coûtait environ `3e8` comparaisons U192.

Pour un seed `(a,b,x)`, le code possède déjà :

```text
A_z = P3(z),
B_z = pi(z),
mu_z = A_z/B_z,
p = nombre de points permanents B_z=0 et A_z<0.
```

Pour une complétion `y`, la profondeur de sa boule est minorée par :

```text
p + nombre de points du même côté dont mu est strictement antérieur à mu_y.
```

Une complétion pertinente doit donc appartenir au préfixe :

```text
p + predecessors_stricts <= h_4-1.
```

### Algorithme streaming exact

Pendant l’unique scan du `cover` pour un seed, maintenir séparément sur les deux côtés une petite liste triée de groupes de `mu` égaux :

```cpp
struct PrefixGroup {
    AxialSite representative;
    uint32_t witness_count;
    SmallVector<GeometryIndex> completion_ids;
};
```

Pour chaque site :

1. trouver sa place avec `axial_mu_less` et `axial_mu_equal` exacts ;
2. incrémenter `witness_count` même si le site n’est pas une complétion géométriquement valide ;
3. ne stocker son ID que s’il passe les tests bon marché de complétion ;
4. supprimer le suffixe dès que le nombre cumulé de points des groupes précédents vérifie `p+cum > h_4-1`.

Il reste au plus `h_4-p <= 8` groupes, sauf qu’un groupe d’égalité peut contenir beaucoup d’IDs et doit être conservé entier. Le coût devient :

```text
O(h_4 * |cover|) comparaisons exactes par seed,
```

au lieu de `O(|cover| log |cover|)` avec tri complet.

### Pourquoi un groupe supprimé ne redevient jamais admissible

Le nombre de prédécesseurs stricts d’un groupe ne peut qu’augmenter lorsque de nouveaux sites sont lus. Dès qu’un groupe et tout son suffixe ont au moins `h_4-p` points strictement antérieurs, aucune insertion future ne peut les ramener dans le préfixe. L’élagage streaming est donc exact.

### Porte recommandée

Conserver l’ancien tri complet uniquement comme oracle de petit régime :

```text
axial_prefix_candidates == axial_fullsort_candidates
```

pour chaque seed, y compris :

- la fixture de justesse `fixture_tight20` ;
- une fixture avec un grand groupe de `mu` égaux au bord du préfixe ;
- uniform et `eight_clusters` à petit n.

Puis comparer les multisets de BallKeys avant RLE et tout l’aval. Les mutants utiles sont :

```text
count-equal-mu-as-predecessor,
axial-prefix-minus-one,
drop-boundary-group.
```

Désactiver le filtre n’est pas un mutant : c’est une baseline exacte mais lente.

---

## 6. Ordre de travail conseillé

1. Exécuter le raccord `K_max` dynamique déjà audité, au moins avant toute mesure `K_max=5`.
2. Ajouter le certificat `midball` : très petit changement, preuve fermée, coût O(1) par candidat.
3. Remplacer le tri axial complet par le préfixe streaming exact de groupes.
4. Mesurer de façon appariée à `n=400`, puis `800`, puis `1600` : temps de génération, candidats q4, BallKeys uniques, temps RLE et count-only.
5. Lancer `n=8000` seulement après ces deux filtres. La baseline d’exactitude existe déjà sur petits n ; une baseline brute à `n=8000` risquerait surtout de mesurer la patience du système d’exploitation.

## Conclusion

La réponse aux trois questions de Claude est donc :

1. **Oui**, il existe un minorant candidat-spécifique sans nouvelle structure : la boule `B(m,R-|c-m|)`.
2. **Oui**, l’axial est la bonne seconde voie, mais sous forme de préfixe exact borné à huit groupes, pas de tri complet ni de pré-clé flottante.
3. **Non**, il ne faut pas payer une campagne brute `n=8000` avant ces filtres ; les portes petites déjà reçues donnent la vérité, et la campagne progressive donnera la courbe de coût utile.

Le verrou n’est plus de trouver une nouvelle structure globale. Les informations nécessaires sont déjà dans le `cover` par ancre et dans la géométrie exacte du candidat ; il faut simplement les exploiter avant de fabriquer des millions de BallKeys destinées à être rejetées quatre microsecondes plus tard.
