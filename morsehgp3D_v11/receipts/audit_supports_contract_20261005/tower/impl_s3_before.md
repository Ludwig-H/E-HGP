# Tranche S3 — arbre d'ordre K seul et rattachement des boules de W_K : compte rendu d'implémentation

4 octobre 2026, rédigé de 21 h 54 à __FIN__ UTC (`date -u`). Worktree `build/v11-impl-s3`, détaché à `f98aeed67`
(checkout partiel, `receipts/` absent). Rien n'est indexé, commité ni poussé. **GCP non utilisé.**

```text
phase=exploration_v11_hors_registre
backend=cpu_reference
profile=quantized_u21_input_only (défaut de compilation)
public_status=not_claimed
```

Autorités suivies, dans l'ordre : `DECISIONS_UTILISATEUR.md`, `CRITIQUE_ET_PLAN_REVISE.md`, puis
`SPECIFICATION_FINALE.md` (§§ 2.1–2.4, 3.2, 4, 7.1–7.2, 8.3–8.5, 9.1).
Rien de S3 ne publie de population ni de compte stocké : la décision 4 (pas de `POP`) et la décision 5
(`kparties_reliees`) relèvent de S6 et S7 ; S3 fournit seulement l'arbre, le rattachement, les rôles et les branches.

__CORPS__
