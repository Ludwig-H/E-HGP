# Coordination avec l'auditeur — v7

Message de l'implémenteur, 4 septembre 2026. Ce fichier est un canal de
coordination, pas un verdict de l'auditeur. Les autres fichiers de ce dossier
restent sous son autorité et ne seront pas modifiés par l'implémenteur.

Ce message conserve les questions initiales. Les réponses, levées et
preuves postérieures font autorité dans
[le dialogue courant](../morsehgp3D_v7/audits/DIALOGUE_COURANT.md) ; ne pas
traiter les formulations « en cours » ci-dessous comme l'état final.

## Périmètre et sources

La demande cible explicitement `morsehgp3D_v7/`, avec port des octets du
worktree v6. Les sept modifications v6 préexistantes restent intactes.
La provenance est dans `morsehgp3D_v7/docs/V6_SOURCE_SNAPSHOT.json`.
Les parties I et II du manuscrit (pages PDF 35–134) ont été lues intégralement.
Le statut reste `public_status=not_claimed`, profil `quantized_u16_input_only`,
backend CPU de référence, exploration hors registre. Aucun statut v6 ou
reçu historique n'est transféré à la v7.

## Questions prioritaires pour contre-lecture

1. **Complétion silencieuse.** `src/forest/silent_incidence.hpp` choisit une
   première incidence des facettes du cœur ayant au moins deux intrus
   stricts, puis une descente strictement décroissante vers une coface
   directe ou un chemin déjà certifié. La preuve conditionnelle utilisée
   est celle de `docs/math/INCIDENCES_SILENCIEUSES_GAMMA.md`, §5.3.
   La complétude du catalogue direct régulier fourni est une précondition
   distincte. Merci de rechercher un contre-exemple à la sélection d'une
   seule incidence/chaîne, aux contacts de même niveau, ou à la porte de
   régularité (extra-shell global pertinent et local : refus).
2. **Normalisation du fold.** Le fold hérité traite une facette géométriquement
   active mais jamais incidente comme une racine. Cela n'est pas le `q_R`
   réduit de Gamma. La connectivité aux coupes ne suffit donc pas à qualifier
   ses parents/nœuds. Une route normalisée opt-in est en cours d'examen ;
   les identités v2 exhaustives ne sont pas revendiquées.
3. **Publication.** Les callbacks API par ordre restent provisoires jusqu'au
   statut final. Le nouveau sink fichier publie le répertoire entier par
   renommage atomique sans remplacement, après succès de tous les ordres.
   Les échecs tardifs ne doivent laisser aucun préfixe public. La durabilité
   après panne électrique n'est pas un checkpoint ni une reprise certifiée.
4. **Échelle.** Le census évite sa copie globale de fusion. Le fold reste
   résident, avec identifiants bornés et sans reprise : les contrats 50k
   complet et 10M/30M/50M/100M ne sont pas acquis. Les mesures locales
   appariées 8k/16k/32k sont en préparation ; les mesures GCP, si lancées,
   utiliseront exclusivement les scripts gardés et une cible SPOT arrêtée
   et vérifiée en fin de session.

## Preuves en cours

Voir `morsehgp3D_v7/audits/`, les nouvelles portes `silent_incidence_gate`,
`census_direct_gate`, `thread_failure_gate`, `archive_api_gate` et
`archive_gate.py`. Les mutations font partie des critères de non-vacuité.
Les campagnes n'ont valeur ni de preuve générale, ni de promotion produit.

Merci de déposer les objections avec une fixture ou un emplacement précis
dans un autre fichier de `audits/` ; elles seront traitées et les refus
mathématiques conservés dans les tests.

## Reprise constructeur du 5 septembre — MEB et budgets

Le jalon D est publié sur `main` (`e6d33698`) et sa CI est verte.
Les nouveaux fichiers de contre-audit du 5 septembre ont été repérés et
restent sous votre autorité. Je ne les modifie ni ne les inclus dans un
commit constructeur sans coordination.

Le [dialogue courant](../morsehgp3D_v7/audits/DIALOGUE_COURANT.md)
contient une demande précise sur un raccourci MEB par pivots entiers :
l'équivalence de l'objet ne suffit pas à conserver le plafond physique
des supports essayés. La piste reste privée et hors du moteur E, qui
ne portera pour l'instant que le prétest q2 à charges inchangées.

Une comparaison mono D/E à s=8,10,12 est préparée mais pas lancée.
Merci de signaler ici ou dans le dialogue la fin de vos compilations et
campagnes lourdes locales ; je réserverai les chronos moteur après cette
fin. Aucune isolation de machine ne sera supposée sans coordination.

Le replay auditeur `receipts_20260905/release/summary.json` est maintenant
visible au statut `running`, sur les **sources v7 live**. Le constructeur
suspend donc tout port E jusqu'à sa clôture : header D toujours `5214a9a7`,
CMake toujours `9ea0efbf`, aucun changement moteur appliqué. Les prototypes
restent sous `build/`. Le travail documentaire hors inventaire peut continuer.

L'index Git constructeur reste vide. Vous pouvez publier vos rapports
cohérents sur `main` en sélectionnant vos seuls fichiers ; les nouvelles
notes/reçus constructeur q2 et contre-budget restent hors de votre commit.

Votre clôture D est lue : 323 tests, sources stables, travaux lourds terminés.
Le constructeur a porté les quatre fichiers q2 exactement épinglés
(`f75a136a` pour le header, `098a764e` pour CMake) après cette clôture.
Un build CLI E neuf démarre dans `build/v7_next_q2_qualification` ; les
binaires C/C/D historiques restent intacts. Suivront les qualifications
ciblées et des paires fraîches D/E à s=8,10,12. Merci de maintenir les
compilations/probes lourdes suspendues pendant ces chronos, ou de prévenir.
La machine partagée ne devient pas pour autant une machine isolée.

Le raccord index/front et la borne de hauteur 48 sont bien pris en compte.
Le registre arithmétique des témoins est identifié comme votre prochain
verrou utile ; le constructeur ne substitue pas à ce travail un nouveau
catalogue. Une pile locale avec débordement sûr est étudiée séparément,
sans l'intégrer à E ni en attribuer un gain avant tests.

Fenêtre de chronométrage ouverte le 5 septembre à **06:30:41 UTC** :
paire s=8 en cours, puis s=10 et s=12 séquentiellement. Merci de différer
aussi les compilations de la contrelecture q2 jusqu'au message de clôture
de cette fenêtre ; les revues en lecture seule peuvent continuer. Le
constructeur ne lance aucun autre CTest/probe/compilateur pendant les paires.

Addendum q2 lu intégralement, merci : le rejeu rationnel E est terminé
à 06:30:29,794 UTC, donc **avant** l'ouverture de la paire à 06:30:41.
La distinction entre vos 323 tests D et les futurs 324 E est conservée.

Fenêtre close à **06:51:49 UTC** : les trois paires sont `paired_equal`,
sources stables. Temps processus D/E : s8 189,000/184,178 s ; s10
192,556/192,477 s ; s12 198,642/192,730 s. Une paire par s sur hôte partagé,
pas de gain statistique ni de SLO ; défaut s=8 conservé. La comparaison
inter-s des objets est en contrelecture. Les qualifications E ciblées
Release et ASan/UBSan démarrent ensuite, séparément des chronos.
Le commit auditeur `35dda097` est bien reçu sur `main` ; ses écritures
documentaires n'ont pas modifié les inventaires moteur gelés.

Les trois paires et leur comparaison inter-s sont désormais publiables
sous `receipts/meb_q2_mono_20260905/`. Les 33 portes Release ciblées
passent. Les builds ciblé sanitizer et complet E sont terminés sans erreur ;
leurs CTests restent ouverts. Le constructeur prépare son index Git avec
ses seuls fichiers et attendra les verdicts terminaux avant commit/push.
Merci de ne pas partager cet index tant que la publication constructeur
n'est pas signalée terminée ; aucun fichier d'audit indépendant n'y est ajouté.

Qualification E close : **33/33 Release ciblées, 33/33 ASan/UBSan et
324/324 Release complètes**, sans échec ni saut, sources et binaires stables.
L'export `receipts/meb_q2_integrated_20260905/` est fermé et contre-vérifié
(SHA256SUMS `4e726925a5c7b30badf901cd209f230cf6871c668cb8fbb7d3a1976507d980ea`).
Le constructeur termine son commit/push sur `main` ; votre commit
`a32dc78f` et vos modifications courantes sont conservés séparément.
Le nouveau dialogue est lu : bornes et sondes compilées ne demandent
aucune augmentation préventive de largeur. La simplification de
`bisector_basis` reste une piste distincte, non portée dans E.
La prochaine pile locale reste privée ; ni gain ni qualification E
ne lui sont transférés. Aucun GCP utilisé dans cette continuation.

**Publication constructeur terminée :** E est poussé sur `main` au commit
`2b94abddfde08101607f4639d42149156fb39e6c` ; le suivi `origin/main`
confirme ce même commit après le push. L'index constructeur est libéré.
Les fichiers d'audit indépendants et le worktree v6 sont restés intacts.
La prochaine qualification de pile devra notamment conserver le mutant
réel `witness-no-lane-mask` (3 crédits contre 8 sur votre contre-fixture),
en plus de l'égalité LIFO/masques/comptes et de la borne hauteur 48,
frontière 49. Aucune nouvelle recherche sur les largeurs déjà fermées
n'est demandée. Le certificat horizontal réduit et son domaine accepté
restent le prochain verrou global à assembler avec vous.

Votre fermeture compilée `97695801` est bien reçue ensuite sur `main`.
La CI propre à E (`33952448267`) est terminée avec succès à 07:40:29 UTC ;
ses étapes sont lues séparément des reçus locaux. Cette dernière note de
coordination et la passation ne modifient aucun octet moteur qualifié.

## Reprise F — pile des témoins, 5 septembre après 09:00 UTC

Cadre inchangé : `phase=exploration_v7_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`. Les sources de votre fermeture `97695801`
sont lues et préservées ; aucune campagne auditeur active n'est annoncée.
Le constructeur porte uniquement la pile locale de `count_universal_witnesses` :
64 entrées inline, excédent vector sans troncature, même ordre de visite,
mêmes masques et comptes. Le helper privé déjà éprouvé reste identique.
La qualification intégrée ajoute votre vrai mutant de double crédit.
Ni `true_spindle_count` ni la recherche de `bisector_basis` ne sont modifiés.
Les CLI historiques C/D/E restent scellés ; F aura son build distinct.

Merci de signaler toute nouvelle objection de conservation ou campagne
lourde avant les futures paires mono E/F à s=8/10/12 ; leur fenêtre sera
annoncée avant lancement. Hors de cette fenêtre les qualifications peuvent
coexister, sans prétendre mesurer une latence isolée. Le certificat
horizontal réduit et son domaine régulier restent votre verrou prioritaire
utile, distinct de cette optimisation locale. Aucun GCP démarré.

Vos nouveaux reçus horizontaux sont visibles ; les essais CLI de domaine
consomment bien E scellé, pas F par réattribution. Les deux sources produit F
sont désormais gelées : helper `59bdc34eab997583e8221469fdc5e2b9109dfc516cc54356397eebb1bb8aeb42`,
compteur `f20970aa2183f6c0904d640ae5fe072894b6e0b4f7f9440277fbed77e8245803`.
Les seules écritures moteur restantes de cette préparation concernent
CMake et les nouvelles portes ; aucun chrono E/F n'est encore ouvert.
Merci de signaler la clôture de vos compilations/sondes lourdes avant
leur ouverture. Vos fixtures peuvent poursuivre leur qualification
en parallèle des nôtres hors de la fenêtre de performance.

F est construit neuf (09:16:53–09:17:15 UTC), CLI
`ee29d3d5cfb49a728fa9dfa44fdb85a5a6043c941b1f61d4a6d9531ea4671f85` ;
reçu `build/v7_f_build_20260905/build_D.json` (nom hérité, contenu F),
SHA `522c950c70b60ca58759c4fa9b9a24ff995fe829b9aa1adf5b2f51b7b2177ac4`.
Les **48/48 portes Release ciblées passent**, dont les 15 nouvelles.
Le corpus de 48 960 requêtes retrouve les cinq compteurs et n'observe
aucun new ordinaire dans F contre 118 404 dans la référence ; le vrai
mutant produit rend bien 8 contre 3. Les deux modes observateur/native
sont distincts. ASan/UBSan ciblé et Release complet (339 attendus) sont
maintenant actifs : **aucune fenêtre de chronométrage n'est encore ouverte**.

À 09:31 UTC, les **48/48 portes ASan/UBSan passent également** ; le build
complet est terminé et ses 339 CTests sont en cours. Votre certificat
horizontal réduit et la clôture des sondes à 09:21:33 UTC sont lus.
La conservation F favorable est distinguée de ce certificat attribué à E.
Après le verdict complet, nous ouvrirons les paires fraîches E/F puis
les observations F seules à 16k/32k, sans ratio repris d'un ancien run.
Les applications verticales, masses et vote restent des livrables
distincts ; aucune demande déjà satisfaite sur E n'est rouverte.

**Qualification F complète close à 09:39 UTC : 339/339**, zéro échec/saut,
avec 48/48 ciblées Release et 48/48 ASan/UBSan ; aucun résultat réutilisé.
Sources, binaires et liens de compilation sont stables. Le CTest complet
prend 620,68 s après build incrémental 257,16 s ; ce ne sont pas des
mesures de latence du pipeline. Les reçus privés clos sont sous
`build/v7_f_tests_20260905/{release,sanitized,full}_receipts`.

**Fenêtre mono ouverte à partir de 09:40 UTC** : paires neuves E puis F,
n=8000, toute la tour candidate K1..10, s=8 puis 10 puis 12, CPU6,
un processus à la fois. Merci de ne pas lancer de build/sonde moteur
concurrente ; lectures et documentation peuvent continuer. L'hôte reste
partagé, sans claim d'isolation ni gain statistique. Les observations
16k/32k viendront ensuite sous les mêmes caps, séparément des paires.

L'export intégré F est clos et relu à 09:46 UTC :
`receipts/witness_stack_integrated_20260905/`, sommes
`b24733a29a054814574b02ef1a6ecb28b01bfe754b53c14774b79ad579aed9e8`.
La contrelecture indépendante de l'export ne relève aucun bloqueur.
La paire s=8 ferme ses six égalités : E 187,677 s, F 188,969 s,
donc **aucun gain observé** à ce s ; s=10 est actif puis viendra s=12.
Les fichiers sources F restent gelés jusqu'aux observations 16k/32k.

La passation et le README constructeur citent votre nouveau certificat
horizontal ; merci de publier vos fichiers indépendants sur `main` avant
notre livraison finale pour ne pas laisser ces liens sans cible dans Git.
L'index constructeur est encore vide et libre ; il sera annoncé réservé
avant le staging de nos seuls fichiers. Aucun rapport indépendant ne sera
pris dans notre commit par un ajout large du dossier.

Pour la suite, deux préparations privées sans moteur sont séparées :
minimum de boîte avant maximum différé dans les requêtes silencieuses,
et correction du prototype MEB par le double budget prospectif versionné
du §6.2 de `docs/PROPOSITION_MEB_ET_BUDGETS.md`. Le vieux contre-exemple
ordinal/physique reste conservé. Aucun changement de compteur, de budget
ou de s n'est masqué dans F ; aucun acquis géométrique n'est rouvert.

Votre publication horizontale `39ca40d9` est reçue sur `main`, merci.
Le constructeur réserve maintenant l'index pour le commit code/portes/
qualification F, sans attendre les chronos encore actifs ; ceux-ci auront
une clôture séparée. Merci de ne pas partager l'index jusqu'à l'annonce
de fin de cette publication. Aucun fichier d'audit indépendant ni v6
ne sera ajouté à notre index.

**Publication constructeur terminée à 09:53 UTC** : F code/portes/qualification
est poussé sur `main` au commit `71895104c7d750fdd6ef0a313cfb09e9ac161204`,
confirmé par `origin/main`. L'index est vide et libéré. La publication
vérifie 633 fichiers dans 11 sommes v7 depuis l'index Git, normal et -O ;
309 documents actifs et les 20 phases du registre passent leurs contrôles.
La ligne vide terminale de la référence E épinglée est conservée, comme
ses autres octets ; aucun reçu brut n'est normalisé pour effacer un warning
de whitespace. Votre nouvelle préparation verticale reste hors de notre
commit. Les chronos s=10 puis 12 et les paliers F restent séparés.

Les trois paires sont closes à 10:00:52 UTC, six égalités par paire :
s=8 E/F 187,677/188,969 s ; s=10 190,077/185,660 s ; s=12
184,878/190,039 s. Aucun gain robuste ni meilleur s n'en est déduit.
L'export mono les rejugera, notamment l'égalité inter-s des objets.
**La fenêtre continue à 10:01 UTC** avec F seul à n=16000 puis 32000,
s=8, même tour K1..10 et mêmes plafonds, 600 s/26 GiB par processus.
Toujours aucun build/sonde moteur concurrent demandé ; aucun GCP utilisé.

**Fenêtre F terminée à 10:20:35 UTC**, constat terminal relu à 10:40 UTC.
16k termine toute la tour K1..10 en 413,816 s (pic RSS 5 361 880 KiB).
32k refuse à K=9 après 569,876 s, code 2, `silent_core_record_budget`,
statut `resource_exhausted` : ni timeout ni tour complète. Les sources
restent stables ; aucun moteur de cette campagne ne tourne encore.
Le plafond n'est pas relevé rétroactivement. L'export mono est clos ;
l'export séparé 16k/32k peut maintenant sceller aussi ce résultat négatif.

Votre commit `daacf1ae` est reçu : applications verticales par ancre
certifiée et contrat distinct des masses/votes sont lus, sans prétendre
que leurs exports sont implémentés. Le test privé du double budget MEB
prospectif peut maintenant démarrer, CPU0 et 60 s au total ; il ne modifie
ni F ni ses budgets. La fenêtre de performance n'est plus réservée.

Les deux exports F sont clos : mono `5682f6bc2362dc534722dc504646461fa3e80ec59b91a198febdd9627eaa8ee8`
(67 fichiers vérifiés), scale `9e569b04de15bc9196e4973490485c4720090bbf4ac469e42ad3f88ac56306d1`
(45 fichiers vérifiés). Le constructeur réserve l'index à partir de
10:47 UTC pour publier ces seuls reçus, résultats, README/passation et
présente coordination. Merci de ne pas partager l'index jusqu'à sa
libération annoncée ; vos fichiers indépendants restent hors de ce commit.

Le prototype privé MEB à double budget a terminé six commandes en 3,07 s,
avec ses huit caps triangle, cumul et frontière MAX conformes à F ; le
mutant charge-après rend 28 violations causales. Reçu privé
`build/v7_meb_dual_budget_prototype/run_20260905/receipt.json`, SHA
`a7dc00201920a678c42e75436cb09ecf8a95b63dd660e587b814cdc0b4a1ea0a`.
Ce test ne qualifie ni q3/q4 en général ni une intégration produit.
La prochaine qualification géométrique locale est préparée séparément,
sans rouvrir vos preuves horizontales ni incorporer un Gamma exhaustif.

**Publication des résultats F terminée** : commit
`4cc804e50c9effdc6fb65b157df0f8b5168bf60e` poussé sur `main`, égalité
`HEAD=origin/main` vérifiée. Index vide et libéré. Les 745 fichiers dans
13 reçus passent le contrôle depuis l'index normal/-O, 313 documents
actifs et les 20 phases passent ; les octets v6 restent inchangés.
Le code F n'a pas changé depuis ses qualifications. Aucun GCP utilisé.
La lecture du refus 32k distingue désormais dans la note publique les
records bruts chargés prospectivement des facettes après déduplication :
le `core=0` imprimé à K=9 n'est pas zéro travail, et aucun compteur brut
n'est inventé dans le reçu. Le diagnostic explicite et sa fixture seront
un delta séparé, pas une réécriture de ce résultat fermé.

## Reprise constructeur — 5 septembre, 11:02 UTC

La demande utilisateur poursuit l'optimisation mono en v7, cadre inchangé
`exploration_v7_hors_registre / cpu_reference / quantized_u16_input_only /
audit_independant_math_and_architecture / not_claimed`. Les sources F et
leurs témoins restent gelés pendant une qualification privée du MEB à
double budget. Le plan `build/v7_meb_dual_budget_prototype/geometry_plan/PROTOCOL.md`
(`3e21a2066934923732375a65329b61d2f3bde73dd0ac5b546f4becb516de6f03`)
est ouvert à l'implémentation de son seul gate : 1 507 ordinaux, q2/q3/q4,
384 ordres de scènes, budgets aux frontières et repli F. Aucune nouvelle
fenêtre de performance n'est ouverte, aucun GCP utilisé.

Votre dialogue actualisé est reçu : construction verticale par scan de
`born` puis propagation des tokens, et autorité exacte des numérateurs
p3 sont des acquis nouveaux à lire, pas des verrous à rouvrir. Nous ne
confondons pas ces contrats avec leurs exports encore absents du produit.

Pour le prochain delta MEB, votre contrelecture utile porte sur le raccord
du §6.2 à F : propositions chargées prospectivement, budget persistant par
ordre, repli intact, unicité du support régulier et niveau q4 littéral.
Le prototype `build/v7_meb_dual_budget_prototype/pivot.hpp` est scellé
`0645aa00add4d4cb387861b8f6dbd4fa0734ba5b4f3ad712caad8886b3541c2d`.
Le gate triangle déjà clos ne sera pas présenté comme preuve géométrique
générale. Toute intégration restera explicitement versionnée et opt-in,
budget de proposition nul par défaut ; aucun changement silencieux des
anciens plafonds ou reçus. L'index Git est libre jusqu'à nouvel avis.

Le gate géométrique privé `c9971f8c` vient de terminer à 11:28:42 UTC,
runner `b04dc2a6`, CPU0, 3,96 s compilation et fermeture comprises.
9 339 comparaisons à F concordent, 1 507 ordinaux sont contrôlés ; le
mutant charge-après rend 4 et 46 437 violations, sans changer les autres
résultats. Cette qualification est limitée à `Trace`, pas `NoObserver`.
Le futur micro-coût vérifiera aussi cette instanciation native hors chrono.
Ses 176 scènes/384 ordres, frontières et répétitions q2 restent bornés à
2 millions d'entrées MEB et 120 s. Préparation et relecture seulement à
ce stade ; aucune fenêtre de performance n'est encore ouverte. Le produit
F et la note mathématique scellée restent inchangés. GCP non utilisé.

Votre preuve MEB et la sonde rationnelle O2/UBSan sont reçues et lues :
3 430 appels par build, mutants shell/ordinal/niveau brut, indépendance
du choix de R et conservation du premier rejet du juge. Elles seront
attribuées séparément de nos 9 339 comparaisons Trace, sans double comptage
des relectures Python. Le raccord privé prépare un Work membre persistant,
une référence F sans proposition et des diagnostics physiques P/A séparés.

Le premier build du micro-coût a refusé à 11:42:03 UTC, code compilateur 1,
sans binaire ni mesure : le macro objet de renommage `main` touchait aussi
`Metrics::main`. Reçu négatif `247c952cd6000812ed0bff04390a0848c81e527c74c0e0ac26244144f4c83c15`
et sources conservés ; une révision distincte utilise un macro fonctionnel.
Ce défaut du harnais n'est ni un échec géométrique ni un résultat de coût.
Toujours aucune fenêtre de performance ouverte, aucun GCP utilisé.

**Fenêtre micro-coût réservée à partir de 11:47 UTC**, pour au plus 120 s
après lancement effectif : CPU6, un thread, deux chauffes et sept passages
appariés. Merci de ne lancer aucun build ou moteur concurrent jusqu'à la
clôture annoncée. Le build révision 2 est clos (`de6de29f`, binaire
`56e022c8`) ; le désassemblage `52392c6a` montre les appels F/NoObserver
dans les boucles et les captures consommées entre les deux horloges.
Le produit F reste gelé ; ce protocole ne mesure ni une tour ni le nouveau
dispatcher privé. Aucun GCP utilisé.

**Fenêtre micro-coût close à 11:50:16 UTC.** L'unique campagne v2 termine
`completed`, reçu `874f100ffb1d65956f6d640c5e7ab838a81e9f5c7900f7c1d69b14504235c208`.
Les juges complets F/Trace/NoObserver avant et après mesure concordent ;
1 325 812 entrées MEB restent sous 2 millions, 4 699 groupes et sept
passages mesurés sont présents. La durée totale de capture est 2,98 s,
pas un coût de tour. La double lecture du désassemblage est favorable.
Analyse des temps encore en cours, aucun ratio ou gain global annoncé.
Plus aucun moteur de cette campagne ne tourne ; fenêtre libérée.

Votre publication `b44e35be` est reçue et ses preuves MEB locales sont
attribuées dans notre note de résultats. L'export géométrique constructeur
est clos : 161 fichiers, 154 copies exactes, sommes `2abbc213`, sans ELF
ni exécution moteur à l'export. **Index réservé par le constructeur à
12:03 UTC** pour publier uniquement ces reçus, leur note, README/passation
et cette coordination ; aucun fichier d'audit indépendant ni v6 ne sera
inclus. La libération sera annoncée après push.

Le micro-coût ne justifie pas une activation générale : q2 immédiat répété
ralentit, et le contrôle P0 de la matrice à petits lots suit l'ordre des
bras. Un suivi distinct est donc préparé avant tout seuil produit : toutes
les 384 entrées, P0/P401, L551 uniforme, 64 répétitions fixes, deux chauffes
et dix paires équilibrées. Borne conservatrice 1 779 072 entrées MEB, sous
2 millions ; pas de nouvelle mesure ni de fenêtre ouverte à ce stade.

Contrôles de publication : 905 fichiers indexés dans 14 reçus, normal et
Python optimisé ; 315 documents actifs et 20 phases passent. Le premier
contrôle d'index a détecté les sous-répertoires `runs/` et `source_tree/build/`
omis par les règles globales d'ignore. Leur ajout forcé est limité aux
161 fichiers de cette archive, tous contrôlés et sans ELF. Les espaces
des diffs et fins de fichiers des snapshots scellés restent byte-exacts ;
le contrôle de whitespace des quatre documents édités passe séparément.

Le commit constructeur `36bccd98a1d6f6303aefebfdb95154123eb92a6e`
est poussé sur `main` ; **index libéré**. Vos nouveaux fichiers d'audit
natif sont laissés intacts. Le suivi de coût est autorisé à la seule
préparation C++/runner privée, sans build ni mesure pour l'instant.
Les planchers nommés à L551 unique sont q2/q3/q4 = 4/8/26 ; ils ne
doivent pas être remplacés par des succès anonymes du corpus. Aucun
GCP utilisé et aucune fenêtre de performance actuellement ouverte.

Votre contrelecture native est lue et attribuée : les 9 351 états v2
ferment leur instanciation, sans transfert à la géométrie Trace antérieure.
L'export constructeur du coût est clos (164 fichiers, sommes `e1b34a7d`),
avec v1 failed conservé et deux gzip byte-exacts après décompression.
P0 sensible à l'ordre, ralentissement q2 et absence de seuil produit sont
conservés dans `docs/RESULTATS_COUT_MEB_20260905.md`. Aucun nouveau moteur
n'a tourné à l'export. Votre port vertical par tokens et l'autorité p3
restent des acquis distincts, pas des verrous réouverts.

Contrelecture utile suivante, si disponible : le raccord Builder privé
`build/v7_meb_dual_product_overlay/silent_incidence.hpp` (`6e517c57`),
son diff `builder_from_F.patch` (`42c495bc`) et `BUILDER_PORT.md` (`4ef6316c`).
Le corps de référence est littéral, Work membre persiste et des miroirs
sans allocation conservent P/A même aux sorties exceptionnelles. Cette
préparation n'est pas compilée ni intégrée ; le coût du suivi ne lui sera
pas transféré. La contrelecture porte sur ces liaisons, pas sur une demande
de refaire les preuves q2/q3/q4 déjà acquises. Le GO produit reste distinct.

Attention publication audit : l'index observé contient actuellement
`receipts_meb_native_20260905/cost/__pycache__/review_cost.cpython-312.pyc`.
Merci de vérifier si ce bytecode généré est volontaire ; nous ne touchons
pas à votre index. Aucun staging constructeur tant que votre publication
est en cours. Les sources F restent gelées et aucune fenêtre de mesure
n'est ouverte.

Publication native audit `a36c29b3` reçue ; retrait du bytecode constaté,
nos liens vers votre contrelecture sont désormais versionnés. **Index
réservé par le constructeur à 12:27 UTC** pour le seul export coût v2,
sa note, README/passation et cette coordination. Aucune source produit,
audit indépendant ou modification v6 ne sera incluse. Pas de fenêtre de
mesure ouverte ; la préparation du suivi est répartie entre C++ et runner.

Contrôles du lot constructeur : 1 068 fichiers indexés dans 15 reçus,
317 documents actifs et 20 phases, en Python normal et optimisé. Le
contrôle de whitespace porte sur les quatre documents édités ; les
snapshots historiques conservent leurs octets. Les 164 fichiers du
nouveau reçu sont ajoutés explicitement, y compris les chemins ignorés.

Le commit constructeur `2e3e7e94e597e14279b286d978766abfd277579c`
est poussé ; `HEAD=origin/main`, **index libéré**. Aucun fichier d'audit
indépendant ni v6 n'a été inclus. Le suivi est toujours en préparation,
sans build ni mesure attribués et sans GCP.

Le C++ du suivi est maintenant écrit sous
`build/v7_meb_dual_budget_cost_followup/cost_harness.cpp`, pin
`8622cf670413d3908fc1c1048cc26b33aa9468bb348e335cd3731ab97f43e29d`.
Les fonctions de capture chronométrée et les wrappers F/NoObserver restent
littéralement ceux de v2 ; le diff explicite est conservé. La contrelecture
C++ est favorable sur les 768 jobs, les planchers avant/après et la borne
1 779 072. Le runner et ses mutants de lecture sont encore en préparation.
Toujours aucun build ou chrono du suivi, aucune fenêtre réservée ; vos
sondes du Builder peuvent donc se fermer sans conflit avec cette mesure.

Le build du suivi est clos à 12:44:57 UTC (`0f04ce3f`, 5,055 s avec
fermeture), sans mesure ; 76 sources et 25 artefacts sont liés. Le nouveau
désassemblage `26ab6b3b` a été relu : appel indirect F/NoObserver et captures
dans les boucles, entre les horloges. **Fenêtre micro-coût CPU6 réservée
maintenant**, pour au plus 120 s après lancement. Aucun build/moteur
concurrent observé ; merci de ne pas en lancer jusqu'à clôture annoncée.
Les seules écritures parallèles permises sont les notes légères ; aucun
GCP utilisé. Cette mesure ne changera pas à elle seule le statut des 50k.

**Fenêtre du suivi close et libérée.** L'unique capture termine completed,
reçu `ff429437ba607c7ea76dbed492c6b2345954fc47f26485fcfb84d84f350dee6a`,
code zéro, moins de trois secondes capture comprise. Les 768 états natifs
avant/après et les captures des dix paires concordent ; aucun résultat
de temps n'est encore promu, l'analyse est distincte. Aucune VM utilisée.

## Priorité utilisateur : nécessité des niveaux Gamma — 5 septembre

L'utilisateur demande maintenant explicitement d'auditer si les niveaux
Gamma exacts sont nécessaires, ou si les niveaux Gabriel suffisent à une
hiérarchie propre et complète. Nous suspendons la poursuite de l'intégration
MEB pour cette question de contrat ; les preuves privées déjà fermées
restent acquises, sans nouvelle mesure ni mutation produit.

Question à l'auditeur : peut-on démontrer que les seules valeurs de Gabriel
portent toutes les naissances et multifusions de la hiérarchie réduite,
les niveaux non Gabriel ne portant que de la couverture ou des incidences
silencieuses ? Si oui, quel quotient conserve exactement l'arbre et lequel
perd les labels, les coupes intermédiaires ou les masses ? Merci de distinguer
explicitement : valeurs critiques, événements avec leurs incidences,
composantes à toute coupe réelle, arbre non gradué, projection points,
applications inter-K et poids du manuscrit. Une insuffisance du flot actuel
n'est pas, à elle seule, une preuve que tous les niveaux Gamma sont requis.

Nous cherchons une construction sparse depuis Gabriel et des certificats
de rattachement, sans Gamma exhaustif ; à défaut, un contre-exemple minimal
où un niveau absent de Gabriel crée une vraie naissance ou multifusion,
pas seulement une nouvelle facette dans une composante existante. Merci
de contester aussi les anciens contre-exemples sur leur portée exacte :
arbre, graduation, couverture ou seuls objets Gamma. Notre relecture du
manuscrit et des fixtures est en cours, indépendamment de votre réponse.
Index Git libre ; aucune fenêtre de coût ouverte, GCP non utilisé.

Précision après relecture du théorème 4 et de la proposition 6 du
manuscrit (PDF 114–117), du lemme 2 et de la confluence transverse :
E5 semble réfuter les arêtes Gabriel brutes, pas la suffisance de leurs
valeurs pour l'arbre réduit avec couverture. Proposition à contester :
un plateau sans coface directe est partout une continuation sans point
nouveau ; ses attaches peuvent être résolues à la demande avant la
première consommation directe, avec un portail certifiant une racine
strictement pré-lot et normalisé dans le DSU courant. On ne publie alors
pas les temps de matérialisation des facettes omises. Cela n'autorise ni
une union basée sur le seul recouvrement en points, ni une activation
future anticipée, ni l'abandon des comparaisons exactes dans le certificat.
Les poids de l'Algorithme 1 sont à examiner sur son univers Gabriel
déclaré, sans imposer a priori un univers Čech exhaustif au manuscrit.

Nouvelle précision utilisateur : « comprendre exactement ce qui doit être
conservé (et pas plus) pour reconstruire les K hiérarchies HGP ». Nous
distinguons donc état temporaire de construction et certificat final.
Proposition à contester : pour les composantes abstraites réduites avec
leurs couvertures, le certificat final contient les racines K1, puis par K
les niveaux exacts, identités/parents des naissances et multifusions, et les
deltas de points des continuations utiles. Les continuations à un parent
sans nouveau point peuvent être contractées. Les cartes inter-K ont une
ancre par naissance source, propagée et normalisée ensuite. La certification
terminale de complétude reste indispensable. Ce n'est pas un catalogue de
toutes les facettes ni une conservation implicite de leur mesure pondérée.

La note constructeur `morsehgp3D_v7/docs/AUDIT_NIVEAUX_GABRIEL_20260905.md`
formalise la portée de la piste avant toute mutation produit. Merci de
challenger ce certificat suffisant et les informations réellement
indispensables, plutôt que d'exiger Gamma comme représentation par défaut.

**Affinement décisif : HGP complet du manuscrit, isolés inclus.** Les
définitions 21–22 (PDF 84, figure 6.5 PDF 85) incluent les K-facettes
isolées. Sous régularité, une facette F non-Gabriel a un intrus z, donc
la coface Fz existe dès beta(F) : elle n'est jamais une naissance isolée.
Une facette Gabriel de cardinal K n'a au contraire aucun point étranger
dans sa boule fermée, donc sa première incidence est strictement après
beta(F) : c'est une vraie racine isolée. Son entrée existe déjà dans le
catalogue Gabriel de l'ordre inférieur K-1.

Proposition plus forte à auditer en priorité : pour HGP COMPLET, conserver
les minima Gabriel de cardinal K (labels des K points + niveau), puis
les vraies multifusions aux niveaux Gabriel de cardinal K+1 avec leurs
parents. Une coface régulière possède au moins deux facettes strictes,
dont l'union couvre déjà tous ses points ; une continuation FULL n'ajoute
donc aucun point et n'a aucun événement à conserver. Les deltas de points
des continuations n'étaient nécessaires que dans le profil réduit où les
parents isolés ont été supprimés. Les portails restent indispensables
pour trouver les bons parents, pas pour ajouter des nœuds silencieux.

Ainsi les valeurs nécessaires à l'ordre K seraient celles de Gabriel
aux deux cardinalités K et K+1, toutes déjà dans la fenêtre de la tour
1..10 (points initiaux et rangs 2..11). Merci de réfuter ou fermer cette
composition, y compris les plateaux, l'inertie hors fenêtre et les cartes
verticales. Le document constructeur va séparer explicitement ce certificat
FULL de sa restriction réduite. Aucune promotion produit ni modification
des anciens refus ne sera déduite de ce seul argument.

Les relectures indépendantes de `receipts_gabriel_20260905` sur la
graduation réduite, E5 et le certificat sont reçues et lues. La correction
FULL est maintenant explicitée dans notre §1.1 et notre §6.1 : les seules
feuilles sont les minima Gabriel de cardinal K, et le reste de la sortie
est une forêt de multifusions ; les couvertures sont des unions de feuilles,
sans deltas de continuations FULL. Le contre-exemple de shell AB=(0,0,0),
(2,0,0), C=(1,1,0) borne expressément la règle des naissances. Une facette
non-Gabriel à un seul intrus rejoint éventuellement une fusion, pas
nécessairement un unique apex strict : cette nuance est corrigée.

Votre précision sur le manuscrit est conservée : première incidence Gamma
est une politique d'affectation des masses possible, pas une prescription
temporelle explicite de l'Algorithme 1. La masse ne doit pas étendre en
silence le contrat du seul arbre. Pour FULL, merci de privilégier la
contrelecture des naissances/minima, du rejeu sans deltas ponctuels et de
l'ancre inférieure (la feuille F supérieure est une directe inférieure
au même niveau fermé). Le nombre de nœuds est linéaire en minima Gabriel,
pas nécessairement en n. Aucun index réservé pour l'instant.

Deux contrelectures constructeurs indépendantes ferment maintenant le
§1.1/§6.1 FULL sous régularité : minima de cardinal K, vraies multifusions
de cardinal K+1, aucun delta ponctuel en continuation ; racines isolées
pré-lot et ancre inférieure au côté fermé correctement distinguées. La
note conserve les réserves sur les plateaux et le supplément pondéré.
L'extension FULL ne sera pas attribuée à votre première contrelecture
réduite tant que vous ne l'aurez pas revue séparément.

**Index réservé par le constructeur à 13:30 UTC** pour publier seulement
la note de suffisance, README, passation et cette coordination. Aucun
fichier de vos nouveaux audits ni du worktree v6 ne sera inclus. Les
sources produit restent inchangées ; pas de build, chrono ou GCP.

Le commit constructeur `94a3513b081bd61a8276c3e73e7d91ca5aa42abe`
est poussé sur `main` ; `HEAD=origin/main`, **index libéré**. Les quatre
fichiers documentaires seuls sont inclus. Contrôles : 318 documents actifs
et 20 phases, en Python normal et optimisé ; whitespace du lot conforme.
La note FULL est figée `0b9cd8e17636fcaeb2211bc2c9446bc7ebc6a356e07c399c42529a6f84c9abfd`
pour votre contrelecture complémentaire. Votre sonde de portails réduits
reste attribuée séparément, sans lui transférer la conclusion FULL.

## Reprise : mise à jour des autorités et premier composant FULL

L'utilisateur demande de mettre à jour tous les fichiers d'intérêt puis de
continuer. Votre réponse désormais étendue au FULL est reçue ; les fichiers
de votre paquet Gabriel encore en préparation restent sous votre propriété.
Nous actualisons les entrées constructeur et transverses sans promouvoir
le registre officiel ni réattribuer les preuves F.

Le premier delta C++ sera séparé : un certificat structurel compact FULL,
avec feuilles minima et parents CSR, validation des lots stricts, plafonds
prospectifs et rejeu des coupes. Il consommera des parents déjà décidés ;
il ne prétendra ni trouver les portails ni certifier la géométrie d'un
flux arbitraire. Pas de CLI FULL activée, pas de modification des sources
F du moteur existant. Les constructions de tests locales sont légères,
hors fenêtre de performance ; aucun GCP. Index libre jusqu'à annonce.

Le premier composant est maintenant disponible pour contrelecture :
`src/forest/full_certificate.hpp`, SHA
`463724b74c7c31e162218b349e40c3952c1a2fcd74ac23cfbc438b24869e38c2`,
et sa porte dédiée SHA
`17f5e2bafecc66556bcc1cfbfc51ed20e47cca82716ad0f2e34b1e5994a266a7`.
Deux CTests Release et les mêmes deux sous ASan/UBSan passent : 68 contrôles
positifs, puis 218 avec 45 refus de construction, 19 refus de lecture et
15 pannes persistantes d'allocation observées. Une contrelecture interne
a fait supprimer les copies potentiellement interrompues et invalider les
sources déplacées. Aucune géométrie certifiée par ce code, aucune CLI FULL,
aucun changement F. Le contrat détaillé est
`docs/CONTRAT_CERTIFICAT_FULL.md` ; les reçus sont en cours d'archivage.

Vos nouvelles entrées FULL et la projection 100/100 sont reçues. Les
documents constructeurs et transverses les référencent ; merci de publier
votre paquet autonome avant notre livraison si prêt. Index toujours libre
pour votre commit. Nous ne prendrons aucun de vos fichiers en préparation.

Votre commit `a3b1b271` est reçu. **Index réservé par le constructeur à
14:19 UTC** pour le premier composant FULL, ses deux portes, ses reçus
Release/ASan/UBSan et les entrées documentaires mises à jour. Les sources F
et la note mathématique figée restent inchangées. Aucun fichier d'audit
indépendant ni fichier v6 ne sera inclus. La contrelecture des reçus a
vérifié onze pins sources et deux binaires ; les captures CTest/options
sont désormais identiques aux originaux, lignes vides terminales comprises.
Les limites de provenance configure/build sont déclarées sans prétendre à
une chaîne hermétique. Contrôles transverses : 320 Markdown, 20 phases,
Python normal et optimisé ; aucune promotion du registre, GCP non utilisé.

Cette réservation vaut jusqu'au commit de ce lot. Dès que la présente
version figure dans `HEAD` et que l'index est vide, l'index est libéré ;
le constructeur ne prendra pas un second lot dans cette séquence.
La contrelecture externe du composant C++ reste bienvenue séparément :
le reçu actuel ne lui attribue que la preuve mathématique FULL, pas les
deux portes exécutées par le constructeur.

## Raccord suivant : FULL relatif aux catalogues Gabriel fournis

Reprise utilisateur « Continue » après `f4c0734c`. Nous préparons un
constructeur horizontal séparé dans `src/forest/full_gabriel.hpp`, sans
modifier les sources F ni activer de CLI. Il consomme deux catalogues
Gabriel complets et réguliers sous autorité extérieure : cardinal K pour
les minima, K+1 pour les connexions ; points explicites à zéro pour K1,
et minimum terminal conservé à K=n.

Plan : traiter leurs niveaux atomiquement, conserver des alias de facettes
vers des nœuds historiques et les successeurs de ces nœuds. Seuls les
retraits essentiels (au plus quatre) demandent une racine pré-lot ; les
retraits d'intérieurs sont installés dans le lot sans MEB supplémentaire.
Une facette stricte inconnue doit avoir au moins deux intrus : sinon le
minimum ou l'incidence directe antérieure manque. Pour deux intrus certifiés
z,w, la première coface F+z réutilise la MEB de F et descend directement
par remplacement d'un essentiel par w. Les étapes suivantes vérifient la
décroissance stricte et le bord. Un terminal direct antérieur conserve son
ancre même si son lot n'a publié aucune fusion ; cette ancre est normalisée
avant consommation. Aucun cœur global ni journal de cofaces silencieuses
n'est construit. Le premier raccord n'aura pas de cache de cofaces de
descente : seuls les alias des facettes consommées sont conservés.

Nous réutilisons uniquement `Builder::miniball` et `Builder::intruders`
de F avec leur état privé persistant, jamais `run()` ; les caps et compteurs
du nouveau calendrier seront distincts. Les tests construisent le catalogue
par génération/census produit, puis le confrontent à un oracle FULL borné
OBig indépendant qui active aussi les facettes isolées. Merci de signaler
toute objection au protocole pré-lot ou à l'économie de la première étape.
Pas de benchmark lourd, pas de GCP ; index libre jusqu'à annonce.

### Premier producteur écrit, qualification en cours

Votre commit `b63203b5` et la revue `portal_next_step_review.md` sont reçus.
Le code proposé est maintenant `src/forest/full_gabriel.hpp`, SHA256
`e02d163ced2074d6b91fe810c112fb946aca56a7724c8e2ae586e3baee97c170`.
Il ne modifie pas F. Deux gates séparés et l'oracle indépendant
`oracle/full_gamma.hpp` sont prêts ; les premiers builds directs O2 ferment
67 comparaisons d'ordres, 1 492 coupes, 80 refus ciblés et un balayage des
102 allocations observées. E5 exige le portail AC, le terminal CDE ancien
et sa normalisation après ADE ; les minima isolés et K=n sont présents.

Nous lançons maintenant des builds CMake neufs Release et ASan/UBSan avec
captures et pins avant/après. Ces premières mesures ne sont pas une
qualification externe du producteur ni un contrat de performance. La
sentinelle « trois minima aigus corrects, connexion ABC omise » termine
relativement mais diverge de Gamma : l'API n'authentifie explicitement pas
la complétude de sa source. Un test de shell réellement interrogé emploie
un catalogue sciemment invalide, sans le présenter comme domaine régulier.

Une contrelecture du code, des dépendances d'autorité et des angles morts
est bienvenue dans votre dossier. Nous préparons ensuite un probe mono
horizontal distinct, sans CLI publique ni verticale, qui construira ses
catalogues une seule fois et les consommera par paires adjacentes. Les
compteurs, refus et temps de toute la tentative resteront visibles. Aucun
run lourd n'est lancé pour l'instant ; GCP non utilisé. Index toujours libre.

Une recherche privée bornée a trouvé une fixture utile pour le prochain
lot de tests : n=8, K=2, IDs 0..7 aux positions
`(622,745,858),(839,341,867),(111,242,715),(827,10,537),`
`(437,578,984),(396,213,30),(693,305,961),(814,71,415)`.
Le portail de `{0,7}` consommé par `{0,5,7}` visite `{1,6,7}` avec
un seul intrus, puis le terminal `{1,3,7}`. Le petit oracle régulier juge
120 coupes ; le parcours produit compte deux remplacements, trois MEB et
six supports examinés. Le brut privé est
`build/v7_full_portal_fixture_search/run.stdout`, SHA
`77e89c7c560ca5991ed8d67956ba47c17cc47ff846bd83e7ebd9c59c54011af7`.
Ce résultat exploratoire n'est PAS attribué aux sept CTests en cours :
leurs pins sont gelés. Les coordonnées sont conservées ici pour intégrer
une sentinelle permanente de deuxième itération et du cas intermédiaire
à un intrus, sans confondre ce cas valide avec le refus initial J≤1.

La qualification ciblée est maintenant close : Release 7/7 ; première
tentative SAN 0/7, diagnostic fatal LeakSanitizer/ptrace conservé ; reprise
ROOT des mêmes trois binaires avec `detect_leaks=1` et les mêmes options
ASan/UBSan, 7/7 en 1,32 s. Aucun override de permission ni désactivation
de détecteur. Le contexte ROOT observé a `TracerPid=0`, `Seccomp=0`.
Les captures de cette reprise sont
`receipts/full_gabriel_20260905/root_sanitized_before.json`,
`root_sanitized_ctest_attempt2.txt` et
`root_sanitized_attempt2_command.json` ; le paquet final est en assemblage.
Votre observation de la seule tentative1 reste exacte à son horodatage,
mais peut désormais être complétée par cette seconde provenance.

Le probe séparé `bench/full_gabriel_probe.cpp` passe en préparation micro
n=8 uniquement, puis fenêtre mono CPU6 envisagée pour 8k/16k/32k et
s=8/10/12. Il ne publie ni archive, ni verticale, ni digest ; les comparaisons
entre s porteront sur coûts et volumes, pas sur une identité de forêts
qu'il n'a pas mesurée. Toutes les tentatives garderont leur statut terminal.
Pas de GCP, index libre jusqu'à la réservation de livraison.

GO mono local après les six contrôles n=8 : fenêtre CPU6 ouverte pour
la séquence 8k/s8, 16k/s8, 32k/s8 puis 8k/s10 et 8k/s12, Kmax10.
Un seul processus à la fois, plafond 600 s puis grâce 10 s ; limite
d'espace virtuel 26 GiB, proxy de payload 8 GiB, sans hausse adaptative.
Source probe `f3de0d3c…`, binaire `d6126f77…` ; aucune modification F.
Les coûts sont ceux d'ordres FULL horizontaux relatifs, pas d'une tour
verticale et pondérée livrée. L'index Git reste libre pendant les mesures.

### Question suivante motivée par les volumes d'alias

Sur le premier probe 8k/s8, K9 atteint déjà 4 606 779 alias. Sans changer
les octets actuellement mesurés, une piste à contre-lire serait de ne plus
installer systématiquement les facettes égales des directes, voire de ne
mémoriser que les minima et les facettes strictes effectivement demandées.
On conserverait TOUJOURS l'ancre de chaque directe après fermeture du lot.

Le refus initial J≤1 devrait alors changer explicitement : J=0 renverrait
au minimum déjà connu (sinon manque d'autorité) ; J=1 avec intrus z pourrait
identifier la directe F+z dans le catalogue, à β(F)<a, réutiliser la MEB
de F et normaliser son ancre pré-lot sans descente ni MEB de F+z. J≥2
garderait la descente actuelle. Ce n'est pas une omission de rattachement :
les incidences égales seraient résolues au premier usage ultérieur plutôt
qu'installées par K+1 labels dès la naissance de la directe.

L'enjeu est un échange explicite entre résidence et MEB/census répétés ;
aucun gain de temps n'est supposé. La table actuelle et ses refus restent
inchangés pendant la campagne. Merci de vérifier si ces ancres de directes
et les minima suffisent à rendre ce dispatcher paresseux complet, notamment
pour les continuations muettes et les lots simultanés. Un budget de cache
ne pourrait alors jamais remplacer l'autorité terminale ou le coût du miss.

### Clôture des mesures et livraison en préparation

Votre qualification indépendante O2/SAN (100 ordres, deux représentations,
16 506 coupes par binaire, trois mutations réfutées), la clôture du retry
ROOT et `lazy_alias_next_step_review.md` ont été lus. Les entrées
constructeur les citent sans leur attribuer la campagne mono. Merci de
publier votre paquet dès qu'il est clos ; **index encore libre** pendant
la dernière mesure s12 et les validations documentaires.

Résultats clos à ce message : 8k/s8, dix ordres en 150,776 s ; 16k/s8,
refus d'alias à K9 en 275,497 s ; 32k/s8, même refus à K7 en 464,273 s ;
8k/s10, dix ordres en 150,879 s. Le dernier 8k/s12 termine bientôt. Les
caps sont restés inchangés ; aucun temps de préfixe n'est promu. Le juge
des captures distingue validité du reçu et réussite de la tentative.
La prochaine implémentation utile est le cache facultatif et le dispatcher
J=1 ; aucun changement de ces règles n'est caché dans les mesures actuelles.
GCP non utilisé, aucun fichier v6 ni fichier d'audit indépendant ne sera
pris dans notre staging.

### Publication constructeur FULL — réservation de l'index

Votre publication `6446e248` est présente sur main ; merci. Les cinq
tentatives sont maintenant closes : s12 termine les dix ordres à 8k en
151,795 s. Le juge passe sur chacun des cinq reçus en Python normal et
sous `-O` ; ses neuf mutants sont réfutés dans les deux modes. Les deux
refus d'alias restent explicitement négatifs. Les octets des cinq captures
ont été comparés intégralement aux sorties outils, et les 51 sources plus
le binaire sont inchangés depuis l'admission.

Le paquet mono est scellé : 22 empreintes vérifiées, `SHA256SUMS`
`85eea497fdf6173d806dc66404f9c436e3e8acd45cf2a9af032aa6c1b27c3a51`.
La qualification constructeur et l'admission micro restent deux paquets
distincts, avec respectivement 59 et 11 empreintes vérifiées.

**ROOT réserve maintenant l'index** pour son lot cohérent : composant,
oracle, portes, sonde, juge de reçus, trois paquets et documents associés.
Aucun fichier v6 ni de votre dossier d'audit n'est inclus. Cette réservation
est automatiquement levée dès que le commit constructeur contenant cette
note est créé et que l'index est vide ; aucune branche n'est créée.
Prochaine évolution : dispatcher J=1 et cache facultatif, avec sa fixture
permanente et ses budgets propres, sans réinterprétation rétroactive des
refus ou mesures du présent calendrier. GCP non utilisé.

Contrôles de publication ROOT : documentation 325 fichiers actifs et
registre 20 phases passent en normal et `-O`. La porte des reçus indexés
a d'abord refusé, code 1 dans les deux modes : la règle `CMakeCache.txt`
avait exclu les deux copies brutes du reçu constructeur. Elles ont été
ajoutées explicitement à l'index, sans modifier leurs octets ni le sceau.
La reprise valide 1 172 fichiers indexés dans 19 reçus, normal et `-O` ;
les selftests réfutent neuf corruptions. Les blancs des sources et documents
passent ; les captures épinglées restent intactes. La contrelecture séparée
des cinq résultats et de la notice ne relève aucune incohérence factuelle.

### Delta lazy FULL ouvert après `98bb6578`

La preuve `lazy_alias_next_step_review.md` est relue et mise en application
dans `src/forest/full_gabriel.hpp`, sans modifier les helpers F. L'ancienne
API reste le témoin eager par défaut. La nouvelle API séparée est
`build_full_gabriel_order_lazy(..., FullGabrielLimits, FullGabrielCacheLimits)`.
Son contrat exige `max_aliases=0` ; une valeur non nulle est refusée comme
conflit, sans réinterprétation de ce plafond historique.

Le cache facultatif conserve les premières C résolutions strictes non
minima (`max_entries=C`) : aucune éviction, aucune remise à zéro ; à capacité
pleine ou nulle, le dispatcher calcule sans insertion. Les minima gardent
leurs tokens dans l'index de catalogue ; K1 utilise l'offset PointId trié.
Toutes les ancres des directes restent installées après fermeture du lot,
y compris les connexions muettes. J=1 réutilise la MEB/census de F pour
retrouver exactement F+z et normaliser son ancre pré-lot, sans nouveau MEB.
Les budgets de portails, descentes, MEB, supports et requêtes restent
persistants sur l'ordre. Une panne d'allocation refuse toujours tout l'appel.

Les nouvelles politiques se nomment `eager_all_incident_facets_v1` et
`lazy_first_c_strict_resolutions_v1`, visibles même dans les refus. Les
compteurs cache sont séparés de `aliases`/`alias_hits` historiques. Les
insertions comptent leurs admissions avant allocation ; sur succès elles
égalent la résidence en entrées, pas en octets. Des portes nouvelles
préparent la fixture J=1 proposée, E5, les lots, le deuxième pas de descente,
cache 0/1/grand, les budgets et les pannes persistantes d'allocation.

Une sonde séparée comparera les deux politiques avec de vrais digests
d'entrée et de forêt (niveaux rationnels normalisés, labels et topologie),
dont le coût sera inclus. Aucun ancien chronométrage sans digest ne servira
de bras apparié. Merci de contre-lire ce delta dans votre dossier si utile ;
les sources sont encore en préparation et seront explicitement gelées avant
qualification/mesures. Index libre, aucune session GCP.

### Précontrôle lazy et prise en compte de l'audit mono

Votre `MONO_FULL_COURANT.md`, la décomposition de résidence et les quatre
lacunes du juge historique ont été lus. La nouvelle sonde v2 et son juge
séparé reprennent explicitement les identités de travail et ajouteront les
contrôles de cardinal et d'inclusion des chronomètres ; les reçus v1 restent
intacts. Les empreintes sémantiques normalisent les niveaux rationnels et
l'ordre des nœuds, et engagent les labels et parents, pas seulement les
cardinaux ou la couverture. Le sérialiseur a son juge arithmétique Boost.

Le précontrôle de développement lazy donne 12/14, pas une qualification :
le candidat partagé ABC/ABW/ABV à cinq points échoue à la porte indépendante
de régularité globale. Ce négatif et le source fautif sont conservés dans
`receipts/full_lazy_development_20260905/`. La correction de fixture est
en cours sans relâcher cette porte. Les pannes d'allocation paresseuse
passent sur six cellules et 434 fautes ; le sérialiseur passe 672 divisions
et normalisations contre Boost. Aucun chrono nouveau n'est encore lancé.
Index libre ; merci de poursuivre votre publication indépendante sans
inclure les fichiers constructeur. GCP non utilisé.

Le négatif est isolé : la boule de diamètre CV, niveau 17/2, a les deux
points A et B sur sa coquille en plus du support CV. Le remplacement
A(0,50,0), B(40,50,0), C(20,61,0), W(20,0,0), V(20,10,30) est globalement
régulier et exerce un vrai hit cache à K2 dans le lot ABW/ABV de niveau 841.
La porte conserve le premier cas comme rejet global obligatoire.

**Gel de qualification ROOT** : producteur `13c6cc72…`, porte lazy
`6c325c8b…`, contrôleur `528175a4…`, inventaire source `08dda37e…`
(582 fichiers dont 521 en-têtes Boost pré-épinglés). Admission de deux
builds neufs absents, 14 CTests chacun, ASan/UBSan et LeakSanitizer actif,
contexte ROOT non tracé, aucun override. La sonde n'est pas encore admise.
Les octets produit/tests/digest restent gelés pendant cette campagne ;
index toujours libre pour votre publication séparée.

Qualification ROOT close, code 0 : 14/14 Release puis 14/14 ASan/UBSan,
LeakSanitizer actif, aucune reprise ni censure. Les 582 pins et six
binaires de chaque build restent stables. Le reçu privé clos a SHA
`28a203ea7f46699e9845252bc02f46c9719c2380cef3e4e95d1f5d935a0abdc8` ;
son scellement et sa publication sont en préparation. Les positifs lazy
comptent 81 appels/3 192 coupes ; 127 rejets et 434 fautes persistantes
passent. La contrelecture séparée des tokens, ancres no-op, J1 et budgets
ne relève aucun défaut bloquant, sans ajouter de qualification par elle-même.

Votre publication `6f4b4de5` est présente sur main. Les quatre lacunes du
juge historique sont traitées dans le juge v2 séparé `8d8a612a…`, avec
19 mutants préparés ; leur exécution attend l'admission de la nouvelle
sonde. Ni le juge v1 ni ses octets scellés ne sont modifiés. Aucun chrono
lourd nouveau, index libre et GCP non utilisé.

Votre annonce de passe indépendante lazy (CPU0) est lue. ROOT ne lance
aucun benchmark lourd apparié tant que vous n'avez pas indiqué sa clôture.
La prochaine compilation de sonde et les 24 admissions n=8 restent des
contrôles fonctionnels, sans temps produit revendiqué ; elles peuvent
partager cet hôte. Merci de signaler ici ou dans votre dialogue la fin
des moteurs et les nouveaux verrous éventuels. Les sources restent gelées.

Sonde v2 admise : binaire `1d5a38ce…`, 40 dépendances utilisateur du
compilateur contrôlées. Les 24 micros n=8 (s8/10/12, fenêtres Kmax5/10,
eager et lazy C0/1/1M) terminent ; les empreintes sont égales à fenêtre
fixée. Onze rejets de parsing passent, les 19 mutants du juge sont
réfutés en normal et `-O`. Le reçu micro clos porte `9ce369e2…` ; aucune
mesure n'est attribuée à ces contrôles concurrents.

Les six tentatives 8k sont préparées avec le même binaire et les mêmes
plafonds, mais **aucun GO lourd avant votre clôture CPU**. Ensuite seulement
viendront les admissions distinctes 16k puis 32k lazy si les paires 8k
concordent et les budgets permettent de poursuivre. Notre paquet de
qualification ciblée est scellé (`e0d99feb…`), 198 fichiers, pas encore
indexé. Votre index reste libre pendant votre publication.

Votre `digest_probe_review.md` est lu intégralement, y compris la dent
first-C. Nous ajoutons une porte Python supplémentaire, séparée du juge
v2 déjà capturé : sur chaque ordre lazy réussi, inserts=min(C,portails)
et skips=portails−inserts ; jamais cette égalité de succès sur un refus.
Le modèle C1/P1/I0/S1 devient une contre-fixture permanente. Le supplément
rejouera les 24 micros sans moteur, puis sera obligatoire pour clore
chaque nouvelle tentative lourde. Les captures et leurs pins restent
intacts, et cette porte ne remplace ni le juge v2 ni le sceau de provenance.
La sonde micro est publiée (`9b197a96…`, 469 fichiers), pas encore indexée.
Toujours aucun benchmark lourd ; votre clôture CPU reste attendue.

Votre clôture CPU annoncée à 17:42:45 UTC et le pont indépendant
109 ordres/67 920 coupes sont lus. **GO ROOT du premier passage 8k/s8
eager**, avec le binaire admis `1d5a38ce…`, CPU6 et limite 600 s.
Les autres passages restent séquentiels et attendent chacun la clôture
du précédent ; le supplément first-C sera contrôlé avant de poursuivre
après toute tentative lazy. Aucun changement de source ou de cap.
Index libre, GCP non utilisé.

Reprise ROOT à 18:40 UTC : le premier eager 8k/s8 a terminé (149,9517 s,
empreinte `e6e3fa51…`). La session suivante lazy n'est plus disponible,
aucun processus correspondant ne tourne, et sa capture ne contient que
la configuration, sans terminal ni code de sortie. Cette interruption
reste de cause inconnue ; aucun temps, timeout ou succès ne lui est imputé.
Le contrôleur gelé clôt `heavy_paired` en échec, sources stables ; les octets
sont conservés. Nouvelle campagne `heavy_paired_resume`, avec répétition
de l'eager, admise sur les mêmes pins avant toute comparaison de temps.
Le supplément first-C est qualifié séparément en lecture seule pendant
ce passage. Aucun moteur concurrent demandé à nos sous-agents. Index libre.

Point ROOT après les paires s8 et s10 de la reprise : mêmes digests FULL
par ordre et global `e6e3fa51…`. À s8 : eager 140,956 s, lazy 142,787 s ;
à s10 : eager 137,899 s, lazy 142,968 s. Le pic processus passe d'environ
1 791 MiB à 1 292 MiB. À K10 lazy conserve 746 631 entrées sans skip,
contre 6 209 024 alias eager ; 488 139 nouveaux J1 expliquent exactement
l'excédent de MEB, tandis que les 456 331 pas restent identiques. Pas de
gain de temps revendiqué. s12 puis 16k/32k restent à clore séparément.

Le supplément first-C est publié indépendamment (`d7acb4fe…`) : 58
commandes en lecture seule, 117 ordres lazy par mode, 12 mutants et
quatre types de refus argv sous normal/-O. La capture interrompue est
publiée intacte (`055cf24d…`). Aucun ancien reçu n'a été modifié.
À la demande de l'utilisateur, README/PASSATION sont recentrés sur
l'état courant et `docs/FAUSSES_PISTES.md` regroupe les décisions écartées.
Seuls dix caches Python générés hors audits/receipts ont été effacés.
Votre dossier et vos préparations sont intacts ; index toujours libre.

Préparation de publication ROOT : les liens de nos contrats lazy/digest
visent votre nouveau `receipts_full_lazy_20260905/digest_probe_review.md`,
encore non suivi à cet instant. Merci de signaler votre commit séparé
quand il est prêt ; nous n'inclurons pas vos fichiers en préparation.
Si votre publication reste différée, ces liens seront bornés à notre
supplément/reçu et à cet échange pour ne pas pousser de liens cassés.
Le code et les reçus ROOT n'attendent aucune promotion indépendante.

Votre commit `e7fa5da7` est maintenant présent sur main : les liens lazy
ne sont plus en attente. La contrelecture first-C et son attribution
par composition restent distinctes de nos observations lourdes en cours.
Merci ; les sources consommées par la sonde sont inchangées.

Deux pistes préparées pour le prochain delta mono, sans implantation ni
mesure ajoutée à ce lot : (1) quand un lot contient une seule directe,
les q≤4 demandes strictes dans le même ordre donnent une seule classe
locale ; tableau de quatre tokens puis tri/unique après la dernière
demande, suffixe commun et ancre issue du premier token inchangés ;
(2) en génération q4, tester le `depth_at >= h4` déjà calculé au niveau
du bloc avant la cascade des formes, en conservant tout le balayage,
les sorties avant le compte strict et les entrées après le bloc. La
profondeur n'est pas monotone le long de la corde : aucun arrêt de seed.
Le second delta devrait versionner ses populations de compteurs, pas
réattribuer `depth_killed[2]` aux racines non traitées. Votre avis statique
sur ces frontières serait utile ; aucun moteur concurrent n'est demandé.

Les six passages 8k de la reprise sont clos (`50f22273…`) : 30 ordres
appariés, 27 compteurs front par paire, digests et identités de chemins
sans saturation vérifiés. Les trois pics diminuent d'environ 28 %, les
trois temps lazy augmentent de 1,18–3,68 % ; aucun gain statistique retenu.
Le palier 16k réussit dix ordres en 319,305 s, pic 2,590 GiB, reçu de
phase `d3656155…`. K9/K10 saturent C1M ; K10 rend 677 513 résolutions
sans insertion et first-C reste vérifié. Pas de ratio avec les anciens
refus eager. 32k a démarré, caps inchangés. Publication groupée et index
ROOT après sa clôture ; jusque-là votre index reste libre.

Clôture ROOT : 32k refuse à K9 sur `full_gabriel_successor_budget`,
128 000 000 opérations, code 2 après 548,857375 s, sans timeout. Huit
ordres réussis restent diagnostiques ; aucun digest global de succès.
La phase close porte `86e0f1b2…`. Les huit nouvelles captures lourdes
sont publiées (`b596d564…`), ainsi que leur contrelecture sans moteur
normal/-O (`0dce0816…`). Le groupement des huit contrôles first-C est
autorisé sur leurs seuls paquets clos ; aucun moteur ne sera relancé.
Ce groupement est maintenant publié (`7368216c…`, 222 fichiers), avec
32 commandes de contrôle code 0 et le refus moteur 32k code 2 conservé.

Votre nettoyage `10496d51` est présent sur main. **ROOT réserve maintenant
l'index** pour ce lot code/tests/docs/reçus, après constat d'un index vide.
Les seuls chemins `audits/` indexés seront notre fichier de coordination
racine ; aucun fichier de `morsehgp3D_v7/audits/` ne sera inclus.
La réservation prend fin après le commit ROOT et le retour à un index
vide. Publication sur main uniquement ; v6 et registre officiel intacts.
GCP non utilisé, aucun moteur de benchmark actif.

Contrôles ROOT avant commit : 345 documents actifs, 20 phases du registre
et 3 145 entrées de sommes dans 36 manifestes v7 de l'index passent,
en Python normal et `-O`. Les deux positifs et neuf mutants du contrôle
de publication passent également. Les 1 604 fichiers préparés sont
strictement nos 20 fichiers code/tests/docs/coordination et les 1 584
fichiers des huit paquets clos ; aucun fichier de votre préparation.

Vos avis statiques sur le lot à une directe et le bloc q4 sont lus,
ainsi que la contre-fixture rationnelle. Les conditions proposées sont
retenues pour le prochain delta : demandes strictes non dédupliquées,
ancre initiale et normalisation facturée conservées ; rejet du seul bloc
profond avec poursuite du balayage et compteurs versionnés. Aucun de ces
futurs changements n'est implanté ou mesuré dans le présent lot.

## Suite après `6126b373` : lot à une seule directe

L'avis indépendant `aafe7d93` est lu. ROOT implante maintenant seulement
la spécialisation `de-db==1` ; génération q4, budgets, normalisation des
ancres et API publiques restent inchangés. Le tableau de quatre tokens
remplace la DSU locale, pas les q résolutions ni la liste de parents publiée.
Le suffixe de fermeture est commun aux deux branches. Un forçage du chemin
général et ses observations de branche existent uniquement sous
`MHGP7_TESTING`, pour comparer les forêts et tous les compteurs logiques.

Qualification dans des builds neufs, 17 portes ciblées Release puis
ASan/UBSan, avec fautes d'allocation recalculées sur le nouveau programme.
Les fixtures q2/q3/q4, no-op, racines répétées, minima simultanés et lots
partagés devront être explicitement non vides et géométriquement admises.
Aucun chronométrage lourd avant clôture de ces portes et des moteurs
indépendants ; merci de signaler ici votre éventuelle occupation CPU.
Index constaté vide et laissé libre pendant l'implémentation. GCP non utilisé.

Le refus 32k reste, par construction, inchangé sous ce delta de
regroupement : son budget de successeurs n'est pas assoupli. Pour la suite,
la normalisation actuelle relit puis réécrit même un arc déjà dirigé vers
la racine (3d+1). Une piste distincte serait d'éviter les écritures
redondantes, avec comptage explicite des lectures/écritures réellement
nécessaires et nouvelle version de ce contrat si le calendrier change.
Votre analyse de cette charge est bienvenue ; aucun patch de normalisation
ni gain 32k n'est mêlé à la spécialisation du lot unitaire.

Votre preuve de la dernière paire redondante et le diagnostic des 48
ordres lazy réussis sont lus. La réduction conditionnelle `S-2A` ne sera
pas appliquée au préfixe K9 refusé. Le delta suivant devra distinguer
facturation historique et opérations restantes, sans soustraction après
coup. Votre absence de moteurs/builds/CTests, annoncée au dialogue courant,
est notée ; les calculs de contrelecture CPU0 restent sans chronométrage
produit. Le présent header singleton est encore en précontrôle ; aucune
qualification du pin historique ne lui est attribuée.

Clôture ROOT du header `21b77d29…` : 17/17 Release et 17/17 ASan/UBSan,
LeakSanitizer actif, carte de 584 pins `07e8634d…`. Le reçu clos
`e3b64a03…` est publié dans `receipts/full_gabriel_singleton_20260905/`
(sommes `b37ea780…`). Les balayages frais comptent 49 fautes eager et
209 lazy, toutes refusées sans échappement ; les 357 refus du différentiel
singleton conservent les 33 compteurs et la frontière de refus. Aucun
gain de temps n'est encore annoncé. Le nouveau binaire de la sonde est
`57c598bf…`, même instrument et mêmes caps, en admission micro avant/après.

Votre préparation indépendante CPU0 sur la copie figée `21b77d29…` est
notée. Merci d'annoncer sa clôture avant notre campagne lourde mono :
six passages 8k alternés ancien/nouveau pour s=8/10/12, CPU6, borne 600 s
par tentative, même lazy C1M. Aucun passage lourd ne commence tant que
votre occupation annoncée n'est pas close. Index toujours libre ; nous
ne touchons ni à votre dossier ni à vos fichiers en préparation.

À 20:15 UTC, nos 48 micros et leurs contrôles normal/-O sont clos :
24 paires, 156 ordres comparés, mêmes forêts et tous les champs hors
mesures identiques. L'admission `f143ee0d…` lie les deux binaires et la
qualification fraîche. Nous avons lu vos nouveaux bruts O2/SAN et les
jugements normal/-O, mais attendons votre signal explicite de fin
d'occupation avant le premier 8k. Notre revue de clôture ne lance plus
aucun moteur ; seuls contrôle de documents et lecture du publisher restent
actifs. Les micros ne sont pas utilisés comme résultats de latence.

Votre clôture à 20:19:12 UTC est lue : builds et moteurs indépendants
terminés, aucun nouveau moteur prévu. **GO ROOT du premier 8k/s8 ancien**,
CPU6 et 600 s, puis clôture et contrôle avant chaque tentative suivante.
Les 114 ordres/69 120 coupes et le mutant du quatrième parent restent
votre preuve indépendante, à lier après publication de vos fichiers.
Index libre durant les mesures ; aucun GCP utilisé.

Le premier bras ancien 8k/s8 est clos, code 0, dix ordres : 145,539554 s,
digest `e6e3fa51…`, reçu `97cae691…`. Le nouveau bras s8 démarre seulement
après ses juges v2 et first-C. Aucun changement de cap ni de source.
La revue de notre publisher a ajouté la recomposition des résumés depuis
les reçus sources et interdit de promouvoir deux refus égaux ou une
clôture instable. Le publisher reste inerte jusqu'aux captures closes.
Nous lierons votre nouveau paquet indépendant après votre commit séparé ;
l'index reste disponible pour cette publication pendant nos mesures.

Paire s8 close : 145,539554 s ancien contre 144,336973 s nouveau,
FULL seul 66,801977 s contre 65,381072 s. Tous les champs hors mesures
sont identiques pour les dix ordres. Écart faible, aucune accélération
robuste déduite ; la paire s10 commence dans l'ordre inverse. À K10,
les 250 854 612 supports MEB restent intégralement payés : la suppression
des allocations locales ne traite pas ce coût. La proposition MEB
native devra être mesurée sur la distribution réelle des appels FULL
avant un éventuel dispatch, sans réintroduire l'activation générale
écartée sur les petits q2. Aucun nouveau delta n'est ajouté à ce lot.

À 20:34 UTC, cinq passages clos et dernier nouveau/s12 lancé. s10 donne
141,857304 s ancien contre 145,200786 s nouveau, FULL seul 64,841603 s
contre 64,986103 s : toujours aucune accélération robuste retenue.
Vos preuves singleton et contrelecture constructeur sont lues ; nos
contrats pointent désormais vers votre paquet en préparation. Merci de
signaler votre commit séparé avant notre livraison, prévue après clôture
du sixième passage et du paquet mono. Nous ne réservons pas encore
l'index ; nous annoncerons explicitement cette réservation avant ajout.

Votre commit `c9419bb1` est présent sur main et l'index est revenu vide.
Les liens de nos contrats vers votre paquet singleton ne sont plus en
attente. Le dernier passage est encore actif ; aucune réservation ROOT
ni ajout à l'index avant sa clôture et la publication des captures.

Les six passages sont maintenant clos, reçu `eabece59…`, 30 ordres
appariés identiques et mêmes digests entre s. À s12 : 145,436411 s
ancien contre 145,544498 s nouveau. Le paquet mono est publié, sommes
`ce4b6a06…`, 1 444 fichiers ; le paquet ciblé comporte 182 fichiers.
Les trois variations de temps sont −0,83 %, +2,36 %, +0,07 % : aucun
gain robuste ni contrat 50k acquis. Aucun moteur encore actif.

Index constaté vide après votre commit : **ROOT le réserve maintenant**
pour nos seuls 13 fichiers code/tests/docs/coordination et les deux
paquets clos. Aucun fichier de votre dossier n'est inclus. La réservation
prend fin au commit ROOT et au retour à un index vide ; push sur main.
GCP non utilisé, worktree v6 et registre officiel préservés.

Contrôles ROOT de l'index : exactement 1 639 fichiers, soit nos 13
fichiers et les 1 626 copies des deux paquets, aucun chemin inattendu.
Les 4 769 entrées des 38 manifestes v7 passent sur les blobs indexés,
normalement et sous `-O` ; 349 documents et 20 phases du registre passent
également dans les deux modes. `git diff --cached --check` signale les
blancs bruts des sorties CMake/CTest/compiler, volontairement inchangés
pour conserver les hashes ; le contrôle limité à nos 13 fichiers
code/tests/docs/coordination passe. Aucun nettoyage des reçus scellés.

## Suite après `b2f0dc08` : normalisation v2

La livraison singleton est poussée sur main et sa réservation d'index
est close. Votre état courant et la preuve de normalisation sont relus.
À la demande de l'utilisateur, ROOT engage ce seul delta mono : conserver
le dernier nœud avant la racine à la première passe, puis arrêter la
compression avant lui. La lecture terminale et `normalized_anchors` sont
conservés ; aucun q4, MEB ou regroupement de lot n'est modifié.

La facturation sera explicitement versionnée : 1 opération à profondeur
nulle, 3d−1 sinon, sans soustraction après coup. Les plafonds numériques
restent inchangés mais leur calendrier d'admission change. L'API et la
sonde devront porter cette unité ; les reçus historiques 3d+1 garderont
leur lecture et leurs pins. Les égalités de succès ne seront pas appliquées
aux préfixes refusés, notamment à l'ancien K9/32k.

Votre contrelecture du futur helper, des refus au milieu des deux passes
et de cette frontière de version est bienvenue. Tests prévus : état complet
après d=0/1/2/long, normalisations répétées, caps exacts/cap−1, forêts et
autres compteurs inchangés. Nous signalerons le gel des sources avant vos
éventuels builds, et n'engagerons pas de mesures lourdes concurrentes.
Index actuellement libre, dossier d'audit intact ; GCP non utilisé.

Le helper et son raccord producteur sont désormais proposés, header
`85c27ab91d7f159520a8db3098629447b0a213a134c5c042a86c585416847fad`.
La routine réelle `full_gabriel_detail::normalize_successor` rend un statut
interne, puis Builder conserve les motifs de refus nommés. Une seule
variable de dernier nœud est ajoutée, aucune allocation. L'ancien chemin
est un différentiel sous `MHGP7_TESTING` seulement, avec résultat marqué v1.
Le produit porte `successor_accounting=full_successor_reads_writes_no_last_pair_v2`.
Votre avis statique peut commencer sur ce pin ; la porte neuve et les
lecteurs de sonde v3 sont encore en préparation. Aucun moteur en cours,
pas encore de gel de qualification ni de réservation d'index.

Gel de qualification ROOT : header `85c27ab9…`, gate
`408532e71878b3d7227d8208ed23f6eac994561e3e4ee79924be03152ee7c97f`,
carte de 585 pins `7bd7ec72dc96a7a07f8e64f2928dd1d4f3077be5bd2c40f85be9112e760055ff`.
Le contrôleur neuf `265be9e2…` lance maintenant vingt portes ciblées
dans deux builds neufs Release/SAN, compilations CPU0 et tests CPU6.
Les autres sources compilées sont gelées ; documents et lecteurs Python
restent séparés. Nous vous signalerons leur clôture avant les passages
mono lourds NEW-only 8k/s8–10–12 puis 16k et 32k/s8, sous les mêmes caps.
Votre qualification indépendante est bienvenue, sans chevauchement des
futurs chronométrages lourds. Toujours aucune réservation d'index ni GCP.

Votre nouvel avis statique est lu. Le raccord de l'ancienne sonde est
effectivement à fermer : `bench/full_gabriel_probe.cpp` porte maintenant
un `#error mhgp7_obsolete_full_probe_calendar` explicite. Elle n'est pas
une cible CMake et ne fait pas partie des 585 sources de la qualification
en cours ; aucun header gelé n'a changé. Les anciens octets restent dans
Git et les paquets historiques, sans réécriture de leurs reçus. Un rejet
de compilation frais devra contrôler ce verrou dans l'admission de la
sonde v3. La suite MEB privée démontrée est également lue et conservée
comme piste séparée, sans la mélanger à cette normalisation.

La tentative initiale est close en échec, reçu `70714475…` : compilation
du gate, ambiguïté de `detail` dans son `main`, avant tout CTest. Correction
locale en nom pleinement qualifié ; header `85c27ab9…` inchangé. Le gate
est maintenant `68815ac2…`, carte `8c977bc5…`. ROOT reprend sous
`run_qualification_r2` et builds neufs suffixés `_r2_release`/`_r2_san`.
L'échec initial sera copié tel quel avec la qualification finale, sans
modifier son ancien sceau ni le faire passer pour un résultat moteur.

R2 Release ferme 20/20 CTests : nouveau gate 560 préfixes primitifs,
1 242 appels jugés, 180 paires FULL réussies et 3 320 coupes ; mode
budgets 668 paires, 640 appels refusés, 16 admissions numériques v2/v1
distinctes attendues. Les 32 autres champs restent identiques dans le
différentiel apparié. SAN compile encore ; aucune qualification 20×2
ni performance nouvelle n'est annoncée avant clôture. Les deux lecteurs
compatibles v2/v3 sont gelés séparément, avec 35/27 mutants v3.

Qualification R2 désormais close : 20/20 Release et 20/20 ASan/UBSan,
LeakSanitizer actif, reçu `49be3d72…`. Paquet publié
`receipts/full_gabriel_successor_20260905`, 280 fichiers, sommes
`0e6c84ba…`, comprenant l'échec initial inchangé. Tous les builds et
CTests ROOT sont terminés. Le contrôle de votre ancien point de sonde
est intégré à la prochaine compilation (rejet nommé, code 1, aucun ELF).

Préparation mono NEW-only relue : cinq passages 8k/s8–10–12, 16k/s8,
32k/s8, tous Kmax=10 et lazy C1M. Le contrôleur lie désormais le protocole
entier et le binaire admis avant chaque passage, puis les prédécesseurs
aux reçus et bruts. Les plafonds restent 128 millions de successeurs,
600 s et 26 Gio d'espace virtuel. Le nouveau schéma v3 n'est pas traité
comme une répétition appariée des anciennes latences. Nous commençons
par compilation et 24 micros ; aucun lourd sans cette admission fraîche.

Admission sonde close : build `86475252…`, binaire `8ff0dd10…`,
40 dépendances compilées ; rejet réel de l'ancienne sonde, code 1,
motif attendu et aucun ELF. Micro `c4bbd2cf…` : 24 succès, 156 ordres,
11 rejets d'arguments, 35/27 mutants des lecteurs par mode normal/`-O`.
Vos builds/rejeux indépendants sur CPU0 sont maintenant lus et observés.
**Aucun chronométrage lourd ROOT ne commencera avant votre clôture explicite.**
Nous pouvons préparer son protocole et les lecteurs en attendant, sans
moteur. Merci de signaler la fin de vos compilations/rejeux/mutants avant
que nous réservions CPU6 pour ces cinq passages successifs. Index libre.

Clôture indépendante explicite de 21:51:59 UTC lue ; aucun moteur ni
compilateur restant observé. ROOT engage les cinq passages mono sur CPU6,
séquentiellement et sous les plafonds gelés. Merci de maintenir l'absence
de builds/rejeux lourds jusqu'à notre clôture. Les comparaisons fonctionnelles
aux captures historiques resteront séparées des chronométrages NEW-only.
Aucune réservation d'index à ce stade ; GCP non utilisé.

Question de suite, strictement documentaire pendant ces mesures : votre
preuve des filtres MEB (violateur obligatoire, aucun q2 après diamètre
global) est lue, ainsi que le recul natif n=2. Pour le prochain delta,
je propose de qualifier d'abord le proposeur privé filtré sans toucher
FULL, puis son raccord au Work persistant. Le shell complet, l'ordinal
sur tous les sites et le niveau q4 brut restent impératifs. Voyez-vous
un autre invariant nécessaire pour préserver exactement le premier
support accepté à budget de proposition non limitant ? Aucun seuil de
dispatch ni gain de tour ne sera choisi à partir des anciens petits lots.

Rectification CPU reçue : la fenêtre 21:54:33.912911–21:54:34.770106
recouvre ROOT n8000/s8. Sa capture reste valide fonctionnellement, mais
son temps de 142,456 s est exclu de toute comparaison de performance.
Nous conservons les octets originaux et préparerons un rejeu distinct,
sans remplacer ce reçu. Le passage s10 a commencé après 21:56:01 UTC,
donc après l'incident déclaré. Merci de conserver tous les sous-agents
sans moteurs ni compilations jusqu'à la clôture finale ROOT.

Réponse MEB lue : filtre stable q3 puis q4, même diamètre départagé
strictement, même premier violateur et mêmes slots canoniques. La coquille
intermédiaire peut avoir plusieurs bases ; pas de rejet shell=q avant le
terminal global. La marge P non limitante doit couvrir les deux bras de
toute la séquence Work, pas seulement un appel. Ces conditions seront
le contrat du futur delta privé, sans changement des sources mono gelées.

Les captures fonctionnelles 8k/s8,s10,s12 sont closes avec le même digest
`e6e3fa51…` ; le temps s8 reste exclu. ROOT mesure maintenant 16k/s8,
puis tentera 32k/s8 sous les mêmes caps et un rejeu s8 indépendant.
Le paquet de qualification indépendant et sa rectification sont lus.
Index toujours libre côté ROOT : votre publication audit seule peut
précéder la nôtre, sous réservation annoncée, sans toucher nos fichiers.

Les cinq captures ROOT sont closes, sources stables : trois succès 8k,
un succès 16k, et refus 32k/K9 `full_gabriel_meb_call_budget` à exactement
4 000 000 appels MEB, 125 373 952 accès successeurs, après 567,439 s.
Huit ordres horizontaux réussis seulement à 32k ; aucun cap relevé.
Le temps original 8k/s8 reste exclu. Un unique rejeu distinct 8k/s8
commence maintenant, wrapper gelé `96fa9d1…`, même binaire `8ff0dd10…` ;
les moteurs et compilations restent suspendus côté audit jusqu'à sa fin.
Votre commit `efab6d1a` est vu publié ; index non utilisé côté ROOT.

**Clôture finale des moteurs ROOT : 22:20:00 UTC.** Rejeu 8k/s8 réussi,
138,221 s, même binaire et dix ordres identiques au premier passage sur
tous les champs hors mesures, compteur de successeurs inclus. Reçu
`8f3a1cd4…`, fenêtre 22:17:41.921080–22:20:00.210394 UTC ; aucun nouveau
chevauchement d'audit signalé ni moteur concurrent observé. Cette fenêtre
reste une observation d'hôte partagé, pas une isolation matérielle certifiée.
L'exclusion du premier chrono est conservée. Plus aucun moteur/build ROOT
prévu dans ce lot ; restent comparaisons de captures, documentation et
publication. Nous annoncerons séparément notre réservation d'index.

Publication ROOT préparée : paquet mono de 1 162 fichiers, sommes
`910d87e0…`, cinq captures initiales plus rejeu distinct et exclusion
explicite du premier chrono. Le comparateur clôt 29 cas/204 ordres
normal et `-O`, dont uniquement K1..8 pour les deux refus 32k. Sources
et entrées stables ; aucun refus promu. Documentation : 353 fichiers
validés dans les deux modes ; registre inchangé, vingt phases valides.

**Réservation d'index ROOT** pour l'unique commit
`reduce full successor normalization and qualify mono work` : index
inspecté vide, seulement code/tests/docs/paquets constructeur et ce
dialogue. Aucun fichier de `morsehgp3D_v7/audits/` ni aucune modification
v6 n'est préparé. Réservation close automatiquement une fois ce commit
publié sur `main`. Aucun moteur supplémentaire ni GCP.

Contrôle final de l'index : 6 209 fichiers dans les quarante paquets
SHA256SUMS v7 passent normalement et sous `-O`. Le premier contrôle
avait détecté 44 captures filtrées par les règles d'ignore ; seules ces
copies scellées exactes ont été ajoutées explicitement, puis revérifiées.
Le code et les documents passent `diff --check` ; les bruts restent
byte-exacts. Aucun fichier auditeur, v6 ou registre dans l'index ROOT.

## Reprise constructeur du 6 septembre : filtre MEB privé

Votre reprise et la distinction coût par MEB / nombre d'appels sont lues.
ROOT qualifie maintenant le helper préparé `484a89bc`, sans le modifier,
contre `0645aa00` et F `f75a136a`. Les portes portent sur la première base
acceptée, les transitions locales distinctes des trajectoires natives,
les budgets persistants P/L, les sentinelles et les niveaux q4 bruts.
Les compilations et petits moteurs fonctionnels seront capturés dans
un répertoire neuf ; aucun chronométrage lourd ni GCP prévu dans ce lot.
Cette qualification ne change ni FULL ni son cap de 4 000 000 appels.

Votre examen des terminaux déjà certifiés est bienvenu pour le chantier
suivant : préciser la clé de réutilisation suffisante, les conditions de
validité dans une chaîne silencieuse et la sémantique des caps/compteurs
sur cache hit. Aucun cache global de toutes les cofaces n'est autorisé
implicitement. Nous conservons Work/P comme chantier séparé. Index ROOT
libre ; nouvelle réservation annoncée avant publication.

Question ciblée de la porte de trajectoire : votre corpus rationnel livré
exerce la coquille supplémentaire avec une base q2 unique. Disposez-vous
d'une fixture de pivot admissible (Q positif, z strict, rayon courant au
moins demi-diamètre) avec deux bases acceptables de même boule mais de
supports différents ? Elle permettrait de réfuter causalement l'inversion
de l'ordre sans prétendre qu'une MEB locale arbitraire est un pivot natif.
À défaut, nous distinguerons strictement le test d'énumération local de
la qualification des trajectoires effectivement issues du diamètre global.

La correction par plan radical du 6 septembre est lue : la base positive
du pivot admissible est unique. Notre fixture d'ordre à deux bases est
explicitement hors Q positif, jamais présentée comme une ambiguïté native.
Une sentinelle admissible du tétraèdre régulier complétera la porte sur
le calendrier P (quatre formes q3/q4 contre une q4 en premier, même base).
Les entrées actives et les fausses pistes retiendront cette correction.

Premier lot C++ clos : quatre binaires O2 puis quatre ASan/UBSan, plus
trois copies mutantes rationnelles. Les portes compilées passent ; les
deux juges rationnels de captures sont en cours, donc leur verdict n'est
pas anticipé. Sources du helper inchangées, aucune intégration FULL.
La piste de certification terminale par label immuable et token courant
est lue ; elle reste séparée. Aucun chronométrage de performance ni GCP.

Les deux juges rationnels R1 passent désormais, avec sorties normal/-O
identiques : 3 430 appels et 1 507 ordinaux par build ; la géométrie
F/Trace/NoObserver ferme 9 344 comparaisons, plus 59 frontières ciblées.
R2 ajoute uniquement la sentinelle admissible d'ordre/budget demandée,
dans une nouvelle capture ; le helper `484a89bc` reste strictement identique.
Votre réservation d'index est vue et respectée : aucun fichier préparé
par ROOT. La publication constructeur attendra votre commit annoncé.

**R2 et tous ses moteurs sont clos à 08:14:41 UTC.** Les 41 commandes
passent, quatre binaires O2 puis ASan/UBSan, sans macro de test. Le helper
`484a89bc` est inchangé. Le complément admissible exécute 8 appels locaux,
6 natifs et 1 rejeu ; trois admissions différentes et supports identiques.
Le mutant est réfuté par `order_budget.calendar_changed`, distinct du
mutant hors Q positif. Les deux juges de captures passent, sorties identiques,
sans nouveau C++ : run `981f3b3e…`, carte de 63 sources `7e881f99…`,
contrôleur `0f5f0f6c…`, reçu des juges `44d590bb…`.

Le paquet constructeur `receipts/meb_filtered_20260906/` contient 504
fichiers, sceau `c8268e85…`. R2 seul est publié ; R1 positif reste privé,
non écrasé, pour ne pas dupliquer des preuves intermédiaires. Le rejeu
depuis les copies publiques passe aussi. Les entrées, résultats et fausses
pistes portent désormais la correction de base unique et le statut local,
sans prétendre intégrer le proposeur à FULL ni lever le cap 32k.

Votre commit `e870d706` est vu publié sur `main` ; réservation d'audit
close. **Réservation d'index ROOT** pour l'unique commit
`qualify filtered meb proposals and preserve budget semantics` : index
inspecté vide, uniquement documents constructeur, paquet de preuves et
ce dialogue. Aucun fichier vivant de votre dossier, aucune modification
v6 ni registre préparés. Réservation close à publication. Documents :
363 fichiers valides normal/-O ; registre inchangé, vingt phases valides.
Aucun nouveau moteur ni GCP prévu dans ce lot.

Contrôle final avant commit : 6 712 fichiers indexés dans 41 paquets
SHA256SUMS passent normalement et sous `-O`. Le nouveau paquet complet
est ajouté explicitement, y compris ses snapshots sous `build/`, sans
ELF. Les sept documents/dialogues constructeur passent `diff --check` ;
aucun fichier v6, auditeur vivant ou registre dans l'index ROOT.

## Raccord FULL du proposeur filtré — 6 septembre, après 62e5cd76

La réservation précédente est close, commit publié. ROOT ouvre le petit
delta produit : nouveau `src/forest/meb_proposal.hpp`, port explicite de
`484a89bc` et des formes `d6dbba19`, sans modifier F `f75a136a`. Le Builder
FULL possède un Work persistant par ordre et réutilise son Builder F.
P=0 reste le défaut ; l'opt-in ajoute une comptabilité distincte p/A,
avec miroir sur destruction et charge externe d'appels inchangée.
Votre proposition de mémo terminal est lue et reste un delta séparé.

Qualification prévue : contre-F locale, composition Gamma eager/lazy,
budgets prospectifs, allocations et exceptions ; puis suite FULL existante
dans deux builds neufs. Pas de mesure de tour simultanée ni de GCP.
L'index reste libre pour vous ; vos nouveaux fichiers
`receipts_filtered_review_20260906/` ne sont ni touchés ni préparés.
Avis demandé sur le raccord : Work frère de geometry_result, A seulement
autour de l'appel F, P épuisé repli sans refus, P0 corps F direct, et
distinction entre appel FULL déjà payé et appel géométrique non encore
payé lorsqu'une proposition lève une exception.

Votre `receipts_filtered_review_20260906/constructor_review.md` est lu
en entier : contrelecture des captures privées acquise, sans transfert à
FULL. Le raccord en préparation comporte désormais les deux champs API
(`max_meb_proposal_supports`, `meb_accounting`) et cinq diagnostics séparés.
Aucun mémo terminal ni changement de plafond externe. F reste inchangé.
La qualification compilée à venir n'est pas une mesure de performance ;
ses processus seront tous clos avant une éventuelle campagne mono.

Votre commit `143f751a` est vu ; aucune préparation ROOT dans l'index.
Le raccord actuel passe les **30/30 CTests Release** (20 antérieurs et
10 nouveaux). La porte de composition juge 1 488 sorties / 33 792 coupes
par mode, avec 160 caps exacts et 160 refus cap−1 en mode rejets.
P=0/1/large passent chacun les 49 fautes eager et 209 lazy. SAN est en
cours, donc sans verdict anticipé. Une première configuration sans le
chemin Boost local a échoué et reste conservée ; R2 repart de zéro avec
le chemin déclaré, sans installation externe ni changement de source.
Les injections avant forme seront faites dans une copie explicite où
NoObserver perd `noexcept`, après deux certificats acquis : pas de claim
d'exception dans le NoObserver nominal. Les contre-F locaux requalifient
aussi le header produit, indépendamment de vos reçus privés historiques.

**Clôture constructeur à 09:36:43 UTC : 30/30 Release et 30/30 SAN.**
Le dépassement de fichier temporaire à 64 Mio pendant la compilation SAN
de R2 est conservé ; R3 borne la compilation à 512 Mio, les exécutions
à 64 Mio, sans changer sources ni caps HGP. Sources stables, LeakSanitizer
actif. Les 21 commandes locales/rationnelles et 15 commandes de mutations
sont également closes : 9 344 comparaisons sur le header produit, quatre
mutants réfutés et douze injections FULL tardives par build O2/SAN.

Le paquet `receipts/full_meb_product_20260906/` contient 1 250 fichiers
scellés, sans ELF, sceau `bbdbc40d…`. Les trois lecteurs passent normal/-O
sur les captures originales puis sur les copies publiées, sans moteur.
Les entrées, contrat du raccord, résultats, résidence et fausses pistes
sont à jour. Vos compléments P3/P6 sur tétraèdre et K9/K10 sont vus :
le corpus FULL constructeur reste explicitement borné à n≤8 ; les MEB
locales couvrent n≤11. Aucune extension de ce domaine n'est anticipée.
La fixture terminale n=12/K7 est lue et liée pour le prochain delta séparé.

Votre réservation `143f751a` est close, et le nouveau rejeu ne réserve
pas l'index selon votre dialogue. **Réservation ROOT** pour l'unique commit
`integrate filtered meb proposals into full builders`, limitée aux sources,
tests et documents constructeur, au paquet scellé et à ce dialogue.
Index inspecté avant préparation ; aucun fichier auditeur vivant, v6 ou
registre ne sera inclus. Réservation close automatiquement à publication.
Aucun nouveau moteur constructeur ni GCP dans ce lot.

Contrôle final d'index : **7 962 fichiers dans 42 paquets** passent normal
et `-O`, dont le nouveau paquet ajouté entièrement avec ses snapshots
inertes. Les 366 documents actifs et les vingt phases du registre inchangé
passent aussi dans les deux modes. Aucun fichier auditeur vivant ou v6
préparé ; aucune source testée ne diffère de son pin avant commit.

## Sonde MEB v4 et sentinelle produit — 6 septembre, après 20b28b1d

Le raccord est publié ; réservation ROOT précédente close. Nouvelle étape :
sonde v4 avec P obligatoire explicite, calendrier MEB et cinq diagnostics,
lecteurs stricts et comparaisons P0/opt-in. Aucun changement du helper ou
du Builder FULL. Le complément permanent tétraèdre P3/P6 demandé est porté
sur le helper produit ; votre extension indépendante K9/K10 reste distincte.
Les premières exécutions seront des admissions bornées, pas des temps lourds.

Merci de signaler la clôture de vos moteurs CPU1 avant une fenêtre de mesure
mono : ROOT n'en lance pas encore, et ne s'attribue pas votre résultat en
préparation. Les neuf cellules n=8k/16k/32k et s=8/10/12 auront chacune
deux tentatives indépendantes ; un refus P0 ne censurera pas l'opt-in.
L'index reste libre pour votre publication. GCP non utilisé.

Votre nouveau `receipts_full_meb_20260906/README.md` et la contrelecture
constructeur sont maintenant lus en entier : 116 ordres / 2 784 sorties /
214 704 coupes par build, extension n14/K9–K10 et clôture des 28 commandes
C++ à 09:46:09 UTC. Merci ; je lierai cet apport comme preuve indépendante,
sans requalifier par lui les catalogues ou les mesures. Votre réserve sur
les états individuels non publiés des douze exceptions est conservée.
La sentinelle permanente P3/P6 est prête (gate `ac25cbc7…`), sa compilation
fraîche et son mutant q4-first sont prévus dans l'admission de la sonde.
Sauf nouveau moteur signalé, la fenêtre mono suivra cette admission ;
aucune autre compilation ROOT ne sera alors lancée simultanément.

Votre réservation d'index pour `qualify full meb composition through order ten`
est vue et respectée. Aucun fichier constructeur préparé. Votre confirmation
d'absence de nouveau moteur est reçue ; les captures mono seront séquentielles,
après l'admission bornée, avec toutes les compilations ROOT déjà closes.

## Croissance 8k / 16k / 32k — priorité utilisateur

L'utilisateur demande maintenant explicitement de contrôler la croissance
sous-quadratique dans les différents régimes. ROOT réordonne le plan avant
son gel : le triplet 8k/16k/32k à s8 vient d'abord, chacun P0/grand P,
puis s10 et s12. Les refus/censures ne sont pas des succès de tour ni
des points permettant de calculer un exposant de temps complet.
Le contrôleur `ee9d4640…`, carte de 58 sources `f0b5afcd…`, entre en
admission bornée ; aucun temps lourd avant clôture des compilations.

Question mathématique précise pour vous : dans notre domaine FULL 3D
régulier, K fixe≤10, la famille d'arcs liés établit-elle un nombre
quadratique de **minima FULL effectivement distincts**, ou seulement
de candidats/catalogues intermédiaires ? Ne pas transférer une borne
Delaunay ou du Gabriel brut à la sortie FULL sans ce raccord. Je distingue
le coût de sortie du surcoût du constructeur ; une affirmation universelle
de sous-quadraticité exige une preuve, pas trois timings. La campagne
uniforme actuelle ne représentera pas tous les régimes géométriques.

Votre `3e62aadd` est vu publié. L'admission C++ de la sonde est close à
10:13:55 UTC : build O3 puis 72 micro-exécutions / 468 ordres ; 413
commandes micro toutes closes, porte locale O2/SAN 111+45 appels et
mutant q4-first causal. Aucun lourd lancé pour l'instant. Les trois écarts
de transport du comparateur ont été trouvés aussi en relecture ROOT et
transmis à son auteur **avant** l'exécution du comparateur sur captures :
le brouillon est corrigé vers les vrais snapshots/streams/intentions,
sans changer les captures gelées. Son ancien modèle reste explicitement
synthétique ; nous attendons 48 vraies paires / 312 ordres comparés avant GO.

Une preuve autonome de borne de sortie est également en cours de rédaction :
deux petits arcs rationnels en 3D ont toutes leurs paires croisées Gabriel
strictes, donc des minima FULL à K2. Elle distinguera précision croissante,
univers u16 fini et sortie explicite ; aucun transfert depuis Delaunay.

**GO mono après clôture de l'admission réelle.** Le comparateur corrigé
`910b30ac…` réadmet maintenant les 72 vraies tentatives : 48 paires / 312
ordres comparés, 25 anciens compteurs et tous les autres champs non mesurés
égaux. Les sorties normal/-O sont identiques (reçu `37f6a202…`). Le
brouillon de transport antérieur n'a lancé aucun moteur et reste conservé
comme préparation, pas comme qualification des captures réelles.
Toutes les compilations ROOT sont closes. Début des tentatives CPU6
séquentielles ; merci de ne pas ouvrir de nouveau moteur ou calcul lourd
pendant cette fenêtre. Les lectures/écritures de documents restent libres.

## Sommets Gabriel seuls et suppression des plafonds — 6 septembre

La première tentative n8000/s8/P0 est close : sortie 0, groupe fermé,
159,160 s horizontaux, sans comparaison P0/P>0 encore exécutée. Aucun
moteur ROOT actif, aucune session GCP. Votre réservation d'index actuelle
est vue ; aucun fichier constructeur ne sera indexé pendant celle-ci.

Nouvelle demande explicite : étudier si les sommets de Gamma_K peuvent
être les seuls (K−1)-simplexes Gabriel, au lieu des facettes des K-simplexes
Gabriel, puis supprimer les plafonds de travail arbitraires de la sonde.
Merci d'une contrelecture mathématique indépendante : distinguer le graphe
induit naïf (deux minima facettes d'une même coface Gabriel), le quotient
avec rattachements/chemins silencieux et une éventuelle adjacency directe
entre régions témoins des seuls minima. Les minima suffisent-ils comme
sommets filtrés avec des arêtes explicitement constructibles, et quelle
information reste indispensable pour niveaux, parents, points, verticale
et poids ? Une contre-fixture rationnelle/u16 du graphe naïf et une preuve
de suffisance du quotient seraient utiles ; ne pas confondre la filtration
HGP du manuscrit avec celle d'un graphe k-NN ordinaire.

ROOT conserve temps/RAM, arithmétique vérifiée et limites des types ; les
quotas d'opérations ne doivent plus censurer la campagne à 32k. Les
captures précédentes restent gelées, avec leurs anciens budgets. Vos
bornes p≤146×appels et certified≤c−A≤550×certified sont lues et seront
prises en compte dans la nouvelle admission. Aucun moteur lourd demandé
à l'auditeur pour cette question.

L'utilisateur insiste sur la difficulté du meilleur reconstructeur et
demande explicitement de poursuivre notre échange. Une nouvelle fixture
constructeur n4/K2 est trouvée par l'oracle rationnel et contre-calculée
algébriquement par ROOT : A=(1,1,7), B=(5,2,1), C=(7,2,2), D=(5,2,8).
Minima AB,AD,BC,CD ; BD a l'intrus C (puissance −2). BCD, directe de
niveau carré 49/4, installe BD dans la composante BC/CD ; ABD, directe
de niveau 477/34, fusionne cette composante avec AB et AD. Restreindre
aux minima produit au contraire les deux groupes AB/AD et BC/CD. Même
le graphe d'intersection complet sur minima retarde cette fusion à 31/2.
Cette fixture isole la suppression d'un sommet non-Gabriel sans nécessiter
de coface silencieuse ; elle complète E5 plutôt que la remplacer.

La nouvelle note `docs/SQUELETTE_MINIMA_GABRIEL.md` formule le quotient
filtré avec poids minimax des chemins Gamma, puis sa forêt de L−R arêtes
sur des représentants de minima. Cette suffisance informationnelle n'est
pas présentée comme un algorithme rapide pour découvrir ces poids.
Merci de pousser surtout cette frontière algorithmique : existe-t-il
une caractérisation locale certifiable des connexions entre bassins de
minima évitant les résolutions de toutes les facettes directes ? Une
sélection des seuls bras essentiels avec arrêt après q−1 connexions, une
descente de bassins memoïsée, ou une recherche de coupes de type Borůvka
peut-elle être justifiée sans oracle circulaire ni énumération quadratique
des paires de minima ? Toute prétendue économie doit préserver les parents
pré-lot, leurs identités, et les ancres des directes sans fusion réutilisées.
La v7 ne matérialise déjà pas toutes les facettes comme feuilles ; le gain
reste à chercher dans la découverte/réutilisation des connexions, pas dans
un renommage de son certificat de sortie.

Votre `08cf65dc` est vu publié, réservation d'index de ce lot close selon
votre règle. ROOT ne prépare pas encore d'index. GCP non utilisé.

Précision utilisateur suivante : l'idée de représentation peut évoluer,
la cible est la tour K-NN 3D la plus simple/rapide possible, sous les
contrats 50k et dizaines de millions G4. Point à intégrer à votre analyse :
une directe de cardinal m≤Kmax est aussi une feuille FULL obligatoire de
l'ordre m. La supprimer parce qu'elle ne fusionne rien à m−1 n'économise
pas sa découverte pour la **tour**. Seul le dernier rang Kmax+1 n'a pas
ce second rôle de naissance demandée. Merci d'évaluer les optimisations
sur cette géométrie partagée, pas seulement sur un ordre isolé. Garder
L dépendant des données et la borne quadratique explicite distincts des
contrats matériels bornés ; aucun rapprochement approximatif autorisé.

Votre nouvelle note sur la descente à cardinal K est lue intégralement.
La preuve est cohérente ; ROOT l'a ajoutée comme variante à qualifier,
sans changement silencieux du resolver. Le petit différentiel rationnel
constructeur trouve déjà le compromis J1 : sur A=(0,3,3), B=(3,2,9),
C=(8,6,12), D=(12,9,3), E=(13,6,11), K2, BD consommée par ABD à
1909/41, le raccourci actuel BD→BCD fait une MEB, alors que la descente
BD→CD→DE en fait deux. À l'inverse E5 offre une économie. Les chiffres
seront conservés comme modèle rationnel, pas comme mesure C++.

Question spécifique **tour verticale**, pour ne pas surévaluer la mémoire
économisable : pour chaque Q Gabriel de rang m≤Kmax, son ancre horizontale
fermée dans l'ordre m−1 n'est-elle pas exactement l'ancre verticale requise
par la feuille Q de l'ordre m ? Si oui, ces valeurs restent nécessaires
dans la tour, mais pourraient partager leur stockage/index avec les
feuilles supérieures ; les supprimer du seul resolver horizontal ne donne
pas toute cette économie de sortie. Le rang Kmax+1 est encore distinct.
Un hybride conservant le J1 rapide quand cette ancre partagée est disponible,
puis descendant les facettes sinon, paraît donc à comparer plutôt qu'un
remplacement systématique. Merci de préciser la naturalité aux plateaux
et le risque de confondre token supérieur et ancre inférieure.

### Mesures directes à la demande utilisateur

Le lot auditeur `dad414cb` est vu sur main, merci. L'utilisateur demande
maintenant d'arrêter la surenchère de garde-fous : suivre les scripts et
les arrêter s'ils tournent en rond. ROOT lance donc les mesures directement
avec un enregistreur simple, sans attendre une nouvelle admission de format.
CPU6 est réservé au seul moteur FULL mono pendant les essais 8k/16k/32k,
commençant à s8/P=unlimited. Pas d'autre moteur/compilation en parallèle.

Le binaire fraîchement compilé est `4938b94b…`, sources de calcul stables.
La micro r1 est close en échec de protocole : un selftest first-C ne
renvoyait pas `successor_accounting`, sans effet sur le moteur C++. Les
premiers tests C++ et données brutes sont conservés, sans prétendre que
la campagne entière est passée. La correction est une ligne du lecteur.
Aucune réécriture des captures ni nouveau protocole volumineux. GCP non utilisé.

Premier résultat direct : 8k/s8/P=unlimited réussit les dix ordres à
133,038 s avant terminal ; digest `e6e3fa51…` identique au P0 clos de
159,160 s. FULL 50,477 s contre 73,798 s. Les 4 305 891 MEB sont toutes
certifiées par le proposeur (24 777 382 formes proposées, aucun repli F).
Un passage n'est pas une accélération robuste ; P0 du même nouveau binaire
reste à mesurer. 16k est en cours, sans boucle bloquée observée.

Prochaine simplification minimale proposée par la contrelecture interne :
préserver J=1 ; après census J≥2, chercher seulement le label
F′=(F−premier essentiel)+premier intrus dans le catalogue des minima.
S'il y est avec niveau strictement inférieur à beta(F), normaliser son
token et terminer ; sinon reprendre la route actuelle sans autre MEB ni
census ajouté. Aucun nouveau domaine de boules visitées, un lookup
supplémentaire par J≥2. Ce terminal d'un seul pas pourrait tester votre
nouvelle descente sans remplacer toute l'architecture. Merci de réfuter
si une obligation de parents/plateaux/ancres échappe à cet argument.
Pas encore de changement produit ni de gain C++ revendiqué.

### Priorité utilisateur : élimination par blocs WSPD, h/h_a/h_b

L'utilisateur demande explicitement de renforcer cette élimination pour la
génération. ROOT relit `FRONT_ET_TEMOINS_COURANT.md`, `S1_COURANT.md`,
`witness_count.hpp`, `spindle.hpp` et la double boucle de `generate.hpp`.
Merci de concentrer la prochaine contrelecture sur ce poste, distinct du
rejet tardif des groupes de racines q4. 32k/s8/P=unlimited tourne sur CPU6 ;
pas de compilation ni moteur concurrent. Les dix ordres 16k ont réussi.

Deux points précis à éprouver :

1. Pour un rectangle vivant, min_a h_a=min_b h_b=0 : choisir dans A le
   point le plus proche d'une extrémité fixe de B ; un témoin interne au
   fuseau donnerait un point de A encore plus proche. Les min globaux
   n'ajoutent donc aucun rejet au seul cœur. Un sous-bloc A′×B′ peut en
   revanche réutiliser les témoins des **facteurs originaux**, avec
   h_coeur+min_{A′}h_a+min_{B′}h_b≥h_q ; ne pas modifier leur population
   puis sommer des crédits qui se recouvrent.
2. Après les lignes ha≥need déjà éliminées, toutes les lignes de même
   crédit ha ont exactement la même liste stable de B admissibles :
   hb<need−ha. Construire au plus h_q listes par seuil évite de rebalayer
   tout B pour chaque a, sans changer l'ordre Morton des ancres survivantes
   ni les crédits effectivement transmis. Saturer les histogrammes à need
   est également sûr si les comptes des survivants restent exacts ; les
   évaluations évitées doivent être distinguées de P_factor historique.

La piste plus ambitieuse est de créditer des sous-arbres entiers pour ces
histogrammes, comme pour h_coeur, au lieu du produit quadratique A×A et B×B.
Merci de préciser un certificat strict de bloc valable en q3/q4, les
contraintes de disjonction et si ses tests peuvent coûter moins que les
coins actuellement payés. Aucun raffinement WSPD post-séparation aveugle
n'est proposé : l'ancien résultat négatif +34 % reste un avertissement.

ROOT prépare l'index du lot constructeur « remove full probe work quotas
and prove the minima quotient » : seuls code/notes constructeur et deux
paquets `receipts/full_probe_no_quotas_20260906/` et
`receipts/full_minima_quotient_20260906/`. Aucun fichier de votre dossier
`audits/`, notamment `receipts_shared_anchors_20260906/`, n'est inclus.
L'index est vide à la prise de réservation ; libération après push main.

Pour les histogrammes par blocs de témoins Z : à a,b fixés,
W_q est aussi convexe en z. Avec m=(a+b)/2 et projection perpendiculaire
y⊥ de z−m, le test q3/q4 s'écrit
`|z−m|² + |a−b| |y⊥| / sqrt(t) < |a−b|²/4`, t=3/2.
Il suggère un crédit de sous-arbre via tous les coins de Z puis de B,
en gardant a fixé : la convexité en z et celle, séparée, en b suffiraient
à étendre les tests stricts aux deux boîtes. À confronter au coût du test
de coins et à un minorant Hmin/majorant Xi moins cher ; aucune identité
du seul « cœur rond » ne remplace cette justification près des extrémités.
Merci de contre-vérifier cet usage de convexité avant tout port du crédit.

En parallèle, premier delta de coût sans nouveau rejet étudié : au
rectangle terminal, le compte q2 `with_corners=false` est déjà complet
et ne dépend pas de ce booléen. Le reprendre au lieu de le refaire dans
le second appel `with_corners=true` est une économie distincte, à mesurer.
Le second appel peut être limité aux lanes q3/q4 encore ouvertes. Aucun
changement des cœurs remis, des masques ou des masses n'est attendu.

Lot constructeur publié : `a9ce3639` sur `origin/main`. Réservation d'index
libérée ; l'index est vide après ce commit. Vos fichiers restent exclus.
Le triplet direct est encore en cours (32k a franchi K9 à 4 605 147 MEB
sur cet ordre, puis poursuit K10) ; aucun résultat complet 32k anticipé.

### Réponse au certificat de blocs du 6 septembre

Votre lot `receipts_block_histograms_20260906/README.md` est lu intégralement.
ROOT retient le premier prototype à ancre fixée, H_min/Ξ_max entier, crédit
du nombre de positions du sous-arbre original, feuilles aux coins actuels.
La boule-cœur centrale est écartée de ce poste pour la raison démontrée ;
le rejet hmax avec U variable et la double comptabilisation des populations
parent/enfant deviennent des avertissements explicites dans les notes.
Le prototype reste privé tant que ses histogrammes et son coût ne sont pas
confrontés au scalaire ; pas de changement massif de la route produit.

Le delta q2 terminal est maintenant intégré : différentiel O2 et ASan/UBSan
sur 174 appels par bras, cœurs/rectangles/masses/ordre/coins identiques,
1 283 visites évitées, six témoins n2/q2 de 6 à 3 visites. Le gate permanent
passe les deux compilations et le rejet argument ; pas de gain de temps
déduit de ces fixtures. Les mesures FULL fraîches s8/10/12 suivent sur CPU6
avec le binaire `23646a32…` ; merci de garder ce CPU sans moteur concurrent.

Le triplet direct antérieur s8/P=unlimited est clos : 133,038 / 307,643 /
684,574 s pour 8k/16k/32k, dix ordres chacun. Le P0 du même binaire est
également clos à 8k (154,837 s externes), en cours de comparaison détaillée.
Ces mesures ne comprennent pas de verticale intégrée ni d'archive retenue.
Votre réservation d'index pour le lot WSPD est respectée ; aucun fichier
de votre dossier ne sera inclus dans le lot constructeur suivant.

Le commit auditeur `4931906b` est vu sur main ; merci. Un second candidat
privé vise le coût du **cœur h**, distinct des histogrammes : calculer la
séparation avant le premier compte ; pour un terminal, appeler directement
une seule fois `count_universal_witnesses(..., with_corners=true)`, pour les
autres garder `false`. Le compte avec coins domine le compte sans coins,
lane par lane et après écrêtage ; les cœurs remis, masques et rectangles
devraient donc rester identiques au double passage actuel. En revanche,
des coins supplémentaires peuvent être payés sur des lanes que le premier
passage peu coûteux tuait déjà. Nous comparons cette variante séparément,
sans supposer son gain ni la porter tout de suite. Merci de signaler une
obligation mathématique autre que cette domination et la stabilité des
comptes finaux. Pas de modification de `count_universal_witnesses` envisagée.

L'utilisateur confirme la suite : clore ce lot mono, puis multi-CPU local,
puis G4 SPOT 48 CPU et GPU pour les contrats. ROOT prépare cette transition
sans confondre la construction horizontale actuelle et la tour intégrée.
Le wrapper GCP actuel vise encore F et des primitives device ; il ne sera
pas lancé comme s'il qualifiait déjà FULL.

ROOT réserve maintenant l'index, observé vide, pour le lot q2 terminal et
les captures directes 8k/16k/32k, P0/P∞ et s8/10/12. Aucun fichier de votre
dossier `morsehgp3D_v7/audits/` ni de la v6 n'est inclus. Les prototypes
de crédits histogrammes et de terminal à un passage restent privés et
seront décidés séparément après leur mesure.

La preuve du terminal unique dans `receipts_terminal_count_20260906/README.md`
est lue : domination ET indépendance des lanes sont retenues. Le contrôle
de cœur q2 positif demandé est ajouté au gate, en cours de recompilation
O2/SAN avec mutant d'omission du transfert ; les anciennes captures sont
conservées avec leur ancien gate. Merci pour cette lacune de test identifiée.

Le terminal unique est correct sur 174 fronts et sur les 754 686 rectangles
du cas uniforme 8k, mais **non retenu pour intégration** : O2, une paire
front seul, 37 767,10→38 286,55 ms ; visites 563 616 452→547 864 549,
coins 167 115 088→335 509 837. Le risque annoncé se réalise : les lanes
tuées économiquement auparavant paient maintenant des coins. Aucun gain
total ni conclusion statistique n'en est déduit. Les deux processus sont
clos ; le paquet négatif sera publié séparément. L'index reste réservé
au lot constructeur, sans vos nouveaux fichiers.

Histogrammes sur le front uniforme 8k : 8 436 096 valeurs égales,
93,819 ms scalaire / 186,560 ms blocs forcés / 101,318 ms hybride8,
dans le chronométrage instrumenté. Maximum des facteurs : sept ; aucun
bloc actif dans hybride8. Le scalaire est conservé sur ces petits facteurs.
La conclusion n'est pas généralisée : ROOT demande ensuite le rectangle
racine de deux amas compacts très séparés, A∪B=X donc h_cœur=0, facteurs
de n/2 positions, à 8k/16k/32k. C'est le régime réellement quadratique du
calcul actuel, absent du front uniforme ; aucun garde-fou de temps/CPU
n'est ajouté à ces mesures directes. Le raccord pipeline multi-CPU se
prépare en parallèle, sans exécution benchmark concurrente.

L'audit local des constructions entre K confirme des API horizontales
indépendantes : index/catalogues constants partageables, Builder/DSU/cache/
Work privés. L'émission et le digest doivent rester ordonnés par K, et
la mémoire cumulée doit être comptée. Aucun parallélisme inter-K n'est
encore implémenté ni déduit du seul futur paramètre --threads du pipeline.

Question ciblée pour les grands facteurs : le prototype h_a/h_b sait
créditer des blocs mais ne rejette q3/q4 que via W2/Hmax. Les blocs situés
hors du cône q3/q4 mais dans W2 risquent donc encore une descente jusqu'aux
feuilles. À ancre a fixée, choisir un point b₀ de Box(B), puis calculer
M4=hmax4_boxes({a},{b₀},Z), soit quatre fois un majorant de H, et Ξ_min
par la distance de chaque intervalle de composante du produit vectoriel
à zéro, carrés sommés. Si M4≤0, ou si M4>0 et t M4²≤16 Ξ_min,
t=3/2 selon la lane, tous les z du bloc échouent pour b₀. Ils ne peuvent
donc pas être universels sur Box(B). Le choix d'un coin suffit ; un point
entier intérieur à la boîte paraît aussi valable par convexité du
prédicat en b. Merci de réfuter/vérifier ce certificat de **non-crédit**,
distinct d'une mort d'ancre, ainsi que les quantificateurs et l'i128.
Il n'est ni implémenté ni mesuré pour l'instant ; la sonde des deux amas
doit d'abord montrer le travail restant. Aucun hmax à U variable proposé.

Le lot constructeur `19ff070a` est maintenant poussé sur main : transfert
q2 terminal, gate positif et mutant, mesures directes et deux pistes
négatives conservées. L'index est libéré, aucun fichier de votre dossier
n'a été inclus. Le raccord de la sonde aux workers du pipeline est appliqué
et compile en répertoire neuf ; FULL et la boucle K restent séquentiels.
Les mesures des deux amas passent avant les mesures multi-CPU de taille utile.

La réponse de non-crédit est lue, y compris la distinction entre réfuter
l'universalité sur Box(B) et réfuter un site réel de B, et la contre-fixture
non vacue dans W2. Elle servira à un prototype séparé, avec rejet à l'égalité
et mutant Xi_max à la place de Xi_min. Le prototype positif seul montre
déjà sa limite : deux amas 16k, q4 2,925 s scalaire contre 3,115 s blocs,
avec croissance quasi quadratique des visites q3/q4. Le 32k est en cours.
Une seconde analyse prépare la saturation exacte des histogrammes à
need=h_q−h_cœur : tous les crédits transmis aux ancres survivantes doivent
rester identiques, et P_factor devra compter le travail physique réellement
payé. Aucune de ces deux extensions n'est encore dans le producteur.

Votre commit `56c6e0a8` est vu ; le triplet grands facteurs est publié
dans `receipts/wspd_large_factor_histograms_20260906/`. Le multi-CPU local
termine à 8k en 132,962 / 98,195 / 74,577 s externes pour 1/2/4 threads,
même binaire, FULL encore séquentiel ; l'essai 8 threads SMT est en cours.
Pas de nouveau commit constructeur réservé à cet instant.

Point d'implémentation mémoire pour G4/50k : la sonde conserve le prétest
U*(sizeof(BallCandidate)+sizeof(Survivor)+2*sizeof(BallData)), soit 608U
sur cette ABI, avant le préfiltre. Or le census v7 nominal ne garde plus
qu'un tableau BallData (la deuxième copie est uniquement sous mutant).
ROOT fait vérifier une admission par phases : préfiltre candidats et
copies de Survivor, puis candidats et Survivor réellement survivants plus
un seul BallData par survivante avant census. Le budget nommé est un
proxy logique, pas un plafond RSS ni une raison pour conserver une copie
nominale inexistante. Merci de signaler une coexistence réelle oubliée.
Pas de changement de cette admission dans les mesures multi-CPU en cours.

Les quatre mesures multi-CPU sont closes ; huit threads SMT termine à
69,853 s externes. Les dix digests et tous champs d'ordre hors mesures
sont identiques, seuls les six workers changent au terminal hors mesures.
Le lecteur normal/-O confirme ces comparaisons ; aucun gain statistique
ni contrat 50k revendiqué. L'utilisateur demande de minimiser le budget :
aucune session GCP ouverte, pas de nouveau benchmark lourd dans ce lot.

ROOT réserve l'index, observé vide, pour le raccord de la sonde multi-CPU,
ses micros/mesures, le triplet grands facteurs et leurs documents.
Aucun fichier de votre dossier ni de la v6 ne sera inclus. Les prototypes
négatif/saturation et la correction du proxy mémoire restent séparés.

Le petit supplément `receipts/wspd_noncredit_saturation_20260906/` est
également conservé dans ce lot constructeur : 432 comparaisons O2/SAN,
centre non-site et mutant Xi_max réfuté, huit commandes closes. Aucun
benchmark supplémentaire, aucun changement produit de ce helper privé.
La correction mémoire reste seulement analysée : 176U au préfiltre puis
144U+240S avant census sur cette ABI, proxy logique hors capacités/RSS.

Le lot constructeur `ca2930c5` est poussé sur main et l'index est libéré.
L'utilisateur renouvelle le feu vert aux essais GCP SPOT. ROOT prépare
une route CPU48 FULL distincte, sans réutiliser le parseur F. La correction
du proxy mémoire sera d'abord appliquée et testée localement ; la session
restera ciblée et fermée par les gardes existantes. À cet instant, aucune
création ni aucun démarrage GCP n'a été demandé.

La correction mémoire est appliquée et qualifiée séparément : 40 contrôles
O2/SAN, quatre micros P=0/illimité et 1/4 threads, deux CTests nouveaux.
`run.hpp` est inchangé ; le champ census_payload_accounting distingue le
nouveau proxy. ROOT réserve l'index pour ce petit lot et les scripts FULL
GCP séparés, sans vos fichiers ni ceux de la v6. La cible SPOT déjà arrêtée
`devpod-gpu-exploration/us-central1-b/ehgp-v7-4fa0e0789a7d5bb06b787d35`
sera réutilisée avec double garde, sans nouvelle VM ni nouveau disque.

Votre commit `68713557` est vu et la réservation auditeur est close.
Lecture faite de la distinction U/S, de l'inaccessibilité du refus
préfiltre après admission du tri, et du cap global need plutôt que
d'un cap recyclé propre à une ligne. Les contrôles C++ actuels couvrent
les frontières arithmétiques et S<U, mais ne sont pas un mutant de la
couture allocation après vrai préfiltre. Les micros passent sur le
chemin nominal ; cette portée restera explicite. Aucun nouveau résultat
de performance ne sera attribué au seul changement d'admission.

Lots constructeur `f4ffe38c` et `0072c88d` poussés ; index libéré.
La session SPOT CPU48 est demandée sur la cible annoncée, via les gardes
épinglées. Sources limitées aux 42 dépendances réelles de la sonde, pas de
v6 ni de worker F. Smoke n=8, puis deux exécutions réelles 50k K10/K5 si
le premier contrat échoue ; pas de prétention GPU. ROOT conserve la
responsabilité de récupérer les captures et certifier l'arrêt ciblé avant
passage de relais. Les résultats et la génération exacte suivront.

Session close : génération `2026-09-06T06:19:11.593-07:00`, arrêt ciblé
code 0 et état TERMINATED relu ; aucune autre VM active signalée.
Le smoke n=8 passe. Les deux vrais processus uniformes seed=3 à 50k/s8
refusent avec `probe_rank_relevant_extra_shell`, code 2, avant tout ordre
FULL : quatre boules à coquille supplémentaire pour K10, trois pour K5.
Temps externes de refus 21,372 s et 5,646 s ; pas des temps de tour.
L'admission corrigée passe : U=21 685 604 et S=21 468 368 pour K10.
Les captures sont en publication sous receipts/full_g4_spot_50000_20260906/.

Question mathématique/architecture concrète : le test de régularité de la
sonde rejette globalement toute BallData de la fenêtre dont n_shell diffère
de l'arité. Quelle information locale permettrait de distinguer un vrai
plateau à traiter d'une boule sans effet sur les naissances/multifusions
FULL et leurs parents ? Il ne s'agit pas d'ignorer ces quatre cas ni de
changer la seed pour les cacher. Nous devons extraire les supports,
coquilles et intérieurs en fixtures minimales, puis décider si la garde
est nécessaire au contrat actuel ou si une extension exacte par lots
de niveau peut les traiter. Le nuage u16 50k rend ce sujet industriel,
pas seulement pathologique. Aucune relaxation du producteur appliquée.

Le paquet G4 est maintenant clos et vérifié normal/-O : 79 fichiers,
42 références de sources sans copie, aucun ELF ni clé. Manifeste
`54bd37110187a39e62d85f543f407440515fdc72d0029d5445236766b2be1d72`.
ROOT a lu votre suivi census `d7828138` ; la fixture injectée U=5,S=4
reste un contrôle dynamique utile à ajouter, distinct des micros S=U.
ROOT réserve l'index observé vide pour publier les captures SPOT et les
entrées de résultats, sans fichier de votre dossier ni de la v6. La
réservation se termine automatiquement à la publication de ce lot.

Le lot constructeur `638205bb` est poussé, index libéré. ROOT a lu vos
contrats de plateau puis `BALL_ANCHORS.md` : croissance sans multifusion,
couverture initiale non réduite à une K-facette, intervalle d'ancres
p+q_min−1..p+u et distinction inertie publique / terminal du resolver.
La garde FULL n'est pas relâchée. La fenêtre p+q_min≤smax reste distincte
du faux filtre p+u≤smax hors régularité.

L'extraction locale réelle 50k/s8/K10 est maintenant close, code 2,
quatre JSONL dans
`build/v7_extra_shell_20260906/run_r3/n50000_k10.stderr`
(SHA `3cd74b330c62978d8c3eedd175e12bf5fe02893facb2e008150c32b5054aea72`).
Même digest d'entrée que G4, U=21 685 604 et S=21 468 368. Chaque
enregistrement contient BallKey exacte, niveau rationnel, identifiants
externes, rangs Morton et coordonnées de tout I/U. Les quatre coquilles
ont u=3 et arité émise 2 ; leurs intérieurs ont p=3,0,4,9 dans l'ordre
du fichier. Un lecteur indépendant reconstitue les 50 000 points et
vérifie le census complet de ces seules boules ; il ne certifie pas
l'exhaustivité du catalogue. Les parents globaux restent à résoudre.

Un helper privé de quotient local est en qualification O2/SAN contre
un oracle rationnel séparé : table des 2^u masques, supports minimaux
q≤4, étoiles de cofaces strictes, branche analytique K≤p sans combinaisons
d'intérieurs. Les quatre cas extraits seront des fixtures permanentes.
Pas de nouvelle session GCP : la précédente cible reste arrêtée et nous
travaillons sur ces petits cas. Aucune réservation constructeur de
l'index à cet instant ; vos fichiers en préparation restent exclus.

Le lecteur réel est clos normal/-O, mêmes octets : quatre scans complets
de 50 000 points par mode, q_min=2 confirmé et un unique diamètre positif
par coquille. Les indices 174406 / 254569 / 996863 / 1251653 ont leurs
intervalles d'ancre K4..6 / K1..3 / K5..7 / K10 dans la tour demandée.
La dernière boule (p=9,u=3) est localement inerte partout jusqu'à K10,
mais nous conservons l'obligation de son ancre K10. Les trois autres
ont respectivement deux composantes strictes locales à K5 / K2 / K6,
puis aucune au rang suivant. Aucun compte de parents globaux déduit.
Comparaison historique G4 : mêmes 84 champs de configuration et 67
champs terminaux communs hors mesures/threads/diagnostics ; les anciennes
clés G4 n'ayant pas été exportées, leur identité n'est pas affirmée.
Votre `30d2a4dd` est vu ; entrée constructeur
`docs/PLATEAUX_FULL_ET_ANCRES.md` en préparation, sans nouvelle autorité FULL.

Question pour le raccord du certificat, distincte du quotient : pour
éviter de maintenir un ensemble complet de points par racine, peut-on
encoder une contribution de couverture I∪U (référence de boule) à chaque
bloc fermé, éventuellement redondante avec les couvertures antérieures,
puis faire l'union exacte au lecteur de coupe ? Cela semble restituer
les couvertures sans exiger que chaque delta soit l'ensemble minimal des
seuls points nouveaux. Il faudrait distinguer explicitement ce journal
de contributions d'un vrai delta disjoint et borner son volume par les
blocs utiles, sans gonfler le format avec toutes les facettes. Votre
preuve porte-t-elle aussi sur cette forme factorisée du certificat ?
Nous ne changeons pas la sémantique du delta en cachette et ne proposons
pas de faire de chaque contribution un nœud public.

Le paquet d'extraction est scellé sous
`receipts/full_extra_shell_50000_20260906/` : 148 fichiers dans le
manifeste `5b85ae1a8f9b95c4b5832af3940afcd8447c495a84a9f5e521943fd48f07c4bf`,
sans ELF, avec sources communes référencées plutôt que recopiées. Les
33 contrôles O2/SAN et deux CTests passent ; échecs préliminaires conservés.
ROOT réserve l'index observé vide pour le diagnostic, ce paquet et les
documents de plateau, sans vos fichiers ni ceux de la v6. Réservation
close automatiquement à la publication de ce lot. Le quotient local
C++ reste un second delta, jamais requalifié par cette extraction.

Lot `56ace8d8` poussé sur main, index libéré. Votre réponse factorisée
est lue : D_B est une contribution potentielle locale, pas un delta
global disjoint ; ses points restent liés au segment temporel post-lot.
Le helper local ajoute le masque de coquille correspondant et une
référence à I pour les naissances. Ni ensemble global par racine ni
nouveau nœud public pour une continuation. La qualification de ce
composant local reste distincte du raccord FULL et de la preuve S1.

La qualification promue est close : O2/SAN codes 0/2, mutant d'étoile
réfuté par `quotient.strict_component_count`, deux CTests frais réussis.
18 tables / 96 rangs contre l'oracle, 40 rangs réels supplémentaires,
quinze commandes closes. Paquet `receipts/local_plateau_20260906/`,
manifeste de 150 fichiers
`7386d0173e1707b5c9efeb1dde96c919fb8fc06879465f649c6f742fa4c45609`.
Le header ne change aucun producteur FULL ; la garde reste inchangée.

Votre contrelecture favorable est lue, ainsi que le point ciblé du juge :
la table entière contient/évite le centre est comparée à Gram, mais le
vecteur `minimal_supports()` n'a pas encore son propre contrôle
d'exhaustivité sur toutes les fixtures. Les quatre cas réels vérifient
leur unique support exact. Ce manque de couverture du test n'est pas
présenté comme un défaut géométrique observé ; nous le conservons comme
prochain renforcement, avec un mutant qui retire un seul diamètre du
carré sans changer la table. Les seuils h_x partagés sont une seconde
piste distincte, pas une accélération déjà intégrée.

ROOT réserve l'index observé vide pour le petit composant local, ses
tests/reçus et ses entrées ; aucun fichier auditeur ou v6 inclus. La
réservation se termine à la publication de ce lot.

Lot précédent `7debdbab` publié et index libéré. Suite `22003315` lue
en entier : le contrôle de tous les supports minimaux et le raccourci
q_min=2,u≥3,K=p+1 sont maintenant qualifiés O2/SAN, quatre mutants
causaux réfutés. Les 17 raccourcis testés ne construisent aucune DSU ;
aucun temps global n'en est déduit.

ROOT a lu votre nouveau `GLOBAL_PARENTS.md` en préparation : images
pré-lot 1/2/2 pour 174406/K5, 254569/K2, 996863/K6. La taille deux
n'est pas réinterprétée comme l'arité totale d'une multifusion réunissant
d'autres boules au même niveau. Ces verdicts deviennent les attentes
du prochain raccord, pas une suppression de la garde générale.

Nouveau composant `src/forest/full_coverage_certificate.hpp` en
qualification, schema `full_dated_coverage_forest_v2`, autorité seulement
structurelle. Banque I/U immuable partagée entre ordres ; naissance avec
population entière, continuation sans nœud, multifusion à parents pré-lot ;
contributions masquées datées sur le segment post-lot. Les successeurs
historiques ne sont jamais compressés vers une racine future. Pas de set
par racine au constructeur, ni copie du certificat régulier v1 en v2.
O2/SAN : 710 contrôles, 30 coupes de rejeu, 34 coupes Gram/Gamma, 34
pannes d'allocation réfutées ; trois mutants croissance/dates/racines.
Une première revue interne a corrigé un alias mutable de la banque avant
qualification. Toute contrelecture mathématique reste bienvenue.
Six CTests ciblés frais sont en cours ; aucune réservation de l'index
constructeur pour l'instant, vos fichiers préparatoires restent exclus.

Les six CTests sont clos, tous réussis ; paquets constructeur scellés :
`local_plateau_diameter_20260906` (113 fichiers, manifeste
`e286f78a196add062cda7a7f96f59668973fe7110d61d4849fdeb07911311111`)
et `full_coverage_20260906` (79 fichiers, 17 sources projet liées,
manifeste `54b7ae3066b43b474982714f1cbcf920b36271e8b20ebb6717a1b4bbed0ed4c9`).
Contrôles portables normal/-O du journal et intégrité du paquet local
réussis. Documentation 405 fichiers / registre 20 phases validés sans
modifier le registre. Votre réservation des treize fichiers d'audit est
vue : ROOT ne touche pas à l'index avant votre publication.

Publication auditeur `a22a65f9` vue, réservation close. Les entrées
constructeur distinguent désormais les parents 1/2/2 établis et l'arité
du lot encore à reconstruire. ROOT réserve maintenant l'index observé
vide pour ses sources, gates, deux paquets et documents ; aucun fichier
de votre dossier ou de la v6. Réservation close automatiquement par
la publication du présent lot, tous les processus de test déjà terminés.
