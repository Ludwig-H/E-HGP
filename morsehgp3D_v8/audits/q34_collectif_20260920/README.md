# q3/q4 : rejeter des familles avant leurs événements

20 septembre2026, auditeur indépendant A. Contrelecture de77f659e4 et
modèles mathématiques autonomes, écrits uniquement dans `audits/`.
`exploration_v8_hors_registre`, `cpu_reference`, `quantized_u16_input_only`,
`audit_independant_math_and_architecture`, `not_claimed`. GCP non utilisé.

**Le cover et les raccords23/24 sont cohérents dans leur contrat.** La
propriété et la positivité précèdent bien le transfert du compte couvert
vers une profondeur et une coquille globales ; les refus continuent le
groupe. Aucun défaut d’exactitude nouveau identifié. Le coût S·m demeure,
correctement publié : partager le stockage n’a pas partagé les comptes.

Le certificat proposé pour la tranche25 est sûr. Deux renforcements
sont disponibles : une corde plus courte et un certificat collectif du
petit pool. Leur efficacité supplémentaire apparaît sur l’adversaire
dense, **pas sur les neuf arêtes LiDAR examinées**. Aucun port produit,
gain de temps ou résultat global sous-quadratique n’est revendiqué ici.

## 1. Resserrement de la corde, sans numérateur hors i128

D désigne partout **ab²**, r le rayon au carré de la face aiguë propriétaire,
C=EX, G son Gram. Ainsi r=DC/(4G), D/4≤r≤D/3. Pour un tétraèdre positif,
noter t la somme des trois poids barycentriques de cette face.

La variance des sommets de la face, avec poids normalisés, est au plus r :
ils sont sur sa sphère et la soustraction du carré de leur barycentre
ne peut qu’abaisser cette variance. Leur contribution est donc au plus
t²r. Les trois arêtes allant au quatrième sommet mesurent au plus D au
carré et contribuent au plus t(1−t)D. La variance du support positif est R².
Par maximisation de ce trinôme concave :

$$R^2\leq t^2r+t(1-t)D\leq\frac{D^2}{4(D-r)}\leq\frac{3D}{8}.$$

Avec R²=r+μ²/(4G), S=2G−C>0 et T=4G−C>0 :

$$\mu^2\leq\frac{DS^2}{T}\leq\frac{D(3G-2C)}{2}=\frac{J}{2}.$$

La borne réelle est strictement meilleure hors face équilatérale. Pour
une face tendant vers un angle droit, le rapport des demi-cordes tend
vers √(2/3). L’arrondi entier peut effacer une partie de l’amélioration.

**Ne pas former D·S² en i128.** Calcul sûr proposé : q=ceil(D·S/T),
Unew=min(Uold,ceil_sqrt(q·S)), où Uold=ceil_sqrt(ceil(J/2)).
Puisque S<T et D est entier positif, q≤D ; les produits réellement
calculés D·S et q·S sont au plus54M⁶<2¹⁰², M=65535. Calculer le plafond
de division par quotient/reste et la racine avec un algorithme entier.
Le test final conserve les bornes déjà proposées pour P et B.

La [porte rationnelle indépendante](chord_bound_gate.py) vérifie les
identités de puissance contre Gram, les bornes sur255 tétraèdres positifs
propriétaires et487 présentations aiguës, dont des extrêmes u16. Ce test
est un complément au raisonnement, pas sa preuve générale par énumération.

## 2. Remplacer « mêmes témoins partout » par « assez de témoins partout »

Pour un **même pool d’IDs distincts**, chaque témoin est intérieur exactement
quand P(z)−μB(z)<0. Il fournit une demi-droite, ou un état constant si B=0.
Le minimum de leur population sur[−U,U] est un minorant de la profondeur
globale. S’il atteint K−2, toute la famille q4 positive peut être rejetée,
même sans K−2 témoins individuellement universels.

Le [petit balayage exact](collective_model.py) trie les racines du pool,
groupe les égalités, retire les sorties avant lecture puis ajoute les
entrées. Il contrôle la corde **fermée**, racines aux bornes comprises.
Les témoins dont les racines sont hors intervalle contribuent à l’état
initial. Aucun compte qui subit ensuite des retraits n’est saturé.
Les seuls endpoints ne suffisent pas : une sortie et une entrée égales
peuvent donner un creux strict au groupe alors que les deux côtés sont pleins.

Coût supplémentaire O(M log(1+M)) et O(M) mémoire pour M témoins, donc
O(K log K) si M=O(K). Les formes déjà calculées par l’essai universel
peuvent être réutilisées ; ne payer ce petit tri qu’après son échec.
Le comparateur réduit de la famille évite de croiser P1B2 en i128 ; le
modèle Python utilise des rationnels non bornés et ne qualifie pas ce port.
Les comparaisons d’une racine à ±U utilisent P±UB, dans la borne annoncée.

Fixture géométrique, sans famille vide : a=(8,10,10), b=(12,10,10),
x=(10,13,10), z+=(10,11,12), z−=(10,11,8), y=(10,11,7).
Le pool{z+,z−} a les formes(−96,+24),(−96,−24), et Uold=28, Unew=25.
Chaque témoin a un minimum individuel nul ; leur minimum collectif vaut1.
Le tétraèdre abxy est positif, propriétaire, avec poids
91/324,91/324,16/81,13/54. Sa racine−52/3 a profondeur globale1.
À K3, le rejet q4 collectif est donc concret, même sans témoin universel.

Le test q3 reste indépendant : population P<0 au seuil K−1. Si seul q4
est éliminé, le raccord doit prendre le chemin q3 saturant au seuil et
collecter toute coquille seulement en cas d’acceptation. Les crédits du
pool ne doivent pas être ajoutés au census qui repart de zéro.

## 3. Effet discret mesuré et contre-régimes conservés

[experiment.py](experiment.py) compare quatre certificats avec le même
pool de4K sites au plus, proposés par distance au milieu de l’arête,
départage par ID. Sélection **une fois par arête**, O(m log(1+K)) et
O(K) stockage via tas borné ; m−2 distances de proposition payées.
Ce n’est pas le pool de positions spatiales32/64 du constructeur : les
résultats ne se transfèrent pas automatiquement à sa tranche25.

Adversaire de la tranche24, K10. Nombre de familles où les **deux voies**
sont rejetées, avec S=n−2 et m=n :

| n | Universels, corde initiale | Universels, corde resserrée | Collectif, corde initiale | Collectif, corde resserrée |
| --- | ---: | ---: | ---: | ---: |
| 32 | 0 | 0 | 0 | 0 |
| 64 | 0 | 8 | 10 | 18 |
| 128 | 44 | 56 | 58 | 74 |
| 256 | 172 | 184 | 186 | 202 |

À256, le majorant des lectures initiales résiduelles passe de65 024
sans certificat à20 992 pour les universels initiaux et13 312 pour le
collectif resserré. **10 160 évaluations de formes du pool sont ajoutées**,
ainsi que sa préparation, ses tris et les traitements exacts résiduels.
Le modèle ne chronomètre pas ces postes, ne les assimile pas et ne juge
pas de nouvelles sorties produit. Il donne un résidu et un travail
supplémentaire à confronter, pas une accélération démontrée.
À32, les quatre variantes ajoutent du travail sans rejeter aucune familleq4.

Complément LiDAR : scans08/000000,000100,000200 **séparés**, sites8k
u16 déjà préparés et hashes contrôlés. Trois arêtes par scan relient
le siteID0 à ses voisins de rang8,32,128, déterminés exactement pour
choisir les fixtures. Ce sont neuf arêtes fournies, **pas un échantillon
du résidu WSPD ni une qualification du générateur**. Sur54 faces,
47 sont rejetées conjointement par chacun des quatre certificats :
aucun rejet supplémentaire des variantes renforcées. Leurs surcoûts ne
sont donc pas justifiés par cet échantillon. Aucun alignement ni aucune
correspondance entre points ou scans n’est utilisé par les certificats.

**Conseil au constructeur :** qualifier d’abord le petit pool universel
prévu ; considérer la corde resserrée comme option locale, puis le
balayage collectif seulement sur échec et sur bénéfice mesuré avec son
propre pool. Ne pas retarder ce port pour importer nos modèles d’oracle.
Le résidu S·m reste possible ; ces deux renforcements ne closent pas P0.

## 4. Objet commun pour partager davantage que le pool

Pour une arête fixée, poser v=b−a, D=|v|², w_z=2z−a−b et t=2c−a−b.
Le centre d’une boule passant par a,b vérifie t·v=0. Sa puissance vaut :

$$4\Pi_c(z)=|w_z|^2-D-2w_z\cdot t.$$

Toutes les familles partagent donc **les mêmes demi-plans dans le plan
des centres**. Une seedx restreint ce plan à sa droite Π(x)=0 ; les
événements sont les intersections avec les droites des autres sites.
Toute boule q4 positive propriétaire appartient au disque|t|²≤D/2.
Un contexte d’arête possédé peut ainsi partager les formes des témoins
entre des blocs de centres et de faces, avec compte/certificats distincts
des événements et des payloads. Cette représentation ne suppose aucune
orientation commune des points, contrairement au cas adverse à deux bandes.

Construire tout l’arrangement de droites pourrait encore coûter un carré.
Il faudrait traiter seulement les régions nécessaires, avec bornes sur
blocs et fronts restants, avant de revendiquer un gain. C’est une direction
architecturale démontrée par l’identité, pas un nouveau moteur mesuré.

## Preuves et limites de qualification

[capture/COMPLETION.json](capture/COMPLETION.json) ferme six commandes et
leurs sources : chaque modèle et l’expérience sont exécutés en normal et
`-O`, avec sorties identiques. Le modèle collectif compare2 975 cas à
l’évaluation directe aux racines, bornes et entre racines ;512 contrôles
de minorant, six mauvaises règles réfutées. La porte de corde comporte
108 faces,756 puissances et trois mutants d’arrondi. Seul le quotient
tronqué est réfuté par perte d’un vrai tétraèdre ; les deux racines arrondies
vers le bas violent la couverture théorique annoncée. Aucun mutant produit
compilé, CTest nouveau ou gain CPU/GPU n’est prétendu dans cet audit.

Le [préflight](PREFLIGHT.md) conserve la correction d’un attendu erroné,
avant les captures. Les scripts sont autonomes ; les expériences nécessitent
les trois fichiers LiDAR déjà présents. Tous les résultats restent des
preuves et observations d’audit à porter puis requalifier dans le produit.
