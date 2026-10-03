# Audit courant v11 — corrections et performances G4

3 octobre 2026, 17:02:01 UTC. Moteur jugé : **`c40f40798375a0fc37917499401f16876cccbd2a`**.
Cadre : `exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`.
Cette note remplace mon suivi antérieur ; les preuves et premiers échecs restent dans les reçus.

## Verdict et corrections

La tour FULL native est implémentée : catalogue critique, index/census global,
MEB et descentes datées, naissances, multifusions atomiques, parents et verticales.
La contrelecture des ports depuis 70e494777 est favorable sur les chemins valides.
Deux défauts de contrat étaient à corriger, même sans divergence FULL établie :

| Correction intégrée | Motif et contrôle |
|---|---|
| Propriétaire avant PopulationLookup | Un hit pouvait contourner le refus de mémo/workspace étranger. Gardes avant hit/miss, singleton et appel direct ; mutants ciblés tués. |
| Admission des census possédés | Les espaces fournis sont attachés aux IDs physiques0..S−1. Avec W48/S4, deux tâches prises par 30/31 peuvent nécessiter deux allocations. Borne `min(tâches,W−S)` sous S≤W ; oracle de sous-ensembles et budgets exact/−1. Ce n'est pas un scheduling forcé du Pool. |
| Tri certifié F3/F4 | Quatre arrondis, modes mixtes préparation/comparaison, FTZ/DAZ, rationnels égaux/proches, permutations et pannes ; marge2⁻⁴⁰ et repli exact conservés. |
| Protocole apparié | Inventaire réel LIST et labels unit/oracle vérifiés ; concurrence exigée par noms de portes. Fixture 94 portes et 72 contrôles Python, plus replay simulé81/81. |

[Invariants mathématiques et projection ouverte](AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md).
Les coupes Dom sont monotones, les triplets obtus encore prolongés quand permis,
les contacts fermés et coquilles globales préservés. Tables CAS immuables et publication
après barrières ; unions de racines courantes et composition BirthRuns cohérentes.
Les oracles bornés qualifient leur source, pas automatiquement toute version future.

## Qualification close

**4073/4073 exécutions de portes**, soit 3729 dans la matrice principale et 344 dans ASan18.
**326 mutants tués** :324 par juges et 2 refus de compilation attendus ; aucun signal/délai.
Release18/21/24, ASan24, TSan21, poison21 et ASan18 num/index/tower passent.
Les portes FENV du catalogue sont hors supplément ASan18. Clang est absent, donc non qualifié.
Lecteurs indépendants normal/−O concordants ; paquet source et inventaire exhaustif des
résultats rehachés, arrêt ciblé certifié.

Les premiers essais restent visibles : un mutant de catalogue ne compilait pas
(4072/4073, source672afb71c), puis6503c95ab a passé 4073/4073. Les deux premiers bancs 650
ont passé 94/94 portes natives mais refusé avant chrono : label absent, puis LIST lu
comme DICT. La source c40 rejoue toute la qualification avant le banc corrigé.
[Archives, lectures et commandes](../receipts/qualification_performance_20261003/README.md).

## Mesures appariées G4

**81/81 prises réussies**, sorties FULL identiques. Médianes de trois prises, en ms ;
le mode rapide réduit le mur de **62,7–69,5 %** face à la baseline v11.
Ses intervalles W48 min–max sont 468,7–493,1 /333,6–354,9 /415,7–444,1  ms.

| Trame | Baseline895 /2047 | Courant /2047 | Courant /16379 | Gain |
|---|---:|---:|---:|---:|
| 08/000000 | 1309,9 | 844,0 | 489,1 | ×2,68 |
| 08/000100 | 1070,9 | 678,7 | 345,1 | ×3,10 |
| 08/000200 | 1419,8 | 894,2 | 432,4 | ×3,28 |

| Mode16379, médiane FULL en ms | W1 | W8 | W48 |
|---|---:|---:|---:|
| 08/000000 | 10317,4 | 1416,1 | 489,1 |
| 08/000100 | 8032,6 | 1104,4 | 345,1 |
| 08/000200 | 9688,0 | 1335,9 | 432,4 |

Baseline **v11** 895680ff8/mode 2047 reconstruite et courant c40/2047/16379,
u21, mêmes compilateur/options et six hashes XYZ/IDs de reuse1. Trois trames entières
sans sol 1mm de la seule séquence 08, K=1..5, poids unitaires, W1/W8/W48,
trois répétitions avec rotation des producteurs. Processus et propriétaires neufs ;
caches OS non vidés. Chaque dump complet est comparé octet pour octet avant retrait,
rehaché à chaque tentative, avec une première inspection partagée par trame.
Les dumps réussis ne sont pas archivés : le lecteur recoupe les comparaisons capturées,
pas une nouvelle comparaison de fichiers absents ni 81 oracles indépendants.

Le mode 16379 ajoute graphe/table de populations/ordres concurrents et retire le mémo4.
Son gain ne distingue pas chaque mécanisme. FULL mesure index+domaine+forêts ;
segmentation, préparation, Cloud/Pool, IO, dump et Python restent séparés.
Naissances/publication changent de périmètre : comparer forêt entière ou blocs combinés.
Les phases globales concurrentes sont disjointes ; les murs par ordre et dispatchs
copiés se recouvrent. Une somme de tâches n'est pas un temps CPU.

Domaine médian 227,5 /181,1 /238,6  ms ; forêts 240,8 /163,6 /197,2  ms.
Ces médianes sont indépendantes et ne s'additionnent pas. Génération catalogue
169,1 /131,3 /176,5  ms ; descentes régulières 130,7 /73,5 /83,5  ms.
**Priorités : génération du catalogue, puis descentes non terminales de K5**,
qui concentrent environ 69 % des présentations MEB restantes. Le domaine seul
excède 200  ms sur chaque prise des trames 000000/000200 : les forêts seules ne suffisent pas.
Tri 9–13  ms et classification 2–3  ms sont secondaires. Les MEB baissent de 76–79 %, mais
les appels census augmentent légèrement ;405 bilans de pas et 81 de droites sont vérifiés.

CPU FULL médian 13,54 /10,66 /12,50 s ; pics Buffer+Cloud346,0 /298,7 /368,3 MiB,
soit+16–19 MiB face au courant 2047. Murs de processus 1,046 /0,832 /1,032 s,
dumps et destruction inclus ; inspection Python 146–189  ms séparée.
[Valeurs, phases et compteurs](../receipts/qualification_performance_20261003/review/analysis.md).

## Référence v10 et contrats ouverts

La v10 source 777406b82 mesurait 204–254  ms en u18, troisième passe chaude,
contre 1155–1515  ms pour l'ancienne v11 ae817/mode 2047/u21 : facteur observé 5,7–6,
avec domaine et forêts dominants. Ces captures ne sont pas appariées.
L'échauffement v10/ng00 259,8→252,0  ms n'explique pas seul cet écart.
Le juge public FULL v10 acceptait certaines forêts erronées : ses chronos ne sont
ni oracle de complétude ni qualification héritée.42 petites comparaisons canoniques
existent ; le différentiel **v10/v11 sur LiDAR entier** reste ouvert. Les81 mesures
présentes comparent deux sources v11. [Audit critique v10](../docs/AUDIT_V10_SYNTHESE.md).

Les neuf prises W48/16379 passent 500  ms ; **aucune des81 prises ne passe 200  ms**, ni 100  ms.
K10, plusieurs séquences, GPU, dizaines de millions de points et hiérarchie native
sur les points restent ouverts. FULL refuse les multiplicités conservées par Cloud.
Les coquilles et sorties peuvent dépasser une borne linéaire universelle ; ne pas
extrapoler ces trames. Réservations Buffer/Cloud≠RSS, piles et allocateur exclus.
Les préfixes publiés sont logiques ; visites physiques et préparation des lignes
vivantes (jusqu'à992 popcounts par feuille32) ne sont pas toutes publiées.
L'invariance de steps dépend du mémo : un hit population peut remplacer un hit à zéro pas.

Le Codespace est nettoyé : environ 25 Go disponibles, HGP-old et travaux actifs préservés,
sauvegardes uniques compactes vérifiées. [Reçu](../receipts/developpement_20261003/codespace_cleanup/README.md).
Aucun build/test natif local. Les six sessions G4 CPU sont fermées, arrêt ciblé certifié
pour chaque génération ; les deux définitives exécutent le même paquet c40 avec builds distincts.

## Pipeline publié : réserves levées

Les deux réserves du WIP du3octobre sont levées sur **b87285378** : les blocs
emploient `forest_sort` en place, avec clé totale (première boule, ordre, bloc local),
et le protocole v2 de `claudeab7` applique un verdict strict aux constructions,
portes, TSan, mutants et aux36prises. Toutes réussissent, sont jointes et ont
la même empreinte que leur base. Les346fichiers `src/tests` et le protocole de
cette session correspondent à la publication ; les24payloads du reçu sont clos.
La réserve sur `stable_sort` et les refus silencieux du premier banc ne décrit
plus le code courant. Aucun défaut géométrique n'avait été établi sur le WIP.

Relecture indépendante des JSON et empreintes, sans nouvelle compilation ni test
natif par cet auditeur : **666/666** CTests, **7/7** TSan, **11/11** mutants,
**36/36** prises appariées. Médianes FULL K5/u21/W48 :
**412,431 /351,685 /380,666 ms**, trois trames sans sol de la séquence08.
Ce nouveau lot ne transfère pas les4073portes de c40 aux autres profils ni à K10.
Le contrat100ms, GPU, massif et hiérarchie native de points reste ouvert.

[Reçu du constructeur](../receipts/developpement_20261003/pipeline_g4/README.md)
et [lecteur à verdict](../receipts/developpement_20261003/pipeline_g4/check.py).
La campagne FULL→points conserve volontairement la source c40 figée, afin de
juger les règles de projection sur un même objet FULL.

Conseil restant pour les reprises : dans `protocol/ab_g4.py`, la branche détectant
un groupe survivant le tue sans confirmer ensuite sa disparition ; les étapes
perf/paranoid sont exclues du verdict. Attendre le groupe vide avant l'étape
suivante protège aussi les futurs chronos en cas d'échec. Aucun survivant n'est
observé dans `claudeab7`. Le README du reçu attribue encore par erreur sa
qualification à `claudeab5`, qui est un refus Spot. Enfin, les chronos mesurés
n'excluent pas d'autres réglages de workers ou de planification : écrire
« seuil non atteint dans les configurations testées » ; ils ne prouvent pas
une impossibilité200ms sans refonte ni l'unicité d'une baisse du travail CPU.
