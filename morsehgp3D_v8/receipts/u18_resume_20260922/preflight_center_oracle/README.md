# Préflight conservé — pas une qualification close

Build neuf `build/v8_u18_resume_preflight_20260922`, GCC Release, base
`a74e90f2` plus les 42 portes non commises reprises du précédent développeur.
Commande : `ctest --test-dir build/v8_u18_resume_preflight_20260922 -L gate
-LE slow --output-on-failure -j 1 --output-junit PRELIGHT_GATES.xml`.
Résultat : 78/79 PASS ; `mhgp8_q4_local_gate` échoue sur
`engine q3 center differs from the rational circumcenter in the plane chart`.
Le fichier source exact du test et les sorties CTest sont conservés ici.

Cause : le nouveau test compare x à `expected[1]` et y à `expected[0]` alors
que son repère rationnel et le moteur ont le même ordre A/B. Correction du
juge seulement, pas de correction géométrique du moteur pour cet échec.

Le rebuild intermédiaire suivant a aussi rencontré une attente de test
devenue obsolète : `location beyond the proven quotient guard accepted`.
Après la correction du domaine public, le centre valide `(2^101,0,1)` est
hors racine et retourne `nullopt` avant la division, au lieu d'une erreur
interne de garde. Le test vérifie maintenant cette absence de certificat
ET le refus de la représentation hors domaine `(2^117,0,1)`.

Une première commande multi-cibles de rebuild a échoué avec
`No rule to make target 'mhgp8_u18_numeric_domain_gate'` lors de la
régénération CMake en cours ; la cible était présente au Makefile régénéré.
Le relancement explicite est distinct. Le préflight mutable n'est pas un
build épinglé ni l'autorité des captures finales fraîches.
