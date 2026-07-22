# Progression Phase 9 — flux direct de supports H0

> [!IMPORTANT]
> Phase `9`, backend `reference_cpu`, profil `hgp_reduced`, mode `certified` pour le jalon pair CPU. La porte d'entrée est satisfaite par les Phases 1, 2A, 4 et 7. Ce jalon ne produit qu'un catalogue direct de supports de taille deux : aucune forêt, aucune réduction hiérarchique, aucun `public_status` et aucune assertion de complétude pour les tailles trois et quatre.

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

## Structures globales évitées

La cible dédiée `morsehgp3d_pair_support` contient la seule unité de traduction du producteur et dépend du LBVH, des points exacts et des prédicats de support; elle est séparée de l'archive historique `morsehgp3d_hierarchy` qui contient Gamma. Elle ne construit ni cellule top-$m$, ni coface, ni incidence de mosaïque d'ordre supérieur, ni Gamma global, ni arène de taille $\binom{n}{k}$. Sa mémoire résidente propre est bornée par la frontière active, deux parcours auxiliaires non simultanés, les records budgetés et les petits témoins de rang.

Le reçu statique du jalon publie les quatre zéros architecturaux `persistent_top_m_cell_count=0`, `global_gamma_coface_count=0`, `global_gamma_incidence_count=0` et `materialized_pair_arena_count=0`. Ils décrivent la cible et ses sources; le booléen de résultat homonyme reste une assertion rejouée, pas à lui seul une preuve d'absence.

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

Le test et sa bibliothèque passent en Release avec les avertissements stricts sous GCC 13.3 et Clang 18.1. Un smoke hostile colinéaire limité à $n\leq256$ observe 24 301 visites témoins et environ 111 ms au dernier point après les courts-circuits sûrs; ce diagnostic ne ferme aucun exposant ni SLO. Aucun benchmark long et aucune ressource GCP ne sont nécessaires pour cette validation CPU.

## Limites et ordre de poursuite

Le prochain travail ne doit pas élargir l'oracle de cellules. L'ordre utile est désormais :

1. ajouter un codec borné et un sink durable temporaire--synchronisation--renommage, puis tester crash avant et après publication;
2. ajouter les reçus terminaux bornés nécessaires à la proposition `cuda_g4` P1 et leur recertification CPU exacte;
3. mesurer seulement ensuite les compteurs de croissance sur 12 500, 25 000 et 50 000 sites, sans prétendre fermer le SLO;
4. généraliser la frontière aux supports trois et quatre sans liste $L$-NN excluante;
5. avant la qualification finale, remplacer la canonisation exacte eager et la construction LBVH CPU par des coordonnées binary64 SoA, exact paresseux et Morton/radix/LBVH device;
6. pour 10 M+, persister deux frontières bornées et des chunks externes, sans conserver les événements cumulés ni les chunks de vérification en mémoire.

La Phase 9 reste `in_progress`. Ce jalon ne ferme ni la complétude supports 2–4, ni la Phase 9, ni une forêt Morse, ni un statut public. GCP non utilisé.
