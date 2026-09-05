# Domaine CPU des preuves conservées

Les preuves des primitives concernent Linux x86-64, GCC 13.3, ses bibliothèques C++/mathématique, les entiers du profil u16 et le binaire64 sans réassociation. `public_status=not_claimed`. Chaque reçu conserve son propre binaire ; une qualification des primitives communes ne réattribue pas la route historique F au producteur FULL.

La [compilation D](receipts_20260905/release/compile_commands.txt) emploie `-O3 -DNDEBUG -std=c++20 -Wall -Wextra -Wpedantic -Werror`, sans `MHGP7_TESTING` dans la CLI, avec CUDA désactivé. Le [cache](receipts_20260905/release/CMakeCache.txt), les [37 binaires](receipts_20260905/release/binaries_after.json) et les [115 sources](receipts_20260905/release/sources_after.json) identifient cette exécution historique.

Les filtres supposent conversions et opérations binaire64 conformes, FMA conforme et graphe arithmétique préservé. Le localisateur exige aussi `FLT_EVAL_METHOD==0`. Chaque thread doit garder son environnement numérique pendant le calcul. `__FAST_MATH__` ou un mode différent de `FE_TONEAREST` désactive les filtres et empêche la construction des grilles. Cette garde ne détecte pas toute option isolée de réassociation ou d'hypothèse de finitude.

La [porte d'arrondi](rounding_probe_20260905.cpp), compilée avec les options numériques Release et `-pthread`, exerce quarante appels sur cinq nuages, les quatre modes et un/deux threads. Son [reçu](receipts_20260905/rounding.json) et les [sorties brutes](receipts_20260905/rounding.stdout) établissent, sur ce corpus :

- au plus proche, 65 588 décisions affine/Jung certifiées et 2 292 grilles construites ;
- sous les trois modes dirigés, aucune de ces décisions ni grille et 189 582 replis entiers ;
- catalogues après préfiltre et forêts identiques entre configurations, avec objets non vides ; deux ouvriers effectivement visités héritent du mode demandé, restitué à l'appelant.

Huit appels utilisent la route complétée sur onze points K1..10 ; les autres familles ont leur route Gabriel explicitement enregistrée. Les [options effectives](receipts_20260905/rounding_environment_4.stdout) désactivent réassociation, `finite-math-only` et `unsafe-math-optimizations`, avec `fp-contract=fast` et `rounding-math` désactivé. Aucun problème n'a été observé ; ces constats ne prouvent pas toute conversion, FMA ou toolchain.

Les [sondes compilées du front](receipts_front_compiled_20260905/README.md) portent des commandes distinctes, chacune O2 et UBSan. Secteurs/cordes ajoutent `-frounding-math -ffp-contract=off` pour les quatre modes ; cellules et fuseaux conservent les options enregistrées par leur propre reçu. Les racines corrigées demandent seulement une proposition finie, non négative et convertible : les [bornes des fuseaux](ARITHMETIQUE_SPINDLE_COURANTE.md) et de la [corde](PREUVE_CHORD_SECTOR_COURANTE.md) rendent casts et carrés représentables avant que les boucles entières établissent le résultat exact.

Changer de compilateur, ABI, bibliothèque ou options exige une qualification distincte. Le [contrat constructeur des primitives](../docs/QUALIFICATION_S1_PRIMITIVES.md#5-binaire64-fma-et-compilation-effective) garde ces prémisses ; cette note conserve leurs témoins indépendants, sans nouveau run. GCP non utilisé.
