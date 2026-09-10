# Tours retenues locales et paires 50k sur G4

10 septembre 2026. `public_status=not_claimed`. Ce paquet conserve des
diagnostics de tour FULL retenue, verticale comprise ; il ne certifie ni
la complétude générale de la génération ni une archive industrielle.

Les six processus locaux sont uniform/graine3, K1..10, un thread :
n400/8k/16k/32k à s8, puis 8k à s10 et s12. Machine partagée,
une observation par configuration, sans qualification statistique du temps.
Les paires G4 comparent CPU48 et CUDA-census+FULL CPU, séparément pour
K1..10 et K1..5, à s8 sur les mêmes 50k points. Les sorties et compteurs
appariés sont identiques. La gate device réellement exécutée juge
16 627 contrôles et 4 116 boules, avec 17 rejets. Aucun contrat 1s/100ms.
Les comparaisons G4 s10/12 n'ont pas été lancées pour rester dans la fenêtre
de clôture ; les captures de planification ne sont pas des mesures.

Les deux sessions SPOT de cette étape sont closes sur exactement
`devpod-gpu-exploration/us-central1-b/ehgp-v7-4fa0e0789a7d5bb06b787d35` :
première tentative échouée à la compilation NVCC, puis paires réussies avec
l'adaptateur strict. Les deux générations et arrêts ciblés `TERMINATED`
sont conservés. Aucune clé SSH, aucun profil OS Login, aucun inventaire VM
brut ni ELF n'est embarqué. Le gros stderr de compilation échouée est
compressé sans perte ; `storage_map.json` garde son hash original.

Le snapshot du moteur est identique entre les grands runs locaux et la
session optimisée G4. Le front WSPD optionnel a été renforcé ensuite : il
n'est pas consommé par cette sonde nominale. Ses propres preuves restent
séparées. Les 20 CTests CPU ont un rejeu stable, après une première capture
déclarée en échec pour modification concurrente du seul injecteur du test
cache. Le build CMake CUDA local compile et lie, sans exécuter de device.

Exécuter `python3 -B verify.py` puis `python3 -B -O verify.py`. Le lecteur
ne relance aucun calcul ; il vérifie fichiers, snapshots, captures closes,
autorités, configurations, compteurs, paires et arrêts. Les chemins absolus
des commandes sont ceux de l'environnement d'origine. Les sources projet
consommées sont conservées ; les dépendances système/vendor ne sont pas
redistribuées. Les contrôles nommés 1/2/2 et ancre K10 restent dans leur
paquet **NOT_EXECUTED_50K**, sans promotion par les seuls digests présents ici.
