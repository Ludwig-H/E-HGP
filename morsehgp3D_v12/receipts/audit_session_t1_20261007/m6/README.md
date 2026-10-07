# Relecture du reçu G4 M6 et de son pilote

7 octobre 2026, pin `4147c5460`. Les valeurs viennent de la session du développeur `v12.20261007.t0a` ;
**aucun lancement GPU/GCP n'a été effectué par cet audit**. `check.py` refuse une source ou un reçu différent du
pin, vérifie avant/après et produit la même sortie en normal et sous `-O`.

Les neuf processus annoncés sont présents : trois modes × trois processus, **65 lignes uniques par prise**,
585 lignes au total, codes 0, quantiles finis ordonnés et premiers usages distincts. Les prises publiées ne
présentent donc pas la vacuité reproduite séparément ci-dessous. Les corrections CST-0209/0210 restent closes.

Médianes entre les trois processus (µs ; les tableaux complets sont dans `result.json`) :

| Mesure | spin | yield | blocking |
| --- | ---: | ---: | ---: |
| ouverture du contexte | 116 767,598 | 116 267,618 | 116 018,569 |
| premier lancement | 85,690 | 86,720 | 124,760 |
| lancement vide synchronisé | 8,050 | 8,120 | 12,995 |
| graphe de dix noyaux | 11,450 | 11,530 | 24,360 |
| lecture/écriture logique de 256 Mio | 368,455 | 368,500 | 403,075 |

Le README de la session publie explicitement **un** processus par mode. Son premier `spin` ouvre le contexte en
357 944,810 µs et lance en 578,730 µs ; les deux autres lancent en 85,690 et 83,820 µs. Les trois `yield` sont
86,830, 85,450, 86,720 µs. Le seul premier processus ne démontre donc pas un avantage du mode `yield` au premier
lancement. Les modes sont joués dans l'ordre fixe spin/yield/blocking ; aucune attribution causale de cet écart
initial n'est acquise. Choisir `yield` reste un choix de Session compatible avec ces temps chauds proches ;
annoncer un bénéfice de premier lancement exigerait une comparaison adaptée.

Le volume de transfert appelé « trame 60 000 sites » vaut exactement 720 Kio = 737 280 octets dans le code ;
60 000 XYZ u32 représentent 720 000 octets. C'est une sonde voisine de la taille d'une trame, pas le transfert exact
de celle-ci. Le débit `touch` compte les octets logiques lus/écrits, sans mesure des transactions DRAM.

**CST-0018, complément de pilote.** `run_m6.py` ne lit pas les lignes qu'il publie. En conservant son vrai `main`
et en remplaçant uniquement la frontière externe `capture` par des processus code 0 sans sortie, il écrit neuf
fichiers vides et annonce `mes_m6_ok`, code 0. Le témoin ne lance ni compilateur ni GPU. Exiger les 65 clés de
mesure attendues, les modes, effectifs et nombres finis avant de rendre ce succès. Le code n'émet aucun verdict
d'adoption M6 : il s'agit d'un succès de banc sans preuve, pas d'une fausse adoption historique.

Le pilote ne conserve pas le hash du binaire M6 dans son propre rapport ; le rattachement au paquet de session
doit rester distinct de ce contrôle statistique. Le présent audit ne transforme pas les quantiles en temps FULL.

```sh
python3 -B morsehgp3D_v12/receipts/audit_session_t1_20261007/m6/check.py
python3 -B -O morsehgp3D_v12/receipts/audit_session_t1_20261007/m6/check.py
```
