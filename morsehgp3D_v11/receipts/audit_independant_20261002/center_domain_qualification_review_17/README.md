# CenterRegion / FullDomain — qualification native et banc interrompu

Audit indépendant du 2 octobre 2026, limité à la v11 et à la campagne `region1`. Source exécutée : `7f1922c7743d8682e2665a491b01d32e8f2d546c`. Capture avant lecture à 19:59 UTC, complétée puis recoupée jusqu'à 20:19 UTC. OWN reste `952df06b7` ; DEV est passé à `9c883b93f` pendant la revue. Aucune exécution native, compilation ou opération GCP par l'auditeur. Les seuls rejeux sont les lecteurs Python copiés et une interruption factice du wrapper, sans processus lancé.

Les primitives CenterRegion/J2 et le composant FullDomain disposent de preuves natives conformes sur le pin 7f. Le banc catalogue est une campagne échouée, partiellement persistée, qui ne qualifie ni ses 36 unités prévues ni le contrat FULL. Le port parallèle/sched/cells publié ensuite en 9c n'est pas qualifié par ces pièces.

## Provenance et qualification effectivement jouée

Le [paquet](raw/package/package.tar.gz) contient exactement les 2 617 blobs Git du pin 7f annoncés dans [l'inventaire](package_git_inventory.json), soit 48 774 806 octets décompressés. SHA256 du paquet : `80166c4256152479e0dbcc22a61c1c06c9011391d859b3ca237ba73079e742e9`. L'[archive de résultats](raw/morsehgp3D_v11/receipts/center_region_20261002/region1/results.tar.gz) compte 122 fichiers, 3 360 662 octets décompressés et 121 entrées de manifeste couvrant tous les autres fichiers. SHA256 : `901123aa290a39cdcc6c7a0a56ff118a54620eda85a9db1c426e8ce6e1e002a9`. Les inventaires, les hashes, le plan, le brut original et le `DONE=3` sont conservés et recoupés.

La [matrice principale](raw/morsehgp3D_v11/receipts/center_region_20261002/region1/matrix.json) passe **1 398/1 398 portes** : GCC Release/B18 292 ; ASan-UBSan/B24 217 ; TSan/B21 217 ; B21 217 ; B24 217 ; poison/B21 218 ; mutants 18 ; style 2. Clang est absent, pas qualifié. C'est une extension de 132 portes au lot MEB précédent, correspondant à 22 nouvelles portes sur six configurations natives. Le [supplément ASan18](raw/morsehgp3D_v11/receipts/center_region_20261002/region1/asan18.json), modules num/index/tower, passe **73/73**, soit 18 nouvelles portes ; les portes catalogue n'appartiennent pas à ce supplément.

Les 154 identités de mutants sont recoupées dans les sorties complètes : **149 verdicts code, 3 ligne, 2 construction**, aucun signal ni délai. Ces 154 mutations ne sont pas 154 CTests supplémentaires. Les nouveaux oracles CenterRegion ont 371 cas, 865 contrôles, 55 contacts, 18 dégénérescences, 53 refus et 123 permutations par profil ; normal et −O concordent. Le modèle Python annonce zéro appel natif. Les sélections sont liées aux empreintes du [contrat](raw/morsehgp3D_v11/receipts/center_region_20261002/contract.json), les binaires du banc aux provenances de construction et aux caches des profils 18/21/24.

FullDomain qualifie ici un domaine possédé, ses contextes/identités/recherches, supports globaux, refus, capacité, concurrence et permutations. Cela ne constitue pas la construction de l'arbre de fusion FULL, de ses verticales ou d'une hiérarchie finale sur les points. Les contrôles bornés ne fournissent pas une preuve de coût global sous-quadratique.

## Interruption et mesures conservées

Le [reçu](raw/morsehgp3D_v11/receipts/center_region_20261002/region1/receipt.json) conserve `failed_remote`, le premier échec et l'arrêt ciblé certifié de la génération `2026-10-02T12:28:25.841-07:00`, observée `TERMINATED`. Les trois commandes ont des groupes fermés. Matrice : 208,508 s/code0 ; supplément : 15,217 s/code0 ; banc : **820,003 s/code124**, budget 820 s, `residual_group_killed=1`, sans troncation. Ce dernier résidu tué ne certifie pas l'isolation préalable des mesures ; l'arrêt de la cible et l'isolation des chronos sont deux propriétés distinctes.

Le [rapport catalogue](raw/morsehgp3D_v11/receipts/center_region_20261002/region1/profiles.json) est `complete=false`, `full_schedule_completed=false`. Sur 36 unités demandées : **29 tentatives persistées = 14 succès K5 + 15 délais**, **2 omissions K10 causales** après délai K5 en uniform32k/B21/B24, et **5 unités sans résultat persistant** : ng02/B18/K10, ng02/B21/K5/K10, ng02/B24/K5/K10. Leur statut de lancement est inconnu. Aucun succès K10 n'est conservé. Quatre comparaisons à trois profils sont complètes et égales : uniform8k, uniform16k, ng01 et ng00.

Les six entrées partagent les coordonnées physiques u18 et les IDs, avec poids de site unitaires ; les trois nuages LiDAR sans sol viennent d'une seule séquence. Ce banc CPU catalogue ne mesure ni FULL, GPU, segmentation, trame brute avec sol ou tour K1..K5. Les coordonnées/IDs téléversés et les gros résultats canoniques ne sont pas présents dans cette capsule : leurs hashes déclarés sont liés et comparés, pas recalculés depuis ces données absentes.

Treize succès communs avec la baseline `q4levels1` source `ffc2ff95f` gardent les mêmes tailles/hashes canoniques, hashes sémantiques, nombres B/I/L et comptes qmin. Le 14e succès, uniform32k/B18/K5, est nouveau vis-à-vis d'un délai ancien : aucune sortie ancienne ne permet de comparer son hash. Les valeurs ci-dessous sont descriptives, avec une seule répétition et des tests de filtrage différents ; elles ne prouvent aucun gain stable.

| Cas K5 | Profil | API ffc (s) | API 7f (s) | Processus 7f (s) | Décodage Python en plus (s) |
|---|---:|---:|---:|---:|---:|
| ng01, 35 551 sites | 21 | 19,777 | 17,374 | 17,642 | 8,408 |
| ng01, 35 551 sites | 24 | 19,673 | 17,509 | 17,788 | 8,460 |
| ng00, 39 885 sites | 21 | 24,962 | 21,734 | 22,038 | 9,847 |
| ng00, 39 885 sites | 24 | 24,794 | 21,996 | 22,324 | 9,960 |

L'API paye deux passes de génération, le tri et la mémoire de sortie ; le processus ajoute lecture/sérialisation. Les 14 succès totalisent 216,834 s de processus et **131,945 s de décodage Python distinct**, les 15 délais persistés 450,525 s de processus. Ne pas dimensionner le budget global uniquement à partir de 36×30 s natifs : le décodage et le travail interrompu restant sont aussi payés. La borne de 8 Gio est celle des réservations Buffer natives, pas une borne du RSS ou du décodeur Python. Voir les valeurs exactes et les 13 comparaisons dans [normal.json](checks/normal.json).

## P2 utile : sauvegarder l'intention avant le lancement

Dans [catalogue_profiles.py 7f](sources_git/morsehgp3D_v11/bench/catalogue_profiles.py), `subprocess.run` ligne 120 précède le checkpoint ligne 141. La [source publiée 9c](reader_delta/catalogue_profiles_9c.py.source.txt) conserve cette disposition, lignes 125 et 146. La [sonde factice](review.py) interrompt le wrapper avant son retour : le rapport initial reste sans ligne de tentative ni identité de prochaine unité. Aucun processus natif n'a été lancé par cette sonde. Elle établit le trou de sauvegarde, et non le lancement des cinq unités absentes de `region1`.

Conseil : persister l'intention, argv, identité/hash d'entrée et profil **avant** le spawn ; enregistrer PID/`started` après `Popen` si disponibles, puis sauvegarder l'interruption dans une sortie contrôlée/`finally`. Une marque `running` avant spawn prouve seulement l'intention. Conserver le premier échec et distinguer `intended`, `started`, résultat et absence de résultat. Aucune réserve n'est transférée au nouveau collecteur parallèle sans sa propre lecture.

Le lecteur initial et le lecteur LIVE final gardent correctement la campagne en échec. Le [delta du lecteur](reader_delta/diff.patch) renforce déjà la présence des omissions anciennes, les durées/codes de délai et l'absence de troncation ; ces corrections sont reconnues. Le reçu interrompu est accepté comme cohérent, jamais vert. Le code0 du lecteur signifie cette cohérence, pas la réussite du banc.

## Rejeux autonomes et fermeture

`python3 -B review.py --output checks/normal.json` et la variante `-O` produisent des JSON identiques et refusent sept corruptions ciblées : partiel marqué vert, calendrier interrompu marqué complet, doublon, omission sans cause, compteur invalide, binaire non qualifié, sélection de portes non liée. Le [rejeu final du lecteur](reader_delta/replay.py), normal/−O identiques, refuse en plus une omission ancienne perdue, un délai/code0 et des flux tronqués. Il adapte seulement les chemins vers les copies ; il ne modifie aucune pièce du développeur.

Le [préflight en échec](checks_normal.log) est conservé avec son [script initial](review_initial.py.source.txt) : l'auditeur avait supposé 14 succès communs avec ffc, alors qu'ils sont 13. Ce défaut d'hypothèse d'audit est corrigé et ne constitue pas un échec produit. [commands.json](commands.json) distingue les trois exécutions. [sources_after.json](sources_after.json) vérifie les copies inchangées, les sources Git exactes, les reçus stables et le drift LIVE ; la seule évolution du lecteur est capturée/rejouée séparément dans `reader_delta/`.

`LEDGER.json` inventorie tous les payloads, y compris les inventaires imbriqués ; `SHA256SUMS` inclut le ledger et exclut uniquement son propre fichier racine. Les autres capsules et les notes actives restent intactes. GCP non utilisé par l'auditeur.
