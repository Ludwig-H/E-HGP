# Note de Claude — sélection axiale reçue, et une mesure qui tranche

Date : 17 août 2026. Clôt le point 4 de votre ordre (« recevoir la
sélection axiale contre la baseline énumérée et l'oracle q4, sans changer
les records ni les plateaux »). Reçu complet avec chiffres :
`receipts/q4_events_20260817/ADDENDUM_SELECTION_AXIALE_20260817.md` ;
mathématique : `docs/MATHEMATIQUES.md` § 4.6.

## Reçu

- Re-dérivation v4 complète de la piste v3 : faisceau
  `Phi(z; mu) = P_3(z) − mu·pi(z)`, test intérieur exact par produits
  croisés `< 2^161` (U192 signé), borne de rang
  `depth >= p + prédécesseurs_stricts` — au plus `2(h_4 − p)` groupes par
  seed, minorant fail-open.
- Trois portes jugées (`fixture23`, uniform, clusters) : records
  IDENTIQUES à la baseline, juge brut 0/0.
- Fixture de justesse gravée (`fixture_tight20`) : un événement de
  profondeur EXACTEMENT `h_4 − 1 = 7` dont les sept intérieurs sont du
  même côté que la complétion — la coupe saine le garde, le mutant
  `axial-rank-cut` (un groupe de moins) le perd (code 4) : la borne est
  serrée au groupe près.

## La mesure qui tranche

À uniform n=400 : 7,9× moins de tests de puissance, records identiques —
et pourtant `t_instruction` passe de 6,8 s à 31,7 s. Le tri exact des
`mu` par seed (comparateur U192 signé, ~3·10^8 comparaisons) coûte plus
que tout ce que la coupe économise. La borne atteint son maximum
théorique (~15,8 candidats/seed) : ce n'est pas un défaut d'implémentation
mais un vrai verdict de compteurs — le « 59× moins de propositions » v3
était un compte, pas un temps.

## Arbitrage proposé (je pars sur (c) sauf contre-ordre)

- (a) certifier une PRÉ-CLÉ approchée (flottante/tronquée) avec arbitre
  exact aux frontières — demande une analyse d'erreur fail-open sérieuse ;
- (b) réserver la sélection axiale au port GPU, où le budget STATIQUE de
  16 candidats par seed est précisément ce qu'un warp veut (c'est à mes
  yeux sa vraie valeur, conservée par ce reçu) ;
- (c) garder `--axial` reçu mais optionnel (baseline par défaut), et
  passer à la FORÊT — vos macro-lots `same_exact_level`, les cartes
  verticales, `F_K^conn`/`F_K^render` — qui est le dernier grand morceau
  manquant de l'objet.

57 portes CTest vertes, tout est poussé sur main.
