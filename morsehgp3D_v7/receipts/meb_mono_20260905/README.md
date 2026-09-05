# Paire mono C/D du noyau MEB — 5 septembre 2026

Une paire locale achevée observe **225,747536299 s pour C et 172,674571062 s pour D**, soit une baisse du temps processus de **23,51 %**. Les dix digests chaînés, toutes les cardinalités, tous les compteurs généraux et silent, ainsi que les caps, sont identiques. C'est une observation sur un hôte partagé et une seule paire, **pas un gain statistique, un SLO ni une certification globale d'exactitude**. `public_status=not_claimed` et `normalized_horizontal_h0_candidate` sont conservés.

La paire s'est déroulée du **00:06:26,930933 au 00:13:05,495688 UTC**, C puis D : uniforme, n=8000, coordonnées dans [0,65535], `coord=65536`, seed=3, WSPD s=8, tour K1..10 (`smax=11`), CSR, digest inclus, aucune archive. Elle demande le mono strict : CPU logique 6, `threads=1`, `fold-inflight=1`, `fold-join=1`. Le runner vérifie ces options et les compteurs d'ouvriers ; l'absence réelle de création de threads auxiliaires relève des **portes produit dédiées**, pas d'une inférence depuis le chrono. Limites : 600 s par processus, RLIMIT_AS=26 GiB d'espace virtuel (pas un plafond RSS physique), proxy partiel de payload=16 GiB et caps silent explicites.

Hôte déclaré partagé : AMD EPYC 7763, 8 CPU logiques visibles, noyau `6.8.0-1052-azure`. HEAD/porcelain avant/après, allowlist matérielle et charge sont conservés dans les métadonnées. Aucun dump d'environnement ; les trois variables d'injection LD doivent être vides. Les 323 portes lourdes lancées ensuite appartiennent à une campagne séparée et ne sont pas incluses dans ces chronos.

| Coût observé | C | D |
| --- | ---: | ---: |
| Processus, digest compris | 225,747536299 s | 172,674571062 s |
| Pipeline | 225,7199 s | 172,6230 s |
| Completion silent | 116,615421 s | 65,973963 s |
| Digest cumulé | 3,5972 s | 3,5914 s |
| RSS max externe | 2 302 712 KiB | 2 301 540 KiB |

La baisse observée du coût silent est de 43,43 %, sans réduction des compteurs : **802 125 328 supports MEB**, **581 904 257 visites de nœuds**, **1 270 848 pas/ajouts**, chaîne maximale 18, 4 384 229 événements et 26 434 998 facettes cumulées. Les valeurs par ordre et tous les chronos d'étages restent disponibles dans [pair/runs.json](pair/runs.json). Aucun résultat à 50k points, 1 s, 100 ms ou plusieurs dizaines de millions de points n'est déduit de cette paire.

## Construction et portée des preuves

C est le CLI Release C `25c9bf8e…`, lié à son reçu historique et à ses sources C, jamais au worktree D. D est le CLI fraîchement construit `127c5f92…` : build isolé achevé de 00:05:08,471750 à 00:05:33,926621 UTC, commande limitée à `mhgp7`, Release/CUDA OFF, source/helpers/C stables. Le reçu vérifie le chemin exact `build_dir/mhgp7`, son SHA/taille, le cache, la base de compilation du même build et les flags sans TESTING/PROFILE. D est identique **octet pour octet** au CLI séparé des 32 portes privées ; ce constat n'est pas une nouvelle exécution de ces portes. Les hashes complets et commandes sont dans [build_D/build_D.json](build_D/build_D.json) et [pair/metadata.json](pair/metadata.json). Cette liaison enregistrée n'est pas une attestation hermétique.

[reviews/build_review.json](reviews/build_review.json) conserve 206 vérifications de lecture seule du build. [reviews/pair_review.json](reviews/pair_review.json) conserve le rejeu des sorties brutes par les helpers épinglés : les deux classifications, tous les champs extraits et la décision terminale concordent. Aucun moteur n'a été relancé pour ces revues ni pour l'export. Refus/censure/échec restent des statuts distincts dans le protocole ; une paire incomplète ne peut publier égalité, ratio ou conclusion SLO. La présente paire contient effectivement deux succès, aucun refus et aucune censure.

## Conservation et vérification

`protocol/` conserve les scripts consommés et leurs tests en **snapshots texte inertes**. Leurs chemins relatifs restent ceux des originaux ; ils ne sont pas rendus exécutables depuis ce dossier public. Les reçus de préparation ne sont ni des builds réels ni des mesures. `build_record.provisional.json` reste un brouillon historique ; seul `build_D.json`, après validation terminale, est le reçu accepté.

Les manifestes `SOURCE_HASHES.json` et `SOURCE_*_PREPARATION_MANIFEST.json` sont attribués aux dossiers d'origine, pas aux noms renommés de cette projection. [provenance.json](provenance.json) donne chaque chemin source/public, les deux hashes, les tailles et la transformation éventuelle. **Les 50 fichiers portés sont ici tous identiques octet pour octet ; aucun LF ajouté et aucune sortie brute modifiée.** Le manifeste public couvre le contenu effectivement livré. Aucun binaire, header de dépendance externe, objet ou archive n'est porté. Aucun staging/commit ou GCP n'a été effectué par l'agent d'export.

Depuis ce dossier : `sha256sum --check SHA256SUMS`. Depuis la racine du dépôt : `sha256sum --check morsehgp3D_v7/receipts/meb_mono_20260905/SHA256SUMS.root`. Les deux listes couvrent le même contenu et `manifest.json`, avec des chemins adaptés ; elles s'excluent elles-mêmes et l'une l'autre.
