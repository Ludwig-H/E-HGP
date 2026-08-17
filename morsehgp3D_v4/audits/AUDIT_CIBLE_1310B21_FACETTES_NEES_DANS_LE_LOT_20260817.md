# Audit ciblé après `1310b21` — ne pas compter les facettes nées dans le lot comme des composantes absorbées

Date : 17 août 2026.  
Pin de forêt audité : `1310b21a4b00e9bd2ac564eae895cdf89fc0e7f4`.  
Pins événementiels également contrôlés : `2c76e9a`, `2aa0c3a`.  
Audit concurrent pris en compte : `bc2d502` sur les plateaux cosphériques u16.

## Verdict

Le cours général est bon :

- la sélection axiale q4 est exacte et correctement laissée optionnelle après sa mesure CPU négative ;
- la lane q2 est correcte dans le régime régulier ;
- le tri sémantique U320 et les macro-lots de niveaux égaux sont la bonne architecture ;
- les **partitions** calculées par la fermeture union-find du lot sont correctes pour le sous-flux régulier.

Je confirme aussi le verrou de `bc2d502` : la forêt publique ne peut pas supprimer les événements à coquille sur une grille u16. Je ne le répète pas ici.

Il reste en revanche une erreur distincte dans la **chronologie des nœuds du dendrogramme** : le code compte actuellement les facettes qui naissent dans le lot comme des composantes pré-lot absorbées. Cela gonfle l'arité des nœuds et peut créer des nœuds fantômes, même lorsque la partition finale est correcte.

---

## 1. Où se produit l'erreur

Dans `build_forest`, toutes les facettes du lot sont d'abord créées par `facet_id`, puis toutes leurs racines sont insérées dans `prebatch_roots` :

```text
facettes actives       -> arms
facettes non actives   -> arms
puis toutes arms       -> prebatch_roots
```

Or, pour un événement régulier

```text
sigma = S union I,
```

les rôles ne sont pas symétriques :

- `sigma sans {s}`, avec `s in S`, est **active** et naît strictement avant `rho(sigma)` ;
- `sigma sans {z}`, avec `z in I`, conserve le même support `S` et naît **exactement** à `rho(sigma)`.

Une facette non active créée pour la première fois dans le lot n'est donc pas une composante préexistante. Elle s'attache au niveau courant ; elle ne doit pas devenir un enfant supplémentaire du nœud de fusion.

Le juge indépendant répète actuellement la même convention : il crée toutes les facettes du lot, place toutes leurs racines dans `pre`, puis compte les racines post-lot. Il est indépendant sur la miniboule, mais pas encore sur cette sémantique de naissance.

---

## 2. Fixture minimale qui sépare partition et dendrogramme

Prendre trois points :

```text
a = (0,0,0),
b = (4,0,0),
z = (2,1,0).
```

La boule minimale de `sigma={a,b,z}` est la boule diamétrale de `ab` :

```text
rho(sigma)^2 = |ab|^2/4 = 4.
```

C'est un événement q2 de profondeur `d=1`, donc de la forêt `K=2`.
Ses trois facettes sont :

```text
{a,z}, {b,z}, {a,b}.
```

Les deux premières sont actives :

```text
rho({a,z})^2 = rho({b,z})^2 = 5/4 < 4.
```

La troisième est non active :

```text
rho({a,b})^2 = 4.
```

Juste avant le lot `R^2=4`, il existe donc **deux** composantes singleton pertinentes, `{a,z}` et `{b,z}`. Au lot, elles fusionnent et `{a,b}` naît puis s'attache.

Le nœud correct a :

```text
absorbed = 2.
```

Le code courant crée les trois IDs avant de geler les racines et obtient :

```text
absorbed = 3.
```

La partition après le lot est identique dans les deux cas ; seul le dendrogramme révèle l'erreur. C'est précisément pourquoi les portes de partition restent vertes.

Ajouter une porte :

```text
q2_one_interior_attachment
```

et un mutant :

```text
count_new_attachment_as_prebatch
```

qui doit produire `absorbed=3` au lieu de `2` et mourir.

---

## 3. Correction minimale du macro-lot

Il faut classifier les facettes sur **tout le lot avant les unions** :

```cpp
struct BatchFacetRole {
  bool existed_before_batch;
  bool active_in_batch;
  bool attachment_in_batch;
  FacetKey key;
  i32 id;
};
```

Procédure :

1. parcourir tous les événements du lot et agréger les rôles par `FacetKey` ;
2. mémoriser `existed_before_batch` avant toute création d'ID du lot ;
3. créer les IDs manquants ;
4. définir les racines pré-lot par

```text
prebatch = active_in_batch OR existed_before_batch;
```

5. exclure de `prebatch_roots` toute facette qui est seulement

```text
attachment_in_batch AND NOT existed_before_batch;
```

6. effectuer ensuite les unions sur **toutes** les facettes, comme aujourd'hui ;
7. calculer `absorbed` uniquement à partir des racines pré-lot ainsi définies.

Deux invariants utiles tombent gratuitement :

```text
attachment_in_batch AND existed_before_batch
    => incohérence de niveau du flux ;

active_in_batch AND attachment_in_batch au même niveau
    => incohérence de rayon de naissance.
```

Une facette active peut ne jamais avoir été rencontrée par un événement antérieur. Elle doit néanmoins compter comme singleton pré-lot, puisque son propre rayon de naissance est strictement inférieur. C'est pourquoi le bon critère n'est pas seulement `existed_before_batch`, mais bien

```text
active_in_batch OR existed_before_batch.
```

Les facettes nouvelles non actives restent dans la fermeture union-find et dans l'instantané **après** le lot ; elles sont seulement retirées de la liste des enfants absorbés.

---

## 4. Le juge doit être corrigé par une voie distincte

Le juge connaît déjà le support minimal de chaque `JSimplex`. Il peut donc classifier sans réutiliser le code sujet :

```text
drop d'un point du support   -> active,
drop d'un point hors support -> attachment au niveau courant.
```

Il doit construire sa propre table de rôles de lot et compter uniquement les racines conceptuellement présentes avant le niveau. Comparer ensuite :

```text
partition après chaque lot,
nombre de nœuds,
multiensemble (lot, absorbed),
new_attachments,
attachment_level_violations.
```

La fixture ci-dessus doit être jugée par une attente mathématique explicite `absorbed=2`, pas seulement par accord sujet-juge. Deux implémentations reproduisant la même convention erronée sont d'une entente admirable, mais toujours erronée.

---

## 5. Raccord avec le plateau sphérique de `bc2d502`

La même distinction s'étend proprement au futur `SpherePlateau`.
Pour

```text
sigma = I_B union T,
```

soit `Supports_B(T)` l'ensemble des supports minimaux de la boule `B` contenus dans `T`. Pour un sommet de coquille `v in T` :

```text
sigma sans {v} garde la même miniboule B
ssi
il existe S in Supports_B(T) avec v notin S.
```

Ainsi la facette est active exactement lorsque

```text
v appartient à l'intersection de tous les supports S contenus dans T.
```

Retirer un intérieur `z in I_B` laisse toujours la coquille `T` inchangée : la facette est non active et naît dans le lot.

Cette formule donne directement les rôles du plateau dégénéré sans recalculer une miniboule par facette. Elle complète l'ABI `SpherePlateau` proposée par `bc2d502`.

---

## Ordre utile

1. Corriger le comptage pré-lot et graver la fixture q2 à un intérieur.
2. Corriger le juge indépendamment.
3. Implémenter ou refuser globalement les plateaux cosphériques, conformément à `bc2d502`.
4. Raccorder seulement ensuite les flux WSPD réels au fold.
5. Ouvrir le rendu § 9.1 après stabilisation de ces rôles, puisqu'ils déterminent précisément `F_K^conn`.

## Conclusion

Le noyau actuel calcule la bonne fermeture de connectivité, ce qui est l'essentiel et mérite d'être conservé. La correction demandée est locale : ne pas transformer les facettes nées au niveau courant en enfants préexistants du dendrogramme.

Sans cette correction, les partitions HGP peuvent être justes tandis que les arités, les nœuds et les persistences nulles du dendrogramme sont fausses. Avec elle, le macro-lot correspond réellement à la fusion des composantes présentes juste avant le niveau, et les facettes nouvelles deviennent de simples attachements, comme l'exige la filtration.