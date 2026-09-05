# Raccord suivant FULL : portails depuis les catalogues fournis

5 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

**Le raccourci proposé est mathématiquement correct sous les prémisses ci-dessous.** Cette revue porte sur la section « Raccord suivant : FULL relatif aux catalogues Gabriel fournis » de la [coordination](../../../audits/COORDINATION_MORSEHGP3D_V7.md), à partir de sa ligne 658. Sa capture textuelle exacte et ses pins sont dans [portal_next_step_review.json](portal_next_step_review.json). Aucun code du futur `full_gabriel.hpp` n'est qualifié ; aucun build n'a été exécuté.

## 1. Seuls les retraits essentiels demandent un ancien parent

Soit D une directe Gabriel de cardinal K+1 et de niveau a, dont la coquille globale est exactement le support essentiel U. Le centre est dans l'intérieur relatif de conv(U), U est affinement indépendant et $2\leq |U|\leq4$ pour des positions distinctes.

Pour $u\in U$, la facette $F=D\setminus\lbrace u\rbrace$ vérifie β(F)<a : aucun sous-ensemble propre de U ne porte encore le centre de la boule optimale. F appartient donc à une composante FULL strictement antérieure au lot a, isolée ou non. Ce sont les seules facettes à résoudre vers les anciennes racines.

Pour un intérieur v de D, retirer v conserve U et donc exactement la même miniball. La facette D privée de v naît géométriquement à a ; elle ne peut appartenir à la coupe stricte. Elle n'est pas un minimum FULL, car v est un intrus strict de sa boule. Elle peut être installée comme alias du groupe courant sans recalcul de MEB. Deux directes distinctes du même lot ne peuvent partager une telle facette : l'égalité des miniballs donnerait le même saturé global, donc la même directe.

Le traitement des groupes reste atomique. Les anciennes racines sont figées à a− ; les alias découverts à a sont publiés après la décision du lot. Toutes les facettes utilisées et l'ancre de chaque directe doivent être conservées, même si son groupe n'émet aucune multifusion. Les feuilles Gabriel nouvelles du lot sont disjointes de ses connexions. Ces conclusions utilisent les [lemmes FULL](../receipts_gabriel_20260905/full_proof_review.md), §§2–3 ; K1 utilise ses points déjà présents à zéro, et K=n conserve sa feuille sans nécessiter de connexion n+1.

## 2. Pourquoi une facette stricte inconnue a deux intrus

Supposons que tous les minima et toutes les directes de niveaux <a ont été traités, et que leurs alias n'ont pas été perdus. Pour une facette stricte F inconnue, on calcule sa MEB régulière, de niveau b<a, puis son census global de bord et ses intrus stricts étrangers à F.

S'il n'existe aucun intrus, F est un minimum Gabriel de cardinal K ; son entrée de niveau b devait déjà être connue. S'il existe exactement un intrus z, F+z est une directe Gabriel de cardinal K+1, de même niveau b, qui devait déjà avoir installé F. Ces deux cas contredisent donc l'invariant de table et l'autorité des catalogues complets. Ils doivent produire un refus explicite si rencontrés sur une F stricte inconnue ; ils ne justifient pas de fabriquer un parent dans le lot courant.

Il reste au moins deux intrus distincts z,w. Le helper historique [intruders](../../src/forest/silent_incidence.hpp), lignes 294–335, collecte au plus deux témoins, mais achève la recherche de coquille : son succès exclut les points étrangers sur le bord. Un résultat `count=2` signifie **au moins** deux intrus, pas exactement deux. Après ajout de z, w reste un témoin ; aucun nombre total d'intrus ne se déduit de ce compteur tronqué.

## 3. Le premier pas économise réellement une MEB

Posons Q=F∪{z}. Puisque F⊂Q⊂B(F) et que z est strictement intérieur, la MEB, le niveau et le support essentiel de Q sont exactement ceux de F. La coquille globale est inchangée. Le certificat de B(F), son census de bord réussi et le témoin w certifient donc Q sans nouveau calcul de MEB ni nouvelle recherche d'intrus pour Q.

Choisir $u\in U\subset F$ et former $R=(F\setminus\lbrace u\rbrace)\cup\lbrace z,w\rbrace$. Les identités z,w sont distinctes et étrangères à F, donc |R|=K+1. Tous ses points sont dans B(F), et ses points de frontière sont exactement U privé de u. Si β(R)=b, l'unicité de la MEB imposerait que le centre soit dans conv(U privé de u), en contradiction avec l'essentialité. Ainsi **β(R)<b<a**. Il n'est pas nécessaire que les intrus appartiennent déjà à la MEB de F privé de u.

F est reliée à Q au niveau b. Q et R partagent la K-facette $(F\setminus\lbrace u\rbrace)\cup\lbrace z\rbrace$. Les deux cofaces sont actives avant a : une descente depuis R retrouve donc la composante de F à a−. Retirer un intérieur à la place d'un essentiel ne donnerait pas cette décroissance stricte.

Les pas suivants retirent de même un essentiel et ajoutent un intrus strict, avec validation de chaque nouvelle MEB et de son bord. Les niveaux décroissent strictement parmi un ensemble fini de cofaces : la descente termine mathématiquement sur le domaine régulier, sans énumérer cet ensemble. Une terminaison sans intrus doit correspondre à une directe du catalogue, à un niveau strictement inférieur à a. Son ancre historique, même issue d'une continuation sans événement, est normalisée vers sa racine courante à a−. Le seul niveau numérique ne suffit pas à identifier cette directe : son label et sa géométrie doivent correspondre à l'entrée autorisée.

## 4. Domaine et travail à conserver

La régularité des seules directes fournies n'implique pas celle de toutes les MEB F ou R visitées. L'[extension de fenêtre](../receipts_gabriel_20260905/full_proof_review.md), §5, prouve que certains blocs irréguliers hors fenêtre sont inertes ; elle ne certifie pas un chemin particulier qui les traverse. La réussite du nouveau parcours reste conditionnée aux contrôles locaux effectivement exécutés. Un refus de bord ou de budget n'invalide pas la preuve de graduation FULL et ne reçoit pas automatiquement la qualification des anciens chemins E/F.

L'économie prouvée concerne la MEB et la requête d'intrus redondantes de Q=F+z. La MEB de F, son census achevé, la première MEB de R et les pas suivants restent à charger. Au plus quatre facettes par directe demandent un ancien parent ; cette borne ne limite ni la longueur des descentes ni le nombre total de MEB. Former les autres labels, stocker leurs alias, chercher les terminaux et normaliser les successeurs ont aussi un coût.

Les plafonds et compteurs du nouveau calendrier doivent donc distinguer appels physiques, candidats MEB, requêtes spatiales, pas de descente et stockage. L'état des helpers reste persistant à l'échelle de l'ordre/tentative ; aucun nouvel appel de portail ne doit remettre leurs dépenses à zéro. Omettre une coface du journal final ne rend pas son travail gratuit. Le protocole annoncé, qui sépare ces budgets de ceux du `run()` F historique, est cohérent avec cette exigence.

La prochaine qualification utile est celle annoncée : lots FULL et parents stricts produits à partir des catalogues autorisés, confrontés à un oracle indépendant borné, avec rejets et plafonds effectifs. Cette revue n'exige aucun catalogue Gamma global dans le chemin produit et ne préjuge pas des résultats du futur composant. Aucun benchmark, mutation Git ou GCP.
