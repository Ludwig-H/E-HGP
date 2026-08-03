# Audit de la sonde Morton locale pour les supports trois et quatre

## 0. Portée et statut

Cet audit est un raccord de Phase 15, sans ouverture ni fermeture de phase. Le chemin examiné est `reference_cpu / hgp_reduced / universal_gram_cramer_well_centered_probe_gate_bounded_local_morton_hybrid_v6`; son statut public reste `not_claimed`. Il porte uniquement sur l'accélération exacte du rejet de rang des produits de supports de taille trois et quatre. Il ne certifie ni le temps cible à 50 000 points, ni le régime de dizaines de millions de points, ni la qualité du clustering final.

Verdict principal : ni `HGP-old`, ni le backend historique supprimé avant la consolidation ne contiennent un mécanisme exact qui attribue « la boule d'une paire » à une cellule Morton. Deux mécanismes différents ont été confondus :

- la classification exacte d'une AABB requête par rapport à une boule de support;
- l'attribution combinatoire d'une paire à son extrémité de rang Morton le plus élevé.

La généralisation certifiable aux supports trois et quatre conserve ces rôles séparés. La partition canonique des produits de nœuds est l'unique propriétaire scientifique des supports. Une sonde Morton locale ne propose que quelques points susceptibles de fournir des témoins strictement intérieurs; chaque témoin est recertifié contre tout le produit de supports. Une sonde infructueuse est toujours `fail-open` et provoque la subdivision canonique du produit.

La campagne de qualité massive reste formellement fermée. `morsehgp3d/tests/profiling/point_hierarchy_quality_campaign_v2.json` conserve `campaign_entry_gate_satisfied=false`, `public_exact_status_claimed=false` et `quality_campaign_promotes_exact=false`. `gcp-migration/check_point_hierarchy_quality_preflight.py:70-73` contrôle ce booléen avant tout handshake d'exécutable, toute création de clé SSH et toute mutation facturable. Une relance locale du preflight sur le plan suivi s'arrête avec le code 1 et `point-hierarchy quality preflight refused: campaign entry gate is closed`. Aucun résultat 50 000, 10 000 001 ou 30 000 000 points n'est donc produit ni revendiqué par cet audit.

## 1. Ce qui existe effectivement dans les anciens codes

### 1.1 `HGP-old`

`HGP-old/src/hgp_clusterer/hypergraph.py:45-134` choisit entre Delaunay d'ordre supérieur, Delaunay ordinaire et Rips. Le chemin `orderk_delaunay` se trouve aux lignes 67-128 et transforme les rayons flottants selon `expZ`. `HGP-old/src/hgp_clusterer/geometry.py:32-63` emploie FAISS, `cKDTree` ou `NearestNeighbors`. Aucun de ces fichiers ne construit un ordre Morton, un octree ou un propriétaire de boule.

Une recherche dans tout `HGP-old` ne trouve donc aucun prédicat exact boule--cellule Morton à réutiliser.

### 1.2 Backend historique supprimé

Le dernier code apparenté est accessible dans le parent de la consolidation `075a575` :

- `075a575^:perg_hgp/perg_hgp/gabriel.py:40-125` parcourt des cellules de grille, calcule en flottant la distance minimale centre--AABB et utilise une tolérance `tol=1e-6`;
- `075a575^:perg_hgp/perg_hgp/backends/power_cover_3d_cuda/coverage.py:100-236` utilise un `cKDTree`, des AABB `float64` et une marge arithmétique;
- `075a575^:perg_hgp/POWER_COVER_3D.md:268-289` définit une relation d'intersection entre une boule de puissance et une cellule cubique.

Cette relation historique est une relation plusieurs-à-plusieurs d'intersection, pas un propriétaire unique. Son implémentation flottante ne peut pas servir d'autorité exacte.

## 2. Les deux mécanismes exacts actuels pour les paires

### 2.1 Classifier une cellule requête par rapport à une boule diamétrale

Pour une paire $(u,v)$, le code utilise le polynôme exact

$$\phi_{u,v}(x)=(x-u)\mathbin{\cdot}(x-v)=\left\Vert x-\frac{u+v}{2}\right\Vert^2-\frac{\left\Vert u-v\right\Vert^2}{4}.$$

`morsehgp3d/src/cpu/hierarchy/pair_support_stream.cpp:871-945` calcule les extrema exacts de $\phi_{u,v}$ sur une AABB. Le minimum s'obtient en rabattant le milieu $(u+v)/2$ sur chaque intervalle axial; le maximum d'une quadratique convexe s'obtient aux extrémités axiales.

`morsehgp3d/src/cpu/hierarchy/pair_support_stream.cpp:4465-4635` applique ensuite les règles suivantes :

- $\min_Q\phi_{u,v}>0$ certifie tout le nœud strictement extérieur;
- $\max_Q\phi_{u,v}<0$ certifie tout le nœud strictement intérieur;
- toute égalité ou ambiguïté descend jusqu'aux feuilles, où le signe exact classe intérieur, shell et extérieur.

Le sens géométrique exact est donc $Q\subset B(u,v)$ ou $Q\cap B(u,v)=\varnothing$, jamais $B(u,v)\subset Q$.

### 2.2 Propriétaire combinatoire d'une paire

`morsehgp3d/include/morsehgp3d/gpu/morton_yao48_pair_frontier.hpp:97-111` distingue explicitement le propriétaire opérationnel Morton et l'ordre canonique des `PointId`. `morsehgp3d/include/morsehgp3d/gpu/morton_yao48_pair_frontier.hpp:231-236` donne l'invariant : l'extrémité Morton-haute possède son préfixe strict.

`morsehgp3d/src/gpu/morton_yao48_pair_frontier.cpp:657-720` sépare exactement préfixe possédé, suffixe ignoré et nœud coupant la frontière. Les feuilles survivantes sont canonisées en ordre `PointId` dans `morsehgp3d/src/gpu/morton_yao48_pair_frontier.cpp:859-998`.

Ce propriétaire empêche d'émettre deux fois une paire. Il ne dit rien sur la cellule qui contiendrait sa boule et ne certifie pas son rang.

## 3. Autorité scientifique pour les supports trois et quatre

Soit $m\in\left\lbrace 3,4\right\rbrace$. Une entrée canonique $e$ contient des groupes $(N_i,r_i)$ dont les intervalles de feuilles Morton sont disjoints et ordonnés, avec $\sum_i r_i=m$. Elle représente exactement les sous-ensembles qui prennent $r_i$ points distincts dans $N_i$. Sa masse exacte est

$$M(e)=\prod_i\binom{\lvert N_i\rvert}{r_i}.$$

Les invariants sont vérifiés dans `morsehgp3d/src/cpu/hierarchy/higher_support_stream.cpp:734-861`. La racine canonique est `(root,m)` aux lignes 756-763. La subdivision distribue la multiplicité entre les deux enfants et vérifie l'identité exacte des masses dans `morsehgp3d/src/cpu/hierarchy/higher_support_stream.cpp:1948-2023`.

Le propriétaire scientifique d'un support non ordonné est donc son unique entrée terminale dans cette partition. Aucun point, aucune boule et aucune cellule Morton supplémentaire ne devient propriétaire scientifique. Cela donne par induction une couverture disjointe et exhaustive de $\binom{n}{3}+\binom{n}{4}$ sans matérialiser ces deux ensembles.

Les boîtes supports sont reconstruites dans `morsehgp3d/src/cpu/hierarchy/higher_support_stream.cpp:879-900`. Une boîte de groupe est répétée selon sa multiplicité, mais la sémantique du produit continue d'imposer des feuilles distinctes à l'intérieur de chaque groupe.

## 4. Prédicat exact généralisé

Pour $U=\left\lbrace p_0,\ldots,p_d\right\rbrace$, avec $d=m-1$, posons $e_i=p_i-p_0$, $G_{ij}=e_i\mathbin{\cdot}e_j$, $h_i=G_{ii}$, $\Delta=\det(G)$ et $M_i=\det(G[i\gets h])$. Pour $y=x-p_0$, le polynôme de puissance sans division est

$$P_U(x)=\Delta\left\Vert y\right\Vert^2-\sum_{i=1}^{d}M_i e_i\mathbin{\cdot}y.$$

Pour un support affine indépendant, $\Delta>0$ et

$$P_U(x)=\Delta\left(\left\Vert x-c_U\right\Vert^2-\beta(U)\right).$$

Le signe de $P_U$ est donc exactement celui de la puissance à la sphère circonscrite. `morsehgp3d/include/morsehgp3d/hierarchy/higher_support_product.hpp:85-121` expose une décision exacte à trois valeurs pour une AABB requête $Q$ et tout le produit de boîtes supports :

- borne supérieure strictement négative : chaque point de $Q$ est strictement intérieur à chaque sphère de support affine indépendant du produit;
- borne inférieure positive ou nulle : aucun point de $Q$ n'est un témoin strict pour aucune de ces sphères;
- intervalles se chevauchant autour de zéro : `inconclusive`, sans pouvoir d'exclusion.

Une égalité n'est jamais comptée comme intérieur strict. Le prédicat est universel sur les boîtes supports; il reste donc valide lorsque les points des supports franchissent arbitrairement des frontières Morton.

## 5. Algorithme de sonde locale

### 5.1 Seuil et domaine exclu

Pour un rang fermé maximal $s_{\max}$, le nombre de témoins stricts suffisant pour rejeter tout le produit de taille $m$ est

$$t_m=s_{\max}-m+1.$$

Le domaine support $D(e)$ est l'union des intervalles de feuilles des groupes. Aucun point de $D(e)$ ne peut servir de témoin externe. Le calcul du seuil et l'exclusion sont respectivement visibles dans `morsehgp3d/src/cpu/hierarchy/higher_support_stream.cpp:1352-1373`.

### 5.2 Porte universelle de bon centrage

La sonde de rang n'est ouverte que si l'analyse Gram--Cramer du produit prouve simultanément

$$\Delta_{\min}>0\qquad\text{et}\qquad (M_i)_{\min}>0\quad\text{pour tout }0\leq i<m.$$

Autrement dit, chaque tuple du produit est affine indépendant et possède des coordonnées barycentriques de centre circonscrit strictement positives. Une porte fausse est volontairement `inconclusive` : elle ne certifie aucun mauvais tuple et n'en retire aucun. Le produit non terminal est subdivisé pour resserrer ses boîtes; le terminal poursuit sa classification exacte. Cette asymétrie est indispensable, car l'absence d'une preuve universelle de bon centrage n'est pas une preuve universelle de mauvais centrage.

### 5.3 Propositions déterministes

Pour chaque intervalle support $[b_i,e_i)$ et chaque distance $1\leq d\leq t_m$, proposer, lorsqu'elles existent, les positions

$$b_i-d\qquad\text{et}\qquad e_i+d-1.$$

Le chemin ajoute la feuille obtenue par descente exacte vers le centre du tuple représentatif, puis les positions situées à distance Morton au plus $t_m$ autour de cette feuille. Retirer les positions appartenant à $D(e)$ et dédupliquer avant toute certification. Avec au plus quatre groupes et $t_m\leq9$, le plan propose au plus

$$L\leq(8+2)t_m+1\leq91$$

positions, indépendamment de $n$. Ce chemin n'a qu'un rôle de proposition.

### 5.4 Certification hybride cellule--feuille et arrêt

Chaque position proposée est remontée vers le plus haut ancêtre LBVH qui reste extérieur à $D(e)$ et dont la masse ne dépasse pas $H=t_m$. Les racines ainsi obtenues sont dédupliquées et forment une antichaîne. Chaque racine garde la liste des seules feuilles proposées qu'elle recouvre.

La décision `exact_higher_support_product_query_cell_decision` porte toujours sur les boîtes complètes du produit. Ses trois branches sont consommées ainsi :

1. `strictly_inside_every_independent_sphere` compte toute la masse de la racine et conserve son reçu exact;
2. `outside_or_boundary_every_independent_sphere` saute toute la racine locale, sans conclure sur le produit support;
3. `inconclusive` sur une racine interne évalue uniquement les feuilles proposées associées, jamais tous ses descendants.

Il y a au plus $L$ décisions de racines et $L$ décisions de fallback, soit au plus 182 évaluations exactes. Les reçus positifs sont rejoués contre l'autorité exacte avant émission du certificat. Si leur masse atteint $t_m$, le produit est pruné par rang. Sinon tout l'état positif est effacé : un produit non terminal est subdivisé et un support terminal est classifié par la requête globale exacte de boule fermée. Une sonde ambiguë ou insuffisante demeure donc `fail-open`.

## 6. Frontières Morton : ce qui est couvert et ce qui ne l'est pas

La contiguïté dans l'ordre Morton n'implique pas la proximité spatiale, et la proximité spatiale n'implique pas la contiguïté Morton. Avec trois bits par axe et l'entrelacement usuel, les cellules $(3,0,0)$ et $(4,0,0)$ partagent une face, mais leurs codes sont respectivement $9$ et $64$. Un halo linéaire court peut donc manquer un point spatialement voisin si des feuilles occupent les codes intermédiaires.

Ce manque n'est pas une faute d'exhaustivité :

- une feuille proposée ne produit un prune qu'après preuve universelle exacte;
- une feuille non proposée peut seulement faire perdre une occasion de prune;
- un produit non pruné est repris par la partition canonique disjointe et exhaustive;
- une feuille terminale est toujours fermée par la classification globale exacte.

La sonde n'est donc pas une preuve de couverture spatiale. Elle est un filtre positif borné dont l'absence de résultat n'a aucune sémantique scientifique.

## 7. Déduplication, propriété opérationnelle et reçus

Les halos de plusieurs groupes et le halo central peuvent proposer la même position. La clé de déduplication autoritative est la position de feuille Morton dans l'index certifié. Après retrait de $D(e)$, chaque position n'est associée qu'une fois à une racine locale et à son éventuel fallback singleton.

La provenance opérationnelle des halos ne doit jamais entrer dans le digest scientifique. Seuls les reçus de nœuds LBVH, authentifiés par leur index et leur intervalle, possèdent un pouvoir de preuve. Une racine strictement intérieure peut donc porter une masse supérieure à un; les racines positives et les feuilles positives de fallback doivent ensemble former une antichaîne disjointe hors de $D(e)$.

Le checkpoint `schema=6 / traversal=5` persiste le centre de proposition, un curseur de racines, un curseur de feuilles de fallback, son drapeau d'activité et les reçus positifs. `rank_frontier` reste vide. Lors d'un rejeu ou d'une reprise, le vérificateur régénère le plan, recertifie les intervalles, l'absence d'intersection avec $D(e)$, la branche ambiguë qui autorise un fallback et chaque prédicat universel exact; un compteur persisté seul n'est jamais une preuve.

## 8. Complexité réelle

Soient $g\leq4$, $H=t_m\leq9$, $L\leq10H+1\leq91$, $E\leq2L\leq182$, $d_T$ la profondeur du LBVH et $T_{\mathrm{exact}}$ le coût d'une décision universelle exacte.

La porte universelle coûte une analyse Gram--Cramer. Ensuite, la descente centrale coûte $O(d_T)$, chaque conversion position--nœud au plus $O(d_T)$ et la déduplication sur le petit vecteur borné $O(L^2)$. Les racines et les seuls singletons de fallback totalisent au plus $E$ décisions. Le coût par produit admissible est donc honnêtement

$$O\left(L^2+Ld_T+ET_{\mathrm{exact}}\right),$$

avec un scratch $O(L+H)$. Une table inverse position--nœud feuille construite une fois en $O(n)$ ramènerait le terme de recherche des feuilles à $O(L)$, mais elle n'est pas nécessaire à la correction.

Si $V_m$ produits de taille $m$ sont effectivement visités et si $C_{\mathrm{terminal}}$ désigne le coût cumulé des requêtes terminales, le travail total se borne par

$$O\left(n\log n+\sum_{m=3}^{4}V_m\left[L^2+Ld_T+ET_{\mathrm{exact}}\right]+C_{\mathrm{terminal}}\right).$$

Cette formule ne borne pas favorablement $V_m$. Dans un cas adversarial sans prune, $V_m$ peut rester de l'ordre de $\binom{n}{m}$. Le qualificatif `sparse` signifie ici seulement : pas de DFS global de tous les points pour chaque produit, pas de catalogue global de supports, cellules, cofaces ou incidences, et nombre borné de propositions par produit. Il ne constitue ni une preuve sous-quadratique, ni une preuve de viabilité à 50 000 ou 30 millions de points.

### 8.1 Profil post-sonde

Le profil borné final à $n=32$ conserve 5 000 unités par cas. Pour `uniform`, les couples $(K,\text{résolus},\text{visites rang},\text{porte})$ valent $(5,1793,1063,2158)$ et $(10,1350,1991,1668)$; pour `separated`, $(5,2952,17,2749)$ et $(10,2922,70,2723)$; pour `multiscale`, $(5,2838,505,2507)$ et $(10,2825,531,2494)$. Les six cas terminent par `work_unit_limit` et restent donc `NO-GO`. Les tailles 64 et 128 ne sont pas lancées.

Le champ interne `ru_maxrss` du premier run est rejeté, car il est contaminé ou hérité et `peak_rss_growth=0` dès le premier cas. Sur une nouvelle exécution complète, l'échantillonnage externe de `/proc/<pid>/status` toutes les 20 ms observe un pic de 13 896 Kio. Cette mesure porte sur le processus complet, pas sur chaque famille, et n'est pas extrapolée. La porte de croissance 50 k demeure fermée et GCP n'a pas été utilisé.

## 9. Propriétaire implicite possible par loose octree

Une attribution boule--cellule exacte n'exige pas nécessairement de matérialiser un octree global. Pour une boule non dégénérée $B(c,r)$, choisir l'unique longueur dyadique $h=2^j$ telle que

$$\frac{h^2}{4}<r^2\leq h^2.$$

Soit $Q_h(c)$ l'unique cellule de côté $h$, semi-ouverte sur chaque axe, qui contient le centre $c$. Sa cellule lâche est

$$Q_h^+(c)=Q_h(c)\mathbin{\oplus}[-h,h]^3.$$

Comme tout point de la boule diffère de $c$ d'au plus $r\leq h$ sur chaque axe, on a $B(c,r)\subseteq Q_h^+(c)$. Le couple $(h,Q_h(c))$ est unique : l'inégalité fixe le niveau et la convention semi-ouverte fixe la cellule du centre. Pour un triangle ou un tétraèdre affine indépendant fixé, $c$ et $r^2$ se calculent exactement par Gram--Cramer; cet owner logique peut donc être calculé à la demande sans construire tous les nœuds de l'octree.

Ce propriétaire implicite est toutefois postérieur à l'implémentation auditée et ne résout pas seul l'énumération. Un produit non terminal représente une famille de sphères sans centre, rayon ou owner unique, et ranger un support terminal dans une cellule ne découvre pas les autres tuples qui doivent être rangés dans la même cellule. Il reste à concevoir l'inversion sparse « cellule lâche occupée vers produits supports candidats », ainsi que son index d'occupation local, sans matérialiser $\binom{n}{3}$, $\binom{n}{4}$ ou tout l'octree.

Après le no-go du halo hybride, cette inversion implicite est la suite architecturale la plus prometteuse : l'owner peut fournir un bucket exact de proposition ou d'exclusion, tandis que la partition canonique garde l'exhaustivité et que tout manque reste `fail-open`. Aucune borne sparse moyenne ou pire cas n'est encore démontrée; ce schéma ne rouvre donc ni 50 k ni GCP à lui seul.

## 10. Emplacements de code concernés

La substitution architecturale est la suivante :

- remplacer, sur le chemin des produits non terminaux, toute initialisation d'une recherche de rang depuis `index_.root_index_` et tout DFS témoin global par la porte `all_well_centered_support_certified`, `local_rank_probe_candidate_cells` puis `continue_rank_prune_search`;
- conserver `required_strict_interior_count`, l'exclusion de $D(e)$ et le prédicat `exact_higher_support_product_query_cell_decision`;
- conserver `expand_product` comme continuation `fail-open` obligatoire;
- conserver la classification terminale globale `classify_terminal_support`;
- persister uniquement le centre proposé, les deux curseurs bornés cellule--fallback et les reçus positifs; `rank_frontier` doit rester vide sur le chemin produit et ne peut redevenir un DFS global caché.

Le working tree du 3 août 2026 contient déjà cette substitution dans les fonctions et lignes citées. Cet audit n'en modifie pas l'implémentation partagée.

## 11. Validation bornée et manque de croissance restant

Les validations existantes sont proportionnées et ne multiplient pas les campagnes :

- `test_higher_support_product.cpp` couvre la porte universelle, intérieur, extérieur, shell, cellule coupant la frontière, permutations et différentiel rationnel borné;
- `test_hierarchy_higher_support_terminal_prune.cpp` couvre une racine locale positive de masse supérieure à un, le fallback feuille après cellule ambiguë, l'égalité cosphérique et la reprise du curseur;
- `test_hierarchy_higher_support_stream.cpp` compare la sortie aux oracles exhaustifs bornés, notamment à $n=14$, exige `rank_frontier` vide et rejoue les reçus;
- `exact_higher_support_growth_profile.cpp` applique le coupe-circuit aux trois familles sans poursuivre vers 64 ou 128 après un échec à 32.

La relance ciblée suivante passe quatre tests sur quatre :

```text
cmake --build morsehgp3d/build --target morsehgp3d_hierarchy_higher_support_product_tests morsehgp3d_hierarchy_higher_support_stream_tests morsehgp3d_hierarchy_higher_support_terminal_prune_tests morsehgp3d_exact_higher_support_growth_profile -j2
ctest --test-dir morsehgp3d/build -R '^morsehgp3d\.(hierarchy_higher_support_product|hierarchy_higher_support_stream|hierarchy_higher_support_terminal_prune|exact_higher_support_growth_profile_contract)$' --output-on-failure
4/4 tests passed
```

Ces tests bornés suffisent à vérifier la logique actuellement livrée sans multiplier les fixtures. Le manque restant est algorithmique : le profil post-sonde atteint encore le coupe-circuit sur les six cas à 32 points. Tant que cette croissance n'est pas favorable, les mesures GCP de 50 000 et dizaines de millions de points restent interdites et aucun benchmark ne peut promouvoir `public_status=exact`.
