# Catalogue natif : capacités, deux passes et transaction

2 octobre 2026. Lecture statique de **f391bf13e1a9a982025bde86fc9219b5b7430afc**, 48 sources/documents extraits du commit avant lecture. Aucun build, test produit, allocation massive ou nouvelle session GCP. Les prédicats q2/q3/q4 sont relus par un autre auditeur. Ce reçu ne qualifie pas le catalogue ni ses performances.

**Conclusion : aucun défaut concret nouveau établi dans ce périmètre.** Les gardes sont bien situées ; les coexistences mémoire ci-dessous sont à considérer avant d'interpréter les deux passes comme une petite mémoire de sortie.

## Contrats positifs

- [Catalogue](sources/morsehgp3D_v11/src/catalogue/catalogue.hpp#L45) possède ses propres Buffer, expose des vues const et interdit copie/affectations. Il ne conserve aucun pointeur Cloud, mais ses SiteIdx exigent explicitement le même Cloud pour leur interprétation. Cloud/params doivent rester stables pendant l'appel synchrone ; le budget a un pilote unique. Des poids non unitaires sont refusés avant allocation (catalogue.cpp:16–21).
- [generate](sources/morsehgp3D_v11/src/catalogue/assemble.cpp#L33) utilise le même workspace réinitialisé par feuille, le même Cloud et la même politique déterministe pour count et fill. Le remplissage vérifie les capacités **avant** toute écriture (catalogue.cpp:49–60), puis B, P et toute la ledger des deux passes doivent coïncider (assemble.cpp:49). Aucun préfixe n'est rendu sur refus.
- Le tri par tas est sans allocation, sur niveaux exacts puis support canonique. [finish](sources/morsehgp3D_v11/src/catalogue/assemble.cpp#L73) conserve les anciens tableaux jusqu'à fin de copie, contrôle bornes des populations avant lecture/écriture et les offsets/rangs terminaux après remplissage. Un plateau exact partage son rang. Ces éléments ne remplacent pas la preuve géométrique de complétude.
- checked_add et add_bytes gardent les sommes/produits avant calcul. B<ball_limit≤kNone protège tous BallIdx ; L≤kNone, zéro compris, protège le rang maximal L−1<kNone. Les offsets sont u64. Aucun cast u32 non protégé trouvé sur le chemin public ; K≤12 protège Order. Les tailles feuille/masques sont bornées à1024/16mots.

## Coexistences mémoire exactes

Noter n sites (poids unitaires), c=min(n,max_leaf), B boules, P incidences I+U, L niveaux zéro compris ; **E=sizeof(Emission), C=sizeof(CatalogueBall), S=sizeof(Level), A=sizeof(Point)**. Ne pas remplacer ces sizeof par une somme des champs sans l'alignement, en particulier B18 où Level contient i128.

Workspace : **W=(A+8)c + 8c ceil(c/64)**. Les listes DFS allouent la taille du parent, pas seulement le count conservé. Racine et ancêtres restent vivants pendant la descente. Si T est leur maximum simultané, la profondeur prouvée donne **T≤4n(3b+2)**, b largeur du profil. C'est une borne conservatrice, sans prétention d'atteinte sur une famille ni de complexité du nombre total de boîtes.

Les maxima propres de l'appel réussi sont :

- génération fill : **W+T+EB+4P** ; count n'a pas encore EB+4P et est dominé par cette phase ;
- assemblage : **EB+4P+CB+SL+8(B+1)+4P** ; populations temporaire et finale coexistent ; workspace et DFS sont déjà rendus ;
- résultat persistant : **CB+SL+8(B+1)+4P**.

Donc, pour U octets antérieurs stables, le pic final est **max(pic_initial, U+max(génération fill, assemblage))**. Les locaux de taille fixe et la pile ne sont pas des Buffer et ne figurent pas dans ce compte, pas davantage les métadonnées d'allocateur/RSS. Le tri exact est payé en plus du travail des deux générations ; le maximum de profondeur ne borne pas leurs visites ou sorties.

**Amélioration utile :** après count, enregistrer T effectivement requis et admettre ensemble EB+4P et le scratch de la seconde génération. L'actuel admit(EB+4P), assemble.cpp:40–48, admet seulement les tableaux de sortie alors que le second walk réallouera encore ses listes. Les refus actuels restent corrects et transactionnels ; cette amélioration permettrait de refuser plus tôt après un count coûteux et de publier un modèle de phase précis.

## Branches et portes : portée de cette lecture

La singleton passe par deux générations vides puis produit le seul niveau zéro, avec zéro boule et CSR offset0 ; finish n'exécute aucune copie de population nulle. Les seuils de ressources ne sont pas justifiés par un grand tableau artificiel :

- cube `{0,2}³`, K1, leaf/max_leaf4 : ses huit coins sont équidistants au centre `(1,1,1)`, présent sur la fermeture de la boîte terminale adjacente ; aucune dominance stricte sur cette fermeture ne peut éliminer un coin. Une liste trop large peut donc atteindre wide_leaf ;
- le même cube et max_nodes1 forcent un second nœud après la racine, donc node_budget ;
- trois points alignés et ball_limit2 rencontrent une deuxième émission et refusent avant publication.

Ce sont des arguments de chemin statiques et des attentes écrites dans [unit.cpp](sources/morsehgp3D_v11/tests/catalogue/unit.cpp), **pas de nouveaux tests exécutés**. Les branches invariant/overflow servent aussi de gardes internes ; on ne prétend pas les atteindre sur une vraie géométrie à partir du seul test scalaire checked_add.

Les portes écrites couvrent déjà budget occupé, déplacement, égalité des catalogues répétés, budgets pic−1/pic et [chaque allocation](sources/morsehgp3D_v11/tests/catalogue/fault.cpp#L34) observée sur un octaèdre, restitution au niveau initial et reprise. L'injection octaèdre ne couvre pas automatiquement toutes les formes de DFS. Sur G4, ajouter une fixture subdivisée pour la restitution d'ancêtres et une fixture sans boule positive renforcerait ces chemins ; aucune réussite native de ces ajouts n'est revendiquée ici.

## Prochain raccord propriétaire

L'identité du Cloud est une précondition explicite, sans tag conservé dans Catalogue. Un futur build_tower/index doit lier ou vérifier le même propriétaire ; les types SiteIdx seuls ne distinguent pas deux nuages. Le déplacement conserve les spans mais vide l'objet source : une référence à cet ancien objet ne suit pas la propriété. Cela reste une obligation du futur raccord, pas un défaut d'une tour v11 déjà livrée.

SOURCE_BEFORE/AFTER, le lecteur de capsule normal/−O et SHA256SUMS ferment les copies réellement lues. Aucun produit, note active ou ancien reçu n'est modifié.
