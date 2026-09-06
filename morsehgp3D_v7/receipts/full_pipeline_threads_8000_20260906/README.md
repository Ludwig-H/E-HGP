# FULL : pipeline 1/2/4/8 threads, n8000 — 6 septembre 2026

Quatre mesures directes closes, même ELF, n=8000/s=8/Kmax=10/lazy C=1 000 000,
P=unlimited. Les dix ordres, leurs digests, le digest final et tous les champs
d'ordre hors mesures sont identiques. La configuration ne diffère que par N.
Hors mesures et N, seuls les six nombres de workers diffèrent au terminal :
aucune différence des compteurs de travail, volumes ou pics logiques observés.

| Threads | CPU demandés | Total terminal (s) | Génération (s) | FULL (s) | GNU pic RSS (KiB) |
| --- | --- | ---: | ---: | ---: | ---: |
| 1 | 6 | 132,948 | 59,562 | 51,094 | 1 321 488 |
| 2 | 4,6 | 98,176 | 30,093 | 53,935 | 1 432 136 |
| 4 | 0,2,4,6 | 74,558 | 15,483 | 50,139 | 1 486 940 |
| 8 | 0-7 | 69,824 | 11,538 | 50,765 | 1 495 380 |

Les ratios du temps mural du runner sont 1 / 1,354 / 1,783 / 1,903.
WSPD passe de 29,821 à 6,012 s ; rectangles de 29,741 à 5,525 s ;
préfiltre de 9,930 à 2,129 s ; census de 6,921 à 1,482 s.
FULL et la boucle K restent séquentiels : à 8 threads, FULL prend encore
50,765 des 69,824 s. Le détail par phase et les différences classées sont
dans analysis_r1/normal.stdout (JSON complet).

Une seule observation par bras : aucun gain statistiquement robuste, aucune
complétude géométrique, ni aucun contrat 50k certifié ici. public_status=not_claimed.
GNU peak RSS est distinct des échantillons RSS/HWM ; les pics logiques ne sont
pas une mesure de la mémoire totale. Le protocole direct n'a pas de watchdog.

Les 12 fichiers bruts sont copiés octet pour octet. Le reader vérifie leurs
hashes, commandes, statuts, dix ordres et liaison du digest final. Les deux
lectures Python normal/-O sont closes, code 0, stdout identiques. La clôture
des runs est rapportée par wait du runner, GNU exit0 et terminal completed ;
aucun certificat de groupe de processus n'est inventé dans ces fichiers.
ROOT a en outre observé l'absence de full_probe actif après les quatre runs.

SOURCE_REFERENCE.json pointe vers les 42 dépendances déjà publiées une seule
fois avec les micros. Pas de nouvelle copie des headers ni de l'ELF. Le snapshot
reste explicitement post-compilation. Aucun moteur ni compilation n'est lancé
par le reader ou par cette publication. GCP non utilisé.

Lecture portable depuis ce dossier :

```bash
sha256sum -c SHA256SUMS
python3 -B read_results.py --directory . --run-pin 1=5e3ccdb2880bd449ccc9bd99615300fc9caf609d2a0592dea41d39896cf308eb --run-pin 2=ca12d24b1e1f98e4074193158fb6de716dbc06a2dcdbb9d1dbda158578f8ef32 --run-pin 4=7cf81cb19342059fb320d9514b2004e3cca4e71c19bde47140a986737bb4d4a0 --run-pin 8=2364a2817193b5a11c3ab5f82eab0741f6a6e486a65b03210d1349b0743040d8
```

Ajouter `-O` à Python reproduit la seconde lecture ; aucun moteur n'est appelé.
