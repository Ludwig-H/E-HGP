# MES-C : petits nuages, tour FULL de la v12

Verdict d'ensemble : **non tenu**.

- C1 : non tenu — cout fixe 13.782 ms
- C2 : non tenu — 9.560 us par site (limite 3.727)
- C3 : non tenu — synth_sphere_n3000 cpu : refus (unsupported_degeneracy/wide_leaf) ; synth_sphere_n10000 cpu : refus (unsupported_degeneracy/wide_leaf) ; synth_sphere_n3000 appareil : refus (unsupported_degeneracy/wide_leaf) ; synth_sphere_n10000 appareil : refus (unsupported_degeneracy/wide_leaf)

Droites t = a + b n sur les valeurs chaudes (a en ms, b en us par site) :

| configuration (voie:K:fils) | etat | groupe | nuages | a (ms) | b (us/site) |
| --- | --- | --- | ---: | ---: | ---: |
| cpu:5:4 | ok | clusters8 | 5 | -13.879 | 96.712 |
| cpu:5:4 | ok | reel | 132 | -0.864 | 52.039 |
| cpu:5:4 | ok | slab | 5 | 3.370 | 14.446 |
| cpu:5:4 | ok | uniform | 5 | 91.156 | 84.182 |
| cpu:5:48 | ok | clusters8 | 5 | 17.783 | 14.599 |
| cpu:5:48 | ok | reel | 132 | 13.782 | 9.560 |
| cpu:5:48 | ok | slab | 5 | 7.522 | 3.394 |
| cpu:5:48 | ok | uniform | 5 | 47.453 | 10.684 |
| appareil:5:4 | ok | clusters8 | 5 | 1.691 | 23.056 |
| appareil:5:4 | ok | reel | 132 | 2.617 | 8.271 |
| appareil:5:4 | ok | slab | 5 | 3.008 | 3.520 |
| appareil:5:4 | ok | uniform | 5 | 119.001 | 12.475 |
| appareil:5:48 | ok | clusters8 | 5 | 10.635 | 3.477 |
| appareil:5:48 | ok | reel | 132 | 5.112 | 1.958 |
| appareil:5:48 | ok | slab | 5 | 3.988 | 0.925 |
| appareil:5:48 | ok | uniform | 5 | 47.137 | -0.243 |
| cpu:10:4 | ok | clusters8 | 5 | -160.241 | 532.408 |
| cpu:10:4 | ok | reel | 132 | -32.255 | 223.384 |
| cpu:10:4 | ok | slab | 5 | -3.260 | 48.025 |
| cpu:10:4 | ok | uniform | 5 | 175.903 | 532.128 |
| cpu:10:48 | ok | clusters8 | 5 | 9.192 | 67.188 |
| cpu:10:48 | ok | reel | 132 | 15.648 | 32.177 |
| cpu:10:48 | ok | slab | 5 | 9.390 | 8.456 |
| cpu:10:48 | ok | uniform | 5 | 64.599 | 64.184 |
| appareil:10:4 | ok | clusters8 | 5 | -75.612 | 240.640 |
| appareil:10:4 | ok | reel | 132 | -17.073 | 68.095 |
| appareil:10:4 | ok | slab | 5 | -1.782 | 18.735 |
| appareil:10:4 | ok | uniform | 5 | 296.628 | 232.858 |
| appareil:10:48 | ok | clusters8 | 5 | 6.179 | 29.499 |
| appareil:10:48 | ok | reel | 132 | 3.971 | 9.380 |
| appareil:10:48 | ok | slab | 5 | 4.281 | 2.830 |
| appareil:10:48 | ok | uniform | 5 | 68.447 | 25.602 |

Familles difficiles (chaque nuage seul) :

| nuage | sites | K | voie | etat | mur (ms, derniere passe) |
| --- | ---: | ---: | --- | --- | ---: |
| `synth_sphere_n100` | 100 | 5 | cpu | ok | 25.03 |
| `synth_sphere_n300` | 300 | 5 | cpu | ok | 83.55 |
| `synth_sphere_n1000` | 1000 | 5 | cpu | ok | 258.56 |
| `synth_sphere_n3000` | 3000 | 5 | cpu | refus (unsupported_degeneracy/wide_leaf) | — |
| `synth_sphere_n10000` | 10000 | 5 | cpu | refus (unsupported_degeneracy/wide_leaf) | — |
| `synth_line_n100` | 100 | 5 | cpu | ok | 2.45 |
| `synth_line_n1000` | 1000 | 5 | cpu | ok | 7.15 |
| `synth_lattice_n100` | 100 | 5 | cpu | ok | 12.45 |
| `synth_lattice_n300` | 300 | 5 | cpu | ok | 25.65 |
| `synth_lattice_n1000` | 1000 | 5 | cpu | ok | 48.77 |
| `synth_lattice_n3000` | 3000 | 5 | cpu | ok | 82.98 |
| `synth_lattice_n10000` | 10000 | 5 | cpu | ok | 218.07 |
| `synth_sphere_n100` | 100 | 5 | appareil | ok | 19.92 |
| `synth_sphere_n300` | 300 | 5 | appareil | ok | 72.96 |
| `synth_sphere_n1000` | 1000 | 5 | appareil | ok | 243.18 |
| `synth_sphere_n3000` | 3000 | 5 | appareil | refus (unsupported_degeneracy/wide_leaf) | — |
| `synth_sphere_n10000` | 10000 | 5 | appareil | refus (unsupported_degeneracy/wide_leaf) | — |
| `synth_line_n100` | 100 | 5 | appareil | ok | 2.29 |
| `synth_line_n1000` | 1000 | 5 | appareil | ok | 3.55 |
| `synth_lattice_n100` | 100 | 5 | appareil | ok | 9.14 |
| `synth_lattice_n300` | 300 | 5 | appareil | ok | 19.61 |
| `synth_lattice_n1000` | 1000 | 5 | appareil | ok | 45.02 |
| `synth_lattice_n3000` | 3000 | 5 | appareil | ok | 64.76 |
| `synth_lattice_n10000` | 10000 | 5 | appareil | ok | 160.03 |
| `synth_sphere_n100` | 100 | 10 | cpu | ok | 80.94 |
| `synth_sphere_n300` | 300 | 10 | cpu | ok | 586.52 |
| `synth_sphere_n1000` | 1000 | 10 | cpu | ok | 1617.57 |
| `synth_sphere_n3000` | 3000 | 10 | cpu | refus (unsupported_degeneracy/wide_leaf) | — |
| `synth_sphere_n10000` | 10000 | 10 | cpu | refus (unsupported_degeneracy/wide_leaf) | — |
| `synth_line_n100` | 100 | 10 | cpu | ok | 3.16 |
| `synth_line_n1000` | 1000 | 10 | cpu | ok | 8.90 |
| `synth_lattice_n100` | 100 | 10 | cpu | ok | 104.83 |
| `synth_lattice_n300` | 300 | 10 | cpu | ok | 514.24 |
| `synth_lattice_n1000` | 1000 | 10 | cpu | ok | 2494.78 |
| `synth_lattice_n3000` | 3000 | 10 | cpu | ok | 7355.14 |
| `synth_lattice_n10000` | 10000 | 10 | cpu | ok | 11870.66 |
| `synth_sphere_n100` | 100 | 10 | appareil | ok | 58.52 |
| `synth_sphere_n300` | 300 | 10 | appareil | ok | 564.03 |
| `synth_sphere_n1000` | 1000 | 10 | appareil | ok | 1589.36 |
| `synth_sphere_n3000` | 3000 | 10 | appareil | refus (unsupported_degeneracy/wide_leaf) | — |
| `synth_sphere_n10000` | 10000 | 10 | appareil | refus (unsupported_degeneracy/wide_leaf) | — |
| `synth_line_n100` | 100 | 10 | appareil | ok | 3.30 |
| `synth_line_n1000` | 1000 | 10 | appareil | ok | 4.30 |
| `synth_lattice_n100` | 100 | 10 | appareil | ok | 87.69 |
| `synth_lattice_n300` | 300 | 10 | appareil | ok | 488.87 |
| `synth_lattice_n1000` | 1000 | 10 | appareil | ok | 2444.30 |
| `synth_lattice_n3000` | 3000 | 10 | appareil | ok | 7267.23 |
| `synth_lattice_n10000` | 10000 | 10 | appareil | ok | 11202.59 |

Controles manquants : 0.
