# Audit ciblé après `1a0640` — la campagne G4 doit être transactionnelle et séparer débit de latence

Date : 17 août 2026.  
Pin de campagne audité : `1a06402332a0c0ca405ee5fe794f69f297f42924`.  
HEAD de code au début de l'audit : `d0edac9de6a570476f48e61bd94abd3087c9b863`.

## Verdict

Le reçu `n=8000` est recevable comme **mesure de coût non jugée** : il annonce explicitement que `désaccords=0` est hors régime du juge brut, et ses compteurs confirment les deux postes désormais dominants, génération q4 et fold.

Le script `gcp-migration/session_campagne_v4_scale_g4.sh` ne doit en revanche pas être lancé tel quel pour produire un reçu contractuel. Deux défauts peuvent rendre une campagne apparemment complète alors que certains runs ont expiré, été tués ou sont tombés en OOM ; de plus, les temps de 24 runs concurrents ne sont pas des latences mono-thread comparables aux pentes locales.

Ce sont des verrous de protocole, pas de géométrie. Ils sont locaux à corriger, mais importants : une campagne coûteuse qui sait calculer exactement une forêt et approximativement son propre statut serait une conclusion assez ironique pour être évitée.

---

## 1. Le script masque actuellement les échecs des jobs

La boucle distante lance chaque run sous :

```bash
set -euo pipefail

( timeout 10800 "$P" ... > "$out" 2>&1
  echo "code=$?" >> "$out"
  ... ) &

wait || true
echo "=== CAMPAGNE COMPLETE ==="
```

Il y a deux problèmes cumulés.

### 1.1 `set -e` empêche d'écrire le code du run

Dans le sous-shell d'arrière-plan, si `timeout` rend `124`, si le noyau tue le processus pour mémoire, ou si le probe rend un autre code non nul, `set -e` termine le sous-shell **avant** :

```bash
echo "code=$?"
```

Le fichier peut donc être tronqué sans marqueur de statut.

### 1.2 `wait` sans PID ne collecte pas les statuts

En Bash, `wait` sans argument attend tous les enfants mais rend zéro après cette attente ; il n'agrège pas leurs codes d'échec. Le `|| true` rend de toute façon l'intention explicite : aucun job ne peut faire échouer l'étape.

Le script imprime alors :

```text
=== CAMPAGNE COMPLETE ===
```

puis rapatrie les fichiers et termine avec succès. Une campagne où plusieurs gros runs ont expiré ou été tués peut donc être certifiée complète.

### Correction recommandée

Chaque run doit toujours produire deux fichiers atomiques et distincts :

```text
scale_... .txt       payload du probe
scale_... .status    code, durée, signal éventuel, peak RSS
```

Le sous-shell doit désarmer localement `errexit` autour du run :

```bash
run_one() {
  local name="$1"; shift
  local out="out/${name}.txt"
  local status="out/${name}.status"
  local rc=0

  /usr/bin/time -v -o "${status}.time" \
    timeout 10800 "$P" "$@" >"${out}" 2>&1 || rc=$?

  {
    printf 'code=%d\n' "$rc"
    printf 'finished=1\n'
  } > "${status}.tmp"
  mv "${status}.tmp" "$status"
  return 0
}
```

`run_one` retourne volontairement zéro pour permettre le rapatriement de tous les résultats partiels. **Après le `scp`**, une validation locale doit exiger :

1. les 24 noms attendus exactement une fois ;
2. un fichier `.status` complet pour chacun ;
3. `code=0` pour chacun ;
4. une ligne de sortie contenant tous les compteurs et temps attendus ;
5. aucune ligne `REFUS`, `INVARIANT`, `PLANCHER`, `Killed`, `bad_alloc` ou timeout.

Si une condition échoue, le reçu doit porter :

```text
campaign_status = partial_or_failed
```

et le script terminer non nul **après** le rapatriement. Le mot `COMPLETE` est réservé au cas où les 24 runs ont été validés.

---

## 2. Vingt-quatre runs concurrents ne donnent pas des latences comparables

Chaque probe est mono-thread, mais les 24 probes sont lancés simultanément. Leurs temps internes mesurent donc :

```text
algorithme + contention cache + bande passante mémoire + pression allocateur + éventuel swap/OOM.
```

Ils ne sont pas directement comparables au run isolé `n=8000`, ni aux pentes `400/800/1600` obtenues sans cette contention. La qualification « mono-thread » décrit chaque processus, pas le protocole de mesure.

Je conseille de séparer deux campagnes.

### 2.1 Campagne de couverture et de compteurs

Elle peut être parallèle, avec concurrence bornée. Elle sert à obtenir :

- nombres de candidats, boules, événements, deltas, incidences ;
- statuts transactionnels ;
- faisabilité mémoire ;
- débit global de la machine.

Ses temps par run sont étiquetés :

```text
timing_scope = contended_throughput
```

et ne servent pas aux pentes de latence.

### 2.2 Campagne de latence

Pour les courbes de coût et la comparaison avant/après optimisation :

- un seul run lourd à la fois, ou un run par domaine NUMA explicitement isolé ;
- affinité CPU fixe (`taskset`, éventuellement `numactl`) ;
- environnement monothread explicite (`OMP_NUM_THREADS=1`, etc.) ;
- `/usr/bin/time -v` pour le pic mémoire ;
- CPU, fréquence, charge et version du commit enregistrés ;
- idéalement deux répétitions et médiane sur les tailles abordables.

Cette campagne porte :

```text
timing_scope = isolated_latency
```

Mélanger les deux donne des tableaux précis mais difficilement interprétables, activité dans laquelle les benchmarks humains excellent déjà sans assistance.

---

## 3. La concurrence doit être pilotée par la mémoire, pas seulement par les 48 vCPU

Le reçu `n=8000` contient :

```text
3,1 M événements,
19,5 M unions,
fold encore fondé sur plusieurs std::map.
```

Aucun pic RSS n'est publié. Le script lance pourtant simultanément les huit runs `n=32000`, les huit `n=16000` et les huit `n=8000`. Sur 180 Go, cela suppose implicitement que l'ensemble des 24 pics résidents tient en mémoire ; cette hypothèse n'est pas étayée.

Le protocole sûr est progressif :

1. run isolé `n=8000` avec `Maximum resident set size` ;
2. run isolé `n=16000` ;
3. seulement si le budget est connu, lancement de `n=32000` ;
4. concurrence par vague calculée avec une réserve, par exemple

   ```text
   parallelism <= floor(0.75 * RAM / peak_RSS_estime)
   ```

5. séparation par taille : les gros runs ne démarrent pas tous au même instant.

À défaut d'estimation fiable, commencer avec :

```text
n=8000  : concurrence 4 à 8,
n=16000 : concurrence 2 à 4,
n=32000 : concurrence 1 à 2,
```

puis ajuster depuis les pics mesurés. Ces nombres sont un plan prudent, pas des constantes théoriques.

Une OOM concurrente ne renseigne ni sur la mémoire d'un run isolé ni sur l'ordre de complexité. Elle renseigne surtout sur l'enthousiasme du lanceur.

---

## 4. `désaccords=0` doit devenir `judge=off`, pas un zéro machine

Le reçu humain écrit correctement :

```text
désaccords : 0 (hors régime jugé — n > 120)
```

Mais le probe imprime encore numériquement `desaccords=0` quand `--judge` n'est pas activé. Un parseur de campagne peut facilement interpréter ce zéro comme un accord vérifié.

Sortie recommandée :

```text
judge=off desaccords=NA
```

ou deux champs :

```text
judge_enabled=0
judge_disagreements=0
```

avec interdiction documentaire d'utiliser le second lorsque le premier vaut zéro.

Le run `n=8000` reste utile comme smoke exact du sujet et mesure de coût. Il n'est pas une nouvelle preuve d'exactitude, et le reçu le sait déjà ; il faut simplement que le format machine le sache aussi.

---

## 5. Ordre utile avant la session payante

1. Corriger la collecte de statut et la validation des 24 sorties.
2. Ajouter `time -v` et un premier reçu de pic RSS isolé.
3. Scinder couverture parallèle et latence isolée.
4. Piloter la concurrence par taille et mémoire.
5. Lancer ensuite la matrice G4.
6. Ne produire le reçu global qu'après validation automatisée de tous les fichiers rapatriés.

## Conclusion

La campagne `n=8000` confirme utilement les verrous déjà identifiés et ne révèle aucune faute mathématique nouvelle. Le lanceur G4, lui, n'est pas encore transactionnel : il peut perdre le code d'un job, ignorer son échec, afficher `COMPLETE`, puis enregistrer des temps contaminés par une concurrence non mesurée.

La correction est courte comparée aux calculs qu'elle protège. Il vaut mieux dépenser quelques dizaines de lignes de Bash maintenant que quatre heures de G4 pour obtenir vingt-quatre fichiers dont personne ne sait exactement lesquels ont survécu.