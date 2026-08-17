# Note de Claude — le fold sort/reduce est en place, le protocole G4 est clos

Date : 17 août 2026. Deux fermetures dans ce cycle :

1. **Vos trois audits de protocole G4 sont exécutés** (transactionnel ;
   pin de source + GNU time obligatoire ; rapatriement après rupture
   SSH) — le lanceur est factorisé en trois fichiers testables et la
   porte à faux probe `selftest_campagne_v4.sh` rend PROTOCOLE CONFORME
   sur vos quatre scénarios (elle a d'ailleurs immédiatement attrapé un
   `taskset` hors bornes sur petit poste). Reçu :
   `ADDENDUM_PROTOCOLE_G4_FINAL_20260817.md`. La campagne attend le
   lancement opérateur.
2. **Le fold sort/reduce que vous attendiez en parallèle est fait** :
   internement global par tri + rôles en tableaux à époque
   (`build_forest`), agrégation triée (`build_render`) — sorties
   bit-identiques (mêmes événements/fusions/nœuds, selftest 0 désaccord,
   9 mutants toujours tués), t_fold −42 % à n=1600 et **−49 % à n=8000**
   (112,0 → 56,6 s), gain croissant avec n. Reçu :
   `ADDENDUM_FOLD_SORT_REDUCE_20260817.md`. Les petits maps par racine
   de lot restent (jamais porteurs de la pente) ; la parallélisation par
   K (dix forêts indépendantes) est notée pour la G4.

État : **93 portes vertes**, tout est sur main. Tâche dormante : la
boule intérieure candidate `B(m, R−δ)` — je la rouvrirai si les
compteurs de génération de la campagne G4 la redésignent (t_gen reste
le premier poste : ~52 % à n=8000).
