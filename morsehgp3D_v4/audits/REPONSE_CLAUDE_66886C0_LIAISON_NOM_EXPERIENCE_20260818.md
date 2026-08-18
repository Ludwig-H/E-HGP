# Réponse de Claude — l'étiquette est liée à l'expérience (audit 66886c0 exécuté)

Date : 18 août 2026. Répond à
`AUDIT_CIBLE_DB2F4F_VALIDATEUR_SCALE_THREADS_20260818.md` (66886c0).
Les trois raccords sont fermés AVANT toute session G4, dans l'ordre
conseillé.

## § 1 — Liaison nom → argv → identité imprimée

- Le runner hache désormais une sérialisation SANS ambiguïté :
  `printf '%s\0' "$@"` (chaque argument + NUL, `$@` après le `shift`
  des paramètres de contrôle — donc exactement l'argv du probe).
- Le validateur possède la spécification par nom (famille, n, s=8,
  smax=11, seed=3, fils avec `tmax` = `nproc` du statut) et exige
  SIMULTANÉMENT : (1) `args_sha256` == SHA-256 RECALCULÉ de l'argv
  contractuel reconstruit depuis le nom (`contract_argv`, même ordre,
  mêmes NUL) ; (2) la ligne d'identité du probe
  (`famille=… n=… s=… smax=… seed=…`) conforme au nom ; (3)
  `threads_requested` conforme au suffixe.
- Selftest : scénarios `wrong-n`, `wrong-family`, `wrong-smax` sous le
  bon nom (identité imprimée fausse → refus), et `args_sha256`
  trafiqué dans un statut (→ refus par recalcul, plus jamais par
  simple présence).

## § 2 — Workers mesurés au point de création

- `BallStreamStats` publie `gen_workers_max`, `prefilter_workers`,
  `census_workers`, `expansion_workers`, `fold_workers_max`, tous
  alimentés LÀ où les `std::thread` sont créés (fusion par max), jamais
  recopiés de la CLI. Le probe imprime
  `execution threads_requested=… gen_workers_max=… …` (l'ancienne
  ligne `threads_effective`, qui recopiait la CLI, disparaît).
- Le validateur exige gen/préfiltre/census/expansion == fils demandés
  (tâches abondantes aux tailles de campagne) et
  `fold_workers_max == min(fils, 10)` — le fold plafonne légitimement
  à `K_max`, publié par étage comme demandé.
- MUTANT C++ permanent `parallel-hardcodes-one-worker` : CLI et
  digests inchangés, trahi par la mesure — porte `--workers-gate`
  (0 / mutant 4). Selftest : `FAKE_ONE_WORKER` (CLI et digests
  corrects, `gen_workers_max=1`) → refus.

## § 3 — Schéma complet K=1..10

- Le validateur exige EXACTEMENT une occurrence de `digest_balls`,
  `digest_forest_K1..K10`, `digest_all` et de `cardinalites K=1..10` —
  ni doublon, ni ordre manquant, ni clé inattendue.
- Le faux probe du selftest publie le SCHÉMA COMPLET de production
  (identité dérivée de l'argv, workers, 12 digests, 10 cardinalités) —
  plus jamais une miniature. Scénarios causaux : `omit-digest-K7`,
  `duplicate-digest-K3`, `omit-cardinality-K9` → refusés.
- Porte C++ de SENSIBILITÉ du sérialiseur (`--digest-gate`) : une
  `BallKey` modifiée change `digest_balls` seul ; un `final_canon_fid`
  de K=10 change `digest_forest_K10` ET `digest_all` ; une facette de
  delta de K=3 change `digest_forest_K3` ET `digest_all` — et RIEN
  d'autre à chaque fois (`compute_canonical_digests` factorisé,
  l'impression et la porte consomment le même calcul).

## § 4 — Étiquetage honnête

`campaign_status=complete` porte désormais la revendication exacte :
`thread_equivalence_checked` pour n32000 (appariements t1/t8/tmax),
`digest_recorded_unpaired` pour n64000 (empreinte de reçu, pas une
preuve d'équivalence).

## Directive utilisateur du 18 août (« vérifie qu'aucune VM ne tourne »)

`session_scale_threads_g4.sh` fait un INVENTAIRE LECTURE SEULE du
projet avant toute mutation : toute instance (cible, repli `-ai1a`,
autre) qui n'est pas `TERMINATED` → REFUS code 2 et publication de
l'inventaire — jamais d'arrêt automatique d'une instance qu'une autre
session possède peut-être. `start_and_verify.sh` gardait déjà la seule
cible ; le quota étant dimensionné pour UNE G4 Spot, l'inventaire
couvre le projet entier.

## État

`selftest_scale_threads.sh` : PROTOCOLE CONFORME (happy path étiqueté
+ 12 refus causaux). CTest v4 : 128/128 (nouvelles portes
`--workers-gate` 0/4 et `--digest-gate` 0). La session A (n32000)
reste la prochaine action GCP, sur le pin du commit portant cette
réponse.
