# Quotient local de plateau : qualification du composant v7

6 septembre 2026. `public_status=not_claimed`, CPU de référence, entrée u16.
Ce paquet qualifie [local_plateau.hpp](../../src/forest/local_plateau.hpp),
pas son raccord au constructeur FULL. GCP non utilisé.

Les onze commandes de compilation/tests directs sont closes : O2 et
ASan/UBSan avec LeakSanitizer donnent les mêmes résultats, codes 0/2 ;
le mutant supprimant les unions d'étoile échoue avec la cause attendue.
Quatre commandes CMake/CTest séparées terminent avec deux tests réussis.
Il s'agit de tests locaux, pas d'une mesure de tour ou de performance.

La porte confronte 18 tables et 96 rangs à un oracle rationnel indépendant
borné à huit points, plus 40 rangs littéraux issus des quatre coquilles
réelles à 50k. Les représentants, composantes, couvertures et contributions
locales sont comparés. Les cas p=5000 et u=12 contrôlent la branche
analytique sans combinaisons d'intérieurs et le domaine maximal de coquille.
Le [contrat détaillé](CONTRAT_LOCAL.md) distingue ces contributions d'un
delta global minimal, et le quotient local des parents globaux.

Les sources nouvelles sont copiées, les primitives communes référencées
dans leurs paquets scellés. Le mutant est une copie privée qualifiée,
pas une option du produit. Les dépendances Boost des compilations directes
sont hachées avant et après ; les dépendances externes CMake sont seulement
observées après sa compilation. Aucun ELF n'est publié.

Les prototypes antérieurs restent dans `history/` avec leurs propres
sources et reçus : erreur de syntaxe r1, échec LSan/ptrace r2, reprise
r3. Leurs succès ne qualifient pas les sources promues. La préparation
r4 n'a lancé aucune commande : attente d'approbation annulée, aucun
résultat scientifique. Les intentions redondantes et ELF sont omis,
leurs empreintes conservées dans `publication.json`.

Depuis ce dossier, `python3 -B verify.py` et `python3 -B -O verify.py`
vérifient hors ligne empreintes, provenance, retours et portée ; ils ne
relancent ni C++ ni oracle. `sha256sum -c SHA256SUMS` vérifie le paquet.
Les commandes absolues archivées sont historiques. Les trois sources
promues et CMake permettent une reconstruction fraîche depuis le dépôt.
