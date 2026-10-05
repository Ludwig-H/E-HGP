# Clôture source des corrections API, CLI et banc

Pin publié : `98a00955083d483306c4f92b9031e382e81b0e59` ; corrections API/CLI/banc introduites par `be05bfad8`, puis correction de la matrice au pin 98a. Les quatre empreintes ciblées restent identiques entre ces deux commits. Ancienne capture WIP et ses constats conservés comme historique.

API : `supports_route.cpp` retrouve exactement les octets du pin publié `38b76701b`. Les identités de fichiers, manifestes et journaux entre voies et W1/W4, les planchers et le contrôle du calcul public restent exigés. `tests.cmake` conserve intégralement les six attentes u21 et choisit les empreintes fichier/manifeste par profil. Le journal gravé reste dans les verdicts des trois profils.

Rejeu stdlib : 36 préfixes non vides de 16 chiffres hexadécimaux, six substitutions par profil ; lignes u21 identiques aux anciennes attentes. Les lignes réellement conservées de fins concordent pour les six témoins u21 et les six u24 ; finm concorde pour u18/8000. Les cinq autres paires u18 sont nouvelles attentes à requalifier dans la reprise G4. Aucun ancien verdict `Failed` n’est transformé en résultat de test rejoué.

CLI : mutation `sp_masque_16379` inchangée, désormais raccordée à `mhgp11_cli_points` qui atteint `order_params` ; 28 individus et plancher 28 conservés. Lecture favorable, jugement natif du manifeste corrigé distinct.

Banc : la décision historique est désactivée (`sans_objet_post_l2b`). Les mesures portent toujours sur FULL contre supports/L2b ; ni order_tree, ni points, ni plat. Le reçu finmesure demeure figé : son argument annoté `b319efc84` était erroné, la source réelle `38b76701b` du reçu est l’autorité. La lecture future de `V11_SOURCE_PIN` est une proposition séparée, pas une modification prétendue de ces octets historiques.

Matrice : l’exigence de label LiDAR était encore présente à 23:35, et la capsule initiale garde cet état. Le complément `matrix/` de l’auditeur evidence montre le seul retrait au pin 98a ; le juge réel accepte alors une fixture supposant 873 PASS. C’est un modèle causal et non une qualification native. Il contient aussi les empreintes/ancrages worker et contrôleur et une proposition de marquage de source pour le banc futur.

Commandes : `python3 -B replay.py` et `python3 -O -B replay.py` ; puis, depuis `matrix/`, `python3 -B replay_matrix_correction.py` et `python3 -O -B replay_matrix_correction.py`. Le premier modèle est autonome. Le complément matrice nécessite les objets Git et l’archive fina2 aux chemins déclarés. Aucun build, test natif, acte cloud ou nouveau reçu de tests locaux effectué par l’auditeur.
