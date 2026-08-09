# Audit de `order_k_bfs.hpp` — snapshot `a8111f0`

Date de l'audit : 2026-08-09 UTC.

> [!NOTE]
> **Suivi mathématique : le finding de connectivité est fermé sous arrangement simple.** [AUDIT_CONNECTIVITE_ORDER_K_A8111F0.md](AUDIT_CONNECTIVITE_ORDER_K_A8111F0.md) prouve que le vrai 1-squelette induit par les sommets de niveau au plus $k$ est connexe et qu'un seul germe de niveau zéro suffit sans excursion au-dessus de $k$. Le NO-GO du présent audit reste inchangé : RelevantGP ne garantit pas la simplicité globale utilisée par la représentation à quatre identifiants, le germe et les témoins constants sont encore faux sur la fixture coplanaire, les arités basses exigent un lemme d'attachement distinct, et le coût reste en $\Theta(nV)$.

## Verdict

**NO-GO comme générateur produit M3.** Le relevé et l'ordre exact d'un pinceau sont une piste mathématique sérieuse, mais le prototype courant ne calcule pas encore le catalogue MorseHGP3D. Un contre-exemple entier, bien centré et sans extra-shell critique fait publier un niveau 0 à la place du niveau 1. Ce défaut subsiste après la correction du signe `InSphere`.

Le parcours matérialise en outre le squelette global des sommets shallow de l'arrangement relevé, y compris les sommets non bien centrés qui ne seront jamais émis. Avec huit rescans du nuage par sommet, son coût générique est exactement `8 * (n - 4) * V` itérations de candidats. C'est la forme `sortie * m` que la voie v3 devait éviter, et non un chemin crédible vers 50 000 points.

| Sévérité | Finding | Statut au snapshot `a8111f0` |
| --- | --- | --- |
| P0 | les témoins coplanaires au triangle du pinceau sont ignorés alors que leur signe est constant et peut être intérieur | **ouvert, reproduit exactement** |
| P0 | les sommets d'arrangement d'arité quatre ne sont pas le catalogue Morse stratifié d'arités un à quatre | **ouvert** |
| P0 architecture | `seen` et `visited` matérialisent globalement un squelette shallow de mosaïque d'ordre supérieur, avec un rescan de `n` par voisin | **ouvert** |
| Clos conditionnel | connectivité du vrai 1-squelette sous arrangement simple | **prouvée dans [AUDIT_CONNECTIVITE_ORDER_K_A8111F0.md](AUDIT_CONNECTIVITE_ORDER_K_A8111F0.md) ; non applicable telle quelle à `RelevantGP` ou au code courant** |
| P1 | domaine plus fort que `RelevantGP`, métriques mortes ou trompeuses, clé 16 bits non gardée, sortie non canonique | **ouvert** |
| Clos dans ce delta | convention de signe `InSphere` inversée au premier snapshot | **corrigé par Claude dans `a8111f0`** |

## 1. Périmètre et reçus

- `HEAD` observé au début de l'audit : `389a7428c88d9dede7a9c767634774b9ea842ca0`.
- Premier contenu non suivi audité : SHA-256 `9fe9970a2fced13a72d061015de7e3ea390a12c3d2a36ab86c8f6a0a706bf2b1`.
- Contenu courant figé après correction du signe : SHA-256 `a8111f02f76e458912e2a2e1e1ff2d4ee0b71bba31af7993975f49fa6c792a3c`.
- Le delta entre ces deux contenus ne change que la convention de signe et son commentaire dans `in_sphere_side`.
- Probes compilés et énumérateurs créés uniquement sous `/tmp`; aucun fichier source v3 modifié par cet audit.
- GCP non utilisé.

Les comparaisons ont employé trois témoins distincts : le déterminant du nouveau fichier, `mhgp::sphere4` avec `mhgp::sphere_side` pour l'énumération exhaustive des quadruplets, et un calcul rationnel `Fraction`/Gauss indépendant pour la fixture P0 et ses barycentriques.

## 2. Correction acquise : le signe `InSphere`

Au hash initial `9fe9970a`, `in_sphere_side` inversait intérieur et extérieur. La fixture entière minimale est :

```text
a=(0,0,0), b=(2,0,0), c=(0,2,0), d=(0,0,2), e=(1,1,1)
```

On a `orient3d(a,b,c,d)=+8` et le déterminant relevé du code vaut `-24`. Le point `e` est le centre de la sphère du tétraèdre, donc strictement intérieur. Le premier snapshot rendait pourtant `+1` extérieur. Le hash `a8111f0` teste désormais le produit de signes négatif et rend correctement `-1`.

Reçu dynamique courant :

```text
orient=1 side(center)=-1
tetra+center cap4:
  {0,1,2,4}:L0 {0,1,3,4}:L0 {0,2,3,4}:L0
tetra+center cap5:
  {0,1,2,3}:L1 {0,1,2,4}:L0 {0,1,3,4}:L0
  {0,2,3,4}:L0 {1,2,3,4}:L1
```

Ce point est clos. Il doit néanmoins devenir un test unitaire direct, avec permutations d'orientation et témoins intérieur, shell et extérieur. Un accord entre parcours et force brute partageant ce prédicat ne pouvait pas détecter l'inversion.

## 3. P0 persistant : un témoin coplanaire constant fausse le niveau

### 3.1 Fixture u16 exacte

```text
0=(4,1,0)
1=(14,19,0)
2=(4,17,0)
3=(17,9,0)
4=(15,8,19)
rank_ceiling=4
```

Les points `0,1,2,3` sont coplanaires. L'énumération rationnelle indépendante ne trouve aucun extra-shell parmi tous les supports affinement indépendants et bien centrés d'arités un à quatre. La coplanarité du quadruplet de base ne définit pas un support critique et ne permet donc pas de censurer ce nuage par le raccourci « position générale de tout l'arrangement ».

Le support cible `U={0,1,2,4}` est strictement bien centré :

$c=\left(54/5,9,1347/190\right),\quad R^2=5794073/36100.$

Ses barycentriques, dans l'ordre `0,1,2,4`, sont toutes strictement positives :

$\lambda=\left(23379/72200,9731/36100,2419/72200,1347/3610\right).$

Le point `3` est strictement intérieur, avec puissance exacte $\left\lVert x_3-c\right\rVert^2-R^2=-359/5$. Le vrai niveau est donc 1, le rang fermé est 5, et ce support doit être absent au plafond 4.

### 3.2 Sortie fautive du hash courant

Le binaire compilé directement contre le snapshot `a8111f0` rend :

```text
coplanar_constant cap4 out_of_domain=0 count=2
  {0,1,2,4}:L0
  {0,1,3,4}:L0
```

L'énumération exhaustive exacte des tétraèdres donne au contraire :

```text
{0,1,2,4}:L1
{0,1,3,4}:L1
{0,2,3,4}:L0
{1,2,3,4}:L0
```

Le parcours publie donc les deux mauvais sommets au plafond 4 et manque les deux vrais sommets de niveau 0.

### 3.3 Cause

Pour un triangle fixe `(a,b,c)`, un point `z` tel que `orient3d(a,b,c,z)=0` ne produit pas un croisement fini du pinceau. Il n'est cependant pas « neutre » : hors cocircularité, il est **constamment intérieur** ou **constamment extérieur** à toutes les sphères du pinceau. Ce signe constant se décide exactement par l'incircle du triangle dans son plan.

Le code saute ces points :

- pendant la certification de la face support ;
- pendant la recherche du germe ;
- dans chacun des huit rescans de voisinage.

Le germe choisi ici utilise trois points de la face coplanaire et force son niveau à zéro sans jamais le calculer. Le quatrième point de la face est pourtant intérieur à sa sphère.

### 3.4 Déterminisme réfuté par permutation

Les 120 permutations des cinq mêmes points ont été rejouées en remappant les supports vers les identifiants géométriques initiaux :

- 60 permutations rendent les deux faux niveaux 0 `{0,1,2,4}` et `{0,1,3,4}` ;
- 60 permutations rendent les deux vrais niveaux 0 `{0,2,3,4}` et `{1,2,3,4}`.

La sortie n'est donc pas équivariante par permutation sur un nuage que `RelevantGP` ne permet pas de rejeter. La cause est le choix du premier `p2` parmi des égalités coplanaires, puis l'oubli de l'offset constant.

### 3.5 Correction conceptuelle minimale

Il faut distinguer trois cas sur chaque pinceau : croisement fini, témoin constant intérieur/extérieur, et cocircularité constante. Ajouter seulement un scan exact de la sphère racine rétablirait son niveau, mais ne suffirait pas : si le germe recalculé est au-dessus du plafond, le garde de `pop` arrête le parcours avant toute descente vers les vraies composantes shallow.

Un germe robuste doit donc être certifié de niveau 0. Deux voies cohérentes sont :

1. trianguler exactement et de façon canonique la face support en Delaunay 2D, puis choisir le premier événement hors du plan avec tous les offsets coplanaires comptés ;
2. construire directement un tétraèdre Delaunay 3D certifié, sans matérialiser toute la mosaïque.

Dans les deux cas, le sens de `t` doit être normalisé explicitement d'après le signe du côté support, et non seulement supposé par l'ordre de wrapping.

## 4. P0 de modèle : arrangement d'arité quatre et catalogue Morse ne coïncident pas

### 4.1 Les sommets non bien centrés ne sont pas des événements Morse

Tout quadruplet affinement indépendant définit un sommet de l'arrangement relevé. Il ne définit une miniboule critique de support quatre que si le circumcentre est dans l'intérieur du tétraèdre et si le shell complet respecte le domaine.

Avec seulement les quatre points `(0,0,0)`, `(2,0,0)`, `(0,2,0)`, `(0,0,2)`, le parcours rend `{0,1,2,3}:L0`. Son centre `(1,1,1)` a pourtant les barycentriques `(-1/2,1/2,1/2,1/2)` : ce support n'est pas bien centré et ne doit pas être émis comme sphère critique.

Il est légitime qu'un algorithme de navigation traverse des sommets non bien centrés. Il est faux d'identifier `vertices_visited` au catalogue final, et ces sommets internes invalident aussi la borne de coût par la seule taille de la sortie Morse.

### 4.2 Les arités un à trois sont absentes

`order_k_vertices` retourne immédiatement un vecteur vide pour `n < 4`. Or un nuage d'un point possède déjà sa sphère critique singleton de rang 1 ; les milieux de paires et les circumcentres de triangles acutangles sont les événements d'arités deux et trois.

Dans le relevé, ces événements sont des minima du rayon sur des strates de dimension positive, pas des sommets génériques à quatre hyperplans. Ils exigent un harvest explicite et prouvé :

- arité un : rayon nul au point ;
- arité deux : minimum au milieu sur le plan médiateur ;
- arité trois : minimum du rayon sur le pinceau du triangle ;
- arité quatre : sommet bien centré de l'arrangement.

Les champs `emitted_arity`, `harvest_faces` et `harvest_edges` existent mais aucun site ne les incrémente. Il n'y a ni harvest, ni filtre `well_centred`, ni reconstruction du shell complet dans ce fichier.

### 4.3 Domaine trop fort et mal signalé

Une égalité `compare_t==0` censure immédiatement tout le parcours comme cosphéricité. Or `RelevantGP` n'est pas l'absence de toute cosphéricité : le dépôt possède déjà une fixture où une égalité portée seulement par des supports non bien centrés ne doit pas censurer le nuage.

Inversement, les points coplanaires sont sautés silencieusement sans incrémenter `degenerate_shells`. Le prototype mélange donc :

- arrangement simple global, hypothèse plus forte que le domaine public ;
- dégénérescence d'un vrai événement critique ;
- égalité non critique autorisée ;
- point constant sur un pinceau, qui n'est pas en soi une dégénérescence.

Le domaine doit rester celui défini par référence, avec décision locale sur les événements effectivement critiques.

## 5. Complétude du parcours : connectivité désormais prouvée conditionnellement

Après correction du signe, une campagne différentielle a comparé le parcours à l'énumération de tous les `C(n,4)` tétraèdres sur des nuages strictement simples : aucune coplanarité de quadruplet, aucune cosphéricité de quintuplet, `n` entre 5 et 9 et plafond aléatoire entre 4 et `n`.

```text
OK checked=19864 rejected_as_nongeneric=136
```

Cette campagne soutient l'ordre du pinceau et le transport dans ce domaine **beaucoup plus fort**, mais ne constituait pas une preuve universelle. Le suivi [AUDIT_CONNECTIVITE_ORDER_K_A8111F0.md](AUDIT_CONNECTIVITE_ORDER_K_A8111F0.md) ferme désormais le point mathématique : le vrai 1-squelette induit par les sommets de niveau au plus `k` est connexe, et sous arrangement simple ses arêtes sont les voisinages consécutifs partageant trois hyperplans.

La coupe avant `enqueue` est donc sûre **si** le germe, les niveaux, les constantes de pinceau et les voisinages sont exacts dans ce domaine simple. Ces préconditions ne sont pas satisfaites par la fixture coplanaire, et `RelevantGP` n'implique pas la simplicité globale. La preuve ferme la connectivité abstraite ; elle ne ferme pas l'application de ce théorème au code courant.

Le commentaire « le niveau du voisin vaut le niveau courant plus ou moins un » est aussi trop fort : la variation 0 est possible lorsque l'apex et le nouveau point compensent leurs changements d'état. La formule du code autorise déjà `-1`, `0` et `+1`; le commentaire doit refléter cet invariant exact.

## 6. P0 architecture et SLO : le facteur `n` n'a pas disparu

Pour chaque sommet visité, le code choisit quatre triangles, explore deux directions par triangle, puis rescane les `n` points afin de retrouver le voisin immédiat. En position générique, cela représente `8 * (n - 4)` passages de candidats par sommet, auxquels s'ajoutent plusieurs déterminants exacts.

À `n=50 000` :

- un seul sommet coûte 399 968 passages de candidats ;
- `V=50 000` coûte déjà 19 998 400 000 passages ;
- `V=1 000 000` coûte 399 968 000 000 passages.

Le transport du niveau économise un neuvième scan éventuel, mais ne change pas l'ordre dominant de la recherche des voisins. Le coût réel est $\Theta(nV)$, et `V` compte les sommets shallow de navigation non bien centrés en plus des événements Morse publiés.

Par ailleurs :

- `seen` conserve globalement une clé pour chaque sommet atteint ;
- `visited` conserve à nouveau tous ces sommets ;
- `frontier` conserve la frontière DFS ;
- les huit incidences par sommet matérialisent implicitement le squelette shallow de la mosaïque de Delaunay d'ordre supérieur.

Cette structure globale supplémentaire est incompatible avec l'invariant d'architecture du dépôt comme chemin produit par défaut. Elle peut rester un oracle borné ou un prototype de falsification, mais ne remplace pas A1-source sans un index de voisinage sublinéaire, une borne mémoire et des mesures 50 k.

Une voie crédible doit au minimum remplacer le rescan par un constructeur output-sensitive démontré : listes de conflits/cuttings shallow, structure cinétique par pinceau, ou reverse search avec fonction parent canonique et oracle de voisinage sous-linéaire. Construire préalablement tous les ordres de chaque triple déplacerait seulement le coût vers `C(n,3)` et n'est pas une solution.

## 7. P1 d'implémentation et de reçu

### 7.1 Clé tronquée à 16 bits

`key4` masque chaque identifiant par `0xFFFF` sans garde. Par exemple :

```text
key4(0,1,2,3) == key4(0,1,2,65539)
```

La cible 50 k tient dans 16 bits, donc cette collision ne frappe pas ce profil précis. L'API accepte néanmoins un vecteur arbitraire sans vérifier `n <= 65536`; au-delà, `seen` fusionne des sommets distincts. Il faut soit une précondition vérifiée et publiée, soit une clé composée de quatre `i32` sans troncature.

### 7.2 Compteurs non probants

| Compteur | Comportement courant |
| --- | --- |
| `seed_scans` | incrémenté une fois par boucle complète ; vaut typiquement 5, pas le nombre de points ou de prédicats examinés |
| `level_recomputed` | incrémenté à 1 alors qu'aucun niveau n'est recompté ; zéro est seulement supposé |
| `pencil_candidates` | omet les points coplanaires et les indices exclus, alors que leur orientation a bien été testée |
| `vertices_beyond` | pour un plafond normal supérieur ou égal à 4, un voisin trop profond est rejeté avant `enqueue`; le compteur reste donc nul |
| `degenerate_shells` | aucun site d'incrément |
| `emitted_arity[5]` | aucun site d'incrément |
| `harvest_faces`, `harvest_edges` | aucun site d'incrément |

Le `static_assert` sur la taille de la structure garantit seulement que les champs sont additionnés par `absorb`; il ne garantit ni qu'une branche les alimente, ni que leur sémantique mesure le travail réel.

### 7.3 Déterminisme et ordre public

Pour une entrée strictement simple et moins de 65 536 points, les scans et la pile sont déterministes à identifiants fixes; l'`unordered_set` n'est pas itéré. En revanche, la fixture P0 prouve l'absence d'équivariant par permutation dans le domaine visé. Le vecteur `visited` n'est enfin soumis à aucun tri canonique avant retour. Un éventuel consommateur doit séparer ordre de parcours et ordre public de catalogue.

### 7.4 Largeurs entières

Sous le profil effectivement vérifié `quantized_u16_input`, la borne annoncée d'environ $2^{91.2}$ pour le déterminant relevé est conservatrice et tient largement dans `i128`. Les soustractions sont toutefois effectuées dans le type `i64` de `P3` avant conversion vers `i128`; un appel hors grille u16 n'est pas protégé par cette preuve. L'entrée de cette API doit donc vérifier le profil au lieu de seulement le commenter.

## 8. Portes exigées avant de reparler de M3 produit

1. Ajouter la fixture coplanaire bien centrée ci-dessus comme non-régression permanente, avec niveau, rang, support et test des 120 permutations.
2. Ajouter des tests directs indépendants de `in_sphere_side`, y compris inversion de l'orientation du support.
3. Compter les témoins constants sur chaque pinceau et construire un germe Delaunay certifié sur une face coplanaire.
4. Séparer explicitement `arrangement_vertex`, `critical_sphere_candidate`, `well_centred`, `closed_rank` et `public_event`.
5. Implémenter et juger les quatre arités; `n=1`, `n=2` et `n=3` doivent déjà produire leurs catalogues exacts.
6. Appliquer la preuve de connectivité en certifiant son domaine simple, ou traiter canoniquement les multiplicités permises par `RelevantGP`.
7. Remplacer le rescan `n` par voisin et publier une borne de temps et de mémoire mesurée au profil 50 k.
8. Garder cette voie au statut d'oracle/prototype tant que les points 1 à 7 ne sont pas fermés; aucun benchmark ni accord moyen ne peut promouvoir `public_status=exact`.

## Conclusion

La bonne idée à conserver est l'encodage exact de l'ordre d'un pinceau par `orient3d` et `InSphere`; la campagne strictement simple et la preuve polyédrique montrent qu'elle mérite d'être poursuivie. Ce n'est cependant pas encore une voie complète vers MorseHGP3D v3 : le domaine réel comporte des strates coplanaires autorisées, les arités basses manquent, le bon centrage n'est pas séparé de la navigation, l'application du théorème au code reste ouverte et le facteur `n` reste payé huit fois par sommet.

La décision d'audit est donc : **conserver `order_k_bfs.hpp` comme oracle expérimental borné, ne pas l'intégrer comme remplacement produit d'A1-source au snapshot `a8111f0`.**
