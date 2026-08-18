# Addendum — pin transitif des gardes locales (les octets qui démarrent et arrêtent la VM)

Date : 18 août 2026. Exécute intégralement l'audit **bloquant**
`9d19ede` (« le pin du protocole ne couvre pas encore les gardes
locales exécutées »). Ce reçu précède toute session payante : c'est la
condition posée par l'audit.

## 1. Le trou

`v4_scale_threads_pin.sh` scellait le moteur, le runner et le
validateur. Or `session_scale_threads_g4.sh` lisait et exécutait aussi,
**depuis le worktree courant** :

```text
gcp-migration/set_max_run_duration_and_verify.sh   (coupe-circuit GCE)
gcp-migration/start_and_verify.sh                  (constantes des gardes 5-6,
                                                    conditions de certification,
                                                    démarrage)
gcp-migration/stop_and_verify.sh                   (l'autorité qui permet au trap
                                                    d'annoncer un arrêt certifié)
```

Une modification locale non commitée de l'un d'eux décidait donc du
démarrage et de l'arrêt sans laisser de trace dans le manifeste. Le cas
que l'audit nomme est le bon : un `stop_and_verify.sh` muté qui rend
zéro fait annoncer un arrêt certifié que rien n'atteste. Et même avec
un worktree propre au moment du pin, les helpers étaient relus depuis
le disque **après** le pin et **après** le démarrage — fenêtre TOCTOU
ouverte.

## 2. Ce qui a été fait

**Chemins normatifs et manifeste.** Les trois gardes entrent dans les
trois contrôles de propreté (`git diff`, `git diff --cached`,
`git ls-files --others`), dans le bundle, et dans le manifeste de
protocole. Le manifeste sérialise désormais **chemin + longueur avant
contenu** : une concaténation de contenus laissait deux découpages
différents produire le même digest.

**Matérialisation.** Les trois gardes sont extraites du commit dans
`${WORK}/pinned/gcp-migration/` et le lanceur n'utilise plus qu'elles
(`SET_MAX_RUN_DURATION`, `START_AND_VERIFY`, `STOP_AND_VERIFY`), y
compris pour le `--print-budget` du runner. `start_and_verify.sh`
appelant `stop_and_verify.sh` relativement à son propre `BASH_SOURCE`,
les deux dans le même répertoire pinné ferment aussi le chemin
d'urgence. Le pin publie `pinned_guard_dir` pour que le log dise où.

**Ordre.** Le pin passe AVANT le préflight (§ 3.3 de l'audit : le pin
est local, sans mutation GCP). Les constantes des gardes 5 et 6 sont
donc lues dans le `start_and_verify` **pinné**, jamais dans le
worktree. Conséquence assumée : `--check-envelope` exige maintenant lui
aussi un worktree propre sur les chemins normatifs — un contrôle
d'enveloppe qui lirait des octets hors chaîne de confiance ne vaudrait
rien.

## 3. Deux portes neuves, et la preuve qu'elles mordent

`selftest_scale_threads.sh` construit désormais un **dépôt jetable
depuis HEAD** pour toute invocation du lanceur : hermétique,
déterministe, insensible aux éditions en cours du worktree réel.

- **Scénario 14 — `uncommitted-local-guard`.** Pour chacune des trois
  gardes : une modification non commitée doit être refusée **code 2**
  avec `chemins normatifs modifies dans le worktree` ; la même
  modification mise dans l'**index** doit l'être aussi
  (`… dans l'index`). Six cas.
- **Scénario 15 — `helper-from-worktree-after-pin`.** La copie pinnée
  existe, est exécutable et **égale octet à octet** le commit ; une
  mutation du worktree **postérieure** au pin ne la touche pas ; le
  manifeste dépend bien des gardes (la même mutation, commitée, change
  le digest) ; et le lanceur ne contient plus aucun appel
  `./gcp-migration/{start,stop,set_max_run_duration}_*.sh`.
- **Scénario 13 rejoué** : la mutation de constante doit maintenant
  être **commitée** pour agir (c'est le complément exact du 14) ; la
  fenêtre du TTL dérivé se referme et le refus tombe.

**Efficacité vérifiée par trois mutants du protocole lui-même** (dépôts
jetables, mutation commitée, selftest relancé) :

| mutant | porte qui meurt |
|---|---|
| gardes retirées des chemins normatifs | scénario 14 : `code 2 attendu, obtenu 0` (six sous-cas) |
| gardes retirées du manifeste | scénario 15b : `le manifeste ignore les gardes locales` |
| lanceur rappelant `./gcp-migration/start_and_verify.sh` | scénario 15 : `le lanceur appelle encore une garde du worktree` |

Sans ces mutants, les deux portes neuves auraient pu être vertes par
construction.

## 4. Statut

```text
DONE : identité du moteur, du runner, du validateur ET des trois gardes
       locales — pin, manifeste à frontières explicites, matérialisation
       depuis le commit, fenêtre TOCTOU fermée.
OPEN : rien sur ce point.
```

`bash gcp-migration/selftest_scale_threads.sh` → `violations=0`,
`PROTOCOLE CONFORME` (15 scénarios). Cette porte n'est PAS câblée dans
la CI GitHub : elle invoque le lanceur, et `tools/check_gcp_workflows.py`
interdit à la CI toute indirection vers un script de cycle de vie. Elle
se lance à la main, avant toute session payante — c'est son rôle.
