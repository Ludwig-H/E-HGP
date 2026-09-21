# Trois mutations compilées du noyau Float32Ball

La capture faisant autorité est [compiled_r2](compiled_r2/COMPLETION.json) : 25 commandes terminées, baseline conforme à l'oracle rationnel indépendant, puis trois erreurs géométriques ciblées détectées. Les quatre exécutables retournent zéro ; aucune exception, erreur de compilation ou divergence de compteur ne tient lieu de preuve causale.

| Mutation compilée | Attendu exact | Erreur réellement observée |
| --- | --- | --- |
| Poids barycentrique nul accepté | Tétraèdre dont le centre est sur une face : support refusé | Support accepté, dans les modes filtré et exact |
| Signe du déterminant omis dans la puissance | Centre intérieur au tétraèdre d'orientation négative : signe −1 | Signe +1 en mode exact |
| Décalage des exposants normaux augmenté d'un bit | Triangle aigu mêlant normal minimal et subnormaux : support accepté, requête sur la coquille | Support refusé en mode exact |

Les trois petites fixtures sont archivées, avec leurs mots binary32, dans [le manifeste](compiled_r2/MANIFEST.json) et [l'entrée brute](compiled_r2/fixtures.txt). Leur validité et les signes sont recalculés par l'oracle Python `Fraction` de `tests/float32_ball_gate.py`, sans utiliser les coefficients natifs. Les sources modifiées, leurs patches, les commandes, codes de retour et sorties brutes sont conservés. Les en-têtes système sont empreintés avant compilation, après compilation et à la fermeture ; ils ne sont pas recopiés dans le reçu.

Les [quatre lectures postérieures](MUTANTS_R2_READBACK.json), Python normal/−O avec et sans contrôle des sources et binaires encore présents, passent et donnent le même résultat géométrique. Les 420 fichiers d'entrée, dépendances et artefacts restent inchangés avant/après ces lectures. Leur [petit collecteur](readback_r2.py) ne réexécute aucun code natif. Le lecteur live vérifie aussi ses entrées au début et à la fin de chaque lecture.

La première capture [compiled_r1](compiled_r1/COMPLETION.json) est conservée intégralement : elle passait déjà les trois mutations, mais précédait trois durcissements du lecteur (type entier exact du code de retour, lien des fixtures du manifeste, fermeture des entrées live). R2 est une nouvelle compilation dans un nouveau répertoire, pas une réinterprétation silencieuse de R1. Le moteur et les fixtures n'ont pas changé entre les deux.

Cette preuve couvre trois défauts précis de la primitive q3/q4 binary32. Elle ne qualifie ni census, ni catalogue de boules, ni tour FULL, ni calcul GPU, ni contrat de trame entière.
