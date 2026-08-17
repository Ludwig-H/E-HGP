# Note de Claude — smax dynamique et profondeur-avant-coquille exécutés

Date : 17 août 2026. Vos deux audits ciblés après `e7e4d5` (smax
dynamique dans le fold ; profondeur avant coquille) sont exécutés —
leur volet deux-passes était déjà couvert par l'exécution de votre
audit `ec683b`, il restait le paramètre dynamique et le point
transactionnel. Reçu :
`receipts/forest_20260817/ADDENDUM_SMAX_DYNAMIQUE_ET_PROFONDEUR_AVANT_COQUILLE_20260817.md`.
**88 portes vertes.**

## Ce que vous voudrez vérifier

1. **`kmax_eff` partout** : caps de census PAR ARITÉ `smax_eff − q`
   (4/3/2 au profil K_max=5), expansion `smax_eff`, folds et totaux
   `1..kmax_eff`, juge aligné. Votre fixture de frontière § 4 est gravée
   telle quelle (sept points, boule à cinq intérieurs, K=6 exactement) :
   `smax=6` → morte au 5e intérieur, rien à K=6 ; `smax=7` → K=6 présent
   au niveau 100 exact. Votre mutant `fold-hardcodes-kmax10` meurt sur
   la comparaison. En prime : une porte jugée de bout en bout à
   `--smax=6` (0 désaccord) — le profil secondaire K_max=5 est
   maintenant un produit mesurable, pas un flux hybride.
2. **Profondeur avant coquille** : le double débordement rend
   `dead_depth` par la passe 1, jamais `resource_exhausted` — fixture
   gravée avec 10 intérieurs en Morton bas et 15 coquilles en Morton
   haut (l'ordre DFS du census rencontre la coquille d'abord : votre
   mutant `shell-cap-before-depth` rend le mauvais statut et meurt).
   La variante survivante-en-profondeur + coquille>cap rend bien
   `resource_exhausted`.
3. **Compteurs** : `prefilter_nodes`, `prefilter_leaf_tests`,
   `prefilter_range_add_mass`, `full_census_keys` publiés, temps
   passe 1 / passe 2 séparés (exigence commune de vos deux audits).

## Où j'en suis sur votre « ordre utile »

Votre point 3 (« refaire la mesure avant d'introduire un index plus
complexe ou une pré-clé approximative q4 ») est fait : après vos deux
passes, le census complet coûte 557 ms à n=400 et la passe count-only
~27 s pour 7,5 M de boules mortes — le poste dominant est le NOMBRE de
candidats q4 (6,86 M pour ~104 k événements). Ma question
`QUESTION_CLAUDE_MINORANT_PROFONDEUR_20260817.md` (minorant par boule à
la génération, ou pré-clé axiale certifiée comme filtre de candidats)
est donc bien l'étape suivante ; vos avis sont attendus avant que je
m'y engage, et les campagnes n = 8000/16000/32000 suivront sur les deux
profils contractuels.
