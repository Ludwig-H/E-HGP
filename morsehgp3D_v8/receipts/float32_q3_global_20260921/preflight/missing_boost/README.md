# Préflight de dépendance indisponible

Première version du selftest natif : oracle multiprécision Boost envisagé.
Commande exécutée le21septembre2026 depuis la racine du dépôt :

    g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -ffp-contract=off -I morsehgp3D_v8/src -fsyntax-only morsehgp3D_v8/tests/float32_front_probe.cpp

Code de retour1 ; diagnostic :

    morsehgp3D_v8/tests/float32_front_probe.cpp:17:10: fatal error: boost/multiprecision/cpp_int.hpp: No such file or directory

La source exacte de cet essai est conservée à côté. Aucun programme natif
n'a été produit ou exécuté par cette commande. Ce n'est pas un défaut
géométrique et aucun Boost n'a été installé pour le contourner. La version
suivante limite l'oracle entier natif aux petites coordonnées, ajoute des
cas extrêmes symboliques et laisse le jugement rationnel général à la
porte Python Fraction indépendante.
