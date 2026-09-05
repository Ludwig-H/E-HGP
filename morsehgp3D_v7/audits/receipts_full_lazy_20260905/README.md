# Reçus indépendants : port FULL paresseux

5 septembre 2026, header capturé `13c6cc72`. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

Le [rapport courant](../CACHE_FULL_COURANT.md) porte les conclusions. Ce dossier sépare les preuves suivantes :

| Pièce | Autorité |
| --- | --- |
| [Lecture sémantique](semantic_review.md), [sources et diff](semantic_review.json) | Minima, cache, J1, ancres muettes, lots, budgets et compatibilité EAGER ; aucun run attribué à cette lecture. |
| [Capture des sources compilées](source_pins.json), [clôture exécutée](execution_closure.json) | Dix-neuf en-têtes littéraux, deux nominaux O2/ASan-UBSan et trois mutants privés. CPU0 ; compilations et moteurs clos à 17:42:45 UTC, aucun benchmark. |
| [Fixtures](fixtures.json), [transport](fixtures.txt) | Cent ordres historiques explicitement liés sous hashes et neuf nouveaux ordres recalculés rationnellement. Le format compact évite de recopier quatre fois les anciennes données. |
| [Jugements normal](judgments_normal.json), [optimisé](judgments_optimized.json) | 872 sorties par build nominal, 67 920 coupes, 654 comparaisons croisées, 218 égalités sans skip ; 16 plafonds exacts, 180 cap−1, douze conflits d’API. Trois réfutations causales et 218 témoins EAGER inchangés pour chaque mutant. |
| [CTests constructeur](constructor_review.md), [liaisons](constructor_review.json), [captures portables](constructor_review/capture_manifest.json) | 14+14 CTests propres, précontrôle 12/14 conservé, 434 fautes d’allocation ; JUnit allocation tronqué explicitement, journaux complets conservés. |
| [Digest et sonde](digest_probe_review.md), [sources](digest_probe_review.json), [modèles normal](digest_review/normal.json), [optimisé](digest_review/optimized.json) | Canonicité du wire dans son domaine, quatre lacunes historiques corrigées, contre-fixture first-C du v2 gelé. Modèles de compteurs distincts de toute admission moteur. |
| [Raccord public des CTests](publication_binding.md), [pins](publication_binding.json) | 198 fichiers publics, 190 captures identiques à la campagne privée close. |
| [Admission de la sonde](probe_admission_review.md), [rejeux](probe_admission_review.json) | 469 fichiers publics, 24 reçus n=8 et 156 lignes, onze refus de parsing ; digests appariés, géométrie non rejugée par les empreintes. |
| [Supplément first-C](first_c_companion_review.md), [clôture](first_c_companion_review.json) | Composition obligatoire avec le v2 gelé et le sceau ; contre-fixture réelle réfutée, aucun moteur relancé. |

Le [delta source final](source_delta_review.json) rattache la variante J aux sources relues et aux deux paquets publics. Les [contrôles de publication](repository_checks.json) couvrent les documents maintenus ; le [nettoyage ciblé](cleanup.json) retire seulement les copies privées et un précontrôle positif remplacé par les verdicts finaux. Les échecs et contre-fixtures sont conservés.

Chaque binaire a ses fichiers `_build.json`, `_run.json`, dépendances, stdout et stderr. Les captures C++ nominales sont identiques octet pour octet ; aucun diagnostic compilateur ou sanitizer, avec LeakSanitizer actif. Les patches `reject_j1`, `collapse_minima` et `ignore_capacity` modifient seulement des copies privées. Une erreur de transport ou de processus ne suffit jamais à compter une réfutation.

## Reproduction

Depuis la racine, les juges ne lancent aucun moteur :

```bash
taskset -c 0 python3 -B morsehgp3D_v7/audits/full_lazy_run.py judge
taskset -c 0 python3 -B -O morsehgp3D_v7/audits/full_lazy_run.py judge
```

Ces commandes régénèrent les petits verdicts de jugement à partir des bruts immuables. La reconstruction doit utiliser les sources capturées et les commandes exactes des reçus de build, dans un nouveau temporaire sous `audits/`. Les chemins des copies privées de mutants se recréent depuis les patches ; aucun header produit courant n’est substitué silencieusement. Le `prepare` de la campagne refuse d’écraser une capture déjà présente.

Les sorties de géométrie ne sont ni une mesure massive, ni une admission de la sonde avec digest. Aucun résultat réduit F ou ancienne mesure EAGER n’est renommé résultat lazy. GCP non utilisé.
