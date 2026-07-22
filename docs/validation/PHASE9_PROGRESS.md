# Progression Phase 9 — flux direct de supports H0

> [!IMPORTANT]
> Phase `9`, backend `reference_cpu`, profil `hgp_reduced`, mode `certified` pour les jalons CPU. La porte d'entrée est satisfaite par les Phases 1, 2A, 4 et 7. Les flux directs couvrent désormais séparément les paires et les supports de tailles trois--quatre; ils ne produisent encore aucune forêt, aucune réduction hiérarchique, aucun checkpoint commun d'arité deux à quatre et aucun `public_status`.

## Décision d'architecture après relecture du manuscrit

Les Parties I et II du manuscrit ont été relues avant l'implémentation. Elles confirment deux idées structurantes : la hiérarchie utile est portée par des changements de composantes de régions témoins, et le cas $k=1$ admet déjà une compression exacte par l'EMST sans matérialiser le graphe complet. La mosaïque de Delaunay d'ordre supérieur constitue une voie suffisante dans le manuscrit, pas une obligation architecturale pour le produit.

Le contre-exemple exact permanent à la réduction de Gabriel interdit toutefois de confondre un catalogue local de sphères critiques avec la hiérarchie publique complète. Le chemin retenu est donc : supports directs maintenant, journal Morse avec incidences silencieuses en Phase 10, puis preuve verticale et fermeture de M.1 avant toute promotion publique.

La Phase 5 reste une ancre $k=1$ non fermée et repasse à l'état `ready`. La Phase 8 est gelée comme oracle cellulaire $n\leq8$. La Phase 9 devient le chantier actif parce qu'elle ne dépend ni de la fermeture globale de 5, ni de l'extension de l'atlas top-$m$ de 8.

## Jalon 9.1-RCPU livré

Le producteur `build_exact_pair_support_stream` parcourt le self-produit du LBVH sans tableau de toutes les paires. Pour un nœud $A$, le produit diagonal est partagé en $(L,L)$, $(L,R)$ et $(R,R)$. Un produit croisé scinde déterministiquement le nœud de plus grande diagonale exacte, puis de plus grande cardinalité, puis selon l'indice de nœud. Les feuilles diagonales sont supprimées. Cette partition couvre exactement les $\binom{n}{2}$ paires non ordonnées.

Chaque unité est transactionnelle : elle résout tout un produit par prune, classe une paire feuille complète, ou remplace atomiquement le produit courant par tous ses enfants. Si un budget manque, le produit courant reste dans la frontière résiduelle. L'audit vérifie à tout arrêt l'identité `resolved_pair_count + remaining_frontier_pair_count == total_pair_count`.

### Prune exact de rang

Pour les boîtes supports $A,B$ et une boîte témoin $C$, le producteur calcule exactement le maximum de $\phi(x,u,v)=(x-u)\mathbin{\cdot}(x-v)$. Axe par axe, les huit choix d'extrémités suffisent : l'expression est convexe en $x$ et affine séparément en $u$ et $v$. Une borne strictement négative certifie que tout point de $C$ est strictement intérieur à toute boule diamétrale du produit $A\times B$.

Le parcours témoin accepte seulement des sous-arbres dont les plages Morton sont disjointes des plages supports. Comme un sous-arbre accepté n'est jamais redescendu, les reçus utilisés forment une antichaîne et leurs cardinalités s'additionnent sans doublon. Dès que cette union contient $s_{\max}-1$ points, chaque paire possède au moins $s_{\max}-1$ intérieurs stricts et son rang fermé est au moins $s_{\max}+1$; le produit est alors éliminé.

Lorsque le maximum n'est pas strictement négatif, une paire-ancre réelle et déterministe du produit fournit une seconde décision sûre. Si la distance minimale exacte de l'AABB témoin au milieu de cette paire est supérieure ou égale au rayon diamétral de l'ancre, aucun point du sous-arbre n'est strictement intérieur à cette paire réelle, donc aucun ne peut être témoin intérieur commun à toutes les paires; le sous-arbre est ignoré. L'égalité est bien excluante puisque l'intérieur est strict. Les produits diagonaux évitent entièrement la recherche témoin, dont le résultat négatif est connu par $u=v$, et une paire feuille passe directement à sa classification globale capée.

Une tentative intermédiaire fondée sur le minimax des coins indépendants des deux AABB supports a été falsifiée avant publication : des coins artificiels peuvent détruire les corrélations entre axes des sites réels. La fixture permanente `pair_aabb_corner_correlation` et son test Python interdisent désormais cette inférence; le registre des preuves décrit le contre-exemple exact.

Le backend CPU calcule directement les bornes rationnelles autoritatives. Il n'existe dans ce jalon ni proposition flottante, ni prune GPU, ni reçu device présenté comme décision exacte.

### Classification fermée sparse et capée

La requête existante `lbvh_closed_ball` n'est volontairement pas appelée : elle alloue un tableau de classe de taille $n$, matérialise tous les extérieurs et rescane le nuage pour chaque paire. Le nouveau parcours interne utilise les bornes exactes minimum et maximum du LBVH :

- `minimum > radius` compte tout le sous-arbre comme extérieur sans recopier ses identifiants;
- `maximum < radius` compte tout le sous-arbre comme intérieur et arrête dès que le nombre d'intérieurs dépasse $s_{\max}-2$;
- toute égalité descend jusqu'aux feuilles, où la classification exacte de sphère décide;
- une paire encore pertinente conserve au plus $s_{\max}-2\leq9$ identifiants intérieurs;
- un shell dégénéré est parcouru et compté entièrement, mais seul son plus petit identifiant extra-support est conservé.

Ainsi une cosphère de dix millions de points ne force jamais un `vector<PointId>` de dix millions d'entrées dans un diagnostic. Les extérieurs ne sont jamais matérialisés. Le nombre de références `PointId` émises possède un budget distinct du nombre de records.

### Budgets et statut scientifique

Les budgets séparent :

- unités de travail du produit et de la recherche témoin;
- frontière persistante de produits;
- frontière auxiliaire éphémère;
- records et références `PointId` émis;
- requêtes de boule fermée;
- classifications logiques de points.

Le préflight d'une paire feuille réserve conservativement un record et jusqu'à $s_{\max}+1$ références avant de connaître son issue exacte. Il peut donc refuser une paire qui aurait finalement consommé moins, voire aucun record après rejet par rang. Cette règle transactionnelle sûre n'est pas encore `work-conserving`; son assouplissement appartient au sink et au checkpoint de 9.3.

Une sortie `complete` exige la frontière vide, les partitions et prunes exacts, toutes les paires survivantes classifiées et l'identité de couverture fermée. Une requête rejetée dès que son cap d'intérieurs stricts est dépassé ne termine volontairement pas son shell; l'assertion publique porte donc précisément sur la complétude des shells encore pertinents pour le rang. Toute frontière non vide retourne `budget_exhausted`; elle ne certifie jamais l'absence d'autres supports. Le vérificateur reconstruit un résultat neuf depuis le nuage, le LBVH, $K_{\max}$ et le budget fiables, puis compare toutes les sorties, frontières, audits et assertions.

La projection historique `remaining_frontier` de 9.1-RCPU reste un reçu auditable seulement. L'API monolithique ne la réinjecte pas; le jalon séparé 9.3a ci-dessous porte désormais la reprise.

## Jalon 9.2a-RCPU livré

Le flux `build_exact_higher_support_stream` généralise le parcours aux tailles trois et quatre sans tuple ordonné. Une entrée contient au plus quatre groupes $(N_i,r_i)$ de plages Morton disjointes. La scission d'un groupe $(N,r)$ émet chaque répartition admissible entre ses enfants; l'unique entier $a=\lvert U\cap L\rvert$ prouve que ces enfants partitionnent le parent. Leur somme de cardinalités est vérifiée à chaque expansion. L'univers $\binom{n}{3}+\binom{n}{4}$, les produits résolus et la frontière restante sont comptés en `BigInt`; pour dix millions de points, le premier vaut exactement `416666583333329166667500000`, bien au-delà de 64 bits.

Pour un produit support, l'évaluateur rationnel forme les intervalles de Gram et Cramer, les numérateurs barycentriques homogènes et le polynôme de puissance à la sphère sans division. Une borne supérieure non positive sur la dépendance ou un numérateur exclut universellement le bon centrage. Une borne supérieure strictement négative du polynôme sur une plage requête certifie ses points strictement intérieurs à toutes les sphères affinement indépendantes du produit. Une antichaîne de cardinal au moins $s_{\max}-\lvert U\rvert+1$ exclut alors le produit par rang. Chaque reçu répète nœud, plage, cardinal et analyse exacte; le producteur vérifie disjonction des supports, antichaîne, somme et cohérence des polynômes avant émission.

La spécialisation triangle calcule le maximum continu de chaque produit scalaire d'angle par huit triples d'extrémités sur chaque axe. Pour la requête seulement, la convexité du polynôme en $x$ autorise les huit coins de son AABB. Elle ne s'étend pas aux boîtes supports : les fixtures permanentes montrent des extrémités toutes non aiguës avec un triangle aigu à l'intérieur, puis des puissances négatives aux extrémités avec une puissance positive au centre. L'intervalle complet reste donc l'autorité et toute ambiguïté descend.

À une feuille, `analyze_circumcenter_support` recertifie dépendance, réduction de frontière, centre extérieur ou support minimal. La requête de boule fermée LBVH ne matérialise pas la partition globale : elle compte les sous-arbres extérieurs, conserve au plus $s_{\max}-\lvert U\rvert$ intérieurs et descend toute égalité pour compter le shell complet. Un shell égal au support produit un événement; un shell supplémentaire rang-pertinent produit son cardinal et un unique témoin canonique hors support. Les huit budgets séparent travail, frontière persistante, frontière auxiliaire, records, références, reçus, requêtes globales et classifications. La frontière doit d'abord contenir les zéro, une ou deux racines initiales selon $n$; une capacité moindre est rejetée comme configuration incapable de représenter l'univers. Après cette précondition, un arrêt conserve la couverture exacte de la frontière et ne certifie aucune absence.

Le vérificateur reconstruit un résultat neuf depuis le nuage, le LBVH, $K_{\max}$ et le budget fiables, puis exige l'égalité de toutes les décisions, bornes, frontières et identités `BigInt`. Ce premier incrément reste monolithique : il ne sait pas encore réinjecter sa frontière, publier ses événements par chunks ou partager le protocole durable des paires. Les bornes de texte binary64 propres aux centres et niveaux des triangles et tétraèdres atteignent respectivement 6 973 et 9 503 octets pour le niveau; le cap pair v1 de 2 048 n'est donc pas réutilisé silencieusement. Le pire temps reste combinatoire lorsque les boîtes ne prunent pas; aucune revendication 50 k, 10 M+ ou SLO n'en découle.

## Jalon 9.3a-RCPU livré

Le manifeste compact encode par SHA-256 incrémental, sans chaîne temporaire proportionnelle au nuage, les trois mots binary64 de chaque `PointId` canonique. Un second digest couvre l'image sémantique du LBVH utilisée par le parcours : toutes ses feuilles et tous ses nœuds, leurs plages, extrema, enfants, codes Morton, racine, AABB racine et compteurs de construction. Le digest sémantique lie en plus $K_{\max}$, $K_{\mathrm{eff}}$, $s_{\max}$, le backend `reference_cpu`, le profil `hgp_reduced`, le mode `certified`, le schéma, la version de parcours et la base de preuve, y compris l'exclusion sûre par paire-ancre réelle. Le budget de chunk n'appartient pas à cette identité et peut changer à la reprise.

Le checkpoint conserve la frontière complète et au plus un produit actif, toujours égal au dos de cette frontière afin que sa couverture ne soit jamais comptée deux fois. Trois états sont possibles : recherche témoin, expansion du produit, classification feuille. Une recherche interrompue conserve sa pile DFS, les reçus stricts déjà acceptés, leur cardinalité exacte et, si nécessaire, un nœud interne déjà décidé dont l'expansion attend une capacité supérieure. La reprise ne recharge ni la visite du produit, ni un nœud témoin déjà décidé; elle refuse aussi un budget auxiliaire inférieur à la pile et au nœud d'expansion différée conservés dans le checkpoint en mémoire. Les reçus actifs sont recertifiés par la borne rationnelle $\max\phi<0$, par leur antichaîne et par leur disjonction des plages supports et des autres unités actives. Les liens minimaux entre curseur et audit sont revérifiés, ainsi que les identités exactes des partitions de bornes $\phi$, ancre et boule fermée.

Chaque appel retourne un candidat logique préparé en mémoire contenant les records du seul chunk, leur ordre inter-types et le checkpoint suivant. Les structures restent mutables; toute altération échoue toutefois au rejeu. Le chunk porte le digest exact de son checkpoint source; la chaîne de sortie applique SHA-256 record par record dans l'ordre déterministe du parcours et son digest terminal ne dépend donc pas du découpage. Si un sink abandonne le candidat avant publication, l'ancien checkpoint reste inchangé et un retry avec le même budget reproduit la même valeur canonique et les mêmes digests. Le vérificateur local recalcule le manifeste depuis les autorités, le checksum du checkpoint, les plages LBVH, les reçus actifs, les identités obligatoires de l'audit cumulatif et la transition relative entière. Il ne rejoue pas tout l'historique des compteurs et ne prétend pas établir la provenance de sa source. Le vérificateur de run reconstruit au contraire le checkpoint initial depuis le nuage, le LBVH et $K_{\max}$ fiables, puis rejoue chaque chunk dans l'ordre jusqu'au terminal.

Cette portée est volontairement `in_memory_reinjectable_checkpoint_and_prepared_transition_candidate_only`. Elle ne contient aucun codec binaire, aucune limite de décodage hostile, aucun temporaire, `fsync`, renommage atomique ou checkpoint Hyperdisk. Elle ne certifie donc ni reprise après perte de processus ou de VM, ni streaming 10 M+, ni débit. À la livraison de 9.3a, le chemin chaud recalculait encore le manifeste en $O(n)$ par chunk, la validation locale des antichaînes était quadratique dans les tailles de frontières et le vérificateur de run de commodité imposait à l'appelant de conserver tous les chunks. Le jalon 9.3b ci-dessous ferme ces trois dettes sans changer le schéma scientifique du checkpoint.

## Jalon 9.3b-RCPU livré

`ExactPairSupportAuthorityContext` authentifie le nuage canonique et l'image sémantique du LBVH une fois à sa construction. Son audit instrumente exactement une construction de manifeste, un point haché par point canonique, une feuille hachée par feuille LBVH et un nœud haché par nœud LBVH. Les surcharges de production, de vérification locale et de rejeu de chunk utilisent ensuite le manifeste mis en cache. Les anciennes signatures restent des adaptateurs froids qui créent un contexte temporaire; elles ne doivent pas être utilisées dans une boucle de chunks. Les constructeurs rejettent les nuages et LBVH temporaires.

La validation de la frontière ne compare plus tous les produits deux à deux. Un self-produit devient un rectangle de plages Morton et un produit croisé devient ses deux orientations. Le sweep trie les entrées et sorties sur la première coordonnée, traite les sorties avant les entrées à coordonnée égale pour respecter les intervalles semi-ouverts, puis compare seulement le prédécesseur et le successeur en seconde coordonnée. Tant que le préfixe est valide, les intervalles actifs sont disjoints; ces deux voisins suffisent donc à détecter le premier recouvrement. Pour $F$ produits, le coût est $O(F\log F)$ et la mémoire transitoire $O(F)$, avec au plus deux rectangles et quatre événements indexés compacts par produit source, puis deux tests de voisinage par rectangle.

La pile témoin, l'éventuelle expansion différée et les reçus stricts sont projetés dans un seul tableau léger d'intervalles sans modifier leur ordre persistant. Après un tri, chaque intervalle est comparé au maximum de fin du préfixe; les doublons, recouvrements et couples ancêtre–descendant sont donc rejetés en $O(M\log M)$ pour $M$ entrées et $M-1$ comparaisons adjacentes. Chaque reçu garde séparément sa recertification rationnelle $\max\phi<0$, sa disjonction des supports et sa contribution exacte au cardinal.

`ExactPairSupportIncrementalVerifier` copie le cache d'autorité, reconstruit son checkpoint initial, puis `verify_next` rejoue exactement une transition depuis le seul checkpoint fiable sans revalider cette source privée. Une transition valide avance avec le checkpoint attendu fraîchement recalculé, jamais avec une structure observée non vérifiée. Une transition invalide, exceptionnelle ou post-terminale laisse le checkpoint inchangé, empoisonne définitivement le vérificateur et interdit de sauter l'erreur. L'état retient zéro chunk antérieur; le vérificateur historique de run est désormais un adaptateur sur cette induction. Le singleton reste un run ancré vide. Le schéma, la base de preuve, le manifeste et tous les digests dorés v1 sont inchangés. Le type C++ hôte de résultat de validation gagne toutefois des compteurs : à ce stade pré-public 0.1, aucune stabilité ABI binaire n'est revendiquée pour cette structure.

Cette amélioration borne seulement l'authentification et la validation du curseur. Elle ne rend pas sous-quadratique le parcours géométrique des paires, ne fournit ni codec hostile ni stockage durable et ne justifie encore aucun essai de SLO ou de 10 M+.

## Jalon 9.3c-RCPU livré

Le codec v1 projette chaque `ExactPairSupportStreamChunk`, checkpoint successeur inclus, dans une enveloppe binaire canonique. Le magic fixe, la version, le kind, les flags, la longueur et tous les entiers du payload ont une représentation big-endian indépendante de l'ABI; les identifiants occupent 32 octets, les booléens et enums ont des tags fermés et les rationnels sont reconstruits depuis leur texte canonique. Un SHA-256 séparé par domaine ferme l'intégrité wire seulement. Une mutation suivie d'un recalcul de ce checksum peut encore être acceptée par le codec; elle doit toujours échouer au rejeu scientifique si elle modifie la transition.

Les limites du codec appartiennent à l'appelant et doivent être finies. Elles couvrent les octets totaux, la frontière, la somme pile--reçus--expansion différée, les records, les références `PointId`, un texte exact et leur somme. Le décodeur contrôle le compte, sa contribution minimale aux octets restants et les quotas agrégés avant chaque `reserve`; il contrôle la longueur exacte avant de construire une chaîne ou un `BigInt`. Les troncatures, octets terminaux, versions futures, kinds et flags inconnus, tags invalides, longueurs impossibles, rationnels non canoniques et checksums divergents échouent fermés. À la livraison historique de 9.3c, l'encodeur construisait encore sa chaîne rationnelle avant d'en contrôler la longueur et matérialisait payload puis wire. 9.3d a supprimé le vecteur payload, puis 9.3e ferme ci-dessous le vecteur wire du seul chemin durable et borne la conversion exacte avant `.str()`; cela ne suffit toujours pas à certifier une frontière 10 M+.

Le sink Unix reçoit un budget de chunk fixe et fiable; le budget encodé doit lui être exactement égal et ne pilote jamais le rejeu. Il ouvre un répertoire dédié avec `O_NOFOLLOW`, garde un verrou `flock` mono-écrivain, applique le quota dès l'inventaire, ouvre les finals avec `O_NONBLOCK`, exige les fichiers `pair-support-%020d.p9c` consécutifs depuis zéro et rejoue chacun depuis l'ancre initiale. Un seul chunk décodé est vivant à la fois. Un fichier final tronqué, corrompu, non régulier, non contigu, post-terminal ou de fausse filiation bloque la reprise; il n'est ni ignoré ni supprimé. Seul `.pair-support-%020d.tmp` à la séquence suivante est non autoritatif et nettoyable.

Pour publier, le sink encode et appelle `prepare_next`, qui recalcule le checkpoint attendu sans avancer le checkpoint fiable. Il crée ensuite le temporaire en mode create-only, boucle sur les écritures partielles et `EINTR`, exécute `fdatasync`, relit chaque octet, renomme dans le même répertoire et exécute `fsync` sur ce répertoire. Alors seulement `commit_prepared` déplace sans exception le checkpoint attendu. Abandonner un jeton avant renommage laisse l'état retentable; un jeton étranger, réutilisé ou préparé à une époque antérieure empoisonne le vérificateur. Une transition non terminale sans augmentation du travail est refusée afin qu'une suite de no-op ne consomme pas indéfiniment le quota de fichiers.

Les quatre points de crash appellent réellement `_Exit` dans un processus enfant. Après `temporary_file_written` ou `temporary_file_synchronized`, la reprise retrouve l'ancien checkpoint, supprime le temporaire et reproduit le même bundle byte pour byte. Après `transition_renamed` ou `directory_synchronized`, elle rejoue exactement un final et une seconde réouverture n'ajoute rien. Cette observation vaut pour la perte du processus sur le filesystem Unix local testé avec `flock`. Elle ne mesure ni coupure d'alimentation, ni disparition de VM, ni Hyperdisk, ni filesystem réseau, et ne certifie pas la disponibilité des octets après un sinistre matériel. Surtout, sans `HEAD` fiable, un suffixe final supprimé ou restauré à un ancien snapshot est indistinguable d'un run légitimement arrêté à ce préfixe : 9.3c n'est pas anti-rollback.

## Jalon 9.3d-RCPU livré

Le protocole durable passe au schéma v2 et rend son autorité explicite. `create_new` refuse tout namespace autoritatif ou transition existante; il peut seulement valider puis nettoyer l'unique `.HEAD.tmp` non publié laissé par une création interrompue avant de publier `HEAD_0`. `open_existing` exige un `HEAD` et ne peut plus reconstruire implicitement le plus long préfixe disponible. Le wire `HEAD` v1 occupe exactement 142 octets : enveloppe canonique big-endian, payload fixe de 80 octets et SHA-256 séparé par domaine. Son contrat lie le checkpoint initial, le manifeste scientifique, le budget fixe et les versions de checkpoint, parcours, codec et protocole durable; ses quatre données variables sont le digest de ce contrat, le nombre de transitions committées, leurs octets cumulés et le digest du checkpoint committé. Les caps codec et stockage restent externes et peuvent évoluer seulement s'ils admettent encore tout le préfixe annoncé.

Le chunk v1 conserve exactement son golden wire mais l'encodeur ne construit plus un payload séparé. Un unique `ByteWriter` écrit l'enveloppe, réserve logiquement les 32 octets terminaux, écrit directement le payload, backpatche la longueur en big-endian, calcule SHA-256 puis ajoute le checksum. Avant cet encodage, le sink réduit le cap du codec au reliquat du budget wire total et rejette un compteur déjà saturé; à la reprise, la taille maximale passée à la lecture est le minimum du cap codec et du reliquat annoncé par `HEAD`. Le pic logique passe donc de deux vecteurs proportionnels au chunk à un seul vecteur wire, plus la chaîne canonique exacte courante. Une réallocation de `std::vector`, la conversion décimale préalable des `BigInt` et la coexistence momentanée du wire lu avec le chunk décodé restent hors de cette borne stricte; un codec fd réellement streamé demeure nécessaire pour la voie 10 M+.

La publication immuable n'utilise plus un `fstatat` suivi d'un renommage écrasant. Le temporaire synchronisé est lié au nom final par `linkat`, qui échoue si ce nom existe, puis son ancien lien est supprimé et le répertoire synchronisé. Le sink construit alors le nouveau `HEAD` depuis le checkpoint fraîchement rejoué du jeton `PreparedNext`, écrit et synchronise `.HEAD.tmp`, vérifie que l'ancien `HEAD` n'a pas changé, le remplace atomiquement, synchronise le répertoire, puis committe en mémoire. Seul `HEAD` est remplaçable. Une course qui introduit un final après le précontrôle conserve donc son inode et force la réouverture sans avancement scientifique.

À la reprise, le compteur du `HEAD`, jamais l'inventaire, borne le rejeu. Tous les finals strictement antérieurs doivent exister, être réguliers, singly-linked, respecter les caps et passer le codec puis le rejeu exact. Le seul final éventuellement autorisé à la séquence du `HEAD` est un orphelin : s'il est issu de la fenêtre `linkat`, ses deux noms doivent viser le même inode; dans tous les cas il doit décoder et former exactement la transition suivante avant que ses éventuels liens temporaires soient supprimés et le répertoire resynchronisé. Un `.HEAD.tmp` complet doit annoncer exactement ce successeur; un temporaire incomplet reste non publié et nettoyable. Un orphelin invalide, un chunk committé manquant, un trou, une fausse filiation ou un final post-terminal échoue sans nettoyage. Aucun de ces fichiers non committés ne déplace le checkpoint.

L'ancre externe optionnelle contient un compteur et un digest de checkpoint. Le `HEAD` local peut être plus récent, mais le rejeu doit rencontrer exactement ce digest au compteur ancré; un `HEAD` plus ancien ou une divergence à ce rang est rejeté avant tout nettoyage, sous l'hypothèse d'absence de collision SHA-256. Un préfixe valide au-dessus d'une ancre périmée reste admissible : l'acquittement anti-rollback d'un commit exige donc de persister atomiquement son propre compteur et digest dans l'autorité monotone hors du domaine restauré. Sans cette mise à jour externe, le snapshot cohérent d'un ancien préfixe reste impossible à distinguer localement d'un arrêt honnête. Le SHA du `HEAD` fournit l'intégrité de son wire, pas sa fraîcheur.

## Jalon 9.3e-RCPU livré

Le chemin fd conserve le wire chunk v1 et son domaine SHA. Un compteur rejoue le même visiteur de payload pour connaître sa longueur sans la stocker; l'émetteur positionnel écrit ensuite l'enveloppe finale et le payload par blocs d'au plus 64 Kio, avec SHA incrémental sur les octets intentionnels. Le codec ne ferme ni ne synchronise le descripteur emprunté. Le sink durable garde l'ordre `fdatasync`--relecture bornée--`linkat`--`fsync`--`HEAD`; la relecture exige le reçu taille--checksum avant de rendre le final visible. Après le lien, le fd authentifié, le temporaire et le final doivent désigner le même inode; après suppression du temporaire, le final doit encore désigner ce fd. `.HEAD.tmp` doit de même désigner son fd relu avant `renameat`, puis le `HEAD` publié doit conserver cet inode jusqu'à l'acquittement. Chaque callback qui retourne déclenche une nouvelle authentification du contenu concerné; substitutions de nom et corruptions en place échouent donc sans avancer le checkpoint mémoire.

À la reprise, une première passe fd refuse enveloppe, taille, EOF ou checksum hostiles avant toute allocation pilotée par le payload. Le cap codec est resserré à la taille inventoriée avant cette lecture. Une seconde passe construit un seul chunk avec le décodeur canonique commun, rehache les octets consommés et compare leur checksum afin qu'une mutation de même taille entre les deux lectures échoue. Le décodeur alloue exactement un scratch physique de 64 Kio que la première vérification, le lecteur sans tableau propre et la vérification finale empruntent successivement. Les appels sont positionnels et la position du fd reste inchangée. L'encodeur exige un fichier régulier seekable vide en `O_RDWR` sans `O_APPEND` ni `O_DIRECT`; vérificateur et décodeur acceptent un wire non vide en `O_RDONLY` ou `O_RDWR`. Les offsets non représentables, I/O sans progrès et changements de type, identité ou taille sont refusés.

La limite individuelle exacte par défaut passe à 2048 octets pour la voie paire. La preuve binary64 donne au plus 958 octets par coordonnée de milieu et 1914 par rayon carré 3D; avant `.str()`, un préfiltre de bit-length rejette tout majorant dépassant le cap effectif de plus de deux octets, puis la longueur réelle doit respecter ce cap. Cette amélioration ne supprime ni le champ exact courant et ses temporaires de conversion/normalisation, ni les `BigInt`, ni les vecteurs du chunk décodé, et la borne n'est pas réutilisable telle quelle pour les supports trois et quatre. Aucun résultat 10 M+, SLO ou statut public n'en découle.

## Structures globales évitées

La cible dédiée `morsehgp3d_pair_support` contient la seule unité de traduction du producteur pair et dépend du LBVH, des points exacts et des prédicats de support; `morsehgp3d_pair_support_codec` ne dépend que de ce flux et `morsehgp3d_pair_support_durable` ne dépend que du codec. La cible séparée `morsehgp3d_higher_support` contient seulement l'évaluateur de produits et le producteur groupé des arités trois--quatre; elle dépend directement de la couche spatiale et non du wire pair. Toutes restent séparées de l'archive historique `morsehgp3d_hierarchy` qui contient Gamma. Elles ne construisent ni cellule top-$m$, ni coface, ni incidence de mosaïque d'ordre supérieur, ni Gamma global, ni arène de taille $\binom{n}{k}$. La mémoire de chaque producteur reste bornée par sa frontière active, ses parcours auxiliaires, les records budgetés et les petits témoins de rang; la reprise durable paire y ajoute un seul chunk décodé, un buffer I/O de 64 Kio et les temporaires exacts capés, jamais un wire de transition complet ni l'historique.

Le reçu statique v6 publie les cinq zéros architecturaux `persistent_top_m_cell_count=0`, `global_gamma_coface_count=0`, `global_gamma_incidence_count=0`, `materialized_pair_arena_count=0` et `materialized_higher_support_arena_count=0`. Ils décrivent les cibles et leurs sources; le booléen de résultat homonyme reste une assertion rejouée, pas à lui seul une preuve d'absence.

À titre d'échelle, pour 50 000 sites la somme des nombres de sous-ensembles de tailles deux à quatre dépasse $2.6\times10^{17}$; les matérialiser est exclu avant même l'ordre dix. Le parcours de paires reste néanmoins quadratique en pire cas et sa recherche témoin peut rendre le travail brut cubique. Le jalon est une base de correction et d'instrumentation, pas encore une preuve du SLO.

## Validation courte

Le test ciblé `morsehgp3d.hierarchy_pair_support_stream` couvre :

- maximum exact de la borne $\phi$, témoin canonique, cas d'égalité zéro et exclusion sûre par paire-ancre réelle;
- contre-exemple permanent aux coins AABB artificiels qui perdent la corrélation des sites;
- partition self-duale complète et paire longue absente d'une liste 1-NN fixe;
- prune de rang strict sur trois sites colinéaires;
- triangle rectangle avec shell supplémentaire, compté sans matérialisation globale;
- chaque arrêt budgétaire, frontière intacte et comptabilité exacte du résidu;
- singleton sans travail;
- accord bidirectionnel avec un différentiel brute-force séparé des paires et `brute_force_closed_ball` à $n=14$ pour $K_{\max}\in\left\lbrace 1,4,9,10\right\rbrace$;
- mutations du budget, d'un centre, d'un compte de shell, d'un audit, d'une plage de frontière, du statut et des assertions d'architecture et de réduction.

Pour 9.3a, il couvre en plus :

- l'identité des records, de l'audit cumulatif et du digest terminal entre un chunk résident et des chunks limités à une unité de travail;
- une interruption au milieu d'une recherche témoin à $n=14$, avec reçu strict interne et expansion différée réellement conservés dans le checkpoint en mémoire, suivie d'une reprise sans double compte;
- chacun des sept motifs d'arrêt, repris vers exactement le même audit terminal que le run résident;
- l'idempotence d'un chunk abandonné avant publication et la valeur préparée limitée à un record sur le triangle rectangle;
- l'acceptation d'un nuage et d'un LBVH reconstruits depuis une permutation d'entrée, puis le rejet après rehash d'une mutation de coordonnée, de $K_{\max}$, du schéma, d'une plage Morton, d'une identité obligatoire de l'audit, d'un état actif, d'un reçu dupliqué, ancêtre–descendant, superposé au support ou à la pile active, et d'un nœud feuille présenté comme expansion différée;
- l'ancrage positif depuis l'état initial et les rejets d'un préfixe incomplet, d'un chunk supprimé, réordonné ou dupliqué, d'un digest source ou ordre typé muté et d'un terminal localement intègre mais sans filiation;
- les vecteurs SHA-256 usuels et aux frontières 55, 56, 63, 64 et 65 octets, puis les digests dorés du manifeste, des checkpoints initial et mi-curseur, de la chaîne mixte et du terminal v1;
- le singleton déjà terminal, dont une reprise vide n'avance pas la séquence.

Pour 9.3b, le même target couvre en plus :

- un contexte persistant dont l'audit compte exactement un manifeste, chaque point, chaque feuille et chaque nœud une fois;
- plusieurs dizaines de chunks à une unité, vérifiés en ligne sans tableau de chunks, avec mêmes records, ordre typé, audit et digest terminal que le chunk résident;
- les bornes exactes des compteurs du sweep de rectangles et du tri d'intervalles, y compris un curseur contenant des reçus stricts réellement persistés;
- l'avancement par le checkpoint attendu, zéro chunk retenu, l'empoisonnement sans avance après mutation du digest source, le rejet explicite du chunk successeur qui tenterait de sauter cette transition et l'empoisonnement sur une exception de rejeu;
- le singleton ancré sans chunk et le rejet d'un no-op terminal redondant;
- le checker statique v2, qui exige le contexte, `verify_next`, les deux algorithmes quasi linéaires, l'absence d'un vecteur de chunks dans le vérificateur et l'absence de rescan direct du manifeste sur le chemin mis en cache.

Pour 9.3c, deux targets courts couvrent en plus :

- round-trip complet, réencodage unique et digest doré du wire v1;
- troncation à chaque préfixe, magic, version, kind, flags, longueur, checksum et EOF exact;
- compte dans le cap mais impossible dans les octets restants, `UINT64_MAX`, quotas de records, frontière, auxiliaires, références `PointId` et textes exacts, tags booléens ou enums invalides et rationnel non canonique;
- préparation à deux phases, abandon sans avance, commit unique et rejet d'un jeton périmé;
- publication multi-chunk jusqu'au terminal, reprise avec mêmes digests et zéro historique, verrou concurrent, symlinks, FIFO non bloquant, trou de séquence et finals tronqués ou corrompus;
- quatre pertes de processus réelles et retry byte-identique autour du renommage;
- le checker statique v3, qui inspecte les trois cibles isolées, le codec borné, la couture `prepare_next`--`commit_prepared`, les primitives Unix de publication et les quatre zéros architecturaux.

Pour 9.3d, ces mêmes targets courts couvrent en plus :

- l'identité du golden wire chunk v1 après passage à un seul `ByteWriter`, le succès au cap exact, l'échec un octet en dessous et la frontière d'enveloppe de 61 octets;
- la création explicite de `HEAD_0`, la reprise d'un temporaire initial complet avant lien, le rejet d'une seconde création ou d'une ouverture sans `HEAD`, le contrat de run et la normalisation de la fenêtre initiale à deux liens;
- une ancre externe égale ou antérieure au `HEAD` local, puis le rejet d'une ancre future ou divergente;
- les chunks committés manquants ou hostiles, l'unique orphelin suivant valide, la conservation d'un orphelin invalide et une collision no-replace qui préserve l'inode concurrent sans avancer;
- quatre fenêtres de perte de processus représentatives : temporaire chunk synchronisé, lien final créé, temporaire `HEAD` synchronisé et `HEAD` remplacé;
- le checker statique v4, qui impose le schéma durable v2, le checkpoint fraîchement rejoué, les deux seuls `linkat`, l'unique `renameat` du `HEAD` et le rejeu borné par son compteur.

Pour 9.3e, codec, durabilité et checker couvrent en plus :

- identité octet pour octet entre fd et vecteur v1, reçu taille--checksum, cap exact, cap moins un, position fd inchangée et traversée de plusieurs fenêtres de 64 Kio;
- acceptation `O_RDONLY` en lecture, rejet des pipes et des sorties non vides, `O_APPEND` ou `O_DIRECT`, troncation, corruption et reçu divergent;
- extrêmes binary64 finis dont les textes exacts restent sous 958 octets pour le milieu et 1914 pour le rayon carré, puis acceptation sous le défaut 2048;
- zéro wire de transition matérialisé dans le sink, corruption avant readback, substitutions d'inode du temporaire chunk et de `.HEAD.tmp`, et nettoyage d'un temporaire partiel non committé;
- le checker statique v5, qui impose comptage puis fd, ordre `fdatasync`--readback--`linkat`, décodage direct du fd, borne I/O fixe, absence de vecteur wire dans la publication et contrôles d'identité fd--noms.

Pour 9.2a, deux targets et une fixture Python courte couvrent en plus :

- triangles aigu, obtus et collinéaire, tétraèdre régulier, centre extérieur, intérieur strict, égalité shell et majorant convexe exact à `-12`;
- les deux contre-exemples aux coins supports, avec puissance reconstruite indépendamment en rationnels depuis le centre circonscrit;
- univers `BigInt` exact à quatre puis dix millions de sites, somme des partitions, quatre événements triangle et un événement tétraèdre sur la fixture régulière;
- frontière intacte à budget de travail nul, rejet d'une capacité incapable de contenir deux racines, supports intrinsèquement au-dessus du rang, shell supplémentaire de cinq sites et rejet d'une mutation du compte d'univers;
- neuf reçus universels stricts qui éliminent 32 supports dans un univers de 210, puis rejeu frais de chaque décision;
- le checker statique v6, qui isole aussi `morsehgp3d_higher_support`, exige groupes multiplicatifs, comptabilité `BigInt`, budgets, frontière résiduelle et rejeu frais, et interdit atlas, Gamma et arènes de tous les supports.

La validation finale 9.3e exécute trois CTests ciblés codec, durabilité et checker v5 : GCC Release strict passe en 0,43 s et Clang Release strict en 0,39 s; le test durable GCC ASan/UBSan passe en 0,25 s. Un smoke hostile colinéaire antérieur limité à $n\leq256$ observait 24 301 visites témoins et environ 111 ms au dernier point après les courts-circuits sûrs; ce diagnostic ne ferme aucun exposant ni SLO. Aucun benchmark long et aucune ressource GCP ne sont nécessaires pour cette validation CPU.

La validation finale 9.2a exécute seulement les trois CTests produit, flux et régression rationnelle : GCC 13.3 Release strict passe en 2,06 s et Clang 18.1 Release strict en 1,82 s. Le checker architectural v6 passe séparément. Ces temps incluent le différentiel exhaustif borné et le cas des neuf reçus, mais ne constituent ni un benchmark de croissance, ni une mesure du SLO. Aucun test long, CUDA ou GCP n'a été lancé.

## Limites et ordre de poursuite

Le prochain travail ne doit pas élargir l'oracle de cellules. L'ordre utile est désormais :

1. transformer la frontière groupée 9.2a en checkpoint réinjectable et en chunks externes sans événements cumulés, avec le même contrat transactionnel que les paires mais des caps rationnels propres aux arités trois--quatre;
2. resserrer les produits tétraédriques ambigus par puissance de la face opposée, puis orientation et in-sphère lorsque leur signe est certifié, sans utiliser les coins supports comme oracle;
3. ajouter les reçus terminaux bornés nécessaires à la proposition `cuda_g4` P1 et leur recertification CPU exacte;
4. mesurer seulement ensuite les compteurs de croissance sur 12 500, 25 000 et 50 000 sites, sans prétendre fermer le SLO;
5. remplacer progressivement `.canonical_key()` et le parse décimal par un formateur--parseur exact borné par blocs, en conservant le wire pair v1 et un schéma d'arité supérieure distinct;
6. avant la qualification finale, remplacer la canonisation exacte eager et la construction LBVH CPU par des coordonnées binary64 SoA, exact paresseux et Morton/radix/LBVH device;
7. pour 10 M+, persister deux frontières bornées et des chunks externes, sans conserver les événements cumulés ni les chunks de vérification en mémoire, puis qualifier le protocole de stockage réellement choisi au-delà de la seule perte de processus.

La Phase 9 reste `in_progress`. Les runs terminaux recertifient séparément la complétude de leurs tailles de support, mais ce jalon ne ferme ni un checkpoint commun supports 2–4, ni la Phase 9, ni une forêt Morse, ni un statut public. GCP non utilisé.
