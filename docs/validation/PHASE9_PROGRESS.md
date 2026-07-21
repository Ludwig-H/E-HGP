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

La frontière résiduelle de 9.1-RCPU est un reçu auditable seulement. L'API ne permet pas encore de la réinjecter comme checkpoint lié au nuage et au LBVH. La reprise transactionnelle, le hash de liaison et le sink de chunks restent explicitement ouverts en 9.3; aucune affirmation de streaming 10 M+ n'est faite par ce premier lot.

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

Le test et sa bibliothèque passent en Release avec les avertissements stricts sous GCC 13.3 et Clang 18.1. Un smoke hostile colinéaire limité à $n\leq256$ observe 24 301 visites témoins et environ 111 ms au dernier point après les courts-circuits sûrs; ce diagnostic ne ferme aucun exposant ni SLO. Aucun benchmark long et aucune ressource GCP ne sont nécessaires pour cette validation CPU.

## Limites et ordre de poursuite

Le prochain travail ne doit pas élargir l'oracle de cellules. L'ordre utile est :

1. rendre la frontière et le curseur témoin réellement reprenables, liés à un manifeste compact du nuage/LBVH, avec sink de chunks transactionnel;
2. ajouter les reçus bornés nécessaires à la proposition `cuda_g4` P1 et leur recertification CPU exacte;
3. mesurer seulement ensuite les compteurs de croissance sur 12 500, 25 000 et 50 000 sites, sans prétendre fermer le SLO;
4. généraliser la frontière aux supports trois et quatre sans liste $L$-NN excluante;
5. avant la qualification finale, remplacer la canonisation exacte eager et la construction LBVH CPU par des coordonnées binary64 SoA, exact paresseux et Morton/radix/LBVH device;
6. pour 10 M+, persister deux frontières bornées, des chunks externes et des checkpoints, sans conserver les événements cumulés en mémoire.

La Phase 9 reste `in_progress`. Ce jalon ne ferme ni la complétude supports 2–4, ni la Phase 9, ni une forêt Morse, ni un statut public. GCP non utilisé.
