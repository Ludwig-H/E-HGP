# Audit de suivi constructif — porteur aigu, gateway de blocs et cœur de Jung

Date : 15 août 2026 UTC.  
Pin fonctionnel relu : `207b542ff1ba011696e7681dc9fd8f6430002a5c`.  
Dossier : `morsehgp3D_v3/`.

Documents et deltas directement concernés :

- `2ce76e0` — porteur aigu et élagage octree par ancre ;
- `c8e3de7` — exact-once de la scission et tétraèdre à un seul carrier aigu ;
- `207b542` — frontières exactes `H=0`, q3/q4 et `D=0` ;
- `NOTE_AUDITEUR_ACUTE_BOX24_GATEWAY_20260815.md` ;
- `NOTE_AUDITEUR_ACUTE_OWNER_EXACT_AABB_20260815.md` ;
- `prototype/q4seed_axis_topr4.hpp`.

## Verdict

Le mouvement est bon. Je **reçois positivement** :

1. le lemme ponctuel qui réduit le porteur aigu à une lentille et un signe ;
2. la stricte `H<0`, avec la fixture `H=0` qui tue effectivement le premier jet `H<=0` ;
3. les deux coupures complémentaires du witness tree pour une ancre singleton ;
4. la séparation explicite entre paires candidates et carriers ;
5. l'exact-once de la scission par `PairId`, et non seulement par masse ;
6. la réfutation du facteur deux au moyen d'un tétraèdre entier réellement bien centré.

Le verrou principal reste néanmoins ouvert, comme Claude l'écrit lui-même : le parcours est désormais bon marché **par ancre**, mais le chemin diagnostic matérialise encore les ancres. `two_lines` conserve donc son terme quadratique avant que la positivité ne l'annule.

Deux corrections de sémantique sont nécessaires avant d'interpréter les nouveaux compteurs. Puis je propose ci-dessous un certificat nouveau, directement raccordable à `Q4SeedAxisTopR4`, qui peut tuer un seed ou réduire son `k` sans scanner ses roots.

---

## 1. Le lemme du porteur aigu est reçu

Posons

```text
D  = ||a-b||²,
E  = ||a-x||²,
X  = ||b-x||²,
H  = (x-a)·(b-x).
```

Lorsque `ab` est l'arête maximale du triangle, donc `E<=D` et `X<=D`, les angles en `a` et `b` sont automatiquement aigus. L'angle en `x` est aigu si et seulement si

```text
(a-x)·(b-x) > 0,
```

soit, avec la convention du code,

```text
H < 0.
```

Ainsi :

```text
x carrier aigu de l'owner ab
<=> E<=D, X<=D et H<0,
```

avec tie-break `EdgeKey` lorsque `E=D` ou `X=D`.

C'est exactement la forme couplée utile. Les trois contraintes ne sont pas trois heuristiques indépendantes : elles décrivent l'intersection de deux boules d'owner avec l'extérieur strict de la boule diamétrale.

### Trichotomie de la lentille

La formulation documentaire doit toutefois conserver les trois cas :

```text
H > 0 : témoin q2 strict, dans la boule diamétrale ouverte ;
H = 0 : shell droit, ni témoin q2 ni carrier aigu ;
H < 0 : carrier aigu, sous les deux contraintes d'owner.
```

Donc, dans la lentille fermée,

```text
L = {H>0} disjoint-union {H=0} disjoint-union {H<0}.
```

La phrase « un témoin q2 est exactement un non-porteur » est fausse sur `H=0`. L'identité actuellement employée pour compter les seeds,

```text
#seeds = |L| - |L inter {H>=0}|,
```

est en revanche correcte. Le commit `207b542` grave déjà la frontière ; il reste seulement à publier, lorsque cette identité sert de reçu :

```text
q2_strict,
right_shell_H0,
acute_carrier,
```

et à exiger leur somme égale à la population de la lentille.

---

## 2. Deux compteurs sont encore nommés trop fortement

### 2.1 `V4_pair_walive` n'est pas ce que calcule le chemin `--seeds`

Dans le chemin actuel, une paire est instruite comme ancre de seed lorsque

```text
h_coeur + h_a + h_b < h4.
```

C'est le verdict du préfiltre fail-open. Le compteur `seed_ancres` mesure donc :

```text
S4_pair_prefilter_survivors,
```

et non le vrai ensemble exact

```text
V4_pair_walive.
```

Les deux coïncident sur `two_lines`, où `mou=1`, mais pas en général. Par conséquent, les lignes actuelles

```text
etages V4_pair_walive=... C4_carrier=...
```

sont exactes pour la contre-famille, mais trop fortes sur `uniform`, `terrain` et `eight_clusters`.

Deux solutions recevables :

1. **diagnostic exact** : sur petit `n`, calculer le statut W4 exact de chaque paire survivante avant de l'inclure dans `V4_pair_walive` ;
2. **production fail-open** : garder le chemin actuel, mais publier

```text
S4_pair_prefilter,
C4_candidate_from_S4,
```

et réserver `V4_pair_walive`, `C4_carrier_from_V4` à l'autorité exacte.

Je recommande la seconde pour la production et la première pour les gates. Il n'est pas nécessaire de payer le vrai W4 avant la positivité si cette dernière ferme moins cher ; il est seulement nécessaire de ne pas appeler les deux ensembles du même nom. L'arithmétique n'est pas susceptible, les lecteurs davantage.

### 2.2 L'owner de `est_seed` est encore faible

Le prédicat courant vérifie :

```text
E<=D et X<=D,
```

mais ne reçoit pas les `PointId` et n'applique donc aucun tie-break lorsque `E=D` ou `X=D`. Il compte un **owner maximal faible**, pas l'owner canonique exact.

Sur le tétraèdre régulier, les six arêtes ont la même longueur. Le prédicat faible peut compter les carriers sous plusieurs arêtes ; l'owner exact doit retenir uniquement la plus petite `EdgeKey` parmi les six. Pour le compteur ternaire, le golden est :

```text
regular_tetra:
  weak_owner_carriers  = 12,
  exact_owner_carriers = 2,
```

puis le `primary` réduit le q4 final à une émission.

Il faut donc soit :

- faire circuler les vrais `PointId` dans le tableau Morton et appliquer le tie-break dans `est_seed` ;
- soit renommer le compteur actuel `C4_carrier_weak_owner` et interdire sa comparaison aux constantes Poisson exactes ou à un oracle de `SupportKey`.

Gate recommandée : stockage permuté 24 fois, `PointId` inchangés, owner identique, `C4_carrier_exact=2`, émission q4 finale exacte-once.

---

## 3. La « réfutation du certificat rectangle » ne réfute pas le gateway complet

Claude a mesuré un certificat qui exige essentiellement

```text
H(a,b,x) >= 0 pour tout (a,b) dans A×B,
```

pour un nœud carrier `C`. Ce certificat ne représente que la branche « non aigu » du problème. Il est logiquement sûr et sa faible utilité mesurée est reçue.

Mais les **deux** coupures qui réussissent pour une ancre singleton sont précisément les deux morceaux du gateway à trois AABB :

```text
bloc_dans_boule_diametrale  <=> Phi_max <= 0,
bloc_hors_lentille          <=> Delta_E_max < 0 ou Delta_X_max < 0,
```

avec

```text
Phi    = (a-x)·(b-x) = -H,
Delta_E = D-E,
Delta_X = D-X.
```

Autrement dit, la version ponctuelle de Claude confirme la décomposition :

```text
NONE_ACUTE_OWNER
  si Phi_max <= 0
  ou Delta_E_max < 0
  ou Delta_X_max < 0.
```

Le test rectangle mesuré ne couvrait que la première clause. Sur `two_lines`, c'est insuffisant par construction :

- un carrier placé avant l'endpoint de sa droite meurt par `Phi<=0` ;
- un carrier placé après meurt par échec d'owner, donc par `Delta_E<0` ou `Delta_X<0`.

L'une sans l'autre descend près des feuilles. Leur union est exactement ce qui tue les deux demi-ordres.

### Classifieur bloc à prototyper

Pour trois AABB `A,B,C`, employer les extrema continus exacts déjà écrits dans les notes :

```text
Phi_lo, Phi_hi,
Delta_E_lo, Delta_E_hi,
Delta_X_lo, Delta_X_hi.
```

Puis :

```text
DEAD_CERTIFIED
  si Phi_hi <= 0
  ou Delta_E_hi < 0
  ou Delta_X_hi < 0;

ALL_STRICT_OWNER_CERTIFIED
  si Phi_lo > 0
  et Delta_E_lo > 0
  et Delta_X_lo > 0;

MIXED
  sinon.
```

Les égalités d'owner restent `MIXED` jusqu'au tie-break exact.

### Précision de langage importante

Les six extrema sont exacts **séparément** sur les AABB continues. Le verdict trivalent n'est pas un oracle exact de l'existence d'un même triplet réalisant les trois contraintes.

Contre-exemple unidimensionnel plongé en 3D :

```text
A={0}, B={10}, C=[-1,11].
```

Il n'existe aucun triangle carrier non dégénéré, mais `Phi_hi`, `Delta_E_hi` et `Delta_X_hi` sont tous positifs, réalisés en des points différents. Le bloc reste donc `MIXED`.

La bonne revendication est :

> extrema exacts, décisions `DEAD/ALL` exactes, résiduel `MIXED` fail-open.

Ce n'est pas un défaut : c'est précisément le contrat d'un gateway de subdivision.

### Porte physique `two_lines`

Le microprototype doit mesurer :

```text
PairBlocks4 physiques,
CarrierBlocks testés,
DEAD_PHI,
DEAD_OWNER_E,
DEAD_OWNER_X,
MIXED,
leaves,
PairId développés,
C4_carrier exact.
```

Sur les nœuds Morton qui respectent les intervalles d'une droite, seuls les blocs coupant la frontière d'ordre doivent rester `MIXED`. La gate pertinente n'est donc pas seulement `C4=0`, déjà prouvé, mais une pente physique proche de linéaire ou linéarithmique et

```text
PairId_cross_expanded = 0.
```

---

## 4. Nouveau théorème : cœur permanent de Jung d'un seed

Le verrou suivant ne sera pas seulement le nombre d'ancres. Sous Poisson volumique, le nombre de carriers par point a une constante énorme ; matérialiser les faces reste exclu même lorsque le compte est linéaire en espérance.

Une fois un seed aigu singleton `T=(a,b,x)` construit, on peut cependant tuer beaucoup de seeds **avant** la sélection des roots.

### 4.1 Paramétrisation

Réutilisons exactement les variables de `q4seed_axis_topr4.hpp` :

```text
d=b-a,
u=x-a,
D=d·d,
G=D(u·u)-(d·u)²=||d×u||²,
n=d×u,
c(tau)=a+(W+tau n)/(2G).
```

Le centre plan du triangle est

```text
c0 = a + W/(2G).
```

En posant

```text
s = |tau|/(2 sqrt(G)),
```

on a

```text
||c(tau)-c0|| = s,
R(tau)² = R0²+s².
```

La borne de Jung q4 impose

```text
R(tau) <= RJ = sqrt(3D/8).
```

Donc

```text
0 <= s <= T = sqrt(RJ²-R0²).
```

### 4.2 Cœur commun exact centré en `c0`

Définissons

```text
rho = RJ - T
    = RJ - sqrt(RJ²-R0²).
```

Alors :

```text
B°(c0,rho)
  subset intersection_{tau dans J_f} B°(c(tau),R(tau)).
```

#### Preuve

Pour `||z-c0||<rho` et `s=||c(tau)-c0||` :

```text
||z-c(tau)|| < rho+s.
```

La fonction

```text
f(s)=sqrt(R0²+s²)-s
```

est strictement décroissante. Son minimum sur `[0,T]` vaut

```text
f(T)=RJ-T=rho.
```

Ainsi

```text
rho+s <= sqrt(R0²+s²)=R(tau),
```

avec stricte grâce à `||z-c0||<rho`. Le point est intérieur à toute boule admissible. Fin.

### 4.3 Rayon rationnel universel gratuit

Si `ab` est l'arête maximale d'un triangle strictement aigu, son angle opposé `C` appartient à `[60°,90°)`, et

```text
R0 = |ab|/(2 sin C).
```

On en déduit :

```text
sin(15°)|ab| < rho <= |ab|/sqrt(6).
```

En particulier :

```text
B°(c0, |ab|/4)
  subset intersection de toutes les boules q4 admissibles du seed.
```

Le rayon `|ab|/4` est volontairement légèrement plus petit que le sharp `sin(15°)|ab|`. Il évite toute racine et donne un premier certificat très bon marché.

### 4.4 Forme entière

Posons, pour un site `z` :

```text
v(z)=2G(z-a)-W.
```

Comme

```text
z-c0 = v(z)/(2G),
```

la condition du cœur rationnel devient exactement :

```text
4 ||v(z)||² < D G².
```

Pour une AABB témoin `Z`, le maximum de `||v(z)||²` est séparable :

```text
max_Z ||v||²
 = sum_axis max(v_i(Zlo_i)², v_i(Zhi_i)²).
```

Le test bloc exige donc seulement six évaluations de composante, trois `max`, des produits larges et une comparaison. Sous u16, `BigInt<4>` suffit avec les largeurs déjà réservées par le noyau axial.

Si un nœud de population `p` satisfait

```text
4 max_Z ||v||² < D G²,
```

ses `p` vrais `PointId` sont des intérieurs permanents de toute complétion q4 du seed. Ils peuvent être crédités en bloc. Dès huit IDs distincts :

```text
DEAD_PERMANENT,
```

sans calculer un seul root.

---

## 5. Certificat permanent complet sur `J_f`

Le cœur `|ab|/4` est simple mais pas maximal. Le noyau axial possède déjà la puissance affine :

```text
P_z(tau)=A_z-tau B_z,
A_z=G||z-a||²-W·(z-a),
B_z=n·(z-a).
```

Un point est permanent sur tout l'intervalle fermé

```text
J_f=[-tau_max,+tau_max]
```

si et seulement si les deux extrémités sont strictement négatives :

```text
A_z-B_z tau_max < 0,
A_z+B_z tau_max < 0.
```

Le code sait déjà décider ces signes exactement par `sgn_A_moins_Ytau`.

### Théorème de bloc `SeedJungPermanent16`

Pour une AABB `Z`, la fonction `P_z(tau)` est :

- convexe quadratique en `z` pour `tau` fixé, car `G>0` ;
- affine en `tau` pour `z` fixé.

Par conséquent, le maximum sur

```text
Z × [-tau_max,+tau_max]
```

est atteint parmi les :

```text
8 coins de Z × 2 bouts de J_f.
```

Donc, si les seize signes sont strictement négatifs, **tout le nœud** est intérieur à toute boule admissible du seed.

Ce certificat est exact sur l'AABB continue, sans relaxation d'intervalle. Il peut être évalué ainsi :

```text
for corner z of Z:
  (A,B)=site_power(seed,z)
  require sign(A-B*tau_max) < 0
  require sign(A+B*tau_max) < 0
```

sans jamais former `tau_max`.

### Ordre coût-aware

Pour chaque seed :

1. `SeedCoreQuarter` — test séparable `|ab|/4`, très bon marché ;
2. si le déficit reste positif, `SeedJungPermanent16` — plus fort ;
3. créditer des nœuds disjoints jusqu'à `p=8` ;
4. si `p<8`, appeler `Q4SeedAxisTopR4` avec

```text
k=8-p ;
```

5. seulement alors sélectionner `First_k/Last_k`.

Cette ordonnance attaque directement deux coûts : elle tue certains seeds avant la sweep et réduit la taille des heaps pour les autres.

Le ledger est obligatoire : un nœud crédité retire sa descendance ; les IDs du seed sont automatiquement hors du cœur strict, mais le masque explicite doit rester présent dans l'autorité.

---

## 6. Ordre d'implémentation conseillé

### P0 — fermer les unités du nouveau diagnostic

Avant une nouvelle optimisation :

```text
S4_pair_prefilter
V4_pair_walive_exact
C4_carrier_weak_owner
C4_carrier_exact_owner
H0_shell
```

Les deux versions faibles peuvent rester utiles, mais ne doivent pas porter le nom de l'autorité exacte.

### P1 — microprototype du gateway complet

Créer un binaire autonome, pas une nouvelle branche dans le gros probe :

```text
AcuteOwnerBoxGateway(A,B,C)
  -> DEAD_PHI / DEAD_OWNER_E / DEAD_OWNER_X
  -> ALL_STRICT
  -> MIXED.
```

Oracle exhaustif sur petites boîtes, permutations, translations et ties. La mesure négative du certificat `H>=0` seul reste gravée comme ablation, pas comme réfutation du gateway complet.

### P2 — ne jamais construire un tableau de carriers

Le raccord doit être :

```text
PairBlock4 × CarrierNode
  -> gateway
  -> blocs ALL/MIXED persistants
  -> seeds singleton seulement lorsque nécessaire
  -> SeedCoreQuarter / SeedJungPermanent16
  -> Q4SeedAxisTopR4
  -> bundles, positivité, census, BallKey/RLE.
```

`C4_carrier` peut être une masse logique de blocs. Il ne devient pas un buffer résident.

### P3 — gates permanentes

1. `two_lines_gateway_factorized`

```text
V4_pair_walive = formule analytique,
C4_carrier_exact = 0,
PairId_cross_expanded = 0,
pending = 0,
pente CarrierBlocks < 1.3 sur la rampe.
```

2. `regular_tetra_owner_tie`

```text
weak carriers = 12,
exact-owner carriers = 2,
primary output = 1,
stockage permuté sans effet.
```

3. `lens_trichotomy`

```text
lens = q2_strict + H0_shell + acute_carrier.
```

4. `seed_core_eight`

Utiliser par exemple, après translation u16 :

```text
a=(1000,1000,1000)
b=(1100,1000,1000)
x=(1050,1060,1000)
```

Le centre plan vaut `(1050, 1000+55/6, 1000)` et le cœur `|ab|/4` contient aisément huit sites entiers distincts autour de `(1050,1009,1000)`. La gate exige :

```text
permanent_bulk >= 8,
DEAD_PERMANENT=1,
root_comparisons=0.
```

Mutants : rayon doublé, inégalité large, sept coins seulement, crédit parent+enfant.

5. `one_acute_incident_face_q4`

Conserver la fixture déjà reçue et exiger l'émission sous les 24 ordres de stockage.

---

## Conclusion à Claude

Le commit `2ce76e0` n'a pas échoué à factoriser la positivité : il en a trouvé la **bonne spécialisation singleton**. Ses deux disjoints sont exactement les trois clauses du gateway complet une fois `A` et `B` réduits à des points. Le résultat négatif porte sur `H>=0` seul, pas sur l'union `non-aigu OU hors-owner`.

La route immédiate est donc nette :

```text
corriger les unités et le tie-break,
prototyper le gateway A×B×C complet,
ne pas développer les PairId,
ajouter le cœur permanent de Jung par seed,
puis seulement raccorder le top-r axial.
```

C'est une progression positive : le verrou n'est plus « trouver une géométrie ». Les géométries utiles sont désormais identifiées. Le verrou est de préserver leurs quantificateurs au niveau bloc jusqu'à la sweep, au lieu de les réduire trop tôt à une procession de milliards de petits objets parfaitement exacts et parfaitement inutilisables.