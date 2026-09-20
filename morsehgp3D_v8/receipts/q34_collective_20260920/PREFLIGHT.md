# Préflights conservés comme historique de développement

20 septembre2026. Avant le gel des163 sources et les captures finales.

La première exécution de la nouvelle gate a échoué en Release et sous
Clang ASan/UBSan avec `bad rational: non-zero singular denominator`.
L'oracle construisait une fraction Boost `cpp_int` avec dénominateur
négatif ; le test interne du type rationnel non borné la refusait.
L'oracle normalise maintenant explicitement le signe du dénominateur.
Le code produit n'a pas été changé pour contourner cet échec.

La relecture a aussi retiré des alias de pools encore vivants autour du
test de remise à zéro du propriétaire : le nouveau test exerce bien la
possession conservée par l'appel lui-même. Les deux exécutions corrigées
ont donné les mêmes8549 contrôles. Ce sont les captures finales closes,
pas les préflights, qui font autorité pour cette version.

Le lecteur initial attendait deux tests de propriété de filtre par seed
même avec un pool vide. Le raccord contourne le filtre dans ce cas ;
ses compteurs doivent tous être nuls. La correction du lecteur est
antérieure au gel. Quatre modes à budget0 passent en préflight normal/−O.

Les sondes de développement et la projection dense ont servi à choisir
la campagne, pas à revendiquer un temps de tour. Aucun repli dense à
8k/16k/32k n'a été lancé : son minimum de lectures annoncé est calculé
depuis les familles réellement survivantes, non chronométré.

GCP non utilisé. Aucun build ni reçu antérieur n'a été écrasé.

Contrôle de mise en index : `git diff --cached --check` relève une ligne
blanche supplémentaire en fin de `dense_forecast/run_dense_forecast.py`.
Cet auxiliaire est déjà empreinté par sa capture close165 sources :
il est conservé octet pour octet, avec cette remarque de style explicite,
sans réécrire les preuves pour supprimer un avertissement non fonctionnel.
Ce n'est pas un échec des compilations strictes ou des tests géométriques.
