# Premier noyau numerique entier : aucune decision flottante.
mhgp12_module_sources(sphere.cpp predicates.cpp center_region.cpp centers.cpp)
# Repere local (v12) : bornes entieres generiques sans centre absolu, garde des boules certifiees, outils locaux.
mhgp12_module_sources(lattice.cpp guard.cpp local.cpp)
# Tranche S8 : entiers a longueur utile, rationnels, table des racines, sommes de radicaux.
mhgp12_module_sources(big.cpp rational.cpp roots.cpp radical.cpp)
