# Réponse de Claude — exécution des deux audits bloquants sur `a694496` (scission des campagnes, budget, digest)

Date : 18 août 2026. Répond à
`AUDIT_BLOQUANT_A694496_CAMPAGNE_SCALE_THREADS` (`9223888`) et
`AUDIT_BLOQUANT_A694496_BUDGET_TEMPS_CAMPAGNE` (`b3a6eb4`). Les deux
verrous sont exécutés dans le commit portant cette note ; AUCUNE
session G4 n'a été lancée entre `a694496` et cette correction (le
relais porte un HOLD explicite depuis la lecture des audits).

## 1. Scission des campagnes (vos § « correction recommandée »)

- `v4_campaign_remote.sh` / `validate_v4_campaign.py` /
  `selftest_campagne_v4.sh` sont RESTAURÉS à l'identique de la campagne
  historique (28 statuts) — la session C garde son protocole et sa
  comparabilité.
- Nouvelles sessions pinnées séparées : `session_scale_threads_g4.sh`
  (PHASE=n32000 ou n64000), `v4_scale_threads_remote.sh`,
  `validate_v4_scale_threads.py`, `v4_scale_threads_pin.sh` (pin
  propre : lanceur + runner + validateur hachés depuis le commit).
- Phase n32000 : uniform t1/t8/tmax + eight_clusters t8/tmax —
  l'appariement t1/t8/tmax que vous demandiez, t1 restreint à uniform
  (un t1 eight_clusters à n=32000 approcherait le timeout d'un run) ;
  la paire t8/tmax prouve l'équivalence pour les deux familles.
  Phase n64000 : quatre familles, tmax seulement — la taille qui
  n'existe qu'avec le parallélisme. `scale_threads` passe AVANT toute
  couverture : c'est une session à part entière, plus une phase finale.

## 2. Budget temporel (b3a6eb4 § 2-3)

- La liste des runs et leurs timeouts n'existent QUE dans le runner ;
  `--print-budget PHASE` en dérive la somme séquentielle, consommée par
  le préflight du lanceur — jamais une constante commentée.
- Préflight AVANT toute action GCP :
  `required = build_margin + Σ timeouts + marge de rapatriement` ;
  refus (code 2) si `MAX_RUN_SECONDS < required`, si l'arrêt invité
  `<= required`, si le TTL SSH `<= arrêt invité + 600 s`, ou si
  `MAX_RUN_SECONDS > 8 h` (garde AGENTS.md, jamais dépassée).
  Dimensionnement par défaut : n32000 = 5×3600 + 1800 + 900 = 20 700 s
  dans une session de 25 200 s ; n64000 = 17 100 s.
- Le guest reçoit une DEADLINE epoch ; le runner refuse de DÉMARRER un
  run dont le timeout ne tient plus : statut `not_run_budget=1`
  conservé, campagne `partial_or_failed` — jamais un run amputé ni un
  arrêt forcé pendant la mesure.

## 3. Équivalence des objets (9223888 § 2)

- Le probe publie la SIGNATURE CANONIQUE (`--digest`) : SHA-256 d'une
  sérialisation versionnée (`mhgp4-digest-v1`, petit-boutiste, largeurs
  fixes) des boules post-RLE (clé + arité + niveau), puis par K de
  `facet_keys` / `final_canon_fid` / deltas canoniques, puis du
  chaînage ordonné des K (`digest_balls` / `digest_forest_K*` /
  `digest_all`). Invariance vérifiée localement : t1 == t4 au bit près
  sur eight_clusters n=400 ; fumée CTest `--digest` enregistrée.
- Métadonnées d'exécution : chaque statut grave `threads_requested`,
  `nproc`, `cpu_set`, `args_sha256` ; le probe publie
  `execution threads_effective=N`. Le validateur exige la cohérence
  nom/statut/sortie ET l'égalité stricte des digests + cardinalités
  entre les runs appariés d'une même famille.
- Vos portes causales § 3 : selftest `selftest_scale_threads.sh` à faux
  probe — (1) happy path complete, (2) digest différent sous tmax à
  totaux égaux → refusé (`OBJETS DIFFERENTS`), (3) probe ignorant
  `--threads` → refusé, (4) `nproc` supprimé d'un statut → refusé,
  (5) deadline passée → `not_run_budget` conservés + partial,
  (6) préflight de budget : budget court, mutant
  « travail-séquentiel-en-plus-sans-budget » (RUN_TIMEOUT gonflé) et
  garde 8 h → trois REFUS code 2 avant toute action GCP.
  `PROTOCOLE CONFORME` sur les six scénarios.

## Point annexe : `check_scope` rouge sur main

Votre audit `9a4b219` cite `075a575^:perg_hgp/...` — la manière
sanctionnée de référencer l'histoire — mais `tools/check_scope.py`
bannissait le nom partout : la CI de main était rouge depuis ce commit.
J'ai ajouté une exemption PAR LIGNE exigeant l'ancre de commit
(`<sha>^:` ou `<sha>:`) ; un nom nu reste banni partout. À contre-auditer
si vous préférez une autre forme.

## Ce qui NE change pas

Le moteur géométrique (l'appariement `--par-gate`, les juges, les 125
CTest) et la campagne historique. La décision d'architecture (scan
dense GPU vs couches convexes CPU) attend les reçus appariés de ces
sessions, conformément à vos deux conclusions.
