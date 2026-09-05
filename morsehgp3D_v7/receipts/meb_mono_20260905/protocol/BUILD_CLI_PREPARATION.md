# Builder D préparé, sans compilation exécutée

`build_cli.py` est inerte sans `--execute`. Il importe et exécute uniquement les octets épinglés du validateur runner v3 (`3abe27f8…`). Il ne construit ni ne lance quoi que ce soit en prévisualisation.

```bash
python3 -B build/v7_meb_paired/build_cli.py --output build/v7_meb_paired/build_D_receipt
```

Après revue et GO explicite seulement, ajouter `--execute`. Le build neuf par défaut est `build/v7_meb_qualification`; un autre `--build-dir` doit être un descendant neuf de `build/`. Le dossier de reçu doit aussi être neuf et ne doit pas contenir le build ni être contenu par lui. Aucune reprise ni écrasement. Le CLI C reste celui de Release C épinglé à `25c9bf8e…` ; aucun autre CLI privé déjà construit n'est attribué à cette commande.

La séquence réelle demandée sera : versions compiler/CMake (30 s chacune), configuration Release/CUDA OFF/EXPORT_COMPILE_COMMANDS ON (300 s), puis exactement `cmake --build <build_dir_absolu> --parallel 2 --target mhgp7` (300 s). Chaque commande passe par `run_process` épinglé, avec groupe de processus possédé et drain sur sortie/interruption/censure, RLIMIT_AS=26 GiB, LC_ALL/LANG=C. Les valeurs LD d'injection sont refusées sans exposition. Aucune commande MorseHGP n'est exécutée, aucun GPU ou GCP.

Le reçu conserve commandes, UTC, durées, stdout/stderr, codes et censures, versions, HEAD/worktree avant/après, matériel allowlist, source/hash avant/après, pin du builder/runner/helpers et C. Les métadonnées de compilation sont le cache, la base des commandes, l'entrée du CLI et son SHA/taille. Source et C doivent rester identiques à chaque frontière. La liaison D est validée par `candidate_binding`, qui attache le binaire au vrai chemin résolu `build_dir/mhgp7`, à ses sources et à une compilation Release sans instrumentation.

`build_record.provisional.json` n'est jamais l'autorité à fournir au runner de mesure. `build_D.json` n'est publié qu'après tous les contrôles terminaux : état du dépôt récupérable, source/helpers/C stables, binaire/cache/base identiques au record et nouvelle validation de liaison. En cas de refus tardif, seul le brouillon est conservé, avec `summary.status=failed_or_invalid`. Le résultat à accepter exige `summary.status=completed`, `build_D.json` présent et le manifeste final vérifié. Ce lien de build documenté n'est pas un build hermétique.

`build_cli_selftest.py` teste la préparation inerte et neuf scénarios avec sorties de compilation synthétiques seulement : succès, échec, censure, interruption, drift source, échec Git tardif et dérives tardives binaire/cache/base. Chaque échec tardif doit conserver son brouillon et ne jamais publier `build_D.json`. Les selftests se jouent normal et `-O`, sans vraie compilation.
