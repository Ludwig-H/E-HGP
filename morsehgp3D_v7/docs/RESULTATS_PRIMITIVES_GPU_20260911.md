# Deux primitives exactes exécutées sur G4

11 septembre 2026. `phase=exploration_v7_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.
Le moteur actif reste CPU ; ces deux prototypes CUDA sont qualifiés
séparément, sans raccord au résolveur complet ni à la tour FULL.

## Résultats réellement exécutés

Le [reçu de session](../receipts/gpu_primitives_g4_20260911/README.md)
porte une RTX PRO 6000 Blackwell Server Edition, SM 12.0, dans une G4 SPOT.
Les sources figées sont compilées et liées sur l'invité en C++20/O3 strict,
puis les deux programmes exécutent leurs kernels réels.

| Primitive | Contrôles sur la vraie carte | Portée |
| --- | --- | --- |
| Sélection MEB par lots | 605 cas, 21 432 contrôles, 197 extra-shells, 44 rejets et quatre flags causaux | Support choisi, slots, coquille, compteurs, transport et propriétaire ; validation et matérialisation finales encore hôte |
| Clé primitive, PGCD et division 128 bits | 13 573 cas indépendants, 325 752 mots comparés, 22 refus attendus | Formes/PGCD/divisions pleine largeur et circumboules de supports ; pas un terminal de résolution |

Les deux injections de transport de la seconde gate, écriture omise et mot
corrompu, rendent 1 sur `backend.expected_word`, pas sur un refus artificiel
final. Les arguments inconnus du premier programme rendent 2 avant accès CUDA.
Les tailles et alignements du transport sont contrôlés sur la vraie carte.
Sources et binaires restent identiques avant/après ; les dépendances projet
effectivement compilées sont confrontées au manifeste.

Les [preuves MEB locales](../receipts/gpu_meb_device_route_20260911/README.md)
et [arithmétiques locales](../receipts/gpu_ball_key_device_20260911/README.md)
restent distinctes : O2, ASan/UBSan avec détection de fuites active depuis
ROOT, oracles indépendants puis compilation/lien NVCC. Les anciens échecs
de compilation et de LSan sous ptrace restent conservés. Le nouveau succès
ne les réécrit pas.

## Ce que cela change

Le port des résolutions ne bute plus sur la seule disponibilité de ces deux
primitives en CUDA. Les fonctions entières existantes sont largement
réutilisées ; aucun moteur flottant approximatif n'a été substitué. Le
wrapper possède son index, réutilise les buffers et refuse une publication
partielle. Une libération dont le succès n'est pas certifié reste un échec
permanent du contexte.

Cela ne donne **aucun facteur d'accélération de la tour**. Les programmes
sont des juges, pas des benchmarks de débit. Le premier conserve même une
copie entière de l'index côté hôte : solution privée de qualification,
pas choix final pour plusieurs dizaines de millions de points.

Le prochain raccord doit réunir sélection et clé sur device, rechercher la
boule dans le catalogue exact, décider l'admission à K, rechercher l'intrus,
construire les bornes entières nécessaires et poursuivre la descente jusqu'à
son vrai terminal. Une réponse par facette unique pourra alors revenir au
calendrier CPU. Les ancres pré-lot et les identités des composantes ne sont
pas remplacées par des jetons historiques de workers.

Les limites de provenance sont explicites : les sources projet sont figées,
mais les compilateurs sont identifiés par version et les dépendances système
hashées après compilation seulement. Ce n'est pas une chaîne système
hermétique. Les depfiles observés sont absolus ; le traitement des chemins
relatifs du worker reste une fragilité de portabilité, sans effet observé ici.

## Contrats inchangés

Les derniers [temps de tour 50k](RESULTATS_TOUR_CACHE_G4_20260910.md)
restent environ 419 s pour K1..10 et 33,6 s pour K1..5. Ils précèdent la
voie statique CPU du 11 septembre. Ni 1 seconde, ni 100 ms, ni les nuages
de plusieurs dizaines de millions de points ne sont qualifiés par ces gates.
Le paramètre WSPD s=8/10/12 n'est pas un paramètre de ces primitives locales.

## Fermeture de la session

Une seule session SPOT, sans création de VM ni installation CUDA. La cible
`devpod-gpu-exploration/us-central1-b/ehgp-v7-4fa0e0789a7d5bb06b787d35`,
génération `2026-09-11T01:20:08.309-07:00`, est certifiée `TERMINATED` après
récupération des résultats. GCE STOP/3600 s et arrêt invité à 30 minutes
ont été vérifiés avant tout travail. Aucune autre VM E-HGP active détectée
à la clôture ; aucune autre VM modifiée. Le contrôleur termine avec code 0.
