# Reçu — livraison J0–J2 de MorseHGP3D v6 (31 août 2026)

Cadre : `phase=exploration_v6_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `public_status=not_claimed`.
Ancrage : le commit qui porte ce reçu (sources v6 complètes) ; référence
différentielle v5 au pin `3bad233d` (binaire v5 sha256 `945c9a7f…`,
digests gravés dans `../conformite_v5/`, statut `baseline_v5_capture`).
Hashes des binaires v6 jugés : `BINAIRES.txt`. Machine : codespace 8 vCPU —
les temps de cette exécution ne sont PAS des mesures (suites concurrentes) ;
seuls les codes et les digests font foi.

## Ce qui est livré

- Pipeline v6 complet : index radix → descente WSPD FUSIONNÉE à masques de
  lanes (une descente pour q2/q3/q4, grand-livre global des masses de paires
  par lane) → histogrammes/tueurs d'ancre (W_q exact, secteurs corrigés,
  grille 10.5) → lanes q2/q3 (v5) et q4 avec SWEEP DE CORDE UNIFIÉ
  (mutualisation de la profondeur par seed ; l'incidence seed–complétion
  reste payée et publiée) → RLE → préfiltre exact → census → événements →
  fold streamé par K → digests format v4.
- Familles stationnaires `terrain_stationnaire` / `scanline_stationnaire`
  (régimes synthétiques stationnaires, physiquement motivés ; racine entière
  à arrondi explicite, c0 = 447/566).
- 38 portes CTest (codes exacts 0–4, planchers, mutants, équivariance).
- Réponse à l'audit J0 : `../../audits/REPONSE_CLAUDE_AUDIT_J0_20260831.md`
  (quatre P0 fermés, P1 provenance/familles appliqués).

## Résultats des portes (binaire final, sorties brutes jointes)

- Portes rapides (`-LE scale*`) : **23/23** (`ctest_gate_final.txt`), dont
  `mhgp6_fused_mutant_mask` code 4 (deux détecteurs causaux : invariant
  `core < h` des lanes émises + plancher de masse tuée de l'auditeur),
  `mhgp6_sweep_oracle` (énumération exhaustive des supports, 7 cas dont
  `two_lines` et `collinear_seven`), `mhgp6_sweep_mutant_exit` et
  `mhgp6_conformity_mutant_rle` code 4, refus CLI (parsing exact).
- Conformité v5 ≡ v6 aux tailles d'intérêt : **15/15**
  (`ctest_scale_final.txt`) — pour chacune des 5 familles dilatées ×
  {8000, 16000, 32000}, graine 3, s=8, smax=11 : `digest_balls`, les dix
  `digest_forest_K*` et `digest_all` sont IDENTIQUES aux valeurs v5 gravées.
  Le multiensemble de candidats émis est identique par construction (le sweep
  est une transformation de coût : profondeur au point de racine = filtre v5
  par l'identité affine du th. 10.4 ; racine strictement hors corde ⟺
  tétraèdre non bien centré, rejet exact).
- Petites tailles : conformité également verte sur uniform/terrain 2000,
  400 × 4 familles, collinear_seven 600 (smax effectif 9), two_lines 200.

## Ce que ce reçu ne dit PAS

Aucune mesure de temps ni de pente (les campagnes stationnaires à trois
graines et pentes par terme sont le jalon J3) ; aucun claim de
sous-quadraticité ; aucune exactitude HGP (l'égalité aux digests v5 prouve
« même objet que la v5 », la sémantique reste
`forest_semantics=verified_events_only`) ; aucun résultat GPU. GCP non
utilisé.
