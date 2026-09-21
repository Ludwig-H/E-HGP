# Pilote CPU v8 sur G4 SPOT — sessions du 21 septembre 2026

Ce chemin mesure le flux global de candidats q3/q4, **pas le catalogue, la
tour FULL, le GPU ni le contrat 50k/1 seconde**. Le constructeur v8 n'utilise
ni CUDA ni un exécutable v7. Les scripts ont été préparés sans commande GCP,
puis le responsable a exécuté les sessions R1/R2/R3 décrites ci-dessous.

## Entrées et fermeture

`cpu_probe_session_v8.py` est inerte sans `--execute`. Sa cible unique est
`devpod-gpu-exploration/us-central1-b/ehgp-v7-4fa0e0789a7d5bb06b787d35`.
Le nom historique de la VM n'est pas le nom du moteur exécuté.

Avant OS Login ou démarrage : contrôle explicite de `TERMINATED`, du projet,
du label, de `g4-standard-48`, de `SPOT`, `STOP`, maintenance `TERMINATE`,
redémarrage automatique désactivé et durée GCE exactement 3 600 secondes.
Le contrôleur passe ensuite uniquement par les gardes inchangés du dépôt :

- `start_and_verify.sh`, arrêt invité 30 minutes, handoff et marques privés ;
- relecture de la génération et des deux coupe-circuits avant transfert ;
- `stop_and_verify.sh --yes --expected-last-start-timestamp GENERATION` dans
  le `finally`, même après échec, interruption ou récupération impossible.

Une génération inconnue ou contradictoire n'est jamais devinée. Un arrêt non
certifié reste un échec bloquant ; aucune autre VM n'est arrêtée. La clé privée
de session n'est ni publiée ni transférée. Seule sa clé publique est inscrite
pour 70 minutes, conformément à la fenêtre acceptée par le garde existant.

Le contrôleur réutilise explicitement les fonctions de lecture, collecte de
processus et fermeture de `full_probe_session_v7.py`, dont le SHA256 est
vérifié avant chargement. Il **n'appelle pas** son `run_session`, sa validation
du paquet v7 ou son producteur FULL. Le worker réutilise de même les seules
primitives de garde et de processus du worker v7 épinglé. Les deux fichiers
anciens et les gardes restent inchangés ; cette réutilisation ne transfère
aucune qualification mathématique v7 à la v8.

## Paquet et travail invité

`cpu_probe_snapshot_v8.py` reçoit un manifeste plat fermé, ou explicitement
`--authority-capture` vers la capture de mutants tranche31 à 196 sources.
Cette seconde entrée contrôle `MANIFEST.json`, `COMPLETION.json`, la fermeture
des sources et les hashes des enregistrements avant d'en extraire les sources
v8. La première entrée peut être préparée par un autre lecteur qualifié ; le
paquet marque alors `closed_source_authority=false`, et l'autorité extérieure
doit être conservée séparément, sans prétendre que le constructeur l'a relue.

Le paquet contient les sources v8 sous leur vrai préfixe, le helper worker
historique explicitement épinglé, `data/session_plan.json` et les copies
`data/*.u16le` dont les hashes sont fournis. Il ne contient aucun ELF local,
build, lien symbolique, archive d'audit ou source déguisée sous un préfixe v7.
Les fichiers sont archivés en lecture seule. Les sources et données sont
recontrôlées lors de la création, avant compilation et en fermeture.

Le worker exige 48 vCPU disponibles, `g++`, GNU time et Boost système déjà
présents. Aucune installation, mise à jour, modification de garde ou reboot
n'est permis. Il compile strictement les 22 unités de bibliothèque, huit
compilateurs en parallèle, puis les deux programmes effectivement exécutés :

- `mhgp8_wspd_q34_gate --selftest` ;
- `mhgp8_wspd_q34_probe`.

Le fichier `q4_lidar_probe.cpp` reste une source obligatoire et épinglée : la
sonde globale inclut ses lecteurs et tables de compteurs, sans lancer son
benchmark par arête. Il n'est plus construit comme troisième exécutable en R2.

La gate92 est le 92e CTest, **pas la suite complète de 92 tests**. Son succès
précède toute grande sonde. La construction, les contrôles et les commandes
scientifiques partagent un budget utile de 900 secondes au maximum, également
borné par l'arrêt invité moins 300 secondes de fermeture. Ce budget de coût
arrête la session entière, jamais la recherche interne d'un algorithme.

Le plan est explicite et ordonné : le pilote choisi par le responsable commence
par scan0/1k avec W1 puis W48, avant les tailles 8k/16k/32k/50k. Les répétitions
de mêmes données/paramètres avec différents nombres de workers sont comparées
sur le payload et tous les compteurs géométriques ; seuls les deux pics de
capacités privées sont normalisés. Local28 reste le premier backend testé.
K5/10, s8/10/12 et scan100/200 restent des choix de campagnes distinctes, à
exécuter seulement dans le temps utile réellement restant.

Le plan ci-dessus est historique R1/R2, qui n'ont pas atteint les mesures.
Le [plan R3](gcp_plan_r3.json) diagnostique seulement1k/W1 puisW48,
2k/4k/8k/W48 àK5/s8/Local28, dans le même fichier source8k. Il a terminé
ses cinq commandes ; les tailles16k/32k/50k ne lui sont pas attribuées.

Chaque commande garde invocation, sortie brute, code et hashes. La compilation
enregistre aussi ses dépendances, y compris les headers système. Sources,
compilateur, binaires et dépendances sont relus en fermeture ; le contrôleur
vérifie les reçus et leurs logs téléchargés. Les binaires/builds distants ne
sont pas inclus dans l'archive de résultats, leurs hashes le sont.

Si le budget est atteint, toutes les commandes finies et l'échec de la commande
interrompue sont conservés : le résultat n'est ni « complet », ni un contrat
de tour atteint. Le chronométrage conserve les coûts de préparation et du flux
global ; un succès CPU sur une machine avec GPU n'est pas une exécution GPU.

## Contrôles locaux et limites

`selftest_cpu_probe_v8.py` : sept tests PASS en Python normal et `-O`.
Ils exercent inertie, cible/durée/SPOT, archives malformées, plan, génération,
invocation exacte de la gate, projection de comparaison multiworker et un
échec simulé après démarrage qui doit arrêter la génération exacte. Aucun
test ne lance un compilateur, SSH, une VM ou un appel GCP réel.

La première revue avait trouvé l'oubli de `--selftest` dans la commande gate :
corrigé et couvert avant gel. La contrelecture indépendante n'a pas trouvé de
défaut de fermeture ou de budget ; elle a renforcé le contrôle hôte du manifeste
de dépendances compilées. Ces tests de contrôle ne qualifient pas encore une
exécution distante ni ses performances.

Les quatre scripts sont gelés avec les hashes du reçu
`receipts/cpu_probe_v8_20260921/r2_selftest.json` pour R2. Le reçu initial
`cpu_probe_v8_selftest_20260921.json` et les quatre scripts exacts dans
`receipts/cpu_probe_v8_20260921/preflight_r1/` restent des preuves R1 distinctes.
Le paquet local temporaire
`/tmp/mhgp8-gcp-package.0WMckWMe/package` était un préflight antérieur au gel :
**ne pas le lancer**. Le responsable conserve séparément son paquet final,
son plan de campagne et l'autorisation exclusive du démarrage. GCP non utilisé
par le préparateur de ces fichiers.

## Premier essai distant R1 : échec de construction conservé

Le responsable a ensuite démarré la session gardée R1. GCC **11.4.0**, Ubuntu
22.04, a compilé et lié la gate92 puis la sonde globale avec tous les
avertissements stricts. La construction du troisième exécutable, pourtant
inutilisé par le plan, a échoué sur `q34_cover_probe.cpp:185` :
`record may be used uninitialized [-Werror=maybe-uninitialized]`.
Le `Record` est entièrement initialisé par agrégat à la ligne182 ; les arités
3/4 sont vérifiées et l'accès `i-1` est protégé par court-circuit. Aucun accès
indéfini n'a été établi à cette lecture, mais l'alerte n'est pas déclarée
définitivement fausse sans contre-exemple réduit du compilateur.

R1 n'a **pas exécuté la gate**, ni aucun benchmark : son succès de compilation
partiel ne qualifie donc aucune performance. Les scripts R1 exacts, leurs pins,
le diagnostic et les liens/hashes des reçus distants sont archivés. R2 retire
uniquement cette cible de compilation non utilisée ; aucun avertissement n'est
désactivé, aucun code v8 gelé ni helper historique n'est modifié. Le mode digest
du global n'appelle pas `Output::collect`, où l'alerte a été émise ; les sources
du collecteur restent incluses et épinglées. La gate92 doit encore être lancée
et réussir en R2 avant les grandes mesures. L'arrêt ciblé R1 et tout nouveau
démarrage restent exclusivement sous le contrôle du responsable, jamais de
l'agent qui prépare cette correction.

## R2 interrompue, R3 terminée et cible arrêtée

R2 a compilé les deux programmes utiles mais sa gate rationnelle a tourné
sans terminer. Elle a été interrompue, archivée et la génération arrêtée.
Trois égalités `boost::rational == 0` de l'oracle ont ensuite été remplacées
par `numerator() == 0`, équivalent exact évitant un problème des comparaisons
Boost anciennes sous C++20. Aucun moteur, avertissement, garde ou profil
mathématique n'a été modifié. Les nouvelles qualifications locales puis
la gate distante R3 passent ; les captures antérieures restent distinctes.

R3 :5 mesures achevées, source/dependencies stables, comparaison1k/W1-W48
du travail et des digests réussie. Temps1k/W1=3,849s ; W48=0,863s,
2k=8,470s,4k=59,274s,8k=614,744s. Ce résultat est un diagnostic négatif
de croissance et de répartition, pas une qualification de vitesse50k.
R1, R2 et R3 ont chacun leur arrêt ciblé certifié TERMINATED.
Voir les [reçus complets et limites](../morsehgp3D_v8/receipts/lidar_global_20260921/README.md).

Les lecteurs posthoc `read_cpu_probe_v8.py` et `read_cpu_probe_costs_v8.py`
contrôlent les captures closes, les données et les commandes exactes,
puis extraient croissance et occupation CPU. Leur dépendance au validateur
global, absent du paquet196 initial, est déclarée, archivée et épinglée
séparément : elle n'a jamais été exécutée implicitement sur la VM.
