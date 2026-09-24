# Comparaison bornée s=8/10/12, quart LiDAR sans sol K5

Capture **audit-only CPU locale**, 24 septembre 2026. Même quartier
physique `x≥0,y≥0` de `08/000200`, densité globale 1/4 : 1 288 sites,
grille commune 1 mm/u18, K1..5, W8, même binaire que la
[trace S2→S3 K5](../b_s2_trace_k5_20260924/README.md). Le SHA de l'entrée
est `33630aea…a5e8f`, celui du binaire `1afdf994…aa3b1` ; leurs valeurs
intégrales et les commandes figurent dans [MANIFEST.json](MANIFEST.json).
Les trois sorties complètes sont `s8.stdout.json`, `s10.stdout.json` et
`s12.stdout.json`. Aucune trame entière, G4, autre séquence, borne
sous-quadratique ou contrat 100 ms n'est qualifié par ce quartier.

| s | rectangles q3/q4 | paires étendues q3/q4 | survivantes S2 | `ΣF` core | `chain_total` local |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 8 | 37 459 | 46 218 | 27 099 | 298 205 | 613,872 ms |
| 10 | 42 686 | 40 728 | 27 099 | 298 205 | 714,086 ms |
| 12 | 47 158 | 37 843 | 27 099 | 298 205 | 628,456 ms |

Sur ce seul quartier, `s=12` coupe **18,12 %** des paires étendues par
rapport à `s=8`, mais augmente les rectangles de **25,89 %**. Les
survivantes, les chargements et sites du core, les supports q3/q4
émis, les 12 020 paires q2 acceptées et les digests finaux du
catalogue et de la tour sont identiques aux trois s. Le digest est un
contrôle du produit, non un oracle géométrique indépendant. Les murs
ne classent **pas** les s : un rejeu non archivé de `s=8` dans cette
même séance a pris 323,642 ms contre 613,872 ms dans la capture, signe
d'une forte variation de charge de l'hôte partagé. Il faut un appariement
et une garde de charge sur **plusieurs trames entières G4**, les deux
régimes avec/sans sol, K5 puis K10, pour choisir s par coût total et
non par réduction de paires seule.

Le [lecteur](sweep.py) épingle schéma, entrée, source, binaire et sorties,
contrôle les options et chaque ledger/digest, avec et sans optimisations
Python. `--check-binary` vérifie en plus le build local encore présent :

```sh
python3 -B morsehgp3D_v9/audits/b_s8_s10_s12_k5_quarter_20260924/sweep.py verify
python3 -B -O morsehgp3D_v9/audits/b_s8_s10_s12_k5_quarter_20260924/sweep.py verify
python3 -B morsehgp3D_v9/audits/b_s8_s10_s12_k5_quarter_20260924/sweep.py verify --check-binary
```

Le mode `run` refuse d'écraser cette capture ; pour refaire l'expérience,
copier le script dans un répertoire d'audit neuf et fournir un binaire
dont le SHA correspond, sans altérer ces reçus. GCP non utilisé.
