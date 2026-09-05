# Première comparaison FULL lazy interrompue

5 septembre 2026. public_status=not_claimed ; CPU de référence, entrée u16.

La campagne initiale est close avec le statut failed : une réussite eager à n=8 000/s=8 est conservée, suivie d’une capture lazy non terminale. Le fichier lazy ne contient que la configuration ; son code de sortie, sa durée, sa fin et la cause de l’interruption restent inconnus. Le plafond prévu de 600 s n’est pas un timeout observé.

La date de clôture du contrôleur est administrative : elle ne date pas la fin du processus lazy. La carte de sources prise à cette clôture ne remplace pas sa carte après tentative absente. Aucun artefact terminal manquant n’est reconstruit. Les quatre tentatives suivantes n’ont aucun artefact de lancement dans cette campagne.

Le passage eager reste une observation réussie non appariée : dix ordres horizontaux complets relativement aux catalogues fournis, 149 951,700395 ms jusqu’au terminal et 1 833 004 KiB de pic RSS GNU time. Le juge de reçus est rejoué en lecture seule ; aucun moteur n’est relancé par la publication.

La reprise exige une campagne neuve répétant aussi eager, avec les mêmes pins de binaire et de sources. Aucun ratio entre l’ancien eager et un nouveau lazy n’est autorisé. Ni accélération lazy, ni contrat 50k/1 s/100 ms, ni résultat massif G4 ne sont établis ici.

Tous les fichiers du répertoire clos sont copiés octet pour octet sous [capture/](capture/), sans omission, correction ou écrasement ; les protocoles sous protocol/ sont inertes. [Qualification de la capture](publication.json), [inventaire](manifest.json), [sommes](SHA256SUMS). GCP non utilisé.
