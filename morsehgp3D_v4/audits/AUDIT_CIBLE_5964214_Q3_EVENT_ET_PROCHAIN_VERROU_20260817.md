# Audit ciblé après `5964214` — vrai événement q3 et prochain verrou collectif

Date : 17 août 2026.  
Pin audité : `5964214c43dd58618e5d3c389d889d574f3ba7f6` inclus.  
Commits de code nouveaux : `b6020c3`, `40b309c`, `5964214`.

Cette note ne reprend pas les audits précédents. Je ne conserve que les points
qui changent réellement la correction ou le prochain choix d'architecture.

## Verdict

Le cours pris est bon.

- Les paquets `cœur+h_a+h_b` sont mathématiquement légitimes : ils sont
  disjoints et strictement intérieurs à toute circum-boule q3 possédée par
  l'ancre.
- Le scan site-major est une bonne baseline CPU/GPU ; il ne change pas
  l'autorité ponctuelle.
- Le cover commun par rectangle est sûr. La boîte des sommes et `Dmax²`
  donnent un sur-cover, puis le filtre exact par ancre restaure l'objet exact.
- Les formules de `Q3BallKey` et de `Q3Level` sont correctes :

  ```text
  BallForm = (G, -(2Ga+W), G|a|²+W·a),
  r²       = D·E·X/(4G).
  ```

Le facteur cumulé annoncé sur `eight_clusters,n=2000` est donc un vrai progrès
algorithmique. Le verrou q3 n'est plus la descente de boule par porteur.

Je ne reçois toutefois pas encore le libellé **« événement transactionnel
complet »**. Trois raccords précis suffisent pour le rendre vrai.

---

## 1. Le principal verrou : matérialiser et juger le même événement complet

Au pin courant, `q3_event.hpp` définit une `BallKey` et un niveau, mais pas un
enregistrement d'événement. Le probe conserve encore :

- les supports dans `events` ;
- les BallKeys dans un vecteur séparé ;
- le niveau seulement le temps d'un test de positivité ;
- les `InteriorIds` dans des tampons locaux ensuite jetés.

De plus, `q3_ball_key` est appelée **après** le census, alors que le commentaire
annonce une clé formée avant celui-ci. La formule est indépendante du census,
mais l'ordre du pipeline ne l'est pas encore.

La prochaine structure devrait être explicite :

```cpp
struct Q3Candidate {
  SupportKey3 support;
  EdgeKey owner;
  Q3BallKey ball;
  Q3Level level;
  Q3Form form;
};

struct Q3Event {
  SupportKey3 support;
  EdgeKey owner;
  Q3BallKey ball;
  Q3Level level;
  u8 depth;
  SmallInteriorIds interior;  // triés par PointId
};
```

Ordre recommandé :

1. après acuité et owner, former `support/owner/ball/level` ;
2. effectuer le census ;
3. pour une boule survivante régulière, publier `depth` et la liste complète
   des intérieurs ;
4. comparer le **multiensemble d'enregistrements complets** entre
   `packet=off|on`, `cover=root|rectangle`, `census=tree|cover`.

L'invariant actuel

```text
nombre de BallKeys uniques == nombre de supports
```

est utile contre une collision ou une cosphéricité oubliée, mais il ne valide
ni la valeur de la BallKey, ni le niveau, ni les intérieurs. Une erreur de signe
qui produit une clé fausse mais unique passe encore avec enthousiasme.

### Extension directe de l'oracle indépendant

L'oracle de Cramer possède déjà `c=num/det`. Il peut produire une BallKey sans
réutiliser la formule de production :

```text
Rnum = |a·det-num|²,
A     = det²,
B     = -2 det·num,
C     = |num|²-Rnum.
```

Réduire `(A,B,C)` par pgcd et signe, puis comparer à `Q3BallKey`.
Le niveau oracle est simplement la fraction réduite

```text
Rnum/det².
```

Enfin, la liste triée des sites de puissance négative donne les
`InteriorIds`. Cette comparaison reçoit réellement l'événement complet ; un
simple compte de clés ne le peut pas.

---

## 2. Contrat de capacité : les tampons supposent `K_max<=10`

Les tableaux `core_ids[8]`, `ha_ids[8]`, `hb_ids[8]` et
`Carrier::interior[8]` sont corrects pour la cible actuelle :

```text
K_max=10, h_3=9, profondeur survivante <= 8.
```

Mais la CLI accepte un `smax` arbitraire. Pour `smax>11`, les collecteurs
tronquent silencieusement les certificats et le payload, tandis que le seuil
`h_3` continue de croître. Cela peut publier une fausse profondeur et de faux
événements.

Deux solutions honnêtes :

- profil actuel explicite : refuser `smax>11` dans ce chemin ;
- bibliothèque générale : dimensionner les petits vecteurs par `h_3-1`.

Dans les deux cas, ajouter les invariants forts sur chaque rectangle/ancre
survivante :

```text
core_ids.size == h_cœur,
packet.size   == h_cœur+h_a+h_b,
packet sans doublon,
event.interior.size == event.depth.
```

Ce ne sont pas des vérifications décoratives : le paquet est désormais une
partie de la profondeur, donc une divergence entre le compteur et le
collecteur modifie l'objet mathématique.

---

## 3. Avant q4 : recevoir l'identité de l'owner, pas seulement le support

Le sujet départage les arêtes avec `pid`, mais le juge brut choisit encore son
owner avec les rangs Morton. Comme il compare ensuite seulement le triplet non
orienté, une erreur de tie-break peut rester invisible.

Avant q4, qui dépendra fortement du seed/owner, la porte doit comparer

```text
(SupportKey, OwnerEdgeKey)
```

et non le seul support.

Il faut en même temps terminer l'API d'identités : `pid(u)` ne devient stable
que si l'entrée fournit réellement

```cpp
InputPoint { PointId id; P3 position; }
```

au lieu de fabriquer `PointId` depuis l'ordre du `vector<P3>`.
`SupportKey3` doit stocker directement trois `PointId` non signés, pas trois
`i32`.

Une seule fixture suffit à fermer ce contrat : le triangle à trois arêtes
égales

```text
(0,0,0), (1,1,0), (1,0,1)
```

avec plusieurs affectations d'IDs, notamment au-dessus du bit 31. La plus
petite `EdgeKey` doit gagner dans le sujet et dans l'oracle.

---

## 4. Seul prochain levier de performance à envisager : incidence cover par blocs

Le cover rectangulaire supprime les traversées hautes répétées, mais le code
énumère encore tous les points des handles pour chaque ancre. Si le compteur
`visites_filtre` domine désormais, la suite naturelle n'est pas un nouveau
rayon : c'est un dual-tree entre **blocs d'ancres** et **nœuds de sites**.

Pour une ancre, stocker

```text
s = a+b,
D² = |a-b|².
```

Un nœud d'ancres stocke une boîte `S` contenant les sommes, ainsi que
`Dmin²,Dmax²`. Pour un nœud spatial `Z`, poser :

```text
near² = dist(2Box(Z),S)²,
far²  = maxdist(2Box(Z),S)².
```

Alors :

```text
near² > 3Dmax²  => NONE : aucun site du nœud n'est dans aucun cover ;
far² <= 3Dmin²  => ALL  : tous les sites du nœud sont dans tous les covers ;
autrement       => MIXED, on scinde.
```

La preuve est immédiate à partir de

```text
|2z-s|² <= 3D².
```

Cette primitive généralise exactement le cover rectangulaire actuel : celui-ci
est le premier test `NONE` avec un seul bloc d'ancres. La version récursive
ajoute des blocs plus serrés et une autorité `ALL`.

- Si `visites_filtre` domine, construire ce BVH d'ancres et streamer les
  handles `ALL`, sans matérialiser toutes les incidences.
- Si `tests_puissance` domine déjà, passer directement au LBVH des
  circumcentres et aux bornes affines reçues précédemment.

Il suffit donc de séparer une fois les temps `filtre exact du cover` et
`q3_power`. Inutile d'implémenter simultanément les deux arbres, sport humain
classique consistant à optimiser deux postes avant d'avoir regardé lequel
coûte encore quelque chose.

---

## Ordre conseillé

1. `Q3Event` réellement matérialisé et comparé à l'oracle complet.
2. Contrat `K_max<=10` explicite ou capacités dynamiques.
3. Owner et IDs externes reçus.
4. Ensuite seulement, choisir avec les compteurs entre BVH d'incidence cover
   et LBVH des circumcentres.
5. Ouvrir q4 après ces trois contrats de correction, pas avant.

## Conclusion

Les optimisations récentes sont reçues et le verrou de coût a nettement
reculé. Le danger principal n'est plus géométrique : c'est de déclarer
l'événement terminé alors que ses champs vivent encore dans quatre vecteurs et
tampons sans comparaison commune.

Une fois l'enregistrement unique et l'oracle complet branchés, Claude pourra
continuer à optimiser agressivement sans risque de conserver les bons nombres
d'événements avec les mauvaises boules ou les mauvais intérieurs. C'est le
moment utile pour fermer ce contrat, avant que q4 n'ajoute assez de
Déterminants pour meubler un appartement.
