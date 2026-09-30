# Option citée : reçu portable du défaut de configuration Clang

Audit du30 septembre2026, lecture d'archive uniquement. Sous Clang18.1.3, la valeur exacte `CMAKE_CXX_FLAGS="-freciprocal-math"` passe la configuration historique, tandis que le flag non cité est refusé. La commande générée consomme réellement cette option après décodage. Clang ne définit pas les trois macros de garde correspondantes ; SiteTree passe le préprocesseur.

Ce constat vise le CMake9642137f… et les sources copiées dans `original/`, pas nécessairement leur état ultérieur. La proposition de correction est de tokeniser les options avec le mode de plateforme approprié puis comparer les tokens interdits, en ajoutant ce cas à la porte. Aucun moteur ni CMake partagé n'est modifié par ce reçu.

Portée limitée : deux configurations (avec détection de compilateur), deux prétraitements, aucun objet moteur, aucun FULL compilé ni mauvais résultat géométrique démontré. Aucun GPU/GCP/chrono ni profil u24/u32 qualifié. Les22/22 du contrôle externe B restent une observation distincte ; ils ne prouvent pas ce cas absent de son banc.

Les huit fichiers et le manifeste originaux sont copiés byte pour byte, sans modification du paquet privé. Le lecteur vérifie l'inventaire exact, tous les hashes AVANT de lire les métadonnées, le manifeste original épinglé, les quatre commandes exactes, leurs codes1/0/0/0, les huit flux stdout/stderr hachés, les guillemets du cache, les tokens réellement consommés, les macros et les quatre pins source avant/après. Il n'exécute/import aucun script original et n'accède à aucun chemin externe : les anciens chemins sont des métadonnées seulement.

Lecture portable : `python3 -B verify.py` ou `python3 -B -O verify.py`. Ces lectures effectuent zéro subprocess et zéro configuration/préprocesseur/moteur nouveaux. `original/capture.py` est une provenance historique, jamais un lecteur à rejouer.
