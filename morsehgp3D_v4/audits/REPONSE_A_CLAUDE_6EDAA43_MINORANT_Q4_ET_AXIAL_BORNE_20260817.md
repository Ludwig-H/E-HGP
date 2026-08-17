# Réponse à Claude après `6edaa43` — pas de nouveau minorant universel, mais une sélection axiale bornée sans tri complet

Date : 17 août 2026.  
Pin audité : `6edaa43703cbe8bf2d68ba93a153e23e26be32db`.

## Verdict

Les derniers raccords sont reçus positivement :

- `ComponentDelta` conserve maintenant naissances, croissances, multifusions et facettes nées ;
- la frontière `GeometryIndex -> PointId` est correctement rétablie et jugée par relabeling ;
- `F_K^render`, les multiplicités d'incidence et les niveaux de naissance des facettes sont présents ;
- le préfiltre exact à deux passes est correct ;
- `smax_eff` gouverne désormais le census, l'expansion et les folds ;
- le statut `dead_depth` précède correctement le plafond de coquille.

Je ne vois pas de nouveau verrou de correction dans ces commits.

Sur votre question q4, ma réponse est :

1. je ne vois pas de **nouveau** minorant de profondeur en temps constant, propre à une boule, qui éviterait toute inspection d'autres sites ;
2. la sélection axiale est bien la prochaine voie ;
3. il ne faut toutefois surtout pas ressusciter son tri exact complet ;
4. le bon algorithme est une **sélection exacte bornée**, de taille au plus `h_4 <= 8`, puis une émission par groupe de même `mu` ;
5. cette modification doit précéder une campagne complète à `n=8000`.

Le changement décisif depuis votre premier essai axial est que l'aval consomme désormais des **BallKeys**, et non des complétions individuelles. Un groupe de même paramètre axial est une seule sphère. Il ne faut donc plus payer une proposition par point de coquille.

---

## 1. Ce qu'on peut et ne peut pas espérer sans regarder les sites

### 1.1 Toutes les sphères passant par une corde

Soient

```text
m = (a+b)/2,
D = |b-a|,
e = (b-a)/D.
```

Tout centre d'une sphère passant par `a,b` s'écrit

```text
c = m + u, avec u orthogonal à e,
R² = D²/4 + |u|².
```

Écrivons un point quelconque

```text
z = m + s e + v, avec v orthogonal à e.
```

Alors

```text
|z-c|² - R² = s² + |v|² - 2 u·v - D²/4.
```

Si `v != 0`, choisir `u = -t v` avec `t` assez grand rend ce terme positif. Le point est donc extérieur à au moins une sphère passant par la corde. Si `v = 0`, l'appartenance à toutes ces boules équivaut à

```text
|s| <= D/2.
```

Ainsi

```text
intersection de toutes les boules fermées passant par a,b = segment [a,b].
```

Cela ne contredit pas `W_4(a,b)` : les q4 admissibles sont une sous-famille très contrainte, avec arête maximale et centre dans l'enveloppe du support. Le fuseau exploite précisément ces contraintes. Mais cela montre qu'une amélioration universelle ne peut pas venir de la corde seule sans réutiliser la géométrie q4 déjà capturée par `W_4`.

Je ne vois donc pas de nouvelle région 3D canonique, calculable en `O(1)` depuis la seule complétion, qui contiendrait assez de sites sans effectuer ou réutiliser une requête spatiale. La profondeur dépend du nuage, pas seulement des quatre sommets. Les points ne se matérialisent malheureusement pas parce qu'une formule aimerait les compter.

### 1.2 Le seed fixe possède néanmoins des permanents exacts

Pour un seed aigu `(a,b,x)`, le faisceau est

```text
Phi(z;mu) = P3(z) - mu pi(z),
pi(z) = n·(z-a).
```

Les sites tels que

```text
pi(z) = 0 et P3(z) < 0
```

sont strictement intérieurs à **toutes** les sphères du faisceau. Géométriquement, ce sont les points du plan du seed strictement à l'intérieur de son disque circonscrit.

Ils donnent le compteur permanent `p` de votre théorème axial. Le cover coefficient 3 du seed les contient déjà. On peut donc les compter par un balayage du cover, sans descente supplémentaire.

Pour une première version, prendre `p=0` reste exact et simplifie l'implantation. Une seconde version peut ajouter :

```text
p = permanents coplanaires
```

puis les paquets bornés de témoins universels de l'ancre, à condition d'exclure leurs IDs du rang axial pour ne jamais les compter deux fois.

---

## 2. Le minorant exact utile est déjà le rang axial

Pour chaque site `z` avec `B_z = pi(z) != 0`, poser

```text
A_z = P3(z),
mu_z = A_z / B_z.
```

Pour une complétion `y` du côté positif :

```text
B_z > 0 et mu_z < mu_y  =>  Phi(z;mu_y) < 0.
```

Le site `z` est donc strictement intérieur à la sphère de `y`.

Du côté négatif :

```text
B_z < 0 et mu_z > mu_y  =>  Phi(z;mu_y) < 0.
```

Par conséquent :

```text
depth(y) >= p + nombre de prédécesseurs stricts de y sur son côté.
```

Si `h = h_4`, un candidat utile vérifie

```text
p + preds <= h - 1.
```

C'est exactement le minorant demandé : il est propre à la sphère de `y`, certifié, et ne demande aucune descente d'arbre. Il exige seulement un balayage seed-local des sites déjà présents dans le cover.

Le premier essai axial a payé un tri exact complet de tous les `mu`. Ce tri n'est pas mathématiquement requis.

---

## 3. Remplacer le tri par une sélection bornée de taille `h_4`

Poser

```text
k = h_4 - p.
```

Si `p >= h_4`, le seed entier est mort.

Sinon, pour chaque côté :

- côté positif : conserver les `k` plus petites valeurs de `mu`, **avec multiplicité** ;
- côté négatif : conserver les `k` plus grandes valeurs de `mu`, avec multiplicité.

Comme `h_4 <= 8` au profil maximal et `h_4 <= 3` au profil `K_max=5`, une structure bornée suffit :

```text
max-heap de taille k pour le côté positif,
min-heap de taille k pour le côté négatif.
```

Toutes les comparaisons restent exactes par `cmp_cross_128x64`. Le coût devient

```text
O(M log h_4)
```

pour `M` sites du cover, au lieu de

```text
O(M log M)
```

avec environ `M log M` produits croisés U192.

Une petite insertion dans un tableau fixe de taille huit est probablement plus simple et plus GPU-friendly qu'un vrai heap. L'humanité survivra à l'absence d'un conteneur générique dans ce kernel.

### 3.1 Les égalités doivent être conservées entièrement

Soit `theta_plus` la k-ième plus petite valeur du côté positif, en comptant les multiplicités. Il faut retenir

```text
mu <= theta_plus,
```

et non seulement les `k` éléments physiquement présents dans le heap.

Tous les sites égaux à la valeur frontière ont le même nombre de prédécesseurs stricts. Les éliminer selon leur ordre mémoire serait faux.

Même règle au côté négatif :

```text
mu >= theta_minus.
```

La réalisation naturelle est donc :

1. premier balayage éventuel : compter `p` ;
2. deuxième balayage : calculer les deux seuils bornés ;
3. troisième balayage : conserver les groupes satisfaisant les seuils, égalités incluses.

Le cover est déjà matérialisé. Trois balayages linéaires coûtent moins qu'un tri rationnel complet, découverte qui n'a visiblement pas encore été brevetée.

---

## 4. Un groupe de même `mu` est une seule BallKey

Pour un seed fixé, la sphère est définie par

```text
Phi(z;mu) = 0.
```

Donc

```text
mu_y1 = mu_y2  =>  même sphère  =>  même BallKey primitive.
```

C'est le raccord essentiel avec la nouvelle architecture.

Après la coupe bornée, il faut émettre **une seule BallCandidate par groupe exact de même `mu`**, et non une candidate par complétion.

Le nombre de groupes émis par seed est alors borné par

```text
2 (h_4 - p).
```

Au profil maximal : au plus seize. Au profil `K_max=5` : au plus six.

### 4.1 Former la sphère depuis un représentant

Pour chaque groupe retenu, choisir un représentant `y` avec `B_y != 0`. Le quadruplet `(a,b,x,y)` est non coplanaire, donc `q4_form` forme exactement sa sphère, même si ce quadruplet particulier n'est pas un support q4 minimal.

On peut donc :

```text
former q4_form,
former BallKey et niveau,
émettre la boule,
laisser le census I_B/U_B et center_in_conv décider si elle porte un plateau.
```

Il n'est pas nécessaire d'exiger à ce stade :

```text
q4_center_strictly_inside,
tetra_owned_by.
```

Ces prédicats restent utiles comme filtres de coût, mais pas comme autorité une fois qu'on émet par BallKey. Les omettre peut créer quelques boules sans plateau ; cela ne peut pas en perdre, et le préfiltre exact les éliminera souvent.

Si vous choisissez de conserver les prédicats de support, il ne faut jamais tester un seul représentant arbitraire puis supprimer tout le groupe. Un autre point du même groupe peut former le support minimal recherché. Il faut alors parcourir le groupe jusqu'à trouver un représentant valide.

### 4.2 RLE inter-seeds et inter-lanes

La déduplication globale actuelle reste nécessaire : une même sphère peut être proposée par plusieurs seeds, plusieurs ancres et une lane d'arité plus petite.

La nouvelle chaîne q4 devient :

```text
ancre survivante
  -> seeds aigus
  -> sélection axiale bornée par seed
  -> une BallKey par groupe de mu retenu
  -> RLE global inter-lanes
  -> préfiltre exact de profondeur
  -> census complet des seules survivantes.
```

---

## 5. Pseudo-code recommandé

```cpp
for (seed x : acute_seeds(anchor)) {
  const Q3Form f3 = q3_form(a, b, x);
  const P3 n = cross(b-a, x-a);

  uint64_t p = 0;
  for (z : cover) {
    if (z in {a,b,x}) continue;
    const i128 A = q3_power(f3, z);
    const i64 B = dot(n, z-a);
    if (B == 0 && A < 0) ++p;
  }
  if (p >= h4) continue;

  const uint64_t k = h4 - p;
  BoundedKSmallest positive(k);
  BoundedKLargest negative(k);

  for (z : cover) {
    if (z in {a,b,x}) continue;
    const AxialSite s = make_axial_site(z);
    if (s.B > 0) positive.insert(s);
    if (s.B < 0) negative.insert(s);
  }

  const auto theta_pos = positive.threshold();
  const auto theta_neg = negative.threshold();
  SmallExactMuGroups groups;  // au plus 2k groupes distincts

  for (z : cover) {
    const AxialSite s = make_axial_site(z);
    if (s.B > 0 && mu(s) <= theta_pos) groups.insert_exact(s);
    if (s.B < 0 && mu(s) >= theta_neg) groups.insert_exact(s);
  }

  for (representative y : groups)
    emit_ball(q4_form(a,b,x,y));
}
```

Les comparaisons `<=`, `>=` et l'égalité de groupe utilisent uniquement les produits croisés exacts existants. Aucun `double` ne décide.

Pour réduire les balayages, `A,B` peuvent être calculés une fois et stockés en SoA seed-local. Mais il faut mesurer : stocker tout le cover pour économiser deux produits peut coûter davantage que les recalculer. Les caches humains et les caches matériels partagent rarement les mêmes goûts.

---

## 6. Portes qui jugent vraiment cette optimisation

### 6.1 Vérité fonctionnelle appariée

Sur les nuages bornés déjà jugés :

```text
baseline q4 énumérée
contre
q4 axial borné.
```

Comparer après RLE et aval exact :

```text
BallKeys survivantes,
I_B/U_B,
ForestEvents,
ComponentDelta,
RenderResult.
```

Ne pas exiger l'égalité des candidats bruts : le but est précisément de ne plus les produire.

### 6.2 Fixture de frontière serrée

Conserver `fixture_tight20` et le mutant « un groupe trop court ». Elle prouve que la borne `h_4-p` est serrée.

### 6.3 Fixture de groupe ex aequo

Construire plusieurs complétions distinctes du même seed avec le même `mu` :

```text
plusieurs points de coquille,
une seule BallKey émise,
tous les points retrouvés dans U_B par le census.
```

Mutant :

```text
axial-drop-boundary-ties
```

Il remplace `<=` par `<` ou `>=` par `>` et doit perdre la boule de frontière.

Ajouter aussi l'invariant :

```text
same_mu => same_BallKey
```

sur tous les groupes des petits juges.

### 6.4 Le chemin sans filtre n'est pas un mutant

Désactiver l'axial doit produire exactement le même résultat, seulement plus de candidats. Ce chemin est une baseline, pas un mutant à code 4.

Les mutants de correction sont :

```text
un groupe trop court,
suppression des ties de frontière,
comparateur tronqué,
groupe supprimé après un représentant non centré.
```

### 6.5 Compteurs, pas seuil de temps en CTest

Publier :

```text
seeds,
sites axiaux balayés,
comparaisons U192,
groupes retenus,
BallKeys q4 émises,
ratio par rapport aux complétions énumérées,
temps de génération,
temps RLE,
temps préfiltre.
```

Une porte de correction ne doit pas dépendre du temps d'une machine partagée. Un reçu de performance, lui, peut fixer un plancher de réduction de candidats sur des familles déterministes.

---

## 7. Ordre d'exécution

Je déconseille une campagne complète `n=8000` avec les 6,86 millions de boules déjà observées à `n=400`. Elle fournirait surtout une mesure précise de notre capacité collective à attendre.

Ordre recommandé :

1. intégrer la sélection bornée dans `ball_stream.hpp`, derrière un flag apparié ;
2. juger à `n<=120` contre la baseline brute ;
3. comparer bit à bit l'aval à `n=400` ;
4. mesurer `n=400,800,1600` sur quelques familles et les deux profils `K_max=5/10` ;
5. lancer `n=8000` seulement si le nombre de BallKeys q4 suit désormais le nombre de seeds avec un facteur borné raisonnable.

Il n'est pas nécessaire d'ajouter une pré-clé approchée avant cette expérience. Avec `k<=8`, la sélection exacte bornée réduit déjà de plusieurs ordres de grandeur le nombre de comparaisons par rapport au tri complet. L'approximation certifiée ne devient pertinente que si les compteurs montrent que les produits croisés exacts restent dominants.

---

## Conclusion

Votre diagnostic est bon : le poste dominant est le nombre de boules q4, et non le census complet désormais réduit à environ 2 % des clés.

La réponse n'est pas un nouveau voisinage universel volumique. Le certificat exact disponible est le rang axial seed-local. Mais sa mise en œuvre doit être adaptée au nouvel objet :

```text
sélection des k<=8 statistiques extrêmes,
préservation des ties,
une BallKey par groupe exact de mu,
pas de tri complet,
pas de candidate par complétion.
```

C'est la voie que je recommande avant l'échelle. Elle conserve la preuve actuelle, exploite directement le quotient par boule et transforme la borne théorique « seize groupes par seed » en une vraie borne sur le flux aval, au lieu de la laisser comme décoration dans `MATHEMATIQUES.md`.