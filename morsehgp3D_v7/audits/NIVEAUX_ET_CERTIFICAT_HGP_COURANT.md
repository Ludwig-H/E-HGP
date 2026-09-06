# Certificat HGP FULL : décision indépendante

6 septembre 2026. Parties I et II du manuscrit lues intégralement, PDF 35–134. Cadre : `exploration_v7_hors_registre`, `cpu_reference`, `quantized_u16_input_only`, `audit_independant_math_and_architecture`, `public_status=not_claimed`.

**Sous les prémisses régulières déclarées, FULL conserve les minima Gabriel de cardinal K et les vraies multifusions aux niveaux Gabriel de cardinal K+1, avec leurs parents.** K+1 est la cardinalité des cofaces de connexion, pas le nombre de parents. Les identités des composantes persistent malgré le recouvrement de leurs points ; leur couverture se dérive des feuilles.

La [note principale](../docs/AUDIT_NIVEAUX_GABRIEL_20260905.md) porte désormais l’exposé. Les [contrelectures indépendantes](receipts_gabriel_20260905/README.md) conservent les arguments complets, fixtures et modèles. La présente entrée garde les distinctions nécessaires à leur emploi.

## Suffisance et domaine

Une facette Gabriel régulière n’a aucun point étranger dans sa boule fermée : par unicité de la MEB, aucune extension ne naît au même niveau. Elle est donc une naissance isolée. Une facette non-Gabriel F possède un intrus strict z : F+z apparaît à son niveau et touche des facettes strictement antérieures. F ne crée aucune racine. Avec un seul intrus, F+z peut être directe et fusionner plusieurs parents : l’apex unique concerne les **cofaces non-Gabriel**, pas toute facette non-Gabriel.

Les retraits essentiels d’une coface régulière fournissent au moins deux facettes strictes couvrant ses points. Une continuation FULL n’ajoute donc aucun point. Le lemme d’attaches silencieuses et sa confluence de plateau ramènent les cofaces non-Gabriel à un seul ancien parent ; elles n’introduisent aucune vraie fusion. Les [preuves FULL](receipts_gabriel_20260905/full_proof_review.md) et [de confluence](receipts_gabriel_20260905/level_proof_review.md) justifient l’induction, y compris les niveaux égaux.

Le rejeu active les feuilles, puis contracte atomiquement les groupes de parents lus avant chaque lot. Avec L feuilles, I multifusions et R racines, `I ≤ L−R` et le nombre de liens vaut `L+I−R`. Le stockage est linéaire en minima, avec O(KL) identifiants de feuilles ; L n’est pas borné par n aux ordres supérieurs. Les rangs Gabriel utiles sont 2 à `min(n,Kmax+1)` ; leur géométrie se partage entre ordres adjacents.

À K1, les points naissent à zéro, absent au côté ouvert et présent au côté fermé. À K=n, X est une feuille née à sa MEB, sans coface ni fusion. Hors régularité, AB peut être Gabriel faible et naître avec ABC : ce plateau ne justifie pas une feuille isolée persistante. Le [négatif permanent](receipts_gabriel_20260905/full_proof_review.md) conserve ce cas.

## Extension non régulière : décision désormais constructive

Le [contrat principal](../docs/PLATEAUX_FULL_ET_ANCRES.md) reprend le quotient de coquille, les lots atomiques et les ancres A[K,BallKey]. Les [preuves initiales](receipts_plateaux_full_20260906/README.md) et le [modèle à ancres](receipts_plateaux_full_20260906/BALL_ANCHORS.md) restent leurs témoins indépendants. La fenêtre p+q_min≤smax est conservée ; une ancre n’est pas limitée aux nœuds publics. Les parents locaux sont résolus dans l’état global avant le lot, et les naissances supérieures prennent l’ancre inférieure fermée de la même boule.

Hors régularité, une naissance peut couvrir plus de K points et une continuation gagner des points. Le [complément de certificat](receipts_plateaux_full_20260906/LOCAL_DIAGNOSTICS.md) permet d’encoder des contributions datées, éventuellement redondantes, puis d’en faire l’union exacte à la lecture. Il ne faut donc pas imposer au producteur le calcul de deltas disjoints ni d’ensembles de points par racine. Les contributions restent distinctes des nœuds topologiques et la borne I≤L−R ne borne pas leur volume. Les identités, la géométrie, la complétude et les poids ont toujours des autorités distinctes ; F n’est pas réinterprété FULL.

Les quatre coquilles réelles 50k sont maintenant disponibles et contrôlées localement. Elles donnent trois naissances géométriques et trois blocs à deux parents locaux dont les parents globaux restent à identifier ; aucune croissance de points dans ces derniers. Le C++ intégré et l’archive industrielle ne sont pas qualifiés par ces preuves. Les descriptions régulières ci-dessous gardent leurs prémisses.

## Sommets Gabriel et vraie hiérarchie K-NN

La [preuve et la tour bornée](receipts_gabriel_vertices_20260906/README.md) répondent à la question utilisateur du 6 septembre. Les seuls minima peuvent représenter exactement les composantes de L_K(r), leurs naissances, fusions et couvertures, avec des liens portant les seuils des chemins omis. Leur graphe induit est faux : quatre points u16 réguliers en dimension trois retardent une fusion de 169/9 à 41/2, même avec toutes les unions de la définition 21. Ce n’est donc pas un remplacement littéral des sommets de Γ ou de la définition 29.

Une descente de cardinal K est possible : remplacer un essentiel de la facette par un intrus strict diminue sa MEB et garde un chemin au niveau source. Le minimum terminal, normalisé dans l’ordre concerné, suffit à retrouver le parent. Cela ouvre un resolver sans ancre directe comme autorité horizontale, à qualifier séparément du C++ actuel. Le modèle sur K1..4 vérifie 76 coupes, couvertures et cartes verticales ; aucune performance n’est déduite. La génération et le census sont déjà partagés par la v7 entre les ordres. Les racines inférieures ne résolvent pas les parents supérieurs : ceux d’une même fusion ont déjà une image verticale commune avant celle-ci.

La [suite sur les ancres partagées](receipts_shared_anchors_20260906/README.md) précise l’économie de tour : aux rangs m≤Kmax, l’ancre directe inférieure est exactement l’ancre verticale de la feuille supérieure, même sans fusion. Sa valeur peut être partagée, sans confondre les tokens des deux ordres ni normaliser trop loin dans l’histoire. Un contre-exemple J1 défavorise le choix du premier essentiel ; tester les q≤4 retraits dans les minima retrouve ici un terminal sans seconde MEB. Un hybride peut conserver le J1 disponible puis appliquer ce filtre. Le calendrier, l’historique des coupes et la validité des ancres après refus restent à qualifier dans le produit.

## Taille des feuilles : le pire cas porte sur FULL

La [preuve indépendante à K fixé](receipts_probe_meb_review_20260906/full_output_growth.md) établit, pour chaque K≥2, m² minima sur N=2m+K−2 sites réguliers en dimension ambiante trois. Une boule Gabriel stricte interdit toute coface au même rayon par unicité de la MEB ; elle donne donc une feuille isolée, même si ses points sont partagés. Deux arcs rationnels assurent m² paires croisées, et K−2 ancres dans leur intérieur commun étendent ce résultat à chaque ordre fixé. Une perturbation rationnelle générique assez petite conserve ces inégalités strictes.

La sortie explicite peut donc être quadratique en N. K1 garde n feuilles ; le cas terminal K=n n’est pas une famille à K fixé quand N croît. La précision croissante de la preuve ne devient pas une asymptotique infinie u16 : ses nouveaux témoins entiers donnent seulement 9/25/81/289 feuilles K2 nommées aux tailles 6/10/18/34. Ni l’acceptation de tout leur nuage par le moteur, ni une performance ne sont qualifiées. Le [contrat de performance](../docs/CONTRAT_PERFORMANCE.md) garde ses jalons ; mesurer la croissance demande de distinguer volume de sortie et surcoût intermédiaire. Une API implicite demanderait son propre contrat.

## Ce que le certificat ne remplace pas

Les portails silencieux restent nécessaires au calcul des parents. La [contre-fixture E5](../../docs/math/INCIDENCES_SILENCIEUSES_GAMMA.md) réfute leur suppression brute, pas le certificat muni des bons parents. Aucun Gamma exhaustif n’est requis en sortie ou comme architecture.

Le [producteur relatif](PRODUCTEUR_FULL_GABRIEL_COURANT.md) et son [cache qualifié](CACHE_FULL_COURANT.md) ont leurs propres preuves C++. Un lecteur structurel ou un digest ne certifie pas la complétude du fournisseur de catalogues. F reste un objet réduit distinct.

Pour la verticale FULL, une feuille est une directe inférieure : conserver son ancre **après fermeture du plateau inférieur**, puis normaliser. Les minima FULL ne sont pas toutes les facettes contributrices de l’Algorithme 1 ; [incidences, masses et dates d’affectation](CONTRAT_MASSES_VOTE_COURANT.md) restent un supplément explicite. Le manifeste industriel doit lier entrée, métrique, ordres, horizon, coupe et succès terminal. GCP non utilisé.
