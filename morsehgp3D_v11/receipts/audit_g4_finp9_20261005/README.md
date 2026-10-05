# Relecture P9 : différentiel exact de la hiérarchie de points S9

Cadre : `exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only / not_claimed`. La session `v11.20261005.claudefinp9` est close : `DONE=0`, reçu `completed`, worker code 0 et arrêt ciblé certifié. Source publiée : `38b76701b9b0198fc1c37afe16e1480e638e513c`. Les 635 fichiers utiles du paquet égalent leurs versions Git. Archive SHA-256 : `e1497f2db57f5351048a2709e83352e8fe3b579f20061b72b2a20da3b0f72a57`.

Les quatre CTests sont explicitement `Passed`, sans échec, absence ni coupure : `mhgp11_points_vs_python`, puis les trois `mhgp11_points_vs_python_lidar_ng00/ng01/ng02_k5`. Build Release u21 ; versions observées conformes aux épingles NumPy 2.2.6, SciPy 1.15.3, scikit-learn 1.7.2 et hdbscan 0.8.44.

| Porte | Plancher du compteur nuages | Sites comparés cumulés | Sites retardés cumulés |
| --- | ---: | ---: | ---: |
| Synthétique | ≥407 | ≥100000 | ≥50000 |
| Chaque trame ng00/ng01/ng02 K5 | aucun | ≥30000 | ≥20000 |

Ces minima découlent du code 0 et des planchers du juge épinglé. **Aucun total métier exact n'est affirmé : P9 n'exige pas de `LINE` exacte de compteurs.** Les logs verts de `CTest --output-on-failure` ne conservent ni sa ligne de compteurs, ni `LastTest.log`, ni JUnit. Le juge ne possède pas de compteur `python_refusals` et ne compte pas un refus avant de sauter son cas : refus d'un programme ou écart rendent la porte non conforme.

Le parcours synthétique comprend 400 petits nuages aléatoires, quatre fixtures fixes et les uniformes 300/2000/8000, K=1..5 quand l'ordre est dans le domaine. `--m-all` compare m=1 pour K1, puis m=1 et m=K+1 pour K≥2. Chaque porte LiDAR lit la trame complète K5, avec m=6 et W8 ; elle liste aussi les quatre fixtures fixes, dont deux seulement sont admissibles à K5. Le compteur de nuages listés ne représente donc pas cinq nuages effectivement pendus dans ces portes.

Le différentiel exige, sur le même catalogue et la même tour exportée, l'identité exacte de tous les sites : PointId en ordre Morton, parents/rangs, niveaux Fraction t/M/Q, propriétaire, plancher, drapeau strict et niveaux référencés ; puis tous les plateaux, blocs et entrées de sites. Cette comparaison qualifie la hiérarchie de points sur ces tours. Elle n'ajoute pas un oracle indépendant de construction de FULL.

Les portes longues ne possèdent pas de jumelles Python `-O`. La qualification globale reste ouverte : cette capture ne reprend ni les 41 occurrences ordinaires manquantes de fina2, ni supports_route u18/u24, ni les deux campagnes mutants API/CLI, ni le contrat temps FULL. Aucun build, test natif ou accès cloud n'a été lancé par l'auditeur.

`summary.json` ne contient que métadonnées, empreintes et bornes imposées. Depuis ce dossier : `python3 -B replay.py --repo /workspaces/E-HGP`, puis `python3 -O -B replay.py --repo /workspaces/E-HGP`. La relecture vérifie la fermeture, les sources, les empreintes, les quatre verdicts et le contrat ; elle dépend de l'objet Git épinglé et des fichiers locaux de la session. Aucun journal brut, donnée LiDAR, identité de compte ou binaire natif n'est copié.

La [contrelecture mathématique indépendante](scope_review/README.md) fixe les ancrages source, les champs comparés et les limites du juge.
