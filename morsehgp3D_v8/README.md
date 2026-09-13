# Morse HGP 3D v8 — audit avant refonte

Ouverture demandée le 13 septembre 2026, sur `main` uniquement.

```text
phase=exploration_v8_hors_registre
backend=none
profile=quantized_u16_input_only
mode=audit_v7_math_and_architecture
public_status=not_claimed
```

La v8 commence par l'audit de la v7. Aucun code moteur ni résultat de
performance n'est repris automatiquement. La cible reste toute la tour
HGP FULL K=1..10 à 50 000 points sous une seconde, repli sur toute la tour
1..5, puis 100 ms ; la grande échelle G4 est un contrat distinct.

La structure v7 est conservée pour organiser la refonte : `src/`, `cli/`,
`oracle/`, `tests/`, `bench/`, `cmake/`, `docs/`, `audits/`, `receipts/`.
Les dossiers moteur sont pour l'instant des emplacements réservés,
pas un moteur compilable. Un test Python vérifie seulement l'inclusion
documentaire v8. Les preuves et mesures géométriques citées restent v7.

## Commencer ici

- [Audit général et décisions](docs/AUDIT_V7_SYNTHESE.md) : verdict, contrats,
  causes de lenteur, ce qui doit être conservé ou refait.
- [Tout l'algorithme expliqué simplement](docs/ALGORITHME_EXPLIQUE.md).
- [Plan de refonte priorisé](docs/PLAN_DE_REFONTE.md).
- [Fausses pistes à ne pas réintroduire](docs/FAUSSES_PISTES.md).

Pour approfondir : [fondements et objet FULL](audits/FONDEMENTS_ET_OBJET.md),
[WSPD, q2/q3/q4 et témoins](audits/WSPD_Q2_Q3_Q4.md),
[code et parallélisation](audits/IMPLEMENTATION_PARALLELISATION.md),
[mesures et contrats](audits/CONTRATS_ET_MESURES.md),
[périmètre et preuves de l'audit](audits/PERIMETRE_ET_PREUVES.md).

Verdict : dernières tours 50k publiées, environ 419 s pour 1..10 et
34 s pour 1..5 ; aucune chaîne GPU FULL industrielle ni qualification
multi-millions. Les optimisations privées ultérieures n'ont pas leur
nouvelle mesure 50k. La v8 démarre sur ces constats, sans statut hérité.

Entrées de suivi : [passation](PASSATION.md), [état de l'audit](audits/ETAT_COURANT.md).
GCP non utilisé pour l'audit d'ouverture.
