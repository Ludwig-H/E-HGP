# MES-C : petits nuages, tour FULL de la v12

Verdict d'ensemble : **refuse**.

- C1 : non tenu — cout fixe 20.337 ms
- C2 : non tenu — 11.747 us par site (limite 3.727)
- C3 : non tenu — synth_sphere_n3000 cpu : refus (unsupported_degeneracy/wide_leaf) ; synth_sphere_n10000 cpu : refus (unsupported_degeneracy/wide_leaf) ; synth_sphere_n3000 appareil : refus (unsupported_degeneracy/wide_leaf) ; synth_sphere_n10000 appareil : refus (unsupported_degeneracy/wide_leaf)

Droites t = a + b n sur les valeurs chaudes (a en ms, b en us par site) :

| configuration (voie:K:fils) | etat | groupe | nuages | a (ms) | b (us/site) |
| --- | --- | --- | ---: | ---: | ---: |
| cpu:5:1 | ok | clusters8 | 5 | -88.877 | 374.829 |
| cpu:5:1 | ok | reel | 132 | -26.617 | 195.701 |
| cpu:5:1 | ok | slab | 5 | -1.785 | 50.370 |
| cpu:5:1 | ok | uniform | 5 | 268.155 | 332.897 |
| cpu:5:4 | ok | clusters8 | 5 | -15.499 | 104.025 |
| cpu:5:4 | ok | reel | 132 | -0.374 | 54.857 |
| cpu:5:4 | ok | slab | 5 | 3.873 | 15.663 |
| cpu:5:4 | ok | uniform | 5 | 86.950 | 91.586 |
| cpu:5:48 | ok | clusters8 | 5 | 23.265 | 18.840 |
| cpu:5:48 | ok | reel | 132 | 20.337 | 11.747 |
| cpu:5:48 | ok | slab | 5 | 13.216 | 4.667 |
| cpu:5:48 | ok | uniform | 5 | 53.137 | 15.141 |
| appareil:5:1 | ok | clusters8 | 5 | -21.268 | 92.457 |
| appareil:5:1 | ok | reel | 132 | -4.162 | 30.855 |
| appareil:5:1 | ok | slab | 5 | 1.517 | 11.724 |
| appareil:5:1 | ok | uniform | 5 | 375.237 | 57.587 |
| appareil:5:4 | ok | clusters8 | 5 | 1.391 | 28.881 |
| appareil:5:4 | ok | reel | 132 | 3.501 | 10.536 |
| appareil:5:4 | ok | slab | 5 | 3.880 | 4.630 |
| appareil:5:4 | ok | uniform | 5 | 115.916 | 18.326 |
| appareil:5:48 | ok | clusters8 | 5 | 15.786 | 6.927 |
| appareil:5:48 | ok | reel | 132 | 9.672 | 3.605 |
| appareil:5:48 | ok | slab | 5 | 8.475 | 1.796 |
| appareil:5:48 | ok | uniform | 5 | 52.524 | 3.065 |
| cpu:10:1 | ok | clusters8 | 5 | -747.973 | 2188.203 |
| cpu:10:1 | ok | reel | 132 | -172.395 | 892.846 |
| cpu:10:1 | ok | slab | 5 | -34.908 | 179.247 |
| cpu:10:1 | ok | uniform | 5 | 512.106 | 2194.010 |
| cpu:10:4 | ok | clusters8 | 5 | -185.814 | 591.554 |
| cpu:10:4 | ok | reel | 132 | -33.801 | 242.599 |
| cpu:10:4 | ok | slab | 5 | -1.079 | 51.909 |
| cpu:10:4 | ok | uniform | 5 | 156.148 | 587.519 |
| cpu:10:48 | ok | clusters8 | 5 | 12.658 | 90.187 |
| cpu:10:48 | ok | reel | 132 | 28.371 | 40.759 |
| cpu:10:48 | ok | slab | 5 | 20.111 | 11.724 |
| cpu:10:48 | ok | uniform | 5 | 71.455 | 89.263 |
| appareil:10:1 | echec (expire) | — | — | — | — |
| appareil:10:4 | non_joue (delai) | — | — | — | — |
| appareil:10:48 | non_joue (delai) | — | — | — | — |

Familles difficiles (chaque nuage seul) :

| nuage | sites | K | voie | etat | mur (ms, derniere passe) |
| --- | ---: | ---: | --- | --- | ---: |
| `synth_sphere_n100` | 100 | 5 | cpu | ok | 43.24 |
| `synth_sphere_n300` | 300 | 5 | cpu | ok | 96.91 |
| `synth_sphere_n1000` | 1000 | 5 | cpu | ok | 280.46 |
| `synth_sphere_n3000` | 3000 | 5 | cpu | refus (unsupported_degeneracy/wide_leaf) | — |
| `synth_sphere_n10000` | 10000 | 5 | cpu | refus (unsupported_degeneracy/wide_leaf) | — |
| `synth_line_n100` | 100 | 5 | cpu | ok | 7.57 |
| `synth_line_n1000` | 1000 | 5 | cpu | ok | 13.90 |
| `synth_lattice_n100` | 100 | 5 | cpu | ok | 20.47 |
| `synth_lattice_n300` | 300 | 5 | cpu | ok | 40.33 |
| `synth_lattice_n1000` | 1000 | 5 | cpu | ok | 78.41 |
| `synth_lattice_n3000` | 3000 | 5 | cpu | ok | 120.61 |
| `synth_lattice_n10000` | 10000 | 5 | cpu | ok | 312.01 |
| `synth_sphere_n100` | 100 | 5 | appareil | ok | 23.27 |
| `synth_sphere_n300` | 300 | 5 | appareil | ok | 77.37 |
| `synth_sphere_n1000` | 1000 | 5 | appareil | ok | 246.91 |
| `synth_sphere_n3000` | 3000 | 5 | appareil | refus (unsupported_degeneracy/wide_leaf) | — |
| `synth_sphere_n10000` | 10000 | 5 | appareil | refus (unsupported_degeneracy/wide_leaf) | — |
| `synth_line_n100` | 100 | 5 | appareil | ok | 4.35 |
| `synth_line_n1000` | 1000 | 5 | appareil | ok | 6.52 |
| `synth_lattice_n100` | 100 | 5 | appareil | ok | 15.82 |
| `synth_lattice_n300` | 300 | 5 | appareil | ok | 33.27 |
| `synth_lattice_n1000` | 1000 | 5 | appareil | ok | 62.84 |
| `synth_lattice_n3000` | 3000 | 5 | appareil | ok | 95.65 |
| `synth_lattice_n10000` | 10000 | 5 | appareil | ok | 259.13 |
| `synth_sphere_n100` | 100 | 10 | cpu | non_joue (delai) | — |
| `synth_sphere_n300` | 300 | 10 | cpu | non_joue (delai) | — |
| `synth_sphere_n1000` | 1000 | 10 | cpu | non_joue (delai) | — |
| `synth_sphere_n3000` | 3000 | 10 | cpu | non_joue (delai) | — |
| `synth_sphere_n10000` | 10000 | 10 | cpu | non_joue (delai) | — |
| `synth_line_n100` | 100 | 10 | cpu | non_joue (delai) | — |
| `synth_line_n1000` | 1000 | 10 | cpu | non_joue (delai) | — |
| `synth_lattice_n100` | 100 | 10 | cpu | non_joue (delai) | — |
| `synth_lattice_n300` | 300 | 10 | cpu | non_joue (delai) | — |
| `synth_lattice_n1000` | 1000 | 10 | cpu | non_joue (delai) | — |
| `synth_lattice_n3000` | 3000 | 10 | cpu | non_joue (delai) | — |
| `synth_lattice_n10000` | 10000 | 10 | cpu | non_joue (delai) | — |
| `synth_sphere_n100` | 100 | 10 | appareil | non_joue (delai) | — |
| `synth_sphere_n300` | 300 | 10 | appareil | non_joue (delai) | — |
| `synth_sphere_n1000` | 1000 | 10 | appareil | non_joue (delai) | — |
| `synth_sphere_n3000` | 3000 | 10 | appareil | non_joue (delai) | — |
| `synth_sphere_n10000` | 10000 | 10 | appareil | non_joue (delai) | — |
| `synth_line_n100` | 100 | 10 | appareil | non_joue (delai) | — |
| `synth_line_n1000` | 1000 | 10 | appareil | non_joue (delai) | — |
| `synth_lattice_n100` | 100 | 10 | appareil | non_joue (delai) | — |
| `synth_lattice_n300` | 300 | 10 | appareil | non_joue (delai) | — |
| `synth_lattice_n1000` | 1000 | 10 | appareil | non_joue (delai) | — |
| `synth_lattice_n3000` | 3000 | 10 | appareil | non_joue (delai) | — |
| `synth_lattice_n10000` | 10000 | 10 | appareil | non_joue (delai) | — |

Controles manquants : 1.
- Session appareil:10:1 : echec (expire)
