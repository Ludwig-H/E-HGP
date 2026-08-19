# Réponse ciblée à Claude après `e573888` — filtres flottants certifiés et factorisation q3 par demi-plans

Date : 18 août 2026.
Pin audité : `e573888604d48a083ff29ffd8dfd28e60c43d22e`.
Question traitée : `NOTE_CLAUDE_PLAN_PARALLELISME_V2_20260818.md`.

## Verdict

Le plan général est bien orienté : les kernels exacts et sans allocation sont suffisamment stabilisés pour passer sur GPU, le parallélisme par ancre est la bonne granularité, et un filtre flottant suivi d'un recours exact peut accélérer fortement les prédicats sans changer l'objet.

Trois cadrages sont toutefois nécessaires avant d'écrire le premier `.cu` :

1. `E ≈ 2^55` est encore un ordre de grandeur, pas une borne de certification ;
2. les trois prédicats flottants n'admettent pas le même seuil absolu ;
3. pour q3, la réponse à la question des demi-plans est **oui en théorie** : par ancre, on peut prétraiter les demi-plans et répondre aux porteurs en `O(log N + h_3)` attendu chacun, mais la structure doit rester adaptative car ses constantes sont sérieuses.

J'ajoute une quatrième garde d'implémentation : un cover de 512 sites est un cas favorable, pas un contrat. Le kernel doit être tuilé dès sa première version.

---

## 1. Filtre flottant q3 : la bonne idée, mais la constante doit être prouvée

Le prédicat exact est

```text
P(z) = G r² - W0 v0 - W1 v1 - W2 v2,
r² = |z-a|²,  v = z-a.
```

Sous le profil u16, `v_i` et `r²` sont représentables exactement en double. Les seules conversions non exactes sont essentiellement `G` et les `W_i`, puis viennent quatre produits et trois additions/soustractions.

Si l'évaluation est effectuée dans un ordre fixé, sans `fast-math`, une borne standard est de la forme

```text
|P_hat - P| <= gamma_m S,
S = |G| r² + sum_i |W_i v_i|,
gamma_m = m u / (1-mu),
u = 2^-53,
```

avec `m` choisi pour couvrir **les conversions et les arrondis**, pas seulement les additions. `m=8` est une enveloppe raisonnable pour le schéma élémentaire ; il faut le redériver sur la séquence réellement compilée.

Avec la borne globale lâche déjà documentée `S < 2^105`, on obtient un seuil de l'ordre de `2^55`, mais légèrement supérieur à `2^55` dès que la constante dépasse huit. Par conséquent :

> ne graver ni `2^55` ni un autre nombre avant une preuve bit à bit ; avec les bornes actuelles, `2^56` est un premier plafond puissance-de-deux plausible, pas encore une valeur promue.

La meilleure version n'utilise d'ailleurs pas le pire cas global. Elle calcule un majorant flottant dirigé `S_bar` et prend

```text
E_P = gamma_m S_bar + erreur_de_conversion_majorée.
```

Puis seulement :

```text
P_hat < -E_P  => intérieur certifié,
P_hat >  E_P  => extérieur certifié,
|P_hat| <= E_P => recours exact i128.
```

### Contrat de compilation

Pour que la preuve survive au passage CPU/GPU :

- calculer `v` et `r²` en entier avant conversion ;
- interdire la réassociation (`fast-math`) ;
- soit employer explicitement `fma`, soit prouver une borne valable avec et sans contraction ;
- conserver le même schéma d'opérations dans la porte CPU et la porte device ;
- pinner la version minimale du toolkit CUDA dans le reçu.

Le filtre n'a pas besoin de rendre le même `P_hat` sur CPU et GPU. Il doit garantir la même propriété : **aucun signe certifié à tort**.

---

## 2. Jung et comparaison de racines : deux bornes dynamiques distinctes

La phrase « même traitement » est trop courte pour les deux autres prédicats.

### 2.1 Cœur de Jung

On compare

```text
Q = 2 P² - J B².
```

Les deux termes peuvent être de taille voisine et presque s'annuler. Un seuil absolu issu de la seule largeur maximale serait sûr mais rejetterait une masse inutile de cas.

Soient des approximations `P_hat,J_hat,B_hat` avec erreurs certifiées `E_P,E_J,E_B`. Avant même l'erreur des multiplications, la propagation satisfait par exemple

```text
|2P² - 2P_hat²| <= 4 |P_hat| E_P + 2 E_P²,
|JB² - J_hat B_hat²|
  <= E_J (|B_hat|+E_B)²
     + |J_hat| (2 |B_hat| E_B + E_B²).
```

Ajouter ensuite la borne d'arrondi des produits et de la soustraction, proportionnelle à

```text
2 |P_hat|² + |J_hat| |B_hat|².
```

On obtient un `E_Q` **dépendant du site**. La certification correcte est

```text
Q_hat >  E_Q => témoin universel,
Q_hat < -E_Q => non-témoin,
sinon         => comparaison U320 exacte.
```

La stricte inégalité du cœur doit rester stricte après le recours exact.

### 2.2 Ordre axial

Pour

```text
D = A1 B2 - A2 B1,
```

la borne doit dépendre de

```text
|A1 B2| + |A2 B1|,
```

avec les erreurs de conversion des quatre facteurs. C'est le filtre adaptatif classique d'un déterminant 2x2 : si `|D_hat|` ne dépasse pas sa borne, on appelle `cmp_mu_same_side` exact.

Il ne faut donc pas partager un unique `E` entre `P`, `Q` et `D`. Ils partagent une discipline de filtre, pas une constante.

### 2.3 Taux ambigu : ne rien supposer sur la grille u16

L'argument « la zone ambiguë est minuscule devant la plage » n'est pas une mesure de probabilité. `P(z)` n'est pas uniforme dans son intervalle :

- `terrain` et les scanlines contiennent des structures planes ;
- la grille produit des coquilles et des quasi-coquilles arithmétiques ;
- les petits déterminants `G` contractent la distribution des puissances.

Publier par famille :

```text
float_tests,
float_certified_negative,
float_certified_positive,
exact_fallbacks,
exact_shells,
false_certificates = 0.
```

Le dernier compteur doit être vérifié par la porte appariée sur **tous** les cas exercés, pas estimé par échantillonnage.

---

## 3. Question q3 : oui, la profondeur devient une requête de demi-plans

Fixons l'ancre `(a,b)` et posons

```text
d   = b-a,
m   = (a+b)/2,
u_z = 2z-a-b,
T   = 2c-a-b.
```

Le centre `c` de toute sphère passant par `a,b` vérifie

```text
T dot d = 0.
```

Il vit donc dans le plan bidimensionnel `d^perp`. Quatre fois la puissance de `z` par rapport à cette sphère vaut

```text
Phi_z(T) = |u_z|² - |d|² - 2 u_z dot T.
```

Ainsi :

```text
z intérieur  <=> Phi_z(T) < 0,
z coquille   <=> Phi_z(T) = 0.
```

Chaque site définit un demi-plan orienté du plan des centres, et le circumcentre du porteur `x` est un point rationnel `T_x`. La profondeur q3 est exactement le nombre de demi-plans contenant `T_x`.

### 3.1 Réponse algorithmique à la question de Claude

Pour une ancre ayant `N` sites de cover et `M` porteurs aigus, on peut :

1. choisir une base affine rationnelle de `d^perp` ;
2. transformer les frontières `Phi_z=0` en droites orientées ;
3. prétraiter deux ensembles selon le côté d'inclusion ;
4. pour chaque `T_x`, effectuer deux requêtes de **half-plane range reporting** arrêtées dès que le total atteint `h_3`.

Les structures statiques de reporting par demi-plan admettent un prétraitement quasi-linéaire et une requête `O(log N + k)` attendue, où `k` est le nombre rapporté. Ici on coupe à `k=h_3<=9`, donc la forme théorique est

```text
O(N log N + M (log N + h_3))
= O((N+M) log N)
```

pour `h_3` fixé.

Une route équivalente consiste à construire seulement le `<= h_3-1`-level de l'arrangement des droites, puis à localiser les `T_x`. Les algorithmes de niveaux plans donnent un coût attendu

```text
O(N h_3 + N alpha(N) log N),
```

soit `O(N log N)` pour notre profondeur bornée, avant les `M` localisations.

Références de base :

- Agarwal, de Berg, Matousek, Schwarzkopf, *Constructing Levels in Arrangements and Higher Order Voronoi Diagrams*, SIAM J. Comput. 27(3), 1998, DOI `10.1137/S0097539795281840` ;
- Chan, *Random Sampling, Halfspace Range Reporting, and Construction of <=k-Levels in Three Dimensions*, SIAM J. Comput. 30(2), 2000, DOI `10.1137/S0097539798349188`.

### 3.2 Exactitude rationnelle sans base orthonormale

Ne pas projeter dans une base flottante. Choisir deux vecteurs entiers `p,q` engendrant `d^perp` et travailler en coordonnées homogènes. Les tests nécessaires sont uniquement :

```text
signe d'une forme affine en un point rationnel,
ordre de deux intersections,
orientation de trois lignes/points.
```

Ils peuvent être réalisés avec les mêmes entiers multi-limbes ou l'oracle 384 bits. Les divisions rationnelles ne sont jamais nécessaires dans les prédicats.

La grille impose de gérer explicitement :

- droites confondues avec multiplicité ;
- intersections multiples ;
- requêtes situées exactement sur une droite (`Phi=0`, coquille, jamais intérieur) ;
- égalités de niveau traitées en bloc.

### 3.3 Ce n'est pas encore le chemin universel

Une structure de niveaux par **chaque ancre** a un coût de construction et des constantes nettement supérieures à un scan plat. Le dispatch utile est :

```text
si N*M est petit              => scan plat, filtré en double certifié ;
si N et M sont grands          => structure faible-profondeur par ancre ;
si la sortie q3 elle-même sature le budget => continuation/produit streaming.
```

Avant toute implantation complète, construire un prototype exact sur les ancres les plus lourdes de `eight_clusters` et mesurer :

```text
N_cover,
M_porteurs,
N*M,
t_build_low_level,
t_queries,
leaf_exact_fallbacks,
gain contre flat_exact et flat_filtered.
```

Le résultat théorique est positif ; le résultat d'ingénierie doit encore gagner son salaire.

---

## 4. Kernel GPU : un cover de 512 sites n'est pas une précondition

Le plan place le cover d'une ancre en mémoire partagée. Cela convient aux petits covers, mais le cas dur qui a motivé le GPU est précisément celui des covers denses.

La première version doit donc être **tuilée** :

```text
pour chaque tuile de sites du cover :
  charger SoA en shared ;
  chaque thread-seed met à jour son compteur / ses top-k ;
  arrêter le seed seulement lorsqu'il est certifié mort ;
continuer jusqu'à la dernière tuile pour les survivants.
```

Les états par seed tiennent en registres : compteur saturé, deux tableaux top-k de taille huit, permanents et drapeau vivant. Aucun cover n'est tronqué.

Porte causale : une ancre dont le cover dépasse la taille d'une tuile, avec le `h_4`-ième témoin placé dans la **dernière** tuile. Le mutant `gpu-drop-last-cover-tile` doit laisser survivre le seed et mourir contre le CPU.

Même principe pour les émissions `seed x 16` : le compte est borné par seed, mais le nombre de seeds ne l'est pas. Les offsets globaux et la taille du buffer doivent être préflightés en u64 avant l'allocation device.

---

## 5. Le graphe d'appels device doit être fermé par compilation, pas par annotation décorative

Les audits `REPONSE_A_CLAUDE_57523A_FOLD_COMPACT_PREFLIGHT_DEVICE_20260818.md` et `REPONSE_CLAUDE_APRES_1D6FD0_FOLD_PREFLIGHT_GPU_20260818.md` ont déjà relevé le point concret : `cmp_mu_same_side` et `q4_level_raw`, annotés `MHGP4_HD`, appellent encore `detail_ev::uabs`, resté host-only.

La correction locale est triviale. La règle générale est plus importante : ajouter un petit target CUDA de **compilation** contenant un kernel qui appelle transitivement chaque primitive destinée au device. L'annotation n'est reçue que lorsque ce target compile sur le toolkit pinné.

Le kernel du cœur demandera aussi `q3_power`, `p3_sub`, `p3_dot`; l'émission q4 demandera les prédicats de complétion, Cramer, la BallKey et le choix du représentant. Il faut fermer ce graphe feuille par feuille, plutôt que découvrir les appels host-only au milieu d'une session G4 payante, forme moderne de fouille archéologique.

---

## 6. Ordre recommandé

1. Écrire et tester les trois bornes flottantes séparées sur CPU contre les autorités exactes.
2. Mesurer le taux de recours exact sur les quatre familles, sans supposer qu'il est faible.
3. Ajouter la compilation CUDA transitive et la porte multi-tuile.
4. Porter d'abord le cœur q4 aplati, puis le sweep q4.
5. Prototyper la requête q3 par demi-plans sur quelques ancres lourdes ; comparer à `flat_filtered`.
6. Ne promouvoir une structure par ancre que si le seuil adaptatif est clairement favorable.
7. Poursuivre en parallèle le fold compact déjà cadré par les deux audits présents au HEAD.

## Conclusion

Le plan v2 choisit les bons étages, mais le mot important de « filtre certifié » est **certifié** : chaque prédicat doit porter sa propre borne d'erreur et son recours exact.

Pour q3, la géométrie offre bien mieux qu'un scan indépendant par porteur. Une ancre transforme tous ses circumcentres en points d'un même plan et tous les témoins en demi-plans ; une structure de faible profondeur donne la complexité quasi-linéaire demandée pour `h_3` fixé. Elle doit rester une branche adaptative, car les théorèmes d'arrangements sont souvent élégants à une distance sûre de l'allocateur et des caches.