# MES-C : provenance locale et arrêt certifié

La session du 8 octobre est fermée : contrôleur `completed`, worker et commande `mes_c` de code **0**, `DONE=0`, aucune erreur, résultats vérifiés et arrêt ciblé certifié (`RUNNING → TERMINATED`, une tentative, code 0). La garde invitée reste intacte et la réserve locale est libérée. Ce constat vient des métadonnées locales ; aucun appel distant n'a été fait par l'audit.

L'archive de résultats compte **1 101 322 octets**, SHA-256 `3667572879eb8849bc2d235adb6c8759cd91a64c86093d956c926c4636b4a78c`. Ses **70 entrées** correspondent toutes au manifeste `b118d733…`. Elle contient 34 JSONL de sondes et le rapport `14f42133…`. Le mur externe de **2 050,154 s** mesure le pilote entier sous un délai de commande de 2 150 s ; ce n'est pas une latence FULL.

Le paquet `86eb5522…`, le plan `71ef953a…` et le Git **83ed7620d0b243343688a45473aa479240129725** concordent. La comparaison de **342 fichiers** (src 128, bench 41, tests 168, cmake 4, CMakeLists 1) reprend le vérificateur préparé par l'autre auditeur avant le retour des résultats ; l'inventaire est `6c80a131…`. Les sources du pilote MES-C `6309ac7e…` et du lecteur commun `3594c5d3…` sont également identiques au paquet et à Git. Le contrôle complet tient en Python normal et `-O`.

Le rapport déclare un build Release/u21/CUDA activé et une sonde `b08f9bb8…`. Cette empreinte de binaire est une déclaration conservée dans l'archive, pas une empreinte recalculée sur un ELF récupéré. Le paquet de données est déclaré `1b11650d…` ; aucun tar de données ni payload XYZ n'a été lu par ce contrôle. La liaison indépendante du manifeste local à cette archive de données reste hors de ce reçu.

Le code 0 du pilote signifie qu'il a rendu un rapport : celui-ci annonce **C1/C2/C3 non tenus, verdict refusé et un contrôle manquant**. L'admission, les températures et les chiffres sont relus séparément avec le [contrelecteur MES-C](../mes_c_contrelecture/README.md). La fermeture ne transforme pas les configurations non jouées ou interrompues en succès. Les codes des sondes individuelles ne sont pas archivés séparément ; leur éventuelle inférence demeure explicitement conditionnelle au pilote épinglé.

Les captures privées du contrôleur ne sont pas recopiées ici : seuls leurs hashes et champs publics nécessaires sont conservés dans [capture.json](capture.json). Le sous-ensemble rapport/JSONL/construction a été extrait hors Git pour le lecteur ; ni données, ni identité de compte, ni commande de récupération ne figurent dans ce reçu.

Rejeu local, sans moteur, en réutilisant le [lecteur d'archives publié](../session_l1_recuperation/check.py) :

```sh
python -B check.py --session /chemin/session-mesc --repo /chemin/depot
python -B -O check.py --session /chemin/session-mesc --repo /chemin/depot
```

Les hashes locaux sont vérifiés avant et après le rejeu. Le reçu publie les pins nécessaires, sans dupliquer l'archive ni les 4,4 Mo de journaux.
