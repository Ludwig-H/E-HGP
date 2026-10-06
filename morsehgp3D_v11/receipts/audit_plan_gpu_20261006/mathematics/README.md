# Capsule mathématique du plan GPU

Pin `cf5da0e91fb8740c1d748577fefb129c37b02d3d`, sources vérifiées par Git ; aucun moteur exécuté.
Voir `REPORT.md` pour le lemme de contraction des niveaux égaux, la limite des compteurs MST,
les formules `ancestor_*` et les réponses T1/O8.

Rejeu portable depuis un dépôt contenant ce pin :

```sh
python3 -S -B replay.py --repo /workspaces/E-HGP
python3 -O -S -B replay.py --repo /workspaces/E-HGP
```

La sortie est exacte et déterministe, sans chronomètre. `executions.json` conserve les commandes,
codes et empreintes ; `normal.json` et `optimized.json` doivent être identiques.
`SHA256SUMS` couvre les pièces sauf lui-même. Les empreintes des plans privés lus sont informatives :
le rejeu ne dépend pas de leur maintien dans un worktree développeur.
