# Contre-audit de `772a8d9` : q2 impose aussi une masse linéaire de facettes de forêt

Date : 17 août 2026.  
Audit reçu : `CONTRE_AUDIT_0328_BORNE_POISSON_SORTIE_Q2_30M_20260817.md`.  
Pins de code concernés : `2b7bb3299e1a75f0fe9dd3bc5fdfff96e186fb57` pour le fold `sort/reduce`, `5107f4e53f8c027f779c3f2aa6ddc8d1400c8a28` pour le protocole G4.

## Verdict

La borne Poisson de `772a8d9` est correcte : en dimension trois, à chaque profondeur `j`, les paires q2 dont la boule diamétrale contient exactement `j` sites ont une intensité asymptotique égale à quatre par point.

Il faut cependant distinguer deux affirmations.

1. Les `4 K_max n` événements q2 montrent immédiatement que le flux complet de certificats de Gabriel ne peut pas être le produit résident associé au SLO 30M.
2. Pour `K=1`, ces événements ne sont pas une borne sur la taille du MST : environ `4n` arêtes de Gabriel se réduisent à `n-1` arêtes de MST. Le nombre d'événements n'est donc pas, à lui seul, une borne sur la forêt critique.

La bonne nouvelle mathématique est que, pour `K>=2`, on peut fermer cette lacune : chaque événement q2 de profondeur `j>=1` crée `j` facettes non actives distinctes, et ces facettes doivent réellement apparaître dans toute représentation explicite du K-MST au niveau des facettes.

Ainsi, q2 seule impose en espérance :

```text
2 K_max (K_max-1) n
```

facettes nouvelles à travers les ordres `K=2,...,K_max`, et au moins autant d'attachements dans une forêt couvrante explicite.

Pour `K_max=10`, cela vaut `180n`. À 30 millions de points : environ **5,4 milliards de facettes nées**, avant q3 et q4.

Le verrou de sortie ne concerne donc pas seulement le flux symbolique complet. Il concerne aussi une `connectivity_hierarchy` explicite qui prétend conserver tous les sommets-facettes du K-MST.

---

## 1. Rappel de la constante Poisson q2

Pour une paire non ordonnée `{a,b}` de longueur `D`, sa boule diamétrale ouverte a le volume

```text
v_2 D^3, avec v_2 = pi/6.
```

Sous un processus de Poisson homogène d'intensité `lambda`, Campbell-Mecke donne, pour le nombre `N_j` de paires ayant exactement `j` sites dans cette boule :

```text
E[N_j] / E[n] -> 4
```

pour tout `j>=0`, hors un terme de bord `o(n)`.

Une telle paire définit l'événement q2

```text
sigma(a,b) = {a,b} union I_ab,
|I_ab| = j,
K = j+1.
```

La boule diamétrale est la miniboule de `sigma(a,b)` et son support minimal est `{a,b}`.

---

## 2. Injection des événements q2 vers les facettes nées

Fixons un événement q2 de profondeur `j>=1` :

```text
sigma = {a,b} union I,
|I| = j.
```

Pour chaque `z in I`, considérons la facette

```text
tau_z = sigma minus {z}.
```

Elle a `j+1=K` sommets et contient encore `a,b`.

### 2.1 `a,b` est le diamètre unique de `tau_z`

Tout point de `I` est strictement à l'intérieur de la boule de centre `(a+b)/2` et de rayon `D/2`. Par conséquent :

```text
|a-u| < D,
|b-u| < D,
|u-v| < D
```

pour tous `u,v in I` distincts.

Ainsi `{a,b}` est l'unique paire de `tau_z` à distance `D`. La miniboule de `tau_z` reste la boule diamétrale de `a,b`, et

```text
rho(tau_z) = rho(sigma) = D/2.
```

La facette `tau_z` est donc non active : elle naît exactement dans le macro-lot de `sigma`.

### 2.2 Les `tau_z` sont globalement distinctes

À partir de la seule facette `tau_z`, on retrouve :

1. `{a,b}` comme son diamètre unique ;
2. la boule diamétrale `B_ab` ;
3. l'ensemble complet `I = X intersection int(B_ab)` ;
4. le point omis `z`, unique élément de `I minus tau_z`.

L'application

```text
(sigma,z) -> tau_z
```

est donc injective, sous le régime continu en position générale utilisé par la borne Poisson.

Aucune collision entre deux événements q2 ne peut réduire ce nombre.

### 2.3 Ces facettes doivent être raccordées à la forêt

Les deux facettes obtenues en retirant `a` ou `b` sont actives : retirer un point du support minimal fait strictement décroître le rayon de naissance.

Au niveau `rho(sigma)`, chaque `tau_z` est reliée par le K-simplexe `sigma` aux facettes actives. Elle n'est donc ni un certificat décoratif ni un sommet isolé à élaguer : c'est une nouvelle facette du K-polyèdre, attachée à une composante déjà présente.

Tout arbre couvrant exact du K-graphe doit ajouter au moins une arête pour intégrer chacun de ces nouveaux sommets. Pour un événement de profondeur `j`, il faut donc au moins `j` attachements nouveaux, indépendamment du fait que l'événement fusionne ou non plusieurs anciennes composantes.

---

## 3. Bornes fermées pour `K_max`

À la profondeur `j`, il y a asymptotiquement `4n` événements q2, chacun créant `j` facettes non actives distinctes.

En sommant `j=1,...,K_max-1` :

```text
E[nouvelles facettes q2] / E[n]
  -> 4 sum_{j=1}^{K_max-1} j
  = 2 K_max (K_max-1).
```

Le même nombre est une borne inférieure sur les attachements nécessaires d'une forêt explicite au niveau des facettes.

### Profil principal `K_max=10`

```text
180 facettes nouvelles par point,
180 attachements de forêt par point.
```

À `n=30 000 000` :

```text
5,4 milliards de facettes,
5,4 milliards d'attachements.
```

Même un enregistrement fictif de 16 octets par attachement représente déjà :

```text
86,4 Go.
```

### Profil secondaire `K_max=5`

```text
40 facettes nouvelles par point,
40 attachements par point,
soit 1,2 milliard à n=30M.
```

### Occurrences de `PointId` dans une représentation explicite des facettes

Chaque `tau_z` possède `j+1` identités. Le nombre moyen d'occurrences imposé par q2 vaut :

```text
4 sum_{j=1}^{K_max-1} j(j+1)
  = (4/3) K_max (K_max-1)(K_max+1)
```

par point.

Donc :

```text
K_max=10 : 1320 PointId par point,
            39,6 milliards de PointId à 30M,
            158,4 Go pour les seuls u32 ;

K_max=5  : 160 PointId par point,
            4,8 milliards de PointId à 30M,
            19,2 Go pour les seuls u32.
```

Cette borne porte sur des facettes distinctes, pas sur les occurrences redondantes du flux d'événements.

---

## 4. Ce que cette borne prouve, et ce qu'elle ne prouve pas

Elle prouve que les deux produits suivants sont nécessairement `output-sensitive` :

```text
gabriel_certificate_stream
facet_level_KMST_stream
```

Le second reste exact et beaucoup plus pertinent que le premier, mais sa matérialisation exhaustive ne peut pas être le résultat résident d'un calcul 30M en moins de 100 ms.

Elle ne prouve pas qu'une réponse exacte au niveau des points est impossible. Le Théorème 2 du manuscrit identifie les K-polyèdres aux amas discrets des composantes de

```text
L_K(r) = { y : |B(y,r) intersection X| >= K }.
```

Il autorise donc un produit exact orienté requête ou étiquetage sans prétendre sérialiser toutes les facettes qui en constituent un certificat simplicial.

Il ne fournit toutefois pas gratuitement l'algorithme rapide : calculer les composantes de `L_K(r)` reste le problème topologique central. Il fixe seulement le bon objet de sortie.

---

## 5. Contrat de produit recommandé

La séparation proposée par `0328bf5` doit être légèrement durcie.

### A. `gabriel_certificate_stream`

Toutes les `BallKey`, tous les événements et toutes les incidences symboliques.

```text
exact ;
streaming/out-of-core ;
temps et mémoire proportionnels à la sortie.
```

### B. `facet_hierarchy_stream`

K-MST exact au niveau des facettes, avec rayons de naissance, macro-lots, naissances, croissances et multifusions.

```text
exact ;
streaming/out-of-core ;
q2 seule impose 2 K_max(K_max-1)n facettes nées en espérance.
```

Ce produit ne doit pas être présenté comme une structure résidente 30M.

### C. `implicit_hierarchy_index`

Structure exacte permettant des requêtes sans matérialiser toutes les facettes simultanément.

```text
exact ;
construction et mémoire à définir ;
réponses output-sensitive.
```

C'est le véritable objectif de recherche si l'on veut conserver toute la hiérarchie mathématique.

### D. `point_query_or_partition`

Pour des paramètres déclarés `(K,r,psi,min_cluster_size)`, produire :

```text
soit les appartenances recouvrantes exactes,
soit la partition stricte du § 9.1,
soit les scores nécessaires à cette partition.
```

C'est le seul produit actuellement crédible pour un SLO fixe à 30M. Le contrat doit préciser si le temps est :

```text
cold_build_from_points
ou
warm_query_after_index.
```

Cent millisecondes pour un `warm_query` et cent millisecondes pour construire toute la tour depuis les points ne sont pas le même problème, malgré l'enthousiasme traditionnel des tableaux de benchmark à les mettre dans une même colonne.

---

## 6. Conséquences d'implémentation

### 6.1 Ne pas promouvoir aveuglément tous les index chauds en u64

Les compteurs globaux, tailles, offsets de fichiers et préflights doivent être en u64.

En revanche, doubler tous les `fid`, `event_id` et tableaux du kernel aggrave précisément le verrou mémoire. La politique correcte est :

```text
global_count/global_offset : u64 ;
shard_id/local_index       : u32 ;
refus ou nouvelle tuile avant dépassement local de 2^32.
```

Une porte doit vérifier les additions d'offsets et les limites de tuile. Le flux par K et par partition rend cette convention naturelle sur CPU comme sur GPU.

### 6.2 Mesurer trois cardinalités distinctes

Pour chaque K, publier séparément :

```text
Gabriel events generated,
unique facets born,
critical forest edges/deltas emitted.
```

Le rapport `critical/Gabriel` mesure le gain encore possible par un filtre de séparation de type RNG-HGP ou par une recherche Boruvka directe. Le rapport `born_facets/events` mesure la part incompressible d'une sortie au niveau des facettes.

### 6.3 Le prochain verrou mathématique n'est plus un autre census

Pour `facet_hierarchy_stream`, la question décisive devient : peut-on chercher directement les K-simplexes séparants ou les arêtes du K-MST sans énumérer tout le graphe de Gabriel ?

Le cas `K=1` montre que oui en principe : environ `4n` arêtes q2/Gabriel se réduisent à `n-1` arêtes du MST. Pour `K>=2`, le filtre RNG-HGP et une stratégie de Boruvka sur composantes de facettes sont les directions pertinentes.

Pour `point_query_or_partition`, la question est différente : comment exploiter la caractérisation par `L_K(r)` ou accumuler directement les scores du § 9.1 sans conserver les certificats simpliciaux ?

Ces deux voies ne doivent plus partager par défaut la même matérialisation.

---

## 7. Portes utiles

1. `q2_birth_lower_bound` sur des nuages Poisson croissants : par profondeur `j`, compter les événements q2 et les facettes `tau_z` distinctes ; vérifier l'injection par diamètre unique.
2. `resident_vs_streaming` sur petits n : égalité des partitions, macro-lots, `ComponentDelta`, naissances et rendu.
3. `u64_global_u32_local` : base globale artificielle proche de `2^32`, sans allocation géante ; le mutant qui additionne en u32 doit mourir.
4. `product_contract` : le même run doit refuser ou streamer selon `product` et `max_output_bytes`, jamais changer silencieusement d'objet.
5. `K=1 distinction` : environ quatre fois plus de q2/Gabriel que d'arêtes MST ; cette porte grave que le nombre de certificats n'est pas le nombre de transitions critiques.

---

## Conclusion

Le calcul Poisson de `772a8d9` est juste. Sa conséquence peut être rendue plus forte et plus précise : pour `K_max=10`, q2 seule crée en espérance **180 nouvelles facettes par point** dans les ordres `K=2,...,10`.

Ainsi, même le K-MST explicite au niveau des facettes est un produit massif et output-sensitive. La sortie 30M associée au SLO doit être une requête point-level ou un index implicite, pas la sérialisation de la preuve simpliciale complète.

La géométrie v4 reste utile : `BallKey`, `SpherePlateau`, le fold `sort/reduce` et le sweep axial sont les bonnes briques des flux d'autorité. Il faut désormais empêcher que le produit rapide soit défini comme leur trace exhaustive.