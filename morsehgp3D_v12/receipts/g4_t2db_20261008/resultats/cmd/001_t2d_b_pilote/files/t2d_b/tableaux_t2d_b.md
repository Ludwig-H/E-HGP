# Chantier T2-d-B : etage G par levier (genere par pilote_t2d_b.py)

Verdicts : lot_t2d_b **adopte**, garde_seule **rejete**, report_seul **adopte**, temoins_seuls **adopte**, census_combine **adopte**, proposition **rejete**

| trame | G avant (ms) | G avant_bis (ms) | G garde (ms) | G report (ms) | G temoins (ms) | G census (ms) | G proposition (ms) | G apres (ms) | empreinte | travail identique |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| ng00 | 53.09 | 53.11 | 52.81 | 52.87 | 49.62 | 49.04 | 53.12 | 48.58 | e5a81154fb1b15f1 | True |
| ng01 | 42.02 | 42.04 | 41.89 | 41.87 | 39.74 | 39.20 | 41.89 | 38.89 | 10bb4c6d14096b2d | True |
| ng02 | 48.22 | 48.20 | 48.05 | 48.00 | 45.84 | 45.32 | 48.03 | 45.01 | d66647a043f682da | True |

| trame | lot_t2d_b (IC 95 %) | garde_seule (IC 95 %) | report_seul (IC 95 %) | temoins_seuls (IC 95 %) | census_combine (IC 95 %) | proposition (IC 95 %) | temoins_apres_garde (IC 95 %) | proposition_apres_census (IC 95 %) | A/A (IC 95 %) |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| ng00 | 0.916 (0.914-0.917) | 0.994 (0.991-0.998) | 0.995 (0.993-0.997) | 0.935 (0.931-0.939) | 0.924 (0.922-0.926) | 0.999 (0.995-1.003) | 0.929 (0.927-0.931) | 0.991 (0.989-0.993) | 1.001 (0.998-1.005) |
| ng01 | 0.926 (0.923-0.929) | 0.998 (0.995-1.000) | 0.996 (0.993-0.998) | 0.945 (0.941-0.948) | 0.933 (0.931-0.936) | 0.998 (0.995-1.000) | 0.936 (0.933-0.938) | 0.992 (0.989-0.995) | 1.000 (0.998-1.003) |
| ng02 | 0.932 (0.931-0.934) | 0.996 (0.993-0.998) | 0.994 (0.992-0.997) | 0.951 (0.949-0.954) | 0.939 (0.936-0.941) | 0.996 (0.994-0.999) | 0.943 (0.940-0.946) | 0.993 (0.991-0.996) | 0.999 (0.996-1.001) |

Informations (jamais jugees) :

```json
{
 "full_k5_appareil": {
  "ng00": {
   "ful1_identique_entre_bras": true,
   "full_apres": {
    "ful1": [
     "3a2bfb4f9f48b4b0cc5b0318d9fcf4887906638e034b2c97dda1918e3170a6fe"
    ],
    "full_ms": 157.180732,
    "g_ms": 53.566814,
    "prises": 2,
    "valides": 2
   },
   "full_avant": {
    "ful1": [
     "3a2bfb4f9f48b4b0cc5b0318d9fcf4887906638e034b2c97dda1918e3170a6fe"
    ],
    "full_ms": 161.898877,
    "g_ms": 58.568384,
    "prises": 2,
    "valides": 2
   }
  },
  "ng01": {
   "ful1_identique_entre_bras": true,
   "full_apres": {
    "ful1": [
     "2d58a72a623ab293a05f30c215cd65fcb455419a0a118f6d301bb17600645e26"
    ],
    "full_ms": 126.940253,
    "g_ms": 40.866044,
    "prises": 2,
    "valides": 2
   },
   "full_avant": {
    "ful1": [
     "2d58a72a623ab293a05f30c215cd65fcb455419a0a118f6d301bb17600645e26"
    ],
    "full_ms": 129.755167,
    "g_ms": 44.1300395,
    "prises": 2,
    "valides": 2
   }
  },
  "ng02": {
   "ful1_identique_entre_bras": true,
   "full_apres": {
    "ful1": [
     "27d7650add157cdd582d55f50305f0b8f9319865c8ffd0737cf12b76ac2197f0"
    ],
    "full_ms": 164.278955,
    "g_ms": 49.355973,
    "prises": 2,
    "valides": 2
   },
   "full_avant": {
    "ful1": [
     "27d7650add157cdd582d55f50305f0b8f9319865c8ffd0737cf12b76ac2197f0"
    ],
    "full_ms": 165.7563955,
    "g_ms": 52.9748235,
    "prises": 2,
    "valides": 2
   }
  }
 },
 "k10_w48": {
  "ng00": {
   "apres": {
    "empreintes": [
     "918dbb9383edb29b"
    ],
    "g_ms": 413.86661,
    "prises": 2,
    "valides": 2
   },
   "avant": {
    "empreintes": [
     "918dbb9383edb29b"
    ],
    "g_ms": 441.35488175,
    "prises": 2,
    "valides": 2
   }
  },
  "ng01": {
   "apres": {
    "empreintes": [
     "349c286d4a1a6c05"
    ],
    "g_ms": 304.5789665,
    "prises": 2,
    "valides": 2
   },
   "avant": {
    "empreintes": [
     "349c286d4a1a6c05"
    ],
    "g_ms": 321.73127875,
    "prises": 2,
    "valides": 2
   }
  },
  "ng02": {
   "apres": {
    "empreintes": [
     "d8bb9f962f2b3dc1"
    ],
    "g_ms": 340.1056055,
    "prises": 2,
    "valides": 2
   },
   "avant": {
    "empreintes": [
     "d8bb9f962f2b3dc1"
    ],
    "g_ms": 354.4256205,
    "prises": 2,
    "valides": 2
   }
  }
 },
 "profil_k5_w1": {
  "ng00": {
   "profil_apres": {
    "empreintes": [
     "e5a81154fb1b15f1"
    ],
    "g_ms": 1508.590882,
    "prises": 1,
    "profil_ms_cumules": {
     "arret": 295.5,
     "census_complet": 332.3,
     "census_sature": 639.9,
     "certificat": 183.9,
     "pas": 16.0,
     "proposition": 438.5,
     "repli": 0.0,
     "sonde": 594.9,
     "t1": 882.6,
     "total": 4202.9,
     "trace": 818.7
    },
    "valides": 1
   },
   "profil_avant": {
    "empreintes": [
     "e5a81154fb1b15f1"
    ],
    "g_ms": 1651.7344545,
    "prises": 1,
    "profil_ms_cumules": {
     "arret": 300.1,
     "census_complet": 432.1,
     "census_sature": 916.8,
     "certificat": 181.3,
     "pas": 16.0,
     "proposition": 494.1,
     "repli": 0.0,
     "sonde": 611.8,
     "t1": 885.8,
     "total": 4634.4,
     "trace": 795.7
    },
    "valides": 1
   }
  }
 },
 "uniformes_k5_w48": {
  "u16000": {
   "apres": {
    "empreinte_gravee": true,
    "empreintes": [
     "cf7c7745fcb4ae6e"
    ],
    "g_ms": 48.377859,
    "prises": 1,
    "valides": 1
   },
   "avant": {
    "empreinte_gravee": true,
    "empreintes": [
     "cf7c7745fcb4ae6e"
    ],
    "g_ms": 49.720379,
    "prises": 1,
    "valides": 1
   }
  },
  "u32000": {
   "apres": {
    "empreinte_gravee": true,
    "empreintes": [
     "d1f08fd0dbdf48eb"
    ],
    "g_ms": 105.716228,
    "prises": 1,
    "valides": 1
   },
   "avant": {
    "empreinte_gravee": true,
    "empreintes": [
     "d1f08fd0dbdf48eb"
    ],
    "g_ms": 110.481073,
    "prises": 1,
    "valides": 1
   }
  },
  "u8000": {
   "apres": {
    "empreinte_gravee": true,
    "empreintes": [
     "a40f1b2ef8547269"
    ],
    "g_ms": 23.5198795,
    "prises": 1,
    "valides": 1
   },
   "avant": {
    "empreinte_gravee": true,
    "empreintes": [
     "a40f1b2ef8547269"
    ],
    "g_ms": 24.5760995,
    "prises": 1,
    "valides": 1
   }
  }
 }
}
```

Mur FULL sur l'appareil (information, mediane des passes 2..10 par processus) :

| trame | bras | FULL (ms) | G dans FULL (ms) | FUL1 |
| --- | --- | --- | --- | --- |
| ng00 | full_avant | 161.57 | 58.69 | 3a2bfb4f9f48b4b0 |
| ng00 | full_apres | 157.91 | 53.89 | 3a2bfb4f9f48b4b0 |
| ng00 | full_apres | 156.45 | 53.24 | 3a2bfb4f9f48b4b0 |
| ng00 | full_avant | 162.23 | 58.45 | 3a2bfb4f9f48b4b0 |
| ng01 | full_avant | 129.80 | 43.97 | 2d58a72a623ab293 |
| ng01 | full_apres | 127.59 | 41.15 | 2d58a72a623ab293 |
| ng01 | full_apres | 126.29 | 40.58 | 2d58a72a623ab293 |
| ng01 | full_avant | 129.71 | 44.29 | 2d58a72a623ab293 |
| ng02 | full_avant | 166.04 | 53.15 | 27d7650add157cdd |
| ng02 | full_apres | 163.83 | 49.36 | 27d7650add157cdd |
| ng02 | full_apres | 164.73 | 49.35 | 27d7650add157cdd |
| ng02 | full_avant | 165.48 | 52.80 | 27d7650add157cdd |
