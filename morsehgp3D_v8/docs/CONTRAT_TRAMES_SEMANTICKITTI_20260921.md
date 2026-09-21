# Contrat principal : la tour sur une trame LiDAR entière

Décision explicite de l'utilisateur du 21 septembre 2026. Elle remplace
la taille nominale de 50 000 points comme référence principale, sans
réinterpréter les anciennes mesures ni changer le contrat mathématique.

## Résultat attendu

Sur **GCP G4**, calculer toute la tour HGP **K=1..10** d'une trame
SemanticKITTI complète en moins d'une seconde. Si cet objectif n'est pas
atteint, qualifier séparément le repli **K=1..5**. Après le jalon d'une
seconde, viser **100 ms** sur le même périmètre. Un seul niveau K, une
voie q2/q3/q4 ou un flux de candidats ne constitue pas cette tour.

Le chronométrage produit doit couvrir la préparation nécessaire depuis
l'entrée déclarée, le calcul des événements, les parents et la construction
de toutes les hiérarchies demandées. Les transferts CPU/GPU nécessaires
ne disparaissent pas du bilan. Distinguer le temps de calcul en mémoire,
le temps incluant préparation/transferts et les lectures disque, ainsi que
le démarrage à froid et les répétitions à chaud ; ne pas choisir après
mesure la frontière qui ferait passer le contrat. Le décompte définitif
sera figé dans le lanceur FULL avant toute revendication de réussite.

Tester **plusieurs scènes** et publier chaque résultat, pas seulement une
moyenne ou le scan le plus favorable. Comparer K5/K10 et s8/s10/s12 à
entrées identiques. Les scènes et répétitions doivent être sélectionnées
avant les chronométrages de qualification. Les trames08/000000,
08/000100 et08/000200 sont trois acquisitions distinctes déjà disponibles,
mais ne représentent pas plusieurs séquences ni une validation de tout
SemanticKITTI ; élargir la diversité avant une qualification produit.

## Entrée entière et précision demandée

Ne pas réduire la trame à 50k, à un préfixe ou à un tirage. La décision de
précision ultérieure du21 septembre donne la préférence aux **coordonnées
float32 originales, conservées exactement**. Une grille isotrope est
autorisée en option, avec précision paramétrable et **pas de 1 mm par défaut**.
Voir le [contrat numérique et son état d'implémentation](PRECISION_FLOAT32_ET_GRILLE_20260921.md).
Ne pas mélanger les mesures de ces deux profils ni changer automatiquement
le pas pour faire rentrer une scène. Une grille de1mm donne au plus0,5mm
d'erreur par coordonnée ; cela ne dit rien de la précision physique du capteur.

Publier les retours bruts, sites géométriques distincts, doublons d'origine
et fusions dues à la grille, en conservant **chaque correspondance de retour**.
Le modèle reste celui des sites distincts, pas des multiplicités indépendantes.
Le moteur actuel reste `quantized_u16_input_only` ; les anciennes mesures
à2cm (119142/119942/120725 sites contre123389/124479/125526 retours) sont
historiques. La préparation sans perte et un prédicat exact isolé ne
transforment pas ce moteur en moteur float32. Aucun contrat de tour sur
le nouveau profil n'est encore acquis.

Le [protocole spatial](PROTOCOLE_LIDAR_SPATIAL_20260921.md) garde la scène,
ses deux moitiés et ses quatre quarts, définis par deux plans perpendiculaires
passant par le capteur. Les morceaux servent au diagnostic de croissance.
Leur somme, leur meilleur temps ou l'exécution d'un quart en moins d'une
seconde ne valide **jamais** le contrat sur la trame entière. Les expériences
synthétiques8k/16k/32k et les anciennes mesures50k restent des diagnostics
distincts. L'objectif de plusieurs dizaines de millions de points sur G4
reste également ouvert, sans transfert de qualification depuis une trame.

## Régime prioritaire complémentaire : LiDAR sans sol

L'utilisateur demande aussi, le21 septembre, un retrait du sol très rapide,
plus pertinent qu'un seuil de hauteur. Le [protocole sans sol](LIDAR_SANS_SOL_PROTOCOLE_20260921.md)
prévoit une segmentation géométrique de la trame entière, puis ses sept
objets spatiaux avec le même masque figé. Conserver les coordonnées float32
et les IDs originaux de tous les retours, retirés compris. Mesurer le coût
du retrait, celui de HGP et leur somme ; publier les effectifs réellement
retenus et les erreurs de segmentation. Aucun plafond de cardinalité.

C'est un régime prioritaire **supplémentaire**, pas un remplacement de la
référence brute entière. Les hiérarchies exactes du sous-nuage ne sont pas
celles du nuage original : enlever des témoins change profondeurs, contacts
et événements. La qualité de segmentation et l'exactitude HGP sont deux
questions distinctes. Aucune mesure sans sol n'est encore qualifiée.

## Situation à cette décision

Le [raccord spatial q3/q4](Q34_MESURES_SPATIALES_20260921.md) mesure un
producteur de candidats avec index, census et coquilles, **pas FULL**.
Il n'existe pas encore de résultat qualifiant la tour sur cette nouvelle
cible. Les modes multi-CPU exécutés sur une machine G4 restent des mesures
CPU : ils ne deviennent pas des résultats GPU par le nom de la machine.

L'autorisation GCP est renouvelée. Employer une seule session **SPOT**
utile à la fois, les scripts gardés et les deux coupe-circuits du dépôt ;
conserver configuration, sources, sorties, ressources et arrêt ciblé
certifié. Les petites vérifications de protocole restent locales.
