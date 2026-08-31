# Réponse Claude au troisième tour de l'audit GCP — les deux P0 et les P1 exécutés, preuves rejouées depuis un HEAD propre

Cadre : `phase=exploration_v6_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.
GCP non utilisé ; le NO-GO reste respecté — rien ne sera lancé avant votre
audit statique frais concluant GO.

## P0 — point d'entrée et étage 2 : FAIT

- **L'entrée documentée fonctionne hors du dépôt** : l'étage 1 accepte une
  racine EXPLICITE (`MHGP6_BOOTSTRAP_REPO_ROOT`), la canonise (`pwd -P`), la
  valide (`git -C … rev-parse --git-dir`), normalise le commit imposé en
  hash complet RÉSOLU (`rev-parse --verify …^{commit}`), et tous les accès
  git emploient `git -C`. La documentation d'en-tête donne la commande
  exacte. Le selftest EXÉCUTE réellement cette commande : bootstrap
  matérialisé depuis le commit du clone vers `/tmp`, lancé depuis `/`, PATH
  empoisonné par le faux gcloud (zéro contact GCP possible) — la chaîne
  atteint l'étage 2 ré-authentifié et le pin (`source_commit=` imprimé),
  échoue plus loin sur le faux gcloud, sans aucun arrêt.
- **L'étage 2 n'est plus forgeable** : il refuse le marqueur hérité quand
  `$0` n'est pas exactement le `bootstrap.sh` du répertoire privé
  (`readlink -f`), puis RÉ-AUTHENTIFIE son propre contenu ET le pin du
  répertoire privé contre `git show` du commit — un WORK forgé par
  l'appelant est refusé même si le marqueur est posé. Scénario gravé :
  entrée directe avec `MHGP6_BOOTSTRAP_STAGE2=1` + WORK arbitraire →
  « entree directe en etage 2 » (code 2).

## P0 — registre partagé jusqu'au terminal : FAIT

- **Création initiale réellement exclusive** : `O_CREAT | O_EXCL` (plus de
  test-puis-replace) dans le garde.
- **Le cleanup extérieur publie ses transitions** (même schéma, même patron
  atomique fsync fichier + parent) : `targeted_stopping` avant l'arrêt
  nominal, puis `targeted_stopped` / `targeted_stop_failed` selon le
  résultat ; le reçu EXIGE la cohérence `stop_rc=0 ⟺ terminal
  targeted_stopped` (incohérence → rc 66, jamais avalée).
- **Reprise bornée** : nommée ainsi partout ; scénario gravé
  (`stop_failed_by_guard` : l'arrêt interne du garde a échoué → UNE
  retentative extérieure ciblée, terminal `targeted_stopped`).
- **Fixture d'INTÉGRATION réelle** :
  `tests/gcp/test_v6_lifecycle_integration.py` compose le VRAI
  `start_and_verify.sh` (via le faux gcloud du harnais de sûreté, scénario
  `guest-success`, générations figées), le VRAI cycle de vie et un FAUX
  `stop_and_verify.sh` compteur. Résultat prouvé : le vrai garde publie le
  registre (`targeted_running` + génération), l'échec ultérieur déclenche le
  cleanup extérieur qui atteint le TERMINAL `targeted_stopped` avec
  EXACTEMENT UN appel au stop portant la génération exacte, et le reçu
  durable grave `stop_rc=0` + `etat_cycle_vie=targeted_stopped`.

## P1 — pris

- **Profil lié** : le validateur (9 arguments) exige `profil_canonique` +
  `profil_canonique_sha256` et RECOUPE ce sha contre le fichier canonique
  fourni ; seul `profil == canonique == decision_v1` rend
  `campaign_status=decision_complete` — tout autre profil valide rend
  `verifie_non_decisionnel` (le selftest l'atteste sur son profil réduit,
  et une prétention `decision` sur un canon non-décision ne rend jamais
  `decision_complete`). `RUN_TIMEOUT`, `THREADS_VM`, `V5_GATE_MIN`,
  `V6_GATE_MIN` sont désormais des axes du profil canonique — toute
  surcharge dégrade en `custom` — et sont écrits dans
  `profil_campagne.txt`.
- **Reçu durable unique et atomique** : identifiant de run avec la
  GÉNÉRATION (`<prefix>_<gen_epoch>`), dossier préexistant refusé,
  construction en `.partial` puis publication atomique, `SHA256SUMS`
  canonique (find trié, SHA256SUMS exclu) couvrant sorties brutes
  rapatriées (`out/` archivé), verdict de validation (`validation.txt`),
  profil et manifeste revalidé.
- **`CLOUDSDK_CONFIG` TOUJOURS privé** : répertoire créé dans tous les cas,
  source copiée si elle existe (héritée ou défaut), export inconditionnel.
- **Runner** : `.txt` et `.status.time` publiés atomiquement ;
  `MANIFESTE_DISTANT.txt` (sha256 de chaque artefact, gravé en dernier)
  recoupé par le validateur après rapatriement — la corruption scp est
  tuée. Le validateur refuse liens symboliques et répertoires dans `out/`.
- **Signature de déterminisme étendue** aux monnaies de décision : vwspd,
  octaves_q4, octaves_q4_seeds, vcensus, p_factor, ledger_paires, ouvriers,
  en plus de famille/génération/sweep/cardinalités.
- **K11 surnuméraire** : porte `mhgp6_juge_refus_k11` (fixture aux digests
  réels + `digest_forest_K11` — le chargeur refuse, code 2).

## Rejeux depuis le HEAD propre `320299df`

- `selftest_cycle_vie_v6.sh` : 29 verts (15 scénarios de cycle de vie dont
  reprise bornée, entrée `/tmp` hors dépôt, entrée directe refusée ; 10
  refus de pin ; + préalable pin propre) — code 0 ;
- `selftest_campagne_v6.sh` : 26 verts — code 0 ;
- `tests/gcp/test_v6_lifecycle_integration.py` : OK ;
- `tests/gcp/test_gcp_safety.py` : 81/81.

Votre porte de réouverture : les points 1–4 sont livrés avec leurs tests ;
reste le point 5, votre GO. Aucun démarrage facturable avant.
