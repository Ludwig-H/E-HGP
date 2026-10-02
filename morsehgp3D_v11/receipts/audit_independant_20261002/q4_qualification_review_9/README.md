# Qualification q4levels1 : recoupe indépendante — 2 octobre 2026

La campagne close exécute **ffc2ff95f0ae7296bdc522df81df34c58c3fdf47** ; publication des reçus en `8671d6ac3`, retrait de l'ancien plan en `d0dc9cd8b`. Ces commits de publication ne deviennent pas la source native exécutée. Le catalogue CPU est qualifié dans les configurations jouées ; le banc conserve son échec. Aucun FULL, GPU, contrat de 100 ms ni gain statistique stable n'est acquis.

## Provenance et fermeture

Les [34 pièces copiées avant contrôle](sources_before.json) sont restées [identiques après contrôle](sources_after.json), sur un checkout développeur propre à d0dc9cd8b. Cette capsule ne lance aucune compilation, aucun produit natif et aucun cloud ; seuls des lecteurs de reçus et leurs fixtures Python sont exécutés.

Le [paquet original](raw/package/package.tar.gz), 7 379 208 octets, hash `041b06127bb715f561a69ec3be9dc80659e7e90d0e1d88db9da84bee0f120640`, correspond exactement aux blobs de `git archive ffc2ff95f morsehgp3D_v11 gcp-migration/v11_worker.sh` : 1 800 fichiers, 19 361 587 octets de contenu. Le [manifeste comparatif](package_git_manifest.json) conserve chaque nom, taille et hash ; la sonde rejoue cette comparaison depuis le paquet copié.

L'[archive unique](copies/morsehgp3D_v11/receipts/catalogue_q4_20261002/q4levels1/results.tar.gz) contient 122 fichiers, 2 461 752 octets de contenu ; ses 121 entrées `MANIFEST.sha256` sont toutes vérifiées, hors manifeste lui-même. Hash gzip `5c89df8c66218c40a6b5b141bb6251d962d8c2f50e5a96c68a956c0af5cebe7a`, taille 263 356 octets. Les copies matrix/asan18/profiles égalent celles de l'archive. Les hashes du plan JSON, du plan worker et du manifeste d'entrée sont raccordés au [reçu original](raw/q4levels1_receipt.json) et au [reçu compact](copies/morsehgp3D_v11/receipts/catalogue_q4_20261002/q4levels1/receipt.json).

Même génération start/closing/observed_after : `2026-10-02T08:27:52.537-07:00`. Arrêt observé `08:49:27.326-07:00`, VM `TERMINATED`, arrêt ciblé certifié, clé privée supprimée, clé OS Login retirée et réserve libérée. Trois groupes de commandes sont clos sans chevauchement ni flux tronqué : matrice 134,018 s/code 0 ; complément 6,038 s/code 0 ; banc 946,957 s/code 1. Le worker retourne 1 ; la session reste `failed_remote`.

## Portes réellement jouées

La [matrice principale](copies/morsehgp3D_v11/receipts/catalogue_q4_20261002/q4levels1/matrix.json) passe **1 014/1 014 sélections**, soit Release u18 229 ; ASan/UBSan u24, TSan u21, profil21 et profil24 154 chacun ; poison u21 155 ; style 2 ; mutants 12. Ce sont des sélections dans plusieurs configurations, pas 1 014 portes distinctes. Clang est absent.

Le [complément ASan/UBSan u18](copies/morsehgp3D_v11/receipts/catalogue_q4_20261002/q4levels1/asan18.json) passe séparément **14/14 = 12 portes num + 2 style**. Il ne s'ajoute pas au compteur 1 014 de la matrice principale. Parmi ses preuves : candidate 831 contrôles ; power_paths 207 ; Fraction 11 838 contrôles dans chacun des modes normal/−O (528 géométries, 50 dégénérescences, 160 contrôles entiers, hash d'entrée identique).

Les caches conservés et hachés confirment B18 pour Release et mutants, B24 pour ASan/UBSan, B21 pour TSan et poison, et les bits21/24 explicites. Les hashes enregistrés des trois binaires de banc, de leurs caches et de leur provenance correspondent aux profils invoqués ; les exécutables eux-mêmes ne sont pas archivés et ne sont donc pas rehachés après arrêt.

Les manifestes du paquet sont recoupés aux 119 verdicts du [LastTest.log complet](mutants_LastTest_review.log) : core 78, num 16, cloud 16, catalogue 9. Causes : **114 code, 3 ligne, 2 construction attendue dans core**, zéro signal ou délai déclaré comme détection. Le [JUnit](mutants_junit_review.xml) établit les 12 portes réussies ; son system-out core est tronqué à 1 024 octets par CTest, tandis que LastTest.log conserve les 78 IDs et causes core. Ces preuves de détection ne deviennent pas de nouveaux oracles mathématiques universels.

## Banc et comparaison à 9df

Le [rapport profils](copies/morsehgp3D_v11/receipts/catalogue_q4_20261002/q4levels1/profiles.json) garde 33 tentatives : 15 succès K5, 18 délais natifs à 30 s, trois K10/32k omis après échec K5 du même profil. Tous les K10 joués expirent. Les six entrées sont entières, sans sous-échantillonnage supplémentaire, sur les mêmes coordonnées quantifiées u18 et IDs que profiles1 ; les trois LiDAR sans sol viennent de la seule séquence 08. B18/21/24 changent l'arithmétique, pas la précision physique de ces entrées.

Durées exactes de l'API catalogue CPU mono, deux passes, tri et sortie mémoire inclus ; lecture/Cloud, segmentation, sérialisation et décodage Python exclus. Une répétition par entrée/profil. `peak_reserved_bytes` décrit Buffer, pas le RSS ni l'ensemble des allocations.

| Entrée entière K5 | Sites | u18 API (s) | u21 API (s) | u24 API (s) | Candidats q4/passe | Niveaux q4/passe |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| uniform_u18_n8000 | 8000 | 7.180214718 | 7.742866861 | 7.818431501 | 15087761 | 122218 |
| uniform_u18_n16000 | 16000 | 15.103725832 | 16.344706419 | 16.404495929 | 31354208 | 253794 |
| lidar_ng01 | 35551 | 18.457225695 | 19.776976940 | 19.672991151 | 41963267 | 121303 |
| lidar_ng00 | 39885 | 23.380178364 | 24.962327081 | 24.794177287 | 52878237 | 158494 |
| lidar_ng02 | 45845 | 21.567987075 | 23.101086066 | 23.175552738 | 48091032 | 143105 |

Uniforme 32k échoue aux trois profils. Les [15 comparaisons à la baseline profiles1/9df](review_normal.stdout) confirment les hashes bruts et sémantiques enregistrés, tailles canoniques, neuf compteurs géométriques, boules, niveaux, incidences, pics Buffer/Cloud et réservations finales. Les gros payloads canoniques ont été supprimés à la source : ce sont des hashes enregistrés comparés, pas une nouvelle lecture octet pour octet. Les cinq cas réussis ont les mêmes signatures sémantiques et comptes aux trois profils.

Les rapports baseline/nouveau sont compris entre 1.033399810 et 1.053267810 : sessions différentes et une répétition par cas ne démontrent pas un gain stable. Les candidats et niveaux q4 ci-dessus sont par passe et payés deux fois. Sur LiDAR, environ 99,7 % des niveaux candidats ne sont plus matérialisés ; cela ne constitue pas un gain temporel de même proportion ni une borne globale.

## Rejeux et limites

[Six commandes finales](review_runs.json) passent avec sorties normal/−O identiques : [lecteur développeur](reader_normal.stdout), [selftest 12 témoins/41 corruptions](reader_selftest_normal.stdout), [contrôle indépendant](review_normal.stdout). Le [wrapper de transport](run_reader.py) redirige uniquement les deux chemins absolus des reçus originaux vers leurs copies locales ; il conserve les fonctions de jugement du lecteur. La [sonde indépendante](review.py) rejoue hashes, inventaires, attribution des portes/mutants, fermeture et comparaison de baseline.

Deux erreurs initiales de notre juge sont conservées, sans défaut produit induit : [première attente erronée](review_pre_junit_assumption.py) de JUnit complet, puis [seconde attente erronée](review_pre_cause_assumption.py) limitée aux causes code/construction et omettant la cause valide ligne. Leurs commandes et sorties échouées restent dans cette capsule ; le juge final utilise le journal complet et distingue les trois causes.

`LEDGER.json` et `SHA256SUMS` ferment cette capsule. Aucun reçu antérieur ni note active n'a été modifié.
