# Chantier T2-d-B2 : mur FULL par levier (genere par pilote_t2d_b2.py)

Verdicts : lot_b2 **adopte**, tables **adopte**, relecture **rejete**, census **rejete**

Identite FUL1 : {"empreintes": {"kitti_ng_02_001606": ["fe47826e6624e174"], "kitti_ng_08_001176": ["0ebdb26063348554"], "ng00": ["3a2bfb4f9f48b4b0"], "ng01": ["2d58a72a623ab293"], "ng02": ["27d7650add157cdd"]}, "etat": "etablie"}

| trame | mur avant (ms) | mur avant_bis (ms) | mur tables (ms) | mur relecture (ms) | mur census (ms) | mur apres (ms) |
| --- | --- | --- | --- | --- | --- | --- |
| ng00 | 87.29 | 87.37 | 84.76 | 87.44 | 85.21 | 82.63 |
| ng01 | 71.63 | 71.65 | 69.11 | 71.85 | 70.27 | 67.77 |
| ng02 | 88.07 | 88.02 | 85.32 | 88.11 | 87.51 | 84.87 |
| kitti_ng_02_001606 | 149.91 | 150.14 | 147.87 | 149.95 | 149.58 | 147.50 |
| kitti_ng_08_001176 | 175.64 | 175.35 | 172.02 | 175.23 | 174.88 | 172.66 |

| trame | lot_b2 (IC 95 %) | tables (IC 95 %) | relecture (IC 95 %) | census (IC 95 %) | relecture_apres_tables (IC 95 %) | tables_apres_relecture (IC 95 %) | A/A (IC 95 %) |
| --- | --- | --- | --- | --- | --- | --- | --- |
| ng00 | 0.944 (0.939-0.948) | 0.970 (0.966-0.974) | 1.000 (0.994-1.005) | 0.975 (0.971-0.978) | 0.973 (0.969-0.976) | 0.944 (0.940-0.948) | 0.999 (0.994-1.005) |
| ng01 | 0.948 (0.944-0.951) | 0.968 (0.962-0.977) | 1.003 (0.997-1.008) | 0.980 (0.975-0.984) | 0.978 (0.970-0.985) | 0.945 (0.937-0.952) | 1.000 (0.994-1.005) |
| ng02 | 0.963 (0.959-0.968) | 0.970 (0.966-0.974) | 1.000 (0.995-1.004) | 0.996 (0.991-1.000) | 0.993 (0.991-0.996) | 0.963 (0.959-0.968) | 1.001 (0.996-1.006) |
| kitti_ng_02_001606 | 0.982 (0.978-0.985) | 0.985 (0.982-0.988) | 0.999 (0.994-1.004) | 0.996 (0.992-1.000) | 0.996 (0.993-1.000) | 0.983 (0.978-0.987) | 1.000 (0.996-1.005) |
| kitti_ng_08_001176 | 0.984 (0.980-0.988) | 0.980 (0.975-0.984) | 0.998 (0.995-1.001) | 0.995 (0.991-0.999) | 1.004 (0.997-1.011) | 0.986 (0.980-0.992) | 0.999 (0.994-1.003) |

Informations (jamais jugees) :

```json
{
 "campagne": {
  "kitti_ng_02_001606": {
   "apres": {
    "fin_g": 66.8377875,
    "murs": 147.5034665,
    "ouverture": 13.051643,
    "tables": 8.3328405
   },
   "avant": {
    "fin_g": 71.90969,
    "murs": 149.912206,
    "ouverture": 15.2241815,
    "tables": 10.512199
   },
   "avant_bis": {
    "fin_g": 71.810269,
    "murs": 150.142821,
    "ouverture": 15.259611,
    "tables": 10.539529
   },
   "census": {
    "fin_g": 69.181532,
    "murs": 149.581457,
    "ouverture": 15.2506515,
    "tables": 10.5378245
   },
   "relecture": {
    "fin_g": 71.641019,
    "murs": 149.948172,
    "ouverture": 15.223156,
    "tables": 10.548594
   },
   "tables": {
    "fin_g": 69.7721205,
    "murs": 147.8695585,
    "ouverture": 12.9669325,
    "tables": 8.29695
   }
  },
  "kitti_ng_08_001176": {
   "apres": {
    "fin_g": 80.8344355,
    "murs": 172.65821,
    "ouverture": 14.757861,
    "tables": 9.4986095
   },
   "avant": {
    "fin_g": 87.64314,
    "murs": 175.6357135,
    "ouverture": 17.01434,
    "tables": 11.773618
   },
   "avant_bis": {
    "fin_g": 87.3752315,
    "murs": 175.348103,
    "ouverture": 17.102451,
    "tables": 11.8082665
   },
   "census": {
    "fin_g": 83.2954845,
    "murs": 174.881121,
    "ouverture": 16.98435,
    "tables": 11.7201685
   },
   "relecture": {
    "fin_g": 87.4215425,
    "murs": 175.2257755,
    "ouverture": 16.973971,
    "tables": 11.6900585
   },
   "tables": {
    "fin_g": 85.3589635,
    "murs": 172.021291,
    "ouverture": 14.759522,
    "tables": 9.473575
   }
  },
  "kitti_ng_08_002119": {
   "apres": {
    "fin_g": 140.019922,
    "murs": 286.792528,
    "ouverture": 22.527276,
    "tables": 14.726642
   },
   "avant": {
    "fin_g": 148.917551,
    "murs": 291.892106,
    "ouverture": 24.747894,
    "tables": 16.93658
   },
   "avant_bis": {
    "fin_g": 149.239271,
    "murs": 291.039007,
    "ouverture": 24.709585,
    "tables": 17.056189
   },
   "census": {
    "fin_g": 141.995465,
    "murs": 288.568158,
    "ouverture": 24.212285,
    "tables": 16.491
   },
   "relecture": {
    "fin_g": 148.647351,
    "murs": 289.979798,
    "ouverture": 24.628216,
    "tables": 16.83993
   },
   "tables": {
    "fin_g": 146.767547,
    "murs": 288.047807,
    "ouverture": 22.502897,
    "tables": 14.749241
   }
  },
  "ng00": {
   "apres": {
    "fin_g": 45.7165555,
    "murs": 82.62993,
    "ouverture": 8.030636,
    "tables": 5.088132
   },
   "avant": {
    "fin_g": 53.9877255,
    "murs": 87.286083,
    "ouverture": 10.4856095,
    "tables": 7.490551
   },
   "avant_bis": {
    "fin_g": 53.312396,
    "murs": 87.3700075,
    "ouverture": 10.574894,
    "tables": 7.581716
   },
   "census": {
    "fin_g": 48.3338835,
    "murs": 85.2075995,
    "ouverture": 10.580724,
    "tables": 7.573706
   },
   "relecture": {
    "fin_g": 53.704626,
    "murs": 87.4385435,
    "ouverture": 10.7005545,
    "tables": 7.741251
   },
   "tables": {
    "fin_g": 50.9531625,
    "murs": 84.7599745,
    "ouverture": 8.0432455,
    "tables": 5.020382
   }
  },
  "ng01": {
   "apres": {
    "fin_g": 35.438031,
    "murs": 67.765654,
    "ouverture": 6.960941,
    "tables": 4.3039425
   },
   "avant": {
    "fin_g": 39.7636435,
    "murs": 71.626247,
    "ouverture": 9.51217,
    "tables": 6.8460465
   },
   "avant_bis": {
    "fin_g": 39.525029,
    "murs": 71.647352,
    "ouverture": 9.231985,
    "tables": 6.5789615
   },
   "census": {
    "fin_g": 38.2031395,
    "murs": 70.265248,
    "ouverture": 9.57891,
    "tables": 6.861216
   },
   "relecture": {
    "fin_g": 39.613889,
    "murs": 71.851932,
    "ouverture": 9.414935,
    "tables": 6.823657
   },
   "tables": {
    "fin_g": 37.231865,
    "murs": 69.1089985,
    "ouverture": 6.956846,
    "tables": 4.366853
   }
  },
  "ng02": {
   "apres": {
    "fin_g": 41.9360115,
    "murs": 84.866213,
    "ouverture": 8.826325,
    "tables": 5.532812
   },
   "avant": {
    "fin_g": 46.2208495,
    "murs": 88.0743065,
    "ouverture": 11.5560085,
    "tables": 8.2167355
   },
   "avant_bis": {
    "fin_g": 45.976166,
    "murs": 88.020138,
    "ouverture": 11.245024,
    "tables": 7.9089655
   },
   "census": {
    "fin_g": 44.511297,
    "murs": 87.513569,
    "ouverture": 11.317014,
    "tables": 7.9903355
   },
   "relecture": {
    "fin_g": 46.0002195,
    "murs": 88.1089635,
    "ouverture": 11.281649,
    "tables": 7.947721
   },
   "tables": {
    "fin_g": 43.669732,
    "murs": 85.315685,
    "ouverture": 8.875445,
    "tables": 5.5768725
   }
  }
 },
 "k10": {
  "ng00": {
   "apres": 519.5725775,
   "avant": 548.786924
  },
  "ng01": {
   "apres": 384.933467,
   "avant": 406.272195
  },
  "ng02": {
   "apres": 444.925975,
   "avant": 465.676897
  }
 },
 "sequentiel": {
  "kitti_ng_02_001606": {
   "g_bras_sur_avant_moyenne_geometrique": {
    "apres": 0.9571609881501897,
    "census": 0.963659064698844,
    "relecture": 0.9993493557739485
   },
   "medianes": {
    "apres": {
     "g_ms": 64.392637,
     "mur_ms": 202.100285
    },
    "avant": {
     "g_ms": 67.503595,
     "mur_ms": 203.988214
    },
    "census": {
     "g_ms": 65.005016,
     "mur_ms": 202.683255
    },
    "relecture": {
     "g_ms": 67.200385,
     "mur_ms": 204.309894
    }
   }
  },
  "ng00": {
   "g_bras_sur_avant_moyenne_geometrique": {
    "apres": 0.9458139502732931,
    "census": 0.951255990974585,
    "relecture": 0.9988532093739074
   },
   "medianes": {
    "apres": {
     "g_ms": 44.526691,
     "mur_ms": 126.59188
    },
    "avant": {
     "g_ms": 47.03723,
     "mur_ms": 129.760809
    },
    "census": {
     "g_ms": 44.737942,
     "mur_ms": 126.16987
    },
    "relecture": {
     "g_ms": 47.235601,
     "mur_ms": 128.852478
    }
   }
  }
 }
}
```
