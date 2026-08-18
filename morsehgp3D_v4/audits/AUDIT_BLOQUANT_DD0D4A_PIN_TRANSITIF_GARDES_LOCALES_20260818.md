# Audit bloquant après `dd0d4a69` — le pin du protocole ne couvre pas encore les gardes locales exécutées

Date : 18 août 2026.  
Pin audité : `dd0d4a69fdbc87ab6b73ca26ff343dea0ff26827`.

## Verdict

Les derniers correctifs de code sont reçus positivement :

- `ed6a798` ferme correctement les audits sur les workers et l’affinité : `parallel_ranges` retourne le nombre de workers créés, les trois lanes q2/q3/q4 sont distinguées, et l’affinité effective est mesurée dans le processus par `sched_getaffinity` ;
- les modifications `ab8fde5` / `b4dcdb4` / `113b25c` / `5c326c1` restent fail-closed et modélisent désormais les gardes réellement rencontrées sur GCE ;
- l’audit `dd0d4a69` sur les statuts périmés de `PASSATION.md` est correct et doit être exécuté ;
- je ne trouve aucune nouvelle faute géométrique ni perte de boule q2/q3/q4 dans ces commits.

Il reste cependant **un verrou transactionnel avant de relancer `scale_threads`** : le pin scelle le moteur, le runner distant et le validateur, mais pas les scripts locaux qui déterminent le budget, démarrent la VM et certifient son arrêt.

---

## 1. Les gardes locales sont normatives mais absentes du pin

Le script courant `v4_scale_threads_pin.sh` protège seulement :

```text
morsehgp3D_v4/
session_scale_threads_g4.sh
v4_scale_threads_remote.sh
validate_v4_scale_threads.py
v4_scale_threads_pin.sh
```

Son `protocol_manifest_sha256` ne hache que :

```text
session_scale_threads_g4.sh
v4_scale_threads_remote.sh
validate_v4_scale_threads.py
```

Or `session_scale_threads_g4.sh` lit ou exécute aussi, depuis le **worktree courant** :

```text
gcp-migration/start_and_verify.sh
gcp-migration/set_max_run_duration_and_verify.sh
gcp-migration/stop_and_verify.sh
```

Ces fichiers ne sont pas annexes :

- le préflight lit `TIMESTAMP_TOLERANCE_SECONDS` et `SSH_KEY_TTL_SLACK_SECONDS` dans `start_and_verify.sh` ;
- `set_max_run_duration_and_verify.sh` change le coupe-circuit GCE ;
- `start_and_verify.sh` choisit les conditions de certification de la génération, de la clé OS Login, de `terminationTimestamp` et de la garde invitée ;
- `stop_and_verify.sh` est l’autorité qui permet au trap d’annoncer un arrêt certifié.

Le protocole réellement exécuté est donc plus grand que le protocole actuellement pinné.

---

## 2. Contre-exemple causal

Une modification locale non commitée de :

```bash
gcp-migration/start_and_verify.sh
```

par exemple sur :

```text
TIMESTAMP_TOLERANCE_SECONDS
SSH_KEY_TTL_SLACK_SECONDS
read_termination_timestamp
verify_running_guard
```

n’est pas détectée par `v4_scale_threads_pin.sh`.

Le pin rend alors avec succès :

```text
source_commit = HEAD
protocol_manifest_sha256 = digest des trois scripts historiques
```

mais la session :

1. lit les constantes du fichier modifié ;
2. exécute ce fichier modifié pour démarrer la VM ;
3. grave dans les statuts un manifeste qui ne contient pas ces octets.

Le cas le plus dangereux concerne `stop_and_verify.sh` : un mutant local qui rend immédiatement zéro peut faire croire au cleanup qu’un arrêt est certifié alors que le script ayant produit cette affirmation ne correspond ni au manifeste ni au commit annoncé.

Le fait que `source_commit` permette de retrouver les versions commitées ne suffit pas : **rien ne garantit aujourd’hui que ces versions sont celles qui ont été exécutées**.

Il existe également une fenêtre TOCTOU : même si le worktree est propre au moment du pin, les helpers sont relus plus tard depuis le disque, après le pin et après le démarrage.

---

## 3. Correction minimale et transitive

### 3.1 Élargir les chemins normatifs

Ajouter au moins :

```bash
LOCAL_GUARD_PATHS=(
  gcp-migration/set_max_run_duration_and_verify.sh
  gcp-migration/start_and_verify.sh
  gcp-migration/stop_and_verify.sh
)
```

aux trois contrôles :

```bash
git diff --quiet
git diff --cached --quiet
git ls-files --others --exclude-standard
```

et au manifeste de protocole.

Le manifeste devrait idéalement sérialiser chaque entrée avec son chemin et sa longueur avant son contenu, plutôt que concaténer des contenus sans frontière explicite.

### 3.2 Matérialiser les helpers depuis `SOURCE_COMMIT`

Le simple contrôle de propreté ferme l’état initial, mais pas la fenêtre entre pin et exécution. Il faut extraire les helpers dans `${WORK}/pinned/gcp-migration/` depuis le commit :

```bash
git archive ... \
  gcp-migration/set_max_run_duration_and_verify.sh \
  gcp-migration/start_and_verify.sh \
  gcp-migration/stop_and_verify.sh
```

puis n’utiliser que :

```bash
PINNED_GCP="${WORK}/pinned/gcp-migration"
SET_MAX="${PINNED_GCP}/set_max_run_duration_and_verify.sh"
START_AND_VERIFY="${PINNED_GCP}/start_and_verify.sh"
STOP_AND_VERIFY="${PINNED_GCP}/stop_and_verify.sh"
```

`start_and_verify.sh` appelle déjà `stop_and_verify.sh` relativement à son propre `BASH_SOURCE` : placer les deux dans le même répertoire pinné ferme aussi son chemin d’urgence.

### 3.3 Faire le pin avant le préflight qui lit les constantes

Le pin est une opération locale, sans mutation GCP. L’ordre correct est :

```text
création de WORK
→ pin et matérialisation depuis HEAD
→ lecture des constantes dans le start_and_verify pinné
→ préflight des six gardes
→ seulement ensuite toute action GCP
```

Le préflight courant se déroule avant `v4_scale_threads_pin.sh` et lit donc déjà un fichier hors de la chaîne de confiance.

---

## 4. Deux portes suffisantes

### 4.1 `uncommitted-local-guard`

Dans un dépôt temporaire :

1. laisser le moteur, le runner et le validateur inchangés ;
2. modifier seulement `start_and_verify.sh` ;
3. appeler `v4_scale_threads_pin.sh` ;
4. exiger un refus code 2 avant toute action GCP.

Même porte avec `stop_and_verify.sh`.

### 4.2 `helper-from-worktree-after-pin`

1. produire `${WORK}/pinned` depuis un commit propre ;
2. modifier ensuite le helper du worktree ;
3. vérifier que les chemins préparés pour le lancement et le cleanup pointent toujours vers `${WORK}/pinned` ;
4. vérifier que le manifeste inclut les helpers pinnés.

Ce mutant distingue une vraie matérialisation du simple contrôle ponctuel de propreté.

---

## 5. Statut de la passation

Après exécution de l’audit `dd0d4a69`, la passation doit également nuancer :

```text
DONE : identité du moteur, du runner et du validateur.
OPEN : fermeture transitive des helpers locaux de sécurité.
```

Une fois les trois helpers ajoutés au pin et exécutés depuis `${WORK}/pinned`, je considère la chaîne de provenance de `scale_threads` fermée. Le prochain travail scientifique redevient alors celui indiqué par le contre-audit de passation : scan q3/covers, streaming de l’internement et contrat de sortie 30M.

## Conclusion

Le moteur récent est reçu. Le protocole sait maintenant mesurer les workers et l’affinité réels, mais il ne sait pas encore prouver quels octets ont décidé que la VM pouvait démarrer puis être déclarée arrêtée. La correction est locale, testable sans GCP et doit précéder la prochaine session payante.
