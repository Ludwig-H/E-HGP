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
  `b79e29a5`, reprise/revalidation `c2d2ac69`, harnais de sonde et pin
  sémantique KeyCSR `8afd1057`, profil d'échelle `d8d7a7f7` et corrections de
  portée `788b22da`.
  Il prime sur toute réponse historique ; aucun GO GCP n'est actuellement
  ouvert.
- [`REPONSE_AUDITEUR_ECHELLE_V6_20260902.md`](REPONSE_AUDITEUR_ECHELLE_V6_20260902.md) :
  arbitrage des six verrous de `4d79dbd3`. Les cinq paliers RAM peuvent
  avancer ; doublons, statuts, digest v2, disque et oubli des facettes sont
  bornés sans changement silencieux d'objet. L'ordre résident est confirmé,
  l'ordre externe doit porter son ordinal. La contre-lecture du WIP P1--P3
  ferme une référence préfixe clairsemée réellement acceptée, borne `VmHWM` au
  processus cumulatif et propose l'écriture directe qui retire réellement les
  doubles capacités ; aucun gain mémoire n'est encore reçu. `788b22da` reçoit
  les corrections de provenance, l'erreur numérique `×6`, les claims C1–C6 et
  la portée des gardes. Jalons CPU obligatoires, sérialisation et indépendance
  supposée de `ru_maxrss` restent à fermer dans le WIP.
- [`REPONSE_AUDITEURS_MULTICPU_V6_20260901.md`](REPONSE_AUDITEURS_MULTICPU_V6_20260901.md) :
  réponse à la saturation du fold et à la conception GPU ; profil apparié,
  snapshots du design A, réception locale CPU/stub, portes device enregistrées
  et réceptions critiques des reçus série C au § 5.17, du durcissement au
  § 5.19, de la reprise et du reçu tests au § 5.20, puis du lot local et de la
  sonde équilibrée au § 5.21. Les pins plus récents sont reçus et bornés
  dans les audits dédiés ci-dessous et dans l'état courant.
- [`REPONSE_AUDITEUR_COMPACTDELTA_CSR_20260902.md`](REPONSE_AUDITEUR_COMPACTDELTA_CSR_20260902.md) :
  pin sémantique KeyCSR `8afd1057` reçu : deux routes sans repli, ownership,
  rejet des temporaires, comparateur tiers, rejeu, kind construit et vrais
  offsets sont verts sur la matrice complète et sous sanitizers ciblés. FidCSR
  et toute performance restent des paliers séparés. Le générateur `d6888093`
  est reçu comme calendrier déterministe (210/280 runs), pas comme plan final :
  grammaire/adjacence, variante CTest `-O`, rôles, identités des copies,
  commandes, coordonnées, callback, affinité et `reduce_v3` restent à sceller
  avant campagne.
- [`ALERTE_SONDE_ABLATION_REDUCE_20260902.md`](ALERTE_SONDE_ABLATION_REDUCE_20260902.md) :
  réception fonctionnelle de `32da1550`, puis contre-lecture du harnais
  `8afd1057` : outils critiques hors `PATH` et 23 scènes reçus ; famille, argv
  et CPU sont liés, mais `liveness`, layout, inflight/pics, coordonnée, clés
  inconnues, `lscpu` et identité stricte restent permissifs ; l'option `--` de
  `sha256sum` doit être restaurée en confinant le faux test. Ces dents précèdent
  une nouvelle mesure, sans bloquer KeyCSR.
- [`ALERTE_G4_ECHELLE_V6_20260902.md`](ALERTE_G4_ECHELLE_V6_20260902.md) :
  préflight actualisé au profil épinglé `d8d7a7f7` ; **NO START resserré**.
  Smax/layout, normalisation `:11`, inventaires, Q2 désarmée et budget sont
  reçus. Restent le corps code 2 sans payload et son vrai trajet CLI, le secours
  sans tas, les créations partielles de fils, `BIN_MATRICE`, le lifecycle à
  layout non vide et la portée exacte de la grille. Le selftest campagne du pin
  sort aussi 141 sous `pipefail` avant sa contre-fixture finale. L'audit corrige sa
  demande trop forte d'interdire tout code 134 en v2 et repousse les dents
  génériques de grammaire/version en P2.
- [`REPONSE_AUDITEUR_CONCEPTION_C6_20260902.md`](REPONSE_AUDITEUR_CONCEPTION_C6_20260902.md) :
  GO de conception borné pour C6, sans code ni GO G4. Le premier jalon emploie
  deux IN + deux OUT hôte aux leases séparés, un flux et un jeu device ; la
  réponse fixe validation transactionnelle, chronos non additifs, modèle C6
  séparé du stub séquentiel et jalons sans rouvrir l'objet mathématique. La
  contre-lecture du WIP d'encodeur reçoit les offsets fixes mais ferme une
  double lecture de `Source` qui permet encore d'écrire une clé invalide après
  prévalidation ; l'API réelle doit être stable, `noexcept` et sans alias avant
  son raccord CMake. L'anneau différé progresse, mais doit encore rendre
  ses fins one-shot, rendre `publish` et l'abandon terminaux, figer le contexte
  des erreurs device et libérer ses baux sans allocation dans les destructeurs
  RAII avant son raccord CMake/registre.
- [`CONTRE_AUDIT_REPRISE_PERSISTANTE_V6_20260902.md`](CONTRE_AUDIT_REPRISE_PERSISTANTE_V6_20260902.md) :
  contre-audit du chemin de reprise, mis à jour pour `c2d2ac69`. Les quatre
  mutants résiduels de `4ef96717` sont corrigés et le revalidateur passe 22
  scènes ; une conversion de génération encore placée avant l'armement du
  funnel reste le seul écart de sûreté. L'échec de journal après STOP est
  classé séparément comme contrat P2.

## Historique de l'échange

- [`QUESTION_CLAUDE_ECHELLE_V6_20260902.md`](QUESTION_CLAUDE_ECHELLE_V6_20260902.md) :
  question source sur le plan d'échelle et ses six verrous, répondue par la
  réponse active ci-dessus.
- [`NOTE_CLAUDE_PIN_KEYCSR_20260902.md`](NOTE_CLAUDE_PIN_KEYCSR_20260902.md) :
  livraison du pin sémantique `8afd1057`, reçue dans la réponse KeyCSR active ;
  elle diffère explicitement la campagne de performance.
- [`REPONSE_CLAUDE_PREFLIGHT_ECHELLE_20260902.md`](REPONSE_CLAUDE_PREFLIGHT_ECHELLE_20260902.md) :
  réponse de planification au NO START, reçue et resserrée dans l'alerte G4
  active ; son pin `fec58e1f` ne contient encore aucun des changements annoncés.
- [`REPONSE_CLAUDE_ECHELLE_ET_C6_20260902.md`](REPONSE_CLAUDE_ECHELLE_ET_C6_20260902.md) :
  accusé de Claude aux arbitrages échelle/C6, reçu par `788b22da` pour ses
  corrections documentaires et l'arithmétique wire 9/91 ; les paliers produit
  et l'encodeur C6 restent des WIP séparés jusqu'à leur pin.
- [`QUESTION_CLAUDE_CONCEPTION_C6_20260902.md`](QUESTION_CLAUDE_CONCEPTION_C6_20260902.md) :
  question source sur l'anneau de lots, répondue par la réponse C6 active.
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
- [`QUESTION_CLAUDE_PREREG_MESURE_KEYCSR_20260902.md`](QUESTION_CLAUDE_PREREG_MESURE_KEYCSR_20260902.md) :
  pré-inscription source, répondue dans la même réponse KeyCSR afin de ne pas
  multiplier les audits actifs.
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
