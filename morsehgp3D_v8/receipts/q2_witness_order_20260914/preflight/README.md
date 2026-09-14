# Préflight invalidé, conservé

Le 14 septembre 2026 à 10:25 UTC, les deux portes de reçus ont été
lancées avant le gel des sources. La porte normale passe ; la porte
Python optimisée échoue sur la fermeture des hashes, pendant l'ajout
de la nouvelle cible CMake par le constructeur. La couverture indiquait
encore 44 tests. Ce n'est pas une qualification des sources finales.

`LastTest.log.snapshot` conserve le journal brut CTest avant son
remplacement par les qualifications suivantes. Aucun contrôle de hash
n'a été désactivé ; les tests sont relancés après gel explicite.
GCP non utilisé.
