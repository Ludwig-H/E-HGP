# Pilote pondéré FULL — tableaux prédéfinis

1 200 points par scène ; K=5, seuil=20. Moyenne ± écart-type entre graines : 3 sphériques, 2 anisotropes/déséquilibrées. Ce ne sont pas des intervalles de confiance.

Le seuil porte sur la masse des facettes avant vote pour HGP pondéré, sur les points pour les comparateurs. Aucun filtre de taille n’est appliqué après le vote. La racine est exclue de l’EOM commun. HDBSCAN standard conserve sa voie propre, uniquement en z1.

Les cinq régimes ont été choisis après la campagne précédente : diagnostic connu, pas test tenu à l’écart ni preuve de supériorité. Les 364 lignes, dont K10 et seuil50, restent dans [rows.csv](rows.csv) ; les 140 agrégats dans [aggregates.csv](aggregates.csv).

## expZ=1

Chaque cellule : **ARI moyenne ± écart-type** ; F1 macro apparié ; couverture ; nombre de groupes.

| Régime | HGP pondéré FULL | Première couverture | HDBSCAN EOM commun | HDBSCAN standard z1 |
|---|---:|---:|---:|---:|
| Sphérique G2, δ8 | 1.0000 ± 0.0000 ; 1.0000 ; 100.0% ; 2.00 | 1.0000 ± 0.0000 ; 1.0000 ; 100.0% ; 2.00 | 1.0000 ± 0.0000 ; 1.0000 ; 100.0% ; 2.00 | 1.0000 ± 0.0000 ; 1.0000 ; 100.0% ; 2.00 |
| Sphérique G8, δ4 | 0.1729 ± 0.0007 ; 0.2813 ; 96.9% ; 3.00 | 0.4948 ± 0.2825 ; 0.6348 ; 89.6% ; 6.00 | 0.2239 ± 0.1191 ; 0.4286 ; 78.5% ; 4.67 | 0.2243 ± 0.1205 ; 0.4286 ; 78.7% ; 4.67 |
| Sphérique G16, δ2 | 0.0226 ± 0.0117 ; 0.0502 ; 81.6% ; 2.00 | 0.0201 ± 0.0164 ; 0.0777 ; 84.8% ; 2.67 | 0.0197 ± 0.0063 ; 0.0539 ; 68.5% ; 2.33 | 0.0196 ± 0.0062 ; 0.0542 ; 68.5% ; 2.33 |
| Anisotrope G8, δ4 | 0.1970 ± 0.1734 ; 0.1938 ; 95.7% ; 2.50 | 0.1970 ± 0.1717 ; 0.1940 ; 95.0% ; 2.50 | 0.1856 ± 0.1686 ; 0.4410 ; 70.2% ; 5.00 | 0.0660 ± 0.0003 ; 0.1438 ; 82.3% ; 2.00 |
| Déséquilibré G8, δ4 | 0.3115 ± 0.0096 ; 0.4034 ; 97.4% ; 4.00 | 0.5359 ± 0.2008 ; 0.6966 ; 86.0% ; 7.00 | 0.2686 ± 0.0373 ; 0.3231 ; 89.7% ; 3.50 | 0.2685 ± 0.0372 ; 0.3229 ; 89.8% ; 3.50 |

## expZ=2

Passer de z1 à z2 modifie chez **HGP pondéré** les scores de facettes Sτ, leurs masses et λ. **HDBSCAN EOM commun** conserve ses masses ponctuelles unitaires et change λ seulement ; il en va de même pour la projection première couverture figée. La règle EOM est commune, mais les mesures sont différentes : cette comparaison n’est pas une ablation pure de λ.

Chaque cellule : **ARI moyenne ± écart-type** ; F1 macro apparié ; couverture ; nombre de groupes.

| Régime | HGP pondéré FULL | Première couverture | HDBSCAN EOM commun |
|---|---:|---:|---:|
| Sphérique G2, δ8 | 1.0000 ± 0.0000 ; 1.0000 ; 100.0% ; 2.00 | 1.0000 ± 0.0000 ; 1.0000 ; 100.0% ; 2.00 | 1.0000 ± 0.0000 ; 1.0000 ; 100.0% ; 2.00 |
| Sphérique G8, δ4 | 0.5134 ± 0.2994 ; 0.6406 ; 91.0% ; 6.00 | 0.4948 ± 0.2825 ; 0.6348 ; 89.6% ; 6.00 | 0.2239 ± 0.1191 ; 0.4286 ; 78.5% ; 4.67 |
| Sphérique G16, δ2 | 0.0224 ± 0.0116 ; 0.0660 ; 83.1% ; 2.33 | 0.0357 ± 0.0242 ; 0.1962 ; 63.1% ; 9.00 | 0.0197 ± 0.0063 ; 0.0539 ; 68.5% ; 2.33 |
| Anisotrope G8, δ4 | 0.1970 ± 0.1734 ; 0.1938 ; 95.7% ; 2.50 | 0.4755 ± 0.1483 ; 0.6386 ; 80.3% ; 10.00 | 0.2523 ± 0.0743 ; 0.5943 ; 59.7% ; 8.00 |
| Déséquilibré G8, δ4 | 0.3110 ± 0.0089 ; 0.4037 ; 97.4% ; 4.00 | 0.5359 ± 0.2008 ; 0.6966 ; 86.0% ; 7.00 | 0.3473 ± 0.0740 ; 0.4813 ; 75.9% ; 5.50 |

## Portée de la preuve

Le contre-audit recalcule les ARI par contingences entières/Fraction, les votes, les masses et les scores par classe. Le solveur Hungarian SciPy reste partagé ; le NMI n’est pas recalculé. Les métriques non définies restent vides dans le CSV ; chaque agrégat publie son nombre de valeurs définies.

Les masses, rayons de sélection et votes sont un post-traitement flottant non certifié. Les avertissements près des seuils et les égalités de votes sont conservés dans le CSV. Le vote final ne fournit pas à lui seul une famille emboîtée de partitions de points. Les références privées, hashes LIVE, commandes et qualifications sont liés dans [receipt.json](receipt.json).

## Exécution et reprise

10 unités complètes reprises, 16 unités exécutées dans de nouveaux workers ; maximum observé de 2 workers simultanés. Une unité est une scène entière et un ordre K, avec les quatre sélections pondérées. Les comparateurs restent hérités de la campagne précédente, sans nouveau fit.

Le reçu séquentiel interrompu reste **failed** et est conservé par hash. Seules ses unités complètes, avec les six payloads et quatorze lignes liés, sont reprises. Sa clôture originale des sources était absente ; la nouvelle vérification LIVE ne la recrée pas rétroactivement.

Les durées mélangent unités héritées et exécutions sous concurrence. Elles ne mesurent ni un gain série/parallèle, ni une borne mémoire. La compression gzip a changé, pas les valeurs JSON attendues. Les métadonnées de reprise et les commandes des workers restent dans le reçu publié.
