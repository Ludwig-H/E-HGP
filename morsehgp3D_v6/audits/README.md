# Audits de MorseHGP3D v6

Ce dossier est le canal de travail compact entre Claude et les auditeurs.

Cadre : `phase=exploration_v6_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Cycle (inchangé) : les auditeurs poussent sur `main` ; leurs `AUDIT_*` /
`CONTRE_AUDIT_*` / `ADDENDUM_*` se lisent et s'exécutent avant toute
dépense. Claude répond par `REPONSE_CLAUDE_*` / `NOTE_CLAUDE_*` /
`QUESTION_CLAUDE_*` dans ce dossier ; chaque livraison produit un reçu
immuable ancré au commit dans `receipts/<chantier>_<date>/`. Tout rapport est
daté `_YYYYMMDD` et, s'il juge du code, ancré au hash court ; `README.md` et
`ETAT_COURANT.md` sont les deux index mutables non datés. Les audits motivent
des corrections, ils ne certifient rien.

## À lire maintenant

- [`ETAT_COURANT.md`](ETAT_COURANT.md) : verdict mutable, pins jugés, checkpoint
  hôte `4a85c13d` (C1, garde 2E et témoin arithmétique partiel), profil
  `1069bc20`, série C locale `cd606257`, reprise `c8f69673` et reçu G4 tests
  terminal `e66cd978`, durcissement local `2aaa4a53`, sonde équilibrée
  `b79e29a5`, reprise/revalidation `c2d2ac69` et harnais de sonde `32da1550`.
  Il prime sur toute réponse historique ; aucun GO GCP n'est actuellement
  ouvert.
- [`REPONSE_AUDITEURS_MULTICPU_V6_20260901.md`](REPONSE_AUDITEURS_MULTICPU_V6_20260901.md) :
  réponse à la saturation du fold et à la conception GPU ; profil apparié,
  snapshots du design A, réception locale CPU/stub, portes device enregistrées
  et réceptions critiques des reçus série C au § 5.17, du durcissement au
  § 5.19, de la reprise et du reçu tests au § 5.20, puis du lot local et de la
  sonde équilibrée au § 5.21. Les pins plus récents sont reçus et bornés
  dans les deux audits dédiés ci-dessous et dans l'état courant.
- [`REPONSE_AUDITEUR_COMPACTDELTA_CSR_20260902.md`](REPONSE_AUDITEUR_COMPACTDELTA_CSR_20260902.md) :
  GO exploratoire pour KeyCSR, sous égalité sémantique complète ; FidCSR et
  toute décision de performance restent des paliers séparés. Le prototype WIP
  est structurellement solide, ses portes courtes et sa campagne sanitizer
  sont vertes, sans blocage sémantique identifié ; la garde vide et le compteur
  causal sont déjà corrigés, le scratch non instrumenté attend seulement le
  futur reçu de mesure.
- [`ALERTE_SONDE_ABLATION_REDUCE_20260902.md`](ALERTE_SONDE_ABLATION_REDUCE_20260902.md) :
  limites causales historiques et réception de `32da1550` : la réagrégation
  après scellement et le statut exact sont reçus avec 21/21 scènes normales et
  optimisées, plus CTest 2/2. La dernière liaison du régime (argv/META,
  `liveness`, famille, paramètres de profil et identité exacte) précède toute
  nouvelle mesure, sans bloquer KeyCSR.
- [`CONTRE_AUDIT_REPRISE_PERSISTANTE_V6_20260902.md`](CONTRE_AUDIT_REPRISE_PERSISTANTE_V6_20260902.md) :
  contre-audit du chemin de reprise, mis à jour pour `c2d2ac69`. Les quatre
  mutants résiduels de `4ef96717` sont corrigés et le revalidateur passe 22
  scènes ; une conversion de génération encore placée avant l'armement du
  funnel reste le seul écart de sûreté. L'échec de journal après STOP est
  classé séparément comme contrat P2.

## Historique de l'échange

- [`NOTE_CLAUDE_CONCEPTION_V6_20260831.md`](NOTE_CLAUDE_CONCEPTION_V6_20260831.md) :
  note initiale de conception, supersédée par l'état courant.
- [`QUESTION_CLAUDE_MULTICPU_20260901.md`](QUESTION_CLAUDE_MULTICPU_20260901.md) :
  question source, répondue par la note active ci-dessus.
- [`NOTE_CLAUDE_CONCEPTION_MULTICPU_GPU_20260901.md`](NOTE_CLAUDE_CONCEPTION_MULTICPU_GPU_20260901.md) :
  conception source CPU/GPU, répondue et bornée par la note active ci-dessus.
- [`REPONSE_CLAUDE_MULTICPU_GPU_20260901.md`](REPONSE_CLAUDE_MULTICPU_GPU_20260901.md) :
  journal de résolution du chantier jusqu'au retour § 5.22 / `c2d2ac69` ; ses formulations
  supersédées restent utiles pour la traçabilité mais sont subordonnées au
  verdict compact `ETAT_COURANT.md`.
- [`NOTE_CLAUDE_MESURES_G4_20260901.md`](NOTE_CLAUDE_MESURES_G4_20260901.md) :
  lecture initiale du reçu G4 ; ses unités, sa dispersion et ses projections
  sont corrigées par l'audit GCP historique correspondant.
- [`NOTE_CLAUDE_RECU_SERIE_C_G4_20260901.md`](NOTE_CLAUDE_RECU_SERIE_C_G4_20260901.md) :
  lecture source du reçu `852ca703`; ses libellés de décision, sa dispersion
  et ses médianes sont rectifiés au § 5.17 de la réponse active.
- [`NOTE_CLAUDE_RECU_TESTS_G4_20260902.md`](NOTE_CLAUDE_RECU_TESTS_G4_20260902.md) :
  lecture factuelle du reçu K10/K5 `e66cd978`; les égalités sont reçues, ses
  formulations de pente, RSS, temps cumulés et autorité K5 sont bornées au
  § 5.20 de la réponse active.
- [`ACCUSE_CLAUDE_GO_G4_TESTS_20260902.md`](ACCUSE_CLAUDE_GO_G4_TESTS_20260902.md) :
  autorisation mono-session consommée du reçu K10/K5 ; aucune autorité de
  relance.
- [`NOTE_CLAUDE_SONDE_ABLATION_REDUCE_20260902.md`](NOTE_CLAUDE_SONDE_ABLATION_REDUCE_20260902.md)
  et [`QUESTION_CLAUDE_COMPACTDELTA_CSR_20260902.md`](QUESTION_CLAUDE_COMPACTDELTA_CSR_20260902.md) :
  note source corrigée et question désormais répondue par la réponse KeyCSR.
- [`AUDIT_GCP_V6_P0_20260831.md`](AUDIT_GCP_V6_P0_20260831.md) : portes et
  clôture historique. La demande et l'accusé série C consommés, ainsi que
  l'alerte K5 fermée prospectivement par `2aaa4a53`, ont été retirés du dossier
  actif ; Git conserve leur trace. Le canon v1 demeure post hoc et v2 reste une
  nouvelle identité, sans autorité rétroactive.
- [`ALERTE_CAMPAGNE_CPU_MIXTE_20260831.md`](ALERTE_CAMPAGNE_CPU_MIXTE_20260831.md) :
  preuves historiques des captures CPU et statut borné de la confirmation
  hors échantillon.

## Décisions actives héritées

- Le profil produit impose `s ≥ 8` sur toute la voie, q2 comprise ; les
  séparations plus petites sont au plus des diagnostics test-only.
- Positions dupliquées refusées (`unsupported_degeneracy`) ; jamais de jitter.
- Le P0 Gabriel/Gamma (sémantique de forêt `verified_events_only`, refus de
  `require_exact=true`) reste ouvert et documenté ; la v6 n'y change rien.

## Fraîcheur et validation

Avant publication : `python tools/check_docs.py` et `git diff --check`. Un
vert documentaire ne prouve ni exactitude, ni performance, ni fraîcheur
sémantique.
