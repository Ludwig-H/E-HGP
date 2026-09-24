# Contre-audit R21 avant toute nouvelle session G4

24 septembre 2026. Lecture **sans mutation du moteur** du worktree de
développement `build/v9-open-worktree` à `61cfba666` et des quatre fichiers
`gcp-migration/tower_*_v9.py` qu'il contient. Le `main` public observé était
`092ad4ae4` : cette note ne transforme pas un worktree non publié en reçu G4.

## Porte nominale rouge, reproduite

`python3 -B gcp-migration/tower_selftest_v9.py
Protocol.test_nominal_session_completed` finit en **ERREUR (1 test, 20,429 s)** :
`ValueError: GPU labels from complete LiDAR towers`, ligne 1277 du selftest.
Le plan v26 a 30 cas : douze cas initiaux sans sol, six répétitions à 00,
puis douze cas bruts avec sol. Le selftest de snapshot reconnaît lui-même
ces 30 cas (`tower_selftest_v9.py:824-831`). Mais la réception nominale
attend encore les douze indices GPU historiques
`[0,2,4,6,8,10,12,13,14,15,16,17]` et `range(18)` ; les douze cas bruts
ajoutent six indices GPU `18,20,22,24,26,28` et six jumeaux moteur.
Plusieurs tests de budget/protocole emploient également 18 cas ou quinze
cas restants (`:1391`, `:1496`) et doivent être adaptés à la liste réelle,
sans perdre leur sens de refus. Une simple correction de la première
assertion ne suffit donc pas. Les comparaisons appariées doivent inclure
les six nouveaux couples bruts et les labels `gpu_cold` doivent rester
distincts de la session préouverte.

## Trou de comptabilité du temps mur

`tower_probe.cpp:230-238` lit l'entrée, puis appelle
`open_device_session()`, puis seulement `run_tower_chain()` ; le commentaire
à sa fin indique explicitement que cette session n'entre pas dans
`chain_total`. `filter_runner.cu:1443-1452` mesure `context_ms` puis
`reserve_ms` en intervalles successifs. Or
`tower_worker_v9.py:1098-1106`, repris par le lecteur de réception,
contrôle seulement `read + chain_total + digest + catalogue_digest`
contre `/usr/bin/time -v`, avec 50 ms de tolérance. Reproduction sans
GCP, sur le même module WIP : une valeur portant `context_ms=10 000`,
`reserve_ms=10 000`, quatre autres phases de 1 ms chacune et un mur externe
de **10 ms** est acceptée par `validate_external_wall`. Ce n'est pas une
accusation de chrono réel falsifié ; c'est un mutant causal que la porte
actuelle laisse passer en Python normal **et** `-O`. Reproducteur minimal :

```python
v = {"times_ms": {"read": 1.0, "chain_total": 1.0,
                  "digest": 1.0, "catalogue_digest": 1.0},
     "device_session": {"opened": True,
                        "context_ms": 10000.0, "reserve_ms": 10000.0}}
worker.validate_external_wall(v, 0.01)  # accepte à tort
```

Ajouter les deux phases de session au minorant
externe, en conservant les règles d'arrondi et la tolérance explicite ;
tuer séparément les deux mutations sous Python normal et `-O`.

Le contrat de 100 ms porte sur la chaîne FULL explicite et non sur
l'ouverture CUDA à froid si une session résidente est déclarée, mais le
**mur du processus** doit toujours couvrir cette ouverture ; les deux
mesures doivent être publiées distinctement. Ne jamais soustraire une
phase non effectivement recouverte d'une autre.

## Épingles brutes déjà disponibles, pas encore branchées

`tower_worker_v9.py:203-208` ne contient que les six épingles sans sol.
Le reçu indépendant [C des trois trames brutes](c_raw_pins_20260924/README.md)
et son [lecteur avec mutants](b_raw_pin_reader_20260924/README.md) livrent
six couples K5/K10/s8 moteur↔lots CPU et leurs digests de tour/catalogue,
sur les trois trames 08 entières à 1 mm. Le selftest R21 doit confronter
les nouveaux cas `b00/b01/b02` à ces épingles **avant** d'utiliser leur
jumeau GPU comme seule autorité. Ces références restent `complete_relative`
et ne prouvent pas une complétude mathématique absolue ; la
[lacune d'oracle q4 brut](b_full_raw_completeness_gap_20260924/README.md)
est séparée. Elles ne qualifient ni float32 brut, ni plusieurs séquences,
ni G4.

## Décision de reprise

Garder R21 hors GCP jusqu'à un commit atomique sonde+protocole, un snapshot
commité inchangé, la suite Python normale et `-O` entièrement verte, les
mutants de temps et de labels tués, et une comparaison indépendante des
entrées et digests épinglés. Une session exploratoire ponctuelle ne doit
pas être présentée comme qualification de contrat. Le présent audit n'a
utilisé ni GCP ni nouveau calcul de tour.
