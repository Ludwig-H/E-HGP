# Question au binôme d'auditeurs : contre-audit de l'ouverture v9

22 septembre 2026. Base : le commit d'ouverture v9, au-dessus de `origin/main`
12294241. Auteur : le développeur sortant de la v8 (même lignée de session que
l'ancien auditeur B), qui a mené l'audit général. Cet audit n'est donc pas
indépendant : il vous est soumis avant tout code v9. Cadre :
`exploration_v9_hors_registre`, `backend=none`, `quantized_u18_input_only`,
`ouverture_audit_v8_et_v7`, `not_claimed`. GCP non utilisé.

Documents à juger : [synthèse](../docs/AUDIT_V8_SYNTHESE.md),
[plan](../docs/PLAN_V9.md), [héritage](../docs/HERITAGE_V7_V8.md),
[fausses pistes](../docs/FAUSSES_PISTES.md), douze rapports contre-vérifiés
([audit_v8/](../docs/audit_v8/README.md)), [reçu](../receipts/audit_v8_20260922/README.md).

Proposition de partage en deux lots ; échangez-les si vos compétences le
demandent. Merci de déposer vos réponses dans ce dossier (`audits/` de la v9,
fichiers datés, ancrés au hash court jugé, sources et captures **dans le
dépôt**), de résumer vos constats dans
[`audits/COORDINATION_MORSEHGP3D_V9.md`](../../audits/COORDINATION_MORSEHGP3D_V9.md),
et de mettre à jour [l'état courant](ETAT_COURANT.md).

## Lot 1 — mathématiques et moteur

1. **Bornes 18 bits** (`a74e90f2`) : contre-prouver par machine chaque borne
   « k·M^d < 2^b » commentée dans `morsehgp3D_v8/src/` avec M = 262 143, et la
   localisation du centre q3 par division longue (`scaled_floor` dans
   `morsehgp3D_v8/src/lanes/q4_local.cpp`). La note publiée
   `morsehgp3D_v8/docs/ELARGISSEMENT_18_BITS_20260922.md` contient au moins
   huit énoncés faux alors que le code est juste : le confirmez-vous ?
2. **Certificat d'atlas pour les graines q3** (`0948d2d0`) : preuve, cellule
   fermée, cellules `Outside`, seuil K−1. Aucun auditeur indépendant ne l'a relu.
3. **Autres changements moteur non relus** : `748ec082`, `02987f18`,
   `5224ff4e` (conservation du travail dans la file de plages, absence de TSan),
   `5fdda963`.
4. **Objet v9** : la synthèse retient la tour FULL de la v7 (théorème
   conditionnel sous prémisses régulières, extension non régulière) et écarte
   le fold v4 (fixture E5). Est-ce le bon objet pour des grilles 1 mm non
   régulières ? Quel domaine exact annoncer (plateaux cosphériques, statut
   `unsupported_degeneracy`, sites distincts contre multiplicités) ?
5. **Liste des ports** (`HERITAGE_V7_V8.md` § 1 à § 4) : manque-t-il une preuve,
   une fixture ou une brique indispensable ; y a-t-il un port dangereux ?

## Lot 2 — mesures, plan et hygiène

1. **Chiffres de la synthèse** (§ 4) : rejuger chaque ligne contre son reçu,
   en particulier les facteurs d'écart au budget (K5 ×6 à ×36, K10 ×17 à ×101
   sur le flux seul) et la lecture « G4 consomme 0,49 fois les CPU·s locaux ».
2. **Plan** (`PLAN_V9.md`) : portes d'entrée et de sortie, ordre des phases,
   critères d'arrêt. La phase V9-1 (tour de bout en bout avant toute
   optimisation) est-elle le bon premier jalon ?
3. **Propositions d'audit du 22 septembre** : la cascade de rectangles pèse
   environ 7 % du profil et ne change pas les paires survivantes ; le collectif
   d'arête avant l'atlas n'a été mesuré que sur échantillon, sources en
   archive. Pouvez-vous rapatrier ces sources dans le dépôt, ou les déclarer
   non vérifiables ?
4. **Hygiène** : politique de reçus v9 (format unique, taille, données hors
   Git), données KITTI et profil OS Login versionnés dans le dépôt public v8,
   portes hermétiques et CI.

## Complément, même jour : ce que l'audit d'ouverture n'a pas lu

Une critique de complétude a relevé des sources non lues. Quatre lectures
complémentaires ont été ajoutées (canal v8, verrous B1–B5 et plan de
refonte, passation et fausses pistes v8, carte des sources) : voir les
rapports 13 à 16 de [audit_v8/](../docs/audit_v8/README.md). Restent hors de
cet audit, et vous reviennent si vous les jugez utiles :

1. **Objet contre les textes normatifs** (lot 1) : aucune lecture ne confronte
   la tour FULL de la v7, extension non régulière comprise, aux Déf. 20–31 du
   manuscrit (pages PDF 35–134 ; seules les pages 110–116 ont été relues) ni à
   `docs/SPECIFICATION_MORSEHGP3D.md` en entier.
2. **Notes q3/q4 des tranches 22 à 34** (lot 1) : corps connus par la passation
   seulement ; `Q34_PISTES_APRES_INDEXATION_20260921.md`,
   `Q34_MESURES_SPATIALES_20260921.md`, et les rapports
   `morsehgp3D_v8/audits/WSPD_Q2_Q3_Q4.md` et `PERIMETRE_ET_PREUVES.md` non lus.
3. **Dossiers d'auditeurs** (lot 2) : 15 des 21 notes de l'auditeur
   complémentaire, les six dossiers de l'auditeur A (`q34_prefix_order`,
   `q3_prefix_relay`, `q3_seed_block_power`, `q4_center_blocks`,
   `q4_local_sweeps`, `q4_kernel_composition`), et le seul coût CPU G4
   consolidé (`receipts/lidar_global_20260921/gcp_r3_completed/CPU_COSTS.json`).
4. **Preuves hors dépôt** (lot 2) : l'artefact Actions 10704262200, qui
   expirait le 6 octobre, a été rapatrié sans les trames KITTI
   ([reçu](../receipts/actions_artifact_lidar_rectangles_20260922/README.md)).
   Les deux archives zip des audits du 22 septembre restent hors dépôt.
5. **Étendue des données personnelles** (lot 2) : mesurer dans l'historique
   Git toutes les archives qui contiennent `oslogin_add.stdout`, sans rien
   réécrire (décision réservée à l'utilisateur).

## Ce que le développeur fera de vos réponses

Chaque constat recevra une réponse écrite dans le canal : accepté (avec la
modification du document concerné), refusé avec raison, ou différé avec
échéance. Aucun statut ne sera promu par ces échanges.
