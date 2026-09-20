# Contrelecture des noyaux q4 — tranches 28/29

20 septembre 2026, source examinée :
`31b0243a7593a7eeb32903ce3270c4ef0535c24e`. Aucun défaut identifié dans
les contrats relus ci-dessous. Lecture seule du produit et des reçus,
sans nouveau test moteur, build ni requalification. Les empreintes exactes
et contrôles de fermeture figurent dans [PRODUCT_PINS.json](PRODUCT_PINS.json).

## Partition locale 28

Les quatre fichiers `src/lanes/q4_local{,_partition}.{cpp,hpp}` ont été
contre-lus. Leurs octets correspondent aux sources épinglées des captures
28 **et** 29 : ce noyau est resté stable entre les deux captures.

- Le fragment classe intérieur seulement pour `maximum < 0`, extérieur
  seulement pour `minimum > 0`. Les contacts `min = 0`, les formes nulles
  et toute population non examinée restent actifs. Compte exact non saturé,
  frontière de nœuds disjoints et curseur `escape` conservent la couverture.
  L'enfant ne reclasse que la frontière héritée ; aucun crédit déjà retiré
  de cette frontière ne rentre une seconde fois dans le compte.
- Une feuille terminale achève une fois les blocs encore indécis par
  `refine(..., UINT64_MAX)`, avant partage entre seeds. Le budget Z512 borne
  les fragments intermédiaires, pas cette finition ni le travail total.
- Le clipping conserve les racines aux bords ; une racine extérieure
  devient une contribution constante évaluée sur l'intersection fermée.
  Les sorties sont retirées avant lecture du compte strict, les entrées
  ajoutées après. La propriété droite/haut affecte seulement l'émission,
  jamais les mises à jour du compte. Positivité et propriété de l'arête
  précèdent la promotion du compte/coquille couverts au nuage global.
- Les capacités simultanées parent/enfants et ancienne/nouvelle frontière
  sont comptées. Le pic final est le maximum entre construction et atlas
  conservé plus buffers. Objets fixes, contrôles `shared_ptr`, transitoires
  allocateur, consommateurs, nuage/index/cover et RSS restent exclus,
  conformément au contrat. Les factories ne publient aucun objet partiel
  et ne modifient pas leurs parents immuables.

## Sélection duale 29

Synthèse de la contrelecture indépendante du noyau 29 transmise dans la
coordination de cet audit : les signes de `c` sont séparés avant
normalisation vers un dénominateur positif ; frontières colinéaires et
tous les IDs de coordonnées duales coïncidentes sont conservés.
Le déterminant d'orientation est calculé en i128, avec la majoration
`5760 M^6 < 2^109` sous u16. Le census réduit repart de zéro et reste
non saturé ; aucun crédit des sites retirés n'est ajouté.

Sous le seuil `K−2`, le certificat donne compte exact et coquille complète
du cover ; au-dessus, il certifie seulement le rejet. Une seed aiguë a
`c_x = 2 (|x−a|² + |x−b|² − |a−b|²) > 0`. Cela ne permet pas de supprimer
les **témoins** de signe `c ≤ 0`, toujours nécessaires au certificat et
aux coquilles. Aucun assemblage 28+29 n'est qualifié par cette relecture.

## Reçus et portée terrain

Les fermetures de [scale 28](../../receipts/q4_local_20260920/scale_ipjudaz2/COMPLETION.json)
et [scale 29](../../receipts/q4_shallow_20260920/scale__b3aquyb/COMPLETION.json)
ont été contrôlées directement : 39 et 40 reçus, dont 32 mesures chacun ;
hashes de manifestes/reçus, statuts PASS et sources avant/après concordants
(177 et 184 sources). Ce contrôle de fichiers ne rejoue pas leurs juges.

Les deux campagnes ne comportent **aucune mesure LiDAR du port produit** :
leurs régimes sont `far`, `cap`, `adversarial`, `dense_prefix`,
`dense_permuted`. Les anciennes mesures LiDAR du prototype A ne se
transfèrent pas à ces moteurs. Des mesures appariées 28/29 sur les mêmes
arêtes réelles doivent payer préparation, sélection, balayages et coquilles.

29 exécute réellement sa référence 28. Les grands juges contrôlent les
boules publiées, sans constituer seuls une preuve de complétude ; celle-ci
repose aussi sur les petits oracles et les certificats. Les suites 89/90
CTests annoncées sont Release ; les gates sanitizer ne constituent ni
une exécution sanitizer de toute la suite ni une qualification TSan.
Pas de borne globale sous-quadratique, de générateur de toutes les arêtes,
de tour FULL, de qualification GPU/G4 ou de garantie de mémoire totale.
