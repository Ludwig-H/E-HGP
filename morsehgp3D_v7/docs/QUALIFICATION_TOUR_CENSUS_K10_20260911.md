# Qualification du vrai raccord census → FULL jusqu'à K10

11 septembre 2026. `phase=exploration_v7_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

Le [nouveau reçu](../receipts/full_t2_census_tower_20260911/README.md) confronte
la génération WSPD, le prefilter et le census réels à un inventaire rationnel,
puis leur tour FULL aux composantes Gamma indépendantes. Le constructeur
ne reçoit jamais le catalogue de l'oracle à la place de son census.
Cette porte ferme un manque de test K9/K10 sur les fixtures nommées ; elle
ne prouve pas la complétude de la génération sur tous les nuages.

## Un oracle plus économique, toujours borné et indépendant

L'ancien modèle n≤8 reste inchangé. Le nouveau juge réemploie seulement ses
rationnels Boost et sa résolution de Gram, pas les formes, puissances,
prédicats MEB ou parcours du produit. Pour chaque support positif Q de
cardinal au plus quatre, il calcule sa boule B et la population fermée C
du nuage. Toute facette F telle que Q⊆F⊆C a exactement B pour MEB : Q impose
le rayon minimal et B contient F. Toutes les attributions multiples doivent
coïncider, et tous les sous-ensembles non vides doivent recevoir une boule.
L'existence d'un tel support est la propriété usuelle de la MEB euclidienne
en dimension trois, déjà utilisée par l'oracle historique.

Gamma reste explicite dans ce juge : ses voisins sont énumérés par ajout
d'un point puis retrait d'un point, avec une coface présente de cardinal
K+1. C'est la même adjacence, pas un graphe induit sur les seuls minima.
Le catalogue rationnel énumère séparément les supports positifs et conserve
les boules de fenêtre `p+qmin≤min(Kmax+1,n)`, avec populations complètes.
Les grandes populations sont filtrées avant de remplir les tableaux bornés.

Ces deux économies de juge concordent avec le modèle historique sur les
14 géométries existantes : 1 022 MEB exactes et 14 724 requêtes de composantes,
aux côtés ouverts et fermés, sur le nuage entier et chaque suppression d'un
site. Le nouveau modèle refuse n>14 : c'est une borne de test exponentiel,
pas un plafond algorithmique du chantier. Aucun Gamma exhaustif n'est ajouté
au chemin produit.

## Résultats et contrôles

Chaque géométrie est testée avec deux jeux d'identifiants et ordres d'entrée,
s=8/10/12, puis cache temporel, statique mono-thread et statique quatre threads.
L'amont utilise un thread. Cela donne 54 tours complètes K1..10 par build,
540 ordres et 18 vrais census. L'inventaire compare clés primitives, niveaux
rationnels, arités minimales et ensembles intérieurs/coquilles.

| Géométrie | Boules retenues par census | Coupes jugées | Vérifications verticales |
| --- | ---: | ---: | ---: |
| Ligne de 12 points | 65 | 720 | 352 752 |
| Coquille de 12 points, centre et extérieur | 207 | 2 080 | 4 658 780 |
| 12 points spatiaux, supports q3 et q4 exercés | 252 | 10 200 | 3 092 416 |

Pour les six tours de référence, toutes les coupes aux niveaux MEB des
sous-ensembles sont contrôlées ouvertes et fermées, plus les extrêmes.
L'identification des composantes utilise leur histoire et leurs parents,
pas seulement leur recouvrement de points. Pour chaque facette présente,
toutes ses sous-facettes doivent retrouver la même image verticale à la
même coupe. K9/K10 contribuent 101 570 occurrences de facettes et 942 784
vérifications verticales ; la non-vacuité est explicite.

Les 48 autres tours sont comparées physiquement aux références : niveaux,
nœuds, parents, successeurs, populations, contributions et ancres verticales,
pas seulement un digest. O2 et ASan/UBSan/LSan ROOT ont les mêmes résultats :
12 315 605 contrôles dans les trois campagnes de raccord, 13 000 coupes et
8 103 948 vérifications verticales. Les compteurs du différentiel historique
et des rejets restent séparés.

Neuf requêtes ou profils invalides sont refusés. Quatre fautes sont réfutées
avec leur diagnostic causal : omission d'une attribution MEB, confusion
ouvert/fermé, suppression des adjacences et omission d'une boule du vrai census.
Le premier essai du comparateur de populations échouait seulement sur l'ordre
des listes ; sa capture est conservée et aucune défaillance géométrique du
produit n'en est déduite.

## Limites de cette preuve

Les sources sont celles de c03f6be8, dont le header FULL `33e7d05e…`.
Une modification du résolveur ou un futur raccord GPU demande sa propre
capture ; les signatures présentes ne lui seront pas transférées.
La contre-lecture de publication a identifié deux gardes héritées non reprises
dans le nouveau juge : égalité du champ `forest.order()` à K et taille exacte
de `lower_nodes`. Les coupes/parents/images sont bien jugés, mais une faute
commune de ces seules métadonnées pourrait échapper aux paires physiques.
Ce n'est pas un défaut nominal démontré. Un supplément séparé prépare leurs
contrôles et mutants, sans réécrire les captures fermées du présent reçu.
Les prémisses d'entrée u16 et de positions distinctes restent déclarées.
Le cas terminal K=n des petites fixtures, les poids du manuscrit et l'archive
industrielle gardent leurs contrats séparés. Aucun résultat F n'est promu FULL.

Ce sont des tests de conformité, pas les tests de croissance 8k/16k/32k ni
un benchmark 50k. Les contrats de toute la tour sous 1 s puis 100 ms et le
régime de plusieurs dizaines de millions de points restent non acquis.
GCP non utilisé pour ce reçu.
