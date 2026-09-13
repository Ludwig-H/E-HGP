# Crédit sûr de groupes qui se recouvrent

13 septembre 2026. Complément indépendant au
[certificat collectif](../../morsehgp3D_v8/audits/P0_SOUS_RECTANGLES_ET_GROUPES.md),
relu après `3589a2c9`. Cadre : `exploration_v8_hors_registre`,
`cpu_reference`, `quantized_u16_input_only`,
`audit_independant_math_and_architecture`, `not_claimed`.
Modèle mathématique borné, aucun port produit ou contrat de tour.

## Compter les groupes sans imposer leur disjonction

L'identité de puissances du premier auditeur est correcte : chaque groupe
certifié contient au moins un intérieur strict, qui peut varier avec la
sphère. La somme brute des groupes reste incorrecte lorsqu'ils se recouvrent.
On peut cependant conserver cette information par des capacités sur les IDs.

Pour chaque groupe certifié G, choisir un poids rationnel β_G≥0 tel que
chaque ID reçoive une charge totale au plus un. Ces poids d'agrégation
sont distincts des poids α utilisés pour certifier la géométrie du groupe.
Si I désigne les IDs strictement intérieurs à une sphère visée :

$$\sum_{G\ni z}\beta_G\leq1\quad\text{pour tout ID }z,\qquad \sum_G\beta_G\leq\sum_G\beta_G|G\cap I|=\sum_{z\in I}\sum_{G\ni z}\beta_G\leq|I|.$$

La profondeur étant entière, le crédit sûr est donc
$\left\lceil\sum_G\beta_G\right\rceil$. Un choix immédiatement vérifiable
est β_G=1/Δ, où Δ est le degré maximal d'un ID dans la famille de groupes :
le crédit devient ⌈nombre de groupes/Δ⌉. Aucun solveur de programmation
linéaire ni recherche de sous-famille disjointe n'est requis pour ce choix.

Cette vérification visite chaque incidence groupe–ID une fois lorsque les
charges sont indexées par ID ; un dictionnaire ordonné ajoute son coût
logarithmique. La préparation de groupes géométriquement utiles reste à payer.
On ne développe aucune paire d'ancres pour seulement agréger ces crédits.

Le raisonnement vaut aussi pour un sous-rectangle lorsque chaque groupe
y garantit déjà un intérieur pour toutes les sphères concernées. Il ne
fournit pas cette certification géométrique à la place du test aux moments
ou de la preuve paramétrique. Un crédit de cœur ou de ligne ne s'ajoute
toujours pas librement : ses populations doivent être disjointes, ou ses
certificats identifiés intégrés au même contrôle des capacités.

## Fixture q4 où le gain est strict

Prendre a=(0,17,10), b=(20,17,10) et les cinq sites suivants :

| ID | Coordonnées |
| --- | --- |
| 0 | (10,26,10) |
| 1 | (10,21,18) |
| 2 | (10,10,16) |
| 3 | (10,10,4) |
| 4 | (10,21,2) |

Les cinq triplets ci-dessous ont un barycentre égal au milieu d'ab, avec
les poids positifs indiqués dans l'ordre des IDs. Leur marge collective
est strictement négative : chaque triplet rencontre l'intérieur de toute
sphère passant par a,b.

| IDs | Poids α | Marge |
| --- | --- | --- |
| (0,1,3) | (16/79,27/79,36/79) | −1384/79 |
| (0,2,3) | (7/16,9/32,9/32) | −67/4 |
| (0,2,4) | (16/79,36/79,27/79) | −1384/79 |
| (1,2,4) | (2/11,4/11,5/11) | −200/11 |
| (1,3,4) | (5/11,4/11,2/11) | −200/11 |

Chaque ID appartient à trois groupes. Leur poids d'agrégation 1/3 donne
le crédit **⌈5/3⌉=2** ; une sélection disjointe de ces triplets en conserve
au plus un. Les cinq sites échouent tous aux tests ponctuels W3 et W4.

Ce gain atteint la profondeur d'un vrai support q4 positif : ajouter
c=(10,4,7), d=(10,4,13). Son centre est (10,14,10), son rayon carré 109,
ses poids barycentriques sont (5/13,5/13,3/26,3/26). Le déterminant vaut
−1560 et ab est l'unique arête maximale. Les puissances des cinq sites
valent respectivement (35,4,−57,−57,4) : exactement deux sont intérieurs.
À Kmax=4, h_q4=2, le crédit fractionnaire suffit donc au rejet.

Cette fixture qualifie la géométrie collective ; elle ne représente pas
une WSPD construite ni la recherche automatique de ces groupes.

## Réduire les IDs d'un certificat : deux portées distinctes

Pour le certificat de **toutes** les sphères passant par une paire fixée
a,b, trois IDs positifs suffisent toujours en dimension trois. Les poids
doivent préserver leur somme et les deux coordonnées transverses du
barycentre : trois contraintes linéaires. Au-delà de trois poids positifs,
une dépendance de ces contraintes permet un déplacement jusqu'à annuler
un poids. Choisir son signe pour ne pas augmenter la marge conserve sa
stricte négativité. Répéter donne au plus trois IDs. Le paramètre de corde
reste dans (0,1), car la convexité de la norme carrée impose :

$$\text{marge}\geq\lambda(\lambda-1)\Vert b-a\Vert^2.$$

Le juge exerce une compression effective de cinq à trois IDs sur la
fixture. Aucun de ses sites n'est sur la corde et aucune paire de leurs
projections transverses n'est colinéaire avec l'origine : trois IDs sont
nécessaires dans cet exemple.

Pour le nouveau **certificat de moments sur tout un sous-rectangle**,
la compression précédente à trois IDs ne se transfère pas. Une autre
preuve donne au plus **quatre** IDs : préserver la somme des poids et
le barycentre 3D, tout en diminuant Q=Σα_z|z|². Ce sont quatre contraintes
linéaires ; la même élimination de dépendances réduit le support. Le
barycentre préservé laisse C̄ identique pour toutes les paires, et la
diminution de Q augmente H̄ partout. Chaque ancien test strict aux moments
reste donc vrai. C'est une preuve de taille de certificat, sans port C++
ni nouveau juge de compression des moments dans cette livraison.

Ces compressions n'autorisent pas l'énumération de tous les triplets ou
quadruplets. L'élimination naïve répétée peut elle-même être quadratique
dans la taille du groupe. Les nouveaux poids rationnels doivent aussi
respecter une porte arithmétique propre : leur mise au même dénominateur
ne garantit pas S≤2²⁸. Conserver le certificat initial ou l'indécision
reste possible lorsque cette porte n'est pas satisfaite.

## Vérification bornée

Le [juge autonome](collective_overlap_gate.py) passe en normal et `-O` :
208 sphères, 1 040 identités pondérées de puissances, 17 membres sur
la frontière, un vrai support q4 positif, et compression effective 5→3.
L'énumération indépendante des 32 sous-ensembles d'IDs confirme que deux
intérieurs sont nécessaires pour rencontrer tous les groupes. Des
contre-modèles réfutent les capacités omises et le compte brut des groupes,
qui sont non sûrs. L'arrondi inférieur reste un minorant sûr, mais perd
le second crédit garanti : le juge détecte aussi cet affaiblissement.

Le [reçu](COLLECTIVE_OVERLAP_CHECKS.json) conserve les commandes, codes,
empreintes et le rejeu du modèle de l'autre auditeur. Aucun fichier de
ce dernier, source produit, registre, index Git ou ressource GCP modifié.
GCP non utilisé.
