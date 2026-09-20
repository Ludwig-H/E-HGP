# Préflights de la reprise q4 — 20 septembre2026

Avant gel, la première configuration du build neuf
`build/v8_q4_family_20260920` a échoué : CMake n'a pas trouvé
`Boost_INCLUDE_DIR` dans les chemins système. L'appel de build suivant
échoue donc sans Makefile. Aucun moteur ni test exécuté par ces commandes,
aucun reçu qualifié, aucun build épinglé modifié. Réutiliser explicitement
le chemin d'en-têtes déjà déclaré dans les caches historiques, s'il existe.
