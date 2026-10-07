# Audit Codex — ouverture du suivi v12

7 octobre 2026. Rôle demandé par l'utilisateur : auditeur du développeur v12.

La base de compréhension est le [contre-audit indépendant de la v11](../../morsehgp3D_v11/receipts/audit_independant_v12_20261007/README.md),
sur `33c2ae3c8` / moteur `ac081a06f`, avec [19 constats à examiner lors des ports](../../morsehgp3D_v11/receipts/audit_independant_v12_20261007/findings.json).
Le dossier de préparation `52a790443`, puis l'ouverture formelle `f31845d16`, ont été lus pendant la clôture de cet
audit. Les décisions D1–D9 sont prises : Session résidente, latence FULL K1..5 en mémoire avec verticales, G4,
plusieurs séquences, K10 en objectif, u18 abandonné, progression u21/u24/u32, grands LiDAR et petits nuages mesurés.
Ces décisions sont enregistrées ; elles n'ont pas à être redemandées sur la base des questions historiques v11.

Cadre du suivi : `phase=exploration_v12_hors_registre`, `backend=cpu_reference ; cuda_g4 pour le catalogue`,
`objet=full_pi0`, `quantification=quantized_u21_input_only`, `public_status=not_claimed`.
Le présent reçu qualifie seulement les vérifications v11 qu'il décrit, pas l'architecture v12 proposée.

Points supplémentaires à reprendre au registre :

1. Juges de leviers : un banc refusé doit interdire toute adoption, de même qu'une prise ou une preuve manquante.
2. Cache : compter la capacité physique des blocs vivants et les réserves ; qualifier l'empoisonnement aussi sous
   Clang. Témoins chiffrés et prétraitements disponibles.
3. Cohortes : dimensionner le scratch au travail concurrent utile ; une cohorte géante ne justifie pas W tampons.
4. Preuve et chrono : 3 285 vidages disponibles pour 3 303 tentatives ; distinguer les régimes dits chauds, les
   frontières du temps FULL et la provenance effective du binaire.
5. Objets mathématiques : identité par chaîne, couverture datée, hyperarêtes de Kruskal et polyèdre restent des
   contrats distincts. Les huit réponses polyèdre et les réponses T/V/X/Y sont déposées dans le rapport mathématique.
6. L'élargissement u24/u32 impose de recalculer les budgets, centres absolus, mots Morton et exports. Une étendue
   locale certifiée sur une feuille ne couvre pas un site extérieur interrogé par un prédicat.

Pour chaque tranche, l'audit suivra définition et preuve, source portée, fixture indépendante, refus, capacité,
concurrence, objet canonique, puis mesure sur le chemin livré. Une qualification reste attachée à son pin, profil,
compilateur et données. Les nouvelles propositions d'algorithme seront examinées comme propositions à prouver et
mesurer. Aucun nouveau défaut FULL établi dans cet audit v11 ; statut public `not_claimed`. GCP non utilisé.
