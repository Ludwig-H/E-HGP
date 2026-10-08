# Cache2b : provenance, portes et fermeture de la nouvelle mesure

La session `v12.20261008.cache2b` est **fermée et récupérée** : completed, worker0, DONE0, aucune erreur ; arrêt ciblé certifié code0 en une tentative, observation `RUNNING → TERMINATED`, garde intacte et réserve libérée. Lecture normale et `-O` identiques. L'audit n'a exécuté ni contrôleur, moteur, compilation ou appel distant.

| Preuve locale vérifiée | Valeur |
|---|---|
| Source effective | `bdfca8fb198e6711626c6506b816a2a656315c83` |
| Paquet | 12 451 687 octets ; SHA `c2c9d19a628418c440b7dfa3f3f23233bbf7cb5e2ee6664433bd6001519f644a` |
| Plan | SHA `e242791190532004d392e43714b10530dda16eb6d52413ff423fa04a290e744b` |
| Résultats | 488 774 octets ; SHA `daeb2bd9aab5b16452bf78ca6011bd5e6bed5b94f08a96ec035434869f7a8e1a` |
| Manifeste intérieur | **264 fichiers**, tous couverts et rehachés |
| Sources produit/tests/construction | **359 fichiers identiques à Git**, inventaire `61f9bf7f…` |

Les deux commandes sont closes avec code0, sans résidu tué ni flux tronqué : socle CTest **141,168 s** sur limite900 ; pilote apparié **262,105 s** sur1500. Ces murs englobent des campagnes et ne sont pas des latences FULL.

Le journal primaire du socle contient **730 Passed /730, zéro Skipped/Failed**, avec les six portes `tower_region` (native, inventaire, carte, juge de carte et variantes `-O`). Sa configuration demande Release sans option CUDA ou précision ; les défauts de la source sont CPU/u21. Ce résultat sur hôte G4 n'est pas730 portes CUDA. La sonde appariée est construite **séparément** en Release/u21/CUDA ON. Ces nouvelles portes sur G4 complètent la [preuve native de terminaison](../a_terminaison_native/README.md) sans réécrire sa capture locale.

Le pilote exécuté est `c795dae5…`, le lecteur FULL `15437e5f…` : les [correctifs livrés en85](../lecteurs_livraison_85db/README.md) sont effectivement présents. Le rapport archive maintenant **l'ELF d'ouverture et de fermeture**, égaux à `a72884f5a4d25078c8192f2dabc698ed6490567931b4ffca8d2509b0c402d903`. Le pilote livré prend la fermeture après les Sessions informatives, contrairement au pilote de M. Deux hashes égaux ferment ces deux instants, sans prouver une immuabilité continue contre toute intervention extérieure.

Le plan et le rapport concordent : catalogue appareil, u21, **K5/W48**, dix processus par bras/trame et dix passes ; `ng00,ng01,ng02`. Les bras sont explicitement `ref=--cache=0`, `aa=--cache=0`, `cache=` (défaut8 Gio). Le gain du cache est donc confronté à une référence réellement sans cache. Les six Sessions informatives couvrent37 trames, deux processus par bras, deux passages par trame ; ce reçu ne modifie aucun critère ni ne les remplace par les trois trames décisives. **Aucune nouvelle mesure CPU ou K10 dans ce lot.** L'admission, les règles A/A et les statistiques sont traitées séparément par le lecteur mathématique.

Les sept déclarations nom/taille/SHA des entrées sont exactement celles de [Session M](../session_m_provenance/README.md) : trois fichiers ng avec leurs IDs et archive37 `0f94df19…`. Cela raccorde les métadonnées d'entrée déjà fermées à cette campagne. Le contrôleur déclare la vérification distante réussie ; aucun tar de données, XYZ ou ID n'a été ouvert ou haché ici.

La première tentative nommée `v12.20261008.cache2` est **déclarée refusée avant démarrage faute de disque local** par le README développeur389b5e453. Aucun reçu primaire de cette tentative n'est disponible dans le dossier des sessions au moment de la contrelecture ; l'audit n'atteste donc pas indépendamment son absence de démarrage. Cette limite est distincte de la fermeture complète de cache2b.

Une capture persistante extérieure au dépôt rassemble213 fichiers utiles au lecteur, dont105 JSONL (1 928 663 octets), et leurs hashes. Aucun journal volumineux, ELF, identité de compte ou commande privée n'est copié dans ce reçu. [capture.json](capture.json) conserve uniquement pins et métadonnées publiques ; les helpers d'archive et de comparaison Git déjà publiés sont réutilisés.

```sh
python3 -B check.py --repo DEPOT --session SESSION_CACHE2B
python3 -B -O check.py --repo DEPOT --session SESSION_CACHE2B
```

La relecture exige les objets Git et fichiers locaux fermés ; aucune sonde n'est appelée. [SHA256SUMS](SHA256SUMS) couvre les quatre fichiers du lot. Le pic actif, le cache physiquement retenu et le RSS cumulatif restent trois notions distinctes ; aucune somme mémoire ou baisse garantie du pic n'est déduite de cette fermeture.
