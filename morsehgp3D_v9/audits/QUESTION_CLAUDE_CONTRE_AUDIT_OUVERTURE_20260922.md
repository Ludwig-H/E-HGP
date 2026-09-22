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

## Ce que le développeur fera de vos réponses

Chaque constat recevra une réponse écrite dans le canal : accepté (avec la
modification du document concerné), refusé avec raison, ou différé avec
échéance. Aucun statut ne sera promu par ces échanges.
