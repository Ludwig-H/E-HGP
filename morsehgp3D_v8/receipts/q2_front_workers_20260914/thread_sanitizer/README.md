# ThreadSanitizer — porte parallèle q2

Le 14 septembre 2026, la porte `mhgp8_wspd_q2_parallel_gate --selftest` passe réellement sous ThreadSanitizer Clang 18.1.3, dans le contexte `workspace`, sans élévation ni changement des contrôles d'espace d'adressage. Le build est neuf, Debug, `MHGP8_SANITIZE=OFF`, avec `-fsanitize=thread -fno-omit-frame-pointer` à la compilation et à l'édition de liens. Cette option CMake désactive uniquement le couple ASan/UBSan du projet ; elle ne désactive pas l'instrumentation ThreadSanitizer explicitement ajoutée.

La portée est une porte CPU bornée : 184 537 contrôles, 438 essais parallèles dont 232 multi-workers, 45 161 supports, réentrance, durée de vie du propriétaire, exception du callback et échec partiel de création des threads. Aucun diagnostic ThreadSanitizer n'est observé ; `stderr` est vide. Cela ne prouve pas l'absence universelle de courses et ne qualifie ni la tour FULL, ni le contrat 50k, ni le GPU.

## Capture et coût

La seule tentative est [prepare_workspace_yncai04j](prepare_workspace_yncai04j/COMPLETION.json). Chaque commande conserve ses arguments, stdout/stderr bruts et encodés, code de sortie, durée et coût CPU. Tous les codes valent zéro.

| Étape | Temps écoulé | CPU utilisateur + système |
| --- | ---: | ---: |
| Configuration | 2,026 s | 1,707 s |
| Construction de la seule cible, deux tâches | 7,138 s | 10,816 s |
| Porte ThreadSanitizer | 2,508 s | 3,198 s |

La capture complète dure 12,299 s. Aucun GCP, coût cloud nul. Ces durées sont celles d'une qualification Debug instrumentée, pas des mesures de performance produit ; d'autres qualifications locales étaient actives. Le RSS fourni par `getrusage` est un maximum cumulatif des enfants du runner, pas une mesure isolée de la porte.

## Empreintes et rejeu

La capture épingle HEAD `329e5b86402d17a69815151c201c3bffec25ff7e` et son worktree, ainsi que les sources réellement compilées. Sources et artefacts sont identiques avant/après exécution. Toutes les empreintes de fermeture ont été revérifiées.

- Binaire : `d353ff78522650d86b838f8fd3ac9da52f22709c299ff28651e3bafc6c8bb614`.
- Runner : `7e93a93fb059f0a62d31a836e0700bdb8a89ae42fb7e6a709c23aa5498ac9025`.
- `COMPLETION.json` : `d26ee51a563b253553ec7e5f738d2aeb2a750c64b18b5cc9b9b4c05eba4bc8a8`.

Le [snapshot du runner](runner_snapshot.py) conserve exactement le script exécuté, prévu à l'emplacement `build/v8_front_workers_tsan_20260914/record_tsan.py`. Il n'est pas exécutable directement depuis ce dossier de reçus : son calcul de racine dépend de son emplacement original.

Commande initiale, désormais historique car elle refuse de reconfigurer ce build épinglé :

```bash
python3 -B build/v8_front_workers_tsan_20260914/record_tsan.py prepare --execution-context workspace
```

Le rejeu sur le même binaire, sans reconstruction et avec création d'un nouveau dossier de tentative, est :

```bash
python3 -B build/v8_front_workers_tsan_20260914/record_tsan.py execute --execution-context workspace
```

Le runner transmet exactement `TSAN_OPTIONS=halt_on_error=1:exitcode=66`. Les commandes CMake intégrales sont dans [configure.json](prepare_workspace_yncai04j/configure.json), celles de construction dans [build.json](prepare_workspace_yncai04j/build.json). Toute indisponibilité future du runtime ou tout échec de rejeu doit rester conservé et ne remplace pas cette capture par un succès supposé.
