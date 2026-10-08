# MES-C : petits nuages, tour FULL de la v12

Verdict d'ensemble : **non tenu**.

- C1 : non tenu — cout fixe 14.278 ms
- C2 : non tenu — 9.697 us par site (limite 3.727)
- C3 : non tenu — synth_sphere_n3000 cpu : refus (unsupported_degeneracy/wide_leaf) ; synth_sphere_n10000 cpu : refus (unsupported_degeneracy/wide_leaf) ; synth_sphere_n3000 appareil : refus (unsupported_degeneracy/wide_leaf) ; synth_sphere_n10000 appareil : refus (unsupported_degeneracy/wide_leaf)

Droites t = a + b n sur les valeurs chaudes (a en ms, b en us par site) :

| configuration (voie:K:fils) | etat | groupe | nuages | a (ms) | b (us/site) |
| --- | --- | --- | ---: | ---: | ---: |
| cpu:5:4 | ok | clusters8 | 5 | -13.965 | 97.076 |
| cpu:5:4 | ok | reel | 132 | -1.044 | 52.441 |
| cpu:5:4 | ok | slab | 5 | 3.153 | 14.517 |
| cpu:5:4 | ok | uniform | 5 | 92.020 | 84.201 |
| cpu:5:48 | ok | clusters8 | 5 | 17.618 | 14.661 |
| cpu:5:48 | ok | reel | 132 | 14.278 | 9.697 |
| cpu:5:48 | ok | slab | 5 | 7.676 | 3.464 |
| cpu:5:48 | ok | uniform | 5 | 48.457 | 10.550 |
| appareil:5:4 | ok | clusters8 | 5 | 1.628 | 23.260 |
| appareil:5:4 | ok | reel | 132 | 2.599 | 8.332 |
| appareil:5:4 | ok | slab | 5 | 3.031 | 3.505 |
| appareil:5:4 | ok | uniform | 5 | 118.999 | 12.781 |
| appareil:5:48 | ok | clusters8 | 5 | 10.639 | 3.517 |
| appareil:5:48 | ok | reel | 132 | 5.126 | 1.975 |
| appareil:5:48 | ok | slab | 5 | 4.000 | 0.914 |
| appareil:5:48 | ok | uniform | 5 | 48.022 | -0.514 |
| cpu:10:4 | ok | clusters8 | 5 | -160.302 | 535.337 |
| cpu:10:4 | ok | reel | 132 | -32.038 | 225.444 |
| cpu:10:4 | ok | slab | 5 | -2.962 | 48.089 |
| cpu:10:4 | ok | uniform | 5 | 175.209 | 535.199 |
| cpu:10:48 | ok | clusters8 | 5 | 9.509 | 67.169 |
| cpu:10:48 | ok | reel | 132 | 16.993 | 32.173 |
| cpu:10:48 | ok | slab | 5 | 10.451 | 8.497 |
| cpu:10:48 | ok | uniform | 5 | 64.118 | 64.480 |
| appareil:10:4 | ok | clusters8 | 5 | -76.991 | 242.796 |
| appareil:10:4 | ok | reel | 132 | -17.150 | 68.415 |
| appareil:10:4 | ok | slab | 5 | -1.771 | 18.863 |
| appareil:10:4 | ok | uniform | 5 | 295.160 | 233.688 |
| appareil:10:48 | ok | clusters8 | 5 | 6.060 | 29.634 |
| appareil:10:48 | ok | reel | 132 | 3.949 | 9.417 |
| appareil:10:48 | ok | slab | 5 | 4.264 | 2.838 |
| appareil:10:48 | ok | uniform | 5 | 68.093 | 25.750 |

Familles difficiles (chaque nuage seul) :

| nuage | sites | K | voie | etat | mur (ms, derniere passe) |
| --- | ---: | ---: | --- | --- | ---: |
| `synth_sphere_n100` | 100 | 5 | cpu | ok | 25.13 |
| `synth_sphere_n300` | 300 | 5 | cpu | ok | 82.30 |
| `synth_sphere_n1000` | 1000 | 5 | cpu | ok | 263.91 |
| `synth_sphere_n3000` | 3000 | 5 | cpu | refus (unsupported_degeneracy/wide_leaf) | — |
| `synth_sphere_n10000` | 10000 | 5 | cpu | refus (unsupported_degeneracy/wide_leaf) | — |
| `synth_line_n100` | 100 | 5 | cpu | ok | 2.36 |
| `synth_line_n1000` | 1000 | 5 | cpu | ok | 6.78 |
| `synth_lattice_n100` | 100 | 5 | cpu | ok | 11.09 |
| `synth_lattice_n300` | 300 | 5 | cpu | ok | 26.09 |
| `synth_lattice_n1000` | 1000 | 5 | cpu | ok | 47.93 |
| `synth_lattice_n3000` | 3000 | 5 | cpu | ok | 96.88 |
| `synth_lattice_n10000` | 10000 | 5 | cpu | ok | 227.57 |
| `synth_sphere_n100` | 100 | 5 | appareil | ok | 19.92 |
| `synth_sphere_n300` | 300 | 5 | appareil | ok | 72.46 |
| `synth_sphere_n1000` | 1000 | 5 | appareil | ok | 248.05 |
| `synth_sphere_n3000` | 3000 | 5 | appareil | refus (unsupported_degeneracy/wide_leaf) | — |
| `synth_sphere_n10000` | 10000 | 5 | appareil | refus (unsupported_degeneracy/wide_leaf) | — |
| `synth_line_n100` | 100 | 5 | appareil | ok | 2.56 |
| `synth_line_n1000` | 1000 | 5 | appareil | ok | 3.63 |
| `synth_lattice_n100` | 100 | 5 | appareil | ok | 9.22 |
| `synth_lattice_n300` | 300 | 5 | appareil | ok | 19.86 |
| `synth_lattice_n1000` | 1000 | 5 | appareil | ok | 38.58 |
| `synth_lattice_n3000` | 3000 | 5 | appareil | ok | 71.11 |
| `synth_lattice_n10000` | 10000 | 5 | appareil | ok | 159.71 |
| `synth_sphere_n100` | 100 | 10 | cpu | ok | 77.18 |
| `synth_sphere_n300` | 300 | 10 | cpu | ok | 576.66 |
| `synth_sphere_n1000` | 1000 | 10 | cpu | ok | 1610.61 |
| `synth_sphere_n3000` | 3000 | 10 | cpu | refus (unsupported_degeneracy/wide_leaf) | — |
| `synth_sphere_n10000` | 10000 | 10 | cpu | refus (unsupported_degeneracy/wide_leaf) | — |
| `synth_line_n100` | 100 | 10 | cpu | ok | 3.35 |
| `synth_line_n1000` | 1000 | 10 | cpu | ok | 9.73 |
| `synth_lattice_n100` | 100 | 10 | cpu | ok | 108.02 |
| `synth_lattice_n300` | 300 | 10 | cpu | ok | 519.20 |
| `synth_lattice_n1000` | 1000 | 10 | cpu | ok | 2494.77 |
| `synth_lattice_n3000` | 3000 | 10 | cpu | ok | 7346.22 |
| `synth_lattice_n10000` | 10000 | 10 | cpu | ok | 11716.92 |
| `synth_sphere_n100` | 100 | 10 | appareil | ok | 58.16 |
| `synth_sphere_n300` | 300 | 10 | appareil | ok | 551.86 |
| `synth_sphere_n1000` | 1000 | 10 | appareil | ok | 1583.56 |
| `synth_sphere_n3000` | 3000 | 10 | appareil | refus (unsupported_degeneracy/wide_leaf) | — |
| `synth_sphere_n10000` | 10000 | 10 | appareil | refus (unsupported_degeneracy/wide_leaf) | — |
| `synth_line_n100` | 100 | 10 | appareil | ok | 3.40 |
| `synth_line_n1000` | 1000 | 10 | appareil | ok | 4.28 |
| `synth_lattice_n100` | 100 | 10 | appareil | ok | 86.54 |
| `synth_lattice_n300` | 300 | 10 | appareil | ok | 485.35 |
| `synth_lattice_n1000` | 1000 | 10 | appareil | ok | 2451.55 |
| `synth_lattice_n3000` | 3000 | 10 | appareil | ok | 7171.04 |
| `synth_lattice_n10000` | 10000 | 10 | appareil | ok | 10977.05 |

Controles manquants : 0.
