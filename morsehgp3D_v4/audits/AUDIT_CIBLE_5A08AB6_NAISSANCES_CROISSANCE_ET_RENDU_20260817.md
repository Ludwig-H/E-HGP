# Audit ciblé après `5a08ab6` — la forêt doit enregistrer naissances et croissances, pas seulement les fusions

Date : 17 août 2026.  
Pin audité : `5a08ab682b53c13fcea1aee4ccd0dad2fe928644`.  
Commits reçus conjointement : `052fed427b75bab7356da796544977a7a905d3e8` et `5a08ab682b53c13fcea1aee4ccd0dad2fe928644`.

## Verdict

Les deux corrections sont bonnes et je les reçois :

- le quotient `SpherePlateau` implémente correctement la caractérisation `sigma = I_B union T`, `c in conv(T)` fermé ;
- les lanes q2/q3/q4 sont bien les générateurs des BallKeys minimales, puis le census commun reconstruit `I_B/U_B` ;
- le rôle d'une facette de plateau est correctement décidé par son rayon de naissance : retrait `v in T` actif ssi `c notin conv(T sans v)`, retrait d'un intérieur = attachement ;
- `build_forest` ne compte plus les attachements nés dans le lot comme enfants préexistants ;
- le juge recalcule désormais les rayons des facettes par une voie indépendante.

Le passage de `plateaux_multi = 369` à `144` confirme que la correction n'était pas cosmétique. La fermeture union-find et les partitions après lot sont maintenant crédibles.

Il reste un seul raccord réellement bloquant avant de brancher les flux WSPD et le rendu : **`ForestResult` est encore un squelette de fusions, pas la hiérarchie HGP complète.**

---

## 1. Une composante peut naître entière dans un lot

Le manuscrit définit `theta_K^HGP(r)` par les composantes de `Gamma_K(X,r)` et précise que les K-polyèdres **croissent et fusionnent**. Un macro-lot peut donc avoir trois effets distincts sur une composante post-lot :

```text
0 composante pré-lot  -> naissance ;
1 composante pré-lot  -> croissance / continuation ;
>= 2 composantes      -> fusion.
```

Le code courant n'émet un `ForestNode` que dans le troisième cas. `new_attachments` n'est qu'un compteur global. Il ne dit ni quelles facettes sont nées, ni dans quelle composante elles entrent.

### Fixture déjà présente : le carré cocyclique, K=3

Pour les quatre points du carré, au niveau `R^2 = 100` :

- les quatre triangles naissent dans le lot ;
- le 3-simplexe les relie ;
- aucune composante de `Gamma_3` n'existe strictement avant ;
- après le lot, une composante non triviale de quatre facettes existe.

La correction `5a08ab6` conclut justement : `0` enfant absorbé, donc aucun **nœud de fusion**. Mais « aucun nœud de fusion » ne signifie pas « aucune information à publier » : il faut un **enregistrement de naissance de composante** au niveau 100. Sans lui, le flux de sortie ne permet pas de reconstruire `theta_3^HGP(r)` ; seul le snapshot de test, non conservé en production, connaît cette composante.

De même, avec une seule racine pré-lot et plusieurs attachements, la topologie ne fusionne pas, mais le polyèdre croît et `F_K^render` change.

---

## 2. `ForestNode{batch, absorbed}` ne porte pas l'arbre

Même pour les vraies fusions, le record courant ne contient que :

```cpp
batch;
absorbed;
```

Il manque :

- l'identité de la composante post-lot ;
- l'identité des composantes enfants ;
- les facettes nées dans le lot et rattachées à cette composante ;
- les événements responsables.

Deux forêts différentes peuvent avoir le même multiensemble `(batch, absorbed)`. Les snapshots du juge détectent actuellement une mauvaise partition, mais l'ABI publique ne transporte pas ces snapshots et ne peut donc pas reconstruire les relations parent-enfants.

Fixture simple pour la porte d'identité : quatre points formant deux paires très éloignées, avec les deux arêtes de même longueur au premier lot. Le résultat comporte deux fusions binaires indépendantes. Un mutant qui croise les deux paires conserve le multiensemble

```text
(batch 0, absorbed 2), (batch 0, absorbed 2)
```

mais produit une partition fausse. Le juge par snapshots le voit ; le record actuel, non.

---

## 3. ABI minimale recommandée : un delta par composante post-lot

Pour chaque racine post-lot touchée, agréger :

```cpp
enum class DeltaKind : uint8_t {
    kBirth,       // aucun parent pré-lot
    kGrowth,      // un parent pré-lot
    kMerge        // au moins deux parents pré-lot
};

struct ComponentDelta {
    BatchId batch;
    ExactLevel level;
    ComponentId output;
    DeltaKind kind;
    SmallComponentIds parents;   // identité, pas seulement cardinal
    FacetRange born_facets;      // attachements créés dans ce lot
    EventRange events;           // hyperévénements du lot touchant output
};
```

Algorithme, sans changer la fermeture déjà reçue :

1. geler les racines pré-lot comme dans `5a08ab6` ;
2. effectuer toutes les unions du lot ;
3. grouper les racines pré-lot par racine post-lot ;
4. grouper aussi les facettes `attachment && !existed && !active` par cette racine post-lot ;
5. émettre exactement un `ComponentDelta` par composante post-lot touchée :
   - `parents.empty()` : naissance ;
   - `parents.size()==1` : croissance ;
   - `parents.size()>=2` : fusion ;
6. attribuer un `ComponentId` stable : nouveau pour naissance/fusion, conservé pour croissance.

Le `ForestNode` actuel devient alors une vue dérivée des seuls deltas `kMerge`, avec `absorbed = parents.size()`.

### Convention sur les composantes isolées

La Définition 22 inclut les composantes réduites à une facette ; les Théorèmes 5/6 les écartent seulement lorsqu'ils comparent le K-MST aux polyèdres **non triviaux**. Deux politiques sont possibles, mais elles doivent être explicites :

- hiérarchie complète : les facettes isolées sont des feuilles à leur rayon propre ;
- hiérarchie non triviale : une transition à un parent singleton peut devenir la naissance publique du composant non trivial.

Dans les deux cas, le niveau de naissance exact d'une facette ne se résume pas au bit `active`. Pour la condensation/persistance, construire une table `FacetKey -> rho(facet)^2` en dédupliquant les facettes et en calculant leur miniboule exacte (au plus dix points). Il ne faut pas supposer que toute facette est elle-même un événement de Gabriel de l'ordre inférieur.

---

## 4. Le rendu exige toutes les incidences, pas seulement les rôles de connexion

Ce contrat est déjà tranché dans `MATHEMATIQUES.md` § 2.0 :

```text
F_K^render = toutes les facettes distinctes de tous les événements ;
F_K^conn   = la compression suffisante pour la connectivité.
```

Les attachements nés dans le lot ne sont pas des enfants de fusion, mais ils appartiennent pleinement à `F_K^render`. Le carré K=3 est encore la fixture parfaite : toutes les facettes y sont des attachements. Une variante active-only produirait un rendu vide alors qu'un K-polyèdre non trivial vient de naître.

De plus, `S_tau` somme les contributions de **chaque** K-simplexe incident. Une compression d'un `SpherePlateau` par supports minimaux doit préserver les multiplicités d'incidence, pas seulement la connectivité. Pour une boule `B`, écrire explicitement :

```text
mult_B(tau)
  = nombre de T subset U_B tels que
    |T| = K+1-|I_B|,
    c_B in conv(T),
    tau facette de I_B union T.
```

Alors la contribution du plateau au score est

```text
Delta S_tau = mult_B(tau) * psi(r_B).
```

Le chemin oracle peut continuer à énumérer les `T`. Le futur chemin comprimé doit soit produire ces multiplicités, soit conserver un objet symbolique permettant de les calculer. Une compression qui ne garde qu'un arbre couvrant est exacte pour `F_K^conn`, mais fausse pour le § 9.1.

---

## 5. Portes utiles avant le raccord WSPD

1. **`square_K3_component_birth`** : au niveau 100, zéro parent pré-lot, quatre facettes nées, un delta `kBirth` ; le mutant `drop_birth_component` doit rendre la hiérarchie vide à K=3.
2. **`q2_one_interior_attachment`** : conserver l'attente actuelle `parents=2`, et ajouter `born_facets={{a,b}}` dans le même delta de fusion.
3. **`two_independent_equal_level_merges`** : comparer les ensembles d'identités enfants, pas seulement deux valeurs `absorbed=2`.
4. **`render_keeps_batch_born_facets`** : sur le carré K=3, `F_K^render` contient les quatre triangles ; un mutant active-only meurt.
5. **`plateau_render_multiplicity`** : sur le carré K=2, chaque côté/diagonale reçoit exactement le nombre d'incidences donné par les quatre triangles rectangles.

---

## Ordre conseillé à Claude

1. Étendre l'ABI de forêt aux `ComponentDelta` avant de figer le raccord des flux WSPD.
2. Faire juger les identités parent-enfant, les naissances et les facettes nées par le chemin miniboule indépendant.
3. Raccorder q2/q3/q4 + RLE `SpherePlateau` à cette ABI.
4. Implémenter ensuite `F_K^render` et les multiplicités `S_tau` ; la connectivité et le rendu restent deux consommateurs distincts du même plateau.

## Conclusion

Les corrections `052fed4` et `5a08ab6` ferment bien les deux erreurs précédentes. La partition après chaque niveau est maintenant le bon invariant de base.

Le prochain verrou n'est plus géométrique : il faut empêcher la refactorisation WSPD de figer une sortie qui ne sait enregistrer que les fusions. Une hiérarchie HGP complète doit aussi dire quand une composante apparaît, comment elle croît et quelles facettes contribuent au rendu. Le macro-lot contient déjà toute cette information ; il reste seulement à ne pas la réduire à deux entiers.