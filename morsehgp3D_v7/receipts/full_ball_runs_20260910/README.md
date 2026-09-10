# Tour retenue CPU et tentative G4 — 10 septembre 2026

`phase=exploration_v7_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

Ce paquet conserve la nouvelle sonde sur uniforme u16, seed3, s8,
tour K1..10, un thread : n400 puis 8000/16000/32000, quatre processus
distincts. La tour conserve ses dix forêts et ses cartes verticales.
`local/receipt.json` doit être completed avec toutes les commandes closes.
La compilation O3 stricte et les dépendances sont épinglées avant/après.
Les stdout contiennent les résultats relatifs, stderr les étapes et GNUtime
(temps du processus, pic RSS). Le digest dense est une empreinte du payload,
pas une qualification géométrique de toute l'entrée.

Ces observations ne sont pas des mesures répétées sur hôte réservé :
compilations et travaux de contrôle ont coexisté pendant une partie de la
campagne. Les lectures loadavg et cgroups sont conservées. Les anciens
probes horizontaux, qui libéraient chaque ordre, ne sont pas des baselines
appariées. Aucune extrapolation 50k ou dizaines de millions n'est validée.
La première baseline 8k interrompue reste dans le paquet de preuves locales
distinct ; elle n'est pas promue à une réussite par ce nouveau passage.

`cmake/` conserve la vérification incrémentale finale du build privé neuf,
quatorze CTests et les deux tests purs du worker normal/-O. Les pins de
tous les fichiers source observés avant/après n'impliquent pas qu'ils
aient tous été compilés par ces sept cibles. Les ELF et headers système
ne sont pas distribués. Ce n'est pas un reçu de compilation NVCC.

`snapshot/` contient l'archive source de la tentative G4 : les dépendances
de la sonde locale ont exactement ces mêmes octets. L'archive ne contient
aucun exécutable ni clé SSH. Les sources/commandes historiques portent
leurs chemins de capture ; ils ne promettent pas des ELF identiques sur
un environnement différent.

Le start SPOT a été refusé pour quota global GPU occupé, avant tout worker.
Le contrôleur a laissé `shutdown_uncertified` faute de génération nouvelle ;
son reçu original est conservé tel quel dans `gcp/attempt/`. ROOT a ensuite
constaté la même cible TERMINATED avec son lastStart historique inchangé,
puis exécuté le stop gardé sur cette identité exacte. `gcp/recovery/`
conserve ce contrôle séparé et réussi, sans réécrire l'échec initial.
L'inventaire d'une autre VM active est signalé seulement, sans mutation.
Le quota étant global, un changement de zone ne résout pas ce refus.

Ni GPU FULL, ni contrat 50k/1s ou100ms, ni régime massif ne sont qualifiés.
Pour contrôler les octets et les observations, sans C++ ni GCP :

```bash
python3 -B verify.py
python3 -B -O verify.py
```

Le manifeste n'est pas une signature externe ; son hash publié ancre ce
paquet. Les champs structurels contrôlés par le lecteur ne se substituent
pas aux oracles et certificats de complétude du producteur.
