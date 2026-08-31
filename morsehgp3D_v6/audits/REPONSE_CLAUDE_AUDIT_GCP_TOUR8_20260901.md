# RÉPONSE_CLAUDE — huitième tour GCP : les trois portes fermées, la sobriété incluse

Date : 1er septembre 2026. Réponse au huitième tour (`526b713b`, resserré
par `44ff709d`). Le commit porteur contient tout ; suites rejouées depuis
son HEAD propre. **Aucune session lancée** — condition de l'exploitant.

## Porte 1 — les deux arrêts budgétés, re-tentative avant toute validation

- `SCP_ATTEMPTS` fixé à **1** (plage validée `[1, 1]`) ;
  `POST_BUDGET_S` réserve **`2 × STOP_RESERVE_S`** : 2 915 s, fenêtre
  nominale 24 295 s, marge G4 **501 s** — vos chiffres.
- **Fonction d'arrêt PARTAGÉE** (`attempt_targeted_stop` : publication des
  états, journal en panne toléré, compteur de tentatives) entre le chemin
  nominal post-scp et le cleanup. Après un premier échec : **re-tentative
  IMMÉDIATE avant toute validation** ; si le second échoue, la validation
  est **sautée** (aucune `campaign_status` publiée), exit 70 ; le cleanup
  **ne crée jamais une troisième tentative** (`STOP_TRIES ≥ 2` → aucune).
- Fixtures : échec-puis-succès → deux arrêts, message « re-tentative
  IMMÉDIATE avant toute validation », `validation.txt` présent, terminal
  `targeted_stopped`, pas de troisième ; double échec → exit 70, deux
  tentatives exactement, **aucun** `validation.txt`, **aucune**
  `campaign_status`.

## Porte 2 — grâce protocolaire fixée à 30 s

`GRACE_S` vaut 30 des deux côtés et **toute surcharge est refusée avant
toute garde** — fixtures 29 et 31 exigeant zéro SETMAX/START/STOP. La
relation avec la queue SSH de +60 s ne peut plus être rouverte par une
surcharge.

## Porte 3 — instrumentation épinglée ou déclassée

`time_bin` est **gravé dans chaque statut** (champ exigé exactement une
fois par le validateur). Le verdict `decision_complete` exige désormais
`time_bin == /usr/bin/time` sur TOUS les runs ; toute autre instrumentation
**déclasse** en `verifie_non_decisionnel` avec la cause exacte
« instrumentation de test (TIME_BIN non épinglé /usr/bin/time) ». Mutant de
bout en bout : le profil **decision_v1 exact** (82 runs, canon réel lié au
manifeste) exécuté sous un faux GNU time → déclassé, jamais
`decision_complete` — votre probe est mort.

## Sobriété (même tour, locale — non conditionnante)

- Relation `GUEST_SHUTDOWN_MINUTES × 60 + 300 ≤ MAX_RUN_SECONDS` testée
  **avant `set_max_run_duration_and_verify.sh`** (fixture : zéro SETMAX —
  plus de reconfiguration mutante d'une cible qui sera refusée ensuite).
- `too_late()` unique (`/usr/bin/date`, `now` lu dedans) devant **chaque
  opération initiale** : describe, handshake boot_id, envoi du bundle — la
  fixture « génération passée » exige désormais **zéro SSH/SCP** (exit 77,
  un arrêt ciblé).

## Rejeu depuis le HEAD propre du commit livré

- `selftest_campagne_v6.sh` : **66 vérifications**, code 0 (dont
  decision_v1 + faux TIME_BIN déclassé) ;
- `selftest_cycle_vie_v6.sh` : **32 scénarios + 11 refus de pin**, code 0
  (dont grâce 29/31, relation invité/GCE, STOP1<STOP2<VALIDATE, double
  échec sans validation, trop-tard sans SSH/SCP) ;
- `tests.gcp` : **83/83** ; `git diff --check` propre.

Demande : **verdict frais sur le commit porteur** — puis, sur votre GO et
celui de l'exploitant, lancement de `g4_mesure_v1`.
