# Clé primitive de boule : réemploi exact pour device

11 septembre 2026, snapshot `ce842a3f1c0d55786250b1deba85e7bd92c8807a`.
`phase=exploration_v7_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`. Prototype privé ; aucun fichier actif modifié.

## Réemploi, pas un second moteur entier

`q2_ball_key`, `q3_form`, `q3_ball_form`, `q4_form` et `q4_ball_form` sont déjà
`MHGP7_HD`. Trois annotations privées suffisent à rendre accessibles
`ugcd64`, `ugcd128` et `ball_key_reduce`. Leur algorithme nominal n'est pas
réécrit ; les autres lignes ajoutées dans ces deux headers sont des mutations
de test exclues par défaut. Aucun nouveau diviseur par limbes n'est nécessaire.

Le témoin historique `k_native` de `src/gpu/device_witness.cu` borne son
numérateur à environ 78 bits et son diviseur à i64. Il ne prouve donc pas
la division générale demandée ici. Le présent kernel reçoit au contraire
les deux mots de chaque entier : aucun dénominateur n'est implicitement
réduit à 64 bits. La compilation/lien NVCC 12.9 SM120 du premier kernel a
réussi, puis la gate autonome exerce le même code selon son backend déclaré.
Sans exécution CUDA fermée, cela reste une qualification hôte et de compilation.

## Domaine et preuve arithmétique

`ugcd128` couvre tout u128, zéros compris. Si un opérande est nul, il rend
l'autre. Tant que y a un mot haut non nul, l'identité de l'algorithme
d'Euclide préserve le PGCD et le reste est strictement inférieur à y.
Quand y tient dans u64, le dernier reste x modulo y tient aussi dans u64 :
le passage au PGCD64 ne tronque donc aucune information. Deux zéros donnent
zéro, ce qui est permis pour ce PGCD mais pas comme diviseur d'une clé.

Le réducteur vérifié exige a>0, comme la fonction CPU originale. Pour
$g=\mathrm{gcd}(a,|b_0|,|b_1|,|b_2|,|c|)$, on a $1\leq g\leq a\leq2^{127}-1$.
Le cast de g vers i128 est donc sûr. Toutes les divisions par g sont exactes,
la nouvelle composante a reste positive et le PGCD final vaut un. Cela
caractérise l'unique forme primitive pour une classe de multiples positifs.
Une valeur a négative n'est pas renversée implicitement : elle est refusée.
Le calcul `uabs128` accepte MIN128 sans appliquer la négation signée interdite.
Les coefficients b/c peuvent valoir MIN128 ; diviser par un g positif reste
défini. Le cas MIN128/-1 ne se produit pas dans cette normalisation.

La primitive séparée `divide128` couvre les deux i128 arbitraires : quotient
tronqué vers zéro et reste de même signe que le numérateur, sauf zéro.
Elle refuse le diviseur nul et MIN128/-1 **avant** `/` ou `%`. Les sorties
de refus sont entièrement nulles avec statut distinct. Ce n'est pas une
division euclidienne ni un plancher ; le futur minimiseur d'axe devra choisir
explicitement sa direction d'arrondi. Aucun plafond de quotient, nombre de
points ou longueur de descente n'est ajouté.

## Domaine géométrique

Le wrapper de support contrôle cardinal 2..4, coordonnées u16 et positions
distinctes avant les formes. q3 colinéaire et q4 coplanaire donnent un refus
par a=0. q4 oriente déjà son déterminant positivement. q2 a=1 n'appelle pas
le PGCD. Les largeurs u16 documentées des lanes assurent l'absence de
débordement pendant la formation, jusqu'à environ 104 bits pour c de q3.

Cette primitive normalise une **circumboule**, pas nécessairement une MEB.
Un triangle obtus ou un tétraèdre mal centré peut donner une clé correcte.
La positivité du support et la sélection lexicographique restent l'autorité
du sélecteur MEB séparé. Le test commun q2/q3/q4 sur une même sphère utilise
volontairement des supports non strictement positifs en arité supérieure.
Pour les formes i128 arbitraires, le normaliseur est purement algébrique :
il ne certifie ni existence d'une sphère réelle ni sûreté d'une future
évaluation de puissance hors du profil géométrique u16.

## Juge indépendant et transport

Le juge local emploie `boost::multiprecision::cpp_int` pour PGCD/divisions et
un système de Gram rationnel résolu par élimination pour les circumboules.
Il ne réemploie ni le déterminant ni les formes q2/q3/q4 pour calculer les
attentes géométriques. Les cas couvrent pleine largeur signée/non signée,
MIN128, dénominateurs à mot haut non nul, coefficient c essentiel au PGCD,
rejets, permutations, bords u16 et égalité des clés entre lanes.

`gate --export` écrit les attentes depuis ces entiers/rationnels indépendants,
après passage des mêmes contrôles. Le fichier figé contient 13 573 cas :
4 103 formes, 2 171 PGCD, 4 217 divisions et 3 082 supports ; 22 sorties sont
des refus, dont deux dégénérescences géométriques. La gate portable n'utilise
pas Boost : elle compare d'abord la voie hôte à ce fichier, puis son backend
déclaré aux mêmes attentes, soit 325 752 comparaisons de mots.

`WireInput` contient 14 u64 (112 octets), `WireOutput` 12 u64 (96 octets),
alignement 8, tableau au décalage zéro. Aucun objet i128 natif ne traverse
le transport ; chaque scalaire est encodé bas/haut en complément à deux.
Tailles/alignements/décalages sont vérifiés à la compilation ; un kernel
distinct en restitue aussi les valeurs et SM120, avec l'endianness, à l'hôte.
Les mots réservés des sorties sont zéro et sont comparés eux aussi.

Le fichier texte est authentifié par SHA-256 avant parsing. Les allocations
sont bornées par la taille physique de ce fichier authentifié, jamais par
un plafond algorithmique de nuage. Les vecteurs device restent sentinellés
avant le kernel ; toute écriture manquante est détectée. Les trois allocations
sont libérées et leurs retours contrôlés **avant** publication de PASS.
En échec, le destructeur tente le nettoyage et signale un état inconnu ;
ce chemin ne peut pas publier PASS.

## Coût et limites

La normalisation demande au plus quatre replis PGCD et cinq divisions exactes,
avec sorties anticipées si le PGCD atteint un ; q2 ne paie aucun PGCD.
Euclide effectue un nombre logarithmique de restes en fonction de l'amplitude
des entiers. La largeur est fixe à 128 bits, pas liée à n. Le coût matériel
des divisions 128 bits est un développement logiciel du compilateur CUDA,
pas une instruction matérielle 128 bits supposée gratuite. Les traces PTXAS
qualifient les registres et spills de ces kernels précis, pas le débit ni
l'occupation du futur sélecteur/résolveur fusionné.

La normalisation ne fournit pas encore recherche de catalogue, admission K,
minimiseurs entiers d'axe, ordre des intrus, descente complète ni remise au
calendrier. Elle ne réduit aucune occurrence WSPD et ne qualifie aucune
hiérarchie entière sur GPU, contrat 50k/1s, 100ms ou régime massif.
