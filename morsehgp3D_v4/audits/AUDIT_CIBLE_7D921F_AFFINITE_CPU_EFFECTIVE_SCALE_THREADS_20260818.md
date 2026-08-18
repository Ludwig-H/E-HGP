# Audit ciblé après `7d921fff` — mesurer aussi l’affinité CPU effective

Date : 18 août 2026.  
Pins audités :

- correction du protocole : `ed28a898f203b0ff6b2af6fc84b796ce9121b69b` ;
- audit des workers réellement créés : `7d921fff4c78debe0af15f7010e8bab66b28f667`.

## Verdict

Les corrections de `ed28a898` répondent correctement aux trois points de l’audit `66886c0` :

- le nom d’un run est lié à son `argv` réel et à l’identité imprimée ;
- le schéma `digest_balls`, `digest_forest_K1..K10`, `digest_all` et les dix lignes de cardinalités est reçu ;
- la campagne distingue honnêtement l’équivalence appariée à `n=32000` du simple enregistrement de digest à `n=64000`.

L’audit `7d921fff` est également correct : les workers aval doivent être remontés par la primitive qui crée les `std::thread`, et la génération doit être détaillée par lane. Je ne duplique pas ces prescriptions.

Il reste toutefois **un verrou expérimental indépendant avant de lancer `scale_threads`** : le protocole sait combien de workers ont été créés, mais il ne vérifie pas sur combien de CPU le processus a effectivement le droit de s’exécuter.

---

## 1. `cpu_set` est actuellement une intention auto-déclarée

Le runner calcule :

```bash
NCPU=$(nproc)
CPU_SET="0-$((NCPU - 1))"
...
taskset -c "${CPU_SET}" "${PROBE_BIN}" ...
```

puis recopie simplement dans le statut :

```text
nproc=...
cpu_set=...
```

Le validateur exige seulement :

```python
re.search(r"^cpu_set=\S+$", status)
```

Il ne vérifie ni la valeur attendue du masque, ni l’affinité réellement observée par le processus.

Le défaut causal suivant passerait donc encore :

```text
CPU_SET=0
threads_requested=48
```

Le probe créerait bien 48 `std::thread` :

```text
gen_workers_q*=48
prefilter_workers=48
census_workers=48
expansion_workers=48
fold_workers_max=10
```

Les digests et cardinalités resteraient identiques, mais les 48 workers seraient tous confinés au même CPU. Le validateur déclarerait la campagne complète, et les temps conduiraient précisément à la mauvaise conclusion architecturale : « cette structure ne se parallélise pas ».

Ce cas est distinct de l’audit `7d921fff`. Compter les contextes créés ne mesure pas l’ensemble de CPU sur lequel l’OS peut les ordonnancer.

---

## 2. Correction locale recommandée

Sur la VM G4 Linux, faire publier par le **processus mesuré** son affinité effective, par exemple via `sched_getaffinity` :

```cpp
cpu_set_t mask;
CPU_ZERO(&mask);
if (sched_getaffinity(0, sizeof(mask), &mask) != 0)
    return refusal;

const int affinity_cpus_effective = CPU_COUNT(&mask);
```

Sortie machine :

```text
execution ... affinity_cpus_effective=48 affinity_mask=0-47
```

Le validateur doit alors exiger, pour le protocole actuel où tous les runs sont volontairement autorisés sur toute la machine :

```text
affinity_cpus_effective == nproc
```

et vérifier que le masque effectif correspond au masque contractuel.

Pour éviter de supposer gratuitement que les identifiants CPU autorisés sont toujours contigus, le runner peut dériver `CPU_SET` de sa propre affinité autorisée plutôt que de fabriquer `0..nproc-1` :

```bash
CPU_SET=$(taskset -pc $$ | sed 's/.*: //')
NCPU=$(python3 - <<'PY'
# compter proprement la liste/range CPU_SET
PY
)
```

Sur la G4 actuelle, cela donnera vraisemblablement `0-47`, mais le reçu portera alors le véritable environnement plutôt qu’une convention implicite.

Une variante plus compacte consiste à publier le nombre effectif et un hash canonique du masque binaire ; le nombre seul suffit pour le verrou principal, le masque rend le reçu reproductible.

---

## 3. Porte causale

Ajouter au selftest du protocole :

```text
cpu-set-one-core
```

Le faux probe conserve :

```text
threads_requested=48
workers créés=48
digests et cardinalités inchangés
```

mais publie :

```text
affinity_cpus_effective=1
```

Le validateur doit refuser avec un motif explicite d’affinité.

La porte réelle du probe peut simplement vérifier que la valeur publiée par `sched_getaffinity` est cohérente avec l’affinité du processus appelant. Il n’est pas nécessaire de mesurer le temps CPU de chaque worker ni de prétendre contrôler l’ordonnanceur ; il suffit d’empêcher qu’une expérience dite « 48 fils » soit en réalité enfermée sur un cœur.

---

## 4. Ordre conseillé avant G4

```text
1. exécuter l’audit 7d921fff : workers remontés par les primitives réelles ;
2. publier workers q2/q3/q4 séparément ;
3. publier et valider l’affinité CPU effective ;
4. tuer les mutants parallel-ranges-one-worker, q3-one-worker et cpu-set-one-core ;
5. lancer seulement ensuite la phase n32000.
```

Le protocole est désormais très proche d’être une vraie expérience de parallélisme. Il lui manque seulement de vérifier que les nombreux threads, une fois créés, ne sont pas tous assis sur la même chaise — arrangement courant dans les organisations humaines, mais assez peu informatif pour un benchmark multicœur.
