# Session O : contre-extraction FULL et petits, 10 octobre 2026

Source `aa6338ee8b3d5daf6a821043c3bdd9d63ce85c66`, A6c adopté. Relecture de
métadonnées JSONL seulement ; aucun moteur, compilateur, cloud ou payload.
Admission, codes natifs et clôture des exécutables relèvent du reçu de session,
pas de ce lecteur de temps. Les lecteurs indépendants FULLN sont réutilisés
par empreinte ; aucune importation du pilote produit. Leurs sources et
rapports sont remplacés explicitement par les pins O dans un dossier temporaire.
Les statistiques FULL du rapport public sont toutes retrouvées exactement.

| ng | K5 catalogue GPU | K5 catalogue CPU | K10 catalogue GPU |
| --- | ---: | ---: | ---: |
| 00 | 80,364862 ms | 354,732391 ms | 492,641250 ms |
| 01 | 66,932516 ms | 298,496191 ms | 365,516970 ms |
| 02 | 79,335583 ms | 355,135846 ms | 422,867588 ms |

Ce sont des murs FULL, avec la tour sur CPU, à 48 fils. K5 GPU : 5 processus
× 10 passes, 45 chaudes par trame ; CPU K5 et GPU K10 : 3 × 5, 12 chaudes.
Médianes de toutes les passes chaudes, pas médianes des médianes de processus.
38 processus, 610 FULL, 392 retenues au total. Pas de CPU K10 sur ces trames,
ni de bras CPU sur les 37 trames. `cpu_ns` reste le temps CPU cumulé des fils.

Les 37 trames de six séquences ont chacune cinq secondes visites retenues :
médiane des 37 médianes **116,592136 ms**, pire médiane **239,771652 ms**,
maximum jugé et brut **241,892572 ms**, **15/37** sous 100 ms sur toutes les
secondes visites. Contrat non tenu. La première visite d'une trame n'est pas
un nouveau démarrage à froid. Les premières passes ng00–02 GPU K5 valent
103,294629 / 92,136353 / 99,985581 ms (médianes). Ouverture de Session,
lecture, segmentation/préparation, validation, empreinte et libération sont
hors mur ; l'empreinte est effectivement calculée entre les chronos.

## N vers O : décomposition, sans attribution causale

Les noms, tailles de cohortes et empreintes FULL concordent entre N et O,
à groupe et K identiques. Deux sessions distinctes : comparaison descriptive,
pas un essai A/B apparié et pas une preuve causale d'un gain d'A6c. FULLN
avait 142,410389 ms de médiane des trames, 287,186635 ms de pire médiane,
288,220946 ms de maximum et 14/37 trames sous 100 ms. Toutes les trames ne
s'améliorent pas individuellement.

Sur les 185 passes retenues des 37 trames, les **différences de moyennes**
O − N, calculées à partir de sommes entières, sont en ms :

| FULL | P | C | G | queue | reste |
| ---: | ---: | ---: | ---: | ---: | ---: |
| −16,602882 | +0,034840 | −0,142711 | +3,135284 | −19,630862 | +0,000568 |

Ne pas additionner des médianes d'étages. G est la fenêtre murale qui va
jusqu'au dernier calcul G, ouverture et travail forestier concurrent compris.
Une hausse de cette fenêtre n'est pas une hausse prouvée du seul travail G.
La queue démarre après G ; son raccourcissement ne mesure pas à lui seul le
gain FULL. Les moyennes O sont : FULL 114,410958, P 2,727517, C 35,994732,
G 65,021181, queue 10,624378, reste 0,043150 ms.

Le dernier registre R n'est plus principalement K5 : **170/185 → 17/185**.
Dans O, K2 finit dernier 105 fois, K3 59, K1 deux et K4 deux. Sur les deux
trames les plus lourdes examinées (`00/001896`, `08/002119`), K3 finit
dernier 5/5 fois. Le dernier calcul G appartient à K1 dans 94/185 cas et
K2 dans 91 ; `prepare_graph` attribue toutefois les tranches par ordre K→1.
Ces rangs de fin ne permettent donc pas de déduire que K1 est intrinsèquement
le calcul le plus cher.

Si seule la queue devenait nulle, tous les autres termes de chaque passe
restant fixes, seulement **16/37** médianes passeraient sous 100 ms : médiane
des trames 104,754743 ms, pire médiane 219,228385 ms. C'est une borne de
cette transformation conditionnelle, aucune prédiction d'une vraie modification.
Sur `00/001896`, la moyenne O vaut 240,536564 ms : P 4,632630 + C 64,318043
+ G 150,110061 + queue 21,436362 + reste 0,039468. Accélérer la queue seule
ne peut expliquer le chemin jusqu'à 100 ms. Supprimer G **et** la queue,
reste fixe, donnerait au plus 72,165998 ms de médiane par trame : cette
borne idéale n'assure pas que l'accélération nécessaire soit réalisable.

**Suite développeur :** juger B3b sur FULL, et publier les fenêtres par ordre
ainsi que le retard entre préfixe réellement publié et consommé ; mesurer
la contention avec la forêt avant de modifier les priorités. Le rang du
dernier R invite à examiner K2/K3, sans supprimer leurs dépendances ni
supposer que leur coût est isolable. Une accélération de G peut avancer le
début de la queue sans accélérer le dernier R ; son gain G n'est pas un gain
FULL garanti. Les changements d'ordonnancement gardent la même frontière
chronométrée et les mêmes sorties exactes.

## CPU et petits

Les 36 passes CPU chaudes gardent C > 100 ms : minima ng00/01/02
298,674416 / 253,188930 / 296,700765 ms ; moyennes des rapports C/FULL
84,364910 / 85,215191 / 84,714897 %. À catalogue inchangé, accélérer la tour
seule ne peut atteindre 100 ms. Les feuilles sont le plus gros sous-chrono
C sur 36/36 ; les retirer fictivement, reste fixe, laisse des médianes FULL
199,426174 / 172,810573 / 207,988454 ms. Ce chrono inclut la région du Pool,
pas seulement les prédicats.

MES-C : 56 processus, 3608 FULL, 2392 retenues, 48 processus réussis et
huit refus `wide_leaf` (sphères 3000/10000, CPU/GPU, K5/K10). Les 147 nuages
réguliers comprennent 132 réels et 15 synthétiques, à trois visites par
configuration ; les 12 difficiles restent séparés. Médianes des médianes
chaudes des 132 réels, à 48 fils : CPU/GPU K5 **23,348222 / 7,248853 ms**,
K10 **43,024642 / 11,503143 ms**. Régression CPU K5 : intercept
13,781946 ms et pente 9,559561 µs/site ; C1/C2/C3 restent non tenus.

Parmi les 20 réels de 100–140 sites, CPU4/CPU48 : 6,126240 / 10,098407 ms ;
GPU4/GPU48 : 4,072225 / 4,263640 ms. W48 est plus lent sur les 20/20 dans
les deux voies. Une seule exécution par configuration : signal pour mesurer
un seuil de parallélisme, pas validation causale d'une politique automatique.
Sur ces 40 passes CPU, W48−W4 ajoute en moyenne 3,542677 ms FULL dont
3,392318 ms C ; le delta C se concentre dans parcours (+2,073905 ms) et fin
d'étage (+1,672882 ms), avec feuilles −0,370611 ms. Éviter de ne cibler que
les feuilles sur ces très petits nuages.

## Rejeu

`python3 -B -S [-O] check.py --raw <O/results/extracted/results/cmd> --reference-raw <N/results/extracted/results/cmd> --repo <repo> [--public-repo <repo-contenant-le-reçu-O>]`

Les JSONL demeurent hors dépôt ; pins dans `capture.json`, résultats compacts
sans identité de compte ni coordonnées dans `results.json`. La première
version de ce lecteur supplémentaire supposait noms et étiquettes MES-C
identiques ; elle refusait la cohorte. Le lecteur final utilise la table
publique nom/étiquette épinglée, sans changer une mesure ou le produit.
Lectures normales et optimisées réussies ; pas de qualification native ajoutée.
