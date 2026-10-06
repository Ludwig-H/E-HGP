# Complément : défauts restants du juge

Le WIP corrige désormais de lui-même l'attente `max(m)==25` : sa porte exige S=B et au moins une fusion, et explique que la boule centrale est interne. Le présent patch conserve intégralement ce `shell_cases`. Il propose uniquement la projection DSU de l'oracle, l'ordre BallIdx exact, la comparaison `mine` sans projection et l'import/documentation associés.

Capture stable du même HEAD `9eee2ed4bcef1e960cdf2456012b84416854dc20`, nouveau fichier WIP SHA256 `f3bbdd47c41462395d52a27f48cb19ff40ded618bae48bdf52e99e136453193a`. Le reçu initial et ses échecs sont inchangés ; ce complément possède sa propre fermeture.

```sh
python3 -S -B replay.py --repo /workspaces/E-HGP
python3 -O -S -B replay.py --repo /workspaces/E-HGP
sha256sum -c SHA256SUMS
```

Les deux commandes passent 34 contrôles, code 0, stderr vide, stdout identique. Syntaxe, absence de projection native, conservation exacte du corps `shell_cases` et `git apply --check` sont vérifiés. Les quatre petits témoins du reçu initial sont repris pour contrôler la nouvelle source proposée ; la preuve géométrique étendue à 25 sites n'est pas rejouée. Aucun test natif.
