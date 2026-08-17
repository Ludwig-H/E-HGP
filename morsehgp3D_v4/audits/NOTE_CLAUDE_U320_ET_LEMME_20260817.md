# Note de Claude — preuve du lemme versée, ordre sémantique U320 gravé

Date : 17 août 2026. Exécute les points 1 et 3 de
`AUDIT_Q4_LEMME_PREFIXE_ET_NIVEAU_20260817.md` (le point 2 — fixture
renforcée à 22 points avec les deux mutants de source — était déjà poussé,
nos messages se sont croisés : voir `NOTE_CLAUDE_FIXTURE_RENFORCEE`).

## Point 1 — la preuve du lemme du préfixe ternaire est au dossier

Votre preuve barycentrique (`tau = |u+v|²/2`, produit scalaire de la
relation barycentrique par `s = u+v`, contradiction `centre = milieu de
ab`) remplace le statut `theoreme_v3` dans `docs/MATHEMATIQUES.md` § 4.5,
créditée à l'audit. Le maillon que je signalais comme le plus faible de la
chaîne q4 est fermé : la source par `AcuteSeed` est complète, sans aucun
héritage q3.

## Point 3 — U320 et l'égalité sémantique (`q4_event.hpp`)

- `mul_192_128_to_320` (précondition prouvée `< 2^260`), `cmp_u320`,
  `compare_exact_level`, `same_exact_level` — et `same_level_representation`
  qui rend la distinction VISIBLE dans l'API, comme demandé.
  `Q4Level::operator==` porte désormais l'avertissement gravé : égalité de
  représentation, légitime pour l'identité de record et le juge en
  multiensemble, JAMAIS pour les macro-lots.
- `promote_q3_level` : le comparateur U320 ordonne q3 et q4 ensemble ; la
  cohérence U192/U320 est vérifiée paire à paire sur les niveaux q3.
- Porte `mhgp4_q4_level_cmp_accord` : 1 485 niveaux q4 récoltés (uniform
  n=22, équilatéral maximal, grande cosphère), 1 101 870 paires jugées
  contre l'oracle obigint 384 bits, 0 désaccord, antisymétrie ; et
  **282 552 plateaux sémantiques à représentations différentes** — les
  tétraèdres cosphériques partagent la boule avec des `(|N'|², det²)`
  distincts : le plancher garde vivant très exactement le cas que
  `operator==` raterait et que les macro-lots devront grouper.
- Fixture de largeur : produits croisés égaux sur 256 bits, différents au
  seul mot `w[4]` ; mutant `level320-trunc-hi` tué (code 4).

51 portes CTest vertes. Votre ordre restant est donc épuisé jusqu'au
point 4 : la prochaine étape est la **sélection axiale reçue contre la
baseline énumérée et l'oracle q4** — sans changer les records ni les
plateaux de niveau, comme vous l'exigez. J'y vais, sauf contre-ordre.
