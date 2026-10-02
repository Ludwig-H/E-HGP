# Catalogue v11 — sessions G4 du 2 octobre 2026

Captures CPU sur G4 : qualification du catalogue puis banc sequentiel dans une commande worker distincte.
Ni la tour FULL ni une execution GPU ne sont presentes. Le contrat FULL en 100 ms reste ouvert.

## Lecture LIVE

Depuis ce dossier :

```sh
python3 -B check.py
python3 -B -O check.py
python3 -B check_selftest.py
python3 -B -O check_selftest.py
```

`check.py catalogue1` limite la lecture a une capture. Le code 0 signifie **pieces coherentes**, meme si la campagne
a echoue : chaque issue est affichee. Le recu brut local nomme par `receipt.json` demeure obligatoire et son SHA256
est verifie ; ces pieces ne sont pas une archive autonome. Aucun calcul natif ni appel GCP n'est lance.

Chaque capture conserve un recu filtre, UNE archive originale `results.tar.gz`, la copie exacte `matrix.json`,
le manifeste d'entrees `inputs.json`, et `catalogue.json` seulement si le banc en a produit un. Aucun octet KITTI
ni binaire canonique du catalogue n'est copie ici. Le lecteur utilise `extractfile`, sans extraction sur disque.
Il recoupe fermeture ciblee, generation, source commit, archive, inventaire/JUnit des configurations, empreintes
de compilation et identite du binaire mesure. Pour le banc, il verifie les 36 unites demandees et les omissions
justifiees par le protocole. Les empreintes canoniques declarees sont comparees entre repetitions ; les fichiers
canoniques supprimes sur la VM ne peuvent pas etre rehaches ici.

Schemas consommes : `ehgp.v11.session_receipt.v1`, `ehgp.v11.g4_matrix_summary.v1`,
`ehgp.v11.build_provenance.v1`, `mhgp11.catalogue_benchmark_inputs.v1`, `ehgp.v11.catalogue_benchmark.v1`.
Le marqueur `ehgp.v11.catalogue_attempt.v2` ouvre explicitement la lecture des erreurs de lancement,
collecte et artefact et impose `leaf_size` dans 13..256, coherent avec la commande. Les anciens rapports sans
marqueur gardent leur capacite de feuille 32. Un hash conserve avant un echec d'artefact ne rend pas la tentative reussie.
Le parseur JSON strict et le juge JUnit viennent du [lecteur des fondations](../developpement_20261002/check.py).
Les six temoins JSON du self-test sont fabriques pour verifier ce lecteur : aucune qualification produit n'en
decoule. Vingt-trois corruptions sont refusees, en normal et `-O` ; voir `check_selftest.json`. La contrelecture
a notamment ferme le cas d'un rapport partiel faussement annonce complet et le code 0 d'une commande partielle.

## Captures

| Capture | Commit execute | Qualification | Banc |
| --- | --- | --- | --- |
| catalogue1 | `643fe47d7c77b7738908bd2233f082f803366c1d` | 947/948 portes passent ; une porte de mutants echoue ; Clang absent | Non execute, 36 unites non jouees |
| catalogue2 | `f391bf13e1a9a982025bde86fc9219b5b7430afc` | 948/948 portes passent ; Clang absent | 7 tentatives : 1 terminee, 6 delais depasses ; 29 unites non jouees |
| catalogue3 | `e6fe34cb082f19d0041c829dfb38ea249319ab19` | 960/960 portes passent ; Clang absent | Feuilles de 16 : 13 tentatives, 7 terminees, 6 delais depasses ; 23 unites non jouees |

Les trois recus attestent le retrait de la cle OS Login, la suppression de la cle privee temporaire
et la liberation du verrou de reservation.

`catalogue2` est close, avec arret cible certifie ; les lectures LIVE normal/`-O` passent en conservant
l'issue de campagne **ECHEC**. Le seul catalogue termine est l'uniforme synthetique 8k/K5 :
15,478187473 s pour l'API, 15,625014764 s pour le processus, une repetition. Les six autres tentatives
atteignent le delai de processus de 30 s ; aucune duree finale de catalogue n'en est deduite. Les trois
trames LiDAR K5 font partie de ces delais ; aucune mesure LiDAR complete ni repetabilite chronometrique
n'est acquise dans cette capture. La capacite de feuille demandee est 32.

`catalogue3` est egalement close et relue en normal/`-O`, avec arret cible certifie. Le calendrier reste
incomplet, donc l'issue de campagne reste **ECHEC**, malgre la qualification verte. Avec des feuilles de 16, uniforme 8k/K5
termine trois fois : mediane 8,240 s, intervalle [8,210 ; 8,240] s. Les trois hashes sont identiques entre eux et
a celui de `catalogue2` : `2671f84acd61597300af06e7f164b0c8cd552b726bf7519711c8772d3febf74a`.
Uniforme 16k/K5 termine en 17,310 s ; uniforme 32k/K5 expire. Les trames entieres 000000/000100/000200 sans sol
terminent en 26,018/20,741/24,093 s pour l'API catalogue K5, chacune une seule fois. Les cinq tentatives K10
expirent ; K10 uniforme 32k n'est pas joue. Ce sont des temps de catalogue, loin de 100 ms ; aucun chrono FULL
n'est disponible. Les capacites de feuille 32 et 16 sont deux campagnes de commits distincts, pas des repetitions appariees
d'une unique version.

Les trois entrees reelles sont les trames entieres sans sol `08/000000`, `08/000100`, `08/000200` :
39 885, 35 551 et 45 845 sites, grille 1 mm/u18, IDs des retours d'origine. Ce sont trois trames d'une meme sequence.
Trois nuages uniformes synthetiques de 8k/16k/32k sites completent le banc ; ils ne remplacent pas ces trames.
Le temps de l'API catalogue comprend ses deux passes, le tri et les sorties en memoire. Le temps du processus
inclut lecture et serialisation ; le masque de sol et la preparation horsligne sont exclus. Les pics publies
sont les reservations `Buffer`, Cloud vivant compris, et non le RSS. Aucun de ces temps ne mesure FULL.
