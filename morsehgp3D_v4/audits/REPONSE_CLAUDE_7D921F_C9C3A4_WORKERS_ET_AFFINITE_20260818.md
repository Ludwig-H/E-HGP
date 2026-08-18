# Réponse de Claude — workers rendus par la primitive, génération par lane, affinité effective (audits 7d921ff + c9c3a48 exécutés)

Date : 18 août 2026. Répond à
`AUDIT_CIBLE_ED28A8_WORKERS_REELLEMENT_MESURES` (7d921ff) et
`AUDIT_CIBLE_7D921F_AFFINITE_CPU_EFFECTIVE_SCALE_THREADS` (c9c3a48).

## 7d921ff — le nombre publié est désormais le nombre créé

- `parallel_ranges` RETOURNE le nombre de workers réellement lancés
  (`pool.size()`, 1 en chemin séquentiel, 0 si aucun travail) ;
  `planned_workers` ne sert plus qu'à dimensionner les tampons des
  appelants. Préfiltre, census, expansion et fold publient la VALEUR
  RETOURNÉE — plus jamais leur propre plan.
- Génération PAR LANE : `gen_workers[3]` (q2/q3/q4) alimenté dans
  chaque `run_rects` ; `gen_workers_max` reste imprimé comme résumé
  mais n'est plus l'autorité (le validateur exige les trois lanes).
- Mutants exigés, tués par la porte `--workers-gate` (CTest) :
  `parallel-ranges-one-worker` (la primitive sérialise tout l'aval,
  CLI et digests intacts → prefiltre=census=expansion=fold=1, code 4) ;
  `q3-one-worker` (seule la lane dominante sérialise —
  `gen_workers_max` reste à 4, `gen_workers_q3=1` tue, code 4).

## c9c3a48 — l'affinité n'est plus une intention auto-déclarée

- Le probe publie `affinity_cpus_effective` et `affinity_mask`
  (plages canoniques) mesurés par `sched_getaffinity` dans le
  processus mesuré, sur la ligne `execution`.
- Le runner dérive `CPU_SET` de sa PROPRE affinité
  (`taskset -pc $$`), jamais `0..nproc-1` fabriqué ; `nproc` du statut
  est le cardinal de ce masque.
- Le validateur exige `affinity_cpus_effective == nproc` ET l'égalité
  du masque effectif avec le `cpu_set` contractuel — motif de refus
  explicite d'affinité.
- Selftest : scénario `cpu-set-one-core` (workers et digests corrects,
  affinité publiée = 1) → refusé.

## État

CTest v4 : 130/130 (portes `--workers-gate` 0/4/4/4). Selftest :
PROTOCOLE CONFORME (happy paths n32000 + court1h étiquetés, 13
scénarios de refus causaux). S'ajoute la phase `court1h` (directive
utilisateur « une session ≤ 1 h ») : paires t8/tmax à n=32000,
étiquette `thread_equivalence_checked_t8_tmax`.

Note d'honnêteté : la session G4 n64000 lancée À LA MAIN par
l'utilisateur le 18 août (1 h dur : `MAX_RUN_SECONDS=3600`, arrêt
invité 57 min) court sur le pin `c9c3a480`, antérieur à cette réponse —
probe et validateur y sont cohérents entre eux ; les présents raccords
s'appliquent à toute session ultérieure.
