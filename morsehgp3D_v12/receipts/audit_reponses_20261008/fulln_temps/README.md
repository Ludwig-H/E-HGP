# FULLN : contre-extraction des temps, 8 octobre 2026

Portée : 38 JSONL publics, 610 passes FULL, 392 retenues ; sources au commit
`8a0716e74` (R1, sans A6b/B3). Aucun moteur, cloud ou payload lu. Admission
complète de session, arrêt et provenance : auditeur evidence_1704, séparément.
Les statistiques du rapport public sont toutes recalculées et identiques.

| Trame | K5 catalogue GPU | K5 catalogue CPU | K10 catalogue GPU |
| --- | ---: | ---: | ---: |
| ng00 | 80,310 ms | 354,359 ms | 495,103 ms |
| ng01 | 66,592 ms | 298,896 ms | 366,141 ms |
| ng02 | 83,320 ms | 361,175 ms | 422,194 ms |

Le GPU ne traite que le catalogue ; G et la forêt restent sur CPU. Ce sont
les murs FULL à 48 fils sur G4, jamais des temps GPU seuls. `cpu_ns` mesure
le temps CPU cumulé des fils, distinct de cette colonne CPU.

K5 GPU : 5 processus × 10 passes par trame, passe 0 exclue, **45 valeurs**.
CPU K5 et GPU K10 : 3 × 5, passe 0 exclue, **12 valeurs**. La médiane publiée
porte sur toutes ces valeurs, pas sur les médianes des processus. Ainsi CPU
ng00 vaut 354,359490 ms contre 354,092845 ms par la seconde convention.
Le maximum jugé est le maximum des médianes des processus ; le maximum brut
K5 GPU ng00 vaut 97,824405 ms, contre 81,943134 ms pour le maximum jugé.

Les 37 trames ont chacune cinq secondes visites retenues, une par processus
(deux tours complets, ordre tournant). Médiane des 37 médianes :
**142,410389 ms** ; pire médiane : **287,186635 ms** ; maximum jugé et brut :
**288,220946 ms**. Ces deux derniers concernent `00/001896`, 95 586 sites.
14/37 ont toutes leurs secondes visites sous 100 ms. Le contrat reste
**non tenu**. Aucun bras CPU sur les 37 trames n'est présent ici.

La colonne « 1re passe » des 37 est la première visite de chaque trame,
**pas 37 démarrages à froid**. Les vrais premiers murs ng00–02 GPU ont pour
médianes 103,340 / 88,713 / 108,992 ms. Lecture, préparation hors ligne,
segmentation et ouverture de Session sont hors mur, même pour la passe 0.
Validation, empreinte FUL1 et libération sont aussi hors mur ; toutes les
passes portent une empreinte, calculée entre deux chronos.

## Décomposition et piste développeur

`G` est la période jusqu'au dernier calcul de G, avec ouverture et tâches de
forêt concurrentes. La queue est la partie après G ; elle n'est pas toute
la forêt. Les fenêtres des tâches sont des sommes de temps de fils, pas des
murs ni des temps CPU. Ne pas additionner les médianes d'étages.

Sur la trame médiane `00/003624`, la **moyenne** exacte de cinq murs se
partitionne en ms : 142,267449 = P 2,858836 + C 38,120028 + G 59,537954 +
queue 41,709402 + reste 0,041230 (arrondis affichés). Les sommes entières
et médianes séparées des trois trames repères sont dans `results.json`.

Hypothèse explicite : si seule la queue devenait nulle et chaque autre
terme de chaque passe restait fixe, recalculer `wall - queue` passe par
passe laisserait 100,752529 ms de médiane sur `00/003624`, 214,637798 ms sur
`00/001896` et 210,374496 ms sur `08/002119`. Seules 18/37 médianes seraient
sous 100 ms. Une réduction de queue reste utile, mais cette transformation
isolée ne suffit pas au contrat ; elle ne prédit aucun gain d'A6c. Une
modification réelle peut aussi changer G, C et la clôture.

Identité CPU/GPU vérifiée pour ng00–02 et répétitions vérifiées pour chaque
nom. Ne pas imposer l'identité avec leurs alias v12set : DONNEES.md au pin
annonce des coordonnées différentes pour ng00/ng01 malgré les mêmes IDs ;
ng02 seul est identique. Une vérification trop large de notre draft a été
corrigée sur cette preuve documentaire, sans modifier une donnée produit.

## Borne du catalogue CPU

Sur les **mêmes 36 passes CPU chaudes**, C dépasse 100 ms dans chaque cas.

| Trame (12 passes) | C minimum | Moyenne des parts C/FULL | Ratio des sommes C/FULL |
| --- | ---: | ---: | ---: |
| ng00 | 297,194066 ms | 84,251760 % | 84,251619 % |
| ng01 | 253,067167 ms | 85,200567 % | 85,199983 % |
| ng02 | 298,262715 ms | 83,777109 % | 83,777110 % |

La moyenne des rapports individuels, le rapport des sommes, la médiane des
rapports et le rapport des médianes sont quatre statistiques distinctes,
recalculées dans `cpu_catalogue_share` ; aucune n'est une somme de médianes.
Au banc, P puis C précèdent la tour. Donc **à C inchangé**, même une tour
instantanée laisserait FULL ≥ C > 100 ms sur chacune des 36 observations.
Accélérer G seul ne suffit pas au contrat CPU. Cette borne conditionnelle ne
s'applique pas à une transformation qui accélère aussi le catalogue.

## Rejeu

`python -S [-O] check.py --raw <fulln/results/extracted/results/cmd/001_mes_full/files/full/brut> --repo <repo>`

`capture.json` ferme chaque JSONL, le rapport public et quatre sources Git.
`check.py` extrait indépendamment, sans importer le pilote. Lectures normal
et optimisée ; les JSONL sont rehachés après extraction. Pas de qualification
statistique d'un gain contre une autre session : ces comparaisons restent
descriptives et les protocoles de chronométrage doivent être conservés.
