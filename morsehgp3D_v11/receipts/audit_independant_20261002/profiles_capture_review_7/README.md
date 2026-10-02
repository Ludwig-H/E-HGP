# Capture profiles1 — recoupe indépendante du 2 octobre 2026

**Matrice conforme, banc non conforme.** La campagne source `9df77494732b03ddf11dbcf1dcb11d96bef54a3b` a exécuté 1,002 instances de portes puis 33 tentatives de catalogue : 15 succès K5, 18 délais de processus30s, 3 omissions K10 justifiées. Les cinq comparaisons K5 complètes concordent entre B18/B21/B24, pour l'empreinte sémantique et les neuf compteurs géométriques. Aucun K10 ni32k/K5 n'a terminé. Il n'y a ni FULL, ni exécution GPU, ni contrat100ms acquis.

## Provenance et fermeture

Les pièces développeur de `profiles1` étaient closes localement et non versionnées à l'ouverture de cette tranche. [Capture initiale](sources_before.json), [recoupe finale](sources_after.json) : les 27 fichiers copiés restent identiques. Le checkout LIVE a avancé de `9df774947` à `d77e4b77c`, sans changement de ces pièces; aucune qualification de ce dernier commit n'est transférée.

- [Le paquet source](source_package_verified.json), 5,976,240 octets, SHA`1fe9b9691ee92f761250166fc309a5febe21ae6d3683d9a501c85ab6b049530b`, est identique octet pour octet à un `git archive` indépendant du commit exécuté, périmètre `morsehgp3D_v11` et worker. L'archive source volumineuse n'est pas dupliquée dans ce reçu compact; le résultat de comparaison et sa commande exacte sont conservés.
- L'archive originale `results.tar.gz` est copiée intégralement : 249,968 octets compressés, 2,376,336 octets développés, 127 membres. Les 106 entrées du [manifeste original](excerpts/results/MANIFEST.sha256) sont toutes rehachées; aucun fichier de résultats n'est omis ou hors manifeste. Rapport de matrice, rapport de banc et plans correspondent exactement aux versions archivées et au paquet publié.
- Le [brut local exact](original_local/receipt.json) correspond au reçu compact par hash et par champs communs. Génération unique : `2026-10-02T06:58:37.362-07:00`; arrêt enregistré `07:20:11.517-07:00`, même génération, VM `TERMINATED`, arrêt ciblé certifié. Garde invitée et départ certifiés; clé privée supprimée, clé OS Login retirée, réserve libérée. Aucune nouvelle requête GCP par l'auditeur.
- Matrice : code0, mur130.594s, groupe fermé avant début du banc. Banc : code1, mur956.475s, groupe fermé. Les deux méta-reçus portent `group_closed=1`, `residual_group_killed=0`, aucun flux tronqué. La session entière conserve `failed_remote`, sans interruption ni overflow non résolu.

Un fichier optionnel `package/SHA256SUMS` a disparu entre inventaire et lecture initiale; cet échec de capture est conservé dans [le constat](capture_initial_optional_manifest_missing.json). Le manifeste **des résultats** est intact, le paquet disponible a été vérifié directement contre Git : aucun défaut de qualification n'est déduit de ce retrait.

## Portes effectivement jouées

| Configuration | Bits explicites | Portes passées/sélectionnées |
| --- | ---: | ---: |
| GCC Release |18|227/227|
| GCC ASan/UBSan |24|152/152|
| GCC TSan |21|152/152|
| GCC bits21 |21|152/152|
| GCC bits24 |24|152/152|
| Poison |21|153/153|
| Style |21|2/2|
| Campagnes de mutants |18, avec overrides ci-dessous|12/12|

Clang est absent et facultatif; aucune porte Clang acquise. Les 1,002 sont des **instances de portes**, avec répétitions normal/−O et configurations, pas 1,002 scénarios indépendants. Les inventaires sélectionnés et exécutés JUnit concordent; zéro porte échouée/non exécutée.

Le journal complet de mutants contient 116 noms uniques : 111 `TUE/code`, 3 `TUE/ligne`, 2 refus de construction explicitement attendus par le manifeste core, zéro signal/délai/INVALIDE/SURVIVANT. Modules : core78, num13, cloud16, catalogue9. Les [deux refus attendus](mutant_expected_compile_refusals.json) sont vérifiés par nom et déclaration, pas assimilés à des résultats géométriques. Quatre mutants num reconfigurent leur copie en21/24 : tag q3 et termes linéaires à21, signe q4 et coquille `side` à24. Le profil global de la configuration mutants ne doit donc pas leur être attribué uniformément.

## Catalogue : résultats réels

Chaque profil exécute11 unités : cinq succès K5, six délais, une omission32k/K10 après32k/K5. Les 18 délais concernent les quinze K10 réellement tentés et les trois32k/K5. Ils ne fournissent pas de durée API complète; le délai30s concerne le processus avec lecture/préparation/sérialisation. Les sept autres comparaisons sont `incomplete`, pas une égalité démontrée par vacuité.

Temps API en secondes, une répétition par profil :

| Entrée entière, sites | B18 | B21 | B24 |
| --- | ---: | ---: | ---: |
| uniforme8k |7.420032528|8.065767137|8.090137013|
| uniforme16k |15.618847332|16.920469280|17.076106671|
| LiDAR08/000100,35,551 |19.440401683|20.551074808|20.508314221|
| LiDAR08/000000,39,885 |24.523824026|25.846926848|25.914578157|
| LiDAR08/000200,45,845 |22.673631723|24.051390726|24.094295286|

Mêmes fichiers u18 et PointId pour les trois profils, même manifeste et aucun sous-échantillonnage des sous-nuages déclarés. Il s'agit de trois trames d'une seule séquence, pas de plusieurs séquences. Ce banc compare l'arithmétique/encodage compilé sur une géométrie commune, pas trois précisions physiques.

Les sorties brutes diffèrent par largeur de limbes mais les empreintes sémantiques K5 sont identiques. Les trois binaires et caches sont distincts et liés à la qualification; le pilote les rehache avant mesure. L'auditeur rejuge leurs traces/provenances, sans disposer des exécutables natifs dans cette archive. Les sorties canoniques sont supprimées après mesure : leurs hashes bruts/sémantiques restent des résultats déclarés du pilote qualifié, pas des payloads rehachables ici.

Les valeurs exactes de B, niveaux, incidences, neuf compteurs, hashes, temps processus/décodage et réservations figurent dans [la recoupe autonome](review_normal.stdout). API = deux passes, tri et sorties en mémoire; processus = lecture/préparation/destruction/sérialisation comprises; décodage Python séparé, SHA brut préalable hors `semantic_wall_seconds`. Pics `Buffer` pendant catalogue, Cloud vivant compris, après réinitialisation du pic : ni pic global du processus, ni RSS, ni mémoire Python. La commande entière mesure aussi son mur956.475s et son maximum RSS343,460KiB, coûts distincts des temps API de chaque unité.

## Comparaison B18 au leaf16 e6

Les cinq succès B18 communs au [rapport antérieur figé](comparison/catalogue3_e6fe34cb0_leaf16.json) conservent exactement hash/taille canoniques, B/niveaux/incidences, neuf compteurs géométriques, pic et réservations `Buffer`. Cela constate l'absence de variation de ces sorties/travaux sur ces cas, sans preuve globale sur les nouveaux chemins arithmétiques.

8k/K5 conserve SHA`2671f84acd61597300af06e7f164b0c8cd552b726bf7519711c8772d3febf74a`, 109,592,066 octets, 597,998 boules, 597,987 niveaux, 2,895,136 incidences, pic133,416,208 octets. Nouveau temps7.420032528s; anciens8.239569411/8.240274530/8.210329939s. Le nouveau binaireB18 SHA`eeea7b15f82bdb35dfa78a9bca2c218a05f179a94b7e00d91890f1aaa08368c9` diffère de celui du leaf16 précédent. Ces temps sont des observations séquentielles non appariées; aucune causalité ni répétabilité du gain n'est qualifiée.

## Lectures closes

[review.py](review.py) utilise uniquement les copies locales, y compris le brut de session, et rejuge archive/manifeste, profils/portes/commandes, calendrier, comparaisons et fermeture. Lecture normale et−O : code0, résultats byte-identiques, **campagne explicitement échouée**. Le nouveau self-test développeur est relu et rejoué sur copies : cinq témoins JSON, 24 corruptions refusées, normal/−O byte-identiques et conformes aux hashes/résultats annoncés. Ces contrôles du lecteur ne s'ajoutent pas aux 1,002 portes G4.

[Commandes et codes](review_runs.json), [self-test recoupé](selftest_verified.json). Aucun build, test natif ou appel cloud nouveau; aucun reçu antérieur modifié. Les sources et captures de ce reçu sont figées; aucune campagne suivante n'est importée.
