# Mesure post-L2b : portée et décision historique

Source figée : `38b76701b9b0198fc1c37afe16e1480e638e513c`. Le préflight local de finmesure fixe cette source, tandis que le plan passe encore `--commit b319efc84`, copié sans vérification dans la provenance du banc. Aucun résultat de session en cours n’est promu ici.

Le banc appelle seulement `full` et `supports`. Les deux construisent les forêts 1..K au masque 16379 ; supports ajoute le journal et extrait K. Il ne mesure pas `build_order`/7035, ni `points`, ni `plat`. Pourtant `decide()` peut émettre `build_order_par_defaut`. Le modèle stdlib démontre uniquement cette branche avec trois booléens, sans échantillon de temps.

Les valeurs mesurées restent exploitables comme FULL versus supports/L2b après contrôle du reçu. Corriger l’étiquette de commit réel, conserver l’annotation originale comme erreur de métadonnée, et désactiver ce choix historique ; une décision sur order_tree requerrait une variante explicitement mesurée. Aucune nouvelle campagne demandée pour utiliser le périmètre actuel.

Preuves : `proof.json` contient empreintes et ancrages source, identité du plan/préflight et modèle causal. Rejeu borné : `python3 replay.py` et `python3 -O replay.py`. Aucun build, test natif ou acte cloud effectué.
