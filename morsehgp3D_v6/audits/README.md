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
  `1069bc20`, série C locale `cd606257` et reçu G4 terminal `852ca703`. Il
  prime sur toute réponse historique ; aucun GO GCP n'est actuellement ouvert.
- [`REPONSE_AUDITEURS_MULTICPU_V6_20260901.md`](REPONSE_AUDITEURS_MULTICPU_V6_20260901.md) :
  réponse à la saturation du fold et à la conception GPU ; profil apparié,
  snapshots du design A, réception locale CPU/stub, portes device enregistrées
  et réception critique du reçu série C au § 5.17. Les prochaines sondes
  locales y sont ordonnées sans rouvrir une session facturable.
- [`NOTE_CLAUDE_RECU_SERIE_C_G4_20260901.md`](NOTE_CLAUDE_RECU_SERIE_C_G4_20260901.md) :
  lecture source de Claude du reçu `852ca703`; ses libellés de décision, sa
  dispersion et ses médianes sont rectifiés au § 5.17 de la réponse active.

## Historique de l'échange

- [`NOTE_CLAUDE_CONCEPTION_V6_20260831.md`](NOTE_CLAUDE_CONCEPTION_V6_20260831.md) :
  note initiale de conception, supersédée par l'état courant.
- [`QUESTION_CLAUDE_MULTICPU_20260901.md`](QUESTION_CLAUDE_MULTICPU_20260901.md) :
  question source, répondue par la note active ci-dessus.
- [`NOTE_CLAUDE_CONCEPTION_MULTICPU_GPU_20260901.md`](NOTE_CLAUDE_CONCEPTION_MULTICPU_GPU_20260901.md) :
  conception source CPU/GPU, répondue et bornée par la note active ci-dessus.
- [`REPONSE_CLAUDE_MULTICPU_GPU_20260901.md`](REPONSE_CLAUDE_MULTICPU_GPU_20260901.md) :
  journal de résolution dont la dernière livraison reçue est ancrée par
  `1069bc20` pour le profil ; ses formulations supersédées restent utiles pour
  la traçabilité mais sont subordonnées au verdict compact
  `ETAT_COURANT.md`.
- [`NOTE_CLAUDE_MESURES_G4_20260901.md`](NOTE_CLAUDE_MESURES_G4_20260901.md) :
  lecture initiale du reçu G4 ; ses unités, sa dispersion et ses projections
  sont corrigées par l'audit GCP historique correspondant.
- [`AUDIT_GCP_V6_P0_20260831.md`](AUDIT_GCP_V6_P0_20260831.md),
  [`NOTE_CLAUDE_GO_G4_SERIE_C_20260901.md`](NOTE_CLAUDE_GO_G4_SERIE_C_20260901.md)
  et [`ACCUSE_CLAUDE_GO_G4_SERIE_C_20260901.md`](ACCUSE_CLAUDE_GO_G4_SERIE_C_20260901.md) :
  portes, demande et accusé désormais consommés ; historique seulement, sans
  autorisation transférable à une nouvelle session.
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
