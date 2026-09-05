# Alias FULL résolus à la demande

5 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

**Oui : les minima, les ancres de toutes les directes et leurs successeurs suffisent ; les autres alias peuvent devenir un cache facultatif.** La proposition « Question suivante motivée par les volumes d'alias » de la [coordination](../../../audits/COORDINATION_MORSEHGP3D_V7.md) est correcte sous le contrat suivant. Sa capture exacte, les sources lues et les fixtures analytiques proposées sont dans [lazy_alias_next_step_review.json](lazy_alias_next_step_review.json). Aucun code nouveau, build ou résultat compilé n'est attribué à cette preuve.

## 1. Autorité permanente et cache dérivé

Conserver les deux catalogues Gabriel complets, exacts et réguliers de cardinal K et K+1, la correspondance de chaque minimum avec son nœud de naissance, l'ancre de **chaque** directe après fermeture de son lot, puis les successeurs historiques nécessaires à leur normalisation. Les ancres des directes sans fusion sont conservées elles aussi. Une liste de labels de minima sans moyen de retrouver leurs nœuds ne suffit pas à ce contrat.

Les seules demandes de parents sont les facettes F obtenues par retrait d'un essentiel d'une directe D du lot a ; elles vérifient β(F)<a. Retirer un intérieur conserve B(D), donc produit une facette de niveau a qui ne peut être un parent strict. Deux directes régulières distinctes du même lot ne partagent pas de facette égale. La [preuve précédente](../receipts_full_cpp_20260905/portal_next_step_review.md) ferme ces faits.

Il est donc possible de supprimer la seconde passe d'installation systématique des K+1 facettes, **en conservant la fermeture du lot et l'ancrage de toutes ses directes**. Les demandes strictes effectivement résolues peuvent être mémorisées. Un cache ne fait autorité que pour accélérer une résolution déjà justifiée ; sa suppression ne retire ni minimum, ni ancre directe, ni successeur.

## 2. Dispatcher complet d'une demande stricte

Un minimum ou un alias connu fournit un jeton historique que l'on normalise vers la coupe a−. Sinon, calculer la MEB de F, contrôler β(F)<a et terminer le census global de coquille et d'intrus. La coquille doit être exactement le support essentiel. Le helper stocke au plus deux intrus : zéro et un sont alors des comptes exacts ; deux signifie au moins deux.

| Intrus étrangers stricts | Autorité à retrouver | Résolution |
| --- | --- | --- |
| J=0 | minimum Gabriel F, niveau β(F) | Retrouver son nœud, vérifier son niveau antérieur et normaliser son jeton. Une absence est un défaut de catalogue ou de liaison, pas une naissance tardive. |
| J=1, témoin z | directe D₀=F∪{z}, niveau β(F) | Chercher son label complet dans le catalogue, vérifier le niveau exact, l'ancre installée et l'antériorité stricte ; normaliser cette ancre. |
| J≥2 | terminal direct d'une descente stricte | Conserver la descente déjà prouvée, avec chaque nouvelle MEB, contrôle de bord, charge et contrôle terminal. |

Dans le cas J=1, F⊂D₀⊂B(F) et z est strictement intérieur : **B(D₀)=B(F)**, avec même support essentiel et même niveau. Le census achevé de F prouve que le saturé global est exactement D₀. Cette directe est donc unique et déjà traitée, puisque β(F)<a. F et ses autres facettes sont connectées dans Gamma FULL dès ce niveau ; son ancre normalisée donne la bonne composante pré-lot. Aucun calcul de MEB ni census de D₀ n'est nécessaire. L'égalité du seul rayon ne remplace jamais l'identité du label.

Pour J≥2, Q=F+z garde B(F), puis remplacer un essentiel par le second intrus produit une coface strictement plus petite. Les cofaces successives partagent une K-facette et restent avant a. Le terminal identifié dans le catalogue donne donc également la composante de F à a−. La finitude prouve la terminaison mathématique sur le domaine où les contrôles réguliers réussissent ; les plafonds peuvent produire un refus explicite.

Le refus initial J≤1 du producteur `e02d163c` correspond à son ancienne table contenant toutes les incidences directes. Il est correct dans ce contrat historique. La variante proposée change expressément cet invariant : un miss J=1 devient normal et doit emprunter la nouvelle branche. Supprimer les insertions sans modifier ce dispatcher serait un faux refus reproductible.

## 3. Lots, continuations et évictions

Avant toute union globale du lot a, normaliser tous les jetons utilisés dans l'état a−. Les résolutions et compressions de chemins peuvent enrichir le cache, mais elles ne créent aucune composante. La DSU du lot groupe ces anciennes racines ; les minima du lot reçoivent leurs IDs séparément, puis les multifusions sont installées atomiquement. Chaque directe du lot est enfin ancrée dans sa composante **fermée**, même si cette composante n'a créé aucun nœud. Cette ancre ne devient un parent d'un autre lot qu'après normalisation à sa coupe stricte.

Le cas J=1 ne peut viser une directe de niveau a : son niveau vérifié est β(F)<a. Il ne réintroduit donc ni parent créé dans le même lot ni ordre binaire artificiel entre directes simultanées. Une continuation muette conserve sa composante abstraite ; l'ancre de la directe reste parfaitement valide sans événement public associé.

L'induction sur les lots ferme la complétude : si les anciennes racines sont correctes, chaque demande stricte retrouve sa composante par l'une des trois branches. La DSU reçoit donc les mêmes parents que la construction conservant tous les alias ; les minima et multifusions publiés sont les mêmes, à renommage des IDs près. Omettre des alias égaux ne change ni cet état fermé, ni les coupes intermédiaires du certificat FULL, ni les unions de points portées par ses feuilles.

Une entrée du cache de facettes strictes peut être évincée : au prochain usage, le dispatcher reconstruit le même rattachement depuis les autorités permanentes. Un cache de capacité zéro peut donc être correct, en calculant sans insertion. Cela suppose un **contrat de cache distinct** ; réinterpréter silencieusement l'ancien plafond cumulatif d'installations comme une capacité remboursée à l'éviction serait incorrect. Les minima et les ancres directes ne doivent pas disparaître avec ce cache. Une suppression de leur stockage exige un autre resolver certifié, absent de cette proposition.

La régularité de toutes les boules du nuage n'est pas nécessaire à cette preuve de graduation. En revanche, les MEB et requêtes réellement exécutées gardent leurs contrôles. Une réussite historique avec moins de misses ne prouve pas que les nouveaux compteurs respectent les mêmes plafonds. Une autre politique de choix d'intrus ou d'essentiel ne reçoit pas automatiquement le domaine des anciennes descentes. Les refus restent transactionnels, sans préfixe de forêt publié.

## 4. Fixture J=1 réalisable et rejets proposés

Prendre les quatre points u16 A=(0,5,0), B=(4,5,0), C=(2,6,0), W=(2,0,0), d'identifiants 0,1,2,3. Les minima K2 sont AC et BC au niveau 5/4, puis AW et BW au niveau 29/4. Les directes K2 sont ABC au niveau 4, de support AB, puis ABW au niveau 841/100, de support ABW. C a une puissance strictement positive 6/5 dans la boule de cette dernière directe. Les autres triangles ACW et BCW ont la boule CW, de niveau 9, avec respectivement B et A comme intrus : ils ne sont pas Gabriel.

ABC fusionne les minima AC et BC au niveau 4. Sa facette **AB**, obtenue par retrait de l'intérieur C, est égale au niveau de la directe et son alias peut être omis. Lors de la directe ABW, AB est désormais une facette stricte inconnue ; son unique intrus est C. La branche J=1 retrouve ABC, réutilise B(AB), puis fournit le parent ancien correspondant à {AC,BC}. Les deux autres parents sont les minima AW et BW. Le lot 841/100 est donc une multifusion à trois parents, sans nouvelle feuille AB.

Attention au choix de la fixture : **AC n'est pas la facette J=1 de ce triangle obtus**. AC est un minimum, donc relève de J=0 si la table de minima est recherchée après le census. Les niveaux et puissances de cette fixture sont des dérivations analytiques inscrites dans le JSON ; aucune nouvelle exécution n'est annoncée.

Pour tester séparément le stockage des ancres, une petite fixture structurelle peut donner à une directe muette le jeton historique r, faire fusionner r avec une autre racine au niveau c, puis demander un rattachement au niveau a>c. L'attendu est le successeur courant, jamais r. Cette fixture teste le protocole des jetons ; elle ne prétend pas être un contre-exemple géométrique réalisé. Les fautes ciblées proposées sont : garder le refus J≤1 ; chercher un terminal par rayon seul ; accepter un terminal du même lot ; oublier une ancre muette ; omettre la normalisation ; évincer une autorité permanente ; ne pas facturer un miss après éviction.

## 5. Coût et décision constructive

Avec D directes, le nombre V de demandes strictes vérifie V≤4D. Le cache contient au plus les facettes strictes distinctes effectivement demandées, plus la structure séparée des minima ; avec capacité explicite C, sa résidence propre est au plus C. Les D ancres directes et l'histoire des nœuds restent nécessaires au protocole annoncé. Le nombre d'alias systématiques évités n'est pas une borne de gain mémoire total.

Pour une exécution réussie, si M demandes passent réellement par MEB+census et S remplacements sont exécutés par les branches J≥2, le calendrier proposé demande **M+S appels MEB**. J=1 n'ajoute aucun appel pour D₀. Les census, recherches de catalogue, lectures et compressions de successeurs, insertions et évictions restent à compter. Les budgets des helpers persistent sur tout l'ordre/tentative ; un cache vidé ne remet aucune dépense à zéro. Recalculer après éviction peut augmenter le temps et provoquer un refus plus tôt.

La proposition lève donc le besoin mathématique de conserver toutes les facettes égales. Elle n'établit aucun gain de latence, de RSS ou de tour intégrée. Le prochain delta devra versionner la politique de cache et les nouveaux cas de résolution, puis qualifier ces branches avec des contrôles positifs et fautifs. Aucun Gamma exhaustif n'est requis dans le chemin produit ; aucune modification des octets de la campagne courante n'est demandée.
