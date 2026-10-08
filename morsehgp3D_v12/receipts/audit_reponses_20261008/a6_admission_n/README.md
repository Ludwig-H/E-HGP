# A6 — admission du scratch de numérotation N

8 octobre 2026, Codex. Proposition pour **CST-0020**, sans changement d'état.
Source livrée `30a69104a697fa2ea2dadd8499bde6c7cb8d72c0` ; neuf empreintes dans
[capture.json](capture.json). Lecture statique seulement : aucune implantation,
exécution native ni mesure de mémoire ou de temps.

## Borne par tâche

Dans la Session recouverte, N appelle `number_births_range`, par morceaux de
`G = kNumberItems = 16384` naissances. Une cohorte est un bloc maximal de rang
égal. Son propriétaire est **le morceau contenant son début** ; il traite la
cohorte entière, même si elle dépasse plusieurs frontières de morceaux. Les
morceaux suivants sautent cette continuation.

Pour chaque tâche `j`, soit `a_j` la plus grande taille d'une cohorte qu'elle
possède, ou zéro si elle ne possède aucune cohorte de taille au moins deux.
Poser également `a_j = 0` à **k = 1** : cette voie n'alloue aucun des deux tampons
considérés. Pour k ≥ 2, la tâche alloue une seule fois ses deux `Buffer`, aux
tailles `a_j` en `num::Sphere` et en `u32`. Poser
`α = sizeof(num::Sphere) + sizeof(u32)` ; conserver les `sizeof`, sans figer un
nombre d'octets indépendant du profil.

Si `a_(r)` désigne les maxima de tâches triés par ordre décroissant, une borne
du scratch **logique vivant** est :

```
S_N = α × somme des min(W, J) plus grands a_j
```

`W` est le nombre de participants du Pool, appelant compris, et `J` le nombre
de tâches N. Chaque participant exécute un seul `run_job` synchrone à la fois.
Les deux buffers locaux sont détruits avant le retour de `number_births_range`,
y compris après un refus ; une allocation partielle reste sous la même borne.
Il existe donc au plus W tâches N avec scratch vivant simultanément. Leur somme
est au plus celle des W plus grands maxima. Les réservations de tâches sont
uniques ; le Pool refuse la réentrance. La preuve ne suppose pas que les W plus
grandes tâches puissent effectivement coïncider : c'est une borne suffisante.

Prendre les maxima **par ordre** serait insuffisant : deux tâches du même ordre,
de tailles 100 et 90, peuvent coexister avec W = 2. Tronquer une cohorte à la
frontière du morceau serait également incorrect.

## Remplacement local dans `region_bytes`

Le code additionne actuellement le terme `α × W × M`, où M est la plus large
cohorte des ordres k ≥ 2, **et** les `kernel_bytes` de tous les ordres. Chacun de
ces derniers contient déjà `α × w_k` pour sa plus large cohorte (valeur plancher
1 comprise). Le chemin recouvert utilise la numérotation par morceaux ; il ne
réexécute pas la numérotation séquentielle qui justifiait ce second terme.

Pour K ≥ 2, la proposition est donc :

```
A_nouveau = A_actuel − α × W × M − α × somme(k=2..K, w_k) + S_N
```

À K = 1, ces termes sont tous absents. Retirer le doublon **uniquement dans la
composition de `region_bytes`** : conserver `kernel_bytes` pour ses autres
appelants, notamment le chemin séquentiel. Préserver tous les autres termes de
l'admission et vérifier les domaines et opérations de taille avant calcul.
La proposition ne certifie pas les autres majorants du budget global.

## Préparation et limites

Une passe sur les cohortes, regroupées par `floor(début/G)`, suffit à obtenir les
maxima des tâches. Un tas minimal de capacité W conserve les plus grands : coût
`O(B + J log(W+1))`, stockage auxiliaire `O(W)`, B étant le nombre total de
naissances. Aucun tableau par naissance ou par tâche n'est nécessaire. On peut
fusionner les deux parcours actuels de `widest_cohort`. Une préparation parallèle
par ordre peut garder W valeurs par ordre puis les fusionner, avec `O(KW)` de
stockage : une valeur écartée localement possède déjà W concurrents au moins
aussi grands. **Le stockage de préparation doit lui aussi être admis** ; ne pas
le déplacer implicitement hors budget avant l'admission principale.

Le cache arrondit les deux allocations séparément. `S_N` ne désigne donc pas
leur taille physique exacte : conserver la politique de classes et la marge
existante `bytes/8` de `MemoryBudget::admit`, sans les supprimer au titre de ce
lemme. Une vérification d'implantation devra maintenir cette garantie pour les
tailles arrondies. Les blocs inactifs peuvent rester dans `held` après le retour
d'une tâche ; ils ne sont pas du scratch vivant. Cette formule ne borne ni le
RSS ni le cache inactif et ne promet aucune diminution de leur pic. Aucun gain
de mémoire mesuré, aucune accélération et aucune qualification FULL n'en découlent.

## Pointeurs au pin

- `src/tower/pipeline.cpp:16–48,177–202` : préparation, somme actuelle, admission,
  lancement des W participants.
- `src/tower/forest_build.cpp:34–43` : terme historique par ordre.
- `src/tower/forest_births.cpp:118–145` : propriétaire de cohorte, tailles et
  durée de vie des deux buffers ; `pipeline_steps.cpp:21–41` : appel par morceau.
- `src/tower/pipeline_run.cpp:351–369` et `src/sched/pool.cpp:38–42,57–86` :
  exécution synchrone, participants et refus de réentrance.
- `src/core/buffer.hpp:112–120` et `buffer.cpp` : admission, cache et réservations.

Les sources sont désignées par Git et leurs SHA-256, sans copie de leur contenu.
Ce reçu complète l'avis d'admission ; il ne modifie pas les preuves précédentes
de concurrence A6 ni l'obligation de préfixe CST-0242.
