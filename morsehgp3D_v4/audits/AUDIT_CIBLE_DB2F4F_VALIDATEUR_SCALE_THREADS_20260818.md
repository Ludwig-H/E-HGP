# Audit ciblé après `db2f4f` — le protocole est bien scindé, mais le validateur ne lie pas encore le nom du run à l'expérience réellement exécutée

Date : 18 août 2026.  
Pins audités :

- `ecd455d8773ed968bd3af73bcf00d543a03a741d` : lentille q4 matérialisée par ancre ;
- `db2f4f265ea976c155078065a22fc07c8c8325e7` : sessions `scale_threads`, budget transactionnel et digest canonique.

## Verdict

Les corrections principales des audits `9223888` et `b3a6eb4` sont reçues positivement.

- La restriction q4 à la lentille de l'ancre est exacte : tout seed aigu possédé par `ab` et toute complétion valide satisfont bien
  `|z-a|² <= |b-a|²` et `|z-b|² <= |b-a|²`.
- La campagne massive est désormais séparée de la couverture historique.
- Le budget est calculé avant toute action GCP depuis la liste de runs du runner lui-même.
- Un run qui ne tient plus avant la deadline n'est pas commencé.
- Le digest canonique sérialise explicitement les boules post-RLE puis la forêt compacte par `K`.

Je ne vois pas de nouvelle fausse mort géométrique.

Il reste toutefois **trois raccords de protocole à fermer avant de dépenser une session G4 massive**. Ils sont locaux : il n'est pas nécessaire de revoir l'architecture de campagne.

---

## 1. Le validateur ne vérifie pas que le run nommé a réellement exécuté les bons paramètres

Le runner écrit

```text
args_sha256=<64 hex>
```

mais le validateur vérifie seulement que ce champ a la bonne forme :

```python
re.search(r"^args_sha256=[0-9a-f]{64}$", st, re.M)
```

Il ne recalcule jamais la valeur attendue depuis le nom du run. Il ne vérifie pas non plus les champs pourtant imprimés par le probe :

```text
famille=...
n=...
s=...
smax=...
seed=...
```

Le motif de compteurs exige seulement la présence de `boules_uniques`, `evenements` et `juge=off`.

### Deux faux `complete` possibles

1. Une régression remplace, dans la phase `n64000`,

   ```text
   --n=64000
   ```

   par

   ```text
   --n=32000.
   ```

   Chaque famille n'a qu'un run dans cette phase. Il n'existe donc aucun appariement capable de révéler l'erreur. Le code peut rendre `0`, publier ses digests et être déclaré `complete` sous un nom `n64000`.

2. Les deux runs nommés `eight_clusters` exécutent accidentellement `uniform`. Leurs digests `t8/tmax` sont alors parfaitement égaux, ce qui satisfait le validateur, alors que l'expérience dense visée n'a jamais été lancée.

Le `args_sha256` actuel documente une commande, mais n'établit pas qu'elle est la commande contractuelle.

### Correction minimale

Le validateur doit posséder une spécification exacte par nom :

```text
name,
family,
n,
s=8,
smax=11,
seed=3,
threads_label.
```

Puis il doit exiger simultanément :

1. le hash exact de l'argv canonique attendu ;
2. la concordance de la ligne de sortie `famille/n/s/smax/seed` ;
3. la concordance de `threads_requested` avec la spécification.

Pour éviter l'ambiguïté de `$*`, le hash du runner devrait porter sur une sérialisation sans ambiguïté, par exemple les arguments séparés par `NUL`, ou être remplacé par des champs explicites dans le statut.

Portes causales à ajouter au faux probe :

```text
wrong-n-under-correct-name,
wrong-family-under-correct-name,
wrong-smax-under-correct-name.
```

Les cardinalités et les digests peuvent rester parfaitement cohérents entre fils ; le validateur doit néanmoins refuser.

---

## 2. `threads_effective` est actuellement le nombre demandé, pas le nombre effectivement créé

Le probe publie :

```cpp
std::printf("execution threads_effective=%d\n", a.threads);
```

Cette valeur provient directement de la CLI. Elle n'est pas mesurée dans `run_rects` ni dans `parallel_ranges`.

Le test actuel compare donc essentiellement deux copies du même paramètre :

```text
status.threads_requested
contre
sortie.a.threads.
```

Une régression réelle telle que

```text
parallel_ranges(..., 1, ...)
```

ou

```text
run_rects : T = 1
```

laisserait `execution threads_effective=48`, produirait le même digest, et serait déclarée conforme. Le faux probe du selftest tue un programme qui imprime `1`, mais il ne tue pas le mutant plus dangereux qui **ignore les fils tout en réimprimant la valeur demandée**.

### Correction recommandée

Instrumenter les nombres de workers réellement créés, au moins pour les postes qui portent la mesure :

```text
threads_requested,
gen_workers_max,
prefilter_workers,
census_workers,
fold_workers_max.
```

Ces valeurs doivent être alimentées au point de création des `std::thread`, pas dans le parseur d'arguments.

Pour la campagne `scale_threads`, le validateur doit au minimum exiger que le noyau de génération ait effectivement créé le nombre attendu, modulo le nombre de tâches disponibles :

```text
gen_workers_max = min(threads_requested, work_units_generation).
```

Un mutant C++ permanent

```text
parallel-hardcodes-one-worker
```

doit conserver la CLI et les digests, mais être rejeté par la métadonnée réellement mesurée.

Le fold peut légitimement plafonner à `K_max <= 10`; il faut donc publier les workers par étage plutôt qu'un unique nombre prétendument valable pour toute la chaîne.

---

## 3. Le schéma de digest est accepté même s'il manque presque tous les ordres `K`

À `smax=11`, le probe doit publier exactement :

```text
digest_balls,
digest_forest_K1,
...,
digest_forest_K10,
digest_all.
```

Le validateur actuel exige seulement :

```python
len(digests) >= 3
```

et le happy path du faux probe ne publie précisément que :

```text
digest_balls,
digest_forest_K1,
digest_all.
```

Ainsi une régression qui supprime systématiquement `digest_forest_K2` à `K10` dans tous les runs est déclarée conforme. Une divergence parallèle située uniquement à `K7` devient alors invisible.

Les lignes de cardinalité ont le même défaut : leur ensemble est comparé, mais le validateur n'exige pas exactement une ligne pour chacun des dix ordres.

### Correction exacte

Pour `smax=11`, exiger une occurrence et une seule de :

```text
{digest_balls, digest_forest_K1, ..., digest_forest_K10, digest_all}
{cardinalites K=1, ..., cardinalites K=10}
```

sans doublon ni ordre manquant.

Portes causales :

```text
omit-digest-K7,
duplicate-digest-K3,
omit-cardinality-K9.
```

Le faux happy path doit publier le schéma complet de production, et non une miniature que le validateur apprend ensuite à considérer comme suffisante.

Une petite porte C++ de sensibilité du sérialiseur serait également utile : modifier successivement une `BallKey`, un `final_canon_fid` et une facette née doit modifier respectivement `digest_balls`, le digest du bon `K` et `digest_all`. Le smoke actuel vérifie seulement que le probe imprime quelque chose.

---

## 4. Statut de la phase `n64000`

Cette phase ne contient qu'un run `tmax` par famille. Son digest est utile comme empreinte de reçu, mais il ne constitue pas un appariement inter-fils. Le message final devrait donc distinguer honnêtement :

```text
n32000 : thread_equivalence_checked,
n64000 : digest_recorded_unpaired.
```

Il n'est pas nécessaire de doubler immédiatement les quatre runs à 64k. L'équivalence peut être reçue à 32k et la phase 64k servir à la seule mesure d'échelle. Il faut simplement éviter de lui attribuer une preuve qu'elle ne réalise pas.

---

## Ordre conseillé

```text
1. lier chaque nom à son argv et à l'identité imprimée par le probe ;
2. exiger le schéma complet K=1..10 ;
3. mesurer les workers réellement créés ;
4. rejouer selftest_scale_threads ;
5. seulement ensuite lancer les deux sessions G4.
```

Le travail de Claude est très proche d'un protocole recevable : budget, reprise, pin et digest sont maintenant bien structurés. Les raccords ci-dessus empêchent seulement le validateur de certifier avec beaucoup de sérieux une expérience différente de celle écrite sur l'étiquette, vieille spécialité des campagnes de benchmark lorsque les métadonnées se contentent de se citer mutuellement.
