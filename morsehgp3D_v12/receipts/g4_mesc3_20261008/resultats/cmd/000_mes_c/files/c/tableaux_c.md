# MES-C : petits nuages, tour FULL de la v12

Verdict d'ensemble : **non tenu**.

- C1 : non tenu — cout fixe 14.902 ms
- C2 : non tenu — 10.064 us par site (limite 3.727)
- C3 : non tenu — synth_sphere_n3000 cpu : refus (unsupported_degeneracy/wide_leaf) ; synth_sphere_n10000 cpu : refus (unsupported_degeneracy/wide_leaf) ; synth_sphere_n3000 appareil : refus (unsupported_degeneracy/wide_leaf) ; synth_sphere_n10000 appareil : refus (unsupported_degeneracy/wide_leaf)

Droites t = a + b n sur les valeurs chaudes (a en ms, b en us par site) :

| configuration (voie:K:fils) | etat | groupe | nuages | a (ms) | b (us/site) |
| --- | --- | --- | ---: | ---: | ---: |
| cpu:5:4 | ok | clusters8 | 5 | -15.058 | 100.923 |
| cpu:5:4 | ok | reel | 132 | -1.277 | 54.111 |
| cpu:5:4 | ok | slab | 5 | 3.135 | 15.214 |
| cpu:5:4 | ok | uniform | 5 | 90.514 | 88.092 |
| cpu:5:48 | ok | clusters8 | 5 | 18.625 | 15.567 |
| cpu:5:48 | ok | reel | 132 | 14.902 | 10.064 |
| cpu:5:48 | ok | slab | 5 | 8.195 | 3.545 |
| cpu:5:48 | ok | uniform | 5 | 49.158 | 11.269 |
| appareil:5:4 | ok | clusters8 | 5 | 1.005 | 25.898 |
| appareil:5:4 | ok | reel | 132 | 2.595 | 9.469 |
| appareil:5:4 | ok | slab | 5 | 3.064 | 4.071 |
| appareil:5:4 | ok | uniform | 5 | 119.104 | 15.283 |
| appareil:5:48 | ok | clusters8 | 5 | 10.556 | 4.100 |
| appareil:5:48 | ok | reel | 132 | 5.394 | 2.176 |
| appareil:5:48 | ok | slab | 5 | 4.410 | 1.030 |
| appareil:5:48 | ok | uniform | 5 | 48.049 | -0.075 |
| cpu:10:4 | ok | clusters8 | 5 | -170.939 | 563.256 |
| cpu:10:4 | ok | reel | 132 | -32.496 | 234.810 |
| cpu:10:4 | ok | slab | 5 | -2.634 | 50.573 |
| cpu:10:4 | ok | uniform | 5 | 168.689 | 563.189 |
| cpu:10:48 | ok | clusters8 | 5 | 8.468 | 71.178 |
| cpu:10:48 | ok | reel | 132 | 16.852 | 33.872 |
| cpu:10:48 | ok | slab | 5 | 10.623 | 9.214 |
| cpu:10:48 | ok | uniform | 5 | 65.349 | 68.196 |
| appareil:10:4 | ok | clusters8 | 5 | -84.376 | 264.914 |
| appareil:10:4 | ok | reel | 132 | -17.425 | 75.959 |
| appareil:10:4 | ok | slab | 5 | -1.276 | 20.831 |
| appareil:10:4 | ok | uniform | 5 | 289.519 | 256.010 |
| appareil:10:48 | ok | clusters8 | 5 | 6.929 | 33.036 |
| appareil:10:48 | ok | reel | 132 | 5.374 | 10.941 |
| appareil:10:48 | ok | slab | 5 | 4.883 | 3.656 |
| appareil:10:48 | ok | uniform | 5 | 69.655 | 29.213 |

Familles difficiles (chaque nuage seul) :

| nuage | sites | K | voie | etat | mur (ms, derniere passe) |
| --- | ---: | ---: | --- | --- | ---: |
| `synth_sphere_n100` | 100 | 5 | cpu | ok | 27.24 |
| `synth_sphere_n300` | 300 | 5 | cpu | ok | 89.44 |
| `synth_sphere_n1000` | 1000 | 5 | cpu | ok | 259.64 |
| `synth_sphere_n3000` | 3000 | 5 | cpu | refus (unsupported_degeneracy/wide_leaf) | — |
| `synth_sphere_n10000` | 10000 | 5 | cpu | refus (unsupported_degeneracy/wide_leaf) | — |
| `synth_line_n100` | 100 | 5 | cpu | ok | 3.11 |
| `synth_line_n1000` | 1000 | 5 | cpu | ok | 7.51 |
| `synth_lattice_n100` | 100 | 5 | cpu | ok | 12.22 |
| `synth_lattice_n300` | 300 | 5 | cpu | ok | 27.65 |
| `synth_lattice_n1000` | 1000 | 5 | cpu | ok | 50.27 |
| `synth_lattice_n3000` | 3000 | 5 | cpu | ok | 100.10 |
| `synth_lattice_n10000` | 10000 | 5 | cpu | ok | 226.86 |
| `synth_sphere_n100` | 100 | 5 | appareil | ok | 19.74 |
| `synth_sphere_n300` | 300 | 5 | appareil | ok | 72.12 |
| `synth_sphere_n1000` | 1000 | 5 | appareil | ok | 242.60 |
| `synth_sphere_n3000` | 3000 | 5 | appareil | refus (unsupported_degeneracy/wide_leaf) | — |
| `synth_sphere_n10000` | 10000 | 5 | appareil | refus (unsupported_degeneracy/wide_leaf) | — |
| `synth_line_n100` | 100 | 5 | appareil | ok | 2.32 |
| `synth_line_n1000` | 1000 | 5 | appareil | ok | 3.75 |
| `synth_lattice_n100` | 100 | 5 | appareil | ok | 9.44 |
| `synth_lattice_n300` | 300 | 5 | appareil | ok | 22.20 |
| `synth_lattice_n1000` | 1000 | 5 | appareil | ok | 40.36 |
| `synth_lattice_n3000` | 3000 | 5 | appareil | ok | 61.56 |
| `synth_lattice_n10000` | 10000 | 5 | appareil | ok | 165.17 |
| `synth_sphere_n100` | 100 | 10 | cpu | ok | 84.07 |
| `synth_sphere_n300` | 300 | 10 | cpu | ok | 576.18 |
| `synth_sphere_n1000` | 1000 | 10 | cpu | ok | 1619.46 |
| `synth_sphere_n3000` | 3000 | 10 | cpu | refus (unsupported_degeneracy/wide_leaf) | — |
| `synth_sphere_n10000` | 10000 | 10 | cpu | refus (unsupported_degeneracy/wide_leaf) | — |
| `synth_line_n100` | 100 | 10 | cpu | ok | 3.06 |
| `synth_line_n1000` | 1000 | 10 | cpu | ok | 9.84 |
| `synth_lattice_n100` | 100 | 10 | cpu | ok | 108.76 |
| `synth_lattice_n300` | 300 | 10 | cpu | ok | 519.16 |
| `synth_lattice_n1000` | 1000 | 10 | cpu | ok | 2495.96 |
| `synth_lattice_n3000` | 3000 | 10 | cpu | ok | 7301.60 |
| `synth_lattice_n10000` | 10000 | 10 | cpu | ok | 11597.62 |
| `synth_sphere_n100` | 100 | 10 | appareil | ok | 58.45 |
| `synth_sphere_n300` | 300 | 10 | appareil | ok | 554.24 |
| `synth_sphere_n1000` | 1000 | 10 | appareil | ok | 1575.81 |
| `synth_sphere_n3000` | 3000 | 10 | appareil | refus (unsupported_degeneracy/wide_leaf) | — |
| `synth_sphere_n10000` | 10000 | 10 | appareil | refus (unsupported_degeneracy/wide_leaf) | — |
| `synth_line_n100` | 100 | 10 | appareil | ok | 3.11 |
| `synth_line_n1000` | 1000 | 10 | appareil | ok | 4.41 |
| `synth_lattice_n100` | 100 | 10 | appareil | ok | 90.10 |
| `synth_lattice_n300` | 300 | 10 | appareil | ok | 498.43 |
| `synth_lattice_n1000` | 1000 | 10 | appareil | ok | 2421.54 |
| `synth_lattice_n3000` | 3000 | 10 | appareil | ok | 7117.67 |
| `synth_lattice_n10000` | 10000 | 10 | appareil | ok | 10946.55 |

Controles manquants : 0.
