# Difficultés actuelles du développeur — capacité, bootstrap et transport

Revue indépendante du 2 octobre 2026, limitée à la v11. Sources figées avant lecture au pin `a7cd34ee2a5edbefc6ad98d9854e56e4df278b2e` ; les reçus gardent leurs propres pins 9c/2e/a7. Aucun natif, build ou appel GCP par l'auditeur. Capsule unique, sans modification des notes actives ni des capsules précédentes.

Les échecs récemment rencontrés sont **matériels et de bootstrap**, avant tout test de l'algorithme. Le bootstrap manquant est maintenant traité par `tools1`. La source produit 2e/9c et les nouvelles forêts/centres restent à qualifier : la réussite d'une installation ne transfère aucune qualification du moteur.

| Capture | Source | Résultat constaté | Portée |
|---|---|---|---|
| [parallel1](raw/v11.20261002.parallel1/receipt.json) | 9c883b93f | Stockout G4 en zone b ; DONE74, génération inconnue | Aucun worker ni porte native |
| [parallel2](raw/v11.20261002.parallel2/receipt.json) | 9c883b93f | Même stockout ; DONE74, génération inconnue | Aucun worker ni porte native |
| [parallel3](raw/v11.20261002.parallel3/receipt.json) | 2e3af233f | VM zone c démarrée ; outils absents, DONE3 | Aucun configure/build/test natif/banc produit |
| [tools1](raw/v11.20261002.tools1/receipt.json) | a7cd34ee2 | Installation gardée réussie ; DONE0, arrêt ciblé certifié | Outillage seulement, `product_executed=false` |

## Causes concrètes et réparation acquise

Les stderr de démarrage [1](raw/v11.20261002.parallel1/host/logs/002_guarded_start.stderr) et [2](raw/v11.20261002.parallel2/host/logs/002_guarded_start.stderr) donnent `ZONE_RESOURCE_POOL_EXHAUSTED_WITH_DETAILS` : G4/RTX Pro 6000 indisponible en `us-central1-b`, avec suggestion de zone c. Ce n'est ni une panne de compilation ni une preuve de coût de l'algorithme. Les reçus initiaux restent `shutdown_uncertified`, génération inconnue ; les lectures externes séparées constatent trois fois, par tentative, l'ancienne génération `TERMINATED` et aucune opération start en attente. Elles ne sont pas un certificat d'arrêt d'une nouvelle génération, ni une campagne réussie. Le premier échec demeure conservé.

Dans l'[archive parallel3](raw/v11.20261002.parallel3/results.tar.gz), les huit configurations requises de la matrice refusent pour `g++` absent ; CMake et CTest sont également absents de l'inventaire. Le supplément ASan18 refuse avant configure. Clang, optionnel, est relevé absent. Les codes des trois commandes sont 1/1/2 ; le banc refuse sa qualification avec `ValueError`. Zéro porte sélectionnée/jouée et aucun rapport de banc natif : ne pas diagnostiquer un défaut algorithmique ou un dépassement mémoire depuis cet échec. La génération c `2026-10-02T13:45:31.434-07:00` est fermée/certifiée, groupes fermés sans résidu ni troncation.

L'[archive tools1](raw/v11.20261002.tools1/results/results.tar.gz) conserve apt update/install code0, puis l'inventaire après installation de **cmake, g++, make**. GCC est 11.4.0, CMake/CTest 3.22.1, Make 4.3 ; les cinq commandes `--version`, GNU time compris, reviennent avec code0. Le minimum CMake du [produit](sources_git/morsehgp3D_v11/CMakeLists.txt) est 3.20, donc ce seuil est satisfait. La commande prend 14,314 s, groupe fermé/résidu0/flux non tronqués. Génération c `2026-10-02T13:56:38.986-07:00`, arrêt ciblé certifié, cible observée `TERMINATED`. **Aucun produit n'a été exécuté dans tools1.**

Le [préparateur a7](sources_git/morsehgp3D_v11/tools/g4_prepare_host.py) vérifie l'identité exacte de la cible via metadata sans proxy/redirection, la garde invitée active, l'opt-in d'installation et l'inventaire final. Les commandes apt ont une intention persistée avant lancement et des délais 180/360 s ; le [plan](sources_git/morsehgp3D_v11/bench/plans/host_tools_g4.json) borne la commande à 720 s. Le préparateur n'annonce pas qu'il ferme lui-même les descendants d'un apt expiré : cette responsabilité est explicitement déléguée au worker gardé/à l'arrêt de cible. Le paquet a7 est haché et ses trois blobs préparateur/plan/worker comparés au Git figé ; les paquets complets sont conservés uniquement par leur hash dans cette petite capsule, pas recopiés ni présentés comme des archives autonomes.

**Conseil de reprise :** après le démarrage gardé, faire un préflight explicite de la chaîne nécessaire avant de lancer matrice et banc, particulièrement après changement d'image/cible/génération. La réparation est acquise pour cette VM ; rejouer ensuite les vraies portes et leurs profils sur une source exacte, en conservant l'identité de cette toolchain. Clang absent n'est pas à transformer en échec de la configuration obligatoire. Ne pas répéter un banc pour tenter de résoudre un refus d'outillage ou un stockout.

## Coût de paquet évitable

Le [dimensionnement du paquet tools1](tools_package_layout.json) est exact sur ses métadonnées tar : **112 175 708 octets compressés**, 142 872 059 décompressés, 3 603 fichiers. `morsehgp3D_v11/receipts/` compte **141 119 000 octets et 3 347 fichiers**, soit 98,77 % de la taille décompressée ; tout le reste compte 1 753 059 octets. Nos archives d'audit contribuent à ce volume. Une réparation d'outillage de 14,314 s embarque donc aussi les reçus historiques. Ce constat ne chiffre ni le temps d'upload propre à ces reçus ni un gain produit.

Conseil : produire depuis un Git propre un paquet ciblé contenant **tous** les blobs nécessaires — sources, dépendances, CMake, oracles, scripts de harnais/worker et entrées de preuve réellement utilisées — avec manifeste des blobs sélectionnés, empreintes/sourcepins et contrôle des dépendances. Exclure les reçus historiques sauf dépendance démontrée ; garder les preuves séparées. Cela n'autorise ni à construire un autre arbre ni à omettre une dépendance. Ne pas réécrire les anciennes captures de paquets complets pour appliquer ce conseil. Une sélection de fichiers exige son propre manifeste, pas l'allégation d'un paquet Git intégral identique.

## Corrections reconnues et travail en cours

Le problème d'intention perdue étudié en capsule17 est désormais traité dans les deux collecteurs du pin a7 : [profiles](sources_git/morsehgp3D_v11/bench/catalogue_profiles.py) et [parallel](sources_git/morsehgp3D_v11/bench/catalogue_parallel.py) sauvegardent `launch_intents` avant `measure`. L'intention porte argv, profil, paramètres et hashes des coordonnées/IDs. Son scope dit correctement qu'elle **ne prouve pas le spawn et n'a pas de PID**. Deux interruptions factices sans processus vérifient une intention persistée, zéro résultat et `complete=false`. Ne pas maintenir l'ancienne réserve comme absence d'identité avant lancement sur cette nouvelle source ; ne pas convertir une intention en lancement prouvé.

Cinq [messages ciblés du développeur](developer_excerpts.json), de 20:45 à 20:59 UTC, confirment le diagnostic g++ manquant, la préparation gardée et le travail parallèle sur forêts/verticales et comparaison exacte des centres. Ils distinguent eux-mêmes ces ajouts de leur future qualification native. Ce sont des informations de progression, pas des résultats de test. Le journal JSONL complet n'est pas copié : seuls ces textes, leurs offsets et hashes de lignes sont conservés ; les préfixes privés figés restent inchangés après lecture.

## Rejeux et fermeture

[review.py](review.py), normal et −O, recoupe les quatre états, les deux archives/manifeste, les fermetures, les codes, les absences de portes et les intentions factices. JSON [normal](checks/normal.json) et [−O](checks/opt.json) identiques. Le [selftest préparateur copié](sources_git/morsehgp3D_v11/tools/g4_prepare_host_test.py) donne **87 contrôles**, normal/−O identiques, entièrement mockés, avec `Popen` interdit ; zéro natif/cloud. Les commandes et leurs codes0 sont conservés dans [commands.json](commands.json). Aucun échec de rejeu n'est supprimé ; ces quatre rejeux passent dès leur première prise.

[sources_after.json](sources_after.json) recoupe 65 fichiers/paquets ou sources : copies, bruts, sources figées et hashes inchangés. OWN est passé à `cb5a69ef3` pendant le travail ; DEV reste a7, avec ses WIP hors scope conservés. Aucune future campagne n'est attendue pour fermer cette tranche. `LEDGER.json` inventorie tous les payloads, inventaires imbriqués compris ; `SHA256SUMS` inclut le ledger et exclut uniquement son propre fichier racine. GCP non utilisé par l'auditeur.
