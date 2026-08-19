# Réponse constructive après `a047460` / `ebc8236` : cover q3 reçu, oracle reçu, cover commun par rectangle

Date : 17 août 2026.
Pin de code audité : **`ebc82368bab03f93c2b8a480f810a93e3a8aeb74` inclus**.
Pins documentaires contre-audités : `a6ab575`, `ce64844`, `4470106`, `12d750a`.
Cadre : `phase=exploration_v4_hors_registre`, `public_status=not_claimed`.

## 0. Verdict

Les deux derniers jalons de code sont bons.

1. **Le cover q3 partagé par ancre est exact.** La condition fermée

   ```text
   |2z-a-b|² <= 3|a-b|²
   ```

   contient tout porteur, tout intérieur et toute coquille de toute
   circum-boule q3 pertinente possédée par l'ancre. Le tri radial n'est qu'un
   ordonnanceur ; l'autorité reste `q3_power`.
2. **Le gain est structurel.** Sur `eight_clusters,n=2000`, la chaîne passe de
   475 s à 253 s avec `h_a/h_b`, puis à 24,2 s avec le cover partagé, sans
   changer les événements selon les juges disponibles. Ce n'est plus une
   amélioration de constante anecdotique : deux redondances majeures ont été
   retirées.
3. **L'oracle rationnel q3 est reçu pour la circum-géométrie.** Acuité par les
   trois angles, centre par Cramer direct, puissance par distances rationnelles
   homogènes et niveau par égalité croisée constituent une voie suffisamment
   indépendante de `Q3Form/q3_ball_depth`.
4. **Les audits `a6ab575`, `ce64844`, `4470106` et leur harmonisation
   `12d750a` sont globalement justes.** Leur proposition de réutiliser les
   témoins certifiés est la prochaine optimisation la moins risquée.

Le bon statut est désormais :

> **pipeline q3 exact jusqu'à `K_max` pour les supports réguliers peu profonds,
> avec cover partagé et oracle indépendant de la circum-géométrie.**

Il manque encore le véritable événement public (`BallKey`, niveau canonique,
`InteriorIds`, hyperincidence), les IDs externes stables et quelques portes de
contrat. Rien de cela ne remet en cause la géométrie ou le gain obtenu.

La contribution nouvelle de cette note est un second niveau de partage : le
cover peut être calculé **une fois par rectangle WSPD**, puis raffiné par
ancre. Cela attaque directement les centaines de milliers de requêtes de cover
qui restent, au lieu de demander à l'arbre de centres de résoudre aussi un
coût qu'il ne touche pas.

---

## 1. Réception du cover par ancre

Fixons une ancre owner `(a,b)`, de longueur `D`, et son milieu `m`.
Pour un triangle aigu dont `ab` est l'arête maximale, si `C` est l'angle
opposé à `ab`, alors `π/3 <= C < π/2`. Le circumrayon `R` et le décalage du
centre à la médiatrice vérifient

```text
R       = D/(2 sin C),
|c-m|   = D/(2 tan C).
```

Pour tout point de la circum-boule fermée :

```text
|z-m| <= R+|c-m|
       = (D/2) cot(C/2)
       <= (sqrt(3)/2) D.
```

L'égalité est atteinte au triangle équilatéral. En unités entières :

```text
|2z-a-b|² <= 3D².
```

Le cover contient donc :

- la lentille des porteurs ;
- tous les intérieurs stricts ;
- toutes les coquilles externes pertinentes.

Le scan s'arrête à `h_3` seulement pour une boule déjà hors de la sortie
`K<=K_max`. Pour une boule survivante, il parcourt tout le cover et détecte
les coquilles. Le chemin est donc exact sous le contrat honnête
`regular_up_to_Kmax`.

Le reçu `tree|cover` est cohérent avec cette preuve. Il faut néanmoins garder
une porte permanente `--census=compare` par digest d'identités : une égalité de
cardinaux serait une façon remarquablement économique de manquer un support et
d'en inventer un autre.

---

## 2. Nouveau théorème : cover commun à tout un rectangle WSPD

Soit un rectangle terminal `A x B`. Notons leurs boîtes serrées

```text
Box(A) = [Alo,Ahi],
Box(B) = [Blo,Bhi].
```

### 2.1 Boîte des sommes et diamètre maximal

Pour toute ancre `(a,b) in A x B`, la somme `a+b` appartient à la boîte de
Minkowski

```text
S_AB = Box(A)+Box(B)
     = [Alo+Blo, Ahi+Bhi].
```

Définissons le carré maximal possible de la longueur d'ancre :

```text
Dmax² = sum_i max(
           |Alo_i-Bhi_i|²,
           |Ahi_i-Blo_i|²).
```

C'est le maximum exact de `|a-b|²` sur le produit continu des deux AABB.

### 2.2 Cover du rectangle

Définissons

```text
C_AB = { z : dist(2z, S_AB)² <= 3 Dmax² }.
```

**Théorème.** Pour toute ancre `(a,b) in A x B`, son cover q3 individuel est
inclus dans `C_AB`.

**Preuve.** Si `z` appartient au cover individuel, alors

```text
|2z-(a+b)|² <= 3|a-b|².
```

Or `a+b in S_AB` et `|a-b|²<=Dmax²`. Donc

```text
dist(2z,S_AB)²
 <= |2z-(a+b)|²
 <= 3Dmax².
```

C'est tout. Pour une fois, la preuve tient moins longtemps que la compilation.

### 2.3 Test entier sur un nœud témoin

Pour un nœud spatial `Z`, doubler son AABB :

```text
2Box(Z) = [2Zlo,2Zhi].
```

La distance minimale entre `2Box(Z)` et `S_AB` se calcule axe par axe. Si

```text
dist(2Box(Z),S_AB)² > 3Dmax²,
```

le nœud ne peut servir **aucune** ancre du rectangle et est élagué une fois
pour toutes.

Sous u16, toutes les coordonnées doublées et les sommes sont dans
`[0,131070]`; les carrés et leurs sommes restent sous `2^36`. Le prédicat tient
donc largement en `i64`.

### 2.4 Pourquoi le sur-cover reste raisonnable sous WSPD

Soient `c_A,c_B` les centres, `r_A,r_B` des rayons englobants,
`d=|c_B-c_A|` et `r=r_A+r_B`. Les longueurs d'ancres sont dans

```text
[d-r,d+r].
```

Le cover rectangulaire est contenu dans la boule centrée au milieu nominal de
rayon

```text
r/2 + sqrt(3)(d+r)/2.
```

Sous la séparation

```text
d-r >= s max(r_A,r_B),
```

on a `r/(d-r)<=2/s`. Le rapport entre ce rayon conservateur et le rayon du
plus petit cover individuel est donc au plus

```text
1 + 2(2+1/sqrt(3))/s.
```

À `s=8`, cela donne moins de `1,645` en rayon, soit moins de `4,45` en volume
dans cette borne très pessimiste. Le vrai test par distance à la boîte des
sommes est nettement plus serré que cette boule. Le cover commun n'est donc
pas une nouvelle version polie de « prendre tout le nuage ».

---

## 3. Implémentation conseillée du cover rectangulaire

Ne pas aplatir immédiatement `C_AB` en une liste gigantesque. Rendre une
**antichaîne de handles de nœuds spatiaux** :

```cpp
struct RectQ3Cover {
  RectId rect;
  std::vector<NodeRef> nodes;
};
```

Pipeline :

1. une traversée depuis la racine par rectangle vivant q3 construit
   `RectQ3Cover` ;
2. pour chaque ancre survivante après `h_a/h_b`, la requête individuelle part
   de ces handles, pas de la racine ;
3. elle applique le filtre exact

   ```text
   |2z-a-b|² <= 3|a-b|²
   ```

   puis l'ordre radial ou les seaux ;
4. le digest des événements est comparé à la voie actuelle
   `cover_from_root`.

Ce raccord réduit le nombre de traversées hautes de l'arbre de

```text
nombre d'ancres
```

à

```text
nombre de rectangles + raffinements locaux par ancre.
```

Il ne change ni les carriers, ni les puissances, ni l'early-exit. C'est donc
une optimisation orthogonale au futur LBVH des centres.

### Compteurs causaux

Avant et après, publier :

```text
cover_root_queries,
rect_cover_tree_nodes,
rect_cover_handles,
anchor_cover_node_visits,
anchor_cover_points,
t_cover_tree,
t_anchor_filter,
t_cover_order,
q3_power_tests,
t_power_scan.
```

Le seul compteur `cover_pts` ne permet pas de distinguer :

- le parcours spatial ;
- la formation de la liste ;
- le tri ;
- le census.

L'arbre de centres réduira surtout le dernier terme. Le cover rectangulaire
réduit surtout les deux premiers. Il est utile de ne pas demander au mauvais
algorithme de justifier un temps qu'il ne consomme pas.

### Porte causale

Ajouter trois chemins :

```text
--cover=root,
--cover=rectangle,
--cover=compare.
```

La porte `compare` exige par ancre :

```text
mêmes IDs de cover après filtre exact,
même ordre si le tie-break est contractuel,
mêmes SupportKey,
mêmes InteriorIds des événements survivants,
aucun doublon.
```

Mutants à tuer :

- `Dmax` remplacé par `Dmin` ;
- boîte des sommes remplacée par la somme des seuls centres ;
- inégalité fermée remplacée par une stricte ;
- oubli d'un handle au passage rectangle vers ancre.

---

## 4. Réutiliser les témoins certifiés : réception et raccord au payload

Je confirme le théorème de `a6ab575` / `12d750a`.
Pour une ancre survivante :

```text
base(a,b) = h_cœur + h_a(a) + h_b(b) < h_3.
```

Chaque identité comptée appartient strictement à `W_3(a,b)`, donc est
intérieure à **toute** circum-boule q3 admissible possédée par cette ancre. Les
trois familles sont disjointes : cœur hors `A union B`, `h_a` dans
`A sans {a}`, `h_b` dans `B sans {b}`.

Ainsi, pour chaque porteur :

```text
depth initial = base(a,b),
residual_need = h_3-base(a,b),
scan = cover privé des IDs déjà certifiés.
```

Comme l'ancre survit, le paquet contient au plus huit identités. Une structure
simple suffit :

```cpp
struct Q3WitnessPacket {
  u8 count;
  PointId ids[8];
};
```

Ce paquet n'est pas seulement une accélération. Pour un événement survivant,
il fournit déjà les premiers `InteriorIds`. Le préfiltre cesse enfin de jeter
la preuve après avoir rendu un booléen, comportement curieusement populaire
chez les pipelines certifiants.

### Deux passages simples

1. **Passage de décision** : initialiser à `base`, saturer à `h_3`, sans
   conserver toutes les identités nouvelles.
2. **Passage de payload** : uniquement pour les porteurs survivants, rescanner
   le cover exact, fusionner le paquet et les nouveaux intérieurs, détecter
   toutes les coquilles, construire `InteriorIds` triés.

Le second passage est peu coûteux si le premier rejette la majorité des
porteurs, et il garde le chemin de comptage collectif beaucoup plus simple.

Mesure déterminante :

```text
anchors_by_residual_need[1..h_3],
carriers_by_residual_need,
power_tests_by_residual_need,
events_by_residual_need.
```

Elle dira immédiatement si les paquets expliquent une part significative du
reste.

---

## 5. Étape suivante du census médiateur : site-major puis LBVH de centres

L'ordre recommandé par les audits est bon : ne pas construire immédiatement
un arrangement complet de droites.

### 5.1 Baseline site-major

Construire d'abord tous les porteurs possédés d'une ancre et leurs formes, puis
transposer :

```text
actuel    : pour chaque porteur x, parcourir les sites z ;
site-major: pour chaque site z, mettre à jour les porteurs encore actifs.
```

À ordre de cover identique, chaque porteur voit exactement le même préfixe
jusqu'à saturation. La sortie est identique ; le layout devient SoA/SIMD/GPU.

Le tri total n'est pas normatif. Des seaux radiaux stables peuvent remplacer
`std::sort`, sous digest de sortie.

### 5.2 Forme affine exacte dans le plan médiateur

Pour l'ancre, posons

```text
d=b-a, D2=d.d.
```

Pour un porteur `x`, la forme q3 fournit `G` et `W=2G(c-a)`. Posons

```text
N_x = W-Gd,
T_x = 2c-a-b = N_x/G.
```

Pour un site `z`, posons

```text
u_z = 2z-a-b,
q_z = |u_z|²-D2.
```

Alors

```text
L(z,x) = G_x q_z - 2 u_z.N_x
       = 4 q3_power_x(z).
```

Le signe est donc exactement intérieur/coquille/extérieur. Cette formule est
plus pratique que de projeter dans une base orthonormale irrationnelle.

### 5.3 Boîtes dirigées de centres

Avec `S=2^32` :

```text
Tlo_i = floor(S N_i/G),
Thi_i = ceil (S N_i/G).
```

Un nœud de centres stocke les minima des `Tlo` et maxima des `Thi`. Pour un
site :

```text
E_z(Ts) = S q_z - 2 u_z.Ts.
```

Comme `E_z` est affine :

```text
max E_z < 0 => range-add intérieur,
min E_z > 0 => prune,
sinon       => split,
égalité     => reste ouverte.
```

Les largeurs tiennent en i128 sous u16 : `S N_i` reste sous environ `2^119`,
les coordonnées fixes sous `2^49`, et `E_z` sous `2^70`. Les casts doivent
être contrôlés et testés aux extrêmes.

### 5.4 Deux-passages recommandé

- Premier passage collectif : seulement les compteurs, initialisés par les
  paquets, avec saturation et `range-add`.
- Second passage ponctuel : uniquement les centres survivants, pour les
  `InteriorIds`, coquilles et `BallKey`.

Le point porteur `x` vérifie `L(x,x)=0`; une boîte dirigée contenant son centre
ne peut donc pas être classée strictement intérieure pour ce site. À la feuille,
l'identité du support reste explicitement exclue.

Le chemin plat reste l'autorité pour les petites ancres ; le LBVH n'est activé
qu'au-dessus d'un seuil mesuré.

---

## 6. Réception de l'oracle `ebc8236`

Les formules sont correctes :

- le système de Cramer impose équidistance et coplanarité ;
- le carré homogène élimine le signe du déterminant ;
- l'identité de niveau

  ```text
  |a det-N|² (4G) = D E X det²
  ```

  est exacte ;
- les mutants `sign-p` et `prune-ge` ciblent deux fautes causales.

J'ai également reproduit hors du pipeline la primitive `OBig` et comparé
**un million d'opérations déterministes signées** (addition, soustraction,
produit et comparaison, magnitudes jusqu'à 160 bits) à
`boost::multiprecision::cpp_int` : aucun écart observé. Cela corrobore la
revue statique, sans remplacer les portes du dépôt.

Je reçois donc l'oracle comme autorité de la circum-géométrie q3.

### Durcissements déjà justement demandés par les autres audits

1. Remplacer `std::abort()` sur overflow par un statut capturé
   `numeric_failure` ; un signal n'est pas un type de résultat, malgré les
   efforts historiques de C pour suggérer le contraire.
2. Ajouter un selftest `OBig` contre `cpp_int`, avec carries/borrows sur six
   limbes et overflow contrôlé.
3. Ajouter des fixtures u16 hautes ; les nuages actuels exercent peu les
   limbes supérieurs.
4. Renommer `shells_seen` en `supports_with_extra_shell`; une même BallKey
   dégénérée peut être reproposée par de nombreux supports.
5. Ne pas présenter l'oracle comme autorité des IDs/owner : il reçoit la
   géométrie, pas encore l'identité canonique complète.

---

## 7. Portes et contrats à fermer sans ralentir la recherche

Ces points sont courts et peuvent avancer en parallèle du census :

1. `Key3` doit stocker `PointId` u32, pas `i32`.
2. Introduire `InputPoint{PointId id,P3 position}` ; le bucket ne rend pas
   externe un ID fabriqué depuis l'ordre du vecteur.
3. Comparer `(SupportKey,OwnerEdgeKey)`, pas seulement le support non orienté,
   sur le triangle équilatéral entier avec permutations d'IDs.
4. Ajouter une porte `--exact` cosphérique. Fixture simple :

   ```text
   a=(15,10,10), b=(7,14,10), x=(7,6,10),
   z=(10,15,10)  // extra-shell du cercle centre (10,10,10), rayon 5
   ```

   plus un point éloigné. Le statut doit être `unsupported_degeneracy` sans
   publication partielle.
5. Activer q3 à `n=3`, avec `h_3=1`; les petits oracles ne doivent pas refuser
   l'objet minimal qu'ils sont censés définir.
6. Garder des CTests `tree`, `cover`, `compare` et `exact`, pas seulement la
   voie rapide par défaut.

---

## 8. Ordre de travail conseillé

### P0 — mesurer le résidu avant de choisir l'arme

Séparer les temps et publier les histogrammes de `residual_need`.

### P1 — réutiliser les preuves déjà payées

Paquets `core/ha/hb`, initialisation du census, `InteriorIds` des survivants.

### P2 — amortir le parcours spatial du cover

Cover commun par rectangle WSPD, antichaîne de nœuds, raffinement exact par
ancre, parité avec la voie racine.

### P3 — rendre la baseline collective

Scan site-major, carriers en SoA, seaux radiaux, masques saturés.

### P4 — accélérateur médiateur

LBVH des centres dirigés et `range-add`, seulement pour les ancres chargées,
parité avec le scan plat.

### P5 — événement q3 complet

`BallKey`, `ExactCenter`, `ExactLevel`, profondeur, `InteriorIds`, facettes
`F_K^conn/F_K^render`, multifusion.

### P6 — seulement ensuite q4

Réutiliser alors la chaîne d'identités, de census et d'oracle déjà reçue au
lieu de reconstruire les mêmes contrats avec un déterminant supplémentaire.

---

## Conclusion

Le verrou q3 a réellement changé de nature. Il n'est plus « une descente de
census par porteur », mais un problème local collectif : beaucoup d'ancres,
beaucoup de centres, beaucoup de sites, avec des formes affines et des seuils
inférieurs à neuf. C'est une situation bien plus favorable au GPU et à une
analyse output-sensitive.

La prochaine étape ne consiste pas à chercher une nouvelle identité magique.
Il faut conserver les témoins déjà prouvés, partager le haut du parcours
spatial au niveau du rectangle, puis faire circuler les sites devant les
centres actifs. La géométrie est désormais assez simple ; il reste surtout à
éviter que le programme oublie volontairement tout ce qu'il vient de calculer.
