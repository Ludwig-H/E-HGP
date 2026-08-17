# Audit de l'oracle rationnel q3 — commit `ebc8236`

Date : 17 août 2026.  
Pin audité : **`ebc82368bab03f93c2b8a480f810a93e3a8aeb74` inclus**.  
Cadre : `phase=exploration_v4_hors_registre`, `public_status=not_claimed`.

Ce commit est arrivé pendant l'audit du cover `a047460`. Il répond à la
priorité Q11 : disposer d'une arithmétique indépendante avant d'ouvrir q4.

---

## 0. Verdict

Le jalon est bon et doit être conservé.

Je reçois l'oracle comme **oracle indépendant des prédicats géométriques q3** :

- acuité par les trois produits scalaires d'angles ;
- circumcentre par Cramer direct du système 3×3 ;
- intérieur/coquille par comparaison de distances rationnelles homogènes ;
- rayon exact par identité croisée ;
- entier signe-magnitude à six limbes, distinct de la forme de Gram i128 du
  sujet.

L'accord sur tous les triangles des quatre nuages testés, ainsi que la mort des
mutants `sign-p` et `prune-ge`, est une vraie réception. Le code ne se contente
plus de demander au sujet s'il est d'accord avec lui-même, sport dans lequel
les programmes excellent généralement.

Je ne le qualifierais pas encore d'**oracle complet d'événements HGP** : il ne
juge ni l'API d'IDs externes, ni l'owner en vrais IDs, ni `SupportKey`, ni
`BallKey`, ni la liste canonique des intérieurs. Il juge très bien la
circum-géométrie q3, qui est précisément le socle à recevoir avant q4.

### Deux P0 courts

1. `OBig` appelle `std::abort()` en cas de dépassement. Le projet impose qu'un
   crash par signal ne soit jamais un statut. Remplacer l'abort par un retour
   `numeric_failure`/`invariant_violated` capturé par le probe.
2. Les nuages testés ont des coordonnées très petites. Les 384 bits sont
   justifiés sur le papier, mais les limbes hauts ne sont presque pas exercés.
   Ajouter des fixtures proches de la grille u16 maximale et un selftest de
   l'arithmétique `OBig` contre une troisième autorité.

---

## 1. Vérification des formules de l'oracle

### 1.1 Système du circumcentre

Pour `e_1=b-a`, `e_2=x-a` et `n=e_1×e_2`, le centre `c` vérifie

```text
2e_1·c = |b|²-|a|²,
2e_2·c = |x|²-|a|²,
n·c     = n·a.
```

Les deux premières équations imposent l'équidistance à `a,b,x`; la troisième
impose que `c` appartienne au plan affine du triangle. Le déterminant est non
nul exactement lorsque le triangle n'est pas colinéaire. Le remplacement des
colonnes dans `oracle_circumball` est un Cramer correct.

### 1.2 Signe de puissance

Avec `c=N/det`, multiplier la distance par `det²` donne

```text
|z-c|²-|a-c|²
  a le signe de
|z det-N|²-|a det-N|².
```

Le carré élimine sans ambiguïté le signe éventuel de `det`. La fonction
`oracle_power_sign` rend donc exactement :

```text
-1 intérieur strict,
 0 coquille,
+1 extérieur.
```

Elle est algébriquement indépendante de `Q3Form::P(z)`.

### 1.3 Niveau exact

Pour l'ancre owner `a,b` et l'apex `x`, posons

```text
D=|b-a|²,
E=|x-a|²,
F=(b-a)·(x-a),
X=|b-x|²=D+E-2F,
G=DE-F².
```

Le circumrayon vérifie

```text
R² = D E X / (4G).
```

L'oracle calcule aussi

```text
R² = |a det-N|² / det².
```

L'égalité testée

```text
|a det-N|² (4G) = D E X det²
```

est donc correcte.

### 1.4 Acuité

L'oracle exige les trois produits scalaires strictement positifs. Le sujet
choisit une arête maximale puis teste seulement l'angle opposé par
`V²>D²`. Sous arête maximale, les deux angles adjacents sont automatiquement
non obtus, et la stricte positivité à l'apex donne exactement le triangle
strictement aigu. La confrontation est pertinente et indépendante.

---

## 2. Revue de `OBig<6>`

### 2.1 Opérations reçues

La représentation signe-magnitude est canonisée sur zéro. La conversion de
`INT128_MIN` évite correctement la négation directe. Les additions et
soustractions de magnitudes utilisent un accumulateur u128 ; la détection de
borrow par le mot haut après sous-flux unsigned est correcte. Le produit long
accumule chaque ligne avec retenue, et le maximum

```text
(2^64-1)^2 + (2^64-1) + (2^64-1) < 2^128
```

tient dans l'accumulateur.

Je n'ai trouvé ni wrap signé, ni erreur de signe, ni perte évidente de carry
dans le chemin utilisé par l'oracle.

### 2.2 Le crash par signal doit disparaître

Les branches de dépassement font :

```cpp
std::abort();
```

Or `cmake/run_expect.cmake` grave qu'une terminaison par signal n'est jamais
un succès. Même si la preuve de largeur rend le chemin théoriquement
inatteignable, un bug de preuve ou une future réutilisation ne doit pas
transformer `numeric_failure` en processus abattu.

Préférer :

```cpp
enum class OStatus { ok, overflow };
```

ou une exception locale capturée dans `main`, convertie en code d'invariant et
statut typé. L'oracle doit échouer fermé, pas mourir théâtralement.

### 2.3 Durcir le type générique

`from_i128` ne stocke que les deux premiers limbes. Pour `N=1`, une valeur de
plus de 64 bits serait tronquée. Le code emploie `N=6`, donc aucun défaut live,
mais l'outil générique devrait porter :

```cpp
static_assert(N>=2);
```

ou vérifier les bits hauts. `sub_mag` peut aussi vérifier que le borrow final
est nul, afin que sa précondition ne reste pas seulement un commentaire.

### 2.4 Selftest arithmétique indépendant

Ajouter un exécutable dédié comparant `OBig` à
`boost::multiprecision::cpp_int` dans les tests uniquement :

- zéros signés, `INT128_MIN/MAX` ;
- carries traversant 1 à 6 limbes ;
- borrows traversant plusieurs limbes ;
- produits avec termes hauts non nuls ;
- overflow exactement au septième limbe, rendu comme statut et non signal ;
- distributivité et comparaison signée sur des milliers de valeurs
  déterministes.

L'oracle géométrique et l'oracle de son arithmétique seront alors réellement
décorrélés.

---

## 3. Les largeurs sont sûres, mais pas encore exercées

Les bornes annoncées `<2^323<2^384` sont conservatrices et suffisantes. Le
problème n'est pas la preuve de capacité ; c'est la couverture des tests.

Les familles `uniform/eight_clusters,n=48` ont une petite emprise, et les
fixtures cosphérique/tétraédrique vivent autour de coordonnées inférieures à
10. Les opérations utilisent donc surtout le premier, parfois le deuxième
limbe. Une bibliothèque de six limbes testée avec des entiers de cour de
récréation n'a pas encore fait l'expérience de ses propres ambitions.

### 3.1 Fixtures u16 extrêmes

#### Triangle régulier entier à grande échelle

Avec `M=65535` :

```text
(0,0,0), (M,M,0), (M,0,M)
```

forme un triangle équilatéral en distances carrées `2M²`. Il exerce :

- owner à égalités ;
- Cramer avec grands coefficients ;
- centre rationnel non dyadique ;
- niveau exact près du maximum de la grille.

#### Triangle presque droit mais aigu

```text
(0,0,0), (40000,0,0), (20000,20001,0)
```

est juste du côté aigu de la frontière de Thalès. Il distingue toutes les
strictes et donne une marge géométrique minimale sans quitter u16.

#### Grande cosphère

Centre `(32768,32768,32768)`, coquille par permutations signées de
`(12000,16000,0)`. Toutes les coordonnées restent u16 et le rayon vaut 20000.
Elle exerce les hauts bits tout en fournissant de nombreux extra-shells.

#### Translation près du bord

Rejouer les mêmes formes après translation maximale compatible vérifie que le
Cramer en coordonnées absolues ne cache aucune largeur liée à l'origine.

### 3.2 Fixture de largeur et mutant

Publier le plus haut limbe non nul observé pour :

```text
det,
num_i,
|z det-N|²,
produit du test de niveau.
```

Un mutant `OBig<5>` doit être refusé proprement sur une fixture qui exige le
sixième limbe, si la borne réelle atteint cette zone ; sinon réduire la largeur
revendiquée ou construire une fixture de sharpness. Une largeur « prouvée mais
jamais mordue » reste acceptable, mais elle doit être distinguée d'une porte
expérimentale.

---

## 4. Portée réelle de l'indépendance

Le nouveau chemin réécrit bien les formules structurantes. Il partage encore
avec le sujet :

- `P3`, `p3_sub`, `p3_dot`, `p3_norm2`, `p3_cross` ;
- `CloudIndex` et les familles de nuages ;
- le choix d'owner par rang interne dans ce test ;
- la liste des points du même nuage construit par la production.

Ces dépendances sont raisonnables pour un oracle de prédicats, car les
opérations partagées tiennent en i64 sous u16 et sont élémentaires. Mais la
phrase « un défaut commun ne peut pas se compenser » est trop absolue.

Deux durcissements peu coûteux :

1. coder localement dans le test les trois opérations `dot/cross/norm2` ou les
   juger sur les extrêmes par une seconde écriture ;
2. donner à l'oracle des `InputPoint{id,position}` déjà formés, sans dépendre du
   renommage de `CloudIndex`.

---

## 5. Ce que l'oracle juge, et ce qu'il ne juge pas encore

### Reçu

Pour chaque triangle aigu :

- acuité ;
- profondeur stricte complète ;
- nombre de points de coquille ;
- statut pertinent `depth<h_3 && shell=0` ;
- égalité du niveau exact.

### Pas encore jugé

- vrais IDs externes et permutation physique des records ;
- identité de l'arête owner sous égalités ;
- `SupportKey` exact-once de la source WSPD ;
- `BallKey` primitive ;
- liste triée `InteriorIds` et `ShellIds` ;
- `ExactCenter` canonique ;
- hyperincidence/facettes de l'événement.

Le bon nom actuel est donc

> `Q3CircumballPredicateOracle-v1`

plutôt que « oracle complet de la lane q3 ».

---

## 6. Compteurs à renommer

`shells_seen` est incrémenté une fois par **triangle** possédant au moins un
extra-shell. Sur la grande fixture cosphérique, plusieurs triangles peuvent
représenter la même boule. Le reçu parle de « 453 boules à coquille », alors
que le compteur mesure des **supports/triangles à coquille**.

Renommer immédiatement :

```text
supports_with_extra_shell
```

et, lorsque `BallKey` existe, ajouter :

```text
unique_degenerate_ballkeys.
```

Cette distinction sera cruciale pour décider si les plateaux u16 sont rares ou
si la même cosphère est simplement reproposée de nombreuses fois.

---

## 7. Mutants : deux bons, quatre à ajouter

### Reçus

- `sign-p` tue la confusion intérieur/coquille ;
- `prune-ge` tue l'élagage `mn>=0` qui masque une coquille.

Ils ciblent deux fautes historiques réelles et sont donc bien choisis.

### À ajouter

1. **Cramer colonne permutée** : échange de deux numérateurs du centre ;
2. **signe du déterminant oublié** dans une comparaison non quadratique ;
3. **facteur 4 omis** dans le niveau `DEX/(4G)` ;
4. **carry perdu** dans le produit long `OBig`.

Les fixtures u16 extrêmes doivent tuer au moins les mutants 1, 3 et 4. Le
mutant de carry garantit que les limbes hauts sont réellement traversés.

---

## 8. Ordre de travail recommandé

### P0 oracle

1. remplacer `abort` par un statut fail-closed ;
2. selftest `OBig` contre `cpp_int` ;
3. fixtures u16 extrêmes et compteurs de limbes ;
4. renommer `supports_with_extra_shell` ;
5. mutants Cramer/niveau/carry.

### P1 raccord à l'événement q3 complet

6. `InputPoint` et IDs externes ;
7. comparer `(SupportKey,OwnerEdgeKey)` ;
8. rendre `ExactCenter`, `ExactLevel`, `BallKey`, `InteriorIds`, `ShellIds` ;
9. comparer ces objets complets au petit oracle.

### P2 poursuivre la performance en parallèle

Le travail du cover et des paquets de témoins peut continuer sans attendre :
l'oracle est déjà assez indépendant pour sécuriser les prédicats q3. Il faut
simplement faire passer chaque nouveau backend `tree|cover|site-major|LBVH`
contre les mêmes événements canoniques.

---

## Conclusion

Claude a répondu correctement et rapidement à Q11. L'oracle Cramer/OBig est
une vraie seconde voie mathématique, et son accord actuel retire une incertitude
importante avant q4.

La priorité n'est pas de le réécrire : elle est de le **durcir là où il prétend
être large et transactionnel**. Des fixtures u16 extrêmes, un juge de `OBig`
et l'élimination des `abort()` transformeront ce bon oracle de prédicats en une
autorité durable. L'extension vers l'événement complet viendra naturellement
avec `BallKey` et les paquets d'intérieurs déjà demandés par l'audit du cover.
