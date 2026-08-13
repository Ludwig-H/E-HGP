# Contre-audit du stage 0B : une fermeture d'hypergraphe n'est pas le fold HGP

Date : 13 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## 0. Pin et verdict

Le pin relu est `HEAD=3c11bc8f99dd5f43eeaa973d61157ac2ae58e74e`.
Les empreintes SHA-256 locales sont :

- `prototype/ball_event.hpp` :
  `eedd8521c31fa7963506b4fc1030eb6f92491d3d79f0e8356641c7035660b24a` ;
- `prototype/ball_event_probe.cpp` :
  `4a85f6cbdd74054160266ee2bbb1dd13a1d54fb7ffb9ce522855aff93be3e793` ;
- binaire Release :
  `674cebd3ec06e313e57791a98a09668f11eed51b225c0fcb79f49848cf20e97b`.

Verdict : **le commit `91aa287` reçoit au plus l'équivalence Kruskal--fermeture
minimax d'un même hypergraphe de points borné. Il n'implémente pas le stage 0B
défini dans `PROPOSITION.md`.** Il ne produit aucune des dix forêts HGP, aucun
lot atomique, aucune couverture, aucune application verticale et aucun
`BenchmarkOutputContract-v1`.

Le nom admissible de cette brique est
`PointHypergraphBottleneckClosureProbe-v0`. Les sorties `fold=OUI` et le message
« stage 0B » doivent rester non autoritaires jusqu'au raccord aux autorités
Gamma déjà présentes.

L'auditeur n'a modifié aucun logiciel et n'a pas utilisé GCP.

## 1. Rejeu

```text
cmake -S morsehgp3D_v3 -B build/v3 -DCMAKE_BUILD_TYPE=Release
cmake --build build/v3 --parallel --target mhgp3v_ball_event_probe
ctest --test-dir build/v3 --output-on-failure -R '^mhgp3v_ball_event_'
```

Les dix portes rendent `10/10` en environ `0,23 s`. Le nominal `grid` à seize
points imprime néanmoins :

```text
regulieres=300 refus_domaine=99
ball_event accord=OUI fold=OUI
```

Le nominal `clusters` passe avec `arite4=0`. Le mutant coquille peut rendre
`fusions=0`, `paires_connectees=0`, `desaccords=0` ; il meurt ensuite dans le
juge 0A, pas dans une propriété du fold. Aucun CTest n'injecte la perte d'un
événement, d'un membre, d'un niveau, d'une lane ou d'un lot.

Le générateur de nuage garde aussi un défaut amont permanent :

```text
timeout 2 ./build/v3/mhgp3v_ball_event_probe \
  --family=clusters --points=5 --coord=4
```

rend `124`, car la famille n'offre que quatre positions et la boucle de tirage
n'a ni preflight de capacité ni budget d'essais.

## 2. L'objet calculé n'est pas une forêt HGP

Le sujet trie les `SphereRun` réguliers par rayon, prend
`mem=I_B union U_B`, puis unionne tous les `PointId` de `mem` dans **une seule
DSU**. Le juge forme les mêmes cliques sur les mêmes membres et calcule leur
fermeture minimax Floyd--Warshall.

Cette construction peut représenter la connectivité de cet hypergraphe de
points. Elle n'est pas la connectivité HGP d'ordre `k`. À l'ordre `k`, les
objets connectés sont les générateurs ou facettes de taille `k`; deux
générateurs saturés se joignent lorsque leur intersection contient au moins
`k` identités, pas dès qu'ils partagent un point.

Contre-fixture permanente pour `k=2` :

```text
S = {0,1,2}
T = {0,3,4}
```

On a `|S intersection T|=1<2`. Les deux graphes de Johnson `J(S,2)` et
`J(T,2)` forment donc deux composantes HGP distinctes. La DSU de points les
fusionne par le seul `PointId 0` et rend une composante. Une map
`PointId -> composante` est précisément interdite par
`oracle/gamma_forest_judge.cpp`; `prototype/saturated_fold.hpp` porte déjà la
bonne jointure par générateurs.

Le probe ne distingue d'ailleurs aucun ordre `k`, aucune lane, aucun `q_min`
et aucun rôle naissance/coface. Une unique forêt de points ne peut pas devenir
les dix forêts par duplication.

## 3. Le juge est corrélé à toutes les entrées sémantiques

Kruskal et Floyd--Warshall sont deux algorithmes différents pour la même
fermeture. Ils partagent toutefois :

- les mêmes `runs` et le filtrage `Disposition::kRegular` ;
- les mêmes membres `I_B union U_B` ;
- le même `PrimitiveSphereKey` et `be_level_cmp` ;
- le même ordre et les mêmes rangs de niveaux ;
- l'absence commune de lane, naissance, facette, coverage et verticale.

Le juge peut donc recevoir la fermeture de l'hypergraphe **fourni**, mais il ne
juge ni sa complétude, ni ses identités, ni sa traduction en événements HGP.
Supprimer simultanément un run chez le sujet et le juge laisse zéro désaccord.
Filtrer les 99 runs `unsupported` du nominal grid laisse encore `fold=OUI`.

Un vrai juge 0B part de l'oracle Gamma ou d'un catalogue indépendant de
`BallEvent`, construit ses coupes par ordre et compare lots, racines, parents,
coverage et verticales, pas seulement un nombre de goulots entre points.

## 4. Le comparateur de niveau n'est ni total ni profile-opaque

La formule abstraite du rayon carré d'une forme
`A||z||^2+B dot z+C` est correcte. Son implémentation actuelle effectue avant
toute vérification :

```text
B_i * B_i
sum B_i^2
4*A*C
num_x * A_y * A_y
```

en `i128` signé. Un overflow signé C++ est un comportement indéfini ; tester
ensuite une division ne le répare pas. Une clé dont `B_i=2^80` suffit à faire
mordre UBSan sur le premier carré. Les fixtures u16 du contre-audit 0A exigent
déjà des intermédiaires au-delà de 128 bits.

Une erreur découverte depuis le comparateur de `std::stable_sort` est trop
tardive : retourner `false` et poser un drapeau externe ne garantit pas un
ordre strict faible si seules certaines comparaisons débordent. Il faut :

1. construire un `ExactLevelToken` normalisé avec arithmétique BigInt ou codec
   reçu ;
2. preflighter **tous** les tokens avant le tri ;
3. exposer un comparateur total, pur et sans échec ;
4. exercer égalité, transitivité, anti-symétrie, sérialisation et caps.

Le fold lit directement les champs `i128` de `PrimitiveSphereKey`. Aucun type
`SphereIdentity` ou `ExactLevelRef` opaque n'existe encore. Le claim « il ne
consomme pas la représentation arithmétique » est donc faux ; seule l'intention
d'une frontière future est présente.

## 5. Ce que le stage 0B doit produire

La chaîne minimale reste celle de `PROPOSITION.md` :

```text
BallEvent complets et toutes lanes scellées
  -> RegularDirectRecord / SaturatedGenerator
  -> spool et manifeste de source scellés
  -> tri/merge par ExactLevelToken
  -> macro-lots égaux, racines pré-lot gelées
  -> dix folds horizontaux par générateurs/facettes
  -> coverage_delta, gateways et continuations
  -> neuf applications verticales
  -> BenchmarkOutputContract-v1 atomique
```

Pour une boule régulière, conserver `Q=I_B union S`, le masque de support et
le niveau. Le même record a un rôle de naissance à `k=|Q|` et de coface à
`k=|Q|-1`, dans les limites du profil. Pour un plateau reçu, employer le
générateur saturé et la jointure `|S intersection T|>=k` ; sinon refuser la
transaction entière avant fold.

Les briques existantes à réutiliser comme autorités sont :

- `prototype/saturated_fold.hpp` pour la jointure par générateurs et lots ;
- `oracle/gamma_forest_judge.cpp` pour un juge dont l'état ne se réduit pas aux
  points ;
- `audits/check_gate_d_fold_f0.py` et les fixtures Gamma pour naissances,
  multifusions, coverage, provenance et lots égaux.

## 6. Portes bloquantes

- drop d'un événement et d'un membre ;
- niveau déplacé, égalité scindée entre deux lots et comparateur non transitif ;
- lane ou `q_min` perdus ;
- fixture `S/T` ci-dessus et intersection `k` contre `k-1` ;
- naissance, facette égale, `q=1 coverage_delta`, multifusion et continuation ;
- dix forêts et neuf verticales comparées à Gamma, lot par lot ;
- permutation, chunk coupé au milieu d'un niveau égal et reprise ;
- cap exact puis moins un avec zéro payload publié ;
- run `unsupported` présent : statut transactionnel non succès ;
- payload officiel complet, puis mutation de chaque champ.

## 7. Décision

Conserver le petit probe Kruskal/Floyd comme test d'une primitive de
connectivité d'hypergraphe. Ne pas l'appeler stage 0B, merge forest Morse ou
couture numérique reçue. Le raccorder aux générateurs par ordre et aux juges
Gamma avant de reprendre la source sparse.

Les étapes 0A et 0B restent ouvertes, et le contrat G4 n'est pas rapproché par
ce seul commit.

GCP non utilisé.
