# Audit ciblé après `ed28a898` — les workers aval sont encore *prévus*, pas mesurés

Date : 18 août 2026.  
Pin audité : `ed28a898f203b0ff6b2af6fc84b796ce9121b69b`.

## Verdict

Les corrections principales de l’audit `66886c0` sont reçues positivement :

- le nom du run est désormais lié à son `argv` contractuel et à l’identité imprimée par le probe ;
- le schéma des digests et cardinalités est complet pour `K=1,…,10` ;
- le digest canonique est sensible aux objets importants de la forêt ;
- la restriction q4 à la lentille de l’ancre reste exacte ;
- les campagnes `n32000` et `n64000` sont correctement distinguées.

Je ne trouve aucune nouvelle faute géométrique.

Il reste toutefois **un raccord expérimental réel avant la campagne `scale_threads`** : pour quatre étages sur cinq, les champs appelés « workers mesurés » sont encore calculés hors de la primitive qui crée les fils. Une sérialisation accidentelle de `parallel_ranges` ne serait donc pas détectée.

---

## 1. Le nombre publié est actuellement le nombre attendu

Pour le préfiltre, le code calcule :

```cpp
const size_t T = cands.empty()
    ? 1
    : std::min((size_t)std::max(threads, 1), cands.size());
st->prefilter_workers = std::max(st->prefilter_workers, (u64)T);
...
parallel_ranges(cands.size(), threads, ...);
```

Le census, l’expansion et les folds suivent la même structure :

1. calcul local de `T` ;
2. publication de `T` dans les statistiques ;
3. appel de `parallel_ranges`, qui **recalcule indépendamment** son propre nombre de workers et crée les `std::thread`.

Ainsi, le mutant réellement dangereux

```text
parallel_ranges-hardcodes-one-worker
```

peut remplacer dans `parallel_ranges`

```cpp
T = min(requested, n)
```

par

```cpp
T = 1
```

sans changer :

```text
prefilter_workers,
census_workers,
expansion_workers,
fold_workers_max.
```

Les digests restent évidemment identiques, puisque le calcul devient seulement séquentiel. Le validateur déclarerait donc que les fils demandés ont été appliqués alors que ces quatre étages n’en ont créé qu’un.

Le mutant permanent actuel `parallel-hardcodes-one-worker` ne ferme pas ce cas : il agit uniquement dans `run_rects`, donc sur la génération. La porte `--workers-gate` vérifie bien ce mutant-là, mais pas la primitive générique qui porte tout l’aval.

Ce point n’est pas une querelle de vocabulaire. La campagne doit décider quelles structures se parallélisent réellement ; publier le planificateur demandé à la place du pool créé fausse précisément cette conclusion.

---

## 2. Correction locale recommandée

Calculer le plan une fois, mais faire retourner par la primitive le nombre réellement créé.

Par exemple :

```cpp
inline size_t planned_workers(size_t n, int requested) {
  return n == 0 ? 0 : std::min((size_t)std::max(requested, 1), n);
}

template <typename Fn>
size_t parallel_ranges(size_t n, size_t T, Fn&& fn) {
  if (n == 0) return 0;
  if (T <= 1) {
    fn(0, n, 0);
    return 1;
  }

  std::vector<std::thread> pool;
  pool.reserve(T);
  for (size_t t = 0; t < T; ++t) {
    const size_t b = n * t / T;
    const size_t e = n * (t + 1) / T;
    pool.emplace_back([&fn, b, e, t] { fn(b, e, t); });
  }

  const size_t actual = pool.size();
  for (auto& th : pool) th.join();
  return actual;
}
```

Le caller utilise le `T` planifié pour dimensionner ses buffers locaux, puis publie **la valeur retournée** :

```cpp
const size_t T = planned_workers(cands.size(), threads);
std::vector<...> local(T);
const size_t actual = parallel_ranges(cands.size(), T, ...);
st->prefilter_workers = std::max<u64>(st->prefilter_workers, actual);
```

Même raccord pour census, expansion et fold.

La mesure pertinente est ici le nombre de contextes de calcul effectivement lancés par la primitive. Il n’est pas nécessaire d’instrumenter l’ordonnanceur du noyau ou de prouver que les fils ont reçu exactement le même temps CPU, ambition que même les systèmes d’exploitation évitent de promettre.

---

## 3. `gen_workers_max` peut masquer la sérialisation de q3

La génération possède trois lanes, mais ne publie actuellement que :

```text
gen_workers_max = max(workers_q2, workers_q3, workers_q4).
```

Le scan q3 est précisément l’un des postes dominants mesurés. Une régression qui sérialise seulement q3 tandis que q2 ou q4 crée encore `N` workers laisse :

```text
gen_workers_max = N.
```

La campagne conclurait alors que « la génération » a effectivement utilisé `N` fils, sans voir que son noyau dominant est resté séquentiel.

Je recommande donc :

```cpp
u64 gen_workers[3] = {0, 0, 0}; // q2, q3, q4
```

alimenté séparément dans chaque appel de `run_rects`. Le validateur peut exiger :

```text
gen_workers_q = min(threads_requested, rect_alive_q)
```

ou, aux tailles massives où chaque lane possède assez de rectangles :

```text
gen_workers_q2 = gen_workers_q3 = gen_workers_q4 = threads_requested.
```

Conserver `gen_workers_max` comme résumé est possible, mais il ne doit plus être l’autorité de réception.

---

## 4. Deux portes causales suffisantes

### 4.1 Primitive aval

```text
parallel-ranges-hardcodes-one-worker
```

Le mutant ne touche ni la CLI, ni les résultats, ni les digests. Il force seulement `parallel_ranges` à lancer un worker. La porte doit observer :

```text
prefilter = census = expansion = fold = 1
```

et rendre `4`.

### 4.2 Lane dominante

```text
q3-hardcodes-one-worker
```

q2 et q4 conservent quatre workers, q3 n’en crée qu’un. L’ancien champ

```text
gen_workers_max = 4
```

reste volontairement inchangé ; la nouvelle métadonnée `gen_workers_q3=1` doit tuer le mutant.

Ces deux portes couvrent le défaut structurel. Il n’est pas nécessaire d’ajouter une collection botanique de mutants par caller.

---

## Ordre conseillé

```text
1. faire retourner le nombre effectivement créé par parallel_ranges ;
2. publier les workers de génération par lane ;
3. tuer les deux mutants ci-dessus ;
4. rejouer le selftest du protocole ;
5. lancer seulement ensuite scale_threads n32000.
```

Le protocole est désormais proche d’être recevable. Le dernier raccord consiste simplement à mesurer le mécanisme parallèle lui-même, plutôt qu’à lui demander combien de fils il était censé avoir créé — méthode d’auto-évaluation que les logiciels apprécient autant que les administrations.