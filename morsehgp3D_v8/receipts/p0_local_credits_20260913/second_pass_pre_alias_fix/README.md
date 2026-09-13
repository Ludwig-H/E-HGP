# Deuxième passe : copie du propriétaire fermée, alias d'entrée encore ouvert

Captures r2 conservées intactes : 729 exécutions nominales, 513 configurations,
huit CTests GCC et huit Clang ASan/UBSan réussis. Ces tests ne détectaient pas
la mutation par un pointeur conservé vers le vecteur déplacé en entrée.
Le [contre-exemple indépendant](../../../audits/P0_INPUT_ALIAS_CHECKS.json)
montre ce défaut ; il ne dit pas que la sonde l'exerçait dans ces captures.

La copie privée avant certification de r3 change les sources et le coût
de préparation. Ces mesures ne qualifient donc pas r3 et ne doivent pas
être mélangées à ses captures. Le fichier QUALIFICATION.json consigne les
tests r2 effectivement passés avant cette découverte, pas l'immutabilité
finale. Les chemins originaux dans les reçus restent historiques.

GCP non utilisé ; aucun contrat de tour FULL qualifié.
