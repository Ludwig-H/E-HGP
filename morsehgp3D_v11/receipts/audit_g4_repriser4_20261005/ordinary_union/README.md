# Raccord des inventaires ordinaires R1 et R2

Les lots courts R1 et les lots d’échelle R2 sont disjoints. Leur union
correspond exactement aux noms de l’inventaire ordinaire fina2 :
991 u18, 901 u21, 901 u24 et 902 poison, soit **3 695 PASS** au pin
`98a009550`. Aucun nom supplémentaire, absent ou dupliqué.
Ce raccord couvre seulement le volet ordinaire, sans transfert vers
les mutants, les sanitizers ou les portes longues.

```sh
python3 -B ordinary_r1_r2_union_replay_20261006.py
python3 -O -B ordinary_r1_r2_union_replay_20261006.py
```

Le rejeu lit les trois archives locales épinglées, leurs inventaires
et les verdicts PASS R1/R2, puis compare au résumé figé. Aucun build,
test natif ou accès cloud. Les capsules historiques restent inchangées.
