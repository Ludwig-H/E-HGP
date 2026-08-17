# Audit constructif des raccords q3, du cover partagé et de l'oracle indépendant

Date : 17 août 2026.  
Commits de code audités : `2d26e7a`, `79dc862`, `a047460`, `ebc8236`.  
Pin audité : `ebc82368bab03f93c2b8a480f810a93e3a8aeb74` inclus.  
Cadre : `exploration_v4_hors_registre`, `public_status=not_claimed`.

Cette note prolonge les audits et l'harmonisation déjà présents dans ce
dossier. Je confirme leur cap général, puis je précise les contrats que les
nouveaux commits rendent désormais testables. Aucun statut CI n'est publié au
pin : les mentions `23/23 CTest` restent des reçus de Claude, non une
exécution indépendante de cet audit.

---

## 0. Verdict exécutif

Le progrès depuis `e45a683` est substantiel.

### Reçu mathématiquement

1. **Le raccord `h_cœur+h_a+h_b` dans l'instruction q3 est fail-open.** Le
   seuil `need=h_3-h_cœur` et la mort `h_a+h_b>=need` ont le bon sens et les
   trois familles de témoins sont disjointes.
2. **Le gain mesuré est structurel.** Sur `eight_clusters,n=2000`, retirer
   seulement 17 % des ancres mais 47 % du temps montre que le filtre élimine
   prioritairement les longues ancres inter-amas, exactement celles qui
   portent les grandes lentilles coûteuses.
3. **Le cover commun `B(m,sqrt(3)D/2)` est exact pour q3.** Il contient tous
   les porteurs admissibles, tous les points intérieurs et tous les points de
   coquille des circum-boules q3 possédées par l'ancre.
4. **Le scan plat du cover est exact jusqu'à `K_max`.** Le tri par distance au
   milieu n'est qu'un ordonnancement ; chaque décision reste prise par
   `q3_power`. L'early-exit à `h_3` ne retire qu'une boule déjà trop profonde
   pour contribuer à la sortie demandée.
5. **Le gain du cover est reçu comme avancée algorithmique.** Le passage de
   253 s à 24,2 s sur `eight_clusters,n=2000`, avec mêmes comptes et juges
   0/0, déplace réellement le verrou. Le code dispose maintenant d'une
   baseline q3 simple, exacte dans le régime régulier et environ vingt fois
   plus rapide que le point de départ.
6. **L'oracle q3 de `ebc8236` est réellement différent sur les choix
   géométriques décisifs.** Acuité par trois angles, circumcentre par Cramer
   3x3, distances rationnelles et coquilles complètes forment une autorité
   indépendante de `Q3Form/q3_ball_depth` suffisamment forte pour recevoir la
   géométrie et la profondeur q3.

### À corriger avant de parler de contrat public complet

1. Les `PointId` employés sont désormais distincts des rangs Morton, mais ils
   restent fabriqués depuis l'ordre du `vector<P3>`. Ce ne sont pas encore des
   identités externes stables.
2. Le juge de supports ne valide pas le tie-break d'owner : le sujet utilise
   `pid`, l'oracle brut utilise encore les rangs Morton, puis tous deux
   comparent seulement le même triplet non orienté.
3. `Key3` stocke des `i32` et caste les `PointId` u32. Employer directement
   `PointId` évite une restriction implicite à `id<2^31`.
4. Le chemin q3 appelle encore « histogramme » une double boucle sur toutes
   les ancres ; il ne fait pas encore l'énumération par seaux annoncée et le
   calcul de `h_a/h_b` reste quadratique dans chaque facteur.
5. `--exact` signifie en réalité « exact sous régularité pour les boules
   pertinentes jusqu'à `K_max` ». Une boule déjà profonde peut arrêter le
   scan avant que toute sa coquille soit visitée.
6. Aucun CTest ne force actuellement `--census=tree`, `--exact`, un owner à
   égalité ou une permutation à IDs conservés.
7. L'OBig de l'oracle appelle `std::abort()` en cas de largeur insuffisante.
   Cela contredit l'interdit du dépôt sur les morts par signal ; un débordement
   doit produire un statut explicite `numeric_failure` ou un code contrôlé.

Le bon libellé actuel est donc :

> **pipeline q3 exact pour les supports réguliers peu profonds, avec cover
> partagé et oracle rationnel indépendant de la géométrie**, mais pas encore
> événement HGP public complet ni API d'identités stable.

---

## 1. Réception du filtre `h_a/h_b`

Pour un rectangle terminal `A x B`, le compte `h_cœur` fourni par la descente
complète exclut les positions appartenant à `A` ou `B`. Pour une ancre
`(a,b)`, `h_a(a)` compte seulement les témoins de `A sans {a}` universels
pour tout partenaire de `B`, et `h_b(b)` fait le symétrique dans `B sans
{b}`. Les trois ensembles d'identités sont donc disjoints.

Le minorant est

```text
h_cœur + h_a(a) + h_b(b).
```

Avec `need=h_3-h_cœur`, l'ancre est sûrement morte si et seulement si le
minorant atteint le seuil, donc lorsque

```text
h_a(a)+h_b(b) >= need.
```

Le code applique exactement cette comparaison. L'autorité 8 coins est reçue :
pour `a,z` fixés, l'ensemble des partenaires `b` tels que `z` appartienne au
fuseau est un cône convexe ouvert ; contenir tous les coins de la boîte
implique contenir toute son enveloppe convexe.

### 1.1 Interprétation du reçu

Le reçu `79dc862` est particulièrement informatif : sur la famille dure,
17 % des ancres éliminées retirent 32,9 % des porteurs testés et 46,7 % du
temps. Ce n'est pas un gain uniforme dû à une boucle raccourcie ; le filtre
cible bien les ancres longues dont les requêtes spatiales et les census sont
les plus chers.

Je reçois donc le filtre comme une vraie étape d'architecture, pas seulement
comme un probe exploratoire.

### 1.2 Ce qui n'est pas encore un histogramme énumératif

Le code q3 calcule `h_a/h_b`, puis parcourt encore tout `A x B` pour décider
chaque ancre. Il ne matérialise pas la liste des paires, mais il exécute toutes
les décisions. En outre, les comptes par extrémité utilisent encore les
produits `A x A` et `B x B`.

Le prochain raccord sans risque est :

1. saturer `h_a` et `h_b` à `need` ;
2. ranger les indices de `B` dans les `need+1` seaux de `h_b` ;
3. pour chaque `a`, calculer `t=need-min(h_a(a),need)` ;
4. émettre uniquement les `b` des seaux `0,...,t-1`.

Le coût de l'énumération devient

```text
O(|A|+|B|+nombre_de_survivantes),
```

au lieu de `O(|A||B|)`. Le calcul des valeurs `h_a/h_b` pourra ensuite être
remplacé par la jointure dual-tree à `range-add`, sous porte de parité avec la
version directe actuelle.

À publier séparément : `ha_pair_tests`, `anchor_decisions`, `anchors_emitted`,
`lens_or_cover_queries`. Sans ces compteurs, le temps global mélange le coût
du filtre et le travail qu'il évite.

---

## 2. Preuve du cover q3 `sqrt(3)D/2`

Fixons une ancre owner `(a,b)`, de longueur `D`, et son milieu `m`.

### 2.1 Les porteurs sont dans le cover

Un porteur `x` de la lentille vérifie `|x-a|<=D` et `|x-b|<=D`. L'identité du
parallélogramme donne

```text
4|x-m|^2 = 2|x-a|^2 + 2|x-b|^2 - D^2 <= 3D^2.
```

Ainsi `|x-m|<=sqrt(3)D/2`. Le test entier du code

```text
|2x-a-b|^2 <= 3D^2
```

est exactement cette condition, coquille comprise.

### 2.2 Les témoins et coquilles sont dans le cover

Soit `c` le circumcentre d'un triangle aigu dont `ab` est une arête maximale,
et `R` son rayon. L'angle opposé à `ab` est le plus grand angle, donc il est
dans `[pi/3,pi/2)`. Par conséquent

```text
R = D/(2 sin(angle)) <= D/sqrt(3).
```

La distance du centre au milieu de la corde `ab` vaut

```text
|c-m| = sqrt(R^2-D^2/4) <= D/(2sqrt(3)).
```

Pour tout point intérieur ou sur la circum-boule,

```text
|z-m| <= |z-c|+|c-m| <= R+D/(2sqrt(3)) <= sqrt(3)D/2.
```

Le cover fermé contient donc aussi toutes les coquilles externes. Le scan
plat est une autorité complète pour toute boule qui ne déclenche pas l'arrêt
par profondeur.

### 2.3 Arithmétique des boîtes

Sous u16, les composantes de `2z-a-b` sont dans `[-131070,131070]`. Leur norme
carrée et `3D^2` restent sous `2^36`, donc les `i64` de `cover_query` sont
suffisants. La distance minimale de la boîte au milieu doublé est calculée
axe par axe dans le bon sens ; aucune boîte admissible n'est élaguée.

Je reçois donc `cover_query` et son prédicat fermé.

### 2.4 Early-exit et portée du mot « exact »

Si le scan atteint `h_3` intérieurs, la profondeur vérifie `d>=h_3`. La boule
ne peut produire aucun événement pour `K<=K_max`, quelle que soit sa
coquille. L'arrêt est donc exact pour la forêt tronquée demandée.

En revanche, il ne certifie pas la position générale globale du nuage. Le
mode devrait être nommé explicitement

```text
exact_up_to_Kmax
```

ou déclarer `regularity_scope=up_to_Kmax`. Une validation globale des
cosphéricités est un autre problème et ne doit pas être promise par un census
saturé.

---

## 3. Réception et contre-audit de l'oracle q3

### 3.1 Ce que l'oracle valide réellement

L'oracle construit le circumcentre comme solution rationnelle du système

```text
2(b-a).c = |b|^2-|a|^2,
2(x-a).c = |x|^2-|a|^2,
n.c       = n.a,
n          = (b-a) x (x-a).
```

Cramer fournit `c=num/det`. Le signe de puissance est ensuite celui de

```text
|z det-num|^2 - |a det-num|^2.
```

Cette voie ne réutilise ni `Q3Form`, ni `axis_min`, ni `q3_ball_depth` pour sa
vérité. Le test de niveau croisé

```text
|a det-num|^2 (4G) = D E X det^2
```

est correct. Les largeurs annoncées tiennent dans 384 bits avec une marge
importante.

Les 37 212 triangles, les deux familles, la cosphère et le tétraèdre entier
forment une bonne première campagne. Les mutants `sign-p` et `prune-ge` sont
causaux : ils attaquent respectivement la stricte séparation
intérieur/coquille et l'élagage qui pourrait masquer une égalité.

Je reçois donc l'oracle comme autorité indépendante de la **géométrie q3 et
du census ponctuel**.

### 3.2 Indépendance à ne pas sur-vendre

L'oracle partage encore avec la production les primitives `p3_sub`, `p3_dot`,
`p3_cross`, `p3_norm2`, `edge_key_less` et l'index spatial. Ces primitives sont
simples et leurs largeurs sont faciles à recevoir, mais la phrase « aucun
défaut commun ne peut se compenser » est trop forte.

La séparation honnête est :

- oracle indépendant pour le circumcentre, la puissance, la profondeur et les
  coquilles ;
- oracle non encore indépendant pour les identités externes, le tie-break
  d'owner et les primitives vectorielles élémentaires.

### 3.3 Self-test obligatoire d'OBig

Un entier maison devient lui-même une nouvelle autorité arithmétique. Avant de
l'utiliser pour q4, ajouter un self-test autonome :

- additions avec retenue traversant plusieurs limbes ;
- soustractions avec emprunt traversant plusieurs limbes ;
- signes et zéro canonique ;
- produits aux frontières de limbes ;
- quelques milliers de valeurs aléatoires comparées à
  `boost::multiprecision::cpp_int` dans le test uniquement ;
- débordements volontairement injectés.

Le chemin oracle peut rester écrit avec OBig ; `cpp_int` ne sert qu'à tester
la calculatrice, pas à définir la géométrie.

### 3.4 `abort` doit devenir un statut

`std::abort()` transforme une insuffisance de largeur en signal. Le plan de
tests interdit précisément qu'un crash soit interprété comme un résultat.
Remplacer ce comportement par une propagation contrôlée :

```text
OBigResult { OBig value; bool overflow; }
```

ou une exception locale capturée dans `main`, qui rend un code déterministe
`numeric_failure`. Un oracle qui meurt violemment au moment où il découvre
qu'il ne sait plus compter n'est pas une preuve, seulement une réaction
humaine étonnamment fidèle.

---

## 4. Le juge d'owner ne juge pas encore l'owner

Dans `q3_events_probe`, le sujet appelle `anchor_owns_q3` avec `pid(ua)`,
`pid(ub)`, `pid(ux)`. L'oracle brut choisit pourtant son owner avec les rangs
Morton `i,j,k`. Il convertit ensuite seulement le triplet final en `pid`.
Comme le `SupportKey` non orienté est identique quel que soit l'owner choisi,
le juge peut rester 0/0 alors que les owners diffèrent.

Fixture minimale :

```text
PointId 0 : b=(12,10,0)
PointId 1 : a=(10,10,0)
PointId 2 : x=(11,13,0)
```

Les longueurs carrées sont

```text
|a-x|^2=10, |b-x|^2=10, |a-b|^2=4.
```

Le triangle est aigu. L'ordre Morton est `a,b,x`, donc le tie-break par rang
choisit `a-x`; le tie-break par vrais IDs choisit `b-x`. Le support trié reste
`{0,1,2}` et le juge actuel ne voit rien.

Correction : le record jugé doit contenir

```text
SupportKey + OwnerEdgeKey,
```

et l'oracle doit comparer les IDs externes par une implémentation
lexicographique locale, sans appeler `edge_key_less` du sujet.

---

## 5. Les `PointId` ne sont pas encore externes

Le raccord

```text
pid(u)=bucket_ids[bucket_start[u]]
```

sépare correctement l'identité du rang Morton. Cependant
`build_cloud_index(vector<P3>)` initialise toujours le `PointId` avec l'index
du record d'entrée. Une permutation physique des records change donc les IDs.

Introduire avant q4 :

```text
InputPoint { PointId id; P3 position; }
```

puis vérifier l'unicité des IDs. Le tri Morton doit déplacer le record sans
réécrire `id`. La porte correcte permute les records tout en conservant leurs
IDs et exige exactement les mêmes `OwnerEdgeKey`, `SupportKey`, digests et
événements.

L'overload `vector<P3>` peut rester pour les générateurs internes, mais son
statut doit être `generated_input_ids`, pas « vrais PointId externes ».

Enfin, `Key3` doit employer `PointId u[3]`. Les casts vers `i32` n'apportent
rien et contredisent le type public u32.

---

## 6. Portes manquantes avant le census médiateur

### P0 — correction et vérité

1. Deux CTests explicites par famille : `--census=cover` et
   `--census=tree`, avec même digest de `SupportKey` triés.
2. Publier `event_digest`; « mêmes événements » ne doit pas signifier
   seulement « même cardinal ».
3. Sur petit `n`, vérifier exhaustivement chaque ancre tuée par `h_a/h_b`
   avec `true_spindle_count>=h_3`.
4. Ajouter la fixture owner ci-dessus et comparer aussi l'owner.
5. Ajouter une fixture d'extra-shell et une porte `--exact` attendue au code
   de refus. Exemple cosphérique entier :

   ```text
   a=(15,10,0), b=(7,14,0), x=(7,6,0), z=(10,15,0).
   ```

   Les quatre points sont sur la sphère de centre `(10,10,0)` et rayon 5 ; le
   triangle support a pour côtés carrés `80,80,64` et est aigu.
6. Tester le bord fermé du cover avec le triangle équilatéral entier
   `(0,0,0),(2,2,0),(2,0,2)`, pour lequel `|2x-a-b|^2=3D^2`. Le mutant `<` à
   la place de `<=` doit perdre le porteur.
7. Ajouter le self-test OBig et remplacer `abort` par un statut.
8. Ajouter les fixtures de petites tailles `n=2,3,4` avec masque dynamique des
   lanes ; q3 doit exister dès trois sites.

### P1 — événement q3 complet

9. Publier `BallKey`, `ExactLevel`, profondeur et `InteriorIds`.
10. Distinguer `regular_subset_diagnostic` de `exact_up_to_Kmax` dans le
    statut, pas seulement dans une chaîne imprimée.
11. Publier les facettes `F_K^conn`, `F_K^render` et le payload de
    multifusion avant de déclarer l'étage « événements exacts ».

---

## 7. Prochaine primitive : ne plus matérialiser et trier tout le cover

Le cover trié est une excellente baseline. Son compteur montre cependant
237,6 millions de `CoverPoint` collectés et triés sur la famille dure. Si la
version suivante conserve cette liste, elle n'attaque pas le poste désormais
dominant.

Je conseille deux passages spatiaux par ancre :

1. **passage porteurs** : traverser le cover et ne conserver que les points de
   lentille, aigus et possédés ;
2. construire le petit LBVH des circumcentres de ces porteurs ;
3. **passage témoins en streaming** : retraverser l'arbre spatial du cover et
   envoyer chaque site, puis chaque bloc certifiable, vers le LBVH des
   centres, sans tableau `CoverPoint` et sans tri radial.

Deux traversées de l'arbre coûtent moins qu'une matérialisation de centaines
de points suivie d'un scan par porteur. Le chemin `cover trié` doit rester
comme référence appariée.

### 7.1 Version exacte en point fixe, i128 suffisant

Pour un porteur, `Q3Form` donne `G>0`, `W=2G(c-a)`, et avec `d=b-a` :

```text
N = W-Gd,
T = 2c-a-b = N/G.
```

Pour un site `z`, poser `u_z=2z-a-b`. Quatre fois la puissance géométrique est

```text
ell_z(T)=|u_z|^2-D^2-2u_z.T.
```

À l'échelle `S=2^32`, enfermer chaque coordonnée du centre par

```text
Tlo_i=floor(S N_i/G),
Thi_i=ceil(S N_i/G).
```

Un nœud de centres stocke les minima de `Tlo` et maxima de `Thi`. Pour un site
ponctuel, les extrema de

```text
E_z(Ts)=S(|u_z|^2-D^2)-2u_z.Ts
```

sur cette boîte sont obtenus en choisissant les extrémités selon le signe de
chaque composante de `u_z`.

Décisions :

```text
max E_z < 0 : range-add intérieur à tout le nœud,
min E_z > 0 : extérieur à tout le nœud,
sinon        : subdivision ; égalité gardée ouverte.
```

Sous u16, `N*S`, les coordonnées fixes et `E_z` tiennent en i128. Une largeur
192 bits n'est pas nécessaire pour ce premier prototype dirigé ; des
`static_assert` et fixtures aux extrêmes doivent porter cette affirmation.
La feuille conserve `q3_power` comme autorité exacte et collecte les
coquilles.

### 7.2 Mesures à publier

```text
carriers_per_anchor,
cover_nodes_visited,
cover_leaves_visited,
center_nodes_visited,
range_add_mass,
centers_saturated_by_blocks,
leaf_power_tests,
shells_detected.
```

La voie radiale par directions primitives reste un accélérateur utile lorsque
plusieurs porteurs partagent une direction, mais elle n'a pas de borne globale
sur un nuage générique. Le LBVH dirigé doit rester la référence exacte de la
prochaine étape.

---

## 8. Réception commit par commit

- `2d26e7a` : **reçu pour la correction q3 interne** : filtre fail-open,
  exact-once visible, mode de rejet local et optimisation des lanes à rayon
  nul. Réserves sur les IDs externes et le juge d'owner.
- `79dc862` : **reçu comme mesure appariée convaincante**. Le filtre retire
  bien le coût attendu ; ajouter un digest de sortie au reçu.
- `a047460` : **reçu mathématiquement et algorithmiquement** pour le cover
  fermé et le scan saturé jusqu'à `K_max`. Le gain 10x est crédible et le
  chemin doit être conservé comme baseline.
- `ebc8236` : **oracle q3 reçu pour la géométrie et la profondeur**. Avant q4,
  ajouter son self-test arithmétique, supprimer `abort` et séparer l'owner/ID
  de la production.

---

## 9. Ordre de travail conseillé à Claude

1. Corriger le juge d'owner, passer `Key3` en `PointId` et ajouter le digest
   cover/tree.
2. Ajouter la porte `--exact`, le self-test OBig et un statut de débordement
   contrôlé.
3. Introduire l'API `{PointId,position}` et les petites lanes dynamiques.
4. Rendre l'énumération `h_a/h_b` réellement par seaux.
5. Fermer l'enregistrement q3 complet : `BallKey`, niveau, intérieurs,
   facettes et multifusion.
6. Implémenter le census médiateur en deux passages, avec LBVH de centres et
   streaming du cover ; garder le cover trié comme référence.
7. Seulement ensuite ouvrir q4, qui pourra réutiliser des identités, statuts,
   oracles et payloads déjà reçus au lieu de multiplier les conventions
   implicites.

---

## Conclusion

La direction est très bonne. En quatre commits, Claude a supprimé environ
95 % du temps d'instruction q3 mesuré sur la famille adversariale et a ajouté
l'oracle indépendant réclamé avant q4. Le verrou n'est donc plus une
incertitude géométrique : c'est désormais un problème propre de mutualisation
du census et de finition des contrats publics.

La recommandation n'est pas de ralentir pour polir chaque commentaire. Elle
est de verrouiller maintenant les quelques portes qui empêchent une
régression silencieuse, puis d'exploiter le cover en streaming. C'est la voie
la plus courte vers une instruction q3 à la fois exacte, rapide et réellement
réutilisable par q4.