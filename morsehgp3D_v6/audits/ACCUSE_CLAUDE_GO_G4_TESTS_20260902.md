# ACCUSÉ — session G4 « tests K=10 / K=5 » : SHA d'exécution gravé

Date : 2 septembre 2026. Quatrième départ facturable au sens de votre
§ 5.16 (« toute quatrième tentative demandera un nouvel accusé explicite »),
après fermeture des préalables des § 5.17–5.18 (`13669280`), de l'axe
`smax` et du profil de tests (`1395c4f2`), et de la reprise persistante
§ 5.18.6 (`c8f69673`, `REPONSE_CLAUDE_MULTICPU_GPU_20260901.md` § 10).

**SHA d'exécution : `c8f69673`**
(`c8f696739b0b5f1692432f9a7f4954bdceb7b2bb`, commit `serie c : reprise
persistante apres perte du superviseur (§ 5.18.6) …`, poussé sur `main`).

Profil : `CAMPAIGN_PROFILE=g4_tests_v1` (épinglé, 15 fichiers normatifs) —
phase de tests demandée par l'exploitant (« fais les tests », « aussi pour
K=5 ») : 40 murs CPU (4 familles × {8000, 16000, 32000, 50000} × {K=10,
K=5} au réglage retenu 48 fils / inflight 2 / join 0, sans digest) + 8 bras
`--digest` à 32000 pour les deux K (fixtures d'égalité, `tower_scope=prefix_k5`
exigé à K=5), deux passages aller/retour ; 2 attributions (K=10) ; build
CUDA (arch 120 contractuelle) + inventaire exact des 16 portes gpu, aucun
pilote (mesuré au reçu `1788293187`). Enveloppe 5 h GCE / 285 min invité
(prédicat 285·60 + 300 + 120 + 480 = 18000), budget gravé 8 344 s pour une
fenêtre de 13 195 s. Aucun résultat ne changera `public_status=not_claimed`.

Preuves rejouées à ce SHA, worktree propre : selftest cycle de vie 75/75
(dont reprise R1–R6bis), selftest campagne (rejoué au HEAD), `test_gcp_safety`
81/81 + intégration v6 2/2, `v6_campaign_pin.sh` rejoué (15 blobs),
contrôles racine verts. Session lancée par le point d'entrée de confiance
du bootstrap, WORK dans `/workspaces/.ehgp-sessions` (0700) ; en cas de
perte du superviseur : `recover_v6_session.sh <WORK>` (jamais un
redémarrage). Arrêt certifié `TERMINATED` sur la génération exacte, succès
ou échec.

GCP non utilisé par cet accusé.
