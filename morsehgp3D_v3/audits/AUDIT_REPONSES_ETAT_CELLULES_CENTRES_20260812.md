# Renvoi historique — réponses sur l'état des cellules de centres

Date : 12 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Ce fichier avait été livré comme une seconde réponse aux trois questions de
Claude. Ses corrections utiles sur la fraîcheur, le filtre droite--cellule, le
régime volumique, le potentiel d'intervalles et le Voronoï local ont été
contre-auditées puis fusionnées dans la section 13 de
[`AUDIT_REPONSES_CELLULES_CENTRES_20260812.md`](AUDIT_REPONSES_CELLULES_CENTRES_20260812.md).

Cette synthèse ajoute notamment les deux RLE
`SupportKey -> GeometricBallKey`, le certificat local d'expansion, la sortie
cosphérique `Theta(m^4)`, les scores affines i64, le test owner par paramètre
face--apex et la correction de la face canonique obtuse. Le verdict live reste
[`AUDIT_ETAT_COURANT.md`](AUDIT_ETAT_COURANT.md).

Elle n'est pas l'autorité live et ses ajouts restent auditables. En particulier,
le ledger postérieur `238cf12` ne ferme pas `130 033` occurrences pending; sa
correction et les limites du premier RLE sont dans
[`AUDIT_LEDGER_CAUSES_LIFTS_238CF12_20260812.md`](AUDIT_LEDGER_CAUSES_LIFTS_238CF12_20260812.md).

GCP non utilisé.
