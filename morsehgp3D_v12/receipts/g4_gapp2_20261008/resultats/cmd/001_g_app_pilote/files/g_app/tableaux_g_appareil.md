# MES-G-APP, etape 2 : propositions sur l'appareil (genere par pilote_g_appareil.py)

REGLE_G_APPAREIL_2 : D2 **rejete**, D1 **rejete** ; decision : aucune tranche sur cette base

Rejets D2 :
- ng00 : D2 total, borne haute 0.2471 au-dessus de 0.10
- ng00 : D2 propositions, borne haute 1.0958 au-dessus de 0.20
- mediane : D2 total, borne haute 0.2398 au-dessus de 0.10
- mediane : D2 propositions, borne haute 0.9934 au-dessus de 0.20
- max : D2 total, borne haute 0.2182 au-dessus de 0.10
- max : D2 propositions, borne haute 0.9600 au-dessus de 0.20

Rejets D1 :
- ng00 : D1 total, borne haute 0.1569 au-dessus de 0.15
- ng00 : D1 propositions, borne haute 0.5254 au-dessus de 0.50
- mediane : D1 total, borne haute 0.1552 au-dessus de 0.15
- mediane : D1 propositions, borne haute 0.5107 au-dessus de 0.50
- max : D1 propositions, borne haute 0.5115 au-dessus de 0.50

| trame | mesure | hote (ms) | appareil (ms) | A/A hote | A/A appareil |
| --- | --- | ---: | ---: | ---: | ---: |
| ng00 | census | 7.599 | 0.849 | 0.991 | 0.995 |
| ng00 | sondes | 5.659 | 0.300 | 0.985 | 0.998 |
| ng00 | propositions | 2.512 | 1.893 | 0.989 | 1.000 |
| ng00 | propositions_l4 | 2.316 | 1.307 | 0.995 | 1.002 |
| ng00 | propositions_l4f32 | 4.487 | 2.735 | 0.999 | 0.995 |
| mediane | census | 8.735 | 0.933 | 0.992 | 1.001 |
| mediane | sondes | 8.937 | 0.450 | 0.991 | 1.002 |
| mediane | propositions | 3.722 | 2.729 | 0.996 | 1.000 |
| mediane | propositions_l4 | 3.439 | 1.897 | 0.996 | 0.998 |
| mediane | propositions_l4f32 | 5.980 | 3.684 | 0.998 | 1.000 |
| max | census | 21.190 | 1.929 | 0.997 | 0.997 |
| max | sondes | 16.563 | 0.768 | 0.995 | 0.997 |
| max | propositions | 7.441 | 5.505 | 0.998 | 1.000 |
| max | propositions_l4 | 6.958 | 3.785 | 0.996 | 1.000 |
| max | propositions_l4f32 | 11.668 | 7.116 | 0.998 | 1.004 |

| trame | conception | rapport | valeur | IC 95 % | seuil |
| --- | --- | --- | ---: | --- | ---: |
| ng00 | D2 | total | 0.2452 | 0.2435-0.2471 | 0.10 |
| ng00 | D2 | propositions | 1.0887 | 1.0799-1.0958 | 0.20 |
| ng00 | D1 | total | 0.1551 | 0.1534-0.1569 | 0.15 |
| ng00 | D1 | propositions | 0.5222 | 0.5191-0.5254 | 0.50 |
| ng00 | D1 | neutralite_hote | 0.9211 | 0.9184-0.9239 | 1.05 |
| mediane | D2 | total | 0.2380 | 0.2364-0.2398 | 0.10 |
| mediane | D2 | propositions | 0.9909 | 0.9887-0.9934 | 0.20 |
| mediane | D1 | total | 0.1540 | 0.1527-0.1552 | 0.15 |
| mediane | D1 | propositions | 0.5098 | 0.5089-0.5107 | 0.50 |
| mediane | D1 | neutralite_hote | 0.9239 | 0.9218-0.9260 | 1.05 |
| max | D2 | total | 0.2171 | 0.2157-0.2182 | 0.10 |
| max | D2 | propositions | 0.9579 | 0.9558-0.9600 | 0.20 |
| max | D1 | total | 0.1434 | 0.1422-0.1442 | 0.15 |
| max | D1 | propositions | 0.5097 | 0.5082-0.5115 | 0.50 |
| max | D1 | neutralite_hote | 0.9355 | 0.9346-0.9369 | 1.05 |

Informations (non jugees) :

```json
{
 "max": {
  "informations": {
   "appareil": {
    "bus_bits": 512,
    "cc": "12.0",
    "census_h2d_ms": 3.675,
    "census_h2d_octets": 95914412,
    "execution": 12090,
    "horloge_khz": 2430000,
    "horloge_memoire_khz": 12481000,
    "l2": 134217728,
    "memoire": 101973819392,
    "nom": "NVIDIA RTX PRO 6000 Blackwell Server Edition",
    "phase": "appareil",
    "pilote": 13000,
    "propositions_h2d_ms": 2.21,
    "propositions_h2d_octets": 59546424,
    "sm": 188,
    "sondes_h2d_ms": 11.841,
    "sondes_h2d_octets": 317992556
   },
   "census_hd_sur_produit": 0.5785404217781498,
   "distinctes": 462107,
   "entieres": 1316447,
   "issues": {
    "l4": {
     "census": 498012,
     "refus": 0,
     "table": 1983089
    },
    "l4f32": {
     "census": 498012,
     "refus": 0,
     "table": 1983089
    },
    "p64": {
     "census": 498012,
     "refus": 0,
     "table": 1983089
    }
   },
   "mecanismes": {
    "l4": {
     "certificat": 498015,
     "repli_certificat": 0,
     "repli_sans_proposition": 0,
     "t1": 1983086
    },
    "l4f32": {
     "certificat": 498012,
     "repli_certificat": 0,
     "repli_sans_proposition": 0,
     "t1": 1983089
    },
    "p64": {
     "certificat": 498015,
     "repli_certificat": 0,
     "repli_sans_proposition": 0,
     "t1": 1983086
    }
   },
   "non_resolues": 0,
   "parties": 2481101,
   "replis": {
    "D1": 0.0,
    "D2": 0.0,
    "p64": 0.0
   },
   "representants": 9998189,
   "requetes": 646621,
   "sites": 99099,
   "transferts": {
    "census_d2h_ms": 6.923,
    "census_d2h_octets": 230197076,
    "phase": "transferts",
    "propositions_d2h_ms": 14.802,
    "propositions_d2h_octets": 416824968,
    "sondes_d2h_ms": 1.714,
    "sondes_d2h_octets": 39992756
   }
  },
  "rapports": {
   "l4f32_hote": 1.5690088127948123,
   "p64_appareil": 0.7401637161620513,
   "total_p64": 0.1813243432964005
  }
 },
 "mediane": {
  "informations": {
   "appareil": {
    "bus_bits": 512,
    "cc": "12.0",
    "census_h2d_ms": 1.857,
    "census_h2d_octets": 43630488,
    "execution": 12090,
    "horloge_khz": 2430000,
    "horloge_memoire_khz": 12481000,
    "l2": 134217728,
    "memoire": 101973819392,
    "nom": "NVIDIA RTX PRO 6000 Blackwell Server Edition",
    "phase": "appareil",
    "pilote": 13000,
    "propositions_h2d_ms": 1.094,
    "propositions_h2d_octets": 30086424,
    "sm": 188,
    "sondes_h2d_ms": 6.789,
    "sondes_h2d_octets": 179834980
   },
   "census_hd_sur_produit": 0.585043601900935,
   "distinctes": 211461,
   "entieres": 683502,
   "issues": {
    "l4": {
     "census": 224773,
     "refus": 0,
     "table": 1028828
    },
    "l4f32": {
     "census": 224773,
     "refus": 0,
     "table": 1028828
    },
    "p64": {
     "census": 224773,
     "refus": 0,
     "table": 1028828
    }
   },
   "mecanismes": {
    "l4": {
     "certificat": 224773,
     "repli_certificat": 0,
     "repli_sans_proposition": 0,
     "t1": 1028828
    },
    "l4f32": {
     "certificat": 224773,
     "repli_certificat": 0,
     "repli_sans_proposition": 0,
     "t1": 1028828
    },
    "p64": {
     "certificat": 224773,
     "repli_certificat": 0,
     "repli_sans_proposition": 0,
     "t1": 1028828
    }
   },
   "non_resolues": 0,
   "parties": 1253601,
   "replis": {
    "D1": 0.0,
    "D2": 0.0,
    "p64": 0.0
   },
   "representants": 5477969,
   "requetes": 290357,
   "sites": 64740,
   "transferts": {
    "census_d2h_ms": 3.54,
    "census_d2h_octets": 103367092,
    "phase": "transferts",
    "propositions_d2h_ms": 7.172,
    "propositions_d2h_octets": 210604968,
    "sondes_d2h_ms": 0.934,
    "sondes_d2h_octets": 21911876
   }
  },
  "rapports": {
   "l4f32_hote": 1.6064801224250405,
   "p64_appareil": 0.7336512076783486,
   "total_p64": 0.19306831224397134
  }
 },
 "ng00": {
  "informations": {
   "appareil": {
    "bus_bits": 512,
    "cc": "12.0",
    "census_h2d_ms": 2.379,
    "census_h2d_octets": 37449428,
    "execution": 12090,
    "horloge_khz": 2430000,
    "horloge_memoire_khz": 12481000,
    "l2": 134217728,
    "memoire": 101973819392,
    "nom": "NVIDIA RTX PRO 6000 Blackwell Server Edition",
    "phase": "appareil",
    "pilote": 13000,
    "propositions_h2d_ms": 0.746,
    "propositions_h2d_octets": 20331000,
    "sm": 188,
    "sondes_h2d_ms": 4.28,
    "sondes_h2d_octets": 110527396
   },
   "census_hd_sur_produit": 0.5769015833396581,
   "distinctes": 177863,
   "entieres": 466813,
   "issues": {
    "l4": {
     "census": 186573,
     "refus": 0,
     "table": 660552
    },
    "l4f32": {
     "census": 186573,
     "refus": 0,
     "table": 660552
    },
    "p64": {
     "census": 186573,
     "refus": 0,
     "table": 660552
    }
   },
   "mecanismes": {
    "l4": {
     "certificat": 186573,
     "repli_certificat": 0,
     "repli_sans_proposition": 0,
     "t1": 660552
    },
    "l4f32": {
     "certificat": 186573,
     "repli_certificat": 0,
     "repli_sans_proposition": 0,
     "t1": 660552
    },
    "p64": {
     "certificat": 186573,
     "repli_certificat": 0,
     "repli_sans_proposition": 0,
     "t1": 660552
    }
   },
   "non_resolues": 0,
   "parties": 847125,
   "replis": {
    "D1": 0.0,
    "D2": 0.0,
    "p64": 0.0
   },
   "representants": 3419932,
   "requetes": 252152,
   "sites": 39885,
   "transferts": {
    "census_d2h_ms": 2.969,
    "census_d2h_octets": 89766112,
    "phase": "transferts",
    "propositions_d2h_ms": 5.367,
    "propositions_d2h_octets": 142317000,
    "sondes_d2h_ms": 0.646,
    "sondes_d2h_octets": 13679728
   }
  },
  "rapports": {
   "l4f32_hote": 1.7848732118830608,
   "p64_appareil": 0.7531923548557494,
   "total_p64": 0.19184023325072216
  }
 }
}
```

Information K10 (ng00, un processus, sans verdict) :

```json
{
 "code": 0,
 "entieres": 1390772,
 "identite": {
  "D1": true,
  "D2": true,
  "commune": true
 },
 "mecanismes": {
  "l4": {
   "certificat": 995205,
   "repli_certificat": 0,
   "repli_sans_proposition": 0,
   "t1": 4375850
  },
  "l4f32": {
   "certificat": 995205,
   "repli_certificat": 5,
   "repli_sans_proposition": 0,
   "t1": 4375845
  },
  "p64": {
   "certificat": 995206,
   "repli_certificat": 0,
   "repli_sans_proposition": 0,
   "t1": 4375849
  }
 },
 "mesures": {
  "census": {
   "aa_app": 1.0026508562015333,
   "aa_hote": 0.9972781178599973,
   "t_app_ms": 4.6027000000000005,
   "t_hote_ms": 71.78909999999999
  },
  "propositions": {
   "aa_app": 1.0023711460100708,
   "aa_hote": 0.9995337752556068,
   "t_app_ms": 26.808,
   "t_hote_ms": 31.149949999999997
  },
  "propositions_l4": {
   "aa_app": 1.0017554051263549,
   "aa_hote": 0.9992797065784249,
   "t_app_ms": 22.86615,
   "t_hote_ms": 34.20405
  },
  "propositions_l4f32": {
   "aa_app": 0.9916712685893332,
   "aa_hote": 0.9998882759457979,
   "t_app_ms": 47.170500000000004,
   "t_hote_ms": 65.33070000000001
  },
  "sondes": {
   "aa_app": 1.0004107240752336,
   "aa_hote": 1.000782594167783,
   "t_app_ms": 2.24955,
   "t_hote_ms": 41.58395
  }
 },
 "parties": 5371055,
 "rapports": {
  "D1": {
   "neutralite_hote": 1.0980451011959893,
   "propositions": 0.7340669888715713,
   "total": 0.205630937636224
  },
  "D2": {
   "propositions": 1.5143041963149222,
   "total": 0.37380036395591015
  },
  "information": {
   "l4f32_hote": 2.0972971064159016,
   "p64_appareil": 0.8606113332445158,
   "total_p64": 0.2329058350573957
  }
 },
 "replis": {
  "D1": 0.0,
  "D2": 9.309158070434952e-07,
  "p64": 0.0
 },
 "requetes": 1765151
}
```
