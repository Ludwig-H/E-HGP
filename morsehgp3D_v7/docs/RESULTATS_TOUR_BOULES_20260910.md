# Mesures de la nouvelle tour retenue

10 septembre 2026. `phase=exploration_v7_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

**La tour K1..10 termine à 8k, 16k et 32k. Aucun contrat 50k/GPU n'est
acquis.** Les [captures fermées](../receipts/full_ball_runs_20260910/README.md)
conservent sources, commandes, stdout, stderr, ressources et contrôle portable.
Le [raccord](TOUR_FULL_PAR_BOULES.md) garde cette fois tous les ordres,
contributions datées et cartes verticales simultanément jusqu'à la sortie.

## Protocole et résultats CPU

Uniforme u16, seed 3, s WSPD=8, Kmax10, un thread. Compilation fraîche
O3/NDEBUG C++20, `-Wall -Wextra -Wpedantic -Werror -pthread`.
Un processus par taille ; le micro n400 préalable termine en 4,624 s.
Les sources restent identiques avant/après. Aucun quota de travail ou
de temps par commande n'est ajouté à ces mesures.

| n | Total sonde | Processus GNUtime | Construction FULL | Nœuds retenus | Pic RSS |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 8 000 | 215,169 s | 215,45 s | 119,636 s | 3 976 472 | 2 329 780 KiB |
| 16 000 | 417,627 s | 418,25 s | 242,284 s | 8 310 399 | 5 226 508 KiB |
| 32 000 | 965,053 s | 966,27 s | 565,502 s | 17 166 975 | 10 559 316 KiB |

Le total sonde inclut génération de l'entrée, index, génération des candidats, tri/RLE,
prefilter, census, construction de toute la tour et empreinte du payload.
Le temps du processus inclut aussi sa fermeture et la destruction des
objets. Ni archive industrielle ni lectures applicatives ne sont mesurées.
La construction est relative au census fourni ; son succès n'est pas
un nouveau certificat de complétude de la génération sur ces grands n.

| n | Génération candidats | Tri/RLE | Prefilter | Census | Empreinte |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 8 000 | 59,200 s | 2,092 s | 20,260 s | 12,080 s | 1,842 s |
| 16 000 | 128,153 s | 4,374 s | 21,936 s | 17,075 s | 3,708 s |
| 32 000 | 290,048 s | 10,642 s | 50,950 s | 38,866 s | 8,852 s |

La génération comprend la WSPD, le traitement des rectangles et des voies
q2/q3/q4, ainsi que l'émission des candidats ; ce n'est pas le seul coût
de construction de la WSPD.

Les rapports successifs sont 1,94 puis 2,31 pour le total, 2,09 puis 2,07
pour les nœuds. Ils n'exposent pas de croissance quadratique sur ce
triplet, mais **ne prouvent pas un exposant universel**. Un seul régime,
une graine et un passage par taille ; travaux d'audit et compilations ont
coexisté pendant une partie de la campagne. L'hôte n'est pas déclaré
réservé, les mesures ne sont donc pas une comparaison robuste avant/après.
Les anciens probes horizontaux libérant chaque ordre ont un autre périmètre.

À 32k : 13 638 826 candidats uniques, 13 502 432 census et 51 990 104
appels MEB du resolver. Le normaliseur inférieur active 12 842 033 arêtes
pour 34 205 965 demandes, avec 80 858 760 pas find. Les données sont utiles
pour borner le travail intermédiaire et la résidence, pas pour extrapoler
un résultat G4. Les trois grands runs n'ont aucune extra-shell ; leur
gestion est exercée séparément par les oracles, pas par ce triplet.

La baseline initiale 8k a été interrompue après 575,434 s pendant FULL,
code 143, sans sortie terminale. Elle est conservée dans le
[paquet de preuves](../receipts/ball_tower_20260910/README.md), sans facteur
de gain inventé à partir d'un temps censuré. Le peigne qualifie séparément
l'élimination des parcours temporels quadratiques.

## G4 et contrats ouverts

La nouvelle session SPOT a été refusée par `GPUS_ALL_REGIONS`, limite 1.
Le quota est occupé par `cracksam-frangigraph-g4-spot-ew8c`, zone
`europe-west8-c`, label `project=generalized-frangi` : autre travail,
non modifié. Changer de zone ne libère pas ce quota global.

Cible E-HGP contrôlée avant/après par les scripts gardés :
`devpod-gpu-exploration/us-central1-b/ehgp-v7-4fa0e0789a7d5bb06b787d35`,
**TERMINATED**, dernier démarrage inchangé au 6 septembre. Le contrôleur
a refusé sa clôture automatique faute de génération nouvelle ; ce reçu
d'échec reste intact. Le reçu de récupération distinct confirme ensuite
l'arrêt de cette cible précise, sans arrêter l'autre VM.

Aucun worker distant, aucune compilation NVCC, aucun kernel G4 exécuté.
Le worker prêt compare CPU48 et CUDA-census/CPU-FULL à 50k K10 et K5,
puis s10/s12 selon la fenêtre disponible ; ces cellules sont **non exécutées**.
Le s8 local ci-dessus ne tient pas lieu de comparaison des trois s.

La priorité suivante est le [port des témoins universels WSPD par lots](ELIMINATION_BLOCS_WSPD.md#couture-gpu-proposée-le-10-septembre),
puis la parallélisation de la reconstruction FULL et la réduction de sa
résidence. Le contexte CUDA census seul ne retire ni WSPD/tri ni les
dizaines de millions de résolutions et nœuds. La qualification massive
reste ouverte : census globaux, identifiants candidats u32, grandes
coquilles et reprise restent à traiter. Contrats 1 s/100 ms non atteints.

Pins des paquets publiés : preuves
`111359899340d726cb34e338ee244ac114ce0a2d54f05e52458c73f80c41330e`,
régression parents
`4846a705a4f83db47c87d94f41dcf42cfb78bd8ed352d120411f558c11baa474`,
mesures et fermeture GCP
`16c14f8732a5d32d537ba673d116fd9815cc2daa1082e647d267d43ffa3c38df`.
