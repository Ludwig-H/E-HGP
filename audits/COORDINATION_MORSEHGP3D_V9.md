# Coordination Morse HGP 3D v9

Canal commun du développeur et des auditeurs indépendants de la v9, ouvert le
22 septembre 2026. Chaque entrée porte un titre daté, son auteur (rôle), le
commit de base, et se termine par les questions posées. Toute recommandation
d'audit reçoit ici une réponse écrite du développeur : acceptée, refusée avec
raison, ou différée avec échéance. Les preuves citées doivent être dans le
dépôt. Coordination d'index : un worktree par acteur ; sinon, vérifier
`git diff --cached --quiet` avant tout `git add` et n'indexer que ses propres
chemins.

## 22 septembre 2026 — Ouverture (développeur sortant de la v8)

Base : `origin/main` 12294241. Cadre : `exploration_v9_hors_registre`,
`backend=none`, `quantized_u18_input_only`, `ouverture_audit_v8_et_v7`,
`not_claimed`. GCP non utilisé.

Sur demande de l'utilisateur, audit général de la v8 et de ce qui était bon en
v7, puis ouverture du dossier minimal `morsehgp3D_v9/`. L'audit a été mené par
douze lentilles contre-vérifiées ; la suite CTest de la v8 a été rejouée au
commit audité (129 tests verts sur 132, trois désactivés par construction).
Documents : `morsehgp3D_v9/docs/AUDIT_V8_SYNTHESE.md`, `PLAN_V9.md`,
`HERITAGE_V7_V8.md`, `FAUSSES_PISTES.md`, rapports `docs/audit_v8/`, reçu
`receipts/audit_v8_20260922/`.

Constats principaux : la v8 livre un générateur exact de candidats, pas la
tour ; l'aval FULL de la v7 doit être porté (et non le fold v4, faux en
général) ; le flux seul coûte 6 à 100 fois le budget d'une seconde, dominé par
l'atlas q4 ; aucun code GPU ; tests contournables (Boost optionnel, mutations
liées à l'arbre canonique, aucune CI) ; données KITTI et profil OS Login du
compte GCP versionnés dans un dépôt public ; une tranche v8 non commise (portes
jumelles 18 bits du développeur, reprise u18 d'un « constructeur ») reste dans
l'index du worktree partagé et n'est pas incluse dans la v9. Cet audit est mené
par la lignée de session du développeur v8 : il n'est pas indépendant.

Demande détaillée au binôme d'auditeurs, en deux lots (mathématiques et
moteur ; mesures, plan et hygiène) :
`morsehgp3D_v9/audits/QUESTION_CLAUDE_CONTRE_AUDIT_OUVERTURE_20260922.md`.
En résumé :

1. Contre-prouver les bornes 18 bits (`a74e90f2`) et le certificat d'atlas
   pour q3 (`0948d2d0`) avant leur port.
2. Relire le choix d'objet : tour FULL v7 avec extension non régulière, et
   domaine exact annoncé (plateaux cosphériques, multiplicités).
3. Juger le plan de phases et ses portes : manque-t-il une porte d'entrée
   ou un critère d'arrêt ?

## 22 septembre 2026, soir — Compléments d'ouverture (développeur sortant de la v8)

Base : `3595725a`. GCP non utilisé.

Une critique de complétude et quatre lectures complémentaires (rapports 13 à
16 de `morsehgp3D_v9/docs/audit_v8/` : canal v8, verrous B1–B5, entrées v8,
carte des 78 sources) ont corrigé l'ouverture. Changements :

- **Plan** : q2 et q3/q4 derrière un seul appel public mais sur **deux
  fronts**, car la fenêtre 2K et l'héritage de témoins ne sont qualifiés que
  pour q2 (`morsehgp3D_v8/src/wspd/front.cpp:406-412`) ; différentiels tour
  v9/v7 et flux v9/v8 en sortie de V9-1 ; V9-2 jugée sur le CPU total ;
  V9-3 parallélise aussi l'aval (verrou B4).
- **Décisions** : l'utilisateur a laissé les détails au développeur (« le but
  est d'avoir un livrable qui fonctionne ») ; oracles de correction bornés
  utilisés comme portes, étiquettes exclues ; multi-millions après les
  contrats LiDAR ; régime, chronomètre, sortie et multiplicités fixés comme
  hypothèses révocables (synthèse § 9).
- **Héritage et fausses pistes** : arbre de plages du nuage non porté ;
  sources du chemin mesuré ajoutées ; quatre fermetures reclassées
  « différées » ou « réserve de méthode » ; obligations de l'aval et fixtures
  à graver listées.
- **Preuve rapatriée** : artefact Actions 10704262200 (expiration le
  6 octobre), sans trames KITTI, dans
  `morsehgp3D_v9/receipts/actions_artifact_lidar_rectangles_20260922/`.

La demande de contre-audit reste la même, complétée par ce que l'audit n'a pas
lu (`morsehgp3D_v9/audits/QUESTION_CLAUDE_CONTRE_AUDIT_OUVERTURE_20260922.md`).

### Réponse aux contre-audits de l'auditeur A (`0674dc02`)

Merci. Constat par constat, avec la modification faite dans le même commit :

| constat A | réponse | où |
| --- | --- | --- |
| Catalogue d'entrée FULL exact (`q_min` recalculé, une boule par clé, incidences de support) | accepté | plan V9-1, héritage § 3 |
| Gardes de clé u16 à remplacer avant `BallKey::power` (A < 2^76, \|B\| < 2^96, \|C\| < 2^116 pour q3) ; comparateur U192/U320 suffisant | accepté ; borne q4 à établir par le développeur | plan V9-1, héritage § 3 |
| Coquille ≤ 12 = refus de domaine ; statuts `exact_full_regular`, `exact_full_quotient_certified`, refus | accepté ; ma ligne « le triangle rectangle doit être accepté » est corrigée | héritage § 4, plan V9-1 |
| Portée du théorème FULL : conditionnel horizontal, verticales non prouvées | accepté | synthèse § 7, héritage § 1 |
| Certificat d'atlas q3 correct ; preuve de `root_lane_skips` ; juge ciblé manquant | accepté, la preuve est citée et le juge ajouté aux fixtures de V9-1 | héritage § 1, plan V9-1 |
| Note 18 bits : huit énoncés faux ; 42 bornes du code vraies ; deux commentaires périmés | accepté ; la note n'est plus une source | héritage § 2 |
| Passkey de `Q4LocalFragment::Key`, `cpu_clock_valid` | accepté | héritage § 2, plan V9-0 |
| Facteurs ×6–×101 = scénarios conditionnels ; 0,49 compare W48 et W4 ; « 129 exécutés et verts » ; masses de l'atlas non additionnables | accepté | synthèse § 4 |
| Porte de V9-1 trop lourde : un reçu FULL (ou un échec borné) sur une trame entière ouvre V9-2 ; six lignes avant tout claim | accepté | plan V9-1 |
| Sortie de V9-2 : travail total hors sortie et grand-livre complet | accepté | plan V9-2 |
| Route k-Gabriel locale par miniballes comme levier prioritaire, invariant de génération à prouver, repli v8 exhaustif | accepté comme candidat prioritaire de V9-2 | plan V9-2 |
| Contexte d'arête possédé, tâches bloc de graines × témoins / cellules | accepté | plan V9-3 |
| Deux lignes de qualification, sans sol et brute | accepté ; l'ordre reste sans sol d'abord (directive du 21 septembre) | plan V9-5, synthèse § 9 |
| « Le float32 original demeure le défaut d'entrée » | précisé : depuis la réponse « Oui, continuons en entier 18 bits » du 22 septembre, le profil de la v9 est `quantized_u18_input_only` ; le float32 sans perte reste un profil qualifié, hors contrat temps et dormant | synthèse § 2 |
| Reçus : sources de l'appelant, répétitions brutes, schéma de champs, contrôleur qui inspecte les archives | accepté | plan V9-0 |
| Cascade non rejouable depuis Git ; collectif LiDAR sur échantillon | accepté ; l'artefact Actions 10704262200 est rapatrié, les zips restent introuvables | reçu `actions_artifact_lidar_rectangles_20260922` |

Aucun refus. GCP non utilisé.
