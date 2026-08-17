# Audit bloquant après `c829872` — la prochaine limite est la taille de l'objet, pas le prédicat

Date : 17 août 2026.  
Pin audité : `c829872904e6f6eeb0d277e60c9fa173448fac33`.

## Verdict

Je reçois positivement les deux avancées du cycle courant :

- le fold `sort/reduce` conserve la sémantique des macro-lots, rôles, naissances, croissances, multifusions et multiplicités du rendu ;
- le contre-audit axial à deux côtés est mathématiquement correct : pour un seed fixé,

  ```text
  d_cover(mu) = permanents + positifs strictement avant mu
                           + négatifs strictement après mu
  ```

  remplace exactement le scan `q4_power < 0` sur le cover.

Le prochain verrou utile n'est toutefois plus une micro-optimisation de q4 ou de `std::map`. Le contrat public actuel exige simultanément :

```text
forêt HGP complète K=1..10,
événements exacts et rendu symbolique complet,
30 millions de points,
moins de 100 ms sur G4.
```

Avec la représentation explicite actuellement construite, ces quatre exigences sont incompatibles par **taille de sortie**. Il faut trancher le produit exact rendu à l'échelle avant de poursuivre le backend GPU.

---

## 1. Les mesures ont déjà exposé la taille de l'objet

Sur `uniform,n=8000,smax=11`, le reçu publie :

```text
3 126 158 événements,
1 974 086 nœuds de fusion,
19 465 140 unions effectives,
6 786 612 kB de pic RSS isolé avant la dernière compression du fold.
```

Cela représente environ :

```text
391 événements par point,
247 nœuds de fusion par point.
```

Les valeurs observées par point croissent encore entre `n=400`, `800`, `1600` et `8000`. La famille `uniform` dilate son domaine comme `n^(1/3)` afin de garder une densité volumique constante : on ne doit donc pas attendre que cette masse locale disparaisse miraculeusement à grande taille.

Même une extrapolation seulement linéaire du ratio `n=8000` donne à `n=30 000 000` :

```text
~11 723 092 500 événements,
~ 7 402 822 500 nœuds de fusion.
```

Ce n'est pas une preuve asymptotique tirée de quatre points ; c'est déjà une borne d'ingénierie suffisante pour refuser l'architecture résidente actuelle. Diviser ces constantes par quatre ne changerait pas la conclusion.

---

## 2. Bornes de mémoire indépendantes de `std::map`

### 2.1 Le vecteur `ForestEvent`

Le type courant contient matériellement :

```text
20 PointId fixes                         : 80 octets,
Q4Level = 3*u64 + i128                   : 40 octets,
q + d + active_mask                      :  4 octets,
```

soit **au moins 124 octets de champs** avant alignement. Le `sizeof` réel est vraisemblablement supérieur et doit être publié par reçu, mais ce minimum suffit.

Au ratio observé à `n=8000` :

```text
11,72 milliards * 124 octets > 1,45 téraoctet
```

pour les seuls événements, sans candidats, BallData, facettes, union-find, deltas ni rendu.

Même une représentation fictive de seulement **16 octets par événement**, trop petite pour porter ses identités et son niveau exact, demanderait encore :

```text
~187,6 Go.
```

### 2.2 Le tri d'incidences

Le nouveau `FRec` contient une `FacetKey` fixe de dix IDs, un index d'événement et un slot, soit au moins 46 octets de champs. Chaque événement a au moins deux facettes. La borne minimale est donc déjà :

```text
2 * 11,72 milliards * 46 octets > 1,07 téraoctet
```

pour le tri d'incidences, avant même de considérer que les événements réels ont généralement bien plus de deux facettes.

Le passage `map -> sort/reduce` est donc une bonne primitive, mais il ne transforme pas un objet de plusieurs téraoctets en objet résident GPU. Il retire une constante algorithmique ; il ne supprime pas la sortie.

### 2.3 Même le squelette de fusion est trop gros

L'extrapolation linéaire des seuls `ForestNode{batch,absorbed}` donne environ 7,4 milliards de nœuds. À 16 octets par nœud :

```text
~118 Go
```

sans `ComponentDelta`, sans parents, sans facettes nées et sans table verticale entre ordres.

### 2.4 Le SLO de 100 ms doit nommer ce qu'il exclut

Écrire seulement les événements idéalisés à 16 octets en 100 ms demanderait :

```text
~1,88 téraoctet/s
```

de sortie, avant tout calcul géométrique. Le SLO ne peut donc pas désigner la matérialisation de la forêt complète explicite.

Enfin, le code utilise encore des index 32 bits dans le chemin d'incidences :

```cpp
FRec::e      : u32,
fid          : u32,
first_batch  : u32,
ev_fid       : u32.
```

Le flux extrapolé dépasse `2^32` événements et probablement `2^32` facettes. Il y aurait une faute de correction même si une machine disposait de la mémoire nécessaire.

---

## 3. Décision de contrat indispensable

Il faut distinguer trois produits, que le README confond encore.

### A. `full_symbolic_stream`

Objet exact complet : toutes les boules, tous les événements, toutes les incidences symboliques du rendu et toutes les applications verticales.

```text
statut : exact, mais externe/out-of-core ;
SLO    : proportionnel à la taille produite, jamais 100 ms à 30M.
```

### B. `connectivity_hierarchy`

Forêt de connectivité exacte : macro-fusions et transitions nécessaires aux composantes, sans conserver tous les événements ni toutes les incidences de rendu.

```text
statut : exact pour les K-polyèdres ;
stockage : deltas critiques + feuilles/birth levels comprimés.
```

### C. `query_or_labels`

Réponse à un ensemble demandé de `(K,r)`, ou étiquettes finales pour un choix de seuils et de `psi`, sans matérialiser toute la tour symbolique.

```text
statut : exact pour les requêtes déclarées ;
SLO GPU : c'est le seul candidat crédible au contrat <100 ms.
```

Le pipeline doit publier explicitement :

```text
product = full_symbolic_stream
        | connectivity_hierarchy
        | query_or_labels.
```

Sans cette distinction, une mesure « 100 ms » pourra seulement cacher le coût de sortie dans une étape non chronométrée, vieille coutume des benchmarks qui n'améliore malheureusement pas la mémoire physique.

---

## 4. Route d'implémentation constructive

### 4.1 Préflight de cardinalité avant allocation

Ajouter une première classe de reçus 64 bits par K :

```text
ballkeys,
expanded_events,
facet_incidences,
unique_facets_upper_bound,
expected_component_deltas,
bytes_by_buffer.
```

Avant toute publication :

```text
count -> preflight(max_bytes,max_records) -> fill
```

et `resource_exhausted` si le produit demandé ne tient pas. Aucun `reserve` optimiste sur des milliards de records.

Promouvoir dès maintenant en 64 bits les index d'événements, de facettes et de lots. Une porte à seuil artificiellement petit doit tuer le mutant `u32-event-index` sans avoir besoin d'allouer quatre milliards de records.

### 4.2 Ne plus construire `ev_k[1..10]` résident

Le chemin actuel fait :

```cpp
BallData -> expand_plateau -> ev_k[K].push_back -> build_forest(copy)
```

La référence d'échelle doit devenir un flux partitionné :

```text
BallData par tuile
  -> PlateauEvent
  -> records externes par K et niveau
  -> tri/merge externe
  -> fold macro-lot streaming
  -> rejet immédiat de l'événement après consommation.
```

Le fold `sort/reduce` nouvellement écrit est justement la bonne base pour cette version : ses records plats peuvent être produits par partitions, triés localement puis fusionnés, au lieu d'être tous résidents.

### 4.3 Séparer connectivité et rendu

Pour `connectivity_hierarchy` :

- traiter les événements en ordre de niveau ;
- mettre à jour le DSU et émettre seulement les `ComponentDelta` nécessaires ;
- libérer l'événement dès la fin du macro-lot ;
- conserver les niveaux de naissance des facettes sous forme comprimée ou calculable à la demande.

Pour le rendu :

- si `psi` est fixé, accumuler directement `S_tau`, `T_x`, masses et votes ;
- ne pas conserver la série symbolique `(facet,batch,multiplicity)` entière ;
- si le rendu symbolique pour tout `psi` est réellement exigé, l'émettre comme artefact trié externe, pas comme vecteur GPU résident.

La connectivité exacte et le rendu symbolique n'ont aucune raison de payer simultanément leur pire emprise.

### 4.4 Garder `SpherePlateau` comme unité native

Sur une coquille multiple, ne pas expanser immédiatement tous les sous-ensembles `T`. Conserver :

```text
BallKey, niveau, I_B, U_B, supports minimaux
```

et laisser chaque consommateur développer seulement ce dont il a besoin. Cela ne résout pas le régime générique où une boule donne un événement, mais empêche les plateaux u16 d'ajouter une explosion combinatoire inutile.

### 4.5 Le backend GPU vient après le contrat de sortie

Le futur kernel peut paralléliser :

```text
génération,
tri par partitions,
census,
réductions segmentées,
forêts indépendantes par K.
```

Il ne peut pas abolir la taille d'un résultat explicitement demandé. Le port CUDA doit donc viser un produit nommé et une borne de sortie, pas simplement reproduire `std::vector<ForestEvent>` en mémoire device.

---

## 5. Portes à graver maintenant

1. `--output-preflight-only` : compte exact sans matérialisation, par K.
2. Reçu `sizeof` des structures et octets projetés par buffer.
3. `max_output_bytes` très petit : refus transactionnel avant allocation.
4. Mutant `u32-event-index` tué avec une base d'index artificielle proche de `2^32`.
5. Chemin résident contre chemin streaming sur petits n : égalité de
   `ComponentDelta`, rendu, partitions et niveaux.
6. Pentes séparées :

   ```text
   input points,
   événements produits,
   événements conservés,
   octets de sortie,
   octets de travail maximum.
   ```

Le temps seul ne suffit plus : à ce stade, le nombre d'octets est un invariant d'architecture.

---

## 6. Ordre recommandé

1. Trancher le produit exact associé au SLO de 100 ms.
2. Ajouter les compteurs 64 bits et le préflight de sortie.
3. Promouvoir tous les index du fold en 64 bits.
4. Construire la référence streaming/out-of-core, confrontée à la version résidente.
5. Seulement ensuite porter les kernels plats sur GPU.
6. Garder le sweep axial à deux côtés comme optimisation opt-in ; il réduit le travail de génération, mais ne traite pas la masse de sortie finale.

## Conclusion

La v4 a maintenant une géométrie beaucoup plus solide et un fold nettement meilleur. Ce travail n'est pas perdu : `BallKey`, `SpherePlateau` et `sort/reduce` sont exactement les briques d'un pipeline streaming.

Mais aucune nouvelle constante de fuseau ne peut faire tenir explicitement plusieurs milliards d'événements et de nœuds dans le contrat actuel. Le verrou mathématique et architectural est désormais de **ne pas confondre l'objet exact avec sa matérialisation exhaustive en mémoire**.

Il faut choisir ce que la G4 doit rendre en 100 ms. Une fois ce choix écrit, Claude pourra optimiser un problème défini au lieu de poursuivre une sortie dont la seule écriture dépasse déjà le SLO.