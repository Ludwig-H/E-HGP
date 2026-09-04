# Fold B inline en vrai mono — overlay non intégré

4 septembre 2026. Aucun fichier produit, test réel, CMake, benchmark ou audit
indépendant n'est modifié. Aucun GCP utilisé. Le fichier source original
`morsehgp3D_v7/src/pipeline/run.hpp` reste au SHA-256
`885348a92f48658642e3783027cb7c4f239f1c8e1a0b91c66a698f3be6b29762`.
Le seul header remplacé, `src/pipeline/run.hpp`, est au SHA-256
`6b9526d896850b94e6455c040ca8bcd038c292c003b1138407e1d57f4fd9f441`.
Les dépendances sont les sources gelées, référencées par liens et chemin
d'inclusion ; ce dossier n'est pas une distribution autonome.

## Changement et invariant de progression

Le corps exact de la lambda B est extrait dans `fold_b`. Captures, possession
du Stage, exceptions, réduction, métriques, attente ordonnée, callback,
publication et libération sont inchangés. Seul son mode d'appel diffère :

```cpp
if (opt.threads == 1 && join_b_now) fold_b();
else sp->t = std::thread(std::move(fold_b));
```

`join_b_now` est `fold_join_before_next_k`. Avec ce flag, chaque itération
précédente a terminé puis retiré son slot avant la suivante ; en succès,
`next_publish==K` à l'entrée du B courant. L'attente de publication est donc
immédiatement satisfaite. En échec, le corps positionne `pub_failed` et le
même `reap_front` observe l'exception/statut puis interrompt la boucle.
Un slot inline possède un `std::thread` non joignable : les chemins existants
de reap/drain/RAII le traitent sans modification. `inflight=1` seul n'aurait
pas établi cet invariant, car A(K+1) pourrait encore coexister avec B(K).

La condition ne change pas le chemin `threads=1, join=false`, ni celui où
`threads>1`. Le protocole vraiment mono exige donc explicitement
`threads=1`, `fold_inflight=1`, `fold_join_before_next_k=true` ; ce patch ne
change pas les valeurs par défaut. L'identité du thread recevant le callback
change intentionnellement dans ce mode : le callback est désormais exécuté
sur le thread appelant. Sa séquence et son autorité provisoire, jusqu'au
succès terminal, restent identiques. Aucune promesse de performance n'est
déduite du temps des fixtures.

## Preuves exécutées

Le test est `tests/mono_inline_gate.cpp`, SHA-256
`cb427753568aafe7ec61f392a0b373b5ebe7c129818d27ce749bb21cebad7eee`.
Il interpose le **symbole réel** `pthread_create` et rejoint son implémentation
par `dlsym(RTLD_NEXT)`. `--wrap=pthread_create` n'est pas employé : il ne
suffirait pas pour les appels issus de libstdc++ partagée. Un trampoline
compte les threads actifs ; chaque retour exige leur nombre nul.

La non-vacuité comporte trois contrôles : un `std::thread` isolé observé
exactement une fois ; le chemin original et le chemin `join=false` créant
exactement K threads par tour ; le binaire original exécuté avec
`--require-zero`, qui échoue avec le code 1 et le diagnostic de création.
Les 30 threads cumulés des quatre tours asynchrones sont observés dans les
deux binaires. Le mode inline exige zéro création, y compris en échec, et
des callbacks sur le thread appelant. `threads=2, join=true` reste threaded.

Le nuage est la fixture u16 régulière de onze positions déjà utilisée pour
K10 dans `silent_incidence_gate.cpp`. Le dernier ordre possède effectivement
des événements. Les quatre tours entières 1..5 et 1..10, chacune en mode
compatible et en complétion d'incidences, rendent le même statut, le même
digest global, les mêmes digests par K, cartes, callbacks et phases que le
binaire original compilé avec la même sonde. La comparaison externe porte
sur les octets stdout complets, sans retirer une divergence sémantique.

Les huit pannes de callbacks sont les quatre sites suivants, dans chacune
des deux sémantiques : `on_forest(K2)`, début de réduction K2, observation
de publication K2, et `bad_alloc` dans `on_forest(K3)`. Les exceptions
ordinaires continuent de se propager ; `bad_alloc` devient le même refus
contrôlé avec tous les champs de payload invalidés. Les préfixes de callbacks
restent provisoires et sont exactement ceux attendus.

Trois injections existantes sont lancées dans des **processus séparés**,
avant tout appel du pipeline, car les sites mutants mémorisent leur activation :
échec A à K2, exception B à K3, allocation impossible B à K1. Chacune est
exercée dans les deux sémantiques, avec les mêmes préfixes, messages d'exception
et phases entre original et overlay. Aucun nouveau mutant produit n'est requis.

| Qualification locale | Résultat |
|---|---|
| Deux builds O2, C++20, avertissements en erreurs | code 0 |
| Nominal original et overlay : quatre tours + huit pannes callbacks | codes 0 ; stdout identiques |
| Trois injections, deux sémantiques, original et overlay | six invocations code 0 ; trois comparaisons stdout identiques |
| Original soumis au contrat zéro pthread | code 1 attendu ; défaut effectivement observé |
| Overlay ASAN + UBSAN + détection de fuites : nominal et trois injections | quatre codes 0 ; stdout identiques au build O2 ; aucun diagnostic sanitizer |
| Patch contre les octets source examinés | `git apply --check` code 0 |

Autorités : `receipt.json`, `sanitizer_receipt.json`, stdout/stderr individuels.
Les binaires ne sont pas à versionner. Les résultats sont propres à cet
overlay et n'appartiennent pas au reçu Release 279 portes du produit gelé.

## Commandes et intégration proposée

Depuis la racine du dépôt :

```bash
g++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror -pthread -DMHGP7_TESTING=1 -I/workspaces/E-HGP/morsehgp3D_v7/src/pipeline build/v7_mono_inline_fix/tests/mono_inline_gate.cpp -ldl -o build/v7_mono_inline_fix/mono_inline_gate
g++ -std=c++20 -O2 -Wall -Wextra -Wpedantic -Werror -pthread -DMHGP7_TESTING=1 -DMHGP7_MONO_REFERENCE=1 build/v7_mono_inline_fix/tests/mono_inline_gate.cpp -ldl -o build/v7_mono_inline_fix/mono_reference_gate
python3 -B build/v7_mono_inline_fix/run_gate.py
g++ -std=c++20 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -Wall -Wextra -Wpedantic -Werror -pthread -DMHGP7_TESTING=1 -I/workspaces/E-HGP/morsehgp3D_v7/src/pipeline build/v7_mono_inline_fix/tests/mono_inline_gate.cpp -ldl -o build/v7_mono_inline_fix/mono_inline_gate_asan
python3 -B build/v7_mono_inline_fix/run_sanitized.py
git apply --check build/v7_mono_inline_fix/mono_inline.patch
```

`mono_inline.patch` est prêt, non appliqué. Après autorisation de levée du
gel, intégrer uniquement le delta de lambda, porter la sonde vers un chemin
de test maintenu sans référence absolue au workspace, enregistrer les quatre
modes et le témoin de non-vacuité dans CMake, puis requalifier le produit réel.
Si d'autres corrections ont changé `run.hpp`, rebaser le petit delta sur les
nouveaux octets et refaire ces comparaisons ; ne pas recopier ce header entier
par-dessus des modifications concurrentes. Le contrat reste
`public_status=not_claimed`.
