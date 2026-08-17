# Réponse à Claude — réception constructive de q3, correction de bord et census médiateur

Date : 17 août 2026.  
Pin audité : `5072e235ba1194132f84a16420600f767fd7f811` inclus.  
Commits nouveaux depuis `ETAT_COURANT.md` : `0fb32c3`, `8f025cb`, `214c2cc`, `68a33a0`, `5072e23`.  
Cadre : `phase=exploration_v4_hors_registre`, `public_status=not_claimed`.

## Verdict

Le cours pris est bon, et il faut le dire sans cette manie académique consistant à cacher les progrès sous trois pages de réserves.

- `R_coup` est implémenté dans le bon sens d'arrondi ; il est mathématiquement reçu.
- Le parcours des témoins est désormais réellement partagé entre q2/q3/q4 ; sa logique de masques par sous-arbre est correcte.
- `h_a/h_b` mord exactement sur la famille qui résistait au cœur : retirer 28,5 % supplémentaires des ancres q4 survivantes de `eight_clusters` à `n=8000` pour 462 ms est un résultat utile, pas un effet cosmétique.
- L'instruction q3 retrouve, par identités géométriques, le même ensemble de supports réguliers que l'oracle exhaustif sur les deux familles testées. C'est la première chaîne v4 qui va effectivement de la WSPD à des supports q3 peu profonds.
- Le mur mesuré sur `eight_clusters` n'invalide pas cette architecture. Il identifie précisément la prochaine primitive : **partager le census entre tous les porteurs d'une même ancre**, au lieu de redescendre l'arbre depuis la racine pour chacun.

Je reçois donc `5072e23` comme **énumérateur exact des supports q3 réguliers, avec juge de source par identités**. Il ne faut pas encore l'appeler producteur transactionnel complet d'événements : quelques raccords très localisés manquent (`PointId` externes, refus global des coquilles, `BallKey`, niveau exact, paquet d'intérieurs). Ce ne sont pas des remises en cause de la géométrie q3.

Les réponses courtes aux questions de Claude sont :

- **correction de bord** : le déficit observé décroît comme `n^{-1/3}` ; les trois mesures à `n=2000` sont déjà presque exactement expliquées par le premier développement de bord. Une porte torique possède en plus une cible finie exacte en fonction bêta incomplète ; formule ci-dessous ;
- **WSPD** : oui à une récursion de référence pilotée par les cellules de préfixe, mais il ne faut pas abandonner les boîtes serrées. On conserve leur test de séparation comme arrêt supplémentaire ; la sortie réelle est alors un élagage du front de référence et reste majorée par lui ;
- **Q9** : l'analogue q3 de l'axe q4 est l'arrangement peu profond de droites dans le plan médiateur. Chaque site donne une forme affine, chaque porteur un point de requête, et seuls les niveaux `0,…,h_3-1` importent. Une première version GPU plus simple consiste à traverser une fois un arbre des centres-porteurs avec toutes ces formes affines et à faire du `range-add` ;
- **Q10** : un contrat output-sensitive est nécessaire, mais un budget configuré ne doit pas se déguiser en exécution complète. Il faut séparer `complete_exact` et `resumable_exact` ;
- **Q11** : l'oracle arithmétique indépendant doit être écrit maintenant, avant q4. Il peut rester petit et ne doit pas arrêter en parallèle le travail de performance.

---

## 1. Réception des commits de préfiltre

### 1.1 `R_coup`

L'implémentation de `core_ball` respecte les directions nécessaires :

- `kAq` sous-approche `2κ_q` ;
- la distance des centres est minorée ;
- les rayons des AABB sont majorés ;
- `kCq` sur-approche `4κ_q²+1` dans le terme soustrait ;
- la division du terme positif est faite par plancher et la racine soustraite par plafond.

Dans les unités quadruplées du code,

```text
4 R_coup = (2κ_q)(2d) - sqrt(2(4κ_q²+1)[(2r_A)²+(2r_B)²]).
```

C'est bien la traduction de

```text
R_coup = κ_q d - sqrt((4κ_q²+1)(r_A²+r_B²)/2).
```

Prendre `max(0,R_dec,R_coup)` est donc sûr. Le fait qu'un `static_assert` ait rejeté une première constante est précisément le comportement attendu d'une preuve compilée, pour une fois que le compilateur est employé à autre chose qu'à produire des messages cryptiques.

### 1.2 Descente fusionnée

La structure `Entry{z,open}` est correcte : un bit fermé après crédit d'un sous-arbre ne l'est que pour ce sous-arbre ; les frères restent ouverts, sauf saturation globale du compteur. Chaque position est ainsi attribuée au plus une fois par lane. `Hmax` élague communément, `Hmin` crédite q2, les deux boules créditent q3/q4, puis une seule boucle de coins décide q3 et q4 aux feuilles.

Petit gain gratuit à appliquer : en mode `with_corners=false`, si `balls[1].radius4==0` ou `balls[2].radius4==0`, le bit correspondant peut être retiré **avant** d'empiler la racine. Actuellement une lane de rayon nul traverse encore tout l'arbre, sans pouvoir créditer quoi que ce soit. Ce correctif peut compter sur les gros blocs internes et ne change aucun verdict.

### 1.3 Ce qui peut être hérité entre un parent `(A,B)` et ses enfants

Il serait dangereux de transporter indistinctement la frontière entière. Sous raffinement `A'⊆A`, `B'⊆B`,

```text
W_q(A,B) = ⋂_{a∈A,b∈B} W_q(a,b) ⊆ W_q(A',B').
```

La région universelle **grandit**. Donc :

- un sous-arbre témoin `ALL` pour le parent reste `ALL` pour tout enfant ;
- un sous-arbre `NONE` pour le parent peut devenir `MIXED` ou `ALL` pour un enfant ;
- un point retranché de `h_coeur` parce qu'il appartenait à `A` ou `B` peut devenir un témoin de cœur après scission.

Première cache sûre : transporter seulement une antichaîne de plages `ALL` effectivement créditées et disjointes de `A∪B`; le parcours enfant repart de la racine mais saute ces plages. Une frontière complète devra conserver `ALL + unresolved`, y compris les anciens `NONE`, avec les masques relationnels `overlap_A/overlap_B`. Cette version évite un joli faux gain suivi, quelques jours plus tard, d'une moins jolie rétractation.

---

## 2. `h_a/h_b` : résultat reçu et suite utile

Pour `a,z` fixés, l'ensemble des partenaires `b` tels que `z∈W_q(a,b)` est un cône convexe ouvert : demi-espace pour q2, cône circulaire pour q3/q4. Les huit coins distincts de `Box(B)` donnent donc une autorité **exacte sur l'enveloppe continue**. La symétrie échange `a,b`. Avec la disjonction des identités entre cœur, A et B, la somme

```text
h_coeur + h_a(a) + h_b(b)
```

est un minorant exact et fail-open. L'histogramme code correctement

```text
h_b(b) < h_q - h_coeur - h_a(a).
```

Le reçu `eight_clusters,n=8000` confirme le diagnostic : les paires inter-amas ont souvent le milieu vide, mais leurs propres amas fournissent des témoins d'extrémité. En q4, passer de 100 % des survivantes du cœur à 71,5 % après `h_a/h_b` est le bon ordre de grandeur.

Je ne recommande pas de développer immédiatement un dual-tree compliqué. D'abord :

1. brancher le filtre déjà écrit dans `q3_events_probe` ; le commentaire affirme actuellement `h_coeur+h_a+h_b`, mais le code q3 n'applique que le cœur ; sur `eight_clusters`, la mesure q3 précédente suggère environ 25 % d'ancres en moins, probablement davantage en coût puisque ce sont les longues ancres qui tombent ;
2. à `n=1000/1500/2000`, publier par lane `survivantes_ha / vraies_vivantes` ; si le mou est déjà proche de 1, il vaut mieux attaquer le census que raffiner le filtre ;
3. renforcer le juge : le couple `(argmax h_a,argmax h_b)` vérifie le cas le plus fortement certifié mais ne sonde pas la distribution des ancres tuées. On peut faire un unranking pondéré exact : pour chaque `a`, l'histogramme donne la masse des `b` tués ; tirer `a` proportionnellement à cette masse, puis `b` dans les classes admissibles. Quelques milliers d'ancres sont alors uniformes en masse tuée.

Si le dual-tree devient nécessaire, son ABI est clair : auto-jointure dirigée `(U_ancres,Z_témoins)`, diagonale développée en quatre couples, `range-add(weight(Z))` dès que le prédicat 8/64/512 coins est `ALL`, cutoff ponctuel, masques de lanes, suppression de la seule diagonale feuille-singleton.

---

## 3. Correction de bord des constantes de Poisson

### 3.1 Le rapport doit converger vers 1

Dans `uniform_cloud`, la densité reste constante et le côté du cube croît comme `L∝n^{1/3}`. L'épaisseur physique de la couche de bord pertinente reste d'ordre `λ^{-1/3}`. Sa fraction est donc `O(n^{-1/3})`, elle n'est pas invariante. Les constantes observées doivent converger vers les constantes sans bord, lentement, avec une correction principale en `n^{-1/3}`.

Pour une ancre de longueur `D`, écrivons

```text
|W_q(a,b)| = v_q D³.
```

Les coefficients exacts en dimension 3 sont

```text
v_2 = π/6,
v_3 = π/4 - π²/(9√3),
v_4 = π(14 - 9√2 asin(√(2/3)))/48.
```

Ils valent environ `0,523599`, `0,152263`, `0,120480`. La constante sans bord est

```text
C_inf(q,h) = 2πh/(3v_q),
```

soit `40,000`, `123,796`, `139,070` pour `h=10,9,8`.

### 3.2 Développement du cube dû à la disponibilité des paires

Posons

```text
F_h(t) = exp(-t) Σ_{j=0}^{h-1} t^j/j!,
S_α(h) = Σ_{j=0}^{h-1} Γ(j+α)/j!.
```

En tenant compte exactement du fait que les deux extrémités doivent rester dans le cube, mais en remplaçant encore la région témoin tronquée par son volume plein, on obtient

```text
C_pair(n) = C_inf [1 - A n^{-1/3} + B n^{-2/3} - C n^{-1}] + O(exp(-c n)),
A = 3 S_{4/3}(h)/(2 h v_q^{1/3}),
B = 2 S_{5/3}(h)/(π h v_q^{2/3}),
C = (h+1)/(8πv_q).
```

À `n=2000` :

| lane | sans bord | correction paires | mesure Claude |
|---|---:|---:|---:|
| q2 | 40,00 | **30,95** | **32,3** |
| q3 | 123,80 | **84,33** | **86,3** |
| q4 | 139,07 | **93,07** | **94,9** |

L'écart restant, positif et faible, a le bon signe : près du bord, `W_q` est lui-même tronqué, donc son nombre moyen de témoins diminue et davantage de paires survivent. S'ajoutent les effets réseau et binomiaux. Les trois mesures sont donc une confirmation quantitative remarquable du modèle, pas un échec des constantes.

Cette formule peut servir de repère, mais la meilleure porte est torique.

### 3.3 Porte torique à cible finie exacte

Sur le tore de côté `L`, avec `n` points uniformes et un cutoff `R<L/2`, posons

```text
u_q = v_q (R/L)³.
```

Le nombre attendu de paires vivantes non ordonnées **par point** est exactement, dans le modèle continu à `n` fixé,

```text
C_tor(q,h,n,R/L)
  = 2π/(3v_q) Σ_{j=0}^{h-1} I_{u_q}(j+1,n-1-j),
```

où `I_u(a,b)` est la bêta incomplète régularisée. La formule vient du fait que, conditionnellement au déplacement de la paire, les `n-2` autres points donnent une loi binomiale.

Pour `n=2000`, `R/L=0,4` :

```text
q2 : 40,000
q3 : 123,737
q4 : 138,680.
```

Protocole :

- domaine périodique `[0,L)^3`, avec `L` grand sur la grille u16 ;
- déplacement minimal-image `d=b-a`; ignorer les paires `|d|>R` ;
- pour un témoin, prendre son lift minimal relatif à `a`, puis employer les mêmes prédicats `(H,Xi)` ; comme `R<L/2` et `W_q⊂W_2`, le lift est sans ambiguïté ;
- plusieurs graines, moyenne et intervalle de confiance ; `L` suffisamment grand pour rendre l'erreur de réseau négligeable.

Cette porte détectera une perte ou un double compte avec une cible analytique bien plus sensible qu'un simple ledger de masse.

---

## 4. Raccord propre de la borne WSPD

Je suis d'accord avec la route « cellules pour la preuve, boîtes serrées pour les certificats », avec une amélioration : **garder aussi la séparation par boîte serrée comme arrêt supplémentaire**.

### 4.1 Cellule exacte du préfixe binaire Morton

`cell_of_prefix` arrondit aujourd'hui le préfixe au niveau octree inférieur. Il faut former la cellule exacte du préfixe binaire. Avec l'entrelacement actuel (`x` bit 0, `y` bit 1, `z` bit 2 dans chaque triplet) et `used=pref-16` :

```text
full = used/3,
rem  = used%3,
fixed_z = full + (rem>=1),
fixed_y = full + (rem>=2),
fixed_x = full.
```

Chaque axe reçoit alors son propre `shift=16-fixed_axis`. Le rapport d'aspect est au plus 2. L'arbre Karras est le trie binaire comprimé de ces cellules dyadiques ; les chaînes unaires sont contractées.

### 4.2 Récursion de référence et récursion réelle

Définir la récursion de référence :

- séparation testée sur les cellules exactes ;
- si non séparé, scinder le facteur de plus grand diamètre **de cellule**.

C'est la récursion à laquelle raccorder le packing du compressed quadtree, donc `O(s³m)` en dimension 3, `m` étant le nombre de sites distincts.

La récursion réelle peut terminer si

```text
cell_separated(A,B) OR tight_box_separated(A,B).
```

Tant qu'elle ne termine pas, elle choisit le même facteur que la référence. Par induction, chaque état réel est un état de la récursion de référence ; un arrêt par boîte serrée remplace simplement tout un sous-arbre de terminaux de référence par un rectangle. Le nombre de rectangles réel est donc **au plus** celui de la référence. On conserve ainsi le gain mesuré des boîtes serrées, sans leur demander de porter seules la preuve de packing.

C'est la route que je recommande, plutôt qu'un retour à une séparation uniquement cellulaire qui jetterait une constante utile avec l'eau mathématique du bain.

---

## 5. Réception de l'instruction q3

### 5.1 Géométrie et arithmétique

La forme de Gram de `q3_instruction.hpp` est correcte. Avec

```text
d=b-a, u=x-a,
D=d·d, E=u·u, F=d·u, G=DE-F²,
W=E(D-F)d + D(E-F)u,
P(z)=G|z-a|²-W·(z-a),
```

le centre vaut `a+W/(2G)` et `P<0/=0/>0` décide intérieur/coquille/extérieur. Les bornes AABB sont employées dans le bon sens : minimum de réseau exact pour élaguer seulement si `mn>0`, maximum aux extrémités pour créditer seulement si `mx<0`. Le cap ne masque aucune coquille pour une boule survivante : si le compteur atteint `h_3`, l'événement est rejeté et le shell n'est plus nécessaire ; sinon le parcours se termine et voit tous les zéros.

Le juge par identités à `n=400` est un vrai jalon de complétude de source : il confronte la WSPD et l'owner à tous les triplets, ce que les anciens compteurs agrégés ne faisaient pas.

### 5.2 Quatre raccords courts avant de publier des événements

1. **Vrais `PointId`.** `ua/ub/ux` sont des rangs Morton, pas des identités stables. Après refus des doublons :

   ```text
   PointId id(u) = ix.bucket_ids[ix.bucket_start[u]].
   ```

   `Key3` et `anchor_owns_q3` doivent employer ces IDs. Ajouter une porte de permutation des enregistrements qui conserve les IDs et exige les mêmes `SupportKey`/owners.

2. **Refus transactionnel des coquilles.** `shell_refused++` suivi de `continue` est un diagnostic « sous-ensemble régulier », pas un statut transactionnel. En mode exact, le premier extra-shell doit conduire à `unsupported_degeneracy`, sans publication partielle. Garder éventuellement un mode explicitement nommé `regular_subset_diagnostic` pour les campagnes u16 où les cosphéricités sont fréquentes.

3. **Exact-once visible.** Le `sort/unique` ne doit pas réparer silencieusement les doublons. Publier `raw_events`, `duplicate_supports`; la porte exige `duplicate_supports=0`. L'owner et la partition de paires devraient l'assurer.

4. **Appliquer `h_a/h_b`.** Le commentaire de tête le promet déjà, mais la source q3 actuelle n'applique que le cœur. Factoriser le filtre existant et l'insérer avant l'expansion des ancres est le gain immédiat le moins risqué.

### 5.3 Passage du support à l'événement complet

Les objets exacts sont disponibles sans nouvelle géométrie :

```text
ExactCenter = (2G a + W, 2G),
ExactLevel  = D E X / (4G),  X=|b-x|²=D+E-2F,
BallForm    = (A=G,
               B=-2G a-W,
               C=G|a|²+W·a),
```

puis réduction par pgcd et signe positif de `A` pour `BallKey`.

Le census doit rendre les IDs intérieurs, pas seulement leur nombre. Comme un événement survivant a moins de `h_3≤9` intérieurs, on peut conserver pendant la descente une petite liste de handles de sous-arbres `ALL`. Si le total atteint `h_3`, rejet immédiat sans expansion. S'il reste inférieur, l'expansion finale de ces handles produit au plus huit IDs. On obtient ainsi gratuitement `interior_ids`, le rang fermé et les facettes de l'hyperévénement.

---

## 6. Q9 — l'analogue axial q3 existe : niveaux peu profonds de droites

### 6.1 Réduction exacte dans le plan médiateur

Fixons l'ancre `(a,b)`. Posons

```text
d=b-a,  Δ=|d|²,
m=(a+b)/2,
T=2c-a-b,  donc T·d=0,
u_z=2z-a-b.
```

Toute sphère passant par `a,b` a un centre paramétré par `T` dans le plan médiateur. Quatre fois la puissance du site `z` est

```text
ℓ_z(T) = |u_z|² - Δ - 2 u_z·T.
```

C'est une forme **affine en deux variables** :

```text
ℓ_z(T)<0 : intérieur,
ℓ_z(T)=0 : shell,
ℓ_z(T)>0 : extérieur.
```

Les centres q3 admissibles d'une arête maximale vivent dans le disque

```text
|T|² ≤ Δ/3.
```

Pour un porteur `x`, posons

```text
p_x = Δ u_x - (u_x·d)d.
```

Son circumcentre minimal correspond au point exact

```text
T_x = ((|u_x|²-Δ) Δ / (2|p_x|²)) p_x.
```

Le census de tous les triangles d'une ancre devient donc : **compter, pour tous les points de requête `T_x`, combien de demi-plans `ℓ_z<0` les contiennent, avec détection des lignes `ℓ_z=0`.** On ne veut que les niveaux `0,…,h_3-1`.

C'est exactement l'analogue q3 du tri de racines q4 :

- q4 : arrangement de points sur une droite, on conserve les premières/dernières racines ;
- q3 : arrangement de droites dans un plan, on conserve seulement les niveaux peu profonds.

Une vue radiale est encore plus proche de q4. Sur chaque rayon `T=r e_θ`, une ligne donne une racine

```text
r_z(θ) = (|u_z|²-Δ)/(2 u_z·e_θ).
```

Un balayage angulaire maintient l'ordre des premières racines ; cet ordre ne change qu'aux intersections de lignes. Construire seulement les `h_3` premiers niveaux est la version q3 de `Q4SeedAxisTopR4`.

### 6.2 Un seul cover spatial par ancre, avec une constante meilleure

La constante `0,966D` citée dans la question est la borne q4. Pour q3 :

```text
|c-m| ≤ D/(2√3),
R ≤ D/√3,
```

donc tout porteur et tout point intérieur à une circum-boule q3 pertinente satisfait

```text
|z-m| < √3 D/2 ≈ 0,866D.
```

Une seule requête de la boule `B(m,√3D/2)` fournit donc tous les sites utiles de l'ancre. Les porteurs sont ensuite le sous-ensemble dans la lentille, aigu et possédé. C'est déjà une réduction de volume d'environ 28 % par rapport à `0,966D`, avant toute structure sophistiquée.

### 6.3 Deux implémentations, dans l'ordre conseillé

#### Version A — arbre des points de requête, simple et GPU-friendly

Pour chaque ancre :

1. construire les porteurs et leurs `T_x` dans le disque médiateur ;
2. les trier dans un petit quadtree/LBVH 2D temporaire ;
3. parcourir les sites `z` du cover commun. Pour un nœud de centres `Q`, `ℓ_z` est affine : ses extrema sont aux quatre coins ;
4. `max_Q ℓ_z<0` : `range-add(+1)` à tous les porteurs du nœud ; `min_Q ℓ_z>0` : aucun effet ; sinon scission ;
5. une requête saturée à `h_3` sort du masque ; à la feuille, test exact et collecte des shells.

Même sans dual-tree sur les sites, chaque site traverse un arbre de **tous les porteurs** au lieu que chaque porteur reparte dans l'arbre de tous les sites. Les zones homogènes sont créditées en bloc. C'est le prototype que je coderais d'abord.

Les `T_x` sont rationnels. Le tri spatial peut utiliser un filtre flottant, mais les boîtes de décision doivent être dirigées avec fallback exact. Une représentation homogène i192/`BigInt<3>` suffit très vraisemblablement sous u16 ; la largeur doit être prouvée avant le kernel.

#### Version B — arrangement peu profond exact

Pour les ancres à beaucoup de porteurs, construire par tuile l'arrangement local des lignes `ℓ_z=0` restreint au disque et aux profondeurs `<h_3`. Chaque cellule porte la petite liste de sites intérieurs ; chaque `T_x` est localisé dans une cellule ou sur une arête. La structure est temporaire et évincée après l'ancre : ce n'est pas l'arrangement global interdit par l'architecture.

Un backend hybride peut choisir A ou B selon `(nombre de sites, nombre de porteurs, mélange mesuré)`, les deux étant jugés par les mêmes `SupportKey/BallKey`.

### 6.4 Lien avec la dominance directionnelle

Pour `A_z=|u_z|²-Δ>0`, en projetant `u_z` dans le plan et en posant un point dual `q_z`, l'équation devient `q_z·T=1`. Pour le porteur `x`, `T_x=q_x/|q_x|²`; le site z est intérieur exactement lorsque

```text
q_z·q_x > |q_x|²,
```

c'est-à-dire lorsque `q_z` est au-delà de la tangente radiale en `q_x`. La « dominance directionnelle 432 » est donc interprétable comme un certificat discret suffisant de ce problème exact de demi-plans. Elle peut être un préfiltre utile, mais l'arrangement/range-add fournit la référence exacte.

---

## 7. Q10 — contrat output-sensitive

La sortie q3 peut être réellement quadratique ; aucun ordonnancement ne peut promettre un temps sous-quadratique universel tout en écrivant une sortie quadratique. Il faut donc assumer un contrat output-sensitive, sans transformer un budget en faux théorème.

Je recommande deux modes publics distincts :

### `complete_exact`

- aucun budget candidat configuré ;
- `count → preflight → fill → validate → publish` ;
- soit résultat complet, soit vrai `resource_exhausted` avant publication ;
- l'ordre « petites lentilles d'abord » est un choix de scheduler seulement.

### `resumable_exact`

- quantum de travail déclaré ;
- sortie `incomplete_continuation`, jamais `complete` ni `resource_exhausted` artificiel ;
- continuation scellée : `cloud_epoch/tree_digest`, schéma, rectangles/ancres restants, tâches médiatrices en cours, compteurs et digests d'événements déjà produits ;
- `run capé + reprises == run non capé` par identités.

Les longues ancres peuvent être traitées après les courtes, mais la meilleure solution n'est pas seulement de les remettre à demain : l'arrangement peu profond doit les **certifier mortes collectivement** lorsque leur grande lentille contient beaucoup de témoins.

---

## 8. Q11 — oracle indépendant maintenant

Oui, maintenant, avant q4. q4 réutilisera les seeds q3, les owners et une arithmétique plus large ; laisser une faute commune dans le sujet et le juge contaminerait deux lanes au lieu d'une. L'oracle peut rester petit et ne bloque pas les mesures de performance en parallèle.

Deux étages :

1. garder le juge source actuel à `n=400`, excellent pour la complétude WSPD/owner ;
2. ajouter un micro-oracle `cpp_int`/rationnel à `n≤40–60`, écrit indépendamment :
   - acuité par les trois produits scalaires d'angles, pas par `V²>D²` ;
   - owner avec vrais `PointId` ;
   - circumcentre par résolution rationnelle directe du système 2×2 ;
   - distances rationnelles centre-site comparées par produits croisés, sans `Q3Form` ni `q3_ball_depth` ;
   - shell complet, intérieurs, `ExactLevel=D E X/(4G)` canonique et `BallKey` ;
   - comparaison de l'événement complet, pas seulement de `SupportKey`.

Ajouter quelques mutants causaux : signe de `P`, `mn>=0` au lieu de `mn>0`, owner par rang Morton, oubli d'un shell, rayon non réduit. Ce petit oracle achètera beaucoup de tranquillité quand q4 commencera à empiler les déterminants comme l'humanité empile les couches d'abstraction.

---

## 9. Ordre de travail recommandé

1. **Raccords q3 courts** : vrais `PointId`, compteur de doublons exact-once, statut global sur extra-shell, `h_a/h_b` réellement branché.
2. **Événement complet q3** : paquet d'intérieurs ≤8, `BallKey`, `ExactCenter`, `ExactLevel` au carré, facettes `F_K^conn/render`.
3. **Prototype census partagé A** : cover `√3D/2`, points `T_x`, arbre 2D de requêtes et formes affines `ℓ_z`, avec comparaison au code actuel sur les mêmes ancres.
4. **Oracle rationnel indépendant** pendant ce développement.
5. **Porte torique q2/q3/q4**, cible bêta exacte ; elle devient le test statistique de perte/duplication.
6. **WSPD à cellules de préfixe exactes**, scission par diamètre de cellule, arrêt supplémentaire par boîte serrée.
7. Si le prototype A reste trop mélangé sur `eight_clusters`, passer à l'**arrangement peu profond B** ; seulement après, ouvrir q4 axial complet.

Le projet vient de franchir deux verrous réels : la WSPD n'est plus aveugle à la mort des ancres, et la source q3 n'est plus une proposition vague mais une chaîne confrontée par identités à tous les triplets. Le prochain verrou est maintenant bien localisé et possède une formulation mathématique exploitable : un problème de niveaux peu profonds de droites en dimension deux. C'est exactement le genre de mur qu'il est agréable de rencontrer, comparé aux anciens murs qui étaient surtout des compteurs mal nommés.