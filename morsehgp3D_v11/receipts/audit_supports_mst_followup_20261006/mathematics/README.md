# Contrelecture du différentiel supports v2

Le juge WIP capturé reproduit le filtre incorrect `role != interne` et projette aussi la sortie native avant comparaison. Le patch privé compare cette sortie telle quelle à une sélection indépendante de l'oracle, par Kruskal sur les composantes ouvertes. Il corrige également une attente impossible : sur `sphere9_25`, la boule centrale à 25 sites est interne et absente à K1/K2.

Source WIP au HEAD `9eee2ed4bcef1e960cdf2456012b84416854dc20`, capturée deux fois à octets identiques. Les trois sources de l'oracle exact sont relues par `git show` à ce même pin et vérifiées par SHA256. Le patch est proposé, non appliqué au développeur ; aucune exécution native ou cloud.

Depuis ce dossier, avec le dépôt Git contenant ce pin :

```sh
python3 -S -B replay.py --repo /workspaces/E-HGP
python3 -O -S -B replay.py --repo /workspaces/E-HGP
sha256sum -c SHA256SUMS
```

Les deux exécutions finales donnent les mêmes 2336 contrôles et la même sortie octet pour octet. Les deux essais initiaux échoués sont conservés dans `attempts.json`, `replay_initial.py` et les fichiers `*_initial.*`. Voir `REPORT.md` pour portée, défauts causaux et limites. `proposed.patch` passe `git apply --check` sur la capture ; son Python passe `ast.parse`.
