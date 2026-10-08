# MES-C : petits nuages, tour FULL de la v12

Verdict d'ensemble : **non tenu**.

- C1 : non tenu — cout fixe 16.451 ms
- C2 : non tenu — 11.572 us par site (limite 3.727)
- C3 : non tenu — synth_sphere_n3000 cpu : refus (unsupported_degeneracy/wide_leaf) ; synth_sphere_n10000 cpu : refus (unsupported_degeneracy/wide_leaf) ; synth_sphere_n3000 appareil : refus (unsupported_degeneracy/wide_leaf) ; synth_sphere_n10000 appareil : refus (unsupported_degeneracy/wide_leaf)

Droites t = a + b n sur les valeurs chaudes (a en ms, b en us par site) :

| configuration (voie:K:fils) | etat | groupe | nuages | a (ms) | b (us/site) |
| --- | --- | --- | ---: | ---: | ---: |
| cpu:5:4 | ok | clusters8 | 5 | -15.573 | 104.036 |
| cpu:5:4 | ok | reel | 132 | -0.522 | 55.114 |
| cpu:5:4 | ok | slab | 5 | 3.587 | 15.778 |
| cpu:5:4 | ok | uniform | 5 | 89.097 | 91.233 |
| cpu:5:48 | ok | clusters8 | 5 | 19.557 | 18.354 |
| cpu:5:48 | ok | reel | 132 | 16.451 | 11.572 |
| cpu:5:48 | ok | slab | 5 | 9.438 | 4.637 |
| cpu:5:48 | ok | uniform | 5 | 48.413 | 15.170 |
| appareil:5:4 | ok | clusters8 | 5 | 0.758 | 28.359 |
| appareil:5:4 | ok | reel | 132 | 3.264 | 10.390 |
| appareil:5:4 | ok | slab | 5 | 3.485 | 4.449 |
| appareil:5:4 | ok | uniform | 5 | 117.816 | 17.484 |
| appareil:5:48 | ok | clusters8 | 5 | 10.959 | 6.620 |
| appareil:5:48 | ok | reel | 132 | 6.053 | 3.734 |
| appareil:5:48 | ok | slab | 5 | 4.453 | 1.970 |
| appareil:5:48 | ok | uniform | 5 | 47.819 | 2.878 |
| cpu:10:4 | ok | clusters8 | 5 | -185.367 | 589.523 |
| cpu:10:4 | ok | reel | 132 | -34.703 | 242.961 |
| cpu:10:4 | ok | slab | 5 | -1.662 | 52.196 |
| cpu:10:4 | ok | uniform | 5 | 158.028 | 585.148 |
| cpu:10:48 | ok | clusters8 | 5 | 4.000 | 89.008 |
| cpu:10:48 | ok | reel | 132 | 19.278 | 40.507 |
| cpu:10:48 | ok | slab | 5 | 11.712 | 11.588 |
| cpu:10:48 | ok | uniform | 5 | 65.289 | 88.129 |
| appareil:10:4 | ok | clusters8 | 5 | -94.061 | 282.900 |
| appareil:10:4 | ok | reel | 132 | -17.430 | 81.856 |
| appareil:10:4 | ok | slab | 5 | -0.387 | 21.599 |
| appareil:10:4 | ok | uniform | 5 | 287.692 | 268.466 |
| appareil:10:48 | ok | clusters8 | 5 | 5.673 | 44.489 |
| appareil:10:48 | ok | reel | 132 | 8.687 | 15.858 |
| appareil:10:48 | ok | slab | 5 | 6.310 | 5.577 |
| appareil:10:48 | ok | uniform | 5 | 70.855 | 41.284 |

Familles difficiles (chaque nuage seul) :

| nuage | sites | K | voie | etat | mur (ms, derniere passe) |
| --- | ---: | ---: | --- | --- | ---: |
| `synth_sphere_n100` | 100 | 5 | cpu | ok | 29.06 |
| `synth_sphere_n300` | 300 | 5 | cpu | ok | 83.60 |
| `synth_sphere_n1000` | 1000 | 5 | cpu | ok | 268.37 |
| `synth_sphere_n3000` | 3000 | 5 | cpu | refus (unsupported_degeneracy/wide_leaf) | — |
| `synth_sphere_n10000` | 10000 | 5 | cpu | refus (unsupported_degeneracy/wide_leaf) | — |
| `synth_line_n100` | 100 | 5 | cpu | ok | 2.46 |
| `synth_line_n1000` | 1000 | 5 | cpu | ok | 9.17 |
| `synth_lattice_n100` | 100 | 5 | cpu | ok | 15.46 |
| `synth_lattice_n300` | 300 | 5 | cpu | ok | 35.74 |
| `synth_lattice_n1000` | 1000 | 5 | cpu | ok | 72.97 |
| `synth_lattice_n3000` | 3000 | 5 | cpu | ok | 112.94 |
| `synth_lattice_n10000` | 10000 | 5 | cpu | ok | 290.38 |
| `synth_sphere_n100` | 100 | 5 | appareil | ok | 20.25 |
| `synth_sphere_n300` | 300 | 5 | appareil | ok | 73.80 |
| `synth_sphere_n1000` | 1000 | 5 | appareil | ok | 245.00 |
| `synth_sphere_n3000` | 3000 | 5 | appareil | refus (unsupported_degeneracy/wide_leaf) | — |
| `synth_sphere_n10000` | 10000 | 5 | appareil | refus (unsupported_degeneracy/wide_leaf) | — |
| `synth_line_n100` | 100 | 5 | appareil | ok | 2.15 |
| `synth_line_n1000` | 1000 | 5 | appareil | ok | 3.76 |
| `synth_lattice_n100` | 100 | 5 | appareil | ok | 13.05 |
| `synth_lattice_n300` | 300 | 5 | appareil | ok | 30.03 |
| `synth_lattice_n1000` | 1000 | 5 | appareil | ok | 59.74 |
| `synth_lattice_n3000` | 3000 | 5 | appareil | ok | 96.52 |
| `synth_lattice_n10000` | 10000 | 5 | appareil | ok | 232.38 |
| `synth_sphere_n100` | 100 | 10 | cpu | ok | 79.55 |
| `synth_sphere_n300` | 300 | 10 | cpu | ok | 588.09 |
| `synth_sphere_n1000` | 1000 | 10 | cpu | ok | 1640.93 |
| `synth_sphere_n3000` | 3000 | 10 | cpu | refus (unsupported_degeneracy/wide_leaf) | — |
| `synth_sphere_n10000` | 10000 | 10 | cpu | refus (unsupported_degeneracy/wide_leaf) | — |
| `synth_line_n100` | 100 | 10 | cpu | ok | 3.32 |
| `synth_line_n1000` | 1000 | 10 | cpu | ok | 10.46 |
| `synth_lattice_n100` | 100 | 10 | cpu | ok | 135.21 |
| `synth_lattice_n300` | 300 | 10 | cpu | ok | 593.63 |
| `synth_lattice_n1000` | 1000 | 10 | cpu | ok | 2731.62 |
| `synth_lattice_n3000` | 3000 | 10 | cpu | ok | 7784.08 |
| `synth_lattice_n10000` | 10000 | 10 | cpu | ok | 12745.93 |
| `synth_sphere_n100` | 100 | 10 | appareil | ok | 61.95 |
| `synth_sphere_n300` | 300 | 10 | appareil | ok | 565.45 |
| `synth_sphere_n1000` | 1000 | 10 | appareil | ok | 1607.33 |
| `synth_sphere_n3000` | 3000 | 10 | appareil | refus (unsupported_degeneracy/wide_leaf) | — |
| `synth_sphere_n10000` | 10000 | 10 | appareil | refus (unsupported_degeneracy/wide_leaf) | — |
| `synth_line_n100` | 100 | 10 | appareil | ok | 2.98 |
| `synth_line_n1000` | 1000 | 10 | appareil | ok | 4.94 |
| `synth_lattice_n100` | 100 | 10 | appareil | ok | 114.99 |
| `synth_lattice_n300` | 300 | 10 | appareil | ok | 558.39 |
| `synth_lattice_n1000` | 1000 | 10 | appareil | ok | 2634.56 |
| `synth_lattice_n3000` | 3000 | 10 | appareil | ok | 7639.14 |
| `synth_lattice_n10000` | 10000 | 10 | appareil | ok | 12184.76 |

Controles manquants : 0.
