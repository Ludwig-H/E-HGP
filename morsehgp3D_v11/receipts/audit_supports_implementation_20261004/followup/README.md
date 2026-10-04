# Correctif S6 recoupé avant publication

L'acteur S6, toujours base f98aeed67, a ajouté `p > kMaxInterior` avant
`p+q` dans make_shape. kMaxInterior=11 et q≤4 donnent p+q≤15 ; la
comparaison restante à K+1 impose p≤K−1. Le correctif est favorable.

Source counts.cpp SHA98f7a8c95e2c3e752e6b9d5cb2ef47314da79d5e821932735c29ad4ff67bd1fb.
La porte native refuse maintenant p=UINT32_MAX et conserve la frontière
valide (11,24,2,12). Les fichiers sont capturés et hachés avant/après cette
recoupe. Aucun natif n'est exécuté : ce n'est pas une qualification.

La capsule qb initiale, les 529 gardes et son inventaire restent inchangés.
Elle prouve l'ancienne faiblesse ; cette recoupe empêche de la publier comme
un problème encore ouvert. La garde du cast S3 reste absente à cette date.
