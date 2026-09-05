# Préfixes indépendants de la normalisation v2

5 septembre 2026. `public_status=not_claimed`. Le bridge appelle directement
`normalize_successor`, sans `MHGP7_TESTING`, depuis les dix-neuf headers
capturés du producteur `85c27ab9…`. Cette qualification locale ne remplace
pas celle des forêts FULL.

L'oracle construit le chemin complet dans le tableau initial, puis une
liste d'événements : lectures du chemin, lecture terminale et marqueur
non facturé, puis lectures/écritures des seuls nœuds avant le dernier
prédécesseur. Il applique le préfixe admis de cette liste. Il compare le
tableau entier, la racine partielle, le statut et les deux compteurs.

Les 3 486 cas produisent **3 851 appels par build**, dans une construction
O2 et une construction O1 ASan/UBSan, LeakSanitizer actif. Leurs sorties
sont identiques octet pour octet ; leurs stderr sont vides. Chaque sortie
est jugée normalement et sous Python `-O`.

| Couverture par build | Nombre |
| --- | ---: |
| Succès / refus budget / ancres inconnues | 1 182 / 2 373 / 296 |
| Refus pendant parcours / lecture de compression / écriture | 1 443 / 464 / 466 |
| Marqueur `normalized` incrémenté avant un refus | 930 |
| Refus après au moins une écriture de compression | 700 |
| Appels suivant un premier appel sur le même état | 365 |
| Appels dont le compteur final est près de UINT64_MAX | 1 091 |

Toutes les forêts de successeurs monotones de taille 1 à 4 sont exercées,
ainsi que le tableau vide, des chemins non contigus, plusieurs racines et
une chaîne de profondeur 16. Chaque frontière est confrontée à des états
initiaux nuls, non nuls et proches de MAX ; les compteurs synthétiques
actuels satisfont `2*normalized <= steps`. Cela ne reconstruit pas un
historique complet du Builder. Un plafond abaissé après du travail
antérieur est aussi refusé.

Deux copies privées fautives sont compilées en O2. L'ancienne dernière
paire provoque 813 sorties différentes, notamment un refus au cap exact
v2 avec le même tableau et la même racine. L'écriture avant sa charge
change 569 sorties ; les témoins montrent un tableau modifié malgré un
refus avec racine et compteurs inchangés. Les juges exigent ces motifs
causaux. Leur succès signifie que les mutations sont réfutées.

Le premier build du mutant ancien échoue sous `-Werror`, car `last`
devient inutilisé. Son patch et ses diagnostics sont conservés. Une
révision séparée consomme explicitement `last` sans modifier la faute
`stop=root`, puis passe les mêmes avertissements stricts.

Les commandes, entrées, sorties, mutations, hashes et jugements sont
conservés ici. [dependency_review.json](dependency_review.json) contrôle
les vingt dépendances utilisateur de chaque build, bridge compris ;
ce n'est pas une construction hermétique.
[execution_closure.json](execution_closure.json) clôt tous les builds et
moteurs à **21:54:34,770106 UTC**. CPU0, aucun chronométrage produit,
GCP non utilisé.

Les premières captures sont conservées dans
[initial_loose_normalized](initial_loose_normalized/status.json) : leurs
compteurs synthétiques proches MAX respectaient seulement
`normalized <= steps`. La reprise borne davantage ces données initiales ;
elle ne corrige aucun défaut produit. Les quatre binaires déjà compilés
ont été rejoués entre 21:54:33,912911 et 21:54:34,770106 UTC, après que
CPU0 avait été annoncé libéré. Cette reprise n'a pas respecté la
coordination : un chevauchement avec des chronométrages constructeur
doit être évalué séparément. Les intervalles exacts ont été transmis.

Les scripts de reproduction sont
[full_successor_primitive.py](../../full_successor_primitive.py) et
[full_successor_primitive.cpp](../../full_successor_primitive.cpp).
`prepare` crée les fixtures dans un dossier neuf ; `build` puis `run`
acceptent les modes `O2`, `san`, `legacy_stop` et `write_before_charge`.
`judge` et `judge-mutant` relisent seulement les sorties déjà capturées.
