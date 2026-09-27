# Captures de l'arène collective q34

Voir [l'audit et ses limites](../../audits/b_q34_collective_arena_20260927/README.md).
Aucun moteur/GCP modifié. Préparation CPU seulement, sans S2/S3 ni FULL.

- Racine : première capture conservée, 11 commandes, échec de la porte
  sanitaire dû à LeakSanitizer sous ptrace. Release et ses deux mutants
  passent. Aucun résultat de mesure clos dans cet essai.
- `r2/` : mêmes sources, builds neufs, 23 commandes closes, Release et
  Clang ASan/UBSan/LSan, quatre exécutions mutantes, dix mesures appariées
  un/quatre workers. L'autorité de résultats est `r2/summary.json` avec
  la fermeture des hashes et les lectures LIVE, pas le premier essai.
  `r2/checks/checks.json` clôt les lectures normale/`-O` identiques et huit
  corruptions causales du reçu toutes rejetées.
- `supplement/` : gate API complémentaire autonome, sans modifier les
  sources gelées des deux captures. Neuf commandes PASS, Release et
  Clang ASan/UBSan/LSan : 78 refus à motif exact, 18 refus de requête,
  18 contrôles d'alias, 622 rangs déplacés, 763 IDs non identiques aux rangs.
  Lectures normale/`-O` PASS. Son statut vient de son propre `capture.json`
  et de son `summary.json`, jamais hérité de r2.

Commande de capture r2 (exécutée depuis le worktree propre de reprise) :

```sh
python3 -B morsehgp3D_v9/audits/b_q34_collective_arena_20260927/run.py --capture morsehgp3D_v9/receipts/q34_collective_arena_20260927/r2 --build-prefix /workspaces/E-HGP/build/v9-audit-q34-collective-r2-20260927
```

Lecture normale ou optimisée :

```sh
python3 -B morsehgp3D_v9/audits/b_q34_collective_arena_20260927/run.py --readback morsehgp3D_v9/receipts/q34_collective_arena_20260927/r2
python3 -O -B morsehgp3D_v9/audits/b_q34_collective_arena_20260927/run.py --readback morsehgp3D_v9/receipts/q34_collective_arena_20260927/r2
```

Les builds `v9-audit-q34-collective[-r2]-20260927_{release,sanitize}` sont
épinglés et ne doivent pas être réutilisés pour compiler une nouvelle
source. La sonde réutilise les archives générateur immuables du 26 septembre
sans les recompiler ; leurs hashes sont inclus avant/après. Les bibliothèques
réutilisées ne transfèrent aucune qualification antérieure au nouvel objet.

Les pics mémoire concernent les capacités des tableaux d'un constructeur,
pas le RSS du benchmark : les deux arènes de comparaison coexistent.
Les cas synthétiques restent des diagnostics 8k/16k/32k ; la trame sans sol
08/000000 est entière après masque figé, mais seule sa préparation Pool est
chronométrée. Le front CPU, le fallback et l'aval restent des postes distincts.

Complément causal, dans ses propres builds et reçus :

```sh
python3 -B morsehgp3D_v9/audits/b_q34_collective_arena_20260927/supplement/run.py --capture morsehgp3D_v9/receipts/q34_collective_arena_20260927/supplement --build-prefix /workspaces/E-HGP/build/v9-audit-q34-collective-supplement-20260927
python3 -B morsehgp3D_v9/audits/b_q34_collective_arena_20260927/supplement/run.py --readback morsehgp3D_v9/receipts/q34_collective_arena_20260927/supplement
python3 -O -B morsehgp3D_v9/audits/b_q34_collective_arena_20260927/supplement/run.py --readback morsehgp3D_v9/receipts/q34_collective_arena_20260927/supplement
```

Ses builds `v9-audit-q34-collective-supplement-20260927_{release,sanitize}`
sont également épinglés. Les dossiers existent déjà : les commandes de
capture sont des recettes de provenance, pas une autorisation d'écrasement.
