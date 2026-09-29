# Reçu : test préenregistré du lot A, K = `min_samples` (29 septembre 2026)

`public_status=not_claimed`, `mode=benchmark_only`. Une seule exécution sur l'espace `test`, selon le
[préenregistrement](../../bench/synthetic/prereg/PREREG_V10_KMATCH_A_20260928.json) commité en `e8dd36a91`.

## Exécution

- Scripts du banc exécutés depuis `git archive e8dd36a91` ; binaire figé construit depuis `4a3d09d8a`. `run_test.py`
  vérifie les épingles avant toute scène : sha256 du binaire et des six scripts, versions numpy 2.5.3, scipy 1.18.1
  et scikit-learn 1.9.1, manifeste du plan.
- 960 scènes : 8 familles × 4 niveaux × bruit {0 ; 0,1} × n {8 000 ; 16 000 ; 32 000} × 5 graines de l'espace
  `test`. 7 680 lignes (8 méthodes), 0 refus, 0 échec d'ouvrier. Codespace, 4 processus, du 29 septembre 06:34 au
  29 septembre 09:20 UTC.
- Une première exécution, interrompue au bout de 160 scènes par un redémarrage de la machine, a été conservée. Ses
  1 280 lignes sont identiques à celles de la relance complète : `compare_lotA_runs.py`,
  `compare_lotA_runs.txt`, `results_run_interrompu.csv`. La décision porte sur la relance complète.
- Décision calculée par le `decide.py` épinglé : `DECISION.json` et `DECISION.md`, aucun chiffre écrit à la main.

## Résultat

**La tour bat HDBSCAN à K = 1, 2 et 3**, au sens préenregistré. Chaque condition est remplie :
- p_Holm < 0,05 ;
- Δ ≥ 0,02 ;
- borne basse de l'IC 95 % strictement positive ;
- Δ > 0 à chaque taille ;
- aucune perte significative d'AMI ;
- aucun refus.

| K | Tour (tête du 28 sept.) | sklearn à `min_samples` = K | Δ | IC 95 % | p_Holm |
| ---: | ---: | ---: | ---: | --- | ---: |
| 1 | 0,7751 | 0,7220 | +0,053 | [+0,046 ; +0,060] | 3e-5 |
| 2 | 0,7642 | 0,6719 | +0,092 | [+0,084 ; +0,100] | 3e-5 |
| 3 | 0,7550 | 0,7056 | +0,049 | [+0,043 ; +0,056] | 3e-5 |

- L'écart croît avec n. À K = 3, il vaut +0,028 à 8 000 points, +0,049 à 16 000 et +0,071 à 32 000.
- Par famille, la tour gagne largement sur `shells` (+0,52 à +0,57 pour K ≥ 2), `unbalanced` et `hierarchical`
  (K ≥ 2), ainsi que sur `filaments`. Elle perd sur `bridge` (−0,11 à −0,14 pour K ≥ 2), sur `spherical` (−0,12
  à K = 3) et sur `anisotropic` (−0,07 à K = 3).
- Références descriptives, sans valeur confirmatoire : la tour à K = 1 devance `HDBSCAN()` aux défauts de +0,261,
  et le protocole de la thèse (`min_samples` = 3, EOM, α = 1) de +0,087.

## Lecture

- **Portée.** C'est la tête du 28 septembre : entrée C∩X, EOM avec λ = r^(−ẑ), remplissage borné. La tête v10-b
  (entrée par première couverture, z choisi sur dev) sera testée par un préenregistrement distinct, sur un espace
  de graines neuf.
- **Attribution, écrite avant le test.** À K = 1, les deux hiérarchies coïncident (liaison simple) : l'écart
  de +0,053 vient entièrement de la tête. L'audit du 29 septembre l'a confirmé sur dev : tour et sklearn y sont
  identiques à tête égale. À K ≥ 2, l'écart mesure le couple hiérarchie + tête.
- **Dev contre test.** Les écarts de test dépassent ceux de dev à K = 2 et 3 (dev +0,038 et +0,005). Les tailles de
  test sont plus grandes (8 000 à 32 000 contre 2 000 à 8 000), et l'écart croît avec n.
