# Audit ciblé après `fa7cac4` et `a694496` — campagne `scale_threads`

Date : 18 août 2026.  
Pins audités :

- `fa7cac4bf12689bd08a0d94e995d86e1070ceaf3` : profil des lanes et tampon réutilisé du counting sort ;
- `a694496c9d6bd9116a73e3f1b29893ec7cd4b311` : ajout des runs `n=32000` à 8/max fils et `n=64000` à max fils.

## Verdict

Le changement de `fa7cac4` est reçu : le tampon `cover_tmp` alterne correctement avec `cover` par `swap`, conserve le même counting sort stable et ne modifie aucun prédicat géométrique. Les nouveaux chronos confirment utilement que le scan q3 et la construction des covers sont devenus les postes CPU dominants.

L'ajout de `a694496` répond à la bonne question expérimentale, mais la campagne ne doit pas encore servir de preuve pour choisir l'architecture. Deux verrous transactionnels subsistent :

1. les huit runs décisifs sont placés à la fin d'une session dont le budget global ne peut pas les garantir ;
2. le validateur ne prouve pas que les runs à 8 et à `nproc` fils ont calculé le même objet, ni même quel nombre de fils a réellement été exécuté.

Ces deux points sont bloquants avant de dépenser une session G4 complète. Ils ne remettent pas en cause le moteur géométrique.

---

## 1. Le budget global de la session est incompatible avec la nouvelle phase 3

La session fixe actuellement :

```text
MAX_RUN_SECONDS          = 21600 s  = 6 h,
GUEST_SHUTDOWN_MINUTES   = 350 min,
TTL de la clé OS Login   = 370 min.
```

Le runner distant autorise :

```text
RUN_TIMEOUT = 10800 s = 3 h par run.
```

La phase 3 ajoute huit runs **séquentiels**, après quatre pilotes et vingt-quatre runs de couverture :

```text
4 runs à n=32000 : uniform/eight_clusters × {t8,tmax},
4 runs à n=64000 : une fois tmax par famille.
```

Indépendamment de toute estimation de performance, le protocole n'a donc aucune garantie temporelle : deux runs atteignant leur timeout consomment déjà la totalité des six heures, et la phase 3 pourrait à elle seule durer jusqu'à vingt-quatre heures. Elle commence pourtant après les vingt-huit runs précédents.

Ce n'est pas seulement un pire cas artificiel. Les reçus déjà versionnés montrent que les cellules denses à `n=8000` prennent encore plusieurs minutes, et que la croissance vers `32000/64000` est précisément ce que la campagne cherche à mesurer. Le risque dominant est donc que la VM atteigne son arrêt invité ou sa durée maximale après avoir payé toutes les phases secondaires, avant les mesures qui doivent décider de l'architecture.

Le selftest à faux probe ne peut pas voir ce problème : son faux calcul est instantané et valide seulement la machine à états des fichiers.

### Correction recommandée

La solution propre est de séparer la phase `scale_threads` dans une session pinnée distincte, plutôt que d'allonger silencieusement la campagne historique.

Ordre conseillé :

```text
Session A — échelle de fils, prioritaire
  n=32000, uniform et eight_clusters, t1/t8/tmax appariés ;

Session B — faisabilité n=64000
  tmax, une famille par run ou petite vague séquentielle ;

Session C — couverture multi-familles
  les 24 runs historiques, si elle reste utile après A/B.
```

Chaque session conserve son propre manifeste, ses statuts et son arrêt certifié ; un petit manifeste agrégateur peut relier les trois pins. Cette séparation réduit aussi la perte en cas de préemption spot.

Si Claude préfère absolument une session unique, il faut alors modifier ensemble :

```text
MAX_RUN_SECONDS,
GUEST_SHUTDOWN_MINUTES,
TTL OS Login,
budget restant avant chaque lancement.
```

Le runner doit refuser de démarrer un run lorsque le temps restant avant l'arrêt certifié est inférieur à son timeout plus une marge de rapatriement. Augmenter seulement `RUN_TIMEOUT` ou seulement la durée maximale ne ferme pas le contrat.

Enfin, puisque la phase 3 est celle qui décide de l'architecture, elle doit être exécutée avant la couverture contendue, pas après elle.

---

## 2. `code=0` et `juge=off` ne prouvent pas l'équivalence à grande échelle

Le validateur exige actuellement :

```text
.status présent,
code=0,
pins corrects,
ligne de compteurs présente,
aucun motif interdit.
```

Il ne compare pas les sorties des deux runs appariés :

```text
thr_uniform_n32000_smax11_t8
thr_uniform_n32000_smax11_tmax

thr_eight_clusters_n32000_smax11_t8
thr_eight_clusters_n32000_smax11_tmax
```

Ces runs sont hors juge brut. Une race qui perdrait un shard seulement à 48 fils, un ordre non déterministe mal canonisé, ou un futur dépassement de capacité pourrait donc rendre `code=0` avec une autre forêt, et la campagne serait tout de même déclarée `complete`.

Les portes `--par-gate` à petit `n` restent nécessaires, mais elles ne remplacent pas une confrontation sur les tailles qui motivent justement la nouvelle architecture.

### 2.1 Métadonnées d'exécution manquantes

Les statuts ne gravent pas :

```text
threads_requested,
threads_effective,
nproc,
cpu_set,
arguments du probe.
```

Le protocole pinné permet de connaître la chaîne `--threads=...`, mais pas la valeur runtime de `${NCPU}` ni la preuve que le probe l'a réellement appliquée. La recette finale propose en outre de rapatrier `out/*.txt` et `*.status`, pas nécessairement le journal de session contenant `coeurs=...`.

Je recommande d'ajouter à chaque statut :

```text
threads_requested=8 | N,
nproc=N,
cpu_set=...,
args_sha256=...,
```

et de faire publier au probe :

```text
threads_effective=N.
```

Le validateur doit vérifier la cohérence entre nom du run, statut et sortie.

### 2.2 Signature canonique de l'objet

Les seuls totaux

```text
boules_uniques, événements, fusions, nœuds
```

ne suffisent pas : deux objets différents peuvent partager ces cardinalités.

Pour les campagnes sans juge, le probe devrait publier une signature déterministe du flux canonique :

```text
digest_balls      : BallKey + arité + niveau après RLE ;
digest_forest_K   : facet_keys/final_canon_fid et ComponentDelta canoniques ;
digest_all        : combinaison ordonnée des K.
```

Une SHA-256 sur une sérialisation explicitement versionnée convient. Elle se calcule en streaming et ne change pas la taille asymptotique du produit.

Le validateur doit alors exiger, pour les paires `t8/tmax` à `n=32000` :

```text
mêmes digests,
mêmes cardinalités par K,
mêmes compteurs de violations (=0).
```

Idéalement, le pilote mono-fil `lat_uniform_n32000_smax11` rejoint aussi la comparaison `t1/t8/tmax` pour `uniform`.

Si la signature n'est pas encore disponible, la correction minimale est de parser toutes les lignes `cardinalites K=...` et d'exiger leur égalité exacte entre les runs appariés. Ce n'est pas une preuve aussi forte, mais c'est nettement préférable à la simple présence d'une ligne générique.

---

## 3. Portes causales à ajouter

Le selftest du protocole doit exercer les échecs suivants :

1. le faux probe produit les mêmes totaux mais un digest différent sous `--threads=tmax` : validation refusée ;
2. le faux probe ignore `--threads=8` et annonce `threads_effective=1` : validation refusée ;
3. `nproc` ou `cpu_set` manque dans un statut `scale_threads` : validation refusée ;
4. le budget restant est inférieur au timeout du run suivant : le run n'est pas lancé, un statut explicite `not_run_budget` est conservé, et la session est `partial_or_failed` sans attendre l'arrêt forcé ;
5. dans la version séparée recommandée, une session A complète reste recevable même si la session B est préemptée, avec deux manifestes distincts et aucune confusion entre leurs statuts.

---

## 4. Ordre de travail recommandé

1. Conserver tel quel le tampon de counting sort et le profil de `fa7cac4`.
2. Séparer ou réordonner la campagne afin que `scale_threads` soit exécutée dans un budget réaliste.
3. Graver le nombre de fils, `nproc`, l'affinité et les arguments dans les statuts.
4. Ajouter une signature canonique et confronter `t8/tmax` à `n=32000`.
5. Lancer ensuite les cellules `n=64000`.
6. Utiliser seulement ces reçus appariés pour décider entre scan dense GPU, index q3 par niveaux peu profonds et autres structures.

## Conclusion

Le code de génération continue d'avancer dans la bonne direction et le nouveau profil fournit une information utile : q3 et le cover méritent désormais le prochain effort structurel. Mais la campagne ajoutée dans `a694496` place ses expériences décisives après un volume de travail incompatible avec la fenêtre de six heures, puis ne confronte pas leurs objets lorsque le nombre de fils change.

Fermer ces deux raccords est peu coûteux par rapport au prix d'une session G4 et évite le pire résultat expérimental possible : une campagne déclarée complète dont les runs à 8 et 48 fils n'ont pas calculé la même hiérarchie.