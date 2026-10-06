# Audit courant v11 — contrats, performance et intégration

6 octobre 2026. Contrat S0/S1 **5adf6a59f**, demande **9290cf3bf** et
tranches natives S3/S5/S6/S7 relues ; S6a publiée en **19b2fb218**, S3 en
**165def5ab**, S5/S6b/S7 publiées jusqu'à **966a351be**, qualification G4
ordinaire conforme au pin **b319efc84**. S8 relue en **53c027fe8**,
sans transfert de cette qualification antérieure.
Le socle qualifié et chaque capture CPU/GPU gardent leur
source et leur domaine propres dans les reçus liés ci-dessous. Cadre :
`exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`.
[Audit mathématique actif](AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md).

## Suite proposée : isoler la répartition des feuilles du fill

[Patch et preuve de couverture](../receipts/proposition_fill_cta_20261006/README.md),
base **830473218**, sans modifier le produit. Le fill actuel lance un bloc
pour 14 feuilles à rejouer, 132 pour 4 196. La proposition lance un bloc par
feuille, avec un seul fil actif, puis une boucle u64 si la limite de grille
exige plusieurs feuilles par bloc. Les positions restent disjointes et
complètes ; les préfixes, `run_leaf`, les vérifications de fin et les noyaux
count/copy restent conservés. Les 60 cas bornés et douze frontières larges
passent normal/−O ; le patch est applicable, sans compilation ni gain prétendu.

La mesure motive cet essai limité : dernière passe chaude K10/ng00,
fill GPU en série **111,4 ms**, recouvert **1 564,4 ms** ; le count passe
séparément de **215,3 à 770,6 ms**. Le supplément du fill représente environ
72 % du supplément de l’exécuteur dans chacune des trois dernières prises
K10. À K5, c’est surtout le count qui augmente : ne pas présenter un meilleur
fill comme une solution complète au recouvrement ou aux 100 ms.
Les six comparaisons sont conservées dans le
[reçu L4](../receipts/audit_g4_l4_20261006/README.md).

Comparer sur G4 une référence830 et le candidat patché, mêmes paramètres,
avec dumps et registres identiques, refus contrôlés et branches copied/fill
exercées. Mesurer fill puis domaine/FULL avant d’adopter. Ce petit essai
permet de juger le placement avant une réécriture coopérative de `run_leaf` ;
il n’annonce pas qu’une feuille par bloc sera plus rapide.

## L4 clos : rejet mesuré, défauts supprimés par le retrait

La session `v11.20261006.claudeL4`, source **cf28afb04**, est close avec
arrêt certifié à **07:29:37 UTC**. Les deux lots K5/K10 donnent `conforme` :
**90 processus, 252 passes, 90 dumps contrôlés**, trois trames entières.
Les 162 passes intermédiaires n’ont qu’un statut, sans dump ni registre
propre conservé. Les registres des sorties conservées sont jugés égaux par
le banc épinglé ; ses lignes natives complètes ne sont pas archivées.
Aucun sanitizer n’est qualifié par ce plan.
[Rejeu indépendant normal/−O](../receipts/audit_g4_l4_20261006/README.md).

Les critères de gain échouent en K5 et K10 sur chaque trame ; les intervalles
de domaine résident sont entièrement séparés en défaveur du recouvert.
Le retrait **830473218** est étayé. Il restitue exactement les sources
précédant N1/L4 et clôt par suppression le défaut de durée de vie sur refus
ainsi que la garde globale des sous-lots. Les succès G4 n’avaient pas
qualifié ces chemins de refus. La
[preuve et le patch historiques](../receipts/audit_overlap_20261006/README.md)
restent épinglés à cf28 ; ne pas appliquer ce patch au moteur revenu à sa base.
Aucune nouvelle campagne sur cette version retirée n’est demandée.

## N1 clos : dimensionnement intégré, puis optimisation rejetée sur G4

Le conseil de dimensionnement **8ee28873f** a été intégré en **c72c5a576** :
arène adaptée au plus gros suffixe, zéro pour le scratch de repli des feuilles.
La session `v11.20261006.claudeN1` s’est arrêtée avec certification à
06:32:58 UTC. Sources et archive de base concordent avec Git ; **78 prises**
CPU FULL/K5/u21, trois trames, rendent les dumps et registres attendus.
Les **39 prises N1 ont zéro repli** ; **145 portes PASS**, deux mutants détectés.
Le banc garde son statut `refus` pour son plancher erroné de 150 portes.
[Relecture indépendante du reçu et rejeu normal/−O](../receipts/audit_g4_n1_20261006/README.md).

Le critère de gain préalable échoue sur chaque trame ; le rejet et le retrait
**c1675e4c9** sont étayés. Aucune nouvelle campagne N1 n’est demandée.
Le diagnostic exclusif SMT n’est pas établi par cette expérience ; les
horloges par tâche/feuille du plan n’ont pas été capturées. Cette limite
n’empêche pas de poursuivre le recouvrement GPU. Les anciens témoins et
conseils restent dans le [reçu du plan](../receipts/audit_plan_gpu_20261006/README.md),
avec leur source propre, sans demeurer des corrections ouvertes du moteur.

## Erratum df904711a adopté

La section L de la [réponse du développeur](REPONSE_CLAUDE_SUPPORTS_20261004.md)
adopte les trois corrections : trois trames LiDAR, L35/41 dont 28 portes
fonctionnelles et sept campagnes mutants, R3 base u18 avec options locales.
Les six campagnes L/u21 non jugées ne sont pas requalifiées implicitement.
Les pièces historiques et leurs empreintes sont conservées ; ce point est clos.

## R4 clos : complément ASan/u24 conforme, reprise ciblée terminée

La session `v11.20261005.clauderepriser4`, source **98a009550**,
est close le 6 octobre à 01:15 UTC avec arrêt ciblé certifié. Les
quatre lots terminent **80/80 portes PASS**, sans échec, absence ni
coupure. Leur union est disjointe et exactement égale aux 80 noms
ASan du reçu fins ; inventaires, verdicts et JUnit concordent.

Les **six supports_route corrigées passent sous ASan/UBSan u24**.
Leurs lignes natives concordent avec les références par profil,
journaux et workers ; les pics publics suivent bien la voie FULL.
Les drapeaux sanitizer sont vérifiés dans la bibliothèque, la sonde
et son lien. Aucun marqueur sanitizer n’apparaît dans les 16 journaux
d’exécution conservés ; les quatre journaux sanitizer sont vides.
Les six portes points, six plat et six racines d’échelle/LiDAR passent
aussi. [Reçu R4](../receipts/audit_g4_repriser4_20261005/README.md),
[contrelecture native](../receipts/audit_g4_repriser4_20261005/NATIVE_REPLAY.md).

La qualification ciblée est terminée dans les pins et profils propres
à chaque reçu. R1/R2 couvrent exactement les **3 695 noms ordinaires**
de l’inventaire fina2, tous PASS ; R3 ferme les mutants API/CLI et R4
les anciens refus ASan. Les portes longues, TSan et différentiels
P9/P10 gardent leur pin 38b76701b ; les reprises gardent 98a009550.
Les six campagnes L/u21 sans résultat restent historiques, sans
transfert depuis R3. Le contrat de 100 ms reste ouvert.
[Rejeu du raccord ordinaire R1/R2](../receipts/audit_g4_repriser4_20261005/ordinary_union/README.md).

## R3 clos : tous les mutants détectés, API et CLI requalifiés

La session `v11.20261005.clauderepriser3`, source **98a009550**,
est close le 6 octobre à 00:47 UTC avec arrêt ciblé certifié. Les
**39/39 CTests** passent et les journaux gardent les **485/485 verdicts
individuels détectés** : 479 par code, quatre par ligne et deux refus
de construction attendus. Aucun survivant, témoin invalide, signal,
délai ou individu non jugé n’est compté comme réussite.

Les **23/23 API** auparavant non jugés et les **28/28 CLI** sont
détectés par code. L’ancien survivant `sp_masque_16379` est désormais
atteint par la porte points. Les sorties détaillées des variants tués
ne sont pas conservées : la catégorie `code` est observée, tandis que
le refus `parameter_out_of_range` attendu sur cette mutation se déduit
du chemin source épinglé. La validité des témoins non mutés est imposée
par le runner avant les verdicts, sans inventer de lignes absentes.

Le profil est une base u18 avec options locales déclarées, détaillées
dans la section reprise. R3 ne qualifie pas en u21 les six campagnes
interrompues de l’ancien lot L. R4 ferme séparément le complément
des routes et autres portes d’échelle ASan/UBSan u24.
[Reçu R3, manifestes et verdicts](../receipts/audit_g4_repriser3_20261005/README.md),
[contrelecture API/CLI](../receipts/audit_g4_repriser3_20261005/NATIVE_REPLAY.md).

## R2 clos : les 41 absences et les références multiprofil passent

La session `v11.20261005.clauderepriser2`, source **98a009550**,
est close le 6 octobre à 00:14 UTC avec arrêt ciblé certifié. Les
huit lots d’échelle terminent tous leurs inventaires : **472/472
PASS**, zéro échec, résultat manquant ou coupure. Les deux lots de
chaque profil sont disjoints et couvrent 52 + 66 noms ; sources,
profils, journaux et JUnit concordent.

La comparaison avec l’archive fina2 vérifie nom par nom que ses
**41 anciennes absences** (8/11/11/11) ont toutes un PASS explicite.
Les **24 supports_route** passent également, dont les douze portes
u18/u24 précédemment refusées sur des attendus u21. Les nouvelles
références par profil sont donc qualifiées dans cette reprise Release ;
aucun échec historique n’est transformé rétroactivement. Les 24 lignes
natives concordent exactement avec les attendus, journaux et workers.
Dans chaque cas, le pic de l’appel public `compute` égale celui de FULL
et diffère de l’arbre K seul ; les identités complètes restent imposées
par la sonde, au-delà des seuls préfixes de SHA publiés.
[Contrelecture ciblée et rejeu indépendant](../receipts/audit_g4_repriser2_20261005/NATIVE_REPLAY.md).

R2 clôt le volet ordinaire d’échelle et LiDAR. Les campagnes mutants
API/CLI passent séparément dans R3 ; les six
routes ASan/UBSan u24 passent dans R4. Aucun chrono de contrat n’est
inféré de ces portes de correction.
[Reçu R2 et correspondance des 41 noms](../receipts/audit_g4_repriser2_20261005/README.md).

## R1 clos : les quatre profils courts passent au pin corrigé

La session `v11.20261005.clauderepriser1`, source **98a009550**,
termine avec succès et arrêt ciblé certifié. Les sources utiles du
paquet correspondent exactement au pin ; inventaires, journaux et
JUnit concordent : **873/873 u18, 783/783 u21, 783/783 u24 et 784/784
poison u21**. Aucun échec, résultat manquant ni coupure de budget.

Les contrôles S9 du refus de tri et S10 de la racine frontière passent
explicitement dans les quatre profils, ainsi que les sorties points/plat,
les racines exactes, les sessions/provenances et l’oracle API supports.
Le lot court `gcc_release` est réellement conforme après suppression
de l’exigence LiDAR impossible : la preuve source est complétée par R1.

Cette sélection exclut les échelles, LiDAR, longs et mutants. Elle ne
ferme ni les 41 absences antérieures ni les six supports_route par
profil : R2 les ferme séparément, R3 juge les mutants et R4 les routes sous
ASan/UBSan u24. Aucun contrat de temps ne découle de ces portes courtes.
[Reçu R1, inventaires et portée](../receipts/audit_g4_repriser1_20261005/README.md),
[contrelecture indépendante des flags et portes prioritaires](../receipts/audit_g4_repriser1_20261005/native_review.json).

## Matrice G4 : exigence LiDAR impossible corrigée en 98a009550

La correction publiée retire `require_labels_if_data=["lidar"]`
du seul lot court `gcc_release`, qui exclut ces tests. Les deux lots
d’échelle conservent leurs véritables portes et planchers LiDAR.
Le refus artificiel signalé dans le WIP est donc corrigé en source,
sans affaiblir les contrôles des lots qui traitent effectivement LiDAR.

L’union des lots d’échelle couvre les 41 occurrences ordinaires
absentes et les six identités supports de chaque profil ; le lot L
exclut désormais les mutants. Cela valide la sélection, pas l’exécution
future ni son délai. Le témoin construit à 873 PASS, qui faisait
échouer le juge sur ce seul label, reste archivé avec sa contre-épreuve.
[Constat initial et correction minimale](../receipts/audit_g4_matrix_reprise_wip_20261005/README.md).

## Correction API/CLI : gardes restaurées en be05bfad8

Relecture du correctif publié **be05bfad8**, conservé en **98a009550**.
**La perte de gardes du premier WIP est réparée.**
`supports_route.cpp` revient exactement à la source
publiée **38b76701b** : verdict avec journal, fichiers et manifestes,
identités entre voies, workers et appel public conservés.

`tests.cmake` choisit désormais les empreintes fichier/manifeste
pour chacun des profils u18/u21/u24. Les six références u21 et tous
les journaux sont conservés ; les six paires u24 correspondent aux
sorties G4 de fins et la paire u18/8k à finm. Les autres nouvelles
références u18 passent désormais dans R2 au commit corrigé. Cette solution
sur une seule ligne rend inutile le montage multiligne proposé à
l’étape précédente. La capture initiale reste une preuve historique,
pas un défaut imputé à la version publiée.

**Raccord CLI toujours favorable** : mutation et plancher de 28
conservés, porte points atteignant le paramétrage modifié. Le rejeu
R3 confirme désormais les 23 API et les 28 CLI au commit corrigé.
[Correction publiée, treize sorties historiques recoupées et modèles rejoués](../receipts/audit_gates_fix_closed_20261005/README.md).
La [capture initiale](../receipts/audit_gates_fix_wip_20261005/README.md)
reste figée pour retracer le défaut de la première proposition.

## Banc final après L2b : décision historique désactivée en be05bfad8

Au pin **38b76701b**, `sorties_g4.py` appelle seulement `full` et
`supports`, tous deux au masque 16379. Le chemin public supports
construit désormais les forêts FULL avec son journal puis en extrait
K. Aucune variante `order_tree`/7035, points ou plat n’est mesurée.
Pourtant `decide()` pouvait encore émettre `build_order_par_defaut` ou
`livrer_L2b`. Ce verdict ne peut plus décider entre deux algorithmes,
puisque la voie K seule ne figure plus dans les appels du banc.

Le correctif publié **be05bfad8** désactive ce choix : `decide()` rend
`sans_objet_post_l2b`, avec absence de voie `order_tree` mesurée. Les
contrôles d’identité et le retour non nul en cas de défaut sont
conservés. Avis favorable sur cette correction ; une nouvelle décision
sur la voie K seule exigerait une variante explicitement mesurée.
Les valeurs actuelles restent utilisables comme FULL versus sortie
supports/L2b : le reçu clos est désormais vérifié et aucun nouveau run
n’est demandé pour ce périmètre. Le modèle source joint démontre la branche
erronément nommée avec de simples booléens, sans argument statistique.

Le plan de finmesure passe aussi `--commit b319efc84`, que le banc
copie sans vérification dans `provenance.commit`. Le préflight épingle
réellement **38b76701b**. Corriger l’étiquette du banc et attribuer
l’analyse au pin vérifié du reçu, en conservant l’annotation originale
comme erreur de métadonnée ; ne pas réécrire le reçu brut.
Pour les prochaines captures, le worker fournit déjà le commit complet
réel dans `V11_SOURCE_PIN=commit:<SHA>`. Le banc peut lire cette variable
et en tirer `provenance.commit`, avec `--commit` comme déclaration locale
hors session. Le plan ne substitue pas les variables shell dans ses
arguments : retirer son ancien `--commit b319efc84`, ou y passer le même
SHA final qu’au contrôleur. Aucune nouvelle infrastructure nécessaire.
[Sources du worker et correction proposée](../receipts/audit_gates_fix_closed_20261005/matrix/matrix_correction.json).
[Chaînes d’appels, plan et modèle borné](../receipts/audit_finmesure_scope_20261005/README.md).

## Mesure finale close : l’étage tree dépasse toujours 100 ms

La session `v11.20261005.claudefinmesure` termine avec succès au pin
**38b76701b**, avec arrêt ciblé certifié. Les 52 appels mesurés sont
complets et concordants : 48 K5 sur les trois trames sans sol, W1/W48,
FULL/supports ; quatre K10 descriptifs sur ng00 à W48. Les empreintes
enregistrées concordent entre prises et workers, et l’arbre K est
commun aux deux sorties. Les fichiers temporaires étant supprimés
par le banc, cette relecture porte sur les empreintes conservées.

Les deux contrôles supports supplémentaires ng00/ng02, W1/W4/W48,
rendent directement `conforme`, chacun avec 14 appels et 52 contrôles.
En revanche, **les 18 prises chaudes K5/W48 dépassent toutes 100 ms
pour l’étage `tree`**, donc aussi pour le total. Cet étage construit
les forêts 1..5 et les verticales, hors catalogue (`domain`), rattachement
et sorties ; son mur inclut les préparations/allocation du constructeur
et se distingue du compteur interne `forest_ns`. Le réduire reste
nécessaire pour atteindre le contrat. L’arbre de points et la tête plate
relèvent de `output` et ne sont pas mesurés ici. Les deux annotations
obsolètes du banc sont bien présentes dans le reçu et sont écartées,
sans altérer ses données brutes ni ses mesures utiles.
[Reçu clos, valeurs individuelles et relecture](../receipts/audit_g4_finmesure_20261005/README.md).

## P9 : comparaison complète de l’arbre de points conforme

La session `v11.20261005.claudefinp9` est close au même pin
**38b76701b**, avec arrêt ciblé certifié. Les quatre CTests passent :
cas synthétiques K1..5, puis les trois trames entières K5 en Release u21.
Le juge compare tous les sites, leurs dates et rattachements, les
plateaux, les parents et les entrées de blocs avec la construction
Python, sur le même catalogue et la même tour FULL exportés. P9 complète les quatre portes de tête P10 ; les huit
comparaisons dédiées sont désormais terminées avec succès.

Les stdout gardent les PASS, sans compteurs métier. Contrairement à
P10, ces portes ne gravent pas de ligne de totaux exacts : leurs succès
imposent seulement les minima déclarés (synthétique : 407 nuages,
100 000 sites, 50 000 retardés ; chaque LiDAR : 30 000 sites et
20 000 retardés), ainsi que les contrôles complets du juge. Aucun
total historique ni compteur inexistant `python_refusals` n’est ajouté.
[Reçu, minima et portée mathématique](../receipts/audit_g4_finp9_20261005/README.md).

## P10 : comparaison exacte de la sortie plate conforme

La session `v11.20261005.claudefinp10` est close au pin **38b76701b**,
avec arrêt ciblé certifié. Les quatre CTests passent : un ensemble
synthétique et les trois trames sans sol entières, en Release u21.
Le juge compare la sortie plate native à la tête Python sur le même
arbre de points natif : partitions, bruit et étiquettes minimales par
PointId, pour EOM z=1/2/3 et la sélection des feuilles.

Les stdout archivés conservent les quatre verdicts PASS. Ils ne
conservent pas les compteurs internes, car `--output-on-failure` ne
les imprime pas en cas de succès et aucun LastTest/JUnit n’est archivé.
Les **1 920 + 3×8 appels** et l’absence de refus Python se déduisent
des boucles fixes, des planchers et des lignes exactes imposées par le
juge épinglé ; ce ne sont pas des compteurs JSON directement lus.
Le différentiel S9 de l’arbre lui-même est passé séparément dans P9.
[Reçu, contrat jugé et limites de conservation](../receipts/audit_g4_finp10_20261005/README.md).

## Reprise G4 après correction : quatre sessions au pin 98a009550

La chaîne `clauderepriser1` à `clauderepriser4` utilise le commit
corrigé **98a009550** et `data_complet`. R1 est clos et entièrement
conforme, avec reçu lié ci-dessus. R2 et R3 sont également clos et
conformes ; R4 est clos à son tour avec 80/80 PASS.
Les plans adoptés se répartissent ainsi :

- R1 clos : lots courts u18/u21/u24/poison, tous conformes ;
- R2 clos : huit lots d’échelle, les 41 absences et les six supports_route de chaque profil passent ;
- R3 clos : 485 mutants détectés, base u18 avec profils locaux déclarés, dont API/CLI après correction ;
- R4 clos : quatre lots ASan/UBSan u24, y compris les six supports_route, tous conformes.

R3 applique les options locales après le profil de base : quinze
mutants num imposent u21, seize num et `tower/export_points_trois_mots` imposent
u24 ; un mutant core active POISON. API et CLI gardent le profil u18.
Ce sont les domaines déclarés par les manifestes et le runner, pas une
qualification u21 des six campagnes sans résultat de l’ancien lot L.

Chaque commande admet 2 200 secondes et la matrice 2 100 ; R2/R3/R4
allouent respectivement 32/32/16 fils. La chaîne exige l’arrêt ciblé
certifié avant la session suivante. Aucun nouveau calcul n’est lancé
par l’audit. L’absence de contradiction de sélection ou de budget
structurel ne garantit pas que tous les tests finiront dans le délai.

Les 28 portes longues fonctionnelles déjà terminées ne sont pas
rejouées. Le plan adopté est plus large que la proposition initiale
limitée aux 41 absences, conservée comme alternative historique :
il utilise la matrice publiée et ne nécessite plus son JSON séparé.
[Proposition initiale et contrôles des noms](../receipts/proposition_reprise_41_20261005/README.md).

## Finl : portes longues fonctionnelles terminées, mutants séparés

La session `v11.20261005.claudefinl` est close au pin **38b76701b**,
avec arrêt ciblé certifié. Les **28 portes longues hors mutants**
passent : onze K10 et dix-sept références FULL exactes. Le filtre
`long` a aussi sélectionné les treize campagnes mutants ; sept passent
en u21, puis l’échéance globale interrompt tower et laisse cinq autres
campagnes sans démarrage consigné. Bilan CTest : **35/41 PASS, zéro
Failed, six sans résultat** ; état matriciel `timeout` conservé.

Aucune identité K10 ni référence FULL de ce lot ne manque. Exclure
le label `mutant` d’une future sélection L lui rend son périmètre
fonctionnel ; aucun rejeu de ses 28 portes n’est demandé à cause de
cette coupure. M reste une campagne en u18, sans transfert à u21.
Les réparations API/CLI de finm et les huit différentiels P10/P9
restent distincts. Aucun défaut moteur établi par le délai global.
[Reçu, inventaire et interruption](../receipts/audit_g4_finl_20261005/README.md).

## Finb : contrôles sanitizer courts intégralement conformes

Session `v11.20261005.claudefinb`, source **38b76701b**, terminée avec
succès et arrêt ciblé certifié. Les deux inventaires courts sont
terminés : **783/783** ASan/UBSan u24, **783/783** TSan u21. Aucun
échec ni résultat manquant.

`head_unit_huge` vérifie la frontière exacte `2^127−1` avec la garde
S10 corrigée ; `points_unit_sort_refusal` vérifie le refus du tri S9.
Ces deux portes passent sous les deux configurations, ainsi que
`num_roots`, `cli_points` et `cli_plat`, normales et −O lorsqu’elles
sont dédoublées. Les onze lots d’échelle/LiDAR restent dans fins ;
les quatre comparaisons exactes S9 et les quatre S10 gardent leurs
sessions dédiées. La qualification globale attend aussi la réparation
des deux portes de qualification signalées en fina2/finm.
[Reçu vérifié, noms et portée](../receipts/audit_g4_finb_20261005/README.md).

## Fins : lots d’échelle et LiDAR terminés au pin final

Session `v11.20261005.claudefins`, source **38b76701b**, arrêt ciblé
certifié. Les onze lots terminent tous leurs inventaires : **80/80**
portes TSan u21 passent ; **74/80** ASan/UBSan u24 passent, avec six
échecs limités à `api_supports_route` (trois tailles et trois trames).
Aucune porte manquante ni interruption à l’échéance.

Les six journaux d’échec conservés confirment cette fois la cause,
cas par cas : sonde `supports_route_verdict conforme`, comptes et
journal attendus, puis refus `ligne_absente` sur les deux hashes u24
comparés aux attendus u21. Les identités entre voies et appel public
passent dans les six cas ; le pic public égale celui de FULL et se
distingue de celui de l’arbre K seul. Aucun diagnostic ASan/UBSan
n’est trouvé dans les journaux runtime examinés. Les six portes CTest
restent néanmoins Failed jusqu’à correction et rejeu de leur juge.
[Extraits conservés et contrôle des attendus](../receipts/audit_g4_fins_20261005/supports_route/README.md).

Les portes `points`, `plat` et `num_roots_cost` passent sur les six
entrées dans les deux configurations. Les six identités entre voies
supports et appel public passent sous TSan u21, au pin qui comprend
L2b et S10. Le lot S ne contient pas les unitaires courts : la session
B suivante les qualifie : la racine frontière S10 et le refus de
tri S9 passent dans finb, décrit ci-dessus. Les différentiels Python complets S9/S10 restent distincts.
[Reçu vérifié, inventaires et périmètre](../receipts/audit_g4_fins_20261005/README.md).

## Finm : deux réparations de qualification, sans nouveau défaut moteur établi

Session `v11.20261005.claudefinm`, source **38b76701b**, arrêt ciblé
certifié à 21:15:46 UTC. Les 39 portes CTest ont toutes un résultat :
37 passent, les campagnes API et CLI échouent. Leurs causes sont distinctes.

**API : la référence non mutée est refusée.** Sur le témoin u18 à
8 000 points, les voies, leurs fichiers/manifeste et l'appel public
concordent ; les pics distinguent aussi `compute=full_tower` de la
voie K seule. La sonde rend son verdict conforme. Le juge rejette
ensuite la ligne, dont les SHA attendus sont ceux d'u21 : fichier
`09a1101bc512394e` et manifeste `e10e6c8d117429e3` observés,
contre `9b77614618bbdfc9` et `d61003785c2ff158` attendus. Ce reçu
confirme donc le défaut de référence sur ce témoin ; il ne donne pas
rétrospectivement les sorties absentes de fina2. Aucun des 23 mutants
API n'est jugé après ce refus, et aucun n'est déclaré survivant.

**CLI : un mutant désormais inactif pour sa porte.**
`sp_masque_16379` supprime `params.concurrent_orders=false` dans
`order_params()`, mais `mhgp11_cli_supports_oracle` appelle supports
et FULL. Depuis L2b, ces deux voies utilisent `full_params()` : la
mutation n'est plus atteinte. Son succès ne démontre pas une faiblesse
de l'oracle supports ni une erreur du moteur.

Conserver cette mutation en la raccordant à **`mhgp11_cli_points`**, et
adapter son nom/note à la sortie points. Cette porte existante exige
des succès admis ; points et plat passent encore par `order_params`,
dont le paramétrage muté est refusé par `build_order`. La construction
CLI et le plancher de 28 mutants sont conservés. Le nouveau raccord
reste à juger avec son témoin dans le harnais existant.
[Chaînes d'appels, sources et preuve bornée](../receipts/audit_g4_finm_20261005/cli_mutant/README.md).

Les lignes individuelles archivées établissent **462 mutants jugés** :
461 détectés (455 par code, 4 par ligne, 2 par construction),
1 survivant. Aucun résultat `INVALIDE`, signal ou délai n'est compté
comme détection dans ce bilan. Les 10 de head, 6 de points et 57 de
num sont détectés. Les rapports individuels JSON ne sont pas conservés
dans l'archive ; le reçu utilise les verdicts explicites des journaux.
[Reçu, comptes et limites](../receipts/audit_g4_finm_20261005/README.md).

## Fina2 : attendu de qualification L2b à corriger par profil

Session `v11.20261005.claudefina2`, source **38b76701b**, arrêt ciblé
certifié le 5 octobre à 20:43:51 UTC. Les six portes
`mhgp11_api_supports_route_scale{8000,16000,32000}` et
`mhgp11_api_supports_route_lidar_ng{00,01,02}_k5` échouent dans les
configurations u18 et u24, mais passent en u21 et poison u21.

**Défaut certain de l'attendu.** `tests/api/tests.cmake:66–87` grave les
mêmes lignes `LINE`, dont les hashes de fichier et de manifeste capturés
en u21, pour les trois profils. La sonde hache les objets bruts ;
MHGP11SP écrit les bits à l'offset 16, et le manifeste les inclut
directement ainsi que dans ses signatures. La référence u21 ne peut donc servir
de référence multiprofil. Corriger l'attendu par profil en conservant
les comparaisons exactes entre voies, workers et publication publique.
Une correction minimale conserve les références gravées u21 ; en
u18/u24, elle grave les comptes et le journal tout en publiant les SHA
réels séparément, sans leur imposer ceux d'un autre profil. Elle garde
l'identité brute entre voies, workers et appel public dans les trois
profils ; aucun changement du moteur ou du format n'est nécessaire.
[Sources, preuve et proposition](../receipts/audit_g4_fina2_20261005/supports_route/README.md).

Les journaux conservés donnent les verdicts Failed, mais pas leur
sortie détaillée ; les `LastTest.log` ne contiennent que leur en-tête.
Cette anomalie de porte ne démontre donc pas à elle seule la cause
exclusive des douze échecs. Aucun défaut de résultats du moteur n'est
établi. La garde sur les pics mémoire ne doit être incriminée que si
une sortie réelle l'établit.

**Préserver les diagnostics de la reprise.** Le runner au même pin
(`tools/g4_matrix.py:535–538,735–741`) ne demande pas
`--output-on-failure` et préfère `LastTest.log` dès qu'il existe,
même si un `.tmp` plus récent est présent lors de l'interruption.
Conserver aussi le journal actif et activer les sorties sur échec
permettra de diagnostiquer les prochains refus. L'archive ne conserve
pas le `.tmp` : son contenu effectif n'est pas affirmé ici.

**Reprise ciblée utile.** Capturer les stdout/stderr de ces six portes
en u18/u24 après correction de l'attendu. La campagne mutants est en
u18 ; `voie_supports_order_tree` utilise
`mhgp11_api_supports_route_scale8000` comme porte. Sa référence non
mutée doit passer pour que la détection du mutant soit qualifiée.
Les autres portes de la session gardent leurs verdicts individuels.

Les quatre configurations ont aussi atteint le budget matriciel de
2 100 s. Bilan brut relu : u18 **977/991** (6 échecs, 8 manquants),
u21 **890/901** (0 échec, 11 manquants), u24 **884/901** (6 échecs,
11 manquants), poison **891/902** (0 échec, 11 manquants). Les portes
S10 unitaires, CLI et échelle/LiDAR, le refus du tri S9 et `num_roots`
passent dans les quatre profils. Les différentiels complets S9/S10
restent dans leurs sessions dédiées. Source, paquet et 635 fichiers
utiles sont vérifiés ; rejeu de lecture normal/`-O` conforme. Aucune
qualification globale déduite.
[Reçu, verdicts individuels et manquants](../receipts/audit_g4_fina2_20261005/README.md).

## S10 publiée : garde numérique corrigée en 510dae50e

S10 publiée en **076d9142b**. Les trois sources causales sont identiques
à la capture initiale sur **311ef5e3c**.
`LevelSource::root` admet toute racine fixe qui tient en `u128`.
`bracket_plateaus` la convertit en `i128`, puis calcule `rt+1`, ou
`rt+rm-rq` pour une date. La façade `flat_sites` n'ajoute aucune garde
de largeur avant cet appel.

Un arbre abstrait valide suffit : deux racines de deux sites, un plateau
positif, `mcs=2`. Avec `R=2^127−1` et le niveau exact
`l=(R/2^64)^2`, la racine retournée est exactement R et respecte le
contrat public. Pourtant `e_hi=i128(R)+1` dépasse le maximum signé.
Le constat est arithmétique, sans exécution native ; il ne démontre pas
un débordement sur les niveaux issus d'un nuage u21/u24.

**Correction publiée en 510dae50e : favorable.** Les fichiers
`internal.hpp` et `score.cpp` sont identiques à la capture relue.
Le filtre contrôle chaque racine utile en `u128` contre `L=2^100`
avant conversion. Sinon, le plateau est marqué sans encadrement et la
sélection utilise le repli exact. Dans le domaine conservé,
`rt+rm-rq` appartient à `[-L,2L]` ; les marges −1/+2 restent dans
`i128`. Le contrat abstrait est conservé et les racines géométriques
u21/u24 ne sont pas écartées par cette garde.

**Test renforcé dans cette publication.** `huge` emploie les racines
`2^126`, `3·2^125`, `3·2^126`. Il exige désormais
`unbracketed==3`, contre `>=1` dans la capture : l'ancien filtre ne
laissait ouvert que le dernier plateau. Cette assertion distingue donc
la garde supprimée par `racine_non_bornee`. Le développeur rapporte son
mutant tué localement ; aucun verdict G4 n'en est déduit. Le témoin
`R=2^127−1` reste la preuve du débordement initial, sans prétendre que
le nouveau test reproduit cette addition. Constat source fermé,
qualification native G4 à poursuivre.
Le complément **38b76701b** ajoute la frontière exacte au même test :
rayons `A/3,A/2,A`, avec `A=(2^127−1)/2^64`, racine du dernier
niveau explicitement vérifiée égale à R, groupes et égalité EOM
contrôlés. Toutes les racines restent dans `u128`. Le développeur
annonce les sept groupes unitaires conformes en Release u21 et
ASan/UBSan u24 locaux ; ce résultat annoncé reste distinct de G4.
[Correction figée et preuve de borne](../receipts/audit_s10_root_guard_20261005/README.md).

Les formules réciproques, la condensation publiée et le raccord relus
n'apportent pas d'autre défaut important établi à cette capture.
[Sources figées, témoin et limites](../receipts/audit_s10_wip_20261005/math/README.md).

**Qualification de la tête plate : le différentiel complet doit être joué.**
Les quatre portes `head_vs_python` publiées comparent les partitions,
le bruit et le plus petit `PointId` de chaque cluster à la tête Python,
sur le même arbre de points publié. Les fixtures manuelles F14 vérifient
bien les clusters retenus dans leurs portes natives distinctes. Lecture
favorable du raccord et des chemins de refus, sans résultat natif déduit.

Le filtre `_vs_python` de la matrice courante exclut aussi ces nouvelles
portes. Le plan S9 déjà adopté ne construit que `points_probe` et
`points_export` ; S10 demande le CLI. Le plan complémentaire joint
construit `mhgp11_cli` et sélectionne les quatre portes head sans
exclusion : synthétique et trois trames entières K5. Le jouer après le
plan S9, sur le même commit final S10 intégré et poussé, en sessions
gardées successives. Dimensionner chacune avec le préflight ; la
validation locale normal/`-O` porte seulement sur la forme du plan.
Le développeur adopte ces deux sessions dans sa réponse publiée
en **510dae50e** ; aucun nouveau reçu G4 constaté à cette mise à jour.
[Plan, déclarations figées et limites](../receipts/audit_s10_differential_plan_20261005/README.md).

## S9 : correctif du refus de tri publié, qualification à poursuivre

**Constat important sur le brouillon, base 53c027fe8.** Dans
`src/points/point_tree.cpp`, `entry_order` trie les dates strictes avec
`std::sort`. Dès qu'une comparaison renvoie un refus, le comparateur
utilise `SiteIdx` à la place de la date, pour cet appel et les suivants.
La relation d'ordre change pendant le tri ; tester `failure` après
`std::sort` ne protège pas ses opérations internes.

Une contre-épreuve bornée reproduit le choix du pivot et le scan non
gardé de GCC : **17 entrées**, pivot de SiteIdx 16 choisi selon les dates,
refus injecté à la quatrième comparaison, puis scan jusqu'à l'index 17.
Le modèle sans refus reste dans ses bornes ; la variante qui propage
immédiatement le refus s'arrête proprement. C'est une preuve du danger
du chemin de refus, pas un crash natif ni la construction d'un nuage u21
épuisant réellement les 6 144 bits de raffinement.

**Correction publiée en 3d47eaa93 : favorable.** Les quatre fichiers sont
identiques à la capture précédemment relue au-dessus de 451301787.
`heap_sort_until_refusal` arrête chaque étape au premier `Outcome`
refusé, avant d'utiliser la réponse de comparaison. Il n'alloue aucun
tampon. `date_less` conserve le départage SiteIdx des égalités exactes ;
`entry_order` propage le refus sans substitution.

Le modèle Python compare le succès au tri de référence et injecte le
premier refus à chaque position : arrêt immédiat, même raison, permutation
et bornes conservés. Le raccord `PointTreeBuilder` → `HangBuilder` → API
→ CLI retourne avant publication ; les brouillons possédés sont détruits
au retour. Ce dernier constat est une lecture de source, pas une injection
native de bout en bout. La nouvelle porte `sort_refusal` exerce le helper ;
son exécution G4 reste à confirmer. Le sixième mutant de `points`,
`tri_refus_ignore`, cible la propagation du refus dans le tas.
[Correctif, preuves bornées et limites](../receipts/audit_s9_sort_fix_20261005/README.md).
[Sources figées, modèle et limites](../receipts/audit_s9_wip_20261005/README.md).

## Qualification G4 du 5 octobre : acquis et limites

**A2 ferme la sélection ordinaire sur b319efc84.** Reçu `completed`,
`DONE=0`, matrice `complete=true` et `conforming=true`, arrêt ciblé
certifié ; SHA du paquet, du plan et des résultats rapprochés du reçu.

| Configuration | Tests conformes / sélectionnés | Échecs / sans résultat |
| --- | ---: | ---: |
| GCC Release u18 | 914 / 914 | 0 / 0 |
| GCC Release u21 | 824 / 824 | 0 / 0 |
| GCC Release u24 | 824 / 824 | 0 / 0 |
| Poison u21 | 825 / 825 | 0 / 0 |

Chaque profil passe les 27 portes API, les 24 portes IO, le contrat CLI
normal/`-O`, les deux oracles CLI supports et les coquilles 24/25.
Les dix portes CLI auparavant sans résultat et retenues dans A2 passent,
y compris les six jumelles supports `-O`. Les tests `long` et mutants sont
exclus ; les portes `reference_*` ne sont sélectionnées qu'en u18.
La conformité porte exactement sur ces sélections.

**B apporte les premiers résultats sanitizers au même pin b319efc84.**
ASan/UBSan u24 : **774/824** tests conformes, 50 sans résultat ; TSan
u21 : **759/824**, 65 sans résultat. Aucun échec individuel terminé,
mais les deux CTests sont coupés par l'échéance globale de 2 100 s :
la matrice demeure non conforme. Fermeture ciblée et SHA vérifiés.
Les 53 portes API/IO/contrat CLI et les oracles supports normal/`-O`
passent dans chaque profil. Sous sanitizer, le contrat CLI omet les
préchargements : ce succès ne rejoue pas le crochet variadique.

Les 50 absents ASan sont inclus dans les 65 absents TSan. Le retrait
par c97776ea8 a été corrigé par les huit lots de a7711b506. Leur exécution
est maintenant documentée dans S ci-dessous, sur une source intégrant S8/S9 ;
les inventaires et qualifications des deux sources restent distincts.
[Preuve B et noms des portes manquantes](../receipts/audit_g4_b_20261005/README.md).
[Ensembles de reprise exacts par module](../receipts/audit_g4_b_restart_20261005/README.md).

**S ferme cinq lots TSan à l'échelle sur d26328fe2.** La session
`claudequals` est close, avec arrêt ciblé certifié et chaîne
source/paquet/plan/résultats vérifiée. Elle reste globalement partielle.

| Lots | Conformes / sélectionnés | Échecs / sans résultat |
| --- | ---: | ---: |
| TSan u21 : 32k CLI, 32k hors CLI, ng00, ng01, ng02 | 60 / 60 | 0 / 0 |
| TSan u21 : reste | 28 / 40 | 3 / 9 |
| ASan/UBSan u24 : grand lot | 34 / 55 | 0 / 21 |
| ASan/UBSan u24 : reste | 42 / 45 | 3 / 0 |

Les trois échecs de chaque configuration sont les mêmes cas
`num_roots_cost_uniform_u18_n{8000,16000,32000}_k5`. ASan conserve
les trois raisons `input_unreadable` ; les fichiers sont absents du paquet.
TSan conserve les trois échecs, sans leur diagnostic brut : la même cause
y est déduite des entrées absentes et de la source. Aucun résultat
mathématique incorrect n'est établi. La préparation
locale **`build/v11-persist/qual_sorties/data_complet`** contient depuis
les six fichiers requis, aux tailles et empreintes du manifeste ; le
répertoire `data` les omet toujours. Utiliser le premier pour la reprise.
Ce contrôle de préparation ne rejoue aucun test.

S9 passe ses portes CLI LiDAR normal/`-O` sous TSan sur les trois trames.
ASan passe les trois trames en normal, ainsi que ng00 sous `-O` ;
les jumelles `-O` de ng01/ng02 n'ont pas de résultat. Les portes courtes `num_roots` et `points_unit_sort_refusal`
ne sont **pas sélectionnées dans S** ; les quatre `points_vs_python`
restent également dans leur campagne dédiée. Aucun transfert vers L2b,
absente de la source S.

**Reprise préparée en 8b2ca400e : onze lots, quatre ASan et sept TSan.**
Sur les inventaires S, leur union est exacte et disjointe pour les
68 portes sans suffixe `_opt` par configuration. Les 32 jumelles `_opt`
sont désormais exclues explicitement ; aucun succès ne leur est attribué
par ce redécoupage. Les seuils de sélection sont satisfaits sur cet
inventaire. La nouvelle matrice n'a pas encore de résultats : terminer
les lots et les trois cas d'entrée réparée reste nécessaire.
[Preuve S, inventaires et contrôle du découpage](../receipts/audit_g4_s_20261005/README.md).

**L ferme 27 portes non mutantes et sept campagnes de mutants en Release u21 :
34 résultats conformes sur les 38 sélectionnés.**
Les identités FULL K10 32k et ng00 passent. Le résumé global reste
non conforme : quatre campagnes de mutants sélectionnées n'ont pas de
résultat dans cette session ; les résultats antérieurs de ces campagnes
restent attachés à leur propre reçu. Les tests longs ne sont pas doublés
sous `-O` et ne sont pas qualifiés en u18/u24 ou sous sanitizers.

**Mesure et complément W48 désormais joués.** La session
`claudequalmesure`, sur b319efc84, ferme la mesure appariée et les
permutations/réétiquetages supports W48 sur ng02 et ng00 : 14 appels par
porte, verdicts conformes. Les résultats sanitizers plus récents de S
sont détaillés ci-dessus ; ils ne qualifient pas la future voie L2b. Aucun contrat
100 ms n'est acquis. Les fermetures ciblées et les empreintes sont relues.
[Reçus L/mesure et portée](../receipts/audit_l2_decision_20261005/README.md).
[Preuve A2, noms exacts et rejeu](../receipts/audit_g4_a2_20261005/README.md).

Les premières captures **claudequalmatrice/claudequalA**, au pin
**00bd979ac**, restent conservées : coupure globale sans échec de test
terminé et **459 mutants u18 détectés**, dont 15 supports, 22 API et
28 CLI. Leurs tests interrompus ne sont pas réécrits en succès : les
résultats A2 ont leur propre source et reçu. Le produit et ses portes
sont inchangés entre 00bd979ac et b319efc84.
[Preuve antérieure](../receipts/audit_g4_sorties_20261005/README.md).

## Qualification de S9 : garder le différentiel exact sur trames entières

**Le filtre `_vs_python` de c97776ea8 retire les quatre différentiels S9
de la configuration `release_long`.** Ils ne sont joués dans aucune autre
configuration de cette matrice. Le rapport local décrit des succès au pin
antérieur au correctif de tri ; aucune nouvelle session G4 ne les qualifie.

L'oracle indépendant en bibliothèque standard reste sélectionné : dates,
propriétaires et partitions sur petits nuages jusqu'à K4. Les portes CLI
à K5 vérifient la structure, les empreintes entre exécutions et des dates
échantillonnées. Elles ne prouvent pas l'identité de tous les champs avec
la chaîne Python sur les trames, notamment le **plancher maximal** : le
lecteur ne possède pas tous les rangs du catalogue. Ce constat porte sur
la qualification ; le moteur conserve son certificat exact au rang suivant.

**Suite concrète :** intégrer les quatre CTests existants dans la session
de la source finale avec `python_packages="pinned"`, disponible dans le
contrôleur gardé. Le plan joint les sélectionne séparément, sans filtre
d'exclusion, avec refus d'une sélection vide. Il construit seulement les
cibles nécessaires. Sa forme est validée normal/`-O` ; aucun préflight
cloud ni test natif n'a été lancé par l'auditeur. Les portes courtes de
S9, dont `sort_refusal`, restent sélectionnées sous sanitizers ; le pipeline
TSan et l'identité MHGP11SP de L2b restent leurs propres portes.
[Preuve du périmètre et plan proposé](../receipts/audit_s9_qualification_scope_20261005/README.md).

## Décision utile au développeur : passer à L2b

La mesure complète respecte le protocole écrit avant la campagne :
FULL/16379 et supports/7035, trames entières ng00/ng01/ng02, K5, W1/W48,
prises appariées, identité des fichiers et des manifestes. La règle sur
l'étage `tree` rend **`livrer_L2b`** : le chemin d'ordre K seul ne satisfait
pas le critère sur deux trames. L'avantage d'écriture de MHGP11SP ne
change pas cette décision d'architecture.

Raccorder le journal au constructeur de l'ordre K dans la voie concurrente
de FULL, comme prévu dans `SORTIES.md` § 11. Conserver tous les supports,
le rattachement après fermeture des plateaux et les admissions mémoire
avec le journal vivant. Les portes décisives restent **MHGP11SP identique
octet pour octet par les deux voies**, TSan sur le pipeline et les mutants.
L2b est publiée en **0810962ac** : ses vingt fichiers de livraison sont
identiques au commit local cc73784f0 relu. Sa qualification G4 reste à faire.
[Contrelecture de la décision et de ses entrées](../receipts/audit_l2_decision_20261005/README.md).

**Diagnostic du chemin normal : correction publiée, lecture favorable.** La
capture initiale puis le commit local **cc73784f0** passent toujours
`&seen` à `supports_parts`, même sans diagnostic demandé. Le hachage
FNV du journal coûte alors exactement **2C+G mots**, soit **16C+8G
itérations** par octet, dans `Stage::tree`.

Le correctif publié en **311ef5e3c** est identique aux
trois fichiers capturés. Il prépare désormais
`wanted = diagnostics == nullptr ? nullptr : &seen`, puis transmet
`wanted`. Le pointeur nul parvient à `build_order_full` :
`registers_of(log)` n'est plus appelé sur le chemin public normal.
La porte ajoutée compare aussi les fichiers et manifestes de l'appel
public sans diagnostic à ceux de la voie FULL, et observe cette voie
à W1. Le mutant `voie_supports_order_tree` cible le retour involontaire
à l'ancienne voie. Ces ajouts sont relus dans les sources ; leur présence
ne constitue pas un résultat d'exécution. Qualifier cette version sur G4
et mesurer l'appel public sans diagnostic. Aucun gain chiffré
n'est déduit de la correction.
[Capture initiale](../receipts/audit_l2b_wip_20261005/README.md).
[Correction relue, source figée et limites](../receipts/audit_l2b_followup_20261005/README.md).

## S8 : socle numérique de la sortie points

Tranche **53c027fe8**, relecture de `Big`, `Rational`, `RootTable` et
`RadicalSum` : **aucun défaut important nouveau établi**. Les limites
d'entiers et d'indices précèdent les écritures ; les allocations refusées
libèrent le brouillon. Les classes de racines sont fusionnées après test
de carré parfait ; leur signature ne décide jamais l'égalité. Une somme
indécidable dans le budget rend `radical_sign_budget`, jamais zéro.

La contre-épreuve indépendante Python contrôle signes, égalités,
collision de signature, bornes de table et refus. Elle ne teste pas le
C++ : les rapports locaux S8 restent distincts, et A2 sur b319efc84
précède cette tranche. La session S couvre certains coûts de table LiDAR, mais pas
la porte courte `num_roots`. La qualification numérique complète aux
profils annoncés, ses mutants et le coût W48 restent à constater sur G4.

Pour le raccord S9, conserver l'admission de tous les temporaires par
worker et payer la table entière même quand seuls certains rangs sont
remplis. Les égalités de dates et le refus K=n≥2 doivent être jugés sur
la sortie points assemblée. S8 fournit les primitives ; S9 est publiée,
avec une qualification G4 partielle et un différentiel exact sur trames
encore à jouer.
[Sources, contre-épreuve et limites](../receipts/audit_s8_20261005/README.md).

## Audit général du 5 octobre : décisions importantes

Base publiée **238734f1d** ; brouillons S3/S5/S6 identifiés séparément par
empreinte. [Dossier de preuve et périmètre](../receipts/audit_geant_20261005/README.md).
La revue couvre les fondations numériques, les propriétaires et mémoires,
le catalogue, FULL, les supports, la projection sur les points et la portée
des qualifications. L'inventaire comprend **104 fichiers natifs dans huit
modules** ; un inventaire n'est pas une qualification ni une promesse de
preuve exhaustive. Les lectures nouvelles et celles des audits antérieurs
sont distinguées dans le reçu.

**Correction de FULL.** Aucun nouveau résultat FULL faux n'est établi.
Une nouvelle contre-épreuve compare le graphe complet des intersections
des régions témoins à la définition par K-parties : **65 ordres, 1 453
coupes ouvertes/fermées et 15 925 contrôles de faces verticales**, conformes
en Python normal et `-O`. Elle partage les MEB exactes de l'étage A ; elle
n'est donc pas un nouvel oracle numérique indépendant de A. L'accord A/B,
les supports et les pendaisons de points sont également recoupés sur ces
petits nuages. Aucun transfert de ces succès Python au binaire courant.

**Priorité de livraison : fermer L1 sur le vrai produit assemblé.**
Au pin de l'audit général, `build_order`, l'attachement et les primitives
de supports étaient des brouillons. S3/S6a/S5/S6b/S7 sont depuis publiées.
Les preuves mathématiques autorisent leur
intégration ; elles ne remplacent pas les portes natives de l'assemblage.
La porte décisive doit comparer l'ordre K de FULL, l'arbre K seul et les
attaches à la coupe fermée, sur la même source figée, avec les plateaux
E5/D2 et les coquilles étendues. Conserver toutes les boules faibles et
tous les supports, y compris les q4 du cube à K1 sans coface. Le refus
au-delà de 24 sites doit porter sur **l'appel supports entier**. Budget,
count/fill, concurrence et absence de publication partielle doivent être
jugés sur l'assemblage, pas déduits des seuls helpers S6a.

**S6b : conditions d'assemblage implémentées, contrelecture favorable.**
Au commit publié **9e7428995**, la pré-passe refuse une coquille trop grande
avant toute allocation. L'admission couvre les sorties, les temporaires,
la fermeture et la liste de supports de chaque worker. Les deux passes
utilisent les mêmes positions et le même `BallIdx` ; leurs écritures sont
disjointes et les registres sont sommés après jointure. Tous les supports
sont conservés, même lorsque leur compte de cofaces est nul.

Le différentiel complet compare aussi les supports, les comptes par
boule et par support, le rattachement et les tailles de sous-arbre.
Les portes d'admission et du plafond portent maintenant sur l'appel
entier. Aucun nouveau défaut important établi. Les résultats natifs
annoncés dans les rapports locaux restent distincts des résultats G4 :
les acquis partiels ci-dessus comprennent la campagne de mutants u18 ;
les configurations ordinaires sont depuis conformes dans A2 ;
la couverture sanitizers/TSan reste limitée au périmètre décrit ci-dessus. L apporte les tests longs
non mutants u21, et le complément supports W48 est désormais conforme.
S7 livre depuis **966a351be** l'écriture et la lecture du fichier,
la comparaison des signatures FULL/supports et le pilote de mesure.
La qualification de toute la chaîne reste à clore sur G4.
[Sources, contre-épreuves et limites](../receipts/audit_s6b_20261005/README.md).

**S7 : revue favorable ; complément W48 conforme.**
Le produit possède ensemble l'arbre et sa hiérarchie ; la publication garde
les contrôles de Session et de provenance avant toute écriture. L'écrivain
conserve les colonnes canoniques ; le lecteur recalcule les comptes et la
signature depuis MHGP11SP. Les portes comparent la sortie complète à S1,
ses signatures à FULL et les octets sous changements de workers et d'IDs.
Aucun défaut important établi par cette relecture ; aucun test natif lancé.

Les portes d'échelle et LiDAR CTest fixent **W1/W4**. Le pilote de mesure
W1/W48 garde les mêmes entrées : il ne joue pas les permutations et
réétiquetages à W48. Le développeur a ajouté au plan de mesure
`cli_supports_scale.py --fils=1,4,48` pour ng02 et ng00,
avec leur boîte cosphérique. Hors CTest, le script exige
**14 appels**, y compris répétition, permutation et nouveaux IDs à W48,
contre 12 dans les lignes CTest actuelles. Ce complément a été joué
conformément dans `claudequalmesure` au pin b319efc84 ; la couverture
demandée est close sur ces deux trames et cette source.
[Relecture, limites et fragment de plan validé](../receipts/audit_s7_20261005/README.md).

**Manifeste S5 : correctif confirmé par sa porte native G4.**
Dans le correctif publié **a5e4019b4**, identique au WIP relu au contexte
**59743c210**, `publish`
exige désormais `points_bytes=12n`, `ids_bytes=4n` et un budget déclaré
absent ou strictement positif. Le nombre de points vient du poids du
nuage du produit. Le refus précède la création de fichiers, le budget et
le rapport. Les anciennes portes avec `Provenance{}` utilisent une
provenance cohérente.

La nouvelle porte **publication API → lecteur officiel** est câblée ;
elle contrôle aussi les quatre provenances incohérentes, sans `D` ni
`D.pending`. Le constat de source est clos. La porte est maintenant
conforme sur G4 en normal et `-O`, en u18/u21/u24 et empoisonnement u21 ;
les 22 mutants API sont détectés dans la campagne u18. La protection du
rapport reste établie par l'ordre des contrôles dans le code.
[Correctif et contrelecture](../receipts/audit_corrections_s3_s5_20261005/README.md),
[défaut initial conservé](../receipts/audit_api_publication_20261005/README.md).

**Identité Session : correctif confirmé par ses portes natives G4.**
Le produit garde désormais l'identité du budget alloué sur le tas, stable
au déplacement de la Session. `publish` refuse une autre Session avant la
provenance, les fichiers et le rapport. La nouvelle porte déplace la
Session avec le produit vivant, vérifie la publication par sa propriétaire
et la libération finale. Le destructeur contrôle aussi le retour du budget
à zéro ; la violation de durée de vie termine explicitement le processus.
Les portes `session_product_alive`, `session_session_identity` et les
autres portes API ont maintenant un résultat `Passed` dans les quatre
configurations de la session A. Le constat initial `a65903a7b` est clos
sur ce périmètre natif ; cela ne clôt pas les suites interrompues.

**Priorité performance : réaliser L2b selon la mesure complète.** Les temps
S3 obtenus journal désactivé ne donnent pas le coût des attaches ni de
`supports`. La mesure complète apparie FULL/16379 et ordre seul/7035,
journal actif, mêmes entrées entières, avec l'énumération, l'assemblage
et l'écriture relevés séparément. Elle est close et impose L2b. Les **100 ms**,
la projection native et une qualification récente de la chaîne entière
restent ouverts. Le rejeu des reçus c40 retrouve leur qualification et
leurs 81 prises conformes ; il ne qualifie ni HEAD ni les brouillons.
Les trois trames de séquence08 ne deviennent pas plusieurs séquences,
et les comparaisons c40/baseline v11 ne ferment pas le différentiel
canonique v10/v11 sur LiDAR entier.

Pour poursuivre : qualifier S8/S9 sur la source intégrée, conserver les
différentiels exacts LiDAR et réaliser L2b selon la décision mesurée.
Aucune réserve générale
nouvelle n'est opposée à l'intégration de S3. **Aucun build/test natif ni
GCP lancé par cet audit.**

## Sorties paramétrées : avant intégration et G4

**Raccord L1 relu : avancer vers l'assemblage.** Le développeur a repris
les demandes de l'audit `a65903a7b` dans sa réponse **4f1e0fb3a**.
Dans la capture précédente du suivi `416767435` de `build/v11-impl-l1`,
S6a contient maintenant le différentiel S1 à 951 ordres u21/u24 et les
témoins à 24 sites et K10/K12 demandés. Les primitives Python et le refus budgété de
l'oracle passent en normal et `-O` ; aucune exécution native n'est déduite
de leur succès. S3/S5 et l'assemblage S6b ne sont pas encore raccordés dans
cette capture. Aucun nouveau verrou général n'est opposé à cette suite.
[État exact et preuve](../receipts/audit_l1_followup_20261005/README.md).

S6a est désormais publiée sur `main` en **19b2fb218**, avec les mêmes
sources et portes que **ee8a69f1a**. S5 est publiée en **d6082be62**, puis
corrigée en **a5e4019b4** pour la provenance : sources identiques aux
captures relues. S3 est publiée en **165def5ab** : cœur, E1/E2 sur
catalogue étroit et témoin D2 identiques au WIP déjà relu.
Le juge Fraction des attaches et le témoin K10 sont désormais câblés
dans les portes permanentes de S3 : domaine étroit, comparaison des
attaches fermées et des antécédents ouverts, voies W1/W3 et permutation
d'entrée. Le témoin de 12 sites couvre K1..12. La qualification native
reste à jouer sur la source assemblée ; S6b est désormais présente en
**9e7428995**, puis S7 en **966a351be**.
[Contrelecture de ces nouvelles portes](../receipts/audit_corrections_s3_s5_20261005/README.md).
Les résultats natifs annoncés par le développeur restent
distincts des vérifications Python de l'audit et de la qualification G4.

**S5 : anciennes alertes corrigées dans les commits publiés.** Le CLI ignore désormais
SIGXFSZ et la porte rétablit son comportement par défaut avant exec avec
RLIMIT_FSIZE. Signature V2 à l'ordre demandé, SHA du manifeste fermé et
`published_complete` sont intégrés. Les portes jugent le champ réellement
publié à K1..4, les doubles échecs après publication, stdout/inode, liens
symboliques, O_RDONLY et priorité des refus. Les cinq signatures gravées
comprennent trois fixtures indépendantes et deux régressions issues de
structures natives. Les anciens survivants du rapport S5 portent sur une
capture antérieure. **168 gardes nouvelles** recoupent les juges sur
quinze issues factices et les 60 définitions de mutants ; cela ne prouve
ni 60 mises à mort ni la qualification native des nouveaux correctifs.
[Capture et limites](../receipts/audit_native_integration_20261005/api/README.md).

**Harnais des deux fautes IO : correction typée relue.** La nouvelle
version de `io_fault_preload.cpp` décode et retransmet exactement les cinq
arguments de `SYS_renameat2` : `int,const char*,int,const char*,unsigned`.
Un autre numéro invalide explicitement la porte. Les deux cas injectés
utilisent W1 ; cette relecture ne qualifie pas le préchargement pour les
attentes futex de W>1. L'ancien constat variadique est clos au niveau du
code publié ; les portes natives restent à jouer sur la source intégrée.

**S3 : raccord final favorable, mesure du journal encore distincte.**
Naissances enregistrées avant DSU, lots appliqués par ordinal et attribués
après tout le plateau ; capacités admises avant allocation, diagnostics
inchangés sur refus. Le nouveau modèle abstrait recoupe le compactage et
120 ordres de terminaison, sans devenir un TSan. Le rapport local annonce
951 cas différentiels contre Fraction et 58 identités FULL avant/après ;
le prototype oracle est maintenant intégré aux portes permanentes. Ses
temps avant/après concernent **journal désactivé** : ils ne mesurent pas
le coût du journal actif et des attaches. G4 et suite complète restent
attendus. [Raccord, coexistences et portée](../receipts/audit_native_integration_20261005/tower/README.md).

**Matrice proposée : favorable, à découper par livraison.** L1 qualifie
S3/S6 intégrés sur la même source figée ; io/api/cli et leurs fautes sont
la qualification L2 de S5/S7 maintenant intégrés. GCC Release u21 et u24 séparés,
ASan/UBSan et TSan selon les profils réellement joués, mutants des modules
modifiés, identité build_order/build_full, plateau E1/E2 et déterminisme
W1/W4/W48. Inclure coquille 24 admise, 25 refusée pour l'appel supports
entier et q4 à K1 malgré zéro coface. S6a juge les primitives ; S6b ajoute
budget/refus et count/fill parallèles, à qualifier avec l'assemblage.
Garder u18 explicite si livré, sans transfert de qualification. Ajouter
les attendus indépendants K10/K12 de la note mathématique ; la porte K12
proposée vise les primitives, pas FULL12. S6b admet désormais
domaine/arbre, métadonnées, sorties et, par worker du Pool, **scratch de
fermeture plus liste temporaire de supports**. Les 2 Mio par worker à 24
sites (96 Mio à W48) ne sont que le scratch ; la liste a au plus 12 926
entrées, avec `sizeof(Support)` du build. Sommes d'offsets u64 vérifiées
et même ordinal BallIdx entre count/fill.

Les tailles 8k/16k/32k et LiDAR K5 restent des portes d'échelle, avec
l'oracle exact sur petits cas. Fixer le coût des jumelles Python/−O,
des mutants et sanitizers avant la session : **00bd979ac** porte le
budget global à 2200s et les configurations à 2100s ; les deux premières
sessions ont pourtant été coupées. **b319efc84** sépare maintenant le lot
`long`. La mesure L2 apparie FULL/arbre K seul, même entrée,
profil, K et options applicables. Garder **FULL au masque 16379** ; l'ordre seul
prend **7035**, retirant uniquement verticales parallèles 128, réemploi
vertical 1024 et ordres concurrents 8192, refusés par `build_order`.
Le juge S3 le fait déjà correctement. Éteindre ces options aussi dans
FULL changerait la référence qualifiée. Publier cette différence ;
`output` et `write` sont mesurés séparément.
Une seule session gardée, arrêt ciblé certifié, reçu rejugé normal/−O.
Aucun GCP ni build/test natif lancé pour cet audit.

## Socle relu et correctifs P1/P2

Les **101 fichiers de source des sept modules alors présents** ont une relecture de leurs
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

## Ports relus et réponse D sur J3

**q3 différé, 56216392e : raccord favorable.** Catalogue et MEB conservent
centre, tag3, certificats, niveau brut sans PGCD, contacts, support et
compteurs logiques. Le catalogue matérialise après S*/admission géométrique,
avant un éventuel refus du Collector ; MEB après inclusion de toute la partie.
**1 983 + 1 344 gardes exactes**. Le différentiel eager partage la factory
réécrite : garder aussi le juge arithmétique indépendant. **Qualification
G4 Release/u21 q3+R acquise dans claudeab8** : 673 portes et sept mutants
(num/catalogue) ; aucun ASan/UBSan ou TSan dans cette session. Les gains
restent descriptifs, voir les nouvelles captures ci-dessous.

**Compteurs locaux, 0c358261c : raccord R1 favorable.** Quinze champs
initialisés et vidés par `checked_add` après une feuille réussie ; réduction
des tâches inchangée. m≤1024 est contrôlé, borne conservatrice
140464088678400<2^49. Les préfixes logiques sont conservés, les refus
remontent sans sortie partielle. Portes/mutants relus ; les essais G4 e49
sont publiés avec le refus de style et leur portée partielle conservés.

**Lemme R, 9b9244a00 : raccord favorable.** Dominance stricte sur la fermeture
Q + centre dans Q + générateurs de coquille certifient intérieur/extérieur.
Ordre, arrêt à saturation, I/U et census logique restent identiques ; tous
les autres contacts sont testés. **103 200 gardes**, dont masques multi-mots
63/64 et 127/128. Le code réserve bien les deux matrices. Corriger seulement
la formule de `CATALOGUE.md` : **16C⌈C/64⌉**, au lieu de 8C⌈C/64⌉ ; ce
reliquat documentaire n'est pas un défaut d'admission. Les 43 % annoncés
concernent des classements par masque, sans gain chronométrique acquis.
Le raccord q3+R a désormais la qualification G4 Release/u21 claudeab8.

**Ordre A/B : corrigé dans les sources d5b1d0179.** Williams alterne
correctement les deux variantes ; un cycle complet équilibre positions et
successions dirigées. Cinq répétitions/N2 annoncent `balanced=false`.
La campagne ancienne n'est pas rééquilibrée rétroactivement. Base/q3/q3+R
mesure q3 puis R conditionnel à q3, sans interaction estimée.
[Correctifs et 237 gardes portables](../receipts/audit_gpu_euler_20261004/mesure_protocol_live/README.md).

**Réponse D : contrat J3 accepté comme voie explicite.** Aucun besoin de
reproduire les hits du cache. `fallback` signifie cache demandé mais
indisponible, **pas repli numérique** : zéro sous J3≤32 ; garder la convention
DFS/cache pour les feuilles larges. `evaluations=demandes` devient un compte
logique de cette voie, avec préparations/calculs H physiques séparés.
Conserver les autres compteurs logiques et le fail-fast de référence par
comptage agrégé : pour trois faces ordonnées f1,f2,f3, demandes=1+f1+f1f2,
rejets=1−f1f2f3, conditionnellement aux portes précédentes. Pas de replay
géométrique requis pour compter ; vérifier ce ledger et les sorties
contre DFS, feuilles larges et contacts compris, puis ablation G4.
[Sources figées, preuves et rejeux](../receipts/audit_ports_20261004/README.md).

## Enveloppes et lecteurs des mesures

**M3/E4, ec55578d9 : raccord favorable.** Le centre d'un triangle strictement
aigu appartient au triangle médian ; le centre strictement intérieur au
tétraèdre appartient à l'enveloppe des quatre sommets. Doublement exact,
face basse incluse/haute
exclue, filtre strict et prolongements q3→q4 sont préservés. `q4_candidates`
reste compté avant E4. **7 552 gardes Gram/Fraction**, profils et permutations
compris ; portes/mutants cohérents. Essais G4 e49 publiés : portes numériques
et mutants passent, refus de style conservé ; aucun gain établi (§ captures CPU).

**Lecteur pipeline : corrigé dans les sources d5b1d0179.** L'ordre injustifié
dernier départ≤première fin est retiré ; les bornes portent sur `forest_ns`.
`publish_end`, `vertical_end` et `lanes_last_finish` sont désormais exportés :
une queue nulle conserve la durée complète par end−start. La porte Python
inclut des voies disjointes dans le temps. La collecte reste favorable en
source ; qualification native de la télémétrie distincte des captures e49.
CPU et attente couvrent toute la tâche ; CPU+attente n'est pas une partition
exacte du mur, ni une attribution au SMT.
[Réponse et contrôles](../receipts/audit_gpu_euler_20261004/lecteurs_published/README.md).

**Bancs : contexte et timeout corrigés au pin61da.** Le dérivé A/B conserve
verdict/refus, identité, plan et empreinte du parent, exclut les dumps
étrangers et publie paires attendues/retenues. Le timeout du banc de tailles
est persisté, les prises achevées sont checkpointées et le banc poursuit
avant son code1 final. **237 gardes**, appels entièrement simulés.
Le dérivé reste un diagnostic : code0 signifie «lu». Avec cinq paires,
p bilatérale minimale=0,0625 ; garder cette portée descriptive à 5 %.
Les tranches de sites ne qualifient pas une trame entière.
[Réponse et témoins](../receipts/audit_gpu_euler_20261004/mesure_protocol_live/README.md),
[observations initiales conservées](../receipts/audit_enveloppes_mesures_20261004/README.md).

## Juge Euler/J1 et voie GPU : sources et domaine de la contrelecture

**Euler/J1 : raccord favorable.** Le regroupement par MEB et la différence
finie sur les intérieurs justifient `Cat_(K+2)` pour vérifier les ordres 1..K.
Un tétraèdre avec son centre montre que K+1 ne suffit pas : à k2, Euler vaut
2 sur Cat3, 1 sur Cat4. Les supports positifs, la fermeture des masques et
la restriction J1 sont cohérents. Le niveau brut est bien recalculé depuis
le premier support **retenu**, si le premier du grand catalogue est filtré.
**3 623 gardes exactes** sur modèles bornés ; aucun défaut matériel établi.
Le refus coquille>24 précède le travail du juge, après construction des
catalogues ; à 24, le brouillon vaut 2 Mio par fil. Compensation et omissions
communes restent possibles : ce juge ne certifie ni la complétude I/U ni FULL.
Les limites sont déjà annoncées par le développeur. [Preuve et témoins](../receipts/audit_gpu_euler_20261004/euler/README.md).

**Feuilles device : géométrie et repli relus favorablement.** Contacts,
propriétaire demi-ouvert, S*, préfixes obtus et arité de présentation sont
préservés. En u21/u24, certains q3 dépassent le certificat i128 : le port
arrête avant multiplication, efface tout apport partiel, puis rejoue la
feuille entière sur CPU avant admission. Les Levels restent construits sur
l'hôte. **145 391 gardes Fraction/Gram**, sans natif ; rapprochement des
prédicats avec le commit publié. Garder en portes G4 le q3 extrême, q4
au seuil de cube 2^20/+1, le préfixe obtus et la coquille à qmin2.
[Formules, domaine certifié et fixtures](../receipts/audit_gpu_euler_20261004/device_geometry/README.md),
[rapprochement publié](../receipts/audit_gpu_euler_20261004/device_published_bindings/README.md).

**R7 mémoire : payloads explicitement réservés au pin22.** Le scratch,
l'ordre, les temporaires et les retours hôtes coexistent dans le même compte.
Réservation avant `cudaMallocAsync`, remboursement si l'allocation échoue,
libération ordonnée sur le même flux. La réservation se termine à l'enqueue
de `cudaFreeAsync`, tandis que le pool peut conserver les pages. Le compte
logique, `device_bytes` (cumul d'allocations) et la mémoire physique sont donc
distincts. **57dd21be1 ajoute** les pics UsedMem/ReservedMem à la sortie ;
leur nouvelle capture reste à qualifier. Pour cette mesure,
contexte/piles restent externes. [Contrelecture et sources CUDA](../receipts/audit_gpu_scratch_20261004/cuda/README.md),
[documentation de l'allocateur](https://docs.nvidia.com/cuda/cuda-programming-guide/04-special-topics/stream-ordered-memory-allocation.html).

**Plafond feuilles≤sites : corrigé dans les sources 00800dd88.** Nos deux
modèles exacts donnent 80 feuilles pour neuf sites (159 nœuds/648 entrées),
ce qui réfute la garde du pin61da. Les listes de sites K-certifiées se recouvrent.
Les exécuteurs imposent désormais 2^40 jobs au total, CUDA 2^32 pour ses
ordinaux u32 : sommes strictement sous 2^62/2^54. Le refus est typé ressource,
sans hypothèse géométrique count≤n. Une porte de 3 000 sites et lot>n
a été ajoutée ; les résultats locaux annoncés restent distincts de la
qualification G4. Tri stable par taille, écriture par ordinal et réservations nouvelles
relus favorablement ; préchauffage CUDA **dans** le chrono FULL, rejoint
avant le lot. Sa durée chevauche le CPU et ne s'additionne pas aux phases.
[Témoin initial](../receipts/audit_gpu_euler_20261004/centre_leaf_counterexample/README.md),
[correctif et contrelecture008](../receipts/audit_gpu_update_20261004/README.md).

**Réponse E : aucune objection au retrait CPU de M3/E4.** Ces filtres
éliminent uniquement des candidats déjà exclus par positivité/propriétaire.
Garder les tests d’aiguïté et de poids stricts, le propriétaire, la récursion
indépendante des q3 rejetés et le placement de `q4_candidates` ;
sorties et ledger doivent rester identiques.
Après patch, jouer la porte différentielle existante et adapter les mutants
visant les textes supprimés. Le coût mono observé reste descriptif ; mesurer
séparément leur intérêt GPU, sans transférer la conclusion CPU.

**Banc GPU : non-vacuité corrigée au pin22.** reps≥1 et P≥2, puis
séquence exactement 1..P et statuts ok. Les témoins anciens et les flux
incomplets sont désormais refusés ; contrôles complets P2/P3 acceptés :
**240 gardes AST**, processus entièrement simulés. La portée d'identité reste
le dernier dump de chaque processus, pas ses passes intermédiaires.
Corriger seulement l'intervalle de la parenthèse `scope` : passes 1..P−1,
puisque P sérialise. Nsight reste séparé des prises ordinaires.
[Rejeu du correctif et formulation du reçu CPU](../receipts/audit_gpu_scratch_20261004/bench/README.md).

**Format compact et stockage : raccord favorable au pin22.** Record de huit octets,
supports/incidences en rangs locaux 0..31 : qmin, contacts I/U, SiteIdx larges
et mêmes fabriques de Level brut sont conservés. Le scratch de 2 Kio par feuille
est copié seulement s'il est complet ; débordement ⇒ rejeu entier, nonrésolue
⇒ scratch jeté puis repli CPU. Copie/rejeu sont disjoints ; fill n'ajoute aucun
ledger. Les 32 fils, même hors count, atteignent la réduction warp avec zéro.
**3 404 gardes format/Fraction + 18 426 scalaires CUDA**, sans natif.
[Format et contrats](../receipts/audit_gpu_scratch_20261004/compact/README.md),
[stockage et réduction](../receipts/audit_gpu_scratch_20261004/cuda/README.md).

**Copies parallèles : destinations et joins relus.** Ramassage par ordinal
avec préfixes jobs/sites séparés ; copie batch par tranches aux places fixes,
puis suffixe du repli. Les callbacks terminent avant destruction du contexte
ou téléchargement des pages touchées. **271 contrôles de transport**, aucune
preuve TSan ni gain matériel déduit. [Sources et modèles](../receipts/audit_gpu_scratch_20261004/gather/README.md).

**Porte scratch : instrumentation livrée57dd21be1.** `copied_jobs` est
exporté et la porte exige >0, en plus de fill_jobs>0 à K10. Le contre-exemple
« feuille émettrice débordée + vide » motivait cette séparation ; aucune
panne réelle de la fixture de 3 000 sites n'était établie. Le nouveau rejeu
natif/GPU reste à qualifier. Le ledger reste logique : les seules débordantes
sont réénumérées ; J2 recalcule aussi ses hits. [Témoin causal et limites](../receipts/audit_gpu_scratch_20261004/compact/README.md).

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

- **q3 anticipé dans les captures b872** : cette voie construisait le Level
  avant propriétaire/census/canon, contrairement à la v10. Le port **562**
  diffère désormais la même formule brute. Les captures de temps ci-dessus
  précèdent ce port : elles ne mesurent pas encore son gain isolé.
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

## Nouvelles captures CPU : portée du reçu 61da

**126 prises A/B** des sessions claudeab8/claudediag1 ont code0, statut ok
et dump égal à leur référence. Elles exécutent **54c167bb6 et e49ea4690**,
pas 61da. claudeab8 est completed (673 portes) ; claudediag1 reste
failed_remote/verdict refus : 676/678 portes, huit TSan et sept mutants
passent, seules deux portes de style échouent. Les deux
arrêts ciblés sont certifiés dans les reçus. Les résultats conservés ne
promouvront pas silencieusement ce second refus en qualification globale.
[Lecture et sources des mesures](../receipts/audit_gpu_euler_20261004/mesures_bindings/README.md).

| Diagnostics CPU u21/W48, mur FULL ms | ng00 | ng01 | ng02 |
|---|---:|---:|---:|
| K5, feuilles16, affinité libre | 408,4 | 296,1 | 359,7 |
| K10, feuilles16 | 3 284,7 | 2 506,4 | 2 738,8 |
| K10, feuilles24 | 2 506,1 | 1 822,5 | 2 064,5 |

**Ces 48 prises de diagnostic n'enregistrent pas de hash de dump.** Leur
statut ok et leurs médianes sont recoupés ; l'identité canonique n'est
établie que pour les 126 prises A/B. Le README développeur **corrige cette
portée au pin22**. Les K10 restent au-dessus d'une
seconde ; aucune nouvelle qualification GPU ou 100 ms n'en découle.

À W1, une seule paire par trame indique environ −1 % pour q3 différé,
neutralité de R/R1 et +1 % pour M3/E4 : diagnostics, sans variance estimée.
À W48, cinq paires et le bras A/A ne permettent pas de gain qualifié à 5 %.
Le rapport W48 libre/W24 épinglé change **workers et affinité** : il mesure
ces configurations, sans attribuer seul le gain au SMT ou à une attente
mémoire. Comparer à affinité commune et mesurer occupation/attentes avant
cette attribution. Les sorties actuelles du lecteur dérivé conservent
correctement le contexte et le refus parent.

**Diagnostic GPU5, source16, snapshot des métadonnées.** W48, trois trames :
à K5/leaf16, chaud CPU 371,5/278,4/336,8 ms contre GPU 422,0/346,6/387,1 ms.
À K10/leaf24, CPU 2454,0/1816,0/2064,8 ms contre GPU 2373,7/1778,3/1993,2 ms :
écarts descriptifs de −2 à −3,5 %, pas un gain statistique. Une série chaude par
bras, passes corrélées ; identité enregistrée seulement pour le dernier dump.
Le domaine GPU5 gagne 44–75 ms à K10 ; la forêt reste à 1,17–1,63 s.
**Suite : feuille coopérative GPU, puis forêt K10** ;
accélérer le seul count des feuilles ne ferme pas le contrat. Les phases
chevauchent et leurs médianes ne s'additionnent pas.
Neuf rapports GPU3–5 : 300 dumps froids/72 derniers chauds, 384 événements de passes
complets. Archives/binaires non rejugés, aucune suite numérique/mutants dans
ces plans de mesure. Les cinq reçus déclarent leurs arrêts historiques ;
aucun état actuel de VM vérifié ici. Compression22 non exécutée par ces lots GPU3–5 ; le nouveau reçu GPU6
(source22) est examiné séparément ci-dessous.
[Instantané clos, lectures et paramètres](../receipts/audit_gpu_scratch_20261004/gpu_receipt_triage/README.md).

**Réponse F, reçu GPU6 publié c645b1aab.** La compression22 a désormais été
mesurée : ne pas redemander cette ablation. Son bénéfice sur retour/Level est
partiellement payé par les recherches de rangs locaux au count. Pour J3,
porter directement les indices locaux des générateurs et les masques I/U,
sans inverser les SiteIdx par recherche. Conserver les certificats, qmin,
contacts, propriétaire, récursion q4 après q3 rejeté et repli entier de toute
feuille non certifiée. Contrat des compteurs D/R1 maintenu : métriques
physiques séparées ; copie nonvide et rejeu observables. Une réduction de
la divergence ou du mur des feuilles reste à confirmer sur FULL.
Le lecteur public passe **553 contrôles normal/−O** ; GPU6 est bien source22.
K5 reste favorable au CPU ; à K10, forêt≈1,17–1,64s dans ce lot. Les tableaux
publics mélangent explicitement meilleures passes chaudes et médianes froides :
ne pas les comparer comme une seule statistique. Le profil Nsight soutient
la piste divergence/mémoire locale, mais **ALU24% n’exclut pas un coût critique
de l’i128** (latence, dépendances ou registres) ; adoucir l’attribution exclusive.
Portes natives q3 extrême/q4 seuil/préfixe obtus/contact qmin2 toujours ouvertes.
[Contrelecture bornée du nouveau reçu](../receipts/audit_gpu6_receipt_20261004/README.md).

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
Ils peuvent former une antichaîne ; la tête plate publiée à k5/mcs20,
z=1/z=2 donne pourtant IoU0,529/0,500 et ne retrouve pas ces deux meilleurs blocs.
Une coupe commune reste à vérifier. Les lots mesurent des meilleurs blocs
sur des extraits choisis par annotations, pas le contrat de trame entière.
[Lecture des sessions et témoin](../receipts/audit_deep_20261004/README.md).

## Contrat natif préparatoire — état historique avant S9/S10

Les paragraphes ci-dessous conservent les exigences et les limites des
pins Python cités. Les modules natifs S9/S10 sont désormais livrés et
leurs différentiels P9/P10 passent, comme établi en tête de cette note.

**Tête E1 effectivement livrée, portée Python.** Les sessions
claudeflat1a/1b confrontent export FULL C++ → projection, condensation et
sélection **Python** à l'oracle indépendant ; leurs neuf mutants sont
Python. Le reçu développeur dit « sortie plate en natif » : le lieu G4
et l'export natif ne qualifiaient pas à eux seuls une tête C++, alors absente.
Le dev synthétique a fixé **z=2** sur 192 scènes ; le protocole P08 distingue
z=2 pour les fusions et z=1 pour la non-infériorité IoU. Ces observations
et choix sont distincts des tests encore à venir.

**E1 corrigé dans les sources 723cf6e43.** H_L2 applique maintenant Holm
ET IC basse strictement >−0,02 ; les deux contre-cas sont refusés, autres
primaires conservées. **94 gardes AST + dix contrôles stdlib**, normal/−O.
La porte inclut z=2 et F4b, où son attendu diffère de z=1 ; la nouvelle
qualification G4 reste attendue avant les mesures primaires. L'`oracle_m05`
supervisé des campagnes n'est pas l'oracle indépendant de correction EOM :
préciser cette phrase dans `SORTIE_PLATE.md` §4. Le préenregistrement
synthétique conserve encore l'attribution exclusive à la sélection sous
T−A non significatif : voir la note mathématique. Aucun P08 rejugé ici.
[Recoupe et décalage du protocole](../receipts/audit_ports_20261004/README.md).

Arbre de points N-aire, après suppression des vides/unaires : **≤2n−1 nœuds**.
Le produire depuis FULL et les attaches, sans matrice n² ni liste de membres
par ancêtre. DP : score/décision par cluster, puis un passage d'émission des
labels. Compter ensemble arbre, dates, scores, scratch, IDs/labels et FULL.
Le catalogue et les coquilles n'ont pas de borne linéaire universelle.
À cette étape Python E1, la construction native restait un plan ; les
implémentations et qualifications S9/S10 décrites en tête le remplacent.

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
