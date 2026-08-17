# Reçu — descente ternaire q2, tailles d'intérêt, s=8

Date : 17 août 2026 UTC. Machine : conteneur CPU 4 cœurs, single-thread
(pas un chiffre G4). Commande :

```bash
./build/v4/mhgp4_q2_prune_probe --family=$fam --n=$n --s=8 --seed=3 --judge-pairs=4
```

`fam` dans {uniform, terrain, eight_clusters, scanline_single_pass},
`n` dans {8000, 16000, 32000}. `smax=11` (h_2 = 10).

Résultats (counter-only, ledger `masse_morte + masse_vivante` EXACT partout,
zéro fausse mort au juge sur l'ensemble des 12 lignes) :

- masse de paires fermée pendant la descente : 98,74 % à 99,84 %, croissante
  avec n sur les quatre familles à s=8 ;
- la mort frappe surtout aux niveaux INTERNES (ex. uniform 32k : 4 513 235
  blocs internes contre 183 304 terminaux) — c'est le mécanisme
  d'output-sensitivity recherché ;
- rectangles vivants restants à n=32000 : 1 094 102 (uniform) contre
  19 574 390 au front nu — 18× de réduction du travail d'instruction ;
- mesure appariée `--kill=descente` vs `--kill=terminal` (uniform 8k) : même
  fermeture (98,99 %), 1,37× plus rapide, 3,3× moins de blocs morts.

Limites : q2 seulement (les lanes q3/q4 partageront la descente, fuseaux
emboîtés) ; temps single-thread non contractuels ; le juge échantillonne
4 paires + 4 coins par bloc mort (pas une preuve exhaustive — le fail-open
est un théorème, le juge ne chasse que la faute d'implémentation).
