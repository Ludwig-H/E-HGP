# S4b v21 : rendre la comparaison q4 non vacante et nominative

24 septembre 2026 — contrelecture du port **WIP** `7d4ffef96` et de la modification non publiée de `tests/gpu/lanes_port_gate.cpp` (SHA-256 `93e1c50251123b67af5cf594b3e2d90b3e4096966e5180c3f432e7f34decb5bc`). Ce n'est ni un défaut géométrique confirmé du port q4, ni une mesure CUDA/G4. Le binaire hôte observé porte SHA-256 `0b325ec5aa1b1dac9c9978287603a53aedd25db2698bf1e9ff4ed3cbd475c812` ; sa relation de compilation au source reste une observation locale, pas un reçu gelé.

## Ce que `--compare` prouve et ne prouve pas

Le nouveau mode fichier compare, sur chaque arête **effectivement parcourue**, la voie hôte S4b au moteur. C'est une bonne porte d'objet : clé primitive, support trié, profondeur et taille/empreintes de coquille doivent coïncider. Mais `file_compare` saute toute survivante dont `certificates.deferred[j] != 0`, sans compter ces exclusions dans sa sortie ; il n'impose aucun plancher de graines ou de tétraèdres et imprime `equal=1` même si les comparaisons q4 sont vides. `same_records` compare `shell`, `shell_sum`, `shell_xor`, **pas les IDs nominaux de coquille** : `Q34LaneRecord` ne les transporte pas. Ainsi, même un `equal=1` sur un fichier LiDAR ne signifie ni que toutes les arêtes q4 ont été couvertes ni que les coquilles sont nominalement égales. Les condensés de chaîne communs ne rendent pas indépendantes les omissions communes.

Rejeu court avec le binaire hôte ci-dessus, `--workers=1 --k=3 --compare` :

| entrée u32le | SHA-256 | arêtes q4 comparées | graines q4 | enregistrements q4 | sortie |
| --- | --- | ---: | ---: | ---: | --- |
| [`two_points.u32le`](two_points.u32le), `(0,0,0),(1,0,0)` | `a90d90bbbac09e655865d42536c19ef1068baad0e2318d06db7772ed130968d0` | 1 | 0 | 0 | `equal=1`, code 0 |
| [`tetra_zero_width.u32le`](tetra_zero_width.u32le), tétraèdre régulier entier | `2231f1ac4ad789584e809127246e5630e4c6f3b18e6735a2a1a665f046494f63` | 6 | 4 | 1 | `equal=1`, code 0 |

La première ligne est **correcte géométriquement** : elle démontre seulement qu'une comparaison q4 vide passe. La seconde donne un contrôle positif utile. Pour `a=(0,0,0)`, `b=(1,1,0)`, `x=(1,0,1)`, `y=(0,1,1)`, on a `D=E=F=2`, `G=3`, `Q=D(3G−2EF)=2`, donc `mubar=1` et grille J8 `[-1,0,0,0,0,0,0,0,1]` ; la racine q4 positive est `−1`, à la borne du domaine. La porte hôte compare un enregistrement. Ce cas exerce une grille à seaux de largeur nulle et une borne extrême, **pas** le transfert d'une racine sur une borne intérieure entre deux seaux ; prévoir cette dernière fixture distinctement.

## Porte utile avant une conclusion sur 08/000000

Pour un fichier LiDAR nommé, publier le SHA de l'entrée et du binaire, le total des survivantes, les arêtes certifiées reportées, les masques q4 demandés, les arêtes réellement décidées/comparées, les graines et les enregistrements. Refuser le qualificatif « toutes les arêtes q4 » si une arête éligible est exclue ; si les reports restent légitimes, les compter et les juger séparément par le repli moteur. Exiger un plancher positif de graines **et** d'émissions sur une fixture q4 désignée, et tuer un mutant qui supprime un enregistrement. Sur les petites fixtures, une instrumentation de test doit **exposer les listes triées d'IDs de coquille des deux voies** pour les comparer. Reconstruire la coquille depuis la clé et le nuage donne l'ensemble géométrique attendu et recoupe taille/empreintes, mais ne révèle pas les IDs réellement collectés par S4b tant que `LaneRecord` ne les transporte pas. Les deux empreintes 64 bits restent un contrôle de grand volume, pas une égalité nominative.

Le ledger physique S4b mérite une porte distincte : `check_lanes_batch` ne recoupe qu'une partie des identités `work4`, et `judge_lanes_filter` compare les objets/émissions, pas `pass_site_tests`, `bucket_events` ou `compare_steps`. Injecter une mutation d'un de ces champs et exiger son refus, ou publier ces coûts comme **mesures non jugées**. C'est nécessaire pour interpréter la traîne des gros seaux et décider quand basculer vers un tri exact ; la réussite du différentiel d'objet ne borne pas leur coût.
