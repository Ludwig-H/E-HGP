# Contre-audit du juge rationnel centre-cell `90c06b0`

Date : 12 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Snapshot : `HEAD=90c06b0c436950d29f7617dd6a6765ddf3a8b7fa`.

## Verdict

Le nouveau juge est une **autorité arithmétique indépendante bornée crédible**
pour les supports q2--q4 sur le nuage qu'il reçoit. Il n'emploie ni les lifts,
ni les déterminants, ni la forme de puissance du sujet : il résout le centre
par élimination de Gauss en rationnels multiprécision, lit la positivité dans
les barycentriques et compare directement les distances rationnelles. Son
énumération exhaustive et son census global sont mathématiquement complets sous
coordonnées distinctes et dans sa borne `n<=45`.

Le claim et la gate sont néanmoins sur-vendus :

- sujet et juge partagent `prototype/cloud_families.hpp` et `mhgp::P3`; le juge
  est indépendant de la **géométrie du sujet**, pas de la fabrication de
  l'entrée ni du protocole;
- les quatre accords CTest sont des différentiels `n=32,smax=7`, pas un reçu
  du profil complet `smax=11`;
- la porte censée montrer que le juge voit le mutant ne l'exerce pas.

Le dernier point est P0 pour la réception du juge.

## 1. Vérification mathématique

Pour `U={u_0,...,u_(q-1)}`, le juge écrit
`c=u_0+sum_i lambda_i(u_i-u_0)` et résout exactement
`2(u_i-u_0).(c-u_0)=||u_i-u_0||^2`. La matrice est deux fois la matrice de Gram
des différences. Elle est inversible exactement lorsque `U` est affinement
indépendant.

Les coefficients barycentriques du centre sont
`(1-sum_i lambda_i,lambda_1,...,lambda_(q-1))`. Leur stricte positivité équivaut
à `c in relint conv(U)`, donc à un support propre positif. Le scan rationnel de
tout `X` construit alors exactement l'intérieur strict et le shell global. Le
rejet anticipé lorsque `p+q>smax` est sûr parce que ce support ne doit pas être
publié; aucun shell partiel n'est alors comparé.

Les produits scalaires tiennent dans `long long` sous le profil u16. La CLI du
juge n'impose pas elle-même `coord<=65 535`, contrairement au sujet. Les portes
actuelles gardent les coordonnées par défaut dans le profil, mais toute
extension exige une garde commune explicite et une nouvelle borne avant les
multiplications natives.

## 2. Porte mutant vacueuse

La CMake lance le driver avec `--inject=rank-closed`, mais sans `--judge`. Le
sujet calcule et imprime bien ses identités mutées, puis refuse explicitement
cette combinaison en code 2. Le driver jette alors le stdout au lieu de le
soumettre au juge et transforme le retour du sujet en son propre code 2. CMake
marque pourtant le test `WILL_FAIL TRUE`, ce qui accepte ce refus comme succès.

Rejeu exact au `HEAD` :

```text
REFUS : le sujet rend 2
REFUS : un mutant sans juge ne prouve rien
rc=2
```

La porte `mhgp3v_centre_cell_independant_voit_le_mutant` ne prouve donc pas que
le juge voit une divergence; elle prouve seulement qu'une commande interdite
finit par échouer. Le juge lui-même est sensible : en lui donnant directement
le stdout mutant malgré le code sujet, il rend code 1 avec `510` vérités,
`504` identités sujet et `6` manquantes. C'est le câblage de la porte, pas
l'arithmétique du juge, qui est réfuté.

Réparation contractuelle :

1. autoriser le mutant seulement lorsque `--emit-identities` est actif, ou
   injecter une corruption ciblée du flux d'identités après un calcul sain;
2. exiger que le sujet termine et ferme `IDENTITES`;
3. exiger le code **1** du juge, jamais un non-zéro quelconque;
4. exiger la regex `DESACCORD du juge independant`;
5. ajouter un témoin négatif où sujet absent/refusé rend code 2 et où la porte
   doit rester rouge.

## 3. Portée réelle du `48/48`

Le registre contient `508` tests. La sélection centre-cell plus payload rend
`48/48` en `115,08 s`; la sortie non archivée observée en `/tmp` a pour SHA-256
`23fc8d7a2445a4c4c05d2d3ddac8801048720e6dd08dd753aca8c969d765cb41`.

Les quatre accords indépendants emploient `n=32`, `smax=7`, `seed=5` et les
familles `uniform`, `terrain`, `scanline_single_pass` et
`scanline_overlap_multiecho`. Des rejeux ciblés donnent respectivement
`1 150`, `510`, `437` et `354` identités, avec `222`, `25`, `14` et `11` q4 :
les lanes hautes ne sont pas vacantes. La sortie CTest résumée n'archive ni les
listes d'identités ni leur digest. Le nombre `777` du message de commit est
reproductible sur le défaut `uniform,n=30,smax=6,seed=11`; ce n'est pas le
paramétrage des quatre portes.

Avant de qualifier l'oracle borné :

- publier les comptes et digests d'identités par famille et par arité;
- imposer des planchers q2, q3 et q4 séparés;
- exercer `smax=11` sur au moins une taille bornée;
- tuer séparément dépendance affine, positivité, owner fermé, rang fermé,
  intérieur strict et shell nul;
- graver une fixture rationnelle manuelle indépendante du générateur partagé;
- borner les subprocess par timeout et archiver commande, sources, ELF et
  sortie brute.

Le driver ne compare pas de `cloud_digest`; il suppose que deux processus
appelant `cloud_families.hpp` produisent le même nuage. Or
`std::uniform_int_distribution` n'engage pas les mêmes octets entre toutes les
implémentations de bibliothèque standard. Un reçu durable fixe donc la
toolchain et la bibliothèque, ou remplace le sampler par une spécification
entière stable, puis publie le digest des points et des identités. Le parseur
texte reste aussi à durcir : fin de champ obligatoire, clôture unique et rejet
des lignes surnuméraires.

## 4. Statut corrigé

Le statut acceptable aujourd'hui est :

`bounded_independent_rational_geometry_candidate_with_four_positive_differentials`.

Il n'est pas encore :

`mutation_sensitive_received_oracle`.

Cette correction ne retire rien aux quatre accords observés. Elle empêche
seulement qu'un `WILL_FAIL` satisfait par un refus soit présenté comme une
preuve de sensibilité scientifique.

## 5. Successeur `e6f1ef3` — lecture `k=1`

Le `HEAD=e6f1ef39e76a6bacf6861e84244d7a447ca92559` ajoute une lecture q2 à
zéro intérieur, un Kruskal et un comparateur Prim exhaustif. Les cinq CTests
`mhgp3v_centre_cell_k1_*` passent; un rejeu local les ferme `5/5` en `16,31 s`.
Le théorème employé est juste, mais sa portée documentaire doit être réduite.

Pour des positions distinctes, toute arête d'un EMST a sa boule diamétrale
**fermée** sans troisième site : un troisième point de cette boule serait
strictement plus proche de chaque extrémité que leur distance, et l'une de ces
deux arêtes remplacerait l'arête MST sur sa coupe. Le live collecte plus
largement toute paire à intérieur **ouvert** vide, sans exiger que son shell se
réduise aux deux extrémités. La fixture minimale est
`u=(0,1,0),v=(2,1,0),w=(1,2,0)` : `uv` a zéro intérieur mais `w` sur sa
coquille; `uv` n'appartient à aucun MST. Ce sur-graphe contient néanmoins tout
EMST, donc Kruskal reste exact pour les poids.

Le multiensemble trié des poids d'une MST est invariant : à une valeur
`lambda`, sa multiplicité vaut la différence du nombre de composantes avant et
après le lot. Le juge Prim valide donc utilement les `n-1` poids. Les lignes
`K1 d2` portent toutefois `d2=4 beta`; les appeler « niveaux » sans unité partage
un facteur quatre implicite entre sujet et juge. Le contrat doit publier le
rationnel `beta=d2/4` ou nommer explicitement le champ `four_beta`.

Cette gate ne valide pas une hiérarchie H0 :

- elle trie les scalaires reçus et ne vérifie ni leur ordre ni la contiguïté
  des égalités;
- elle ne reçoit ni endpoints, ni racines pré-lot, ni partitions strictes et
  fermées, ni multifusions canoniques;
- une arête Gabriel non nécessaire à la MST peut disparaître sans modifier le
  verdict, donc la complétude du catalogue Gabriel n'est pas testée;
- aucun mutant k1 ne corrompt poids, arête MST ou sérialisation du facteur
  quatre.

Sur une grille de 27 points, le diagnostic annonce un run de 26 poids égaux,
pas la multifusion à 27 enfants exigée par le contrat. La correction
conceptuelle conserve `(d2,a,b)`, gèle les racines avant chaque run, construit
les composantes du graphe quotient avec un DSU temporaire, publie la
multifusion, puis seulement mute le DSU global.

Enfin `--k1` n'est pas une voie 50 000 sparse. Il exécute d'abord toute la
source cellules q2/q3/q4, ses lifts, owners et census, puis filtre q2. Sur
`terrain,n=400`, les variantes avec et sans `--k1` ont les mêmes `30 265`
cellules, `1 768 790` lifts et `52 665` census; seulement ensuite `832` arêtes
donnent 399 poids MST. Yao-1 demeure donc la voie produit indépendante à au
plus `48n`; le live est un diagnostic de réutilisation lorsque le catalogue
complet a déjà été payé.

La porte mutante historique reste vacueuse au successeur : le rejeu CTest
affiche encore `REFUS : le sujet rend 2`, puis obtient un vert par `WILL_FAIL`.
Le commit ne la répare pas.

GCP non utilisé.
