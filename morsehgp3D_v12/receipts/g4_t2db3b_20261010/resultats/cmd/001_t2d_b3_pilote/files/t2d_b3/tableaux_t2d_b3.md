# Chantier T2-d-B3 : mur FULL par levier (genere par pilote_t2d_b3.py)

Verdicts : lot_b3 **adopte**, cles **adopte**, balayage **rejete**

Identite FUL1 : {"empreintes": {"ful1": {"kitti_ng_02_001606": ["fe47826e6624e174"], "kitti_ng_08_001176": ["0ebdb26063348554"], "ng00": ["3a2bfb4f9f48b4b0"], "ng01": ["2d58a72a623ab293"], "ng02": ["27d7650add157cdd"]}, "resolution": {"kitti_ng_02_001606": ["9abcd15d70345074"], "ng00": ["e5a81154fb1b15f1"]}}, "etat": "etablie"}

| trame | mur avant (ms) | mur avant_bis (ms) | mur cles (ms) | mur balayage (ms) | mur transfert (ms) | mur apres (ms) |
| --- | --- | --- | --- | --- | --- | --- |
| ng00 | 79.86 | 80.09 | 78.63 | 79.53 | 80.55 | 78.62 |
| ng01 | 66.07 | 66.16 | 64.90 | 65.81 | 66.54 | 65.02 |
| ng02 | 79.35 | 79.43 | 78.12 | 79.15 | 79.99 | 78.06 |
| kitti_ng_02_001606 | 125.34 | 125.95 | 122.92 | 124.89 | 126.35 | 122.94 |
| kitti_ng_08_001176 | 146.74 | 146.20 | 142.57 | 146.89 | 147.97 | 143.11 |

| trame | lot_b3 (IC 95 %) | cles (IC 95 %) | balayage (IC 95 %) | transfert (IC 95 %) | cles_apres_transfert (IC 95 %) | balayage_apres_cles (IC 95 %) | A/A (IC 95 %) |
| --- | --- | --- | --- | --- | --- | --- | --- |
| ng00 | 0.983 (0.977-0.989) | 0.982 (0.978-0.986) | 0.994 (0.990-0.998) | 1.006 (1.002-1.010) | 0.976 (0.975-0.978) | 1.001 (0.997-1.005) | 1.000 (0.998-1.002) |
| ng01 | 0.987 (0.980-0.998) | 0.986 (0.978-0.999) | 0.995 (0.992-0.997) | 1.010 (1.003-1.020) | 0.976 (0.964-0.989) | 1.001 (0.986-1.016) | 1.001 (0.999-1.004) |
| ng02 | 0.984 (0.982-0.985) | 0.984 (0.981-0.986) | 1.004 (0.995-1.022) | 1.008 (1.006-1.011) | 0.976 (0.974-0.978) | 1.000 (0.998-1.002) | 1.001 (0.999-1.002) |
| kitti_ng_02_001606 | 0.981 (0.980-0.983) | 0.980 (0.978-0.982) | 0.996 (0.994-0.998) | 1.009 (1.006-1.012) | 0.971 (0.968-0.974) | 1.001 (1.000-1.003) | 1.004 (1.001-1.008) |
| kitti_ng_08_001176 | 0.975 (0.972-0.977) | 0.971 (0.969-0.974) | 0.999 (0.996-1.002) | 1.008 (1.005-1.010) | 0.964 (0.961-0.967) | 1.004 (0.999-1.007) | 0.998 (0.996-1.001) |

Informations (jamais jugees) :

```json
{
 "campagne": {
  "kitti_ng_02_001606": {
   "apres": {
    "c": 40.868393,
    "fin_etage": 3.0393835,
    "fin_g": 66.6261065,
    "murs": 122.9396475,
    "ouverture": 13.6500745,
    "tables": 8.287437,
    "transferts": 4.9070925
   },
   "avant": {
    "c": 39.951238,
    "fin_etage": 3.0442185,
    "fin_g": 71.275239,
    "murs": 125.344436,
    "ouverture": 13.6366095,
    "tables": 8.2630865,
    "transferts": 4.0476675
   },
   "avant_bis": {
    "c": 40.119823,
    "fin_etage": 3.0383445,
    "fin_g": 71.6717195,
    "murs": 125.947941,
    "ouverture": 13.6335595,
    "tables": 8.3252915,
    "transferts": 4.2081925
   },
   "balayage": {
    "c": 40.033348,
    "fin_etage": 3.0508995,
    "fin_g": 70.36808,
    "murs": 124.891417,
    "ouverture": 13.548984,
    "tables": 8.2793115,
    "transferts": 4.1236025
   },
   "cles": {
    "c": 41.035078,
    "fin_etage": 3.047743,
    "fin_g": 66.140892,
    "murs": 122.920248,
    "ouverture": 13.629574,
    "tables": 8.3242065,
    "transferts": 5.069018
   },
   "transfert": {
    "c": 40.9544425,
    "fin_etage": 3.0461035,
    "fin_g": 71.1829145,
    "murs": 126.353066,
    "ouverture": 13.571954,
    "tables": 8.2740915,
    "transferts": 4.9793275
   }
  },
  "kitti_ng_08_001176": {
   "apres": {
    "c": 44.466037,
    "fin_etage": 3.6470385,
    "fin_g": 81.755464,
    "murs": 143.109132,
    "ouverture": 16.0014785,
    "tables": 9.9352705,
    "transferts": 5.6500425
   },
   "avant": {
    "c": 43.4398825,
    "fin_etage": 3.6481035,
    "fin_g": 88.1149465,
    "murs": 146.7448,
    "ouverture": 15.900309,
    "tables": 9.8475205,
    "transferts": 4.6301925
   },
   "avant_bis": {
    "c": 43.5645275,
    "fin_etage": 3.648683,
    "fin_g": 87.4355195,
    "murs": 146.199555,
    "ouverture": 15.8326285,
    "tables": 9.7298705,
    "transferts": 4.6654285
   },
   "balayage": {
    "c": 43.5561105,
    "fin_etage": 3.665184,
    "fin_g": 87.461587,
    "murs": 146.893236,
    "ouverture": 15.9853235,
    "tables": 9.850396,
    "transferts": 4.5947075
   },
   "cles": {
    "c": 44.2475125,
    "fin_etage": 3.657794,
    "fin_g": 80.745797,
    "murs": 142.569367,
    "ouverture": 15.9031135,
    "tables": 9.860481,
    "transferts": 5.4019285
   },
   "transfert": {
    "c": 44.5014635,
    "fin_etage": 3.6438885,
    "fin_g": 88.067187,
    "murs": 147.9711545,
    "ouverture": 15.8841535,
    "tables": 9.843851,
    "transferts": 5.611938
   }
  },
  "kitti_ng_08_002119": {
   "apres": {
    "c": 70.362669,
    "fin_etage": 6.146847,
    "fin_g": 140.095419,
    "murs": 236.129706,
    "ouverture": 24.17791,
    "tables": 15.287403,
    "transferts": 8.380017
   },
   "avant": {
    "c": 68.9145,
    "fin_etage": 6.175408,
    "fin_g": 151.258214,
    "murs": 243.991653,
    "ouverture": 24.389789,
    "tables": 15.597673,
    "transferts": 6.900228
   },
   "avant_bis": {
    "c": 69.05256,
    "fin_etage": 6.192787,
    "fin_g": 151.039084,
    "murs": 243.610693,
    "ouverture": 24.705119,
    "tables": 15.596224,
    "transferts": 6.830948
   },
   "balayage": {
    "c": 68.53264,
    "fin_etage": 6.194187,
    "fin_g": 150.625234,
    "murs": 243.115744,
    "ouverture": 24.55129,
    "tables": 15.515183,
    "transferts": 6.328708
   },
   "cles": {
    "c": 70.171769,
    "fin_etage": 6.172457,
    "fin_g": 138.41202,
    "murs": 235.560947,
    "ouverture": 24.511979,
    "tables": 15.479153,
    "transferts": 7.960287
   },
   "transfert": {
    "c": 70.012259,
    "fin_etage": 6.159368,
    "fin_g": 151.991644,
    "murs": 245.828492,
    "ouverture": 24.528009,
    "tables": 15.674233,
    "transferts": 7.963437
   }
  },
  "ng00": {
   "apres": {
    "c": 26.2414625,
    "fin_etage": 2.291794,
    "fin_g": 43.243842,
    "murs": 78.6181445,
    "ouverture": 8.314068,
    "tables": 5.056738,
    "transferts": 3.0714095
   },
   "avant": {
    "c": 25.869695,
    "fin_etage": 2.291749,
    "fin_g": 45.436787,
    "murs": 79.860479,
    "ouverture": 8.3154875,
    "tables": 5.099623,
    "transferts": 2.6437295
   },
   "avant_bis": {
    "c": 25.8631725,
    "fin_etage": 2.296074,
    "fin_g": 45.434933,
    "murs": 80.0910015,
    "ouverture": 8.301167,
    "tables": 5.020613,
    "transferts": 2.6688095
   },
   "balayage": {
    "c": 25.841005,
    "fin_etage": 2.2928035,
    "fin_g": 45.079852,
    "murs": 79.525179,
    "ouverture": 8.273682,
    "tables": 5.059188,
    "transferts": 2.6333595
   },
   "cles": {
    "c": 26.2211695,
    "fin_etage": 2.294309,
    "fin_g": 43.1320975,
    "murs": 78.6279595,
    "ouverture": 8.3550365,
    "tables": 5.086793,
    "transferts": 3.074684
   },
   "transfert": {
    "c": 26.2479625,
    "fin_etage": 2.2956645,
    "fin_g": 45.4163775,
    "murs": 80.5468035,
    "ouverture": 8.317432,
    "tables": 5.043003,
    "transferts": 3.108459
   }
  },
  "ng01": {
   "apres": {
    "c": 23.11645,
    "fin_etage": 2.1337995,
    "fin_g": 33.7794685,
    "murs": 65.0215815,
    "ouverture": 7.200428,
    "tables": 4.285869,
    "transferts": 2.9772685
   },
   "avant": {
    "c": 22.665551,
    "fin_etage": 2.1357995,
    "fin_g": 35.586203,
    "murs": 66.070014,
    "ouverture": 7.220183,
    "tables": 4.3356685,
    "transferts": 2.522479
   },
   "avant_bis": {
    "c": 22.6978325,
    "fin_etage": 2.129215,
    "fin_g": 35.5347435,
    "murs": 66.1554325,
    "ouverture": 7.204492,
    "tables": 4.3203035,
    "transferts": 2.5085945
   },
   "balayage": {
    "c": 22.734038,
    "fin_etage": 2.133674,
    "fin_g": 35.1059095,
    "murs": 65.8135755,
    "ouverture": 7.152152,
    "tables": 4.2851585,
    "transferts": 2.547004
   },
   "cles": {
    "c": 23.126458,
    "fin_etage": 2.133044,
    "fin_g": 33.5934235,
    "murs": 64.9044695,
    "ouverture": 7.124038,
    "tables": 4.268304,
    "transferts": 2.9760445
   },
   "transfert": {
    "c": 23.125693,
    "fin_etage": 2.129989,
    "fin_g": 35.5133735,
    "murs": 66.5377385,
    "ouverture": 7.209338,
    "tables": 4.316029,
    "transferts": 2.9446135
   }
  },
  "ng02": {
   "apres": {
    "c": 26.490259,
    "fin_etage": 2.476319,
    "fin_g": 41.386892,
    "murs": 78.056891,
    "ouverture": 9.232901,
    "tables": 5.493493,
    "transferts": 3.3133435
   },
   "avant": {
    "c": 25.984974,
    "fin_etage": 2.4765495,
    "fin_g": 43.769791,
    "murs": 79.3512905,
    "ouverture": 9.240876,
    "tables": 5.5433475,
    "transferts": 2.854309
   },
   "avant_bis": {
    "c": 26.028064,
    "fin_etage": 2.4780895,
    "fin_g": 43.823386,
    "murs": 79.431165,
    "ouverture": 9.262066,
    "tables": 5.563687,
    "transferts": 2.9077235
   },
   "balayage": {
    "c": 25.993119,
    "fin_etage": 2.475664,
    "fin_g": 43.089671,
    "murs": 79.1538255,
    "ouverture": 9.194916,
    "tables": 5.479688,
    "transferts": 2.8890375
   },
   "cles": {
    "c": 26.4014185,
    "fin_etage": 2.475749,
    "fin_g": 41.0881225,
    "murs": 78.1220655,
    "ouverture": 9.226616,
    "tables": 5.5145675,
    "transferts": 3.2956725
   },
   "transfert": {
    "c": 26.464164,
    "fin_etage": 2.472889,
    "fin_g": 43.701356,
    "murs": 79.993655,
    "ouverture": 9.337631,
    "tables": 5.5675675,
    "transferts": 3.322199
   }
  }
 },
 "k10": {
  "ng00": {
   "apres": 480.119711,
   "avant": 499.6860015
  },
  "ng01": {
   "apres": 360.200278,
   "avant": 376.846781
  },
  "ng02": {
   "apres": 418.3335325,
   "avant": 436.9907545
  }
 },
 "profil": {
  "kitti_ng_02_001606": {
   "apres": {
    "certificat_ns": 278.9066562886378,
    "resolution_ms": 58.819062,
    "t1_ns": 397.0078158226833,
    "t1_part": 0.2092546806596992,
    "t1_temps_fil_ms": 569.4164
   },
   "avant": {
    "certificat_ns": 275.6803865586158,
    "resolution_ms": 61.597832,
    "t1_ns": 547.0191177393377,
    "t1_part": 0.2745331834655964,
    "t1_temps_fil_ms": 784.57311
   }
  },
  "ng00": {
   "apres": {
    "certificat_ns": 273.70724800913734,
    "resolution_ms": 40.180422,
    "t1_ns": 363.77983344546965,
    "t1_part": 0.20040918922734385,
    "t1_temps_fil_ms": 368.20376
   },
   "avant": {
    "certificat_ns": 272.7947031948983,
    "resolution_ms": 42.42446,
    "t1_ns": 484.01321528887195,
    "t1_part": 0.2526523224934692,
    "t1_temps_fil_ms": 489.8992999999999
   }
  }
 },
 "sequentiel": {
  "kitti_ng_02_001606": {
   "g_bras_sur_avant_moyenne_geometrique": {
    "apres": 0.9397723926382728,
    "balayage": 0.9867173653384002,
    "cles": 0.9360891148949247
   },
   "medianes": {
    "apres": {
     "g_ms": 62.238603,
     "mur_ms": 197.868224
    },
    "avant": {
     "g_ms": 66.615471,
     "mur_ms": 198.863293
    },
    "balayage": {
     "g_ms": 65.495332,
     "mur_ms": 199.242714
    },
    "cles": {
     "g_ms": 62.077343,
     "mur_ms": 196.732154
    }
   }
  },
  "ng00": {
   "g_bras_sur_avant_moyenne_geometrique": {
    "apres": 0.960025263608228,
    "balayage": 0.9887153642147233,
    "cles": 0.9528202395400761
   },
   "medianes": {
    "apres": {
     "g_ms": 43.591301,
     "mur_ms": 122.637706
    },
    "avant": {
     "g_ms": 45.06293,
     "mur_ms": 124.263696
    },
    "balayage": {
     "g_ms": 44.311341,
     "mur_ms": 123.661997
    },
    "cles": {
     "g_ms": 43.310431,
     "mur_ms": 122.953746
    }
   }
  }
 }
}
```
