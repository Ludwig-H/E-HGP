# Raccord réel census → FULL, jusqu'à K10

Qualification CPU bornée sur le commit `c03f6be8488453486b112811071827a96303ec86`,
header FULL `33e7d05e…`. `public_status=not_claimed`. GCP non utilisé.
Le [document de portée](../../docs/QUALIFICATION_TOUR_CENSUS_K10_20260911.md)
explique le juge, ses prémisses et les résultats.

O2 et ASan/UBSan/LSan ROOT passent sur les mêmes sources : 54 constructions
K1..10 chacune, 540 ordres, 18 vrais census, 48 comparaisons physiques,
13 000 coupes et 8 103 948 vérifications verticales. Trois géométries,
deux réindexages, s8/10/12 et cache/statique1/statique4. Neuf rejets et
quatre mutants causaux ; le modèle de juge rejoint l'oracle historique
sur 1 022 MEB et 14 724 requêtes Gamma. Aucun chronométrage contractuel.

```bash
python3 -B morsehgp3D_v7/receipts/full_t2_census_tower_20260911/verify.py
python3 -B -O morsehgp3D_v7/receipts/full_t2_census_tower_20260911/verify.py
```

Le lecteur contrôle les hashes, les snapshots avant/après, les commandes,
les codes, les causes des mutants et l'identité des résultats O2/SAN.
Il ne lance ni compilateur, ni oracle C++, ni GPU. `source/` conserve les
deux fichiers du juge et leur socle complet épinglé ; `runs/` conserve les
commandes exactes, depfiles, sorties et hashes des ELF exclus. Le compilateur
emploie C++20 et `-Wall -Wextra -Wpedantic -Werror` ; Boost reste une dépendance
externe de test, pas un vendor publié ou une dépendance du producteur.
Recompiler `source/t2_gate.cpp` avec ce socle et Boost crée une nouvelle
capture, sans réattribuer les présents résultats à des sources modifiées.

`history/o2_r1/` préserve le premier échec du comparateur et ses deux sources
de juge : il comparait l'ordre de parcours du census à des indices triés.
La correction compare les ensembles de sites, sans réordonner l'entrée du
constructeur ni changer sa géométrie. Les sources produit de ce premier run
sont identiques au socle publié. Les intermédiaires réussis restent en build ;
seules les deux qualifications finales portent les résultats ci-dessus.

Le plafond n14 appartient au juge exponentiel, jamais au produit. Ce reçu
ne certifie ni la complétude universelle WSPD, ni une archive industrielle,
ni les performances 50k ou multi-millions. K=n reste couvert séparément
par les fixtures historiques n≤8, pas par cette nouvelle campagne K≤10.
La [contre-lecture de publication](../../docs/QUALIFICATION_TOUR_CENSUS_K10_20260911.md#limites-de-cette-preuve)
signale aussi deux gardes de métadonnées à compléter dans un reçu distinct :
champ d'ordre égal à K et cardinal exact du tableau vertical. Aucune capture
ni source fermée du présent paquet n'est corrigée après exécution.
