# Session G : objectif de dégénérescence et capacités

Lecture du document à `5c5fc710921ac03f6d4c2aca40f17e8442202a5c`, distincte de l'audit de code à
`58d384721678d11ef8ccd86c76cca182f41a716c`. Entre ces deux épingles, les bornes de T1 examinées ici n'ont pas changé.

**L'objectif est nouveau, sa solution n'est pas livrée.** Le reçu `g4_t0g_20261007/README.md` exige désormais les
synthétiques à toutes les tailles et « aucune feuille refusée pour largeur », en renvoyant au contrat § 9. Or
`catalogue.hpp` plafonne toujours `max_leaf` à 256, `leaf_census.hpp` refuse une coquille au-delà de 64,
`BallRecord::m` est un `u8`, et la tour prévoit des masques de 64 bits. Le warp virtuel de 256 voies ne satisfait
donc pas cette extension. Un dépassement du paramètre est refusé ; relever seulement une constante exposerait
ensuite les types, masques, index locaux, files et budgets. L'extension doit être déclarée comme chantier à qualifier.

**La cause publiée doit aussi être corrigée.** Le reçu dit que `synth_sphere` a « tous les sites cosphériques ».
`prepare_small.py:74–81` projette des directions sur le rayon 10 000 puis arrondit chaque coordonnée à l'entier :
c'est une coquille sphérique approchée. Les cinq premiers points des graines de n100, n3000 et n10000 ont des
déterminants de cosphéricité non nuls, respectivement −5956889720665386, −7198927785526392 et
7919238563384820. Ces cinq points ne sont sur **aucune** sphère exacte commune ; une translation ne change pas ce
fait. Le témoin `check_degeneracy.py` ne génère que cinq points par graine ; ils ont été comparés aux cinq points
renvoyés par le générateur public, sans lire aucune archive de nuage. Les refus `wide_leaf` à 3 000/10 000 restent
avérés dans les lignes brutes, mais ne prouvent ni m=3 000/10 000 ni une coquille étendue de ces tailles : taille de
liste candidate, taille de coquille exacte et nombre total de sites sont trois quantités distinctes.

**Pourquoi q_min ≤ 4 ne résout pas le coût.** Carathéodory borne la taille d'un support minimal, pas la coquille m.
Une sphère critique peut avoir arbitrairement beaucoup de sites sur son bord ; tous ceux-ci appartiennent à sa
population et doivent être conservés. Énumérer ses quadruplets offre déjà C(256,4)=174 792 640 présentations,
C(3 000,4)=3 368 254 124 250, C(10 000,4)=416 416 712 497 500 avant filtres. Ce sont des nombres de combinaisons,
pas des mesures du nombre réellement visité. Une extension par élargissement des tableaux n'apporte donc aucune
borne utilisable de travail.

Le noyau sans lots et `LEM-T4` garantissent les mêmes multifusions après contraction d'un plateau. Ils ne bornent
pas le nombre de cellules ni de représentants consommés en fonction du seul n ; un temps linéaire en événements
produits ne prouve pas un temps linéaire sur le nuage. De même, le quotient de coquille `LEM-T7` évite l'énumération
des parties mais annonce O(m³) prédicats : il demande une implantation et un jugement propres, et ne donne pas à
lui seul un budget satisfaisant à m=10 000.

Avant d'élargir, le protocole utile est :

1. Sur le générateur public épinglé, publier maximum de liste candidate, profondeur, largeur au refus et histogramme
   des coquilles exactes des boules admises ; distinguer quasi-sphère, sphère entière exacte et réseau.
2. Pour le réseau, ventiler préparation/résolution/noyau/contraction et publier cellules, représentants, boules,
   incidences et tailles de coquilles. « 99 % forêt » ne désigne pas à lui seul le coût du noyau union-find.
3. Écrire le contrat d'une voie large : masques/CSR extensibles et budgets, stratégie certifiée de traitement des
   listes larges, canonisation par boule, quotient de traces si adopté ; aucun écrêtage, jitter ou préfixe.
4. Graver les petites versions contre l'oracle, les frontières 32/33, 64/65 et 255/256/257, les mêmes plateaux avec
   différents supports minimaux, puis mesurer sur G4 les tailles prévues avec sorties complètes et refus explicites.

Rejeu léger : `python -S .../mathematiques/check_degeneracy.py`, identique sous `-O` et à
`degeneracy_resultats.json`. Ce complément ne lance aucun moteur et n'établit aucun résultat de vitesse v12.
