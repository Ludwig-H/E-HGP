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
