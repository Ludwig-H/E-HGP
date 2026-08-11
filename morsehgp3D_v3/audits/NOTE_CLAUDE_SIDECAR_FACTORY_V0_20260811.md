# Note de livraison de Claude — sidecar v0 déclaré, non reçu

Date : 11 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Snapshot de code : `9483b1cd5ff691bc53f51eb2776aaba77b011e43`.

Cette note conserve la provenance de la livraison, mais ne la qualifie pas. Le
verdict indépendant et les obligations de correction sont dans
[`AUDIT_LIVE_SIDECAR_SOURCE_50K_20260811.md`](AUDIT_LIVE_SIDECAR_SOURCE_50K_20260811.md).

## Ce qui est effectivement livré

- `ValidatedHybridSidecar` possède les points et le catalogue par déplacement et
  n'est constructible que par sa factory.
- La factory revérifie les digests fournis, la partition du pool, le tri des
  membres et supports, leur appartenance à la coquille déclarée, la saturation
  fermée par scan de tous les points, la miniboule des membres, l'ordre des lots
  et plusieurs témoins de suppression.
- Les fixtures livrées rejettent notamment une `BallKey` dupliquée, une table
  amputée rejouée avec le reçu de la table complète, ainsi que trois mutants sur
  les preuves principales.
- Le pipeline appelle la factory avant le fold dans les modes `hybrid` et
  `hybrid-prefix`; `prefix-all` reste un juge relatif à la table fournie.

Ces vérifications sont utiles pour un harnais CPU borné. Elles ne suffisent pas
à constituer une frontière de confiance ni une source complète.

## Pourquoi la livraison n'est pas reçue

1. `HybridSourceReceipt::make` est public. Un appelant peut calculer les digests
   d'une table amputée et fabriquer un nouveau reçu avec
   `enumeration_completed=true` et `rank_bound>=point_count`. La fixture de
   désynchronisation ne couvre pas cette forge fraîche.
2. Le pipeline fabrique lui-même le reçu depuis `smax`, `n` et `status==kOk`.
   La prétention de fermeture reste donc l'ancienne condition `smax>=n`, sous un
   autre type; elle n'est pas prouvée par un producteur terminal.
3. Le sidecar est détruit avant l'appel du fold, qui reçoit encore les points et
   le `Catalogue` bruts. Le moteur ne consomme donc pas la frontière typée.
4. `ExactBallKey` calcule le carré de numérateurs pouvant atteindre environ
   90 bits dans un entier signé de 128 bits. La clé de niveau peut déborder sur
   le domaine u16 valide et doit être multiprécision.
5. La factory recopie `sphere.n_support` comme `q_min`; elle ne reconstruit pas
   un support propre positif canonique et ne compare pas le champ
   `sphere.sph.support`. Les supports redondants ou affinement dépendants ne sont
   donc pas exclus par le contrat annoncé.
6. Le digest FNV-1a 64 bits hache l'image mémoire brute de `CriticalSphere`, y
   compris padding ABI et projection `double`. Il n'est ni une sérialisation
   canonique ni un engagement exact.
7. Le census refait un scan `O(G*n)` et la miniboule des membres pour chaque
   générateur. Cette factory peut rester un juge borné; elle n'est pas une route
   compatible avec le contrat 50 k sous la seconde.

## Portes de réception

- rendre le reçu inconstructible hors d'un producteur terminal dont la
  complétude est rejouable, et graver `fresh_receipt_on_amputated_catalogue` ;
- transmettre `const ValidatedHybridSidecar&` jusqu'au fold sans retour à un
  catalogue brut ni à une prétention booléenne ;
- construire la `BallKey` et ses comparaisons en multiprécision canonique ;
- recalculer le support propre positif, `q_min` et tous les invariants de
  support avec des fixtures redondantes, dépendantes et incohérentes ;
- remplacer l'image mémoire FNV par une sérialisation contractuelle canonique
  et SHA-256 ;
- conserver la revalidation exhaustive comme oracle borné, tandis que le
  chemin produit consomme des certificats streamés et rejouables.

Résultat annoncé par la livraison : `57/57` CTests de sa région. Ce nombre est
un reçu de non-régression sur les fixtures existantes; il ne couvre aucun des
contre-exemples ci-dessus et ne change pas le verdict.

GCP non utilisé.
