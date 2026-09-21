# R3 G4 SPOT : résultats clos et diagnostic

Les cinq commandes prévues sont achevées ; la gate92 est passée et l'arrêt de
la génération GCP est certifié. Les deux lecteurs passent en Python normal et
`-O`, avec sorties identiques octet pour octet. Voir `POSTHOC_READBACKS.json`,
`ANALYSIS.json` et `CPU_COSTS.json`. Les empreintes de toutes les preuves lues
sont égales avant/après ; le paquet exécuté et les sources n'ont pas été modifiés.

Périmètre : flux exact de candidats q3/q4, K5, s8, backend local28, quantification
u16 du scan0 SemanticKITTI, sur les préfixes 1k/2k/4k/8k du même fichier8k.
Le GPU n'est pas utilisé. Ce n'est ni un catalogue dédupliqué ni la tour FULL.
Les grands flux sont jugés par leurs invariants, digests et comparaisons ; leur
exhaustivité n'est pas recalculée par un oracle quadratique/cubique indépendant.

## Temps et occupation réelle des CPU

Les temps de pipeline incluent nuage/index partagés et front/traitement/collecte.
L'occupation moyenne vient de `(temps utilisateur + système) / temps mur`
rapportés par GNUtime. Ses arrondis au centième expliquent les petits écarts
avec son pourcentage CPU entier.

| n | workers | pipeline (s) | CPU cumulés (s) | CPU occupés en moyenne | callbacks q3+q4 |
|---:|---:|---:|---:|---:|---:|
| 1 000 | 1 | 3,849 | 3,85 | 1,00 | 11 357 |
| 1 000 | 48 | 0,863 | 4,97 | 5,78 | 11 357 |
| 2 000 | 48 | 8,470 | 31,19 | 3,68 | 24 061 |
| 4 000 | 48 | 59,274 | 220,58 | 3,72 | 49 949 |
| 8 000 | 48 | 614,744 | 1 986,83 | 3,23 | 104 670 |

À1k, W1/W48 fournit exactement les mêmes digests et comptes géométriques, avec
un gain de temps ×4,459, pas ×48. À8k, seulement6,73% des 48 CPU demandés sont
occupés en moyenne. Le temps mur est environ10min15, pour les seuls candidats
q3/q4. Aucun contrat50k/1s ou100ms n'est atteint ni extrapolé.

La ligne locale achevée `global_tsd9ofnm/record_0000.json` est identique au8k
G4 pour tous les comptes logiques et payloads après neutralisation de **deux
seuls pics de capacité**. Son temps est1360,996s àW4, contre614,744s àW48 sur
G4 : rapport×2,214. Cela compare deux machines et deux nombres de workers ; ce
n'est pas une mesure isolée d'accélération parallèle. La campagne locale reste
`failed` parce que la commande suivante a été interrompue : sa ligne achevée
revalidée ne transforme pas la campagne entière en succès.

## Le volume de travail n'est pas sous-quadratique sur ces préfixes

| Poste | 1k→2k | 2k→4k | 4k→8k |
|---|---:|---:|---:|
| Arêtes résiduelles développées | ×2,84 | ×2,76 | ×2,97 |
| Population cumulée des covers | ×5,45 | ×5,28 | ×6,27 |
| Triangles q3 à examiner | ×5,95 | ×5,55 | ×6,42 |
| Tests de sites du census q3 | ×10,79 | ×10,08 | ×10,76 |
| Triangles de départ q4 | ×5,59 | ×6,48 | ×5,67 |
| Visites de l'atlas q4 par les requêtes | ×9,38 | ×10,91 | ×10,39 |
| Tests de droites q4 | ×10,46 | ×12,81 | ×12,01 |
| Bornes par blocs de la partition q4 | ×4,80 | ×5,18 | ×5,99 |
| Comparaisons de tri des événements q4 | ×2,34 | ×2,06 | ×2,17 |

Le quadruplement est le repère quadratique pour chaque doublement de n. Le
front réduit suffisamment le nombre d'arêtes pour ce poste, mais **pas le
travail cumulé après chaque arête**. Déclarer le raccord sous-quadratique sur
la seule colonne des arêtes ou sur le tri serait faux.

À8k :2,286M arêtes résiduelles entraînent5,114 milliards de sites cumulés dans
les covers ;780,662M triangles q3 entraînent361,201 milliards de tests de sites
pour seulement93 914 candidats q3 émis. Côtéq4 :439,970M triangles de départ,
6,991 milliards de visites de requête,2,230 milliards de tests de droites,
1,708 milliard de bornesZ et3,946 milliards de tests ponctuels de préparation,
pour10 756 candidats q4 émis. Les comparaisons de tri q4 sont seulement16,403M.

Ce sont des volumes exacts par catégorie, pas des durées par étape. Ils
identifient les répétitions qui explosent ; ils ne mesurent pas à eux seuls
quelle fraction du temps CPU appartient à chaque catégorie. Les sorties
augmentent autour de×2,1 pourq3 et×2,2 pourq4 : leur taille observée n'explique
pas la croissance du travail interne.

## Répartition du travail :48 slots actifs mais des lots trop inégaux

Les runsW48 ont768 jobs terminés et les48 slots ont tous traité des arêtes.
Cependant, à4k les quatre slots les plus chargés en arêtes concentrent48,68%
des arêtes ; à8k,48,77%. Le slot maximal à8k traite375 812 arêtes (16,44%),
contre930 pour le minimum. La répartition des jobs varie de1 à68 par slot à8k :
le nombre de jobs n'est donc pas une mesure homogène de leur coût.

Le code explique une limitation plausible : `wspd_q34.cpp:534` attribue un
sous-arbre Coarse entier, et `:291` parcourt les arêtes du rectangle dans le
même worker ; aucun don de reste de sous-arbre/rectangle n'a lieu ici. Les
comptes montrent un déséquilibre logique et GNUtime une faible occupation.
**Aucune durée par worker n'est enregistrée** : on ne prétend pas mesurer le
temps du slot maximal, sa période d'inactivité ni attribuer toute la perte à
ce seul mécanisme.

Deux chantiers distincts sont donc nécessaires : partager les lots coûteux
restants à une granularité possédée plus fine, et réduire structurellement
les répétitions q3/covers et q4/triangles-atlas. Le premier peut améliorer
l'occupation des CPU ; il ne supprime pas les facteurs de croissance>4 du
second. Optimiser encore le seul tri ne traite pas ces volumes dominants.
