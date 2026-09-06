# Admission mémoire de la sonde FULL — census direct

6 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

La sonde contrôle désormais les phases séparément : candidats et deux ensembles logiques de Survivors avant préfiltre, puis candidats réels + Survivors réels + un seul tableau BallData avant census. Les coefficients sont dérivés des types avec arithmétique contrôlée, soit176U puis144U+240S sur l'ABI observée. Le second tableau BallData du garde hérité n'existe plus dans le census direct nominal. Le transfert final est un swap, pas une copie.

Ce sont des **payloads logiques nommés**, pas des capacités d'allocateur ni une borne RSS globale. La garde d'indices u32, les autres limites mémoire et le code produit `run.hpp` sont inchangés. Chaque ligne JSON porte `census_payload_accounting=preflight_survivor_then_direct_census_v2`. Les anciens reçus et lecteurs ne sont ni modifiés ni réétiquetés.

Captures closes dans [run_r1](run_r1/receipt.json) :20commandes aux codes attendus,40vérifications arithmétiques en O2 et ASan/UBSan, ancienne porte52vérifications, digest24vérifications, rejets CLI code2. Quatre micros n=8, P=0/illimité et1/4threads, terminent avec les mêmes digests de forêt. Ils ne constituent aucun résultat de performance à50k ou de complétude géométrique. Vérification Python normale et optimisée effectuée ; aucune porte Python ne dépend de `assert`.

Intégration CMake : les seuls nouveaux tests sont `mhgp7_full_gabriel_census_payload` (programme0) et `mhgp7_full_gabriel_census_payload_bad_argument` (programme2). Le [reçu CTest](ctest/receipt.json) et le JUnit conservent leur exécution dans un répertoire neuf, avec Boost existant explicitement indiqué. Aucune propriété TIMEOUT nouvelle, aucun quota temporel ajouté par les captures. Aucun gros benchmark, aucun GCP.

Les trois sources nouvelles sont copiées sous `run_r1/sources/morsehgp3D_v7/`. Les sources inchangées sont référencées par chemin relatif et SHA256 dans [publication.json](publication.json), vers les paquets voisins déjà scellés ; aucune copie répétée et aucun ELF. Les empreintes des binaires exclus restent dans les reçus. CMake, les commandes, depfiles, stdout/stderr et sources avant/après sont conservés. Les chemins absolus dans les captures sont des observations historiques, pas des exigences pour vérifier ce paquet.

Vérification portable depuis le paquet, en conservant les paquets voisins référencés : `python3 finish.py --verify .`, ou `python3 -O finish.py --verify .`, puis `sha256sum -c SHA256SUMS`. Cela vérifie les octets publiés et les références, sans exécuter de moteur ni reconstruire les binaires. `run_r1/record.py` conserve le protocole original : sa vérification privée attendait aussi les ELF et le snapshot complet, volontairement non dupliqués ici ; ne pas la présenter comme un rejeu autonome de ce paquet léger.
