# MHGP11SPv2 — cohérence de version du lecteur WIP

Source WIP au-dessus du commit `9eee2ed4bcef1e960cdf2456012b84416854dc20`, empreintes dans `sources.json`. Reproduction standard Python normale et `-O`, sans exécution du produit C++, build, données LiDAR ou cloud. Les trois sources nécessaires au lecteur sont figées ; l'ancien lecteur provient de l'objet Git épinglé.

Le rejeu fabrique un MHGP11SP **v1 valide de 200 octets**, un seul site K1, ainsi que son manifeste canonique. L'ancien lecteur accepte le manifeste ; le WIP le refuse (`manifeste : fichier supports`) alors que son décodeur binaire annonce et accepte v1/v2. La rétrolecture complète des anciens dossiers est à décider explicitement : ce constat ne suppose pas un engagement nouveau de compatibilité.

Le défaut d'intégrité est distinct et reproduit : conserver le même binaire v1 et ses SHA exacts, déclarer `files[0].version=2`, publier les agrégats v2. **`check_directory` accepte le dossier et retourne version déclarée 2, version binaire décodée 1.** Dans `bench/mhgp11_formats.py`, `read_manifest` impose v2 (:203–204), `read_supports` accepte v1/v2 (:618–622), tandis que `check_directory` ne recoupe que K (:271–274). L'empreinte valide ne répare pas cette fausse déclaration de format.

Correction minimale : après décodage, exiger `sp.version == entry['version']` avant de comparer les comptes. Si la rétrolecture complète v1 est conservée, accepter versions 1 et 2 dans le manifeste et choisir ses règles d'agrégats, ainsi que `manifest_counts`, selon la version ; les anciens champs/rôles ne doivent pas être interprétés comme le nouveau MST S* seul. Ajouter une porte de lecteur sur un v1 honnête et sur la mutation de version du manifeste, sans nouveau calcul natif. Le changement MST autorisé et le calcul C++ ne sont pas remis en cause par cette preuve.

`python3 -B replay.py` et `python3 -O -B replay.py` donnent la même synthèse, vérifient les SHA des sources et n'utilisent que des objets Git locaux et le petit dossier synthétique temporaire. Aucun binaire natif ni entrée réelle n'est conservé.
