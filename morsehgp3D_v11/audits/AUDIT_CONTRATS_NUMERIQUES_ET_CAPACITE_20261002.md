# Reprise développeur v11 — audit courant et écart avec la v10

3 octobre 2026, 13:11:38 UTC. Code relu : **`479f53f0b9781a9f035333ce6a283b5b83e70569`**.
Relecture de tous les ports performance depuis70e494777, notamment ef75dafac et479f53f0b.
Je passe côté développeur sur instruction de l’utilisateur. Cette note remplace
mon ancien état du 2 octobre ; les preuves closes restent dans `receipts/`.
Cadre : `exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`.

**FULL est maintenant implémenté et qualifié sur les petites fixtures.**
La dernière campagne entièrement close ici est
[reuse1, source ae817d09e](../receipts/full_regular_vertical_20261003/reuse1/README.md) :
3339/3339 portes, supplément ASan18 299/299, 292 mutants jugés, 29/29 FULL K5.
Les ports récents ont des tests et mesures locaux ; leur qualification G4 reste à faire. Les performances
annoncées ci-dessous exigent des options explicites, inactives par défaut. Le contrat FULL200ms reste ouvert.

## Ce qui a réellement été construit

| Couche | État et portée de l’audit |
|---|---|
| core/cloud/sched | Outcome/Result, propriétaires privés, réservations Buffer, conservation des IDs et poids ; Pool synchrone, slots privés et joins ; erreurs et mémoire jugées dans la matrice. Les piles OS et petits contrôles de threads ne sont pas des Buffer. |
| num | Budgets d’intermédiaires B18/21/24 ; q1/q2/q4 natifs, q3 avec certificat i128 ou essai contrôlé puis Wide ; orientation certifiée séparément. Level rationnel non réduit et comparaison exacte. F3/F4 dans le tri indirect ; pas de F2/F6 dans les prédicats du moteur. |
| catalogue | Listes K-certifiées, domination stricte, boîtes fermées pour les rejets et ownership séparé, J2, coquilles complètes, S* global ; cache de droites, tri indirect, frontière adaptative, assemblage parallèle et une passe déjà portés. Graphe de paires, lignes vivantes, coupe descendante et G3 avant les droites présents ; ports récents non qualifiés G4. |
| index/MEB/descente | Index global possédant Cloud ; census saturé ou I/U complet ; espaces réutilisés par lane. Diamètre exact, premier support positif contenant toute la partie, mémo avant MEB, dates initiale et terminale distinctes. |
| forêts FULL | Naissances, incidences régulières/étendues, multifusions atomiques, parents et verticales fermées ; lots privés, unions de racines courantes, ordres concurrents, PopulationLookup exacte, BirthRuns composés et préchargement ; réemploi des verticales conservé. |
| points/head/api | Modules produit encore absents ; définition core/cover et fixtures présentes dans la référence. Ni hiérarchie de points native ni comparaison effective à HDBSCAN. |
| preuves/outillage | GCC, ASan/UBSan, TSan, profils et poison sur G4 ; Clang absent. Intention, argv, entrées, sorties, mutants et fermetures conservés. Échecs de harnais distincts des défauts géométriques. |

Lecture favorable des chemins critiques, avec les obligations mathématiques
[maintenues dans la seconde note](AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md).
Ce bilan ne signifie pas que chaque ligne possède une preuve indépendante.
Les oracles bornés et les reçus qualifient leur source, pas automatiquement HEAD.

## Comparaison à périmètre utile

Même grille1mm, mêmes XYZ hachés et mêmes sous-nuages entiers sans sol de la
séquence08. G4 CPU, K=1..5, 48 travailleurs, sans projection de points.
V10 : source777406b82, u18, troisième passe chaude ; v11 : ae817d09e, u21,
mode2047, processus neuf et une mesure par case. Aucun ratio statistique apparié.

| Trame / sites | V10 catalogue + tour | V11 FULL | Rapport observé |
|---|---:|---:|---:|
| 08/000000 / 39885 | 252,0ms | 1463,154ms | ×5,81 |
| 08/000100 / 35551 | 204,2ms | 1154,972ms | ×5,66 |
| 08/000200 / 45845 | 253,6ms | 1514,543ms | ×5,97 |

Références : [synthèse v10 §3](../docs/AUDIT_V10_SYNTHESE.md),
[mesures closes v11](../receipts/full_regular_vertical_20261003/reuse1/metrics.json).
La première passe v10/ng00 vaut259,8ms et la dernière252,0ms : l’échauffement
observé ne rend pas compte d’un facteur six. Le différentiel canonique
v10/v11 **sur LiDAR entier** reste à faire ; 42 petites comparaisons FULL existent.
L’égalité des entrées et nombres de boules ne prouve pas l’égalité de toute la tour.
Le juge FULL public v10 acceptait certaines forêts erronées ; ses chronos
ne sont donc ni un oracle de complétude ni une qualification héritée v11.

Sur ng00/u21/mode2047 :

| Coût | V10 | V11 |
|---|---:|---:|
| Catalogue / domaine catalogue+lookup | 163,5ms | 804,679ms |
| Tour / forêts + verticales | 88,5ms | 658,042ms |
| Tri catalogue | 11,2ms | 57,804ms |
| Assemblage catalogue | 16,9ms | 4,660ms |

Ces sous-phases ont des frontières différentes : le domaine v11 inclut le
lookup ; la tour v10 inclut son index. L’index v11 vaut0,412ms, mesuré à part.
La géométrie une passe prend659,331ms. Les plateaux v11 prennent451,315ms,
classification68,736ms, naissances42,391ms et verticales80,846ms.
Les durées de dispatch incluent du travail parallèle : ne pas les additionner
à leurs propres sous-intervalles ni soustraire les sommes de tâches au mur.

## Audit des nouveaux ports pour rattraper la v10

Lecture favorable de la géométrie et des résultats FULL sur entrées valides.
Les deux P0 de la reprise sont maintenant **implémentés**, pas à reproposer.

| Changement | Verdict et contrat à conserver |
|---|---|
| Plan lourd d’abord et réclamation LPT | Répartition modifiée, couverture et ownership inchangés ; réduction de la tâche maximale locale, pas chrono W48 réel. |
| G1 préparé, popcount SWAR | Même expression entière et même population de masque ; moins d’opérations. |
| G3 avant droites, lignes vivantes et coupe d’extension | Sûrs par monotonie de Dom et seuil K−q ; test de paires conservé, triplets obtus encore prolongés quand permis. `prefixes` devient travail logique, pas nombre d’appels exécutés. |
| Tri F3/F4 | Comparaison exacte hors égalité ou bande ambiguë ; E6 et marge2⁻⁴⁰ conservatrices pour les niveaux du catalogue. 16N octets temporaires admis. Les quatre arrondis ne sont pas exercés par les nouvelles portes. |
| Signes natifs census | Même somme certifiée i128 et mêmes signes, sans conversion Wide ; repli historique hors voie native. |
| Tables support/population par CAS | Clés immuables, égalité entière après hash, lecteurs après barrière Pool ; aucun nouveau défaut de concurrence identifié statiquement. TSan courant reste à passer. |
| PopulationLookup avant chaque pas | Lemme terminal valide : I∪U contient S* ; MEB égale à la boule, p<k et t=m. Dates initiale/terminale distinctes préservées. Réserve sur les refus de contexte ci-dessous. |
| Ordres concurrents | Classification, lots et graines privés ; publication DSU par ordre après résolution, verticales après toutes les forêts. Les dates/parents ne deviennent pas dépendants du scheduling. |
| Unions de racines | `first` est mis à jour après chaque fusion ; ancien mutant de racine périmée refusé par code. |
| BirthRuns et préchargement | Composition des séries à la classification cohérente ; données préchargées seulement après contrôles de bornes, aucun choix géométrique modifié. |

Les nouveaux reçus sont **locaux, W4/GCC13**, malgré la consigne de tests sur
G4 ; ils ne ferment pas le contrat G4. Médianes de trois prises LiDAR,
base895680ff8/mode2047 contre ef75dafac/mode16379 :

| Trame | Base locale | Première tranche | Réduction locale |
|---|---:|---:|---:|
| 08/000000 | 7550ms | 4471ms | 40,8% |
| 08/000100 | 5731ms | 3624ms | 36,8% |
| 08/000200 | 6977ms | 4336ms | 37,9% |

Tranche479f53f0b :4662/3541/4371ms, soit+4,3%/−2,3%/+0,8% contre la première
tranche ; pas de gain FULL supplémentaire établi dans ce bruit. Les
naissances passent bien de61/52/70 à30/27/39ms, à ne pas confondre avec FULL.
Les **0,32s sur G4 sont une extrapolation**, pas une mesure. Les options,
le graphe et le retrait du mémo changent ensemble : aucun gain individuel
n’en découle sans ablation.

Les48 runs enregistrés ont les mêmes hashes par entrée, y compris les six
hashes u21 de reuse1. Les dumps sont absents : continuité des empreintes
conservées, pas rehachage indépendant des gros fichiers. Les records ne
fixent ni hash du binaire ni hash de toutes les sources/options par run.
Deux suites fast :665 et666 tests exécutés, plus une sentinelle sautée
chacune (totaux666/667).25 mutants distincts finaux tués par code ; les
survivants et le premier timeout sont conservés. Ni ASan/UBSan ni TSan
courants, profils18/24, K10 ou canonique v10 entier nouvellement acquis.

## Deux corrections utiles avant qualification

1. **Admission des verticales concurrentes sous-estimée** :
   [forest_vertical_parallel.cpp](../src/tower/forest_vertical_parallel.cpp), `vertical_images`.
   Le workspace est choisi par **ID de worker**, tandis que le précontrôle
   soustrait les premiers workspaces du nombre de tâches actives. Avec W48,
   quatre workspaces et deux blocs pris par les workers30/31, il prédit zéro
   census possédé alors que deux sont possibles. Majorant correct :
   `min(min(W,count), W−scratch_count)`, sous `scratch_count≤W`, ou affectation
   explicite aux slots réutilisables. Le cap Buffer reste actif : ce constat
   ne prouve ni dépassement réel ni corruption. Tester un scheduling à IDs
   élevés et budget serré, puis l’admission de la phase entière.
2. **Un succès PopulationLookup contourne les refus de contexte** :
   [descent_memo.cpp](../src/tower/descent_memo.cpp), `resolve_descent`, et
   `PopulationLookup::descend_each_step`. Le propriétaire de la table est
   contrôlé, mais un hit peut réussir avec mémo ou workspace d’un autre
   domaine ; la voie historique les refuse. Valider ces contextes avant le
   hit et tester hit/miss étrangers, y compris singleton. Aucun mauvais
   résultat dans le raccord FULL valide n’est identifié par cette réserve.

Suite prioritaire : ces deux corrections bornées, puis matrice G4 source
figée avec sanitizers, profils et FENV du tri ; A/B froid répété des modes
2047/16379, W1/W8/W48 et sorties canoniques entières. Relever les maxima par
phase et les compteurs réellement exécutés : visites de préfixes physiques
et popcounts de préparation des lignes vivantes (jusqu’à992 par feuille32).
`steps` inchangé exige aussi de fixer l’option mémo ; un hit population
prioritaire peut remplacer un hit mémo qui comptait zéro pas.
La simulation LPT et la somme
CPU ne garantissent pas le mur W48 ni les effets NUMA.

## Capacité et preuve restante

Pic Buffer ng00/u21/mode2047 :344267712octets, dont7657920 pour48 workspaces
census et5226784 pour le cache vertical. Ce n’est pas RSS. FULL unitaire
refuse les multiplicités malgré leur conservation dans Cloud. Taille de
coquille, nombre de boules et sorties peuvent dépasser une borne linéaire
universelle ; ces quelques trames ne qualifient pas les dizaines de millions.
K10, plusieurs séquences, GPU, projection et tête restent ouverts. Les nouveaux murs et budgets locaux ne qualifient pas les dizaines de millions.

Reproductibilité : le paquet source LIVE de reuse1 pointe vers un `/tmp`
disparu ; son hash déclaré n’est pas un rehachage. La relecture autonome
a vérifié l’archive de résultats et121 membres,30 blobs Git et les
provenances conservées. Elle ne relance ni le lecteur LIVE d’origine ni
les binaires absents. Deux erreurs de contrôle de cet audit sont gardées :
paquet absent ; tentative erronée de reconstruire I/U depuis(p,q) sans
les coquilles étendues. Aucun défaut natif n’en découle.

[Reprise historique70e et modèles](../receipts/developpement_20261003/reprise_performance/README.md) ;
[audit compact des ports479](../receipts/developpement_20261003/audit_optimisations/README.md).
Contrôles actuels : sources figées et relecture des31 chemins produit modifiés,
lecteurs des deux reçus locaux normal/−O, modèles indépendants et style349
fichiers. Aucun build/test natif ni commande cloud exécuté pour cet audit.
La fermeture graph4 précédente reste distincte ; ses résultats non rapatriés
ne deviennent pas qualifiés par les nouvelles mesures locales.
