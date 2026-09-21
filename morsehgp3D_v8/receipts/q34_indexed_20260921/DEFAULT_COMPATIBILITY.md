# Compatibilité du défaut 31 → 32

Les **8 paires / 16 commandes** de [DEFAULT_COMPATIBILITY.json](DEFAULT_COMPATIBILITY.json) passent : préfixes du scan0 de 64 et 128 points, K5/10, s8, W1/4, backend28. Chaque exécutable reçoit exactement les anciens arguments, jusqu’à `samples records`, sans les options de filtre/census ajoutées en tranche32. Il produit donc réellement le schéma v1 et le chemin `disabled + scalar`.

L’ancien exécutable épinglé `build/v8_lidar_global_r2_20260921/mhgp8_wspd_q34_probe` est comparé au nouveau `build/v8_q34_indexed_20260921/mhgp8_wspd_q34_probe`. Aucun build n’a été modifié. Les données complètes normalisées, clés, profondeurs, supports, coquilles, signatures, comptes géométriques et front concordent exactement. Les quatre configurations n/K émettent respectivement 457, 1589, 922 et 3746 candidats, identiques à W1 et W4.

Les différences ne sont pas masquées : chaque champ réellement différent figure avec ses deux valeurs dans le reçu. Les temps sont descriptifs, sans revendication de performance. L’affectation des jobs et sorties aux workers peut changer. L’état interne passe de **2688 à 3392 octets par worker**, soit **+704 octets** ; à W4, 10752 → 13568 octets. Les champs v1 visibles restent inchangés, pas la taille interne du worker.

Les autres différences de capacité observées à W4 sont :

| n / K | Somme des capacités d’arête, ancien → nouveau | Capacité des records privés avant fusion, ancien → nouveau |
|---|---:|---:|
| 64 / 5 | 8440 → 9312 | 103200 → 113440 |
| 64 / 10 | 14672 → 13184 | 366136 → 407096 |
| 128 / 5 | 22016 → 23808 | 227144 → 247624 |
| 128 / 10 | 27312 → 27264 | identique |

Ces capacités dépendent des répartitions entre vecteurs privés : la sonde somme leurs capacités avant de fusionner les records. Ce ne sont ni du RSS ni un pic mémoire global. Aucun autre champ de travail ne diffère ; à W1, aucune de ces différences de capacité n’est observée.

Le premier essai est conservé dans [DEFAULT_COMPATIBILITY_FIRST_FAILED.json](DEFAULT_COMPATIBILITY_FIRST_FAILED.json) : quatre commandes natives réussies, puis refus du lecteur parce qu’il ne classait pas encore `memory.worker_record_capacity_bytes_before_merge` comme capacité dépendante de l’ordonnancement (123680 → 113440 octets). Sa fermeture sources/entrées/binaires est intacte. Le helper exact est [archivé](preflight/default_compatibility_before_capacity_fix.py), SHA256 `f47c4ff5fc9efe15fa975e3d13212cb3f4cf95da0b68d8f389de055c8c057eb1`. Seule cette classification du helper hors inventaire a été corrigée ; le premier échec n’est pas promu en PASS. Les 16 nouvelles commandes sont une capture distincte : **20 commandes natives au total**, aucune reconstruction.

[DEFAULT_COMPATIBILITY_READBACK.json](DEFAULT_COMPATIBILITY_READBACK.json) ferme quatre lectures normales/`-O`, historiques/live, avec résultats identiques, **206 sources, 11 entrées et 6 artefacts inchangés** avant/après. Le lecteur réanalyse aussi les quatre lignes du premier essai, sans changer son statut. Les deux helpers sont [check_default_compatibility.py](check_default_compatibility.py) et [close_default_compatibility.py](close_default_compatibility.py).

SHA256 capture finale : `7ce37c972c44cd79393e619e739b9aa88ba3211d2c2254b4a04762d18d978216`. SHA256 readback : `6c0a574d4d51f18f9bcc29bd61690a508c54b3823b177444fec4085a6e38601f`.

Cette borne de compatibilité porte sur de petits préfixes réels et le défaut historique. Elle ne qualifie ni la tour FULL, ni une borne de croissance, ni un contrat G4. Aucun GCP utilisé ici.
