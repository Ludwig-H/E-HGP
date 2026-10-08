# Session T2d-C : provenance et fermeture

La session a **produit des mesures**, après six constructions réussies. Le contrôleur est `completed`, le worker et la commande `t2dc_flux` ont le code **0**, `DONE=0` ; aucune erreur de session. L'arrêt ciblé est certifié (`RUNNING → TERMINATED`, une tentative, code 0), garde invitée intacte et réserve libérée. Aucun appel GCP n'a été effectué par l'audit.

Archive résultats : **699 846 octets**, SHA-256 `c3d2d609099d95366cd593cf7c6b2a9127e046d38bb28788976c67496e139153`. Les **454 entrées** correspondent toutes au manifeste `d54fcfa8…`. Le rapport de 7 286 365 octets est `3e6e9aaa…`. Le temps externe **716,888 s** est celui du pilote complet (délai 2 040 s), pas un chrono de catalogue ou FULL. Aucun résultat n'est attribué à un échec de compilation.

| Source | Archive | Comparaison au Git |
| --- | --- | --- |
| Avant | `v12_src_902041f66.tar.gz`, `2e760983…`, 5 010 884 octets | **342 fichiers exacts** de `902041f6675a079deb4a642068b036635171db1f` |
| Après | paquet `c95555bb…`, 5 383 333 octets | **351 fichiers exacts** de `02b735d6bc7d0eeb0053303e9d9711db96a985df` |

Périmètre comparé : `src`, `bench`, `tests`, `cmake` et `CMakeLists.txt` de la v12. Les inventaires valent `0ec04a34…` et `51de28af…`. Les sources du pilote `88ca8e3f…`, lecteur `dae75c32…` et juge `970482ed…` sont aussi vérifiées dans le paquet après. Les sept pins détaillés et les paramètres du plan `5327ed00…` sont dans [capture.json](capture.json).

**Le Pool est identique avant/après**, SHA `eed86be4…` : c'est l'ancienne implémentation. Le [nouveau Pool livré en 5b3362bbd](../pool_equipes/README.md) est absent de cette campagne. Aucun gain observé ici ne peut lui être attribué.

Le rapport déclare **« adopte »**, zéro refus/rejet du juge, A/A valide et cinq leviers adoptés. Le mutant `flux_sans_attente_appareil` est construit et joué avec code 0, sans expiration ; le juge le classe **tué par empreinte**, pas par crash. Les six empreintes de binaires déclarées avant/après sont égales. Ces empreintes sont archivées dans le rapport : aucun ELF récupéré n'a été rehaché par ce reçu.

Portée déclarée : **210 processus catalogue** (sept bras, trois trames, dix tours), K5/u21/feuille24/W48, dix passes par processus. Les informations distinctes comprennent 18 prises FULL, 18 K10, 18 cache et 74 prises pour 37 trames v12set avant/après ; aucune information non jouée n'est déclarée. Les neuf paires d'identité, 18 contrôles d'identité des bras et 18 prises FUL1 sont des rubriques séparées. Le code 0 du pilote et cette fermeture ne remplacent pas la contrelecture des cohortes, bruts et statistiques, menée dans un supplément distinct.

Les logs natifs de `device_open` et `device_open_budget` sont présents et le rapport leur attribue le code 0 (respectivement neuf témoins, et quatorze appels sous budgets serrés). Cela décrit les preuves disponibles ; leur qualification complète n'est pas dérivée de la seule fermeture.

Les archives **de sources** avant/après ont été lues. Aucun tar de scènes, fichier XYZ ou ID n'a été ouvert. Les hashes de données du contrôleur restent des déclarations vérifiées à distance par lui, sans relecture locale de payload par l'audit. Les commandes privées, identités et reçu brut du contrôleur ne sont pas recopiés ici. Rapport et logs métriques sont conservés hors Git ; pas de duplication de leurs 12 Mo dans ce reçu.

Rejeu Python normal/`-O` uniquement, avec les [helpers d'archive](../session_l1_recuperation/check.py) et de [comparaison source](../session_mes_c_provenance/source_check.py) déjà publiés :

```sh
python -B check.py --session /chemin/session-t2dc --before-archive /chemin/v12_src_902041f66.tar.gz --repo /chemin/depot
python -B -O check.py --session /chemin/session-t2dc --before-archive /chemin/v12_src_902041f66.tar.gz --repo /chemin/depot
```

Le vérificateur contrôle les hashes locaux avant/après et ne lance aucun pilote ni moteur.
