# Naissances triées par cohortes de niveau

Le catalogue immuable est déjà ordonné par niveau exact. Pour k>1, les
naissances en sont une sous-suite ; seul l'ordre des centres à l'intérieur
d'un même niveau reste à établir. La nouvelle préparation conserve cette
sous-suite puis trie chaque cohorte non singleton par le comparateur exact
des centres. Aucune sphère n'est préparée pour une cohorte singleton.

À k=1, les centres sont les sites eux-mêmes. Un tri lexicographique des trois
coordonnées entières remplace leur présentation comme sphères ponctuelles.
Le tableau `BirthEntry` déjà réservé sert de tampon pour ce tri ; les nœuds
reçoivent leurs clés avant que ce tableau soit rempli à nouveau pour le lookup.
Ce lookup demeure trié par clé, indépendamment de la numérotation des nœuds.

## Exactitude et travail

L'ordre canonique reste `(niveau, centre lexicographique)`. À niveau fixé,
deux naissances distinctes ont des centres distincts : sinon il s'agirait de
la même boule du catalogue. Les coordonnées de sites sont également uniques
dans le nuage géométrique. Aucun départage arbitraire nouveau n'est nécessaire.

Une non-naissance entre deux naissances du même niveau dans le catalogue
**ne coupe pas leur cohorte**. Le calcul du tampon et le tri portent tous
deux sur la sous-suite des naissances. Le contrôle d'ordre rejette une
décroissance de rang ; les niveaux eux-mêmes ne sont pas recalculés.

`birth_presentations` compte désormais les sphères réellement préparées
pour ce tri : zéro à k=1, puis la somme des tailles des cohortes de taille
au moins deux à k>1. `center_comparisons` compte toujours les appels réels à
`num::compare_centers`, et vaut donc zéro à k=1. Les comparaisons entières
du tri des points et du lookup ne sont pas ajoutées à ce compteur. Les
anciens reçus gardent leur source et leur ancien sens des compteurs.

Avec B boules du catalogue, b naissances et bλ naissances au niveau λ, le
parcours et les tris géométriques coûtent O(B + somme bλ log bλ). Le tri
final du lookup reste O(b log b). À k=1, le tri XYZ et celui du lookup restent
O(n log n). Cette tranche ne supprime pas tous les tris sériels et ne fournit
aucun gain temporel avant mesure native.

## Réservations et admission de la phase

Les sorties conservent leurs capacités : `2b−1` nœuds, `2b−2` enfants, b
entrées de lookup. Le tampon de sphères a maintenant la taille
`S=max({bλ : bλ>=2}, défaut 0)`, puis est libéré avant les tableaux du DSU.
À k=1, S=0. Il n'y a ni doublement ni allocation par cohorte.

Les portes privées de `ForestBuilder::births` testent la réservation exacte
dans les ABI des profils qualifiés : nœud 24 octets, enfant 4, lookup 8, et
enregistrement temporaire 144/160/176 octets en u18/u21/u24. Ces dernières
tailles correspondent à une Sphere suivie de deux mots u32 et à l'alignement
16 ; elles ne sont pas une promesse d'ABI publique. Pour C octets antérieurs
et B octets du tableau de classification déjà vivant, la phase exige :

`C + B + 24(2b−1) + 4(2b−2) + 8b + S * taille_enregistrement`.

La porte conserve un ancien Buffer de 17 octets, vérifie le succès à ce seuil
et le refus `memory_budget` à seuil−1, avant allocation des sorties. Elle
vérifie aussi le pic exact, les réservations restantes après le retour du
tampon et la conservation des anciennes données. Il s'agit de la phase
classify→births isolée, pas d'une formule de pic de toute la tour FULL.

## Faits indépendants et portes

Le modèle charge seulement la définition Gamma_k et la géométrie rationnelle
Gram/Fraction des tests ; il ne reprend ni le tri ni le scan des cohortes du
produit. Les faits suivants sont vérifiés avec permutations et homothéties :

| Fixture | Ordre | Naissances canoniques par BallIdx | Niveaux | Sphères / tampon |
|---|---:|---|---|---:|
| Triangle `(0,4),(4,0),(4,4)` | 2 | 1,0 | 4,4 | 2 / 2 |
| Ligne 0,2,5,9 | 2 | 0,1,2 | 1,9/4,4 | 0 / 0 |
| Ligne 0,4,6,8,12 | 2 | 0,1,2,4 | 1,1,4,4 | 4 / 2 |
| Ligne 0,4,6,8,12,16 | 2 | 0,1,2,4,5 | 1,1,4,4,4 | 5 / 3 |
| Deux losanges décrits ci-dessous | 3 | 9,8,12 | 4,4,34 | 2 / 2 |

Les coordonnées non précisées ont z=0. Les losanges sont
`{(0,12),(2,10),(4,12),(2,14)}` et `{(10,2),(12,0),(14,2),(12,4)}`.
Les trois naissances ont p=0, m=4 et qmin=2. Leurs centres sont
`(2,12)`, `(12,2)`, `(7,7)` ; Gamma_3 possède une fusion à trois enfants au
niveau 50. Toutes les triples de chaque coquille gardent le rayon de leur
boule : ce sont des naissances étendues, pas des cellules régulières.

Dans la ligne 0,4,6,8,12, les boules 2,3,4 ont toutes le niveau 4, mais la
boule 3 est une cellule à traces strictes avec un point intérieur. Elle
sépare deux naissances dans le catalogue sans séparer leur cohorte.
Le témoin à six points conserve cette non-naissance3, mais ajoute une
naissance5 au même niveau4. Sa cohorte maximale contient trois naissances :
la couper à la clé3 sous-estimerait le tampon à deux enregistrements. Le
premier témoin seul ne détectait pas nécessairement cette erreur de capacité,
car sa cohorte initiale contiguë avait déjà la taille2. Le modèle corrompt
uniquement ce tampon (3→2), en conservant et revérifiant clés, niveaux,
présentations et tailles de la forêt. La porte mémoire native exige le
succès au seuil contenant les trois records et le refus à seuil−1.
Le triangle et les losanges imposent un ordre des centres opposé à celui
des clés. Une fixture K1 distingue séparément XYZ lexicographique et Morton.

Les groupes C++ sont `points`, `cohorts`, `extended`, `memory` et
`parallel_memo`. Ce dernier compare les structures complètes et les coûts
des naissances entre le chemin sériel et W1/W4 avec mémo désactivé/activé ;
il inclut les verticales parallèles. Les attendus sont 849 contrôles pour
les cinq groupes (144/272/152/224/57), calculés statiquement et non mesurés.

Le modèle passe en Python normal et `-O` : 72 cas, 612 contrôles, 156 faits
auxiliaires et 300 corruptions rejetées. Le premier essai a révélé une confusion
de types int/Fraction dans ses attendus auxiliaires ; les valeurs étaient
correctes, leurs types exacts ont été corrigés avant ces passages.

Mutations causales prévues : remplacer l'ordre XYZ par les clés à k=1 ;
remplacer le comparateur de centres d'une cohorte par les clés ; sous-estimer
le tampon d'un enregistrement ; couper la cohorte à une non-naissance.
Dans les deux derniers cas,
un refus d'invariant est un échec causal du contrat de succès, pas une sortie
géométrique fausse. Le CMake et le manifeste partagés sont raccordés par le
pilote du lot. Aucun build ni test natif local n'a été exécuté.

La campagne G4 `census1`, sourcec6954f231, passe ses portes fonctionnelles
mais conserve une mutation invalide : `forest_cohort_nonbirth_reset`
déclenche `-Werror=maybe-uninitialized` après `rank.reset()`. Ce refus de
construction inattendu n'est pas une mort causale par la porte. Le correctif
retire seulement ce reset de l'optional ; `count=0` coupe toujours la cohorte
à la non-naissance et sous-estime la capacité3 en2. Le même défaut logique
doit encore être tué nativement. Produit, assertions et planchers inchangés.
