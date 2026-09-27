# Consommateur q34 sur entrées réelles : mesure CPU complète de S2

27 septembre 2026, base `70168cc3b`, audit isolé, grille 1 mm,
`not_claimed`. Aucun moteur, arène ou consommateur gelé modifié ; aucun
GCP. Cette tranche mesure **S2**, pas la tour FULL ni la segmentation.

Lire [le protocole exact](DESIGN.md). Le candidat et le batch CPU natif
consomment le même front, avec le même index et les mêmes rectangles dans
le même ordre. La comparaison finale porte sur chaque endpoint, masque,
position et compte de rejet ; elle est hors chrono candidat.

## Qualification fraîche close

[Qualification](../../receipts/q34_waves_real_20260927/qualification/summary.json) :
21 commandes PASS, deux nouveaux builds Release et Clang ASan/UBSan/LSan.
La gate de 175 lots du consommateur est réellement réexécutée dans ce nouveau
binaire : 1 050 consommations, 355 632 appels ponctuels, 195 948 survivantes
comparées, trois mutations natives par build. Ce n'est pas une qualification
héritée parce que les sources sont réutilisées.

Trois petits appels du **nouveau harnais de mesure** passent dans chacun
des deux builds, avec compteurs et hashes discrets identiques :

| recette 64, K5/s8 | P | E | S | F |
| --- | ---: | ---: | ---: | ---: |
| uniforme | 1 305 | 1 305 | 1 265 | 0 |
| terrain | 730 | 730 | 704 | 0 |
| amas | 2 015 | 1 249 | 1 205 | 448 |

Les deux premiers testent le fallback ; le dernier prépare 28 plans et
requiert un retour d'ordre effectif. Lectures normales/`-O` identiques et
huit corruptions du lecteur rejetées, dans
[les contrôles](../../receipts/q34_waves_real_20260927/qualification/checks/checks.json).

## Mesures réelles closes

Première campagne demandée : trame 08/000000 sans sol entière après masque
figé, 39 885 sites, puis uniforme 8k/16k/32k. K5/s8, Q4096, **un worker dans
les trois étages** (préparation, curseur, référence). Ordre unique candidat
puis natif, une observation : pas une estimation statistique de gain stable.
Les six coupes capteur ont été mesurées dans une capture distincte.

Les captures [initiale](../../receipts/q34_waves_real_20260927/initial/summary.json)
et [coupes](../../receipts/q34_waves_real_20260927/cuts/summary.json) sont
closes PASS, avec lectures normales/`-O` et huit corruptions rejetées pour
chacune. Toutes les sorties S sont exactement celles du natif. Aucun
chrono réel n'est déduit des petits cas de qualification.

| entrée | n | P | E | S | candidat S2 (s) | natif S2 (s) |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| ng00 entière | 39 885 | 23 686 751 | 9 122 704 | 2 043 612 | 31,048 | 47,691 |
| uniforme8k | 8 000 | 435 717 | 435 709 | 394 343 | 4,555 | 4,245 |
| uniforme16k | 16 000 | 908 066 | 908 050 | 820 584 | 10,158 | 9,407 |
| uniforme32k | 32 000 | 1 876 847 | 1 876 820 | 1 691 166 | 21,530 | 20,041 |

La trame LiDAR progresse de 34,9 % dans cette observation. L'uniforme
**régresse** de 7,31 % / 7,98 % / 7,43 % : le Pool y retire très peu de paires,
et la gestion collective/tri ajoute du travail. Aucun « gain stable »
n'est qualifié avec cet ordre unique.

Sur ng00 entière : préparation 14,133 s, consommation 16,833 s,
tri/conversion 66,155 ms, destruction interne 14,551 ms et observation
diagnostique 1,798 ms. Le front partagé prend 3,813 s ; le vrai intervalle
entrée→S2 vaut 34,949 s. Le total expérimental 82,649 s contient aussi le
natif, le juge et le nettoyage final : il ne faut pas l'attribuer au
candidat. Les visites ponctuelles passent de 1 110 657 775 à 537 798 656.

Le tri n'est donc pas ici le poste dominant. La préparation, qui comprend
le filtre rectangle natif, puis le filtrage ponctuel restent les deux
gros postes à porter réellement en parallèle. Cette observation ne dit
pas que leur futur port GPU tiendra 100 ms.

## Croissance mesurée

À chaque doublement uniforme 8k→16k→32k, les ratios sont : E ×2,084/×2,067,
S ×2,081/×2,061, visites ponctuelles ×2,202/×2,163,
visites du front ×2,337/×2,188, F ×2,634/×1,923 et temps candidat
×2,230/×2,119. Ces postes restent sous le quadruplement sur les tailles
mesurées, sans preuve asymptotique ni transfert à q3/q4/FULL aval.

Les coupes LiDAR gardent le masque entier figé puis utilisent les plans
x=0 et y=0 passant par l'origine du capteur, dans la représentation grille
1 mm. Les sept listes et leurs IDs sont vérifiés comme partitions complètes
et disjointes. Le manifeste historique publie aussi trois changements de
quadrant brut→grille (un selon x, deux selon y) lors de la préparation
initiale ; ne pas prétendre que les frontières float32 brutes et entières
sont identiques. Aucun masque n'est recalculé par morceau.

| morceau ng00 | n | E | S | candidat S2 (s) | natif S2 (s) |
| --- | ---: | ---: | ---: | ---: | ---: |
| x<0 | 24 591 | 5 613 272 | 1 186 494 | 20,498 | 31,446 |
| x≥0 | 15 294 | 2 308 228 | 647 746 | 6,697 | 9,546 |
| x<0, y<0 | 11 536 | 1 640 274 | 470 103 | 6,076 | 7,879 |
| x<0, y≥0 | 13 055 | 1 772 679 | 595 128 | 9,511 | 11,423 |
| x≥0, y<0 | 8 225 | 1 129 994 | 331 218 | 3,341 | 4,514 |
| x≥0, y≥0 | 7 069 | 769 455 | 292 064 | 2,958 | 3,092 |

Pour comparer des tailles inégales, définir l'exposant local descriptif
β=log(travail_parent/travail_enfant)/log(n_parent/n_enfant). Ce n'est pas
une borne de complexité : les morceaux sont spatialement hétérogènes.

| relation parent / enfant | β E | β visites ponctuelles | β temps candidat | β tests de coins Pool |
| --- | ---: | ---: | ---: | ---: |
| entière / x<0 | 1,004 | 1,132 | 0,859 | 0,846 |
| entière / x≥0 | 1,434 | 1,516 | 1,600 | 1,588 |
| x<0 / x<0,y<0 | 1,625 | 1,766 | 1,606 | 1,406 |
| x<0 / x<0,y≥0 | 1,820 | 1,640 | 1,213 | **2,230** |
| x≥0 / x≥0,y<0 | 1,152 | 0,886 | 1,121 | 1,235 |
| x≥0 / x≥0,y≥0 | 1,423 | 1,428 | 1,059 | 1,281 |

E, S, visites ponctuelles, front, rectangles, F et temps restent sous
β=2 dans ces six relations. **Pas tous les compteurs** : les tests de coins
du demi x<0 augmentent à 29 981 359 contre 7 305 823 pour son quart y≥0,
β=2,230, alors que F donne β=1,908. Le nombre borné de tests par site de
facteur ne force pas tous les ratios finis à suivre celui de F.
Verdict limité : croissance sous-quadratique observée des postes dominants
sur ces six relations d'une seule trame, pas une preuve générale ni une
qualification de plusieurs scènes ou d'une tour complète.

## La mémoire restante est surtout celle du front

Sur ng00, `Prepared` retient 187,227 Mo, dont 29,328 Mo d'arène ; le vecteur
front appelant réserve 100,663 Mo, le pic des tableaux du curseur vaut
95,244 Mo et index/nuage partagé 11,579 Mo. Le high-water RSS du processus
vaut 371,208 Mo. Il ne faut donc pas présenter les 29 Mo de l'arène comme la
mémoire de toute cette étape.

Sur uniforme 32k, `Prepared` retient 350,184 Mo alors que son arène ne vaut
qu'environ 0,136 Mo ; le high-water processus est 737,096 Mo. Le front contient
7 761 552 rectangles, dont 6 112 641 seront fermés, et le prototype garde
encore des métadonnées pour tous. Le bornage Q supprime les grands tableaux
temporaires P/E, **pas** la matérialisation/copie du front. Ce poste R est
un obstacle concret pour le raccord industriel ; aucune nouvelle variante
de représentation n'est implémentée dans cette tranche.

Décision technique pour le raccord suivant : séparer le filtre des
rectangles, déjà disponible sur GPU dans le moteur, du constructeur de
l'arène. Celui-ci devrait recevoir seulement les rectangles survivants,
en conservant leur `raw_base` et leur ordinal d'origine calculés avant
compaction. Le prototype actuel conserve aussi les rectangles fermés et
exécute à nouveau leur filtre en CPU ; il ne doit donc pas être activé
tel quel dans le moteur. Les identités P/E/S et l'ordre exact avant S3
restent des portes obligatoires pour cette séparation.

Les résultats distinguent préparation, consommation jusqu'à EOF,
tri/conversion, destruction effective des objets temporaires, surcoût
diagnostique, intervalle réel entrée→S2 et natif. Le total du harnais inclut
l'oracle et le nettoyage final : ce n'est pas le temps du candidat.

Les capacités de `Prepared` incluent l'arène : ne pas additionner les deux.
Le pic des tableaux du curseur inclut sa sortie native et les allocations
ancienne/nouvelle pendant réallocation. RSS et `ru_maxrss` sont ceux du
processus entier, pas des pics isolés par étape. Le natif s'exécute pendant
que la sortie candidate est encore conservée pour être comparée.

La segmentation et la préparation de grille sont déjà figées dans l'entrée
u32LE ; leur coût n'est pas ajouté implicitement à cette sonde. Une baisse
de E ne vaut ni croissance sous-quadratique de toute la chaîne, ni contrat
G4/100 ms. Les coupes sont comparées avec leurs effectifs réellement lus,
jamais en supposant automatiquement n/2 ou n/4.
