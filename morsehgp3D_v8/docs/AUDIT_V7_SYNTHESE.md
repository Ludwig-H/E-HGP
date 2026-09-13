# Audit général v7 : ce qui est acquis, ce qui bloque, ce que v8 doit changer

13 septembre 2026. Base publiée `dc57ffd5`, sur `main`.
Cadre v8 : exploration hors registre, backend absent,
profil `quantized_u16_input_only`, audit mathématique et architectural,
`public_status=not_claimed`. Aucun moteur v8 livré par cet audit.

## 1. Le verdict en quelques phrases

**Priorité de refonte désormais confirmée par l'utilisateur : supprimer
le passage systématique par A×A et B×B pour calculer les témoins locaux.**
Le [P0 du plan](PLAN_DE_REFONTE.md) laisse les architectures en concurrence,
petits ensembles certifiés compris, et exige de mesurer aussi le travail
reporté sur les candidates restantes. Cette décision ne change pas les
résultats historiques de l'audit ni les contrats encore ouverts.

La v7 a établi des bases mathématiques utiles et produit des tours FULL
50k, avec des confrontations indépendantes sur petits cas. Elle **n'a pas
atteint** les contrats temporels, ni livré une chaîne GPU FULL industrielle.
Ses optimisations récentes sont en partie des prototypes privés publiés,
pas un chemin produit unique ni de nouvelles mesures 50k.

La lenteur principale n'est pas le tri amont : sur la tour 50k/K10
publiée, il prend 3,13 s sur 418,87 s. Le constructeur FULL prend 389,67 s,
dont un travail de 42 millions de petites boules englobantes et de
3,90 milliards d'essais de supports. Plus tard, les prototypes réduisent
ce travail et en distribuent une partie, mais paient encore beaucoup de
préparation, d'histoire et d'export. Accélérer un seul de ces postes ne
suffit donc pas. [Mesures et périmètres exacts](../audits/CONTRATS_ET_MESURES.md).

L'intuition de paralléliser par rectangles WSPD est juste. **La v7 le fait
déjà partiellement**, tout en gardant des boucles coûteuses à l'intérieur
d'un rectangle. Une file de tâches plus fine est nécessaire : blocs de
témoins, tuiles de paires, complétions q3 et balayages q4. Cela doit réduire
le travail total autant que sa durée parallèle.

Enfin, on peut éviter de construire tous les niveaux Γ. Il faut cependant
conserver les bons rattachements silencieux pour obtenir les parents,
puis l'histoire datée, les contributions et les verticales. Ni « seulement
les minima et leur graphe induit » ni « seulement un MST des points » ne
reconstruisent la tour générale. [Fondements](../audits/FONDEMENTS_ET_OBJET.md).

## 2. Situation des contrats 50k

| Tour complète | CPU, dernière complétion publiée | Hybride census GPU + FULL CPU | Cible |
| --- | ---: | ---: | ---: |
| K=1..10 | 418,873 s | 418,921 s | <1 s, puis 100 ms |
| K=1..5, repli | 33,853 s | 33,569 s | <1 s, puis 100 ms |

Runs du 10 septembre, uniforme u16, graine3, s8, G4, 48 CPU amont mais
FULL mono-thread, digest inclus. Les temps externes sont légèrement
supérieurs. C'est environ 419 fois et 34 fois une seconde sur ces captures,
**pas une mesure des prototypes ultérieurs**. Les quatre sorties sont
conservées ; un refus rapide n'est pas substitué à une complétion.

À K10, le catalogue partagé compte 21,47 millions de boules et la tour
27,27 millions de nœuds, avec environ 15,5 Gio de RSS CPU. À K5, elle
compte 4,21 millions de nœuds. Cette amplification est réelle ; le nombre
de points seul ne décrit pas le travail demandé.

Le census GPU K10 exécute ses kernels en 189 ms, mais sa phase complète
prend 4,54 s, dont 2,93 s de reconstruction hôte. Ce résultat prouve qu'un
kernel fonctionne sur carte, pas que toute la chaîne est accélérée.
[Quatre JSON, compteurs, mémoire et vérifications](../audits/CONTRATS_ET_MESURES.md#2-les-quatre-vrais-runs-50k-full).

## 3. Les trois « v7 » qu'il ne faut plus confondre

| Chemin | Ce qu'il fournit | Maturité réelle |
| --- | --- | --- |
| CLI `mhgp7` | Pipeline réduit F, archive sans verticales FULL | Intégré, mais ce n'est pas le produit FULL demandé |
| Sonde FULL CMake | Génération/census puis Builder FULL CPU et tour conservée | Raccord réel, petits oracles, tours 50k ; contrat temporel échoué |
| Atlas, graphe daté, flux, rangs et GPU terminal privés | Nouveaux objets et différentiels, sources épinglées dans les reçus | Acquis exploratoires, intégration produit et nouvelle mesure 50k manquantes |

La préparation locale « marques au premier parcours » postérieure au
dernier commit est laissée intacte. Elle est seulement signalée comme
préparation locale, exclue des références publiées de performance et
des acquis intégrés. Elle n'est ni détruite ni promue pour gonfler les résultats.
[Carte de code et maturité GPU](../audits/IMPLEMENTATION_PARALLELISATION.md).

## 4. Réponse à l'idée h, h_a, h_b et aux citrons q3/q4

Un rectangle A×B représente beaucoup de paires. Trouver quelques points
dont on prouve qu'ils sont intérieurs à **toutes** les boules pertinentes
permet d'écarter le rectangle entier. Si cela échoue, des crédits locaux
par a et par b peuvent éliminer davantage de paires sans construire leurs
boules une par une. Les trois populations de témoins doivent être disjointes.

Pour la tour jusqu'à Kmax, les seuils suffisants sont :

| Kmax | q2 | q3 | q4 |
| ---: | ---: | ---: | ---: |
| 10 | 10 | 9 | 8 |
| 5 | 5 | 4 | 3 |

Le fuseau q4 est plus fin que q3, lui-même plus fin que q2. Des boîtes
plus petites relativement à leur séparation donnent souvent de meilleures
preuves, mais un s supérieur augmente aussi la décomposition et les
parcours. Les essais 8k cités voient moins de comparaisons de coins à s12,
mais davantage de visites et un front plus lent. Il faut comparer s8/10/12
par phase et par famille ; aucun optimum universel n'est validé.

Le problème le plus net : les histogrammes locaux scalaires peuvent
coûter O(|A|²+|B|²), même pour un unique rectangle très bien séparé. Sur
deux amas, leur temps passe de 2,04 à 8,39 puis 31,08 s aux trois tailles.
La séparation seule ne corrige pas cette croissance. Il faut éliminer et
créditer par blocs, avec certificats négatifs, saturation et réemploi
sans doublon, puis distribuer les tâches restantes.

La taille linéaire d'une WSPD théorique ne prouve ni un parcours total
linéaire de ses facteurs, ni la borne de coût du trie Morton particulier
de la v7. L'audit identifie précisément cette obligation ouverte.
[Rapport complet WSPD, preuves, code et fausses pistes](../audits/WSPD_Q2_Q3_Q4.md).

## 5. Décisions de reprise

| Sujet | Décision | Pourquoi |
| --- | --- | --- |
| Objet FULL avec vrais parents | Conserver le contrat et les contre-preuves | Il répond à la vraie tour, sans Γ exhaustif |
| Supports q≤4, clés exactes, census partagé | Reprendre explicitement et requalifier | Bonne factorisation géométrique en 3D |
| Petits oracles et mutants | Conserver comme juges indépendants | Ils ont déjà trouvé des erreurs que des digests ne voyaient pas |
| Résolveur statique et facettes uniques | Base de la refonte, avec meilleur chercheur MEB | Le travail géométrique peut être préparé hors calendrier |
| Catalogue/rangs/masques | Un seul propriétaire immuable | Éviter répétitions de tris, validations et tableaux |
| Calendrier mutable déclenchant les MEB | Remplacer comme architecture parallèle | Il mêle calcul indépendant et dépendances temporelles |
| Histoires et verticales | Forêts datées, contraction parallèle, index partagés | Ne pas limiter le parallélisme aux consultations d'une histoire construite en série |
| GPU par allers-retours de petits lots | Refaire le raccord autour d'objets résidents | Le coût hôte dépasse de loin les kernels observés |
| CLI/archive F comme livraison FULL | Ne pas reprendre cette assimilation | L'objet et le format ne correspondent pas |
| Nombre arbitraire de variantes dans les reçus | Une chaîne v8 principale, anciens chemins différentiels | Savoir exactement quel moteur est mesuré et livré |

Ces décisions ne signifient pas « tout le code est faux ». Beaucoup de
primitives et de fixtures sont précieuses. Ce sont le coût total, les
frontières d'objets et l'intégration qu'il faut reprendre.

## 6. Risques prioritaires et preuves manquantes

Critiques pour la livraison : aucun backend FULL unique industriel,
aucun contrat 1 s, aucun contrat de dizaines de millions, pas d'archive
FULL intégrée ni de reprise de calcul qualifiée.

Critiques pour l'exactitude revendiquée : complétude du producteur,
portée réelle des preuves WSPD, traitement de tous les plateaux du domaine
annoncé, vrais parents avant normalisation et provenance des certificats.
Les tests bornés restent nécessaires mais ne closent pas ces questions
universellement. Le profil u16 ne rend pas tous les nuages réguliers.

Critiques pour l'échelle : grosses tâches déséquilibrées, travail local
quadratique, catalogues globaux et copies, offsets/identifiants dérivés
32 bits, coquille bornée à 12 et masques u16. Des dizaines de millions de
points peuvent engendrer bien plus d'objets que d'identifiants de points.

Point mathématique non négociable : la sortie FULL explicite peut elle-même
être quadratique dès K2, à précision croissante. La bonne cible est un
faible surcoût par rapport à la sortie, avec croissance mesurée par régime.
Cela ne condamne pas le contrat 50k ; cela interdit seulement de promettre
une énumération universellement sous-quadratique indépendamment du résultat.

## 7. Ce que cet audit a effectivement vérifié

La revue couvre chaque étape de l'algorithme, les fondements, les objets
de sortie, les chemins intégrés/privés, la parallélisation, les mesures,
la mémoire, les formats, les tests et les contrats. Elle croise trois
contrelectures spécialisées et les audits indépendants antérieurs.

Elle **ne prétend pas** avoir relu ligne par ligne tous les 26 777 fichiers
suivis v7 ni réexécuté toute la suite C++. L'inventaire détaille 240 fichiers
de code/tests/docs avec empreintes ; les gros paquets de preuves sont
identifiés séparément. Six relectures de paquets clos et deux recalculs
arithmétiques ont réussi. Aucun nouveau benchmark, compilation C++ ou
usage GCP n'est présenté comme résultat. La CI v7 existe ; son dernier
run distant n'a pas été interrogé.
[Périmètre, sources et limites](../audits/PERIMETRE_ET_PREUVES.md).

## 8. Lecture conseillée et suite

Commencer par [l'algorithme expliqué simplement](ALGORITHME_EXPLIQUE.md),
puis le [plan de refonte ordonné](PLAN_DE_REFONTE.md). Les quatre volets
techniques permettent ensuite d'aller au niveau voulu :
[mathématiques](../audits/FONDEMENTS_ET_OBJET.md),
[WSPD](../audits/WSPD_Q2_Q3_Q4.md),
[implémentation/parallélisation](../audits/IMPLEMENTATION_PARALLELISATION.md),
[contrats/mesures](../audits/CONTRATS_ET_MESURES.md).

Le premier chantier est P0 : comparer en mono les architectures évitant
les histogrammes quadratiques systématiques, avec petits juges et tranche
FULL minimale pour vérifier leur effet aval. Les optimisations CPU/GPU
suivent cette priorité ; aucun port général ne doit figer l'ancien coût.
Le dossier v8 conserve la structure de la v7 mais ne copie pas son moteur
en bloc. GCP non utilisé pour cet audit et cette décision documentaire.
