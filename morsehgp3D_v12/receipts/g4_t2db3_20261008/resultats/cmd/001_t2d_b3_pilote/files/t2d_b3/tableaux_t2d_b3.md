# Chantier T2-d-B3 : mur FULL par levier (genere par pilote_t2d_b3.py)

Verdicts : lot_b3 **rejete**, cles **rejete**, balayage **rejete**

Identite FUL1 : {"empreintes": {"ful1": {"kitti_ng_02_001606": ["fe47826e6624e174"], "kitti_ng_08_001176": ["0ebdb26063348554"], "ng00": ["3a2bfb4f9f48b4b0"], "ng01": ["2d58a72a623ab293"], "ng02": ["27d7650add157cdd"]}, "resolution": {"kitti_ng_02_001606": ["9abcd15d70345074"], "ng00": ["e5a81154fb1b15f1"]}}, "etat": "etablie"}

| trame | mur avant (ms) | mur avant_bis (ms) | mur cles (ms) | mur balayage (ms) | mur transfert (ms) | mur apres (ms) |
| --- | --- | --- | --- | --- | --- | --- |
| ng00 | 80.02 | 80.10 | 78.69 | 80.07 | 80.15 | 78.36 |
| ng01 | 66.10 | 66.03 | 64.83 | 65.90 | 66.21 | 65.05 |
| ng02 | 83.38 | 83.29 | 84.08 | 82.98 | 83.34 | 84.15 |
| kitti_ng_02_001606 | 144.84 | 144.80 | 144.25 | 144.26 | 145.14 | 144.10 |
| kitti_ng_08_001176 | 169.17 | 169.05 | 166.96 | 168.45 | 169.67 | 167.88 |

| trame | lot_b3 (IC 95 %) | cles (IC 95 %) | balayage (IC 95 %) | transfert (IC 95 %) | cles_apres_transfert (IC 95 %) | balayage_apres_cles (IC 95 %) | A/A (IC 95 %) |
| --- | --- | --- | --- | --- | --- | --- | --- |
| ng00 | 0.980 (0.977-0.983) | 0.984 (0.981-0.987) | 0.998 (0.996-1.001) | 1.000 (0.997-1.003) | 0.984 (0.981-0.987) | 0.996 (0.993-1.000) | 1.000 (0.999-1.002) |
| ng01 | 1.006 (0.981-1.047) | 0.980 (0.976-0.983) | 0.997 (0.994-1.000) | 1.002 (0.999-1.006) | 0.978 (0.974-0.981) | 1.027 (1.000-1.069) | 0.999 (0.996-1.002) |
| ng02 | 1.011 (1.006-1.017) | 1.009 (1.006-1.013) | 0.996 (0.994-0.999) | 1.000 (0.996-1.005) | 1.009 (1.006-1.012) | 1.002 (0.998-1.006) | 1.000 (0.997-1.003) |
| kitti_ng_02_001606 | 0.995 (0.991-0.999) | 0.996 (0.995-0.998) | 0.997 (0.992-1.001) | 1.002 (0.999-1.006) | 0.994 (0.990-0.997) | 0.998 (0.995-1.003) | 1.000 (0.996-1.004) |
| kitti_ng_08_001176 | 0.994 (0.990-0.997) | 0.989 (0.986-0.991) | 0.997 (0.995-0.999) | 1.005 (1.001-1.010) | 0.983 (0.977-0.989) | 1.005 (1.001-1.009) | 1.001 (0.997-1.004) |

Informations (jamais jugees) :

```json
{
 "campagne": {
  "kitti_ng_02_001606": {
   "apres": {
    "c": 40.548207,
    "fin_etage": 3.026345,
    "fin_g": 62.1304005,
    "murs": 144.0957035,
    "ouverture": 13.083304,
    "tables": 8.2974995,
    "transferts": 4.9011645
   },
   "avant": {
    "c": 39.676907,
    "fin_etage": 3.0303645,
    "fin_g": 66.993957,
    "murs": 144.8432595,
    "ouverture": 13.153939,
    "tables": 8.2787195,
    "transferts": 4.037265
   },
   "avant_bis": {
    "c": 39.7752165,
    "fin_etage": 3.03217,
    "fin_g": 67.072625,
    "murs": 144.796529,
    "ouverture": 13.176069,
    "tables": 8.305289,
    "transferts": 4.0810895
   },
   "balayage": {
    "c": 39.713192,
    "fin_etage": 3.03094,
    "fin_g": 66.6810005,
    "murs": 144.2579595,
    "ouverture": 13.154294,
    "tables": 8.30291,
    "transferts": 4.0767995
   },
   "cles": {
    "c": 40.5247065,
    "fin_etage": 3.0233295,
    "fin_g": 62.0222255,
    "murs": 144.248734,
    "ouverture": 13.217054,
    "tables": 8.358884,
    "transferts": 4.918419
   },
   "transfert": {
    "c": 40.597122,
    "fin_etage": 3.0269545,
    "fin_g": 66.2742545,
    "murs": 145.14297,
    "ouverture": 13.167934,
    "tables": 8.3395895,
    "transferts": 4.924015
   }
  },
  "kitti_ng_08_001176": {
   "apres": {
    "c": 43.760925,
    "fin_etage": 3.649815,
    "fin_g": 74.798384,
    "murs": 167.883982,
    "ouverture": 14.910295,
    "tables": 9.51182,
    "transferts": 5.2503995
   },
   "avant": {
    "c": 42.837264,
    "fin_etage": 3.643185,
    "fin_g": 81.1209315,
    "murs": 169.16739,
    "ouverture": 15.0457645,
    "tables": 9.4444,
    "transferts": 4.422285
   },
   "avant_bis": {
    "c": 42.8873705,
    "fin_etage": 3.647705,
    "fin_g": 81.049891,
    "murs": 169.05425,
    "ouverture": 15.000685,
    "tables": 9.48861,
    "transferts": 4.39394
   },
   "balayage": {
    "c": 42.8804905,
    "fin_etage": 3.649595,
    "fin_g": 80.8324545,
    "murs": 168.4514315,
    "ouverture": 15.03006,
    "tables": 9.521285,
    "transferts": 4.369595
   },
   "cles": {
    "c": 43.613104,
    "fin_etage": 3.649095,
    "fin_g": 74.36155,
    "murs": 166.963,
    "ouverture": 14.8144405,
    "tables": 9.49503,
    "transferts": 5.200065
   },
   "transfert": {
    "c": 43.69752,
    "fin_etage": 3.64943,
    "fin_g": 80.338025,
    "murs": 169.6667835,
    "ouverture": 14.8660295,
    "tables": 9.5077145,
    "transferts": 5.151145
   }
  },
  "kitti_ng_08_002119": {
   "apres": {
    "c": 69.342692,
    "fin_etage": 6.17658,
    "fin_g": 126.975123,
    "murs": 279.683148,
    "ouverture": 22.635061,
    "tables": 14.78527,
    "transferts": 7.5464
   },
   "avant": {
    "c": 68.202082,
    "fin_etage": 6.17794,
    "fin_g": 139.284264,
    "murs": 281.305658,
    "ouverture": 22.824951,
    "tables": 14.823251,
    "transferts": 6.50151
   },
   "avant_bis": {
    "c": 67.813402,
    "fin_etage": 6.16819,
    "fin_g": 139.571624,
    "murs": 282.394328,
    "ouverture": 22.71379,
    "tables": 14.79304,
    "transferts": 6.2297
   },
   "balayage": {
    "c": 67.687062,
    "fin_etage": 6.18292,
    "fin_g": 139.505404,
    "murs": 281.919569,
    "ouverture": 22.784511,
    "tables": 14.83825,
    "transferts": 6.07785
   },
   "cles": {
    "c": 68.940242,
    "fin_etage": 6.15937,
    "fin_g": 127.123933,
    "murs": 278.953459,
    "ouverture": 22.559301,
    "tables": 14.79543,
    "transferts": 7.37244
   },
   "transfert": {
    "c": 69.248112,
    "fin_etage": 6.15879,
    "fin_g": 137.515234,
    "murs": 282.944858,
    "ouverture": 22.674551,
    "tables": 14.882961,
    "transferts": 7.56155
   }
  },
  "ng00": {
   "apres": {
    "c": 25.9899695,
    "fin_etage": 2.26282,
    "fin_g": 42.996765,
    "murs": 78.3561735,
    "ouverture": 8.153715,
    "tables": 5.0949145,
    "transferts": 3.0663105
   },
   "avant": {
    "c": 25.56273,
    "fin_etage": 2.259555,
    "fin_g": 45.7141145,
    "murs": 80.019409,
    "ouverture": 8.125225,
    "tables": 5.101435,
    "transferts": 2.617105
   },
   "avant_bis": {
    "c": 25.662155,
    "fin_etage": 2.257835,
    "fin_g": 45.7564395,
    "murs": 80.103845,
    "ouverture": 8.18311,
    "tables": 5.072075,
    "transferts": 2.653735
   },
   "balayage": {
    "c": 25.58552,
    "fin_etage": 2.26131,
    "fin_g": 45.573385,
    "murs": 80.074174,
    "ouverture": 8.160105,
    "tables": 5.10201,
    "transferts": 2.610595
   },
   "cles": {
    "c": 26.0583345,
    "fin_etage": 2.2577945,
    "fin_g": 42.5647045,
    "murs": 78.69179,
    "ouverture": 8.20613,
    "tables": 5.12596,
    "transferts": 3.067975
   },
   "transfert": {
    "c": 26.09399,
    "fin_etage": 2.2605,
    "fin_g": 45.250009,
    "murs": 80.152259,
    "ouverture": 8.182215,
    "tables": 5.10939,
    "transferts": 3.10759
   }
  },
  "ng01": {
   "apres": {
    "c": 23.0469805,
    "fin_etage": 2.114265,
    "fin_g": 33.828842,
    "murs": 65.0458385,
    "ouverture": 7.1972505,
    "tables": 4.40543,
    "transferts": 3.06049
   },
   "avant": {
    "c": 22.605491,
    "fin_etage": 2.114915,
    "fin_g": 35.7334815,
    "murs": 66.097462,
    "ouverture": 7.175785,
    "tables": 4.3972355,
    "transferts": 2.561205
   },
   "avant_bis": {
    "c": 22.605152,
    "fin_etage": 2.119805,
    "fin_g": 35.6488655,
    "murs": 66.031443,
    "ouverture": 7.16859,
    "tables": 4.3600105,
    "transferts": 2.6409205
   },
   "balayage": {
    "c": 22.713996,
    "fin_etage": 2.11326,
    "fin_g": 35.3571915,
    "murs": 65.900979,
    "ouverture": 7.12252,
    "tables": 4.350895,
    "transferts": 2.6606005
   },
   "cles": {
    "c": 23.0803065,
    "fin_etage": 2.115695,
    "fin_g": 33.569937,
    "murs": 64.832749,
    "ouverture": 7.1884805,
    "tables": 4.334165,
    "transferts": 3.0293145
   },
   "transfert": {
    "c": 23.1242265,
    "fin_etage": 2.117645,
    "fin_g": 35.247637,
    "murs": 66.212313,
    "ouverture": 7.1428205,
    "tables": 4.362485,
    "transferts": 3.04914
   }
  },
  "ng02": {
   "apres": {
    "c": 26.3173905,
    "fin_etage": 2.4623855,
    "fin_g": 38.9629705,
    "murs": 84.151464,
    "ouverture": 9.0118405,
    "tables": 5.630055,
    "transferts": 3.354665
   },
   "avant": {
    "c": 25.789655,
    "fin_etage": 2.46179,
    "fin_g": 42.0008595,
    "murs": 83.3790415,
    "ouverture": 8.985255,
    "tables": 5.6032,
    "transferts": 2.86038
   },
   "avant_bis": {
    "c": 25.817834,
    "fin_etage": 2.461985,
    "fin_g": 41.983294,
    "murs": 83.287832,
    "ouverture": 8.9977455,
    "tables": 5.604735,
    "transferts": 2.86374
   },
   "balayage": {
    "c": 25.7693795,
    "fin_etage": 2.4595945,
    "fin_g": 41.5385955,
    "murs": 82.984634,
    "ouverture": 8.921905,
    "tables": 5.54579,
    "transferts": 2.84849
   },
   "cles": {
    "c": 26.3696995,
    "fin_etage": 2.457915,
    "fin_g": 39.054501,
    "murs": 84.0770065,
    "ouverture": 9.072675,
    "tables": 5.600345,
    "transferts": 3.37248
   },
   "transfert": {
    "c": 26.3113555,
    "fin_etage": 2.45832,
    "fin_g": 41.4940155,
    "murs": 83.337202,
    "ouverture": 8.992005,
    "tables": 5.607525,
    "transferts": 3.3089695
   }
  }
 },
 "k10": {
  "ng00": {
   "apres": 466.843964,
   "avant": 498.1930395
  },
  "ng01": {
   "apres": 350.770963,
   "avant": 368.106103
  },
  "ng02": {
   "apres": 404.384168,
   "avant": 422.9329885
  }
 },
 "profil": {
  "kitti_ng_02_001606": {
   "apres": {
    "certificat_ns": 283.3401984453621,
    "resolution_ms": 57.28455,
    "t1_ns": 382.1566441464996,
    "t1_part": 0.20667342855143062,
    "t1_temps_fil_ms": 548.11581
   },
   "avant": {
    "certificat_ns": 277.99801623518636,
    "resolution_ms": 60.670708,
    "t1_ns": 522.908531866385,
    "t1_part": 0.26632523913531364,
    "t1_temps_fil_ms": 749.99202
   }
  },
  "ng00": {
   "apres": {
    "certificat_ns": 278.2543862432184,
    "resolution_ms": 39.43997,
    "t1_ns": 350.106751791464,
    "t1_part": 0.1968520612661067,
    "t1_temps_fil_ms": 354.3644
   },
   "avant": {
    "certificat_ns": 273.6231320790634,
    "resolution_ms": 41.54907,
    "t1_ns": 462.5853199243993,
    "t1_part": 0.24694810290354416,
    "t1_temps_fil_ms": 468.21081999999996
   }
  }
 },
 "sequentiel": {
  "kitti_ng_02_001606": {
   "g_bras_sur_avant_moyenne_geometrique": {
    "apres": 0.9357140276330248,
    "balayage": 0.9950890436763633,
    "cles": 0.9322907711733135
   },
   "medianes": {
    "apres": {
     "g_ms": 60.799021,
     "mur_ms": 194.877342
    },
    "avant": {
     "g_ms": 64.759391,
     "mur_ms": 197.888749
    },
    "balayage": {
     "g_ms": 64.494631,
     "mur_ms": 196.700932
    },
    "cles": {
     "g_ms": 60.40645,
     "mur_ms": 193.341441
    }
   }
  },
  "ng00": {
   "g_bras_sur_avant_moyenne_geometrique": {
    "apres": 0.9421724643796352,
    "balayage": 0.987993770882998,
    "cles": 0.9492547453185192
   },
   "medianes": {
    "apres": {
     "g_ms": 41.98127,
     "mur_ms": 121.447392
    },
    "avant": {
     "g_ms": 44.46652,
     "mur_ms": 123.406141
    },
    "balayage": {
     "g_ms": 43.99551,
     "mur_ms": 122.119051
    },
    "cles": {
     "g_ms": 42.271261,
     "mur_ms": 121.380771
    }
   }
  }
 }
}
```
