# Passation v8 — audit d'ouverture

13 septembre 2026. Cadre : `exploration_v8_hors_registre`, `backend=none`,
`quantized_u16_input_only`, `audit_v7_math_and_architecture`, `not_claimed`.

La demande remplace l'optimisation incrémentale v7 par un audit complet
avant reconstruction. La base publiée examinée est main `dc57ffd5`.
Les modifications v6/v7 déjà présentes sont conservées, sans les inclure
dans cette ouverture v8. Le delta privé fused_history du 11 septembre
reste distinct des résultats publiés ; sa publication n'est pas poursuivie
dans ce changement de cap. Aucun processus de benchmark restant constaté
à l'ouverture du 13 septembre ; GCP non utilisé.

L'audit d'ouverture est rédigé : [synthèse](docs/AUDIT_V7_SYNTHESE.md),
[exposé pédagogique](docs/ALGORITHME_EXPLIQUE.md), quatre rapports détaillés
et [plan de refonte](docs/PLAN_DE_REFONTE.md). Il couvre WSPD et témoins,
supports q2/q3/q4, census, rattachements, histoires, verticales,
parallélisation, mémoire, tests et livraison. Sa portée n'est pas une
certification ligne par ligne de tout le corpus ni une réexécution C++.

Décisions principales : conserver le contrat FULL avec vrais parents,
partager les objets géométriques, préparer les rattachements indépendamment
de l'histoire, découper l'intérieur des gros rectangles, reconstruire
les histoires par contractions parallèles, puis réemployer les marques et
index. Le détail distingue explicitement régularité, coquilles, sorties
quadratiques et propositions encore sans qualification.

Les contrats restent ouverts : 50k 1..10/1..5 en environ 419/34 s sur les
dernières complétions publiées, pas de nouvelle capture 50k des prototypes
récents, pas de chaîne intégralement GPU, pas de dizaines de millions.

L'inventaire épingle la base publiée et les deltas locaux. Six lecteurs
de reçus clos et deux recalculs rationnels ont passé ; les contrôles de
livraison sont dans [PUBLICATION_CHECKS](receipts/audit_v7_20260913/PUBLICATION_CHECKS.json).
Le contrôleur documentaire couvre désormais la v8 et possède un test
positif/négatif de découverte, sans inclure les futurs audits indépendants.
Aucun statut formel modifié, aucun moteur compilé, GCP non utilisé.

Suite réordonnée sur demande explicite du 13 septembre : **P0 d'abord,
supprimer la préparation systématique O(|A|²+|B|²)** après l'échec des
témoins universels. Comparer minorants issus de petits ensembles,
parcours conjoints de blocs, requêtes géométriques groupées et sélection
directe de sous-produits ; aucune solution n'est imposée. Le
[plan](docs/PLAN_DE_REFONTE.md) fixe les preuves et mesures de travail
total, coût aval compris, avant les campagnes CPU/GPU. P0 reste ouverte.

La tranche FULL minimale et les petits juges servent cette comparaison ;
ne pas la repousser derrière un port général de la v7. Garder les tailles
8k/16k/32k et s8/10/12. Aucun moteur n'est modifié par cette décision.
Ne pas reprendre le chantier fused v7 comme si le changement de cap
n'avait pas eu lieu. Le
[journal v8](../audits/COORDINATION_MORSEHGP3D_V8.md) porte les questions
à l'auditeur indépendant et la coordination d'index.
