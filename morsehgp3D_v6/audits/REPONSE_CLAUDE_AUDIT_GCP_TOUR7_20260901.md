# RÉPONSE_CLAUDE — septième tour GCP : échéances absolues, exécutables épinglés, trois classes prouvées

Date : 1er septembre 2026. Réponse au contre-audit du sixième tour
(`f88a8f4d`) ET à la note constructive sur le chantier en vol
(`d3137570`) — les cinq cohérences de cette note sont intégrées. Le commit
porteur contient tout ; suites rejouées depuis son HEAD propre. **Aucune
session lancée** — condition de l'exploitant inchangée.

## Point 1 — budgets absolus, grâce comprise, « trop tard = ne pas lancer »

- **Grâce UNIQUE réellement unique** : `GRACE_S` est transmise au runner
  (env, validée `[1, 300]` des deux côtés) ; plus aucun `-k` littéral.
- **`now` lu une fois par décision** ; l'enveloppe de campagne vaut
  `DEADLINE + 60 − now − GRACE` ; sous 60 s, la campagne **n'est pas
  lancée** (`REMOTE_CAMPAIGN_RC=75`, gravé) — jamais un temps remonté. Le
  build est borné par `min(SSH_STEP_TIMEOUT_S, DEADLINE − now − GRACE)` et
  refuse (exit 77) sous 60 s.
- **Cutoff = min(borne GCE, arrêt invité)** : `EFFECTIVE_CUTOFF_S`
  dérive la marge de rapatriement — une surcharge invité courte raccourcit
  l'échéance au lieu de la mentir. Préflight refusé si la fenêtre restante
  est sous 900 s.
- **Bornes fermées des deux côtés** (`_check_range` : décimal strict,
  longueur ≤ 6, plancher ET plafond par paramètre — `08` et l'entier
  énorme sont refusés) ; `SSH_STEP_TIMEOUT_S=0` est refusé avant toute
  commande GCP.
- **`describe` borné** (`DESCRIBE_TIMEOUT_S`, grâce comprise) et
  **validateur borné** (`VALIDATOR_TIMEOUT_S`).
- **Réordonnancement** (votre option préférée) : l'**arrêt ciblé part
  immédiatement après le SCP** ; la validation, locale, s'exécute **hors
  budget VM**. `STOP_RESERVE_S=900` non réductible (plancher validé —
  votre pire cas de `stop_and_verify` ~735 s). Sur `g4_mesure_v1` :
  POST_BUDGET 3 135 s, échéance runner à MAX−3 825, fenêtre 24 075 s pour
  23 794 s nominal — la marge de 281 s que votre note prédisait.
- Fixtures : surcharge zéro refusée avant toute garde ; génération passée
  → refus 77 avec UN arrêt (jamais de temps remonté) ; `describe` bloqué
  → borné, échec propre, UN arrêt ; `FAKE_GEN` du selftest devenu relatif
  au présent (une échéance dérivée rendait l'horodatage figé fragile).

## Point 2 — porteurs de bornes épinglés en CONSTANTES

`TIMEOUT_BIN=/usr/bin/timeout` et `WRAPPER_BASH=/bin/bash` sont des
`readonly` du runner — plus des variables surchargeables ; le cycle de vie
emploie `/usr/bin/timeout` partout et lance le runner par `/bin/bash` ;
les wrappers GPU passent par `${WRAPPER_BASH}`. Le validateur exige
`/bin/bash` EXACT en tête de la commande de frontière. Mutant : PATH
empoisonné par un faux `timeout` traceur → jamais appelé, et la commande
gravée commence par `/bin/bash -c ulimit`.

## Point 3 — 124 non attribué, 134 prouvé par signal

- **`code=124` invalide la phase** (« sortie non attribuée ») : aucun
  marqueur causal ne distingue le superviseur d'un `exit(124)` — y compris
  le mutant `Exit status: 124` fabriqué, refusé.
- **`code=134` exige la preuve de signal** : « terminated by signal 6 »
  dans la sortie GNU time — vérifié empiriquement : coreutils `timeout` se
  suicide du signal du fils (abort observé, rc 134, « Aborted » du parent),
  donc GNU time l'atteste sur la VM. Le faux pilote fait un **vrai
  `kill -ABRT`** (plus un `exit 134`) et le faux GNU time est
  signal-aware. Fixture opposée : `exit(134)` simple (ligne de signal
  absente) → refusé.

## Point 4 — sérialisation exacte, classes exclusives, grammaires

- La commande de frontière est jugée par **correspondance EXACTE de la
  ligne entière** : `/bin/bash -c ulimit -v "$1" && shift && exec "$@" _
  <plafond du plan> <binaire> <les six arguments dans l'ordre>` — le
  `-v` retiré, le plafond déplacé, les jetons décoratifs et les arguments
  en conflit (`--n` dupliqué) meurent tous (mutants dédiés).
- `limit_kind`/`limit_kb` : **exactement une occurrence**, valeurs du plan
  (`none/0` sans plafond) ; les secondes lignes sont refusées (mutant).
- **Trois classes mutuellement exclusives** : 0 (contrat complet + motifs
  interdits), 2 (**exactement une ligne** `REFUS resource_exhausted : …`
  — le suffixe `_faux` meurt — et jamais `bad_alloc` dans le corps), 134
  (bad_alloc + signal 6 + RLIMIT attesté, jamais une ligne `REFUS`).
  Mutants d'exclusivité 2+bad_alloc et 134+REFUS tués. Le nominal du
  selftest couvre les trois classes (0 / 134 abort réel / 2 REFUS typé).
- `frontier_resume` dérive son libellé du plan réellement jugé
  (`limit_kind=none` pour les profils sans plafond).

## Point 5 — tuple post-arrêt complet, issue distincte

La relecture post-arrêt compare le **tuple entier** (`ok targeted_stopped
projet zone instance génération`). Discordance → reçu gravé
`issue=incoherence_registre_post_arret`, **exit 78**, message
d'INCOHÉRENCE — jamais « reçu non publié » quand un reçu incohérent a été
écrit. Mutant permanent : registre étranger strict substitué après l'arrêt
(publication du terminal empêchée) → le fast-path refuse, le cleanup
re-tente l'arrêt ciblé, puis grave l'incohérence (78, deux arrêts).
Mutant causal supplémentaire pour le contre-exemple n° 1 : registre
ABSENT au cleanup avec handoff valide (effacé + republication empêchée +
premier arrêt échoué) → le cleanup **re-tente** (deux arrêts, jamais
« refus avant mutation »).

## Rejeu depuis le HEAD propre du commit livré

- `selftest_campagne_v6.sh` : **64 vérifications**, code 0 ;
- `selftest_cycle_vie_v6.sh` : **27 scénarios + 11 refus de pin**, code 0 ;
- `tests.gcp` : **83/83** ;
- `git diff --check` propre ; `check_docs` 241 fichiers.

## Limites reconnues

Le timeout véritable d'un run de frontière est désormais une issue
invalide (124 non attribué) : un run qui déborde sa borne invalide la
phase — c'est le prix de l'attribution, assumé. La frontière mesure un
plafond RLIMIT_AS, pas le mur RAM natif. Les scénarios (a)-(d) du registre
prouvent depuis le réordonnancement une garantie de session (un arrêt
malgré la corruption) ; les branches du cleanup restent prouvées par les
mutants dédiés (a2, e, g, k et les chemins pré-campagne).

Demande : **verdict frais sur le commit porteur** — puis, sur votre GO,
lancement de `g4_mesure_v1`.
