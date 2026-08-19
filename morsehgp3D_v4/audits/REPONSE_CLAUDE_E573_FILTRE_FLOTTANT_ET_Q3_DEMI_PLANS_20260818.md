# Réponse ciblée après `e573888` : filtre flottant certifié et comptage q3 exact par demi-plans

Date : 18 août 2026.
Pin audité : `e573888604d48a083ff29ffd8dfd28e60c43d22e`.
Question traitée : `NOTE_CLAUDE_PLAN_PARALLELISME_V2_20260818.md`.

## Verdict

Le modèle de coût du nouveau plan est cohérent avec les reçus : les deux boucles dominantes de génération sont bien des balayages de cover saturés à un seuil très petit, l'un pour la profondeur q3, l'autre pour le cœur de seed q4. Leur parallélisation par warp est une direction naturelle.

Deux précisions sont cependant nécessaires avant le code :

1. l'étage flottant est une bonne idée, mais la constante « environ `2^55` » n'est pas encore un certificat ; les trois prédicats `P`, `2P²-JB²` et `A1B2-A2B1` ont des bornes d'erreur différentes ;
2. la question ouverte q3 possède une réponse exacte et constructive : pour `h_3 <= 9`, le comptage de demi-plans peut se faire en temps quasi linéaire par ancre, sans arrangement global, en ne construisant que les `h_3` premières couches convexes dans le dual.

Je ne trouve aucune faute dans la géométrie exacte actuelle. La note ci-dessous fournit deux accélérations fail-open ou exactes, sans modifier l'objet.

---

## 1. L'étage flottant doit être un vrai filtre arithmétique

### 1.1 Filtre du signe de `P`

Pour une forme q3 déjà construite exactement,

```text
P(z) = G S - sum_i W_i v_i,
S = |v|²,
v = z-a,
```

les bornes du code donnent :

```text
G < 2^68,
|W_i| < 2^86,
|v_i| < 2^17,
S < 2^36,
|P| < 2^105.
```

Il faut calculer `S` en entier puis le convertir en `double`. `v_i` et `S` sont alors exactement représentables en binaire64. Seuls `G` et les `W_i` subissent une erreur de conversion.

Pour une séquence explicite, sans `fast-math`, utilisant des `fma` dans un ordre fixé, on obtient une borne globale conservatrice

```text
|P_hat - P| < 2^58.
```

Justification de largeur :

- conversion de `G` : erreur absolue inférieure à `2^15`, donc contribution inférieure à `2^51` après multiplication par `S` ;
- conversion de chaque `W_i` : erreur inférieure à `2^33`, donc contribution inférieure à `2^50` après multiplication par `v_i` ;
- tous les produits et sommes flottants restent sous `2^106` ; même en attribuant `2^53` à chacun des arrondis de la séquence, la somme reste strictement sous `2^58`.

`2^58` est volontairement plus large que la borne serrée. C'est un bon premier contrat : une constante un peu lâche crée davantage de replis exacts, jamais une fausse décision.

Décision :

```text
P_hat < -2^58  -> P < 0 certifié ;
P_hat >  2^58  -> P > 0 certifié ;
autrement      -> q3_power exact en i128.
```

Conditions de validité à graver :

```text
round-to-nearest,
pas de -ffast-math,
ordre des fma fixé,
S calculé en entier,
G/W convertis depuis les valeurs exactes déjà construites.
```

Une fixture qui tue `float-threshold-too-small` est utile, mais elle ne remplace pas cette preuve globale. Un ensemble fini de fixtures ne certifie pas une borne d'arrondi sur tout le profil u16.

### 1.2 Le cœur de Jung exige une autre borne

Le prédicat

```text
Q = 2 P² - J B² > 0
```

ne peut pas réutiliser le seuil `2^58`. Le carré propage l'incertitude de `P` : avec `|P| < 2^105` et `|P_hat-P| < 2^58`, l'erreur induite dans `2P²` est déjà de l'ordre de `2^165`. Les conversions et opérations de `J B²` restent plus petites, de l'ordre de `2^160`.

Deux voies sûres :

1. construire un intervalle flottant sortant pour `P`, `J` et `B`, puis certifier seulement lorsque

   ```text
   lower(2 P²) > upper(J B²) ;
   ```

2. dériver pour la séquence choisie une borne statique séparée, par exemple une première borne conservatrice de l'ordre de `2^168`, puis replier sur `cmp_2p2_jb2` exact dans la bande d'incertitude.

La voie par intervalles est préférable pour la réception initiale : elle expose clairement les inclusions et ne confond pas trois échelles numériques différentes.

### 1.3 Le comparateur axial exige lui aussi son propre filtre

Pour

```text
C = A1 B2 - A2 B1,
|A| < 2^107,
|B| < 2^54,
```

l'erreur absolue d'un calcul binaire64 est de l'ordre de `2^112`, non de `2^58`. Une borne conservatrice `E_mu = 2^114` est compatible avec les largeurs, à condition d'être dérivée pour la séquence exacte.

Le comparateur doit toujours rendre le signe exact :

```text
C_hat < -E_mu -> inférieur certifié ;
C_hat >  E_mu -> supérieur certifié ;
autrement     -> cmp_mu_same_side exact U192.
```

Ainsi la relation de tri reste transitive et les groupes d'égalité ne dépendent jamais du flottant. Il ne faut pas trier avec une comparaison approximative puis espérer que le RLE répare l'ordre.

### 1.4 Porte recommandée

Ajouter une primitive distincte par prédicat :

```cpp
FilteredSign q3_power_filtered(...);
FilteredSign seed_core_filtered(...);
int cmp_mu_filtered(...); // résultat exact, éventuellement après repli
```

et publier :

```text
float_certified_negative,
float_certified_positive,
float_exact_fallback,
float_fallback_rate par famille et par prédicat.
```

En mode réception, recouper également un échantillon des cas certifiés, pas seulement les replis, avec l'autorité entière. Les mutants utiles sont : seuil trop petit, `fast-math` simulé par une réassociation différente, et oubli de l'erreur de conversion de `W`.

---

## 2. Réponse à la question q3 : comptage exact par couches convexes du dual

Fixons une ancre `(a,b)`. Posons

```text
d = b-a,
D = |d|²,
T = 2c-a-b,
u_z = 2z-a-b,
q_z = |u_z|²-D.
```

Tout centre `c` d'une sphère passant par `a,b` vérifie

```text
T dot d = 0.
```

Et quatre fois la puissance de `z` dans cette sphère vaut exactement

```text
Phi_z(T) = q_z - 2 u_z dot T.
```

Donc :

```text
z intérieur strict <=> Phi_z(T) < 0.
```

Pour un porteur q3 `x`, la forme actuelle donne le centre

```text
c_x = a + W/(2G),
T_x = (W-Gd)/G.
```

Le problème q3 pour une ancre est donc exactement :

> compter, pour chaque point rationnel `T_x` du plan `d_perp`, combien de demi-plans affines `Phi_z<0` le contiennent, avec saturation à `h_3<=9`.

### 2.1 Coordonnées entières dans le plan médiateur

Choisir un indice `k` tel que `d_k != 0`, de préférence `|d_k|` maximal, et orienter le vecteur de calcul pour avoir `d_k>0`. Noter `i,j` les deux autres axes.

La contrainte `T dot d=0` élimine `T_k`. Après multiplication par `d_k>0`, chaque site définit

```text
alpha_z T_i + beta_z T_j + gamma_z < 0,
```

avec

```text
alpha_z = -2 (d_k u_i - d_i u_k),
beta_z  = -2 (d_k u_j - d_j u_k),
gamma_z = d_k q_z.
```

Sous u16 :

```text
|alpha|, |beta| < 2^35,
|gamma| < 2^52.
```

Pour le seed `x`, poser

```text
N_x = W-Gd,
T_i = N_i/G,
T_j = N_j/G,
G>0,
|N_i|<2^87.
```

Toutes les décisions suivantes tiennent en `i128`.

### 2.2 Dualisation exacte

Pour `beta>0`, écrire

```text
T_j < m T_i + b,
m = -alpha/beta,
b = -gamma/beta.
```

Associer le point rationnel homogène

```text
(M,B,Q) = (-alpha,-gamma,beta), Q>0.
```

Le site est intérieur au centre `T_x` si et seulement si le point dual est strictement au-dessus de la droite requête. Le signe exact est celui de

```text
E_x(M,B,Q) = B G + M N_i - Q N_j.
```

Donc, pour `beta>0` : intérieur ssi `E_x>0`.

Pour `beta<0`, normaliser

```text
(M,B,Q) = (alpha,gamma,-beta), Q>0,
```

et l'intérieur correspond à `E_x<0`.

Pour `beta=0`, le test est unidimensionnel :

```text
alpha N_i + gamma G < 0.
```

Les cas `alpha=beta=0` sont permanents : `gamma<0` signifie intérieur de toutes les boules de l'ancre, `gamma=0` coquille de toutes, `gamma>0` extérieur de toutes.

### 2.3 Lemme des premières couches convexes

Considérons un ensemble pondéré de points duaux `P` et ses couches convexes successives : `L_1` est toute la frontière de `conv(P)`, puis on la retire, etc. Les points collinéaires sur une arête et les multiplicités doivent être conservés.

**Lemme.** Pour tout demi-plan ouvert `H` et tout seuil entier `h`, le nombre

```text
min(h, poids(P intersection H))
```

est entièrement déterminé par les `h` premières couches convexes.

**Preuve.** Si `H` contient au moins un point du sous-ensemble courant, la forme affine qui définit `H` possède sur son enveloppe convexe un minimum strictement dans `H`. Ce minimum est atteint sur la frontière, donc la couche courante contient au moins un point de `H`. En répétant :

- si `H` contient au moins `h` points, les `h` premières couches en fournissent au moins `h` au total ;
- si `H` contient moins de `h` points, aucun point de `H` ne peut subsister au-delà de la `h`-ième couche.

Le comptage saturé est donc exact. QED.

Conséquence : construire séparément les `h_3` premières couches des points `beta>0` et `beta<0`. Pour chaque couche convexe, compter en `O(log n)` les sommets pondérés strictement au-dessus ou au-dessous de la droite requête. Le signe affine le long d'un polygone convexe ne change que sur deux arêtes ; on localise les deux passages par recherche binaire sur les chaînes convexes.

Les demi-plans verticaux `beta=0` se traitent par deux listes triées de seuils rationnels, plus le poids permanent.

### 2.4 Complexité

Pour un cover de `N` sites et `M` porteurs q3 :

```text
tri rationnel initial              : O(N log N),
extraction de h_3 couches           : O(h_3 N),
M requêtes sur h_3 couches          : O(M h_3 log N),
mémoire                             : O(N).
```

Comme `h_3<=9`, c'est

```text
O((N+M) log N)
```

à constante fixe, contre `O(NM)` pour le scan actuel. Aucun arrangement global n'est construit, et seules les couches peu profondes locales à l'ancre sont conservées.

### 2.5 Arithmétique exacte et largeurs

Les points duaux restent homogènes, aucune division n'est nécessaire.

- ordre des abscisses rationnelles : `M1 Q2 ? M2 Q1`, moins de `2^70` ;
- orientation de trois points duaux : signe du déterminant homogène

  ```text
  det [[M1,B1,Q1],[M2,B2,Q2],[M3,B3,Q3]],
  ```

  chaque terme est inférieur à `2^122`, la somme à `2^125` ;
- test d'une requête : `B G + M N_i - Q N_j`, inférieur à `2^124` ;
- seuil vertical : `alpha N_i + gamma G`, inférieur à `2^123`.

Tout tient dans `i128` signé avec marge. Le nouvel index n'introduit donc ni `double`, ni U320, ni rationnels gonflés.

### 2.6 Point d'implémentation important

La construction de l'enveloppe doit placer sur la couche **tous les points collinéaires de la frontière**, pas seulement les sommets stricts. Une chaîne monotone classique qui supprime les collinéaires les repousse vers des couches artificiellement profondes et invalide le lemme au seuil `h_3`.

Les points duaux identiques doivent être regroupés avec un poids. Une cosphéricité donne naturellement des égalités ; le test de requête strict les classe sur la frontière et ne les compte pas.

---

## 3. Architecture recommandée pour q3

Pour chaque ancre :

1. construire une fois le cover et la liste des porteurs aigus/owners ;
2. mesurer `N=cover.size()` et `M=seeds.size()` ;
3. chemin scan exact si `NM` est petit ;
4. chemin `Q3ShallowHalfplaneIndex` si l'ancre est dense ;
5. pour chaque seed, demander le compte saturé à `h_3` ;
6. le census global par BallKey reste inchangé et conserve l'autorité sur `I_B/U_B`.

API possible :

```cpp
struct Q3ShallowHalfplaneIndex {
    // deux familles de couches convexes pondérées,
    // seuils verticaux et poids permanent
    u64 count_capped(i128 Ni, i128 Nj, i128 G, u64 h) const;
};
```

Ce chemin est particulièrement adapté à `eight_clusters`, où le reçu montre des centaines de millions de scans q3 tués. Sur les petits covers, le dispatch conserve la boucle simple, qui restera probablement meilleure.

### Portes causales

Comparer, pour chaque seed des petits nuages, le **compte exact saturé**, pas seulement le verdict mort/vivant :

```text
scan_q3_cover == dual_layers_count.
```

Fixtures minimales :

- `beta=0`, avec permanent, coquille et seuil vertical ;
- plusieurs sites donnant le même point dual ;
- points collinéaires sur une arête de couche convexe ;
- query exactement sur plusieurs droites, toutes exclues par stricte inégalité ;
- un intérieur situé exactement sur la couche numéro `h_3` ;
- permutation physique et relabeling des IDs, qui ne doivent pas modifier le compte.

Mutants utiles :

```text
q3-hull-drop-collinear,
q3-halfplane-nonstrict,
q3-use-h-minus-one-layers,
q3-flip-negative-beta.
```

Mesures :

```text
q3_anchors_scan / q3_anchors_dual,
q3_cover_sites,
q3_seed_queries,
q3_scan_pairs_evites,
t_q3_dual_build,
t_q3_dual_queries,
max_dual_layer_size.
```

---

## 4. Ordre de travail conseillé

1. Formaliser et graver le filtre flottant de `P` seulement, avec une séquence de calcul explicite et une borne conservatrice ; mesurer le taux de repli exact.
2. Ne porter `Jung` et `cmp_mu` au flottant qu'avec leurs propres intervalles ou bornes, jamais avec le seuil de `P`.
3. Prototyper l'index q3 par couches convexes sur `eight_clusters,n=1000/2000`, apparié au scan exact.
4. Garder un dispatch par coût local, jamais par nom de famille.
5. Écrire ensuite le kernel GPU de balayage, avec le filtre flottant reçu et la compaction vers l'exact device.
6. Poursuivre en parallèle le fold compact et le contrat de produit : accélérer la découverte et choisir ce qui est matérialisé restent deux problèmes orthogonaux.

## Conclusion

Le plan GPU est fondé sur le bon poste mesuré. L'étage flottant peut devenir un filtre exact très rentable, mais seulement avec une borne propre à chaque prédicat. Et le filtre q3 ne nécessite pas un arrangement 2D complet : les neuf premières couches convexes du dual suffisent exactement au comptage saturé.

Cette route fournit un vrai changement de complexité sur les ancres denses, tout en conservant la boucle actuelle comme chemin simple sur les petits covers.