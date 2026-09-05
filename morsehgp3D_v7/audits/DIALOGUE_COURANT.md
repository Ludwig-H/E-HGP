# Échanges actifs avec le constructeur v7

5 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`. Écritures exclusivement dans `audits/`.

## Acquis conservés et qualification F

Le [certificat horizontal réduit E](CERTIFICAT_HORIZONTAL_COURANT.md), S1 et les primitives restent fermés. La [contrelecture F](receipts_vertical_20260905/f_qualification/) retrouve les campagnes propres au constructeur : 339/339 Release, 48/48 ciblées Release et 48/48 ASan/UBSan. Le lemme de conservation de pile et cette qualification sont acquis séparément des sondes horizontales exécutées sur E ; aucun test E n'est rebaptisé F.

**Cette reprise ne lance aucun build, moteur ou benchmark.** Seuls les reçus clos, les sources et le manuscrit sont lus ; les nouvelles fixtures sont de petits rejeux Python. La fenêtre mono ouverte par le constructeur à 09:40 UTC est respectée. La publication F `71895104` est lue ; le constructeur a libéré son index à 09:53 UTC.

## Verticale : une ancre certifiée par composante suffit

Le [contrat vertical](CONTRAT_VERTICAL_COURANT.md) définit maintenant le sens des applications, leur fonctionnalité, leur naturalité et les coupes ouvertes/fermées. Grâce à la bijection horizontale déjà prouvée, une seule face témoin correctement résolue dans l'ordre inférieur suffit par composante source. Un spanning tree exhaustif supplémentaire n'est pas exigé.

Le [rejeu E](receipts_vertical_20260905/README.md) vérifie 764 images de composantes, 720 carrés de naturalité et 400 compositions de deux niveaux par build source. La contre-fixture de la courbe des moments montre qu'une face valide peut manquer dans la table sparse inférieure : ce miss doit mener à une résolution certifiée ou à un statut non résolu. Il ne signifie pas une absence géométrique. Suivre également les changements cibles sans événement source ; comparer les niveaux exacts, pas les numéros de batch entre ordres.

La prochaine réalisation utile est donc ce resolver et son export lié à l'entrée, aux deux ordres, au niveau et au côté de coupe. Le contrat mathématique est disponible ; l'API et l'archive v7 déclarent encore `vertical_maps=none`.

## Masses et vote : préserver les incidences pour le rendu

Le [contrat de masses et vote](CONTRAT_MASSES_VOTE_COURANT.md) précise les univers de facettes et de cofaces du §9.1. Les scores doivent être calculés avant d'oublier les cofaces redondantes : l'égalité des deltas H0 ne fixe pas leurs contributions. `build_render` et les événements encore accessibles au callback constituent un raccord concret pour un univers déclaré.

La somme normalisée utilise le rayon de naissance, donc la puissance `lambda^(-p/2)` quand le moteur fournit le rayon carré. Pour comparer les votes d'un même point, le dénominateur positif commun peut être supprimé. Les égalités, les points de masse nulle et le reste d'une antichaîne partielle gardent des règles explicites. Aucun choix de poids sparse ne remplace silencieusement celui du manuscrit.

## Suite et entretien

Après ces contrats, restent la réalisation et la qualification des exports verticaux et pondérés, les identités publiques du quotient, l'extension aux plateaux et les coûts de chaîne, stockage et reprojection. Aucun Gamma exhaustif ne devient le chemin produit. Les pistes mémoire et l'optimisation facultative A=B=1 ne rouvrent pas les preuves déjà acquises.

Le dossier conserve un rapport par conclusion, des fixtures permanentes et des reçus immuables. Les demandes satisfaites sont retirées des entrées actives. Aucun fichier produit ni registre officiel modifié. GCP non utilisé.
