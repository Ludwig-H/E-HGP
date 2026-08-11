# Index des audits MorseHGP3D v3

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Ce dossier conserve quatre objets distincts : verdict live, audits épinglés,
notes de proposition ou de livraison, et archives. Un titre contenant « reçu »
ou un CTest vert ne change pas cette classification.

## Autorité live unique

- [`AUDIT_ETAT_COURANT.md`](AUDIT_ETAT_COURANT.md) : `HEAD`, worktree,
  contre-réceptions, portes ouvertes et état des tests. C'est le seul fichier
  dont les phrases au présent décrivent le dépôt courant.

Le résumé racine est [`../README.md`](../README.md) et l'architecture candidate
est [`../PROPOSITION.md`](../PROPOSITION.md). En cas de divergence temporelle,
le présent audit courant prévaut; le contrat et le statut des preuves restent
supérieurs aux trois documents v3.

## Audits actifs épinglés

| sujet | autorité | portée |
| --- | --- | --- |
| self-join q2 | [`AUDIT_Q2_SELFJOIN_8A39C53.md`](AUDIT_Q2_SELFJOIN_8A39C53.md) | snapshot immuable : preuve locale, réfutation du différentiel committé, bornes exactes et direction Yao/LBVH; le delta est uniquement dans l'audit courant |
| sidecar `cbac109` et source | [`AUDIT_DELTA_CBAC109_SIDECAR_ET_SOURCE_20260811.md`](AUDIT_DELTA_CBAC109_SIDECAR_ET_SOURCE_20260811.md) | snapshot `cbac109`; ses défauts live sont mis à jour uniquement dans l'audit courant |
| baseline sidecar/G4 | [`AUDIT_LIVE_SIDECAR_SOURCE_50K_20260811.md`](AUDIT_LIVE_SIDECAR_SOURCE_50K_20260811.md) | baseline `9483b1c`, mass-only et réfutations de source |
| pont H0 et fast path | [`REPONSE_CLAUDE_PONT_H0_FASTPATH_ET_Q4_20260811.md`](REPONSE_CLAUDE_PONT_H0_FASTPATH_ET_Q4_20260811.md) | preuve du quotient horizontal, resolver et contre-fixtures |
| cœur universel q3/q4 | [`NOTE_COEUR_UNIVERSEL_JUNG_ANCRES_Q3_Q4_20260811.md`](NOTE_COEUR_UNIVERSEL_JUNG_ANCRES_Q3_Q4_20260811.md) | certificat polynomial de Jung, hypothèses, limites et gate proposée |
| profondeur de demi-boule | [`QUESTION_CLAUDE_ANCRES_PROFONDEUR_DEMIBOULE_20260811.md`](QUESTION_CLAUDE_ANCRES_PROFONDEUR_DEMIBOULE_20260811.md), [`REPONSE_AUDIT_ANCRES_PROFONDEUR_DEMIBOULE_20260811.md`](REPONSE_AUDIT_ANCRES_PROFONDEUR_DEMIBOULE_20260811.md) | lemme complémentaire reçu, correction q2, résiduels incomparables et structure du falsificateur |
| contrat du sidecar | [`NOTE_CONTRAT_VALIDATED_HYBRID_SIDECAR_20260810.md`](NOTE_CONTRAT_VALIDATED_HYBRID_SIDECAR_20260810.md) | champs et preuves requis d'une frontière validée; pas un reçu d'implémentation |
| ancre de Jung réfutée | [`AUDIT_JUNG_ANCHOR_389A742.md`](AUDIT_JUNG_ANCHOR_389A742.md) | contre-fixture permanente à l'hypothèse d'ancre insuffisante |

L'ancienne note de livraison `P1a` a été supprimée après consolidation : ce nom
était déjà utilisé par le center-cover et ses chronos décrivaient le parcours
antérieur à la sortie précoce. Sa provenance utile et ses compteurs
reproductibles sont intégrés à l'audit q2 ci-dessus.

## Reçus reproductibles

Les sorties G4 mass-only sont dans
[`../receipts/g4_massonly_20260811/`](../receipts/g4_massonly_20260811/).

| fichier | SHA-256 | portée |
| --- | --- | --- |
| `cell_50k_raw.txt` | `6b355d0d9c7bf01dbdeb1d14dc442cab75570e6be044dcd50f314d79b9010afe` | masses par cellules, aucun tuple |
| `mask_scale_raw.txt` | `d82e43c7f4b32a5731cfdb2bbb9edf22cd7cecef0fdc73e84d1457277d61c740` | scaling count-only, aucun fold |

La provenance de session est
[`NOTE_CLAUDE_SESSION_G4_MASSONLY_50K_20260811.md`](NOTE_CLAUDE_SESSION_G4_MASSONLY_50K_20260811.md).
Elle documente la cible GCP comme arrêtée. Ces reçus refusent les routes
combinadiques mesurées; ils ne qualifient aucune source ni aucun temps
`warm_e2e`.

## Autorités mathématiques hors du dossier

- [`SPECIFICATION_MORSEHGP3D.md`](../../docs/SPECIFICATION_MORSEHGP3D.md) :
  contrat et SLO.
- [`STATUT_PREUVES_ET_HEURISTIQUES.md`](../../docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md) : registre des preuves et réfutations.
- [`INCIDENCES_SILENCIEUSES_GAMMA.md`](../../docs/math/INCIDENCES_SILENCIEUSES_GAMMA.md) : théorème d'inertie H0.
- [`CATALOGUE_PAIRES_DIAMETRALES_EXACT.md`](../../docs/math/CATALOGUE_PAIRES_DIAMETRALES_EXACT.md) : architecture exacte q2 Yao/LBVH.
- [`RNG_JUNG_CLIQUES_ET_NIVEAUX_PEUPROFONDS.md`](../../docs/math/RNG_JUNG_CLIQUES_ET_NIVEAUX_PEUPROFONDS.md) : bornes locales de Jung et limites des graphes low-rank.

## Classification de tous les autres fichiers

Tout fichier du dossier non listé dans les sections précédentes est conservé
comme archive, notebook, question/réponse ou provenance; il ne décrit pas le
worktree courant. Les classes sont explicites :

- `AUDIT_*.md`, hors audit courant et audits actifs listés : snapshot historique
  ou ancienne synthèse, généralement épinglé par le hash de son nom;
- `AUDIT_LIVE_*.md` et `AUDIT_RECEPTION_*.md`, hors liste active : verdict d'un
  ancien snapshot, jamais verdict live aujourd'hui;
- `NOTE_CLAUDE_*.md` : déclaration de livraison ou de session à recouper avec
  un audit et un reçu;
- `NOTE_SOLUTION_*.md`, `NOTE_GATE_*.md`, `NOTE_VERROUS_*.md` et autres
  `NOTE_*.md` non listés : proposition ou preuve de travail, sans statut live;
- `QUESTION*.md` et `REPONSE*.md`, hors réponses listées : échanges de
  recherche historiques;
- `check_*.py` : gates reproductibles locales, dont la portée est celle écrite
  dans leur audit parent.

Cette classification conserve les contre-exemples et la traçabilité sans
laisser une ancienne phrase au présent contredire l'autorité courante. Une
contradiction nouvelle devient une fixture permanente avant optimisation.

GCP non utilisé.
