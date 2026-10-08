# MES-G-APP : etage G sur l'appareil, trois postes (genere par pilote_g_appareil.py)

Verdict REGLE_G_APPAREIL : **rejete**

Rejets :
- ng00 : propositions, borne haute 0.7619 au-dessus du seuil 0.50
- mediane : propositions, borne haute 0.7397 au-dessus du seuil 0.50
- max : propositions, borne haute 0.7386 au-dessus du seuil 0.50

| trame | mesure | hote (ms) | appareil (ms) | rapport | IC 95 % | seuil | A/A hote | A/A appareil |
| --- | --- | ---: | ---: | ---: | --- | ---: | ---: | ---: |
| ng00 | census | 7.816 | 0.834 | 0.1070 | 0.1037-0.1104 | 0.20 | 0.998 | 1.003 |
| ng00 | sondes | 5.649 | 0.297 | 0.0526 | 0.0524-0.0527 | 0.20 | 0.980 | 0.997 |
| ng00 | propositions | 2.500 | 1.892 | 0.7576 | 0.7542-0.7619 | 0.50 | 0.993 | 0.998 |
| mediane | census | 8.741 | 0.923 | 0.1056 | 0.1051-0.1060 | 0.20 | 0.998 | 0.999 |
| mediane | sondes | 8.799 | 0.448 | 0.0509 | 0.0508-0.0510 | 0.20 | 0.988 | 0.997 |
| mediane | propositions | 3.701 | 2.731 | 0.7379 | 0.7367-0.7397 | 0.50 | 0.994 | 0.997 |
| max | census | 20.976 | 1.917 | 0.0913 | 0.0899-0.0928 | 0.20 | 0.998 | 0.993 |
| max | sondes | 16.489 | 0.763 | 0.0463 | 0.0461-0.0465 | 0.20 | 0.990 | 1.000 |
| max | propositions | 7.451 | 5.495 | 0.7377 | 0.7368-0.7386 | 0.50 | 0.997 | 0.999 |

Informations (non jugees) :

```json
{
 "max": {
  "appareil": {
   "bus_bits": 512,
   "cc": "12.0",
   "census_h2d_ms": 3.744,
   "census_h2d_octets": 95914412,
   "execution": 12090,
   "horloge_khz": 2430000,
   "horloge_memoire_khz": 12481000,
   "l2": 134217728,
   "memoire": 101973819392,
   "nom": "NVIDIA RTX PRO 6000 Blackwell Server Edition",
   "phase": "appareil",
   "pilote": 13000,
   "propositions_h2d_ms": 2.221,
   "propositions_h2d_octets": 59546424,
   "sm": 188,
   "sondes_h2d_ms": 11.975,
   "sondes_h2d_octets": 317992556
  },
  "census_hd_sur_produit": 0.5847111063961004,
  "distinctes": 462107,
  "non_resolues": 0,
  "parties": 2481101,
  "propositions_hd_sur_produit": 0.9798588476933888,
  "representants": 9998189,
  "requetes": 646621,
  "sites": 99099,
  "transferts": {
   "census_d2h_ms": 7.208,
   "census_d2h_octets": 230197076,
   "phase": "transferts",
   "propositions_d2h_ms": 5.105,
   "propositions_d2h_octets": 138941656,
   "sondes_d2h_ms": 1.726,
   "sondes_d2h_octets": 39992756
  }
 },
 "mediane": {
  "appareil": {
   "bus_bits": 512,
   "cc": "12.0",
   "census_h2d_ms": 1.742,
   "census_h2d_octets": 43630488,
   "execution": 12090,
   "horloge_khz": 2430000,
   "horloge_memoire_khz": 12481000,
   "l2": 134217728,
   "memoire": 101973819392,
   "nom": "NVIDIA RTX PRO 6000 Blackwell Server Edition",
   "phase": "appareil",
   "pilote": 13000,
   "propositions_h2d_ms": 1.091,
   "propositions_h2d_octets": 30086424,
   "sm": 188,
   "sondes_h2d_ms": 6.804,
   "sondes_h2d_octets": 179834980
  },
  "census_hd_sur_produit": 0.5814759506877938,
  "distinctes": 211461,
  "non_resolues": 0,
  "parties": 1253601,
  "propositions_hd_sur_produit": 0.9743610953163344,
  "representants": 5477969,
  "requetes": 290357,
  "sites": 64740,
  "transferts": {
   "census_d2h_ms": 3.691,
   "census_d2h_octets": 103367092,
   "phase": "transferts",
   "propositions_d2h_ms": 2.726,
   "propositions_d2h_octets": 70201656,
   "sondes_d2h_ms": 0.987,
   "sondes_d2h_octets": 21911876
  }
 },
 "ng00": {
  "appareil": {
   "bus_bits": 512,
   "cc": "12.0",
   "census_h2d_ms": 2.338,
   "census_h2d_octets": 37449428,
   "execution": 12090,
   "horloge_khz": 2430000,
   "horloge_memoire_khz": 12481000,
   "l2": 134217728,
   "memoire": 101973819392,
   "nom": "NVIDIA RTX PRO 6000 Blackwell Server Edition",
   "phase": "appareil",
   "pilote": 13000,
   "propositions_h2d_ms": 0.748,
   "propositions_h2d_octets": 20331000,
   "sm": 188,
   "sondes_h2d_ms": 4.279,
   "sondes_h2d_octets": 110527396
  },
  "census_hd_sur_produit": 0.5698841259372346,
  "distinctes": 177863,
  "non_resolues": 0,
  "parties": 847125,
  "propositions_hd_sur_produit": 0.9746515168251138,
  "representants": 3419932,
  "requetes": 252152,
  "sites": 39885,
  "transferts": {
   "census_d2h_ms": 3.019,
   "census_d2h_octets": 89766112,
   "phase": "transferts",
   "propositions_d2h_ms": 1.838,
   "propositions_d2h_octets": 47439000,
   "sondes_d2h_ms": 0.647,
   "sondes_d2h_octets": 13679728
  }
 }
}
```
