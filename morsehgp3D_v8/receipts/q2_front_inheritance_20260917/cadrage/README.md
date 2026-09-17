# Cadrage de la tranche 21 : mesures jetables avant le port

17 septembre 2026, constructeur. `phase=exploration_v8_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u16_input_only`,
`mode=implementation_v8_p0`, `public_status=not_claimed`. GCP non utilisé.

Ce dossier n'est **pas une qualification**. Il archive ce qui a servi à choisir
le levier porté par la tranche, pour que les chiffres de la note
[P0_TEMOINS_HERITES_Q2](../../../docs/P0_TEMOINS_HERITES_Q2.md) restent
rejouables et que la piste écartée puisse être reprise sans la réécrire. Les
compteurs sont déterministes ; les temps viennent d'une machine partagée :
minimum de deux passes consécutives par variante pour le prototype, de trois
répétitions entrelacées pour le moteur réel. Toute variante qui rend un résultat
rend l'empreinte canonique de supports de la référence.

## Pièces

| Fichier | Contenu |
| --- | --- |
| `prototype_front.patch` | Prototype jetable sur `8190e7ab` (variables d'environnement `PROTO_*`) : héritage, réutilisation du pivot, saut des petites masses, reprise de descente par dernier chemin et par remontée. |
| `measure_matrix.py`, `measure_matrix.out` | Matrice du prototype, quatre familles, n = 8 000 et 32 000. Les lignes `FAILED` sont le registre de la sonde qui refuse la variante « petites masses », dont le prototype compte mal les recherches : variante non retenue, lignes conservées telles quelles. |
| `measure_variants.py`, `measure_variants.jsonl` | Variantes du prototype avec compteurs et empreintes, dont les deux réutilisations de pivot écartées. |
| `resume_engine_both_levers.patch` | Moteur réel à **deux** leviers (reprise exacte de la descente et témoins hérités), sa sonde, en patch sur `8190e7ab`. C'est l'état mesuré avant le retrait de la reprise. |
| `cycle_counters_front.patch` | Compteurs de cycles `__rdtsc` posés sur ce moteur (copie hors dépôt), à appliquer après le patch précédent. |
| `resume_measure.py`, `resume_measure.jsonl` | Mesure du moteur à deux leviers : quatre combinaisons, cinq configurations, trois répétitions entrelacées, puis répartition des cycles. Son champ `inherited_rejections` suit l'ANCIENNE définition de ce patch (rejets prononcés avec une liste reçue non vide) ; le compteur livré est plus strict (nouveaux crédits + rangs reçus revus < Kmax : 438 248 au lieu de 510 033 à uniforme 8 000, K = 10, fenêtre 2K) et ne doit pas lui être comparé. Tous les autres compteurs d'héritage coïncident. |
| `callgrind_uniform_8000_k10_*.txt` | Extraits du profil d'instructions qui avait surestimé la descente. |
| `independent_front_model.py` | Modèle Python du front q2 écrit par un relecteur de la réfutation de conception, à partir du contrat et de la lecture du code. Troisième implémentation, indépendante du moteur et du rejeu C++ de la porte. |
| `independent_front_model_constants.py`, `.json` | Compteurs de ce modèle sur les fixtures gravées dans `tests/wspd_front_inheritance_gate.cpp`. |

## Reproduire

```bash
git archive 8190e7ab morsehgp3D_v8 | tar -x -C <copie>
patch -d <copie> -p1 < resume_engine_both_levers.patch
cmake -S <copie>/morsehgp3D_v8 -B <build> -DCMAKE_BUILD_TYPE=Release && cmake --build <build> --target mhgp8_wspd_q2_inheritance_probe
<build>/mhgp8_wspd_q2_inheritance_probe 32000 uniform 10 8 3 1 16 64 2 16 <reprise 0|1> <héritage 0|1>
python3 independent_front_model_constants.py
```

## Ce que ces mesures ont décidé

- **Témoins hérités : portés.** Temps q2 ×0,87 à ×0,95 sur la fenêtre 2K hors
  rangées, ×1,01 sur les rangées ; candidates et produits en baisse, supports
  identiques.
- **Reprise exacte de la descente : non portée.** Le théorème tient (pivot
  identique, tous les compteurs historiques égaux, pas de descente à 17 à 38 %
  de la référence) mais le moteur réel ne rend que ×0,94 à ×0,98, la descente
  pesant 15 à 17 % des cycles. Les rapports du prototype pour le même levier
  (×0,85 à ×0,92) sont remplacés par cette mesure : sa référence instrumentée
  est plus lente de 11 à 12 % et ses pas de descente portaient un surcoût
  propre. L'explication par le cache des niveaux hauts reste une hypothèse.
- **Réutilisation du pivot du parent : écartée.** Jusqu'à ×6,0 en temps et
  ×23 en candidates : le recentrage du proposeur est essentiel.
