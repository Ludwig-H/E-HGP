# Index des audits MorseHGP3D v3

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Ce dossier est volontairement réduit aux autorités encore utiles. Les anciennes
notes de livraison, réponses, benchmarks sans reçu et snapshots remplacés ont
été supprimés du worktree; leur historique reste consultable dans Git. Un titre,
un message de commit ou un CTest vert ne vaut jamais réception.

## Verdict live

- [`AUDIT_ETAT_COURANT.md`](AUDIT_ETAT_COURANT.md) est l'unique verdict mutable :
  il épingle le `HEAD`, les empreintes utiles, les contre-exemples, les tests
  qualifiables et les portes ouvertes.

Le résumé est [`../README.md`](../README.md) et l'architecture durable est
[`../PROPOSITION.md`](../PROPOSITION.md). En cas de dérive temporelle, corriger
ces trois fichiers dans le même passage; les spécifications et le registre des
preuves sous `docs/` restent supérieurs.

## Snapshots et preuves conservés

| objet | portée exacte |
| --- | --- |
| [`AUDIT_Q2_SELFJOIN_8A39C53.md`](AUDIT_Q2_SELFJOIN_8A39C53.md) | preuve locale q2, réfutation du différentiel compensable de `8a39c53` et profil de coût du snapshot; tout successeur est jugé dans l'audit courant |
| [`AUDIT_DELTA_CBAC109_SIDECAR_ET_SOURCE_20260811.md`](AUDIT_DELTA_CBAC109_SIDECAR_ET_SOURCE_20260811.md) | contre-exemples du sidecar `cbac109` et contrat de frontière; le statut du successeur est uniquement live |
| [`AUDIT_JUNG_ANCHOR_389A742.md`](AUDIT_JUNG_ANCHOR_389A742.md) | contre-fixture permanente à une ancre de Jung insuffisamment certifiée |
| [`NOTE_COEUR_UNIVERSEL_JUNG_ANCRES_Q3_Q4_20260811.md`](NOTE_COEUR_UNIVERSEL_JUNG_ANCRES_Q3_Q4_20260811.md) | preuve des certificats cœur/profondeur q3/q4, hypothèses, égalités fail-open et limites industrielles |
| [`NOTE_CLAUDE_SESSION_G4_MASSONLY_50K_20260811.md`](NOTE_CLAUDE_SESSION_G4_MASSONLY_50K_20260811.md) | provenance de la session G4 mass-only et arrêt de la cible; déclaration de session, pas verdict produit |

## Reçus

Les reçus G4 mass-only sont dans
[`../receipts/g4_massonly_20260811/`](../receipts/g4_massonly_20260811/).

| fichier | SHA-256 | portée |
| --- | --- | --- |
| `cell_50k_raw.txt` | `6b355d0d9c7bf01dbdeb1d14dc442cab75570e6be044dcd50f314d79b9010afe` | masses de cellules, aucun tuple ni pipeline |
| `mask_scale_raw.txt` | `d82e43c7f4b32a5731cfdb2bbb9edf22cd7cecef0fdc73e84d1457277d61c740` | scaling count-only, aucun fold |

Le dossier
[`../receipts/selfjoin_q2_20260811/`](../receipts/selfjoin_q2_20260811/)
contient des journaux CPU diagnostiques. `scale_counters_raw.txt` mêle deux
binaires après un append postérieur au marqueur de fin : ce n'est plus un reçu
mono-snapshot immuable. `anchor_core_counters_raw.txt` ne devient qualifiable
qu'après fin d'écriture, empreintes complètes et réception des portes du
falsificateur. Leurs compteurs peuvent falsifier une route; leurs temps sous
charge ne sont ni un benchmark reçu ni `warm_e2e`.

## Autorités externes

- [`SPECIFICATION_MORSEHGP3D.md`](../../docs/SPECIFICATION_MORSEHGP3D.md) :
  contrat et SLO.
- [`STATUT_PREUVES_ET_HEURISTIQUES.md`](../../docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md) :
  registre des preuves et réfutations.
- [`INCIDENCES_SILENCIEUSES_GAMMA.md`](../../docs/math/INCIDENCES_SILENCIEUSES_GAMMA.md) :
  inertie H0.
- [`CATALOGUE_PAIRES_DIAMETRALES_EXACT.md`](../../docs/math/CATALOGUE_PAIRES_DIAMETRALES_EXACT.md) :
  architecture q2 Yao/LBVH.
- [`RNG_JUNG_CLIQUES_ET_NIVEAUX_PEUPROFONDS.md`](../../docs/math/RNG_JUNG_CLIQUES_ET_NIVEAUX_PEUPROFONDS.md) :
  Jung et limites des graphes low-rank.

GCP non utilisé.
