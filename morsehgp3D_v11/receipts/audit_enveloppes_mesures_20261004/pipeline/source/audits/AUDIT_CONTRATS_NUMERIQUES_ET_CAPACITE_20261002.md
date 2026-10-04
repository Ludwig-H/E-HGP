# Audit courant v11 — contrats, performance et intégration

4 octobre 2026. Audit transversal, **deuxième lecture complète au pin e02a6c235** ;
**8f68622b2** relu ensuite, sans modification native. Sources qualifiées :
c40f40798 (FULL), b87285378 (pipeline), ab1a739d1/f1a53fe1c (banc de points).
Suivi ciblé **c22be4e41 → 2b1abb6a5** : tête E1, protocole P08, correctifs P1/P2
et réponses aux sept questions de vitesse.
Cadre : `exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`.
[Audit mathématique actif](AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md).

## Socle relu et correctifs P1/P2

Les **101 fichiers des sept modules natifs** ont une relecture de leurs
implémentations/interfaces : statuts, propriété, budgets/IDs, arithmétique,
index/census, catalogue, descentes, plateaux, parents, verticales et concurrence.
Oracles, bancs, contrats de points/tête et protocole G4 sont examinés séparément.
Les pièges anciens sont confrontés aux invariants actuels ; aucun nouveau
défaut mathématique FULL en succès n'est établi. Au pin **e02a6c235** de la contrelecture complète,
les src sont identiques à b872 ; les fondations et num/index/catalogue à c40.
[Premier audit](../receipts/audit_giant_20261004/README.md),
[contrelecture des 101 fichiers, preuves et corrections](../receipts/audit_deep_20261004/README.md).
La revue de propriété/synchronisation confirme le chemin normal : scratch
privé par tâche, publication par ordre, dépendances dirigées vers les ordres
inférieurs. À W48, le pipeline K5 réserve 39 résolveurs, cinq publieurs et
quatre suiveurs ; K10 réserve 29/10/9. Cela ne mesure pas leur occupation ni
le trafic mémoire. **5 415 gardes de modèle/source** normal/−O recoupent
propriété et progression ; elles ne constituent pas un nouveau TSan.

**P1 — corrigé dans les sources 3bd4d734e.** `await_lower` contrôle
`!low.abandoned` après la boucle ; son appelant sort avant toute lecture
de l'ordre bas. La première fixture force le réveil sur
`closed=kNone, done=false, abandoned=true` et tue causalement le mutant
qui supprime ce contrôle. **45 gardes de modèle/source**, normal/−O.
Les 256 essais avec thread existent ; chacun ne garantit pas une attente
effectivement suspendue. La porte passe dans la matrice G4 **claudequal2**,
TSan compris ; le mutant ciblé est tué.
[Correctif et portée](../receipts/audit_selfreview_20261004/README.md).
[Témoin initial](../receipts/audit_giant_20261004/tower_evidence/README.md).

**P2 — corrigé dans les sources 3bd4d734e.** Le tétraèdre régulier
`(0,0,0),(L,L,0),(L,0,L),(0,L,L)`, L=2^24−1, donne le niveau non réduit
`12L^8/16L^6` : **196/148 bits**. L'export suit maintenant le budget du
profil : quatre mots/version 2 en u24, trois mots/version 1 inchangés en
u18/u21 ; le lecteur admet les deux. **359 gardes stdlib/AST** recoupent
le décodage exact et ses limites de mots. La nouvelle porte exporte ce
tétraèdre, vérifie les coefficients bruts et cible le mutant trois mots.
La matrice G4 **claudequal2 / eb036dbe2** qualifie le correctif, profils
u21/u24 compris, et tue le mutant trois mots. Le reçu est relu normal/−O :
686 portes GCC Release, 611 ASan/UBSan et 611 TSan, sans échec ; Clang absent.
Le delta **eb036dbe2** rend la porte indépendante de NumPy : offset et mots
recoupés sur 48 buffers synthétiques, **297 gardes −S/−O −S**, sans exporteur
natif exécuté.
[Qualification G4 publiée et recoupée](../receipts/developpement_20261004/qualification_p1p2/README.md).
[Correctif et preuve portable](../receipts/audit_selfreview_20261004/README.md).
[Preuve géométrique et arithmétique](../receipts/audit_giant_20261004/geometry/README.md).

## Interopération : producteur corrigé, porte à terminer

`points_lidar_prepare.py` écrit désormais `sites_sha256` **et**
`labels_sha256` depuis **359d51a6f**, confirmé au pin d597. Le refus du
nouveau manifeste signalé précédemment est donc corrigé côté producteur.
Garder la validation stricte de `points_unpack.py` et terminer la porte
ancienne/nouvelle archive. Le reçu initial conserve ses **11 gardes AST**
sur l'ancien producteur ; il ne décrit plus le défaut actuel.
[Sources épinglées et test causal](../receipts/unpack_manifest_review_20261004/README.md).

## Réponses R1–R7 : réduire le travail vers 100 ms

Répond à la [question du développeur d597](QUESTION_CLAUDE_VITESSE_100MS_20261004.md).
Les trois budgets ~3 CPU·s et ~50/~50 ms sont des objectifs conditionnels au
parallélisme observé, pas des bornes physiques. CPU/mur peut changer avec
l'architecture ; mesurer même périmètre FULL, mur, CPU et chemin critique.

1. **Compteurs locaux : oui.** Borner la feuille réelle **m≤1024**, pas
   seulement `leaf_size=32` : une feuille terminale peut être plus grande.
   Préfixes ≤Σ(q=1..4) C(m,q) ; census/incidences ≤mΣ(q=2..4) C(m,q)<2^46.
   Flush `checked_add` explicite avant publication, réduction checked
   conservée ; ledger identique, contacts et préfixes logiques compris.
   Cette preuve ne couvre pas les filtres de nœuds ni le census global.
   `side` peut être total en interne seulement avec un certificat couvrant
   ses coefficients/sites et tous ses replis ; **m≤32 ne le certifie pas**.
   Portes : m32/33/256/1024, compteur global proche u64max, refus sans sortie.
2. **q3 différé : tous les champs actuels de `CatalogueLedger` restent
   identiques**, dont `judged`, `census_tests`, `prefixes`, les `region_*`
   et `q4_candidates/q4_levels`. `census_tests` compte même les contacts du
   support sans appel de puissance. Ajouter des diagnostics séparés de
   constructions/niveaux q3 et rejets par étage ; ne pas redéfinir l'ancien
   ledger. MEB garde aussi ses sept compteurs logiques et son support.
3. **Arène : oui, réservation effective budgétée.** Un Buffer privé par
   tâche active, marque/rewind DFS, alignement et vies des vues maîtrisés.
   `count*(3B−depth)` borne les listes simultanées du suffixe, **pas tous
   les nœuds ni les émissions**. W blocs actifs : W plus grands majorants ;
   tous les blocs préalloués : somme de tous. Frontier, workspace et sorties
   coexistent dans le budget. Publier le pic mesuré des réservations, même
   surdimensionnées, distinct des octets utiles/RSS ; `admit` seul ne réserve
   rien. Portes : plafond/−1, panne d'allocation, abandon, retour au budget
   préexistant ; zéro après destruction de tous les propriétaires.
4. **Census : d'abord q2 couplé, puis ablation de partition.** L'index Morton
   possède déjà des boîtes entières serrées ; k-d changerait sa partition et
   son parcours. Rejouer les mêmes requêtes, séparer owned/workspace, payer
   construction/mémoire et préserver les sorties I/U contractuelles et FULL.
   F6≈1 % ne démontre pas un goulet mémoire. Tout certificat local doit aussi
   couvrir les sites/boîtes interrogés hors de sa feuille ; bande et égalité
   restent exactes. Aucun nouveau défaut d'index ni gain k-d présumé.
5. **Mémo cellulaire : oui comme certificat typé, pas comme faux résultat
   de descente.** Même propriétaire et domaine, ordre k, |R|=k et
   **R⊆P_b complet** : toutes ces parties partagent la composante au niveau λ_b. Un semis de l'une,
   remonté à la coupe demandée, est valable pour a≥λ_b fermé / a>λ_b ouvert.
   Pour le prédécesseur strict d'un plateau λ, exiger **λ_b<λ** ; aux
   verticales fermées, ≤ suffit. La date terminale du cache n'est pas cette
   date de validité. Conserver `DescentMemo` par tuple complet pour graines
   et deux Level bruts identiques ; partage concurrent à publier séparément.
6. **T6 : mesurer d'abord, puis versionner le domaine des boîtes.** Pour
   Q=2^T, la transposition directe QN+D(Qa−L) exige **5B+6+T≤127** ;
   le réservoir mis à l'échelle exige **2(B+T)+5≤63**. B24/T6 échoue à
   ces gardes directes, mais cela n'interdit pas T6. Garder les points en B :
   G1 a un majorant 2B+T+5, J2 affine/SAT 2B+T+4 / 3B+T+5. Pour le centre,
   écrire L=Qℓ+r, 0≤r<Q, D>0, E=N+D(a−ℓ). Si E<0 ou E≥D,
   le signe est immédiat ; sinon comparer **QE à Dr**, produits de budget **4B+5+T**. En u24/T6, E reste
   à 126 bits et ces produits à 107 : i128 suffit pour cette reformulation.
   Sur ces bornes, le réservoir garde un majorant de 65 bits à élargir.
   **6 219 gardes Fraction** vérifient cette proposition, pas un port ni un gain.
   Revoir factories `CenterRegion`, enveloppes, coupes, profondeur 3(B+T)
   et bornes d'arène ; une sous-maille par profil peut alors garder des
   certificats prouvés. XYZ/grille d'entrée inchangés, contacts fermés et
   propriétaire demi-ouvert conservés.
7. **GPU : voie autorisée, preuve et route entières.** Centres i128 ne
   signifient pas catalogue i128 : q3 conserve checked/Wide, puissance
   134/152 bits et niveaux jusqu'à 180/134 ou 204/152 en u21/u24. Exact
   device, ou `unresolved` repris exactement sur CPU **avant admission** ;
   un débordement/refus n'est jamais un rejet géométrique. Garder S*, ordre,
   contacts, sentinelles, baux/epochs et refus transactionnels. Budgéter
   ensemble host/pinned/device et leurs coexistences. Vrai nvcc, portes CPU/
   device puis FULL identique ; chronométrer préparation/transferts/retour/
   canonicalisation. Le piège C6 v6 concerne cette route, pas un temps FULL
   transférable ni une interdiction du GPU.

Ces conditions permettent les prototypes ciblés, sans nouveau feu vert
utilisateur ni dossier de dialogue. Mesurer chaque changement séparément,
puis leur combinaison. [Sources, majorants et certificat cellulaire](../receipts/audit_selfreview_20261004/README.md).

## Ce que les mesures G4 prouvent

La tour FULL native est implémentée : catalogue, census/index, descentes MEB,
naissances et multifusions atomiques, parents et verticales. Les défauts de
propriétaire avant mémo, admission des census, tri/FENV et verdict du banc
sont corrigés et testés. Ce sont des acquis, plus des réserves courantes.

| FULL CPU K1..5/W48, ms | 08/000000 | 08/000100 | 08/000200 |
|---|---:|---:|---:|
| v10 777406b82, u18, troisième passe chaude | 252,0 | 204,2 | 253,6 |
| v11 c40, u21, médiane de trois prises | 489,1 | 345,1 | 432,4 |
| v11 voie b872/claudeab7, u21, médiane de cinq prises | 412,4 | 351,7 | 380,7 |

**Les octets XYZ sont identiques** : grille 1 mm, même masque sans sol,
39 885/35 551/45 845 sites unitaires, aucune réduction supplémentaire.
u18/u21 désigne ici une capacité arithmétique compilée, pas une résolution
d'entrée différente. Les cardinalités du catalogue et des cinq forêts sont
égales ; le différentiel canonique intégral v10/v11 sur ces trames reste
à fermer, leurs IDs/formats différant. Le juge v10 incomplet limite sa
qualification ; il ne prouve pas que sa vitesse provient d'une tour omise.

L'écart actuel observé vaut **×1,50–1,72**, contre environ ×6 avant les
optimisations. Ce rapport entre captures séparées n'est pas un A/B causal :
processus neufs v11 contre troisième passe chaude v10. Les premières passes
v10, déjà préparées, donnent aussi 259,8/218,6/265,5 ms. Index v11 0,35–0,41 ms
et préparation Cloud/Pool hors FULL ne sont pas les postes principaux.
Les dumps, IO et segmentation sont également hors FULL.
[Sources, mêmes XYZ, cardinalités et calculs](../receipts/audit_deep_20261004/performance/README.md).

**Pourquoi l'ancien retard ?** Les mesures appariées v11 montrent le gain
c40 ×2,68–3,28 face à la **baseline v11** 895680ff8, sorties complètes égales.
Le semis par population avant MEB évite de nombreuses descentes :
15,70/12,61/15,62 millions de présentations deviennent 3,79/2,89/3,31 millions
entre modes2047 et16379. Filtrage des préfixes, répartition des tâches,
ordres concurrents, graines verticales et pipeline réduisent aussi le travail
ou les attentes. Les gains sont ceux de paquets de changements ; aucune part
chronométrique n'est attribuée à un mécanisme isolé sans ablation.
Le diagnostic historique catalogue mono sur les mêmes XYZ donne seulement
+5–6 % de u18 à u21 ; il ne qualifie pas le FULL actuel ni ses autres profils.

**Pourquoi un écart subsiste ?** Les deux postes globaux restent lourds :
domaine/catalogue+rangs/lookup b872 253/219/222 ms, forêt+verticales
171/133/158 ms ; v10 catalogue 164/137/164 ms, forêt 89/67/89 ms.
Les médianes par phase ne s'additionnent pas. Le pipeline mesure un délai
jusqu'à la dernière résolution, puis ses queues de publication/verticales ;
une somme de tâches n'est pas un coût CPU. Deux pistes précises sont relues :

- **q3 calculé trop tôt** : la v11 construit le Level de degré6 avant le rejet
  propriétaire/census/canon ; la v10 le diffère après admission. Une feuille
  accessible rejette un triple aigu après avoir construit `3000/464`.
  Différer cette même formule, encodage **non réduit par PGCD** inchangé,
  préserve les contrats sous requalification. Ce travail évitable est établi,
  sa fraction dans les temps LiDAR n'est pas mesurée.
- **Partition des centres différente** : arrêt v11 à largeur1 contre seuil
  possible1/64 de maille v10. Cela change les listes et candidats, sans
  changer les XYZ ni leur précision. Tester ce paramètre après revue des
  bornes ; copier T6 vers u24 échoue au garde i64 `2*(24+6)+5<=63`.
  Une feuille v11 est énumérée intégralement ou refuse `wide_leaf`.

[Rejeu q3 et comptabilité des routes](../receipts/audit_deep_20261004/geometry/README.md),
[précision sur les niveaux et subdivisions](../receipts/audit_deep_20261004/performance_precision/README.md).
Un A/B G4 v10/v11 aux mêmes profils, W1/24/48 et sorties canoniques, puis
ces ablations séparées, permettrait d'attribuer l'écart restant. Ni NUMA,
ni bande passante, ni surcoût Wide global ne sont démontrés par les captures.
Pics b872 Buffer+Cloud : 371,4/320,5/395,7 Mo ; ce ne sont pas des RSS.

c40 : 4 073 portes, 326 mutants, 81 prises appariées ; b872 : 666 portes,
sept TSan, 11 mutants, 36 prises appariées. Trois trames de la même séquence,
profil u21 pour ces chronos ; portes des autres profils distinctes.
[Qualification](../receipts/qualification_performance_20261003/README.md),
[pipeline](../receipts/developpement_20261003/pipeline_g4/README.md).
**100/200 ms, GPU, temps sur plusieurs séquences, massif et points natifs
restent ouverts.** Aucun nouveau chrono natif dans cette contrelecture.

## Idées anciennes retenues pour la v11

Revue des modèles et mécanismes **v1–v10**, confrontés au pin **4fac50118** :
deux reprises concrètes seulement. Sources et preuves sont épinglées dans
le [reçu ciblé](../receipts/audit_heritage_20261004/README.md).

**1. Un candidat q3 différé commun au catalogue et aux MEB.** La v10 diffère
le Level du catalogue ; la v7 `anchor_meb.hpp` garde aussi forme/puissance
avant de matérialiser la MEB acceptée. Étendre la piste q3 ci-dessus à
`tower/meb.cpp` : le tétraèdre régulier entier à coordonnées 0/2 y provoque
quatre triangles stricts rejetés par inclusion, donc **quatre Level jetés**.
Le modèle différé les évite, mêmes gagnant q4, support, six présentations et
17 tests. **2 071 contrôles exacts, 80 parties**, normal/−O.

Conserver N/D, tag 3, certificats puissance/orientation et replis checked/Wide ;
matérialiser le **même Level brut de degré 6**, sans importer le PGCD v7.
Les témoins u21/u24 exigent des puissances de 128/146 bits : retaguer q4 pour
réemployer son candidat serait dangereux. Une primitive privée partagée,
sans second moteur ni changement de recherche/support canonique.
[Preuve et conditions de port](../receipts/audit_heritage_20261004/q3_deferred/README.md).

**2. Les extrema q2 couplés et préparés de la v8/v9.** Pour une présentation
q2 certifiée, poser C=a+b, S=|b−a|² : **2P(z)=Σ(2z_j−C_j)²−S**.
Les extrema continus exacts par axe utilisent proche/loin ; i64 suffit même
en u24. Une boîte intérieure a une borne supérieure actuelle P=142 contre
une borne couplée 2P=−36 : certificat plus fort, sans rayon ni division.
Le modèle à 17 sites conserve
I/U, les six contacts et la saturation aux quatre seuils ; **633 gardes**
normal/−O. Les bornes visitées passent 17→13 dans ce cas, les tests ponctuels
restent 8 : aucun chrono natif déduit.

Ajouter un **helper/préparateur de census distinct**, aux parcours possédé et
emprunté ; conserver `power_bounds` et `power_bound_signs`, dont le contrat
public lie les signes aux mêmes bornes. Ne pas y substituer 2P ni retyper un
q3/q4 à qmin=2. Préparer C/S une fois par requête ; aucun nouveau Cloud ni
tableau proportionnel au nuage. Le scan des feuilles du catalogue ne serait
pas accéléré directement. [Contrat et contre-garde d'API](../receipts/audit_heritage_20261004/q2_coupled/README.md).

Ces deux reprises justifient un port ciblé avec portes G4 et ablation FULL,
pas une promesse de gain ni une qualification héritée. Mesurer constructions
q3 évitées, parcours q2 par arité et coût total ; comparer les sorties entières.

## Banc exact FULL → points

F/claudepts6 joue f02f91c7e, sources identiques à ab1a : **export FULL C++
CPU/u21**, consommateur rayon 457 et oracle 2f05 **Python**. Porte conforme
sur 2 854 nuages, 194 520 comparaisons, 215 974 comparaisons de sites répétées,
12 fixtures et quatre mutants **Python**, exporteur natif inchangé.
Domaine n≤9, k≤4, m≤n ; le propriétaire au plateau algébrique est désormais
exercé. Aucun port natif PointRadiusDate/K10/u24 n'est qualifié par cela.
[Lecture indépendante : 4 343 contrôles](../receipts/points_gate_qualification_20261004/README.md).
E conserve son refus de dossier manquant ; E/F sont fermées, arrêts certifiés.

F termine 205 cas : 128 synthétiques, cinq démos, 72 voisines à k2/3/5/10.
Une voisine duplique la démo02 : **71 scènes distinctes/859 observations
d'instances corrélées**, séquence08 uniquement. Tailles 32 462–126 267 sites,
sol conservé en démo04. Les 201 JSON communs D/F sont égaux hors quatre
champs de temps ; pas toutes les dates/propriétaires internes. Le lot mesure
le **meilleur bloc**, pas une sélection plate. [Archives D](../receipts/pts4_review_20261003/README.md).
m>n refuse actuellement : déclarer m≤n ou des points inactifs.

**Actualisation Zoltan 8f68622b2.** Deux sessions closes sont relues :
claudebouts1, 360 bouts/10 séquences ; claudebouts2, **31 bouts + cinq démos**,
avec membres de blocs publiés. Portes 2 835/2 863 nuages, 12 fixtures et quatre
mutants par lot ; extraction sans écarts, arrêt ciblé certifié. La nouvelle
classification exclusive du lot1 compte 13 cas HGP seul réussi, trois cas
HDBSCAN seul réussi, 11 échecs communs et 333 réussites communes. L'ancien
compte cinq cas inverses était un diagnostic « à au moins un k » : deux
réussissent aussi HGP à un autre k. Ne pas mélanger ces conventions.

Sur les deux vélos `b00_001470_velos_43_61`, à k5, les meilleurs blocs HGP
133/83 sites sont **disjoints**, IoU0,964/0,711 contre HDBSCAN0,819/0,482.
Ils peuvent former une antichaîne ; la sélection EOM et un niveau de coupe
commun restent à vérifier. Les lots mesurent des meilleurs blocs sur des
extraits choisis par annotations, pas le contrat de trame entière.
[Lecture des sessions et témoin](../receipts/audit_deep_20261004/README.md).

## Contrat natif encore à construire

**Tête E1 effectivement livrée, portée Python.** Les sessions
claudeflat1a/1b confrontent export FULL C++ → projection, condensation et
sélection **Python** à l'oracle indépendant ; leurs neuf mutants sont
Python. Le reçu développeur dit « sortie plate en natif » : le lieu G4
et l'export natif ne qualifient pas une tête C++, toujours absente.
Le dev synthétique a fixé **z=2** sur 192 scènes ; le protocole P08 distingue
z=2 pour les fusions et z=1 pour la non-infériorité IoU. Ces observations
et choix sont distincts des tests encore à venir.

**Avant les mesures primaires z=2, compléter la porte.** `points_flat_gate.py`
compare actuellement EOM z=1, z=3 et feuilles, pas z=2. Ajouter ce bras contre
le même oracle et ses gardes algébriques, puis rejouer sur G4 ; l'exercice
du dev z=2 ne remplace pas cette comparaison de correction.

**Avant P08, appliquer tout le critère H_L2.** Le préenregistrement exige
la borne basse IC95% **strictement > −0,02**, en plus de Holm. Le verdict
`lidar_decision` n'utilise actuellement que `p_holm<0,05`. Le helper source,
avec bootstrap injecté, rend `claimed=True` pour p_holm=0,03010 et
IC=[−0,021;−0,019] ; l'égalité à −0,02 est également admise à tort.
Ajouter la condition de borne à H_L2 seulement, garder p/IC séparés et
une fixture à la frontière. **89 gardes stdlib/AST**, normal/−O ; aucune
campagne P08 exécutée ou jugée. [Témoin causal et conditions](../receipts/audit_selfreview_20261004/README.md).

Arbre de points N-aire, après suppression des vides/unaires : **≤2n−1 nœuds**.
Le produire depuis FULL et les attaches, sans matrice n² ni liste de membres
par ancêtre. DP : score/décision par cluster, puis un passage d'émission des
labels. Compter ensemble arbre, dates, scores, scratch, IDs/labels et FULL.
Le catalogue et les coquilles n'ont pas de borne linéaire universelle.
Le modèle compact existe en Python E1 ; sa construction native reste un **plan**.

PointRadiusDate : trois rangs, égalité algébrique, ordre commun avec FULL,
coupes fermées, refus transactionnels. Majorants u18/u21/u24 : niveaux
156/116, 180/134, 204/152 bits ; produits de comparaison quatre racines
2022/2334/2646 bits. Wide2048 ne couvre pas le majorant u21 complet.
Six racines : zéro par classes carrées ; budgets suffisants conservateurs
45996/53097/60198 bits, pas une prévision de coût. 8192 bits est un budget
avec refus. Ces majorants ne sont pas revendiqués atteints par un Cloud ;
le tétraèdre u24 ci-dessus établit séparément le défaut concret d'export.
[Contrat détaillé Q8](../receipts/points_answers_20261003/root/Q8_CONTRAT.md).

La tête EOM exige aussi signe/égalité/refus pour ses réciproques ; l'aide
algébrique nouvelle certifie le zéro sans prouver le budget natif rapide.
[Preuve et gardes](../receipts/eom_exact_audit_20261004/README.md).
Conserver HDBSCAN officiel séparé du bras N-aire commun. Le plafond B(H)
dépend de l'univers de blocs, pas seulement du nom de l'algorithme.
[État des rapports et témoin F2](../receipts/flat_evidence_followup_20261004/README.md).
Le dev et les préenregistrements ne sont pas les résultats confirmatoires
P08/synthétiques, ni une décision finale transférable au produit.

Cette contrelecture utilise sources figées et Python borné normal/−O,
**aucun fit, build/test natif ni GCP**. Documentation de cette publication
contrôlée séparément : le contrôleur global exclut v11 et garde ses
213 liens v10 préexistants en échec. Les anciennes notes sont archivées
intactes ; aucun reçu clos ni travail d'un autre acteur n'est réécrit.
