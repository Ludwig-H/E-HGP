# Contre-audit B — rejet pré-atlas : conserver q3 et compter l'amont

22 septembre 2026. Audit de l'ordre effectif des opérations dans la v8
u18, puis de ses risques pour le premier port v9 `d2700314`.
**Aucun test G4 ni gain de temps v9 n'est qualifié ici.** La [note A q4](Q4_STRUCTURE_ET_BORNES.md)
établit le certificat local de blocs de graines `X×C` avant la partition
des témoins `Z`. Cette note précise ses dépendances de pipeline et ses
risques de coût, sans contester le certificat sous ses préconditions.

## La coupure exacte dans le pipeline actuel

Dans [`wspd_q34.cpp`](../../morsehgp3D_v8/src/pipeline/wspd_q34.cpp),
`edge()` construit le `Q34EdgeCover` avant de lancer q3 et q4. Quand les
deux voies sont actives, un atlas q4 est construit **avant** le parcours
des graines q3. Dans [`q4_local.cpp`](../../morsehgp3D_v8/src/lanes/q4_local.cpp),
`Q4LocalAtlas::Impl::build` construit d'abord la géométrie positive et
ses blocs de cover, puis chaque fragment `Z` ; ce fragment décide si la
cellule devient `Deep`, `Leaf` ou `Branch`. Ce n'est qu'ensuite que
`Q4SeedCellEngine` cherche les graines sur les feuilles vivantes.

Un filtre `X×C` placé juste avant `Q4LocalFragment::child` peut donc
économiser **des fragments enfants et leur descendance**, mais pas le
cover, la géométrie positive, sa décomposition du cover, ni le fragment
racine déjà payés. Les quatre enfants ne sont créés que si le fragment
parent les demande : la version paresseuse doit tester `X×C` avant le
fragment de chaque enfant, sans inventer une subdivision globale dont
le coût ne serait pas compté. Un préflight *avant cover* exige un objet
léger arête+index, distinct de la géométrie actuelle, et doit rejeter
les **deux voies activées** avant d'écarter le cover partagé.

Le [reçu 1 mm](../../morsehgp3D_v8/receipts/u18_resume_20260922/ground_1mm_first/only_probe_01_s00_k5_w8.json)
08/000000, K5/s8/W8 contient exactement :

| Poste | Compte | Portée |
|---|---:|---|
| covers | 2 043 612 | union des arêtes q3/q4 |
| visites de construction du cover | 440 194 038 | avant atlas |
| visites du domaine positif q4 | 502 904 094 | avant fragments |
| visites de décomposition du cover q4 | 315 736 960 | avant fragments |
| atlas q4 | 1 872 168 | dont 1 545 198 partagés avec q3 |
| fragments racines / enfants q4 | 1 772 302 / 24 368 956 | fabriques Z |
| visites Z de l'atlas | 11 432 872 749 | travail principal à éviter |

Ces comptes sont des opérations de natures différentes, pas des durées
à additionner. Le reçu est une seule exécution CPU q3/q4 en mode digest,
non un catalogue FULL ni une preuve de croissance.

## Piège de la mutualisation q3/q4

La voie q3 consulte l'atlas **avant** de construire sa boule. Sur ce
même reçu, `q3.seeds=184 461 509`,
`q3_atlas.rejections=153 036 427` et
`q3.ball_builds=31 425 082` :
`184 461 509 = 153 036 427 + 31 425 082` exactement. Supprimer un
fragment q4 qui portait un certificat `Z` utile à q3 peut donc rendre
à q3 des constructions et census coûteux, même si la sortie reste
exacte grâce à un repli. **Ce n'est pas une prédiction de 153 M
reconstructions** : c'est une borne extrême si tous les certificats
consultés disparaissaient. Le nombre publié de graines q4,
`9 550 974`, est **post-atlas** : le moteur saute déjà `1 133 440`
arêtes q4 sans feuille vivante avant leur recherche. Il ne mesure pas
la taille de l'ensemble `X` complet nécessaire **avant** atlas.
En revanche, `q3_atlas.locations=168 343 794` compte les graines q3
possibles sur les arêtes partagées q3+q4, avec les mêmes tests
`acute+owner` que q4. Un `X` pré-atlas fondé **sur ces seuls tests** et
développé en singletons aurait cette masse brute sur les arêtes
partagées : **17,6 fois** le nombre de graines q4 comptées après atlas.
Ce n'est pas une borne inférieure pour un autre certificat q4 exact ;
les blocs et leurs bornes doivent être mesurés pour éviter cette
matérialisation.

Un `X` qui couvre seulement les graines encore vivantes après atlas
serait circulaire. Pour prouver `NoQ4Seed(C)`, il doit couvrir toutes
les graines q4 **possibles avant Z**, notamment la complétion aiguë
propriétaire garantie pour tout tétraèdre strictement positif ; les
contacts avec une cellule fermée restent actifs. Pour partager ce rejet
avec q3, il faut en outre couvrir toutes les graines q3 possibles et
leurs centres dans cette cellule. Une absence de q4 dans son domaine
positif ne supprime pas q3 : la fixture entière
`(0,0,0),(4,0,0),(2,3,0)` a un support q3 aigu alors que la racine q4
est `Outside` ([note A](Q4_STRUCTURE_ET_BORNES.md)).
Si `X` comprend **toutes** les graines q3 propriétaires, une cellule
`NoSeed` ne peut contenir leur centre : chaque centre q3 est sur sa
droite `L_x=0`. L'intersection des rejets q3 actuels par atlas avec les
cellules ainsi sautées doit alors être **zéro** ; c'est un gate de
couverture, pas seulement un objectif de performance. Si `X` est
q4-seul, cette intersection peut être non nulle et son coût de repli
doit être mesuré.

Ne jamais réduire `X` à partir du **rejet ponctuel q3** d'une graine :
son centre peut être trop profond alors que sa droite porte encore une
boule q4 valable. Fixture entière à K=3 :
`a=(0,0,19)`, `b=(40,0,19)`, `x=(20,25,19)`, `y=(20,0,44)`,
`z₁=(20,0,0)`, `z₂=(21,0,0)` avec IDs dans cet ordre. `ab` est
l'arête maximale unique des supports. La boule q3 de `abx` a centre
`(20,9/2,19)`, rayon carré `1681/4` et les deux puissances strictement
négatives `−39,−38` : elle est rejetée à profondeur `K−1`. La boule
q4 de `abxy` a centre `(20,9/2,47/2)`, rayon carré `881/2`, poids
barycentriques `(8/25,8/25,9/50,9/50)>0`, et puissances `132,133`
pour `z₁,z₂` : elle survit. La graine `x` est aiguë et propriétaire ;
la présentation depuis `y` est supprimée par la règle canonique
`x<y` du balayage v8. Éliminer la **droite** de `x` à cause du rejet
q3 ponctuel ferait donc perdre l'émission q4 canonique.

**Fixture plus forte pour le catalogue v9 : aucune face q3 acceptée.**
Prendre `K=3`, `O=(20,20,20)` et, dans l'ordre des IDs, les quatre
sommets réguliers
`v₀=(30,30,30)`, `v₁=(30,10,10)`, `v₂=(10,30,10)`,
`v₃=(10,10,30)`. Pour chaque `i`, ajouter les deux témoins
`zᵢ,₁₁=O−11 sign(vᵢ−O)` et
`zᵢ,₁₂=O−12 sign(vᵢ−O)` (huit sites, IDs 4..11).
Les coordonnées sont entières positives u18. La boule q4 des `vᵢ`
a centre `O`, rayon carré `300`, poids `1/4` strictement positifs et
aucun témoin intérieur : les puissances des `zᵢ,₁₁/₁₂` sont `63/132`.
Elle est donc admise à K3 ; ses six arêtes ont longueur carrée `800`,
l'arête propriétaire est `(0,1)` selon le départage par IDs.

Pour **chaque** face q3 obtenue en omettant `vᵢ`, le centre est
`O−(10/3) sign(vᵢ−O)` et le rayon carré `800/3`. Les deux témoins
`zᵢ,₁₁/₁₂` ont les puissances `−271/3` et `−124/3` ; les six autres
témoins et `vᵢ` sont extérieurs. La profondeur q3 est donc exactement
`2=K−1` pour les quatre faces : toutes sont rejetées, alors que q4
survit (`0<K−2`). Toute génération q4 limitée aux triangles q3
**acceptés** perd cette BallKey. Le recoupement local du nouveau
catalogue v9 `d2700314` ne la retrouverait pas si elle était omise ;
seul un oracle de complétude du générateur la détecte. Cette fixture
teste en même temps les égalités de plus longue arête, sans supposer
qu'une présentation q3 rejetée transporte encore sa ligne q4.
L'[oracle Fraction autonome](check_q4_without_q3_faces_20260922.py)
vérifie les quatre profondeurs, tous les signes et les seuils ; il passe
en Python normal et `-O`. En diagnostic facultatif sur le premier
exécutable v9 `d2700314`, les quatre ordres d'IDs (identité, supports
inversés, témoins d'abord, tout inversé) donnent chacun une tour K1..3,
`q4_emitted=1` et 69 boules. Les digests diffèrent avec les IDs, comme
attendu ; ce contrôle ne compare pas les forêts après recodage. Le JSON
agrégé **n'identifie pas** à lui seul la BallKey q4 : ajouter la fixture
au juge d'inventaire exhaustif et une mutation supprimant **cette clé**.
Les variantes d'IDs du T2 actuel ne remplacent pas ce test : elles
permutent l'index de son oracle mais reconstruisent le nuage d'entrée
du **générateur** dans le même ordre géométrique `ix.upos` ; les ties
WSPD/propriétaire ne varient donc pas réellement entre ces variantes.

Les états et conséquences doivent rester **typés** :

| État de cellule | q4 | q3 / certificat Z |
|---|---|---|
| `OutsideQ4Domain` | aucun centre q4 | q3 autonome, aucun rejet par profondeur |
| `NoQ4Seed` | aucun support q4 dans `C`, si `X` complet | aucun compte Z nouveau ; q3 continue |
| `DeepZ` | rejet selon le seuil q4 `K−2` | rejet q3 seulement si le minorant atteint `K−1` |
| `ExactFragment` | balayage q4 possible | compte et frontière exacts disponibles |

`NoQ4Seed` n'est **ni** `Outside`, **ni** `Deep`, **ni** une feuille
exacte : il n'a pas de frontière `Z`. Les méthodes actuelles
`certified_inside_count` et `root_certified_inside_count` retournent le
champ `inside_count` pour tout état terminal autre que `Outside`.
Ajouter un état sans certificat à ce schéma, avec zéro ou un compte
hérité présenté comme exact, serait une confusion de contrat. Il faut
une réponse typée « pas de certificat Z » ou un minorant explicitement
étiqueté, et un repli q3. `prepare_live()` ne doit pas non plus traiter
ce terminal comme une `Leaf` possédant un fragment.

## Essai discriminant avant port actif

1. En *shadow* sur arêtes lourdes déterministes, appliquer aux cellules
   le filtre `X×C` **avant** leur fragment, mais construire quand même
   la v8 complète. Compter cellules qui auraient été évitées, tests
   `X×C`, gardes indécises et contacts, fragments/visites `Z` évitables.
2. Pour chaque cellule évitable, compter aussi les consultations q3 qui
   auraient perdu un rejet atlas, le nombre de boules/census q3 à refaire,
   et le coût de retrouver un certificat q3 séparé. Comparer `X` q4
   complet, union q3/q4 et certificats q3 indépendants ; **juger le
   temps de toute la chaîne**, pas seulement les visites q4 supprimées.
   La matrice croise masque de voies, `NoSeed` proposé, ancien état
   `Outside/Deep/Leaf` et `q3_atlas` consultation/rejet ; son intersection
   `NoSeed_conjoint ∩ q3_atlas_rejet` doit être nulle si `X` est complet.
3. Avant le cover, mesurer le nombre d'arêtes sans graine admissible des
   voies activées, le coût du préflight et le nombre de covers réellement
   évitables. Ne pas déduire ce nombre des `1 133 440` *whole-atlas skips*
   ni des `9 550 974` graines q4 post-atlas.
4. Gates exacts : q3 seul avec racine q4 `Outside`, q4 strictement positif
   avec **une seule** complétion aiguë (fixture A), centre/contact sur
   frontière de cellule, deux masques de voies, différentiels des
   supports/coquilles puis des BallKey et de la tour FULL. Un résultat
   `NoQ4Seed` ne doit jamais rejeter un support q3.

Ces tests doivent précéder le GPU. Même s'ils valident le rejet local,
la couverture du produit WSPD jusqu'au bloc de complétions et à la
cellule du centre reste une preuve distincte. Les mesures 8k/16k/32k
sur coupes emboîtées du même masque sans sol diagnostiquent la croissance ;
seules des trames entières multi-séquences, puis la tour FULL G4,
qualifient les contrats.
