# Première occurrence FULL réellement multi-CPU

27 septembre 2026, base `fd1a2c7ee`, audit-only. Cadre
`exploration_v9_hors_registre`, `cpu_reference`, `quantized_u18_input_only`,
`mode=parallel_first_parent_structural_encode`, `public_status=not_claimed`.
Moteur inchangé, aucune utilisation GCP. Ce lot qualifie la concurrence,
pas une performance ni une nouvelle tour FULL complète.

## Ce qui est parallèle et pourquoi c'est exact

Port explicite de `b_full_first_parent_20260927/first.hpp` figé en
`fd1a2c7ee`. Les types, comparateur et règles de refus restent attribués à
ce prototype et au produit. La forme CSR et le domaine sont contrôlés
avant lecture indexée. La recherche d'une continuation parcourt tout le
draft : si une action a un parent unique, repli intégral inchangé sur
`first::encode`, sans résultats partiels du chemin rapide.

Sinon A actions donnent A nœuds et chaque utilisation d'un parent est une
fusion définitive. Les opérations parallèles sont séparées par des joins :

1. Affecter le lot de chaque action ; initialiser `first[A]` à absent.
2. Sur toutes les occurrences j, si `parent[j]<A`, réduire `first[parent]`
   au minimum j par CAS. Le tableau de parents CSR donne déjà l'ordre
   lot/action/position ; l'inversion du parcours couvre aussi cette phase.
3. Vérifier actions et en-têtes avec une erreur privée par worker, puis
   réduire leur minimum canonique après tous les joins. Antériorité au lot,
   ordre strict des parents, populations, masques et naissance sont conservés.
4. Après admission seulement, écrire nœuds et contributions aux positions
   disjointes. Chaque parent admis a une seule occurrence : son successeur
   n'a donc qu'un écrivain.

`atomic_ref<u64>` est utilisé uniquement pendant la phase de minimum.
L'alignement requis est vérifié à la compilation ; aucune lecture ordinaire
n'est concurrente d'un accès atomique. L'initialisation précède les threads
et leurs joins précèdent toutes les lectures ordinaires, donc l'ordre
atomique `relaxed` suffit. Sur les builds présents : cellule 8 octets,
référence 8 octets, alignement requis 8, `is_lock_free()==true`. La sûreté
ne dépend pas du caractère lock-free.

Le draft est **emprunté immuable pendant tout l'appel synchrone**. La banque
est authentique, détenue par un `shared_ptr<const ...>` partagé : cela ne
répare pas un alias mutable éventuellement échappé par l'ancienne factory
publique. Les fixtures utilisent la factory qui copie ; la précondition
d'immuabilité reste explicite pour tout autre appelant.

## Ordonnancement, erreurs et limites de travail

Les grains sont distribués cycliquement. Pour N éléments, le nombre de
workers est `min(W,ceil(N/grain))`, zéro si N=0 : tous les workers lancés
ont un travail non vide. Les multiplications et incréments de jobs restent
bornés par N ; W=0 ou grain=0 est refusé. Chaque worker possède ses slots
d'erreur. Un échec de création ou une exception worker arrête la phase,
joint les threads déjà lancés, puis propage l'échec. Aucun objet de sortie
partiel n'est publié. Les refus métier restent des minima lexicographiques,
jamais « le premier thread arrivé ».

Les refus d'allocation/taille sont conservés ; l'indisponibilité système
d'un thread produit `coverage_worker_start_failed`, ressource épuisée.
L'équivalence porte sur les objets et premiers refus sémantiques lorsque
les ressources suffisent, pas sur le numéro d'une allocation défaillante.

Le travail **logique** est O(B+A+P+C). Ne pas le transformer en borne
inconditionnelle du nombre de tentatives CAS : les parents répétés des
drafts invalides provoquent de la contention et `compare_exchange_weak`
autorise des échecs parasites. Les retries ne sont pas instrumentés ici.
Sur un draft admis, les parents sont uniques, donc pas de contention réelle
sur leurs cellules, mais aucune borne stricte de retries C++ n'est revendiquée.
Il ne s'agit toujours pas d'une borne sous-quadratique en nombre de points.

La parallélisation n'est **pas complète** : validation CSR/domaine et
détection de continuation restent scalaires ; `resize` initialise les
vecteurs `batch`, `first`, nœuds et contributions ; les successeurs sont
initialisés et les parents copiés avant dispersion. `first` subit une
initialisation de valeur puis le remplissage absent parallèle. Tout est
payé. Un gros lot affecte son intervalle dans une seule tâche, une grosse
multifusion valide/disperse ses incidences dans une seule tâche. Des threads
sont recréés à chaque phase. Ces points sont des limitations de performance,
pas des étapes prétendument gratuites.

Scratch principal : `A*sizeof(size_t)+A*sizeof(u64)` soit 16A octets ici,
plus les erreurs de validation O(W), et à chaque phase les vecteurs privés
d'exceptions, d'activité et de handles de threads O(W). Ajouter les piles
des threads, métadonnées d'allocateur et sorties pour une mémoire réelle.
Ce chiffre n'est ni un pic RSS ni la mémoire d'un backend GPU.

## Qualification close

Capture courante `receipts/full_parallel_parent_20260927/r2` : **14 commandes**,
GCC Release et Clang ASan/UBSan/LSan, sorties identiques. Par binaire :

- 372 entrées × six configurations = 2 232 comparaisons ;
- W1/grain1, W4/grain1, W4/grain7, chacune en parcours normal et inverse ;
- 65 entrées admises / 307 refusées, 1 722 voies rapides / 420 replis /
  90 refus de forme ou domaine avant choix de chemin ;
- 13 644 créations de threads et 114 672 occurrences parent cumulées ;
- quatre workers réellement actifs sur la réduction des parents ; une
  barrière indépendante vérifie quatre IDs de threads simultanés ;
- douze contrôles ordonnanceur, bornes de grains, injection d'échec avant
  zéro/un/deux threads lancés, exceptions workers, vide et options invalides.

Les fixtures prioritaires sont réutilisées explicitement via inclusion
des helpers figés, sans les modifier. Trente historiques K1..10 sont testés
avec continuations puis sans, avec neuf variantes invalides. Un stress
contient 2 048 naissances/1 024 fusions ; un suffixe répète 2 048 fois les
mêmes deux parents et contrôle le premier refus quand une population
antérieure est invalide. Les doubles défauts, parents UINT64_MAX, parents
d'un lot futur, niveaux représentés différemment et multifusions 32 parents
ne sont pas réduits à des chaînes binaires.

Les 9 647 entrées de la qualification scalaire précédente n'ont **pas** été
rejouées sous chaque configuration parallèle : ce corpus ciblé est distinct.
Trois mutants par binaire sont réfutés sémantiquement, code 1 sans crash :
doublon ignoré, premier défaut rencontré, maximum d'occurrence au lieu du
minimum. Le premier est exclusivement W1 et son option W>1 est refusée,
pour ne pas introduire volontairement une race de successeurs. Le mutant
maximum est testé W4 et reste atomique.

**ThreadSanitizer GCC a passé le même gate**, directement, dès la première
tentative : quatre commandes séparées dans `tsan_r1`, stderr vide, même
sortie et digest 3935095278205354091. Aucun contournement ptrace, aucune
répétition après un échec environnemental. Les lecteurs r2 et TSan passent
normal/−O, contrôlent sources, dépendances, binaires et commandes, sans
recompiler ni relancer les tests clos.

R1 a échoué avant exécution : `-Werror=misleading-indentation` dans le gate.
Sorties et copies exactes des trois sources initiales sont conservées dans
`r1/`. Aucun avertissement retiré ; r2 est une construction neuve.

Builds épinglés, ne pas reconstruire :
`/workspaces/E-HGP/build/v9-audit-full-parallel-parent-20260927-r2` et
`/workspaces/E-HGP/build/v9-audit-full-parallel-parent-tsan-20260927-r1`.

```bash
python3 -B morsehgp3D_v9/audits/b_full_parallel_parent_20260927/run.py check
python3 -B -O morsehgp3D_v9/audits/b_full_parallel_parent_20260927/tsan.py check
```

La mesure de vrais drafts doit rester dans une génération séparée. Les
présents reçus ne mesurent aucun gain, ne produisent pas les verticales et
ne qualifient aucun contrat GPU/FULL 100 ms.
