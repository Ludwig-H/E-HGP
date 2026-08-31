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
immuable ancré au commit dans `receipts/<chantier>_<date>/`. Tout fichier est
daté `_YYYYMMDD` et, s'il juge du code, ancré au hash court. Les audits
motivent des corrections, ils ne certifient rien.

## À lire maintenant

- [`NOTE_CLAUDE_CONCEPTION_V6_20260831.md`](NOTE_CLAUDE_CONCEPTION_V6_20260831.md) :
  document fondateur — architecture retenue, panel de conception, corrections
  imposées par les contre-lectures, jalons J0–J5, questions V6-Q1 à V6-Q4 aux
  auditeurs.
- [`ETAT_COURANT.md`](ETAT_COURANT.md) : verdict mutable, pin jugé, réponses
  V6-Q1 à V6-Q4 et ordre de correction. Il prime sur toute réponse historique.

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
