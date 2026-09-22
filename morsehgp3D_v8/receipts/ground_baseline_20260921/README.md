# Base de temps du moteur de référence sur les trois nuages sans sol (phase 0)

21 septembre 2026, développeur v8. Cadre : `exploration_v8_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u16_input_only`,
`public_status=not_claimed`. GCP non utilisé. Sonde `mhgp8_wspd_q34_probe`
construite à **92d74c13** (build Release `build/v8-dev-bench`, sources du
moteur identiques à 204b0620, sha256 de la sonde `925daeaa7a6f…`), avant
toute tranche de développement. Entrées : les nuages sans sol au profil u16
([lidar_ground_u16_20260921](../lidar_ground_u16_20260921/README.md)),
39 815 / 35 491 / 45 114 sites. Configuration mesurée : s = 8, masque 6,
Local28, `rectangle-pair`, `boxes`, `affine`, `live` 64, mode `digest`.
Lanceur [bench/run_ground_baseline.py](../../bench/run_ground_baseline.py)
(`read` rejoue les identités : sorties et compteurs géométriques identiques
entre 1 et 8 workers à scène et K égaux, GNU time reproduit, sha256 des JSON).

Ce reçu mesure un **flux de candidats q3/q4** (ni catalogue, ni tour) sur un
hôte partagé de 8 vCPU (4 cœurs physiques SMT) ; il oriente le développement
et ne qualifie rien.

## Résultats

| scène | K | W | mur (s) | CPU (s) | % CPU | q3 émis | q4 émis | charge avant |
| --- | --- | --- | ---: | ---: | ---: | ---: | ---: | --- |
| 00 (000000) | 5 | 1 | 889,5 | 888,7 | 99 | 663 443 | 157 889 | 4,2 |
| 00 | 5 | 8 | 298,0 | 1 283,4 | 430 | 663 443 | 157 889 | 3,1 |
| 00 | 10 | 8 | 911,1 | 3 876,5 | 425 | 2 830 806 | 1 729 751 | 5,2 |
| 01 (000100) | 5 | 1 | 742,9 | 741,6 | 99 | 556 702 | 121 659 | 4,2 |
| 01 | 5 | 8 | 273,2 | 992,8 | 363 | 556 702 | 121 659 | 2,4 |
| 01 | 10 | 8 | 761,8 | 2 891,5 | 379 | 2 305 606 | 1 269 977 | 5,2 |
| 02 (000200) | 5 | 1 | 2 043,8 | 1 886,9 | 92 | 679 702 | 140 147 | 3,1 |
| 02 | 5 | 8 | 1 245,6 | 2 052,0 | 164 | 679 702 | 140 147 | 10,3 |
| 02 | 10 | 8 | 3 533,9 | 5 860,2 | 165 | 2 813 227 | 1 420 900 | 10,7 |

Identité W1/W8 : sorties (xor, somme, comptes, IDs de coquille) et les 439
compteurs géométriques sont identiques à scène et K égaux (trois paires).

## Charge concurrente à déclarer

Les six premières lignes ont couru avec un harnais d'audit sur un cœur et,
par intermittence, des builds et des mesures courtes du développeur. Les
lignes de la scène 02 (à partir de 22:41 UTC) ont couru **en concurrence avec
la campagne appariée** ([ground_phase1_20260921](../ground_phase1_20260921/README.md),
8 workers), démarrée trop tôt par une sentinelle périmée : leurs murs et
pourcentages de CPU ne sont pas comparables (K5 W1 à 92 % de CPU, W8 à
164 %). Les compteurs et les sorties ne dépendent pas de la charge.

## Rejouer

```bash
python3 -B -O morsehgp3D_v8/bench/run_ground_baseline.py read --output morsehgp3D_v8/receipts/ground_baseline_20260921
python3 -B -O morsehgp3D_v8/bench/run_ground_baseline.py run --probe <sonde 92d74c13> --output /tmp/ground_baseline
```

`gprof_scene00_k5_w1/` conserve le profil plat d'un build `-pg` séparé
(RelWithDebInfo, `run.sh`) sur la scène 0 à K5 et un worker : diagnostic
d'attribution du temps (bornes de blocs de l'atlas 40 %, census q3 28 %),
pas une mesure de ce reçu.
