# Relecture de la mesure G4 post-L2b

Cadre : `exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`. La session `v11.20261005.claudefinmesure` est close : `DONE=0`, reçu `completed`, worker code 0, arrêt ciblé certifié. Commit réellement exécuté : `38b76701b9b0198fc1c37afe16e1480e638e513c`, avec 635 fichiers utiles égaux au Git épinglé. Archive SHA-256 : `46fe01d6391bf59a9babf57fec09f60cee02d178da1b2a03541664c81159f62e`.

Les 52 appels mesurés sont tous `ok`, sans défaut d'identité :

- 48 appels K5 : trois trames ng00/ng01/ng02, W1 et W48, sorties FULL et supports, premier appel puis trois prises chaudes par configuration ;
- 4 appels K10 descriptifs : ng00 seulement, W48, FULL et supports, premier appel puis une prise chaude par sortie.

Le binaire mesuré correspond à l'empreinte du worker. Pour chaque même trame/K/sortie, les empreintes du fichier et du manifeste sont identiques entre toutes les prises et W ; `tree_k_sha256` est commune entre FULL et supports. Les huit groupes d'identité et les valeurs par appel sont conservés dans `summary.json`. Les fichiers natifs étaient effacés par le banc après lecture de leurs empreintes ; le rejeu vérifie la concordance des empreintes enregistrées, sans relire ces fichiers disparus.

Deux contrôles supplémentaires ont rendu directement `conforme` sur les trames complètes : ng02 (45845 sites) et ng00 (39885 sites), K5/u21, W1/W4/W48. Chacun comporte 14 appels et 52 contrôles, sur la trame et une fixture cosphérique : lecteur officiel, répétition, permutation, réétiquetage et signature commune avec FULL.

Les voies effectivement mesurées sont **FULL/16379 et supports/L2b, également construit depuis FULL/16379**. Le JSON porte encore `provenance.commit=b319efc84` et `decision=build_order_par_defaut`, hérités du plan et de la règle antérieure. Ces deux annotations sont conservées comme obsolètes, séparées du commit faisant autorité et exclues des conclusions. Aucun `build_order`/7035, `points` ou `plat` n'a été mesuré ; aucune décision entre FULL et order_tree n'est justifiée par cette capture.

Le seuil de 100 ms n'est pas acquis : chacune des **18 prises chaudes K5/W48** dépasse `100000000 ns` pour l'étage `tree` seul et pour `total`. Ce constat porte sur les valeurs individuelles conservées, sans utiliser les médianes ou ratios du document. K10 reste descriptif sur une seule trame ; les trois trames appartiennent à la séquence 08. Le premier appel dit froid ne constitue pas une purge de cache certifiée.

La qualification globale reste ouverte : 41 occurrences ordinaires manquantes de fina2, attentes supports_route u18/u24 et campagnes mutants API/CLI demeurent distinctes. Cette mesure ne fournit aucune comparaison HDBSCAN, exécution GPU ou qualification de trame brute avec sol.

`summary.json` ne conserve que valeurs observées et métadonnées, sans annotations statistiques utilisées pour décider. Depuis ce dossier : `python3 -B replay.py --repo /workspaces/E-HGP`, puis `python3 -O -B replay.py --repo /workspaces/E-HGP`. Ces relectures contrôlent la fermeture, les sources, l'archive, les tuples d'appels, les identités et les deux contrôles W48 ; elles dépendent des archives locales de la session et de l'objet Git épinglé. Aucun nouveau calcul produit, build, test natif ou accès cloud. Aucun journal brut, identité de compte, donnée LiDAR ou binaire natif n'est copié.
