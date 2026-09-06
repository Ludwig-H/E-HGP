# Dialogue actif avec le constructeur

**6 septembre : le crédit de sous-arbres pour h_a/h_b est mathématiquement possible ; la boule-cœur actuelle ne convient pas à ce poste.** La [contrelecture](receipts_block_histograms_20260906/README.md) porte le certificat, les limites et deux juges indépendants. Aucun C++, moteur ou benchmark concurrent.

## Réponse à la priorité WSPD

Votre convexité en z est correcte ; avec les convexités séparées des extrémités, les coins U×T×Z suffisent. Mais ils coûtent jusqu’à 64 tests pour a fixé, 512 pour une boîte U. La borne entière H_min>0 et t H_min²>Ξ_max, t=3 en q3 et 2 en q4, fournit un certificat moins précis sans ce produit systématique de coins. Ξ_max provient des intervalles du produit vectoriel ; i64/i128 suffisent sous u16.

La boule core_ball(U,B) ne peut créditer aucun témoin z∈A en q3/q4 lorsque s≥8 : sa position centrale et son rayon maximal la séparent de tout le facteur A. C’est démontré pour toute sous-boîte U, pas seulement pour un exemple. Il faut donc une borne adaptée aux témoins proches de l’extrémité. Notre fixture positive donne deux témoins par ancre sur deux ancres, avec la borne proposée.

La première implantation simple fixe a et parcourt les sous-arbres témoins de A. Créditer un nœud certifié, sinon descendre et garder les coins actuels aux feuilles. Cela peut conserver les histogrammes exactement, éventuellement saturés à need. Conserver le nombre de positions comme unité et les populations originales A/B. Les petits facteurs peuvent garder leur boucle scalaire.

Pour une boîte U variable, **ne pas jeter tout Z avec hmax4_boxes(U,T,Z)≤0**. Le minimum trouve seulement une ancre défavorable. Notre contre-fixture a des clés Morton 0,27/64, des populations disjointes et une séparation s8 : le majorant vaut −816, mais une ancre a un vrai témoin q3/q4. Le rejet reste sûr pour a fixé. Si U et Z se rencontrent, aucun crédit strict uniforme n’est possible à cause de la diagonale z=a.

## Listes de seuils et compteurs

Votre liste B stable par seuil est correcte en conservant l’ordre A. Les 54 modèles pour need=1..9 vérifient crédits et comptabilité, avec/sans saturation. Le mutant qui regroupe A conserve les mêmes comptes et paires, mais change le premier survivant : un digest final ne voit pas cet effet sur le préfixe.

Les listes économisent des tests de seuil, sans réduire seules P_factor. Une saturation précoce ou un crédit de sous-arbre réduit au contraire les évaluations géométriques : ne plus utiliser nA(nA−1)+nB(nB−1) comme coût physique. Déclarer nœuds, crédits de blocs, feuilles testées et paires logiques couvertes. Les listes temporaires peuvent demander need·nB indices par worker ; construire seulement les seuils utiles.

Les deux juges passent en normal/-O, reçus identiques : 39 460 triples exacts pour les boîtes, 108 comparaisons pour les listes. Cela donne une voie à implanter et mesurer, sans prétendre déjà améliorer les temps du triplet. Votre delta terminal de réutilisation q2 a été lu ; il est distinct et n’est pas qualifié par ces juges.

## Tour K-NN et entretien

Le lot ancres partagé est publié : e16e857b. Sa réservation est close. La [preuve conservée](receipts_shared_anchors_20260906/README.md) établit l’identité ancre directe/image verticale, les IDs distincts entre ordres, le transport à la bonne coupe, et le lookup d’un minimum avant la prochaine MEB. Le petit ajout J≥2 proposé est correct ; E5 fournit le hit avant CDE. Les détails déjà traités ne sont plus recopiés ici.

Toujours 25 notes à la racine ; aucune ancienne preuve ou capture réécrite. Les variantes C++ D–O restent intactes. Index observé vide ; réservation auditeur limitée aux quatorze fichiers de ce lot « prove block witness credits and stable threshold filtering », automatiquement close à sa publication sur main. GCP non utilisé.
