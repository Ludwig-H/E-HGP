# Exécution mono directe : état vérifié

4 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

Le chemin `threads=1` avec `fold_join_before_next_k=true` exécute le corps du fold B sur le thread appelant. Les quatre portes dédiées passent sur une copie courante isolée : zéro appel à `pthread_create` dans les runs mono observés, mêmes objets que la route asynchrone, refus et préfixes provisoires attendus. Cela qualifie ce changement d'ordonnancement sur les fixtures décrites ; aucune mesure de performance industrielle ni qualification globale n'en est déduite.

Le census courant passe aussi ses [six portes AxisBounds indépendantes](CENSUS_AXIS_COURANT.md). Le CLI courant C `25c9bf8e…` a été reconstruit et ses interfaces rejouées ; la [qualification C](AUDIT_QUALIFICATION_20260905.md#autorités-des-mesures-et-du-cloud) précise ce rattachement. Les quatre portes mono décrites ci-dessous restent attachées à leur propre snapshot : leurs sources de fold et de test sont inchangées, et leurs reçus bruts sont conservés.

Les sources, commandes complètes, stdout/stderr et hashes sont conservés dans [mono_current.json](receipts_20260904/mono_current.json), avec le [JUnit courant](receipts_20260904/mono_ctest_current.xml). La phase courante vérifie 110 fichiers identiques avant copie, après copie et après construction/exécution ; les deux binaires restent identiques avant et après les tests. Une première observation de stabilité avait détecté uniquement la correction du nom de version dans le commentaire initial de CMake ; elle est conservée dans le reçu, puis la copie a été actualisée et requalifiée.

| Élément qualifié | SHA-256 |
| --- | --- |
| `src/pipeline/run.hpp` | `1999f901fb44caf3ca743e77e64bb3e5765070fa01a369447b9e89be21ce728c` |
| `CMakeLists.txt` | `25030fb015309b44685101057f9259fc6b39847681777775119cf2905308bf73` |
| Binaire `mhgp7_mono_inline_gate` | `090c268a51a223119823397c25a7dd97cd8777f2a57f262f0232eece1290fd58` |
| Binaire produit `mhgp7` | `c7da95a3a83c1e31fdfd95db852fed86f43208e6b1b051dfb36e78baf45e5175` |

## Revue de l'ordonnancement et des erreurs

[run.hpp:830](../src/pipeline/run.hpp#L830) définit un unique corps `fold_b`, qui possède le `Stage` par capture déplacée. [run.hpp:1026](../src/pipeline/run.hpp#L1026) l'appelle directement si et seulement si les deux conditions mono et jonction sont vraies ; les autres configurations créent un `std::thread` avec ce même corps. Aucun vecteur d'événements n'est copié par ce choix.

La jonction immédiate permet une preuve simple d'absence d'attente circulaire sur `next_publish`. Initialement il vaut 1. Après un ordre réussi, le corps publie, libère son stage, fixe `next_publish=K+1`, puis `reap_front` consomme le slot avant l'ordre suivant. Un défaut empêche cette continuation. L'appel inline ne peut donc attendre un prédécesseur qui nécessiterait le même thread pour terminer. Les chemins de capture d'exception, décision ordonnée, drainage et invalidation finale sont partagés avec la route asynchrone.

Le changement de thread des callbacks est observable et fait partie de ce mode : `on_forest` et les phases B s'exécutent sur l'appelant. L'identité des objets n'implique pas l'identité de tous les effets sur les données locales au thread. Les callbacks doivent respecter le contrat d'environnement numérique stable, notamment s'ils modifient le mode d'arrondi ou d'autres états flottants. La génération initiale précède les callbacks ; cela n'autorise pas une modification arbitraire de l'environnement utilisé par les phases suivantes.

Le garde `Inflight` de [run.hpp:836](../src/pipeline/run.hpp#L836) reste exécuté dans le chemin direct. `peak_fold_inflight`, ainsi que le libellé de profil `pic_workers_b`, désignent ici l'activité du corps B ; ils peuvent valoir 1 quand aucun thread n'est créé. Pour qualifier un vrai mono, le témoin indépendant est le nombre d'appels réels à `pthread_create`, pas ce compteur de phases. Une clarification du libellé de profil serait utile avant d'en faire un indicateur de threads natifs.

## Ce que les quatre portes vérifient

La [fixture dédiée](../tests/mono_inline_gate.cpp) contient 11 points u16 fixes. L'interposition dynamique de `pthread_create` est d'abord vérifiée par un vrai `std::thread` créé puis joint ; elle observe ensuite les appels issus de la libstdc++ liée. Sa portée est celle de cet ABI Linux, pas celle de toutes les bibliothèques possibles.

| Porte | Résultat et non-vacuité |
| --- | --- |
| `mhgp7_mono_inline` | Quatre tours complètes : K=1..5 et K=1..10, avec et sans complétion silencieuse ; dernier ordre non vide, callbacks sur l'appelant, zéro création de thread. Digests globaux, digests par K, cartes et ordre des callbacks identiques à la route asynchrone. |
| Même porte : contrôles de la condition | Avec `threads=1` et jonction désactivée, exactement Kmax threads sont créés, 30 au total sur les quatre références. Avec `threads=2` et jonction activée, la route demeure réellement threadée. |
| Même porte : callbacks adverses | Huit cas, avec et sans complétion : exception de callback forêt K2, phase de début K2, phase publiée K2 et `bad_alloc` tardif K3. Préfixes provisoires attendus et zéro création de thread ; `bad_alloc` devient `resource_exhausted` avec payload invalidé, les exceptions ordinaires sont propagées. |
| `mhgp7_mono_inline_late_a` | Défaut d'expansion K2 : `invariant_violated`, seul callback K1, payload final invalidé, deux sémantiques. |
| `mhgp7_mono_inline_late_b` | Exception B à K3 : exception propagée, callbacks K1/K2 seulement, deux sémantiques. Aucun résultat terminal complet n'est retourné sur cette exception. |
| `mhgp7_mono_inline_early_alloc` | Allocation refusée au premier B : `resource_exhausted`, aucun callback, payload final invalidé, deux sémantiques. |

La porte nominale annonce `mono_inline_gate=passed successful=4 rejected=8 reference_threads=30 failures=0`. Les trois portes injectées annoncent chacune `mono_inline_injected=passed cases=2 failures=0`. Toutes rendent le code attendu 0. Les deux digests complets K=1..10 sont `5413672add57ef2b2eaf335d25c6b2a47db5a72a1dc8cfb9565fd2d9fe8ad3a6` sans complétion et `206e644a921a48e1c74231f4bbc344e43885e2b687f9fea9d8a035034e5ff9d4` avec complétion.

## Construction et portée mémoire

Compilation Release GNU C++ 13.3.0, `-O3 -DNDEBUG -std=c++20 -Wall -Wextra -Wpedantic -Werror`, CUDA désactivé. La porte est compilée avec `MHGP7_TESTING=1` et liée à `dl` ; le CLI produit est compilé sans cette définition. Les lignes exactes de compilation et d'édition de liens figurent dans le reçu.

Commandes exécutées sur la copie sous `audits/`, avec `TMPDIR` interne et `PYTHONDONTWRITEBYTECODE=1` :

```bash
cmake -S morsehgp3D_v7/audits/.work_mono/source/morsehgp3D_v7 -B morsehgp3D_v7/audits/.work_mono/build -DCMAKE_BUILD_TYPE=Release -DMHGP7_ENABLE_CUDA=OFF -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build morsehgp3D_v7/audits/.work_mono/build --target mhgp7 mhgp7_mono_inline_gate --parallel 2
ctest --test-dir morsehgp3D_v7/audits/.work_mono/build --output-on-failure --no-tests=error -V -R '^mhgp7_mono_inline(_late_a|_late_b|_early_alloc)?$'
```

Une reproduction sans copie peut utiliser `-S morsehgp3D_v7` après vérification des hashes, en conservant le répertoire de construction sous `audits/`. Le produit compilé se trouve dans `audits/.work_mono/build/mhgp7` ; ses tests d'interface sont rattachés à leur reçu propre.

Le [retour mémoire](RETOUR_MEMOIRE_COURANT.md) reste pertinent pour la route avec recouvrement : ses fixtures utilisent la jonction désactivée, et les expressions d'admission sont inchangées dans ce delta. Le chemin mono direct ne présente pas de coexistence A2/B1, puisque B1 termine avant A2. Les shards et la destination d'expansion restent présents. Aucun nouveau pic de RAM ni ajustement de budget n'est mesuré ici ; aucune fixture mémoire n'a été recompilée pour cette requalification ciblée. GCP non utilisé. Aucun code produit modifié par l'auditeur.
