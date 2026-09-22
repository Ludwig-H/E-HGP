# Contre-audit B — première campagne locale de la tour v9

22 septembre 2026. Lecture seule du reçu **publié** au commit `ba762036`,
initialement produit dans le worktree développeur,
`morsehgp3D_v9/receipts/first_tower_20260922/` (`SUMMARY.json`, six couples
`raw/*.json`/`*.time`, `run_campaign.sh`, `summarize.py`). Trois sous-nuages
**sans sol complets, sans coupe spatiale**, à grille optionnelle **1 mm**,
issus de la seule séquence SemanticKITTI **08** : 000000/000100/000200.
Moteur entier u18, `s=8`, huit
workers de génération et recensus, résolveur temporel séquentiel
(`tower_static_threads=0`) sur un hôte local AMD EPYC 7763 (`nproc=8`). Un
essai par cas ; aucun GCP/G4 ni GPU. Les six JSON déclarent
`complete_relative`, `run_tower=true`, les ordres K=1..K demandé et zéro
coquille supérieure à 12 ; les six fichiers GNU time et les six `rc`
portent zéro. « Complet » reste **relatif au catalogue recoupé des boules
émises**, pas à un inventaire indépendant sur ces grandes trames.

| Trame 08 | Sites | K | q3/q4 (s) | Tour aval (s) | Chaîne FULL (s) | Mur (s) | RSS max (Gio) | B : clés distinctes |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 000000 | 39 885 | 5 | 128,483 | 10,811 | 142,684 | 142,79 | 1,01 | 1 306 696 |
| 000100 | 35 551 | 5 | 120,094 | 8,949 | 131,486 | 131,57 | 0,83 | 1 095 926 |
| 000200 | 45 845 | 5 | 246,507 | 12,870 | 263,816 | 263,94 | 1,12 | 1 407 885 |
| 000000 | 39 885 | 10 | 381,247 | 129,979 | 522,106 | 522,63 | 3,89 | 5 512 670 |
| 000100 | 35 551 | 10 | 278,084 | 94,259 | 380,898 | 381,33 | 3,10 | 4 383 302 |
| 000200 | 45 845 | 10 | 686,878 | 102,801 | 801,642 | 802,17 | 3,92 | 5 483 320 |

`q3/q4`, `tour aval` et `chaîne FULL` sont respectivement les champs
`times_ms.q34`, `.tower` et `.chain_total` de la sonde : **temps mur de
phases**, non CPU·s. La chaîne part du nuage déjà lu en mémoire ; le mur GNU
englobe aussi lecture et impression. RSS = maximum GNU time en KiB divisé par
1 048 576 ; B = `catalogue.unique_keys = catalogue.balls`, pas le nombre de
présentations ni de nœuds de sortie. q3/q4 occupe 90–93 % du temps de chaîne
à K5 et 73–86 % à K10 ; l'aval monte néanmoins à 94–130 s sur K10.
À titre de localisation, q2 prend 1,23–2,66 s en K5 et 2,73–5,08 s en
K10 ; fusion + census du catalogue prennent ensemble 0,95–1,40 s en K5
et 4,56–5,55 s en K10. Dans cette capture CPU, q3/q4 puis le
résolveur FULL K10 sont donc les premiers postes à réduire ; un gain de
quelques pourcents sur q2 ne change pas la conclusion.

| Même trame, ratio K10/K5 | q3/q4 | Tour aval | Chaîne FULL | RSS max | B |
| --- | ---: | ---: | ---: | ---: | ---: |
| 000000 | 2,97× | 12,02× | 3,66× | 3,87× | 4,22× |
| 000100 | 2,32× | 10,53× | 2,90× | 3,74× | 4,00× |
| 000200 | 2,79× | 7,99× | 3,04× | 3,49× | 3,89× |

Les compteurs expliquent pourquoi optimiser seulement le nombre de paires
ne suffit pas. Sur 000000, les paires q3/q4 développées passent de
23,69 M à 30,78 M (×1,30) entre K5 et K10, mais les covers de 2,04 M
à 4,51 M (×2,21) et le temps q3/q4 de ×2,97. Dans FULL, les tests de
puissance du MEB passent de **24,86 M à 1,065 G** (×42,9) alors que les
clés ne font que ×4,22 ; le temps de tour fait ×12,02. Sur 000100 et
000200, ces tests MEB font encore ×40,9 et ×39,3. Ces compteurs sont
des opérations de nature distincte ; ils orientent les profils à venir,
sans démontrer à eux seuls quel sous-poste domine le temps de chaque lot.

## Portée du reçu et du lecteur

- `run_campaign.sh` n'utilise que `set -u`, non `set -e`, et touche
  `raw/campaign.done` après la boucle même si une commande échoue. Ce marqueur
  n'est donc pas une preuve de six succès. Ici, les six sorties concrètes et
  `rc=0` sont présentes ; le lecteur vérifie statut, cardinalité/indices des
  ordres, nombre de sites, K/W, identité `balls=unique_keys=sum(by_qmin)` et
  refus de coquille, puis produit `SUMMARY.status=passed`.
- `summarize.py` ne vérifie pas les SHA-256 des entrées inscrits dans
  `raw/context.txt` contre les fichiers réellement lus, ni leur empreinte
  d'entrée de sonde ; il ne contrôle pas `grid`, `s`, `K_effective`, la valeur
  `run_tower`, le digest de la tour, ou le champ GNU `Exit status` séparément
  du `rc` ajouté par le shell. Le nombre d'ordres rejette indirectement
  `--no-tower` dans ces six cas, mais pas une provenance substituée.
- `context.txt` donne un commit `d2700314…`, trois SHA d'entrée et un SHA de
  binaire. Après publication, `sha256sum -c SHA256SUMS` passe **30/30** pour
  les fichiers versionnés du reçu ; un recalcul indépendant retrouve aussi
  les trois SHA des entrées brutes actuellement présentes. En revanche, le
  binaire local a été rebâti après la capture et son SHA actuel ne correspond
  plus au SHA de `context.txt` : **le binaire exact de la mesure n'est pas
  conservé dans le reçu**. Le lecteur ne reconstruit pas ce binaire ni ne lie
  ses octets
  et les données aux objets du commit. `SUMMARY.status=passed` qualifie donc
  la cohérence **locale** des six fichiers, pas une clôture hermétique de
  source, binaire, entrée, protocole et sortie. Le digest FNV de la tour
  n'est pas un oracle d'exactitude ou de complétude.

Les trois trames appartiennent à une seule séquence, sont sans sol et à
grille, non aux trames brutes float32 du contrat principal. Ces ratios font
varier **K sur n fixé**, pas la taille d'entrée n : ils ne démontrent ni
croissance sous-quadratique q3/q4 ou FULL, ni vitesse sur G4, ni contrat
GPU ou <1 s. Prochaine qualification minimale : reçu épinglé avec empreintes
rejouées et provenance binaire/source, vérification des préfixes K5 de K10,
répétitions appariées, puis croissance sur nuages entiers et plusieurs
séquences avec compteurs de candidats, cellules, fragments, clés et sorties.
