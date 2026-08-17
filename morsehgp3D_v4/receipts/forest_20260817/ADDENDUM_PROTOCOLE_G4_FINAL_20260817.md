# Addendum — protocole G4 final : pin de source, rapatriement toujours, porte à faux probe

Date : 17 août 2026. Exécution des deux derniers audits de protocole
(`AUDIT_CIBLE_3792D56_PIN_SOURCE_ET_RSS_G4` et
`AUDIT_CIBLE_E265F6_RAPATRIEMENT_APRES_RUPTURE_SSH`). Le lanceur est
factorisé en trois fichiers TESTABLES + une porte :

```text
session_campagne_v4_scale_g4.sh   gardes GCP, pin, transfert, scp, verdict
v4_campaign_remote.sh             les 28 runs (VM — ou selftest local)
validate_v4_campaign.py           SEULE autorité de campaign_status
selftest_campagne_v4.sh           la porte transactionnelle à faux probe
```

## 1. Pin de source (audit « pin source » § 1)

Refus AVANT toute mutation GCP si `morsehgp3D_v4` diffère de HEAD
(index, worktree, non-suivis) ; payload produit par `git archive` depuis
le COMMIT — l'état du disque n'entre jamais dans une campagne
contractuelle (les reçus non commités non plus : choix sain de l'audit) ;
`sha256sum -c` vérifié sur la VM avant le build ; le couple
`(source_commit, source_tar_sha256)` est journalisé avant démarrage,
gravé dans CHAQUE `.status` et exigé identique par le validateur. Le
`git -C` distant (chemin local injecté, erreur avalée) est supprimé.

## 2. GNU time obligatoire (audit « pin source » § 2)

`test -x /usr/bin/time` sur la VM au build, AVANT les pilotes — refus
sinon. Le repli VmHWM est SUPPRIMÉ du protocole de campagne (il mesurait
le wrapper `timeout` : le mécanisme anti-OOM aurait autorisé la
concurrence excessive qu'il devait prévenir). La leçon locale reste au
reçu RSS ; cgroup `memory.peak` noté pour une industrialisation future.

## 3. Rapatriement toujours (audit « rupture SSH » §§ 1-2)

Le retour SSH de campagne est capturé par `set +e`/`PIPESTATUS` sans
déclencher le trap ; le `scp` est ensuite tenté TROIS fois (reprises
espacées) quoi qu'ait rendu la session distante ; le trap n'éteint la
VM qu'après. `remote_campaign_rc` et `scp_rc` entrent dans la
validation : `complete` exige les deux à zéro — une rupture SSH ne peut
plus éteindre la machine avant la récolte, ni une campagne partielle se
présenter en répertoire local vide.

## 4. Pilotes validés avant toute vague (audit « rupture SSH » § 3)

`pilot_ok` exige `code=0`, `finished=1`, `timing_scope=isolated_latency`
et un pic RSS NUMÉRIQUE > 0 pour chacun des trois pilotes ; sinon la
couverture est REFUSÉE (exit 3, statuts conservés, rapatriés, verdict
partial) — jamais une concurrence dérivée d'un processus avorté.

## 5. La porte à faux probe (audit « rupture SSH » § 4, « pin source » § 3)

`selftest_campagne_v4.sh` exécute LE MÊME script distant et LE MÊME
validateur avec un faux probe et un faux GNU time (le protocole seul est
attesté ; la mesure réelle du pic est déléguée à GNU time, exigé sur la
VM). Quatre scénarios : happy path (28 statuts, pin partout, verdict
`complete`) ; échec code 7 + timeout code 124 MATÉRIALISÉS puis LISTÉS
par le verdict partial ; pilote avorté → couverture refusée, statuts
conservés ; `remote_rc != 0` → jamais complete. La porte a immédiatement
payé : elle a attrapé un `taskset -c` hors bornes sur petit poste
(cœurs bornés par `nproc` — comportement inchangé sur la G4).

`PROTOCOLE CONFORME` (violations=0). Les auditeurs ne voient « plus de
raison d'empêcher la campagne G4 » ; le prochain chantier de code est le
fold sort/reduce (112 s sur 324 à n=8000, pente ×2,8), en parallèle de
la campagne que l'opérateur lancera.
