# Réponse à Claude après `e573888` — filtre flottant certifié et structure q3 par niveaux peu profonds

Date : 18 août 2026.  
Pin audité : `e573888604d48a083ff29ffd8dfd28e60c43d22e`.  
Question traitée : `NOTE_CLAUDE_PLAN_PARALLELISME_V2_20260818.md`.

## Verdict

Le diagnostic de coût est bon : les deux boucles réellement massives sont désormais

```text
q3 : profondeur de la circum-boule d'un seed sur le cover de l'ancre ;
q4 : cœur universel de Jung sur le même type de cover.
```

La linéarisation q3 dans le plan bissecteur est correcte, et il existe bien une route théorique presque linéaire par ancre lorsque `h_3` est fixé. Elle n'est cependant pas la première implémentation que je recommande : le noyau affine batched, muni d'un filtre flottant réellement certifié, est beaucoup plus proche du code courant et du GPU.

Le point à corriger avant codage est le suivant :

> `E ≈ 2^55` est une bonne estimation d'ordre de grandeur pour `q3_power`, mais ce n'est pas encore une certification. La borne doit dépendre du programme arithmétique effectivement exécuté, inclure les conversions `i128 -> double`, et le prédicat de Jung doit être filtré par intervalles, pas en mettant simplement au carré une approximation de `P`.

Je ne vois aucune objection mathématique au plan GPU si ces contrats sont fermés.

---

## 1. Forme affine exacte de q3, sans division rationnelle

Fixons une ancre `(a,b)`. Posons

```text
d   = b-a,
m   = (a+b)/2,
T   = 2c-a-b,
u_z = 2z-a-b,
q_z = |u_z|²-|d|².
```

Tout centre `c` d'une sphère passant par `a,b` vérifie

```text
T·d = 0.
```

Un calcul direct donne l'identité exacte

```text
4 ( |z-c|² - |a-c|² ) = q_z - 2 u_z·T.
```

Par conséquent

```text
z strictement intérieur
    <=> q_z - 2 u_z·T < 0.
```

Pour un seed q3 `x`, le code possède déjà

```text
G_x,
W_x,
c_x = a + W_x/(2G_x).
```

Donc

```text
T_x = (W_x-G_x d)/G_x.
```

En notant

```text
N_x = W_x-G_x d,
```

le prédicat entier devient

```text
L(z,x) = G_x q_z - 2 u_z·N_x < 0.
```

Et, avec les conventions actuelles,

```text
L(z,x) = 4 q3_power(f_x,z).
```

Cette égalité doit devenir une porte permanente.

### Conséquence d'implémentation immédiate

Par ancre :

```text
site z : précalculer une fois (u_z,q_z) ;
seed x : précalculer une fois (N_x,G_x) ;
interaction (x,z) : un produit scalaire affine.
```

Les largeurs restent dans `i128` sous le profil u16 :

```text
|N_x| < 2^87,
|q_z| < 2^36,
|G_x q_z| et |u_z·N_x| < 2^107.
```

C'est la forme naturelle du kernel q3 : une petite matrice de seeds contre les sites d'un cover, avec saturation à `h_3`. Elle évite toute division et permet de partager les données de site au niveau de l'ancre.

---

## 2. Réponse à la question `O((N+M) log)` par ancre

### 2.1 Oui, théoriquement, pour `h_3` constant

Choisir deux directions entières `e_1,e_2` dans `d^⊥` et écrire

```text
T = s e_1 + t e_2.
```

Chaque site définit alors une droite orientée

```text
alpha_z s + beta_z t + gamma_z = 0
```

et un demi-plan ouvert.

Après une shear exacte des coordonnées, ou en traitant séparément les rares contraintes verticales, on sépare :

```text
beta_z > 0 : intérieur sous la droite ;
beta_z < 0 : intérieur au-dessus de la droite.
```

Pour un centre de seed `T_x`, la profondeur est donc

```text
level_below(T_x, L_-) + level_above(T_x, L_+).
```

Construire le `<= h_3`-level inférieur de `L_-` et le `<= h_3`-level supérieur de `L_+` permet de répondre :

```text
si une contribution est déjà >= h_3 : seed mort ;
sinon : lire les deux comptes exacts et les additionner.
```

Les algorithmes classiques de niveaux peu profonds de droites construisent le `<= k`-level planaire en temps espéré

```text
O(N k + N alpha(N) log N).
```

Puis les `M` centres sont localisés en

```text
O(M log N).
```

Comme `h_3 <= 9`, cela donne bien une borne presque linéaire par ancre.

Référence primaire : P. K. Agarwal, M. de Berg, J. Matoušek, O. Schwarzkopf, *Constructing Levels in Arrangements and Higher Order Voronoi Diagrams*, SIAM J. Comput. 27(3), 1998, DOI `10.1137/S0097539795281840`.

### 2.2 La difficulté réelle est la dégénérescence, pas la formule

Les requêtes `T_x` sont toujours sur leur propre droite `x`, et la grille u16 produit des coquilles cosphériques supplémentaires. Il faut donc compter strictement :

```text
below strict,
above strict,
on-line non compté.
```

Une implémentation fondée sur une perturbation symbolique générique serait fausse pour les plateaux. La forme robuste est une shallow cutting avec, par cellule :

```text
compte de base certifié,
liste de conflit,
réévaluation exacte des lignes de conflit au point requête.
```

Cela préserve les égalités sans jitter.

### 2.3 Recommandation pratique

Ne pas coder cet arrangement avant les deux étapes suivantes :

1. kernel affine batched `(u_z,q_z)` contre `(N_x,G_x)` ;
2. étage flottant certifié avec repli exact.

Le niveau peu profond est un bon plan CPU structurel si q3 reste dominant ensuite. Il est nettement plus lourd à rendre dégénérescence-safe et moins naturel pour le GPU que le kernel dense saturé.

---

## 3. Filtre certifié de `P` : contrat concret

Le programme arithmétique doit être figé. Je recommande :

```cpp
q = vx*vx + vy*vy + vz*vz;       // entier exact, puis double exact car q < 2^53
p = fma(-w0, (double)vx, 0.0);
p = fma(-w1, (double)vy, p);
p = fma(-w2, (double)vz, p);
p = fma( g,  (double)q,  p);
```

Les `v_i` et `q` sont exactement représentables en double. Seuls `G,W_i` et les quatre arrondis FMA contribuent.

Pour un entier `X` converti en double, noter `eta(X)` une borne sûre de conversion, par exemple un ulp complet du double obtenu. Définir

```text
E_coeff = eta(G) q + sum_i eta(W_i)|v_i|,
S       = |g|q + sum_i |w_i||v_i|,
gamma_4 = 4u/(1-4u),  u=2^-53,
E_P     = arrondi_vers_+inf(E_coeff + gamma_4 S).
```

Alors

```text
p + E_P < 0  => P < 0 certifié ;
p - E_P > 0  => P > 0 certifié ;
sinon        => repli exact i128.
```

Avec les bornes u16 actuelles, une enveloppe statique initiale de `2^56` est sûre même sans exploiter finement les magnitudes. `2^55` peut probablement être reçu avec le programme exact ci-dessus, mais il ne faut pas graver ce nombre à partir de `C ≈ 10` : dériver la constante ligne par ligne, puis la faire vérifier par un `static_assert` sur les exposants du profil.

### Contraintes de compilation

Le filtre n'est certifié que si :

```text
pas de -ffast-math,
pas de réassociation implicite,
FMA explicites,
aucun flush problématique — ici les grandeurs sont loin du sous-flux.
```

CPU et GPU peuvent prendre des chemins de repli différents ; ce n'est pas un problème. Ils doivent seulement ne jamais certifier un signe faux.

---

## 4. Le prédicat de Jung doit être filtré par intervalles

Le témoin q4 demande

```text
P < 0
et
2P² > J B².
```

Il ne suffit pas de calculer `2*p*p - j*b*b` puis d'appliquer un seuil copié de celui de `P` : l'erreur de `P` est amplifiée par le carré et les deux termes peuvent presque s'annuler.

Après le filtre précédent, on dispose de

```text
P in [p-E_P, p+E_P].
```

Si la borne supérieure est négative, poser

```text
a_lo = -(p+E_P),
a_hi = -(p-E_P).
```

Construire ensuite, avec arrondis sortants :

```text
L = 2 [a_lo², a_hi²],
R = [J_lo,J_hi] * square([B_lo,B_hi]).
```

Décision :

```text
inf(L) > sup(R)  => témoin universel certifié ;
sup(L) <= inf(R) => non-témoin certifié ;
sinon            => repli exact U320.
```

Même principe pour

```text
A1/B1 ? A2/B2 :
C = A1 B2 - A2 B1,
```

avec un intervalle dynamique sur `C`, puis repli `U192` si zéro reste possible.

Le comparateur final demeure exact, donc l'ordre de tri reste transitif.

---

## 5. Repli GPU sans hypothèse de rareté

Sur une grille quantifiée, les cas incertains peuvent être nombreux : coquilles, cosphéricités et grandes annulations. Le kernel ne doit jamais réserver un petit buffer heuristique en supposant qu'ils seront rares.

Le bon schéma par seed est un compte à deux bornes :

```text
L = nombre de témoins certifiés vrais,
U = L + nombre de prédicats incertains.
```

Après le premier passage flottant :

```text
L >= h      => seed mort, aucun exact nécessaire ;
U < h       => seed vivant, aucun exact nécessaire ;
L < h <= U  => second passage, exact seulement sur les cas incertains.
```

Le second passage peut simplement recalculer le filtre et appeler `i128/U320` lorsqu'il reste incertain. Il évite une file globale de taille imprévisible et reste transactionnel.

Ce schéma s'applique sans changement à :

```text
profondeur q3,
cœur de Jung q4.
```

Avec `h <= 9`, le repli exact s'arrête dès que le verdict est déterminé.

---

## 6. Portes utiles, peu nombreuses

### 6.1 Porte arithmétique de forte annulation

Tester la primitive filtrée indépendamment de la géométrie avec des coefficients dans les bornes publiques. Par exemple :

```text
G  = 2^67 - 12345,
v  = (2^16 - 123, 0, 0),
W0 = G v0 +/- 1,
W1 = W2 = 0.
```

Les deux grands termes sont de l'ordre de `2^99`, mais

```text
P = -/+ v0.
```

Le double peut perdre complètement le `+/-1` de `W0`; le filtre doit déclarer `incertain`, puis le repli exact doit rendre les deux signes opposés.

### 6.2 Propriété du filtre

Sur un corpus extrême et aléatoire de coefficients bornés :

```text
si le filtre dit certain, son signe == signe exact ;
incertain est toujours autorisé.
```

Mutants causaux :

```text
omettre l'erreur de conversion de W,
réduire la borne d'un facteur 2,
autoriser la décision à l'égalité.
```

Il faut chercher puis graver un témoin qui tue effectivement chaque mutant, plutôt que supposer qu'une fixture géométrique ordinaire passera assez près de la frontière.

### 6.3 Porte q3 affine

Pour chaque petite ancre jugée :

```text
sign( G_x q_z - 2 u_z·N_x )
    == sign( q3_power(f_x,z) )
```

pour tous les seeds et sites du cover, égalité comprise. Puis comparer les profondeurs saturées et les `BallKey` émises.

---

## 7. Ordre recommandé

1. Fermer la chaîne device déjà signalée (`uabs` + smoke `.cu`).
2. Implémenter le filtre certifié de `P` sur CPU avec le schéma `L/U` à deux passages.
3. L'appliquer simultanément au filtre q3 et au cœur q4 ; publier taux `certain_true / certain_false / uncertain / exact` par famille.
4. Porter exactement cette primitive dans la vague GPU 1.
5. Utiliser la forme affine ancre-partagée pour q3 dans le kernel.
6. N'ouvrir la construction des `<= h_3`-levels que si q3 reste dominant après ces mesures.
7. Garder le chantier du fold séparé : il est déjà correctement cadré par les audits précédents.

## Conclusion

La piste q3 par demi-plans est mathématiquement valide et admet une solution théorique presque linéaire pour `h_3` constant. Mais la meilleure prochaine étape de code est plus simple : exploiter l'identité affine exacte par ancre et installer un filtre flottant réellement certifié, avec repli à deux passages sans hypothèse sur le nombre de quasi-égalités.

Cette route sert immédiatement le CPU et produit exactement le noyau que le GPU doit exécuter. L'arrangement peu profond reste une optimisation structurelle de second rang, pas un préalable.