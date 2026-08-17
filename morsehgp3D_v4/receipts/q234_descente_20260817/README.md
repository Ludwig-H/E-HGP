# Reçu — descente trois lanes q2/q3/q4, n=8000, s=8, smax=11

Date : 17 août 2026 UTC. Machine : conteneur CPU 4 cœurs, single-thread —
pas un chiffre G4, aucun SLO qualifié. Commande :

```bash
./build/v4/mhgp4_q234_prune_probe --family=$fam --n=8000 --s=8 --seed=3 --judge-pairs=2
```

Masse de paires fermée PENDANT la descente (q2/q3/q4, % de C(n,2)) :

| famille | q2 | q3 | q4 | rect vivants | juge (paires) | fausses morts |
|---|---:|---:|---:|---:|---:|---:|
| uniform | 98,99 | 96,57 | 95,99 | 743 548 | 10 505 380 | 0 |
| terrain | 99,39 | 98,64 | 98,46 | 219 612 | 1 817 990 | 0 |
| eight_clusters | 98,74 | 91,59 | 88,09 | 625 634 | 5 332 948 | 0 |
| scanline_single_pass | 99,36 | 98,04 | 97,48 | 182 039 | 1 291 324 | 0 |

Ledgers morte+vivante exacts par lane sur les quatre lignes. Repère v3
(préfiltre d'ancres TERMINAL, § 6bis.5, fermeture des ancres q4, K=10) :
uniform 95,41, eight_clusters 84,13, terrain 98,30 — la descente v4 fait
mieux sur les trois familles comparables, en tuant en majorité à des niveaux
INTERNES (ex. uniform : 1 375 330 blocs q4 internes contre 829 014
terminaux). `eight_clusters` reste la famille dure (88,09 %), en accord avec
le diagnostic v3 (paires inter-amas à milieu vide : seuls h_a/h_b mordront).

Coûts single-thread (descente) : 3,5 s (scanline) à 35,5 s (uniform) —
l'autorité 64 coins aux feuilles domine le coût q3/q4 ; les optimisations
documentées (coins distincts, fusion des évaluations (H,Xi), h_a/h_b) ne
sont pas encore appliquées. Les comptes, eux, sont définitifs à graine 3.
