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
