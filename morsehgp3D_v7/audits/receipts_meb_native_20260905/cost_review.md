# Contrelecture du microcoût natif MEB v2

5 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`. Lecture et rejeu de données seulement ; aucun build, benchmark, appel MEB nouveau, produit ou GCP.

Le mécanisme apporte un bénéfice local observable sur une partie des recherches longues : il réduit fortement les candidats effectivement essayés à P401, et plusieurs lots q4 sont plus rapides dans les sept passages. Le cas de deux points montre simultanément un surcoût net de l'enveloppe privée, même à P0. Ces deux résultats sont utiles au constructeur. Ils ne donnent ni un gain de tour, ni un seuil de dispatch, ni un profil optimal de budget par ordre.

Le [reçu mesuré](qualification/snapshots/run/receipt.json), SHA256 `874f100ffb1d65956f6d640c5e7ab838a81e9f5c7900f7c1d69b14504235c208`, est `completed`, du 5 septembre à 11:50:13.229548 au 11:50:16.210694 UTC. Les 2,98 secondes décrivent l'intervalle de capture **après le préflight et les admissions** ; la durée de l'invocation entière du runner n'est pas mesurée. Le [brut conservé](qualification/snapshots/run/measurement.stdout.gz) contient 28 972 744 octets après décompression, SHA256 `2c20ceaf7a8a4757af2ad78554becf2e584f1c397e92860800d6c746de24469f`. La qualification séparée porte l'admission, les captures et le désassemblage ; cette note examine la population, la comptabilité et l'interprétation des temps.

Le [rejeu indépendant](cost/review_cost.py) ne charge ni le runner producteur ni un binaire. Il reconstitue les populations et appariements depuis le brut épinglé ; [normal](cost/normal.json) et [Python optimisé](cost/optimized.json) concordent. Les calculs statistiques emploient des fractions exactes. Les trois contrôles d'audit sont deux corruptions de données (septième paire ou bras manquant) et une contre-fixture contre le ratio des médianes non appariées ; ce ne sont pas des mutants produit.

## Comparaison effectivement mesurée

Le [harnais](qualification/snapshots/protocol/cost_harness.cpp) appelle les deux bras dans le même exécutable C++20/O2 strict, sans LTO ni instrumentation de test. `invoke_dual` instancie explicitement `NoObserver`. F et dual reçoivent les mêmes points, ordre, cap legacy et état initial. Les comparaisons complètes F/Trace/NoObserver précèdent et suivent les temps ; elles vérifient notamment état terminal, statistiques, événements, boule et Work. Les captures effectuées pendant chaque lot sont ensuite contrôlées. Le checksum ne remplace pas cette comparaison complète.

Les tableaux et index sont construits hors chrono ; les emplacements des deux événements sentinelles sont réservés hors chrono. Les resets, sentinelles de boule, appels indirects, barrières, hash de tous les champs du résultat et de Work, et comptage des replis sont **dans** le temps. Les durées décrivent donc le helper avec cette enveloppe et cette capture. Aucune soustraction de coût commun ne produit ici une mesure de helper nu.

Le contrôle P0 reste un appel au wrapper privé, son compteur de repli et son appel F. Ce bras ne se confond pas avec l'entrée F directe. Il permet de mesurer l'enveloppe, et ne doit être ni déclaré neutre par construction ni soustrait des autres temps. Le reçu concerne le CPU logique 6, un thread, sur l'hôte AMD EPYC 9V74 indiqué dans [l'environnement](qualification/snapshots/run/environment.json). Affinité et fenêtre dédiée améliorent la comparaison locale ; elles ne constituent pas une qualification de variabilité entre machines ou invocations.

## Population et compte des entrées

La matrice comprend 176 scènes, 384 ordres, n de 2 à 11, et les huit budgets P={0,1,4,5,15,16,25,401}. Pour chaque ordre, un appel F hors chrono donne R ; les trois plafonds sont L=R−1, R, R+1. Les 9 216 cas principaux comprennent les succès **et** les refus. R calibre cette matrice de budgets ; aucun support F n'est transmis au proposeur. Les groupes sont fixés avant les temps par cohorte, n, q de référence, P/L, compteurs initiaux, plafond de pivots, terminal et route. q de référence provient du résultat déjà jugé P0/L=R ; il ne transforme pas un refus de coquille en succès.

Les frontières ajoutent 117 cas à un appel et deux séquences de quatre et deux appels, soit 123 états. Les 12 jobs `immediate_q2` répètent chacun 4 096 fois l'un des deux ordres de la **même paire de points**, avec L1/L2 et P0/P1/P401. Au total : 9 347 jobs, 9 351 étapes de qualification et 58 491 appels de premier niveau par bras et passage. La partition en 4 699 groupes couvre chaque job une seule fois. Les six groupes q2 répétés ont chacun 8 192 appels par bras et passage ; leurs 49 152 appels représentent environ 84 % de ce total. Cette pondération volontaire suffit à rendre un ratio global du corpus peu informatif pour la tour.

La comptabilité indépendante retrouve exactement **1 325 812 entrées MEB**, sous le plafond de deux millions :

| Origine des entrées | Nombre |
| --- | ---: |
| Calibration R | 384 |
| Deux qualifications hors chrono : F/Trace et F/NoObserver, premier niveau | 74 808 |
| Leurs replis F imbriqués | 15 312 |
| Deux rejeux des frontières du donor, premier niveau | 492 |
| Leurs replis F imbriqués | 106 |
| Lots de chauffe et mesurés, premier niveau et replis imbriqués | 1 234 710 |
| Total | 1 325 812 |

Les 1 507 contrôles d'ordinaux n'appellent pas le MEB et ne s'ajoutent pas à ce total. `helper_entries` compte les entrées, tandis que les candidats F et proposés ci-dessous comptent les essais internes : leurs unités sont distinctes.

Les jobs frais réinitialisent Work/statistiques entre tentatives ; les deux séquences cumulatives les conservent entre leurs étapes. Ce choix est déclaré et correct pour ce protocole local. Il ne reproduit pas la succession des appels d'un ordre produit dont Work serait persistant jusqu'à épuisement de P. Le coût de ce dernier régime reste distinct ; les quatre appels du triangle ne représentent pas une distribution de tour.

## Appariement, dispersion et petits lots

Les 84 582 lignes de temps correspondent exactement à 4 699 groupes × deux bras × neuf passages : deux chauffes, puis sept passages retenus. Chaque groupe reçoit F→dual aux passages impairs et dual→F aux pairs. Les sept paires sont conservées, avec quatre passages AB et trois BA ; cette alternance est partiellement équilibrée. Ce sont des répétitions dans une seule invocation, avec groupes et jobs dans un ordre fixe, pas sept graines ni sept expériences indépendantes.

Le rejeu calcule, pour chaque groupe, la médiane et la dispersion des **sept différences appariées** `(dual−F)/appels` et des **sept ratios appariés** `dual/F`. Le ratio des médianes des deux bras serait une statistique différente. Q1/Q3 sont les médianes des moitiés ordonnées, avec la valeur centrale exclue pour un nombre impair ; les minimums et maximums restent disponibles. AB et BA ont leurs propres descriptifs. Aucun intervalle de confiance de population ou test de significativité n'est déduit de ces sept observations dépendantes.

`clock_tick_ns=30` est le minimum positif observé parmi 128 paires de lectures `now()`. Il ne démontre ni une résolution métrologique ni une borne d'erreur. Le seuil `short_batch<3000ns` est le diagnostic fixé par le protocole. Dépasser ce seuil ne certifie pas la fiabilité d'un lot.

Sur 4 699 groupes, 3 561 n'ont qu'un appel par bras et passage. 2 525 n'ont aucune paire mesurée dont les deux lots dépassent le seuil ; 877 en ont de une à six ; 1 297 ont les quatorze lots au-dessus du seuil. Les chiffres de petits lots restent conservés, sans les transformer en ratios interprétables à l'échelle du produit.

Le contrôle **P0 de la matrice principale** montre une forte sensibilité à l'ordre des bras, distincte du lot q2 répété ci-dessous. Ses 1 152 cas forment 556 groupes. Les médianes de ratio AB et BA sont de part et d'autre de 1 dans 458 groupes : AB<1<BA pour 447, et BA<1<AB pour 11. Dans les autres groupes, les deux médianes sont sous 1 pour 28, au-dessus pour 66, et l'une au moins vaut exactement 1 pour quatre. Parmi les 207 groupes dont les quatorze lots dépassent le seuil diagnostique, ces inversions persistent dans 155 groupes du premier sens et huit du sens inverse. Le filtre de durée n'efface donc pas ce phénomène.

Exemple conservé avec ses sept paires dans `P0_main_order_control` des reçus de rejeu : n9/q4/P0/L210, refus legacy, un appel par lot. Tous les lots dépassent 3 000 ns ; la médiane AB vaut 0,846, la médiane BA 1,380. Le fait que le second bras paraisse souvent favorisé est compatible avec un effet d'ordre ou d'état chaud, mais ce reçu n'en isole pas la cause. Ce contrôle interdit de lire automatiquement un ratio favorable de petite strate comme un bénéfice algorithmique. P0 n'est toujours pas un clone de F : le contrôle révèle une sensibilité de mesure, pas une égalité obligatoire des temps.

## Résultat sur la paire q2 répétée

Chaque ligne suivante agrège les deux ordres de la même paire extrême, soit 8 192 appels/bras/passage. Les temps par appel sont des médianes descriptives en ns, **capture incluse**. AB/BA désignent la médiane des ratios appariés dans chaque ordre de passage ; Q1–Q3 porte sur les sept ratios. La médiane de différence ne se calcule pas en soustrayant les deux médianes affichées.

| P | L | F ns/appel | Dual ns/appel | Différence appariée ns/appel | Ratio médian dual/F [Q1–Q3] | AB | BA |
| ---: | ---: | ---: | ---: | ---: | --- | ---: | ---: |
| 0 | 1 | 227,5 | 250,5 | +21,3 | 1,094 [1,086–1,115] | 1,113 | 1,092 |
| 0 | 2 | 228,5 | 249,4 | +20,6 | 1,090 [1,076–1,100] | 1,095 | 1,076 |
| 1 | 1 | 229,5 | 325,8 | +96,9 | 1,428 [1,386–1,432] | 1,430 | 1,386 |
| 1 | 2 | 229,2 | 319,7 | +90,5 | 1,394 [1,382–1,402] | 1,395 | 1,384 |
| 401 | 1 | 229,7 | 321,3 | +89,9 | 1,395 [1,374–1,411] | 1,384 | 1,411 |
| 401 | 2 | 228,4 | 322,8 | +91,5 | 1,401 [1,375–1,426] | 1,387 | 1,426 |

Dans ces six groupes, dual est plus lent dans chacune des sept paires, y compris P0. La taille des lots et la stabilité du signe rendent ce surcoût local descriptif exploitable. Le contraste AB/BA reste visible et n'est pas effacé par un meilleur passage choisi. Ce résultat ne s'étend pas aux q2 sur n>2, notamment au support q2 tardif de rang 55.

## Ce que la proposition économise

Sur les cas frais, F essaie réellement `delta_legacy` candidats. Dans le bras dual, le nombre physique est `A+delta_P`, où A vaut `delta_legacy` si le repli F est effectivement appelé, et zéro sur un chemin certifié. Le transfert virtuel d'ordinal n'est donc jamais compté comme du travail F. Le tableau garde tous les budgets préparés et les refus :

| P | Cas principaux | Candidats F | Candidats dual A+delta_P | Cas avec moins / autant / plus de candidats |
| ---: | ---: | ---: | ---: | --- |
| 0 | 1 152 | 67 884 | 67 884 | 0 / 1 152 / 0 |
| 1 | 1 152 | 67 884 | 66 781 | 186 / 157 / 809 |
| 4 | 1 152 | 67 884 | 69 157 | 186 / 157 / 809 |
| 5 | 1 152 | 67 884 | 57 872 | 477 / 157 / 518 |
| 15 | 1 152 | 67 884 | 56 295 | 567 / 159 / 426 |
| 16 | 1 152 | 67 884 | 27 902 | 696 / 159 / 297 |
| 25 | 1 152 | 67 884 | 24 684 | 723 / 159 / 270 |
| 401 | 1 152 | 67 884 | 10 722 | 795 / 159 / 198 |

À P401, le rapport de candidats est exactement 1787/11314, soit environ 0,158. Cela étaye le mécanisme d'évitement de l'énumération F sur ce corpus. Un essai peut échouer avant matérialisation d'une forme ; distances, puissances, copies, certification et finalisation restent hors de cette unité. Ni cette réduction ni son classement des P ne sont un classement de latence industrielle.

Une sous-cohorte **exploratoire choisie après lecture** fournit un signal de temps favorable : n≥8, q de référence 4, P401, tous terminaux et tous L conservés. Elle comprend 156 cas, 144 groupes ; 44 groupes ont les quatorze lots au-dessus du seuil diagnostique. Parmi ces 44, dual est plus rapide dans les sept paires pour 41 groupes, plus lent dans les sept pour un groupe, et les signes sont mixtes pour deux groupes. Les médianes de ratio par groupe vont de 0,082 à 1,298 ; leur médiane descriptive vaut 0,365. Cette dernière n'est ni un ratio apparié de la sous-cohorte entière ni une estimation du gain moyen de la tour.

Les trois groupes non uniformément favorables restent dans l'export : n9/q4/P401/L138, L139 et L140. Tous les 44 groupes, leurs sept paires, différences, ratios, dispersions et AB/BA sont conservés avec les six groupes q2 dans [les données sélectionnées](cost/selected_groups.json.gz). Le script recalcule les 4 699 groupes depuis le brut, sans dupliquer un grand tableau. La sélection postérieure et le filtre sur les durées interdisent de traiter ces 44 groupes comme un échantillon sans biais. Ils montrent néanmoins où le coût de certification peut être compensé par des essais F évités, sans cacher les contre-exemples.

## Conséquence constructive et limite de cette campagne

Le microcoût justifie de poursuivre le raccord exact et de distinguer les appels F immédiatement acceptés des recherches longues. Il ne justifie pas d'introduire un seuil n≥8, de choisir rétroactivement le meilleur P, d'annoncer P0 gratuit, ou de projeter les répétitions chaudes sur une tour. L'autorité de budget persistant par ordre, l'intégration du wrapper, les caches et la distribution réelle des rangs et routes doivent rester explicites dans toute comparaison produit.

Le constructeur a déjà préparé séparément `build/v7_meb_dual_budget_cost_followup_plan/PROTOCOL.md` : les 384 ordres, P0/P401, L551, 64 répétitions et dix paires équilibrées. Son admission appartient à la contrelecture du plan ; cette note n'en demande ni le lancement ni une relance et n'en incorpore aucun résultat. La campagne v2 est close. Aucune extrapolation CLI Release, tour 8k/16k/32k, GPU, objectif 50k ou statut global d'exactitude n'est produite ici.

Rejeu depuis la racine, sans accès au dossier privé `build/` :

```bash
python3 morsehgp3D_v7/audits/receipts_meb_native_20260905/cost/review_cost.py
python3 -O morsehgp3D_v7/audits/receipts_meb_native_20260905/cost/review_cost.py
```
