# Relecture P10 : différentiel de la tête plate S10

Cadre : `exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`. La session `v11.20261005.claudefinp10` est close : `DONE=0`, reçu `completed`, worker code 0 et arrêt ciblé certifié. La source publiée est `38b76701b9b0198fc1c37afe16e1480e638e513c` ; les 635 fichiers utiles du paquet égalent exactement leurs versions Git. Archive SHA-256 : `0a8aae431925ef07271f752ee42b328432e25bc513192bdd627fcf4c8fafcd89`.

Les quatre CTests sont explicitement `Passed`, sans échec, absence ni coupure : synthétique `mhgp11_head_vs_python`, puis les trois `mhgp11_head_vs_python_lidar_ng00/ng01/ng02_k5`. Le build est Release u21 ; les versions observées sont NumPy 2.2.6, SciPy 1.15.3, scikit-learn 1.7.2 et hdbscan 0.8.44.

| Cas | Nuages×K jugés | Appels plats imposés | Clusters / retenus imposés | Bruit imposé |
| --- | ---: | ---: | ---: | ---: |
| Synthétique | 24×5 = 120 | 1920 | 16474 / 16474 | 49474 |
| ng00 K5 | 1 | 8 | 6086 / 6086 | 52434 |
| ng01 K5 | 1 | 8 | 4774 / 4774 | 42194 |
| ng02 K5 | 1 | 8 | 4978 / 4978 | 52770 |

**Les PASS sont lus directement ; les compteurs ci-dessus sont déduits du contrat exact de ces portes.** `CTest --output-on-failure` conserve seulement leurs verdicts verts : aucun JSON métier, `LastTest.log` ou JUnit n'est présent dans cette archive. Le juge épinglé exige ces valeurs par `LINE`, sur la même exécution que le code 0. Le minimum d'appels égale le maximum des boucles fixes : 120×4 mcs×4 sélections = 1920, puis 1×2 mcs×4 = 8 par trame. Chaque refus Python saute un appel ; les 1944 appels imposés impliquent donc zéro refus Python. Ce n'est pas un compteur `python_refusals` directement observé.

Les sélections sont EOM z=1/2/3 et feuilles ; mcs=3/5/10/20 sur les nuages synthétiques, mcs=10/20 sur les trois trames complètes sans sol. Le différentiel compare tous les sites du même arbre de points publié : bijection des clusters, même bruit, étiquette native égale au plus petit PointId et même `tree_k_sha256`. Il qualifie la tête plate sur cet arbre ; il n'ajoute pas un oracle indépendant de construction de l'arbre. Les portes longues ne possèdent pas de jumelles Python `-O`.

La qualification globale reste ouverte : cette capture ne reprend ni les 41 occurrences ordinaires manquantes de fina2, ni supports_route u18/u24, ni les deux campagnes mutants API/CLI, ni le contrat temps FULL. Aucun build, test natif ou accès cloud n'a été lancé par l'auditeur.

`summary.json` contient uniquement métadonnées, empreintes et distinctions observation/déduction. Depuis ce dossier : `python3 -B replay.py --repo /workspaces/E-HGP`, puis `python3 -O -B replay.py --repo /workspaces/E-HGP`. Ces relectures vérifient fermeture, source, empreintes, quatre verdicts et contrats ; elles dépendent de l'objet Git épinglé et des fichiers locaux de la session. Aucun journal brut, donnée LiDAR, identité de compte ou binaire natif n'est copié.

La [contrelecture indépendante de portée](scope_review.json) conserve les ancrages du juge et les preuves des versions Python observées.
