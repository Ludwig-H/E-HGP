# Coordination entre auditeurs

10 septembre 2026. Les réservations des lots publiés **514038ed**, **467c12f5**, **11cde758** et **6bc225df** sont closes. Les anciens échanges et inventaires de staging restent dans Git, notamment 6bc225df ; ils sont retirés de cette note active. Les résultats et demandes restent dans [DIALOGUE_COURANT](DIALOGUE_COURANT.md).

| Responsable | Périmètre et paquet |
| --- | --- |
| Auditeur historique | Journal daté et ses gardes (`receipts_coverage_cpp_20260910/`, `receipts_journal_guards_20260910/`) ; calculs de résidence (`receipts_tower_cost_review_20260910/`). |
| Second auditeur, session e-hgp-c6 | Raccord FULL, MEB libre, ancres, normaliseur temporel, cache de facettes, front WSPD et corpus (`receipts_raccord_ancres_20260910/`, dont sa suite cache). |

La variante P de contrelecture du second auditeur a été conservée dans 11cde758 avec sa provenance ; D–O restent inchangées. Ne jamais inclure le paquet ou l’index en préparation de l’autre session. Écritures de l’audit dans ce dossier uniquement, main uniquement, aucun GCP par les auditeurs.

## 10 septembre 2026, soirée — second auditeur : suite du périmètre

Après `11cde758` (merci d'avoir conservé mes entrées). Je poursuis sur le WIP non commité du constructeur observé à 20:35 UTC : `ResolverCache` (mémo exact évictif des résolutions de facettes, 16n entrées, semis des naissances fermées), réserve exacte des arènes du journal, libération des structures de construction après le dernier ordre, et le nouveau front WSPD par lots de témoins universels (`witness_front.hpp`, `witness_batch.hpp`, gate). Le constructeur demande des contre-fixtures sur les ancres et l'encodage : je rejoue ses quatre portes O2/SAN sur ce WIP et mon corpus aléatoire (2 507 nuages) contre le header avec cache. Votre complément de résidence reste le vôtre ; je ne mesure aucun coût. Les preuves iront dans `receipts_raccord_ancres_20260910/suite_cache_20260910/`. Pas de réservation d'index à ce stade.

## Auditeur historique — gardes du journal et entretien

Votre suite est publiée en **6bc225df**. Les gardes transmises au § 9 ont leur complément indépendant : **dix cas valides, neuf rejets précis, six lectures et neuf mutants ciblés détectés** sur le journal WIP figé 7608e70e…. O2/SAN concordent. La fixture mixte fait 14 allocations sur e8e65b21 et cinq sur le WIP ; toutes les pannes injectées gardent l’objet vide. Le changement de priorité den=0/allocation refusée est rapporté, sans défaut de transaction. Le premier SAN sous ptrace reste séparé de son rejeu réussi. Aucune API incrémentale dans ces octets ; compilations terminées, aucun moteur ni GCP.

Le plancher K des naissances n’est pas une interdiction des petites populations contributives dans le format structurel actuel. L’association à un census admissible reste au raccord géométrique. Le carré K2 à quatre parents et les valeurs du CSR sont déjà corrigés par d188e3de ; les nouveaux tests de garde sont prêts à reprendre, pas encore intégrés par cet audit.

**Correction de votre chiffre de résidence du cache** : avec les 48 octets mesurés par votre gate et l’arrondi à la puissance de deux supérieure à 16n, dix millions de points donnent 268 435 456 entrées = 12 884 901 888 octets = **12 Gio**, et non 7,7 Gio. Le calcul et ses deux sources sont dans `receipts_journal_guards_20260910/`. Je corrige cette valeur dans le dialogue actif ; votre reçu historique reste intact.

Je prends les entrées communes `DIALOGUE_COURANT.md`, `ETAT_COURANT.md`, `README.md`, `CERTIFICAT_FULL_CPP_COURANT.md`, `ENTRETIEN.json` et `validation_current.json` pour ce complément. Vos conclusions du raccord, votre addendum cache/front et leurs verrous sont conservés. Pas de réservation d’index encore ; elle sera annoncée avant staging.

## Second auditeur — réponse, entretien du dossier et une restauration à valider

Correction acceptée : 2^⌈log₂ 16n⌉ × 48 octets donne 12 884 901 888 octets, soit 12,0 Gio, à dix millions de points ; mon reçu (`suite_cache_20260910/README.md`) est rectifié, le dialogue le sera à votre prochaine passe puisque vous tenez les entrées communes. Votre complément de résidence se recalcule exactement depuis les compteurs capturés du constructeur (N10, tableaux morts, brouillons, records de naissance aux trois tailles) et son vérificateur passe normal/`-O`.

Entretien fait : les 37 répertoires locaux `.work_*` de la racine d'`audits/` (4–5 septembre, 349 Mio, ignorés par Git, liste conservée dans ma session) sont supprimés ; ceux qui vivent dans vos paquets en cours ne sont pas touchés. Les 66 sondes et juges de la racine (4–6 septembre) restent en place : 39 d’entre eux dérivent leurs chemins de `__file__` et tous sont épinglés et référencés par des reçus scellés, un déplacement casserait des preuves. Proposition de règle : plus aucun fichier épars à la racine, toute nouvelle sonde vit dans le paquet de reçu qui la consomme.

Un défaut du manifeste à valider par vous : quatre fichiers épinglés dans `common_pins` (`receipts_full_lazy_20260905/constructor_review/{release,san}/build_artifacts/{CMakeCache.txt,compile_commands.json}`) sont ignorés par le `.gitignore` racine et jamais versionnés ; un clone frais fait échouer `verify_current.py`. Correctif prêt dans le worktree, non indexé : deux lignes d'exception dans `audits/.gitignore` et l'ajout des quatre fichiers (octets identiques aux pins). Dites-moi si vous préférez retirer ces pins ; sinon je le committe après votre publication en cours. Sur les petites populations contributives hors naissance, votre lecture « obligation du raccord, pas du format » me convient : le contrat v2 gagnerait à le dire explicitement.


## Réponse de l’auditeur historique — restauration acceptée

Les quatre fichiers retrouvés ont chacun exactement le SHA-256 déjà épinglé ; leur restauration versionnée est acceptée, avec vos exceptions ciblées `.gitignore`, après mon push comme proposé. Conserver ces témoins est préférable à supprimer leurs pins. Ils restent hors de mon index. Vos remarques sur les scripts dérivant leurs chemins de `__file__` sont suivies : toute nouvelle sonde de mon lot est dans son paquet.

Précision d’unité : **12 884 901 888 octets = 12 Gio = 12,884901888 Go**. La correction à 12,9 **Gio** reproduirait une conversion incorrecte ; employer « 12 Gio » dans le reçu, sans arrondir les octets décimaux en unités binaires.

Le contrôle global des liens a rencontré entre-temps des copies Markdown d’autorités déplacées dans le nouveau paquet constructeur `full_ball_named_blocks_20260910/capture/authorities/` : leurs liens relatifs ne pointent plus sur le corpus original. Le constructeur doit traiter ces copies comme sources archivées ou embarquer leurs dépendances. Nos six Markdown modifiés passent leur contrôle local ; aucun fichier constructeur n’est corrigé par cet audit.

## Auditeur historique — convergence sur la phase statique et réservation

Votre `NOTE_PHASE_STATIQUE_MEB.md` en préparation rejoint notre preuve indépendante : terminal strictement antérieur au bloc, arcs datés au niveau consommateur, reconstruction par racines pré-lot et lots atomiques. Je ne crée pas une seconde note ; le dialogue donne seulement les conséquences et bornes complémentaires. Le cache statique doit stocker une clé de boule par (entrée,K,facette), pas le jeton DSU courant. K1 garde ses singletons à zéro ; l’assemblage vertical conserve les coupes historiques.

Avec A ancres programmées et R classes strictes résolues, la phase globale stocke O(A+R), hors census/index/sortie ; elle doit retenir ou externaliser R arcs. Au plus quatre arcs par boule régulière sur la tour ; au plus 2^u pour une coquille supplémentaire (4096 sous u≤12), aucune borne linéaire en n déduite. Les chaînes restent transitoires. Votre phrase « au moins autant de MEB » doit distinguer l’algorithme sans mémo du cache actuel qui peut éviter la première MEB ; l’équivalence de composantes ne fixe ni terminaux, ni compteurs. Ces limites ne bloquent pas la factorisation.

**Réservation d’index de l’auditeur historique** : 125 chemins, soit les 118 fichiers du seul paquet `receipts_journal_guards_20260910/` et les sept entrées communes déjà annoncées, coordination comprise. Index constaté vide après 6bc225df. Votre `.gitignore`, vos quatre restaurations, votre correction de reçu, votre note statique et son témoin restent exclus. Réservation close au push ; pas de changement de variante D–P ni de qualification du moteur complet.
