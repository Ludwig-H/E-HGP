# FULL M — provenance et arrêt clos

Session `v12.20261008.fullm`, source **957e9784fb19ccd2d6348e779ed8b7affadc1f3d**.
Le [protocole capturé](../session_m_protocole/README.md) reste exact : 356 fichiers
moteur/sondes/tests/CMake et sept fichiers ciblés concordent avec le paquet et
les objets Git. Le correctif de terminaison livré ensuite en `e78904c49` et les
propositions d'admission ne sont pas dans ce paquet. Aucun blocage n'est observé
dans les prises revenues ; cela ne réfute pas le scénario abstrait de CST-0241.

**Clôture vérifiée localement :** `completed`, worker0, DONE0, zéro erreur,
résultats vérifiés ; arrêt ciblé certifié code0, une tentative, garde invité
intact, réserve libérée, RUNNING→TERMINATED. Archive de 942 840 octets,
SHA-256 `2aa478be748e41854767a5e65dfbfc77430a0a261c22b328c58b583b0e24524f` :
les **518 entrées** du manifeste concordent, archive stable à la relecture.
Aucun appel GCP ni moteur lancé par l'audit.

| Commande | Code | Durée du processus (s) | Limite (s) |
| --- | ---: | ---: | ---: |
| socle CTest | 0 | 141,284 | 900 |
| MES-FULL | 0 | 555,383 | 2400 |
| apparié cache/séquentiel | 0 | 371,816 | 1500 |
| mutants tour | 0 | 99,626 | 600 |
| MES-D6 profils | 0 | 230,401 | 1800 |

Ces durées englobent les pilotes/constructions et ne sont **pas** des latences
FULL. Les cinq groupes sont clos, sans processus résiduel tué ni flux tronqué.
Le journal primaire du socle donne **722/722 Passed**, aucun Skipped ; la porte
de campagne des mutants tour donne **1/1 Passed**. Ce reçu ne recompte pas les
mutants individuels effacés par le lanceur. MES-M0 LiDAR n'était pas demandé ici ;
son succès antérieur B demeure distinct. Ces portes n'exécutent pas le futur
correctif de terminaison ni sa nouvelle porte.

L'archive contient **38 JSONL FULL, 140 appariés et 126 D6**, soit 304 journaux.
Leur admission, le comptage des passes et les verdicts numériques sont traités
séparément : [FULL](../session_m_admission/README.md), puis relectures appariée
([reçu](../session_m_apparie/README.md)) et D6. Un code0 du pilote signifie qu'il a rendu son rapport, pas que le contrat
100 ms ou tous les bras sont adoptés.

## Construction et périmètre de la provenance

Les deux pilotes FULL déclarent Release/u21/CUDA ON. Leurs empreintes de sonde
**à l'ouverture** diffèrent, conformément aux constructions isolées :

- MES-FULL : `e56983908a275fdfbbb7d92e4d66f81d0d4bc4f360c28d4871f87e2b0435b290` ;
- apparié : `c48961e28e9afd1e267b2bb69f8c3e01ffd0decae586fdcddbcd8cb7e747107e`.

Les quatre bras appariés réutilisent un même chemin de sonde sans reconstruction
dans le pilote. L'archive ne contient pas de rehash de fermeture de ce binaire :
la provenance est donc celle des sources, des constructions et des déclarations
d'ouverture, sans certificat supplémentaire d'immuabilité de l'ELF pendant la
campagne. Aucun transfert implicite de l'identité binaire entre les deux pilotes.
Le lecteur partagé déclaré `c3e9e0f4…805e8f` et les hashes des pilotes correspondent
aux sept sources épinglées, pas aux propositions publiées après le lancement.

Les métadonnées d'entrée (noms, tailles, hashes annoncés) concordent entre reçu et
capture, sans relecture de coordonnées/IDs par l'audit. La cohorte37 se fonde sur
le manifeste JSON et les tailles des membres, pas une inspection des payloads.
L'environnement déclare RTX PRO 6000 Blackwell Server Edition ; son contrôle
d'activité et les valeurs CPU/mémoire sont relus avec chaque campagne.
Dans le pilote apparié, le relevé d'environnement « après » précède les huit
Sessions informatives : il ferme la campagne décisive, sans observation finale
supplémentaire de ces Sessions.
Pour le cache8 Gio, `pic_octets` reste le pic du budget actif : les blocs inactifs
retenus ne s'y ajoutent pas. Le RSS maximal du processus et les capacités GPU
doivent rester séparés, comme annoncé dans le protocole.

## Rejeu

`python3 check.py --repo DEPOT --session DOSSIER_FULLM`, puis `python3 -O` :
deux lectures conformes. Le script réutilise les lecteurs épinglés de protocole
et de manifeste ; hashes avant/après des fichiers de clôture, cinq métadonnées
de commande, bilans CTest et provenance des rapports sont vérifiés. Il ne lit
que les artefacts locaux, n'affiche aucun compte/cible/commande privée, ne lance
ni sonde ni contrôleur. Le dépôt conserve seulement ce reçu et ses hashes ;
les rapports et journaux sont hors Git dans la capture persistante de l'audit.
