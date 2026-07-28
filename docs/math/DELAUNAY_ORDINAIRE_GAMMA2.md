# Delaunay ordinaire et exactitude de Gamma$_2$ réduit

> **Statut.** Le théorème ci-dessous établit une réduction exacte de `hgp_reduced` à l'ordre deux, sous position générale et avec le 1-squelette complet du complexe de Delaunay ordinaire. Il ne fournit pas une énumération scalable : la famille certifiée peut rester cubique et ne doit jamais être matérialisée dans le chemin produit. Les niveaux binary64, les dégénérescences résolues par un SoS particulier et le SLO 50 k/$K=10$ restent séparément non certifiés.

## 1. Univers de cofaces

Soit $X\subset\mathbb{R}^{d}$ fini et soit $G_{\mathrm{Del}}(X)$ le graphe de Delaunay ordinaire : deux sites sont adjacents lorsque leurs cellules de Voronoï ordinaires se rencontrent. Pour une coface $Q\subset X$ de cardinal trois, notons $E(Q)$ ses trois facettes de cardinal deux. La famille à une arête est

$$\mathcal{C}_{1}=\left\lbrace Q\in\binom{X}{3}:E(Q)\cap E(G_{\mathrm{Del}})\neq\varnothing\right\rbrace.$$

La filtration restreinte conserve chaque $Q\in\mathcal{C}_{1}$ à son niveau exact de miniboule $\beta(Q)$. Elle ne conserve pas seulement les triangles de Gabriel et ne se confond pas avec la famille historique des wedges, qui exige au moins deux arêtes de Delaunay.

## 2. Lemme de connexité radiale

**Lemme 1.** Pour toute boule fermée $B(c,r)$, le sous-graphe de Delaunay induit par $S=X\cap B(c,r)$ est connexe dès que $S$ est non vide.

**Preuve.** Fixons $p\in S$ et parcourons le segment de $p$ vers $c$. Si $y=(1-t)p+tc$ appartient à la cellule de Voronoï d'un site $x$, alors $\lVert x-y\rVert\leq\lVert p-y\rVert=t\lVert p-c\rVert$. Par l'inégalité triangulaire, $\lVert x-c\rVert\leq\lVert x-y\rVert+\lVert y-c\rVert\leq t\lVert p-c\rVert+(1-t)\lVert p-c\rVert=\lVert p-c\rVert\leq r$; tout site rencontré appartient donc à $S$. Deux cellules successives le long du segment fournissent une arête de Delaunay. Les chemins issus des différents $p$ terminent sur des cellules contenant $c$; ces cellules ont une intersection commune et leurs sites appartiennent au même simplexe du complexe de Delaunay. Tous les sites de $S$ sont ainsi reliés dans le sous-graphe induit. $\square$

En cas de cosphéricité, cet argument porte sur le 1-squelette complet du nerf de Voronoï. Une triangulation particulière obtenue par perturbation symbolique peut en omettre des arêtes; cette situation doit être recertifiée ou déclarée non supportée, jamais assimilée silencieusement au cas générique.

## 3. Exactitude de la restriction à une arête

**Théorème 2.** Sous les hypothèses du lemme 1, la filtration Gamma$_2$ exhaustive et sa restriction à $\mathcal{C}_{1}$ possèdent exactement les mêmes facettes actives et les mêmes composantes de facettes à toute coupe stricte ou fermée, avec `include_isolated=false`.

**Preuve.** Considérons une coface Gamma active $Q$ et sa miniboule $B(c,r)$. Posons $S=X\cap B(c,r)$. Le lemme 1 donne un arbre couvrant $T$ du graphe de Delaunay induit par $S$. Toute coface de trois sites de $S$ qui contient une arête de $T$ appartient à $\mathcal{C}_{1}$ et son niveau de miniboule est au plus $r^{2}$.

Les facettes correspondant aux arêtes de $T$ sont toutes actives : comme $\lvert S\rvert\geq3$, chaque arête de $T$ peut être complétée par un troisième site de $S$. Deux arêtes adjacentes de $T$ sont reliées par le triplet formé par leurs trois sommets; toutes les arêtes de $T$ appartiennent donc à une même composante de facettes. Enfin, pour toute paire $\left\lbrace a,b\right\rbrace$ qui n'est pas une arête de $T$, choisissons un voisin $c$ de $a$ dans $T$. Le triplet $\left\lbrace a,b,c\right\rbrace$ contient l'arête de Delaunay $\left\lbrace a,c\right\rbrace$ et relie la facette $\left\lbrace a,b\right\rbrace$ à la composante précédente. Toutes les paires de $S$ sont donc actives et connexes dans la filtration restreinte au plus tard au niveau $r^{2}$.

Les trois facettes de chaque coface Gamma exhaustive sont ainsi déjà actives et reliées par des relations de $\mathcal{C}_{1}$ à un niveau qui n'est pas plus tardif. Réciproquement, $\mathcal{C}_{1}$ est une sous-famille des cofaces Gamma exhaustives et ne peut créer aucune relation étrangère. Les facettes actives et les partitions coïncident donc. Si $r^{2}<a$, toutes les relations construites restent strictement sous $a$; si $r^{2}\leq a$, elles restent dans la coupe fermée. Le résultat vaut pour les deux conventions de seuil. $\square$

Ce théorème certifie la structure combinatoire après calcul exact des niveaux. Il ne certifie pas l'ordre produit par les seuls rayons binary64.

L'égalité des coupes implique la même généalogie de naissances, prolongements et multifusions de la forêt `hgp_reduced`. Elle n'implique pas l'égalité des catalogues de cofaces : les triplets sans arête de Delaunay, redondants pour cette généalogie, restent absents de $\mathcal{C}_{1}$. Le contrat v2 actuel exige encore `gamma_exhaustive_reference`; publier cette réduction comme nouvelle base de preuve demanderait donc une migration contractuelle explicite et les raccords de verticalité et de M.1, pas seulement le théorème horizontal.

La fixture positive [`delaunay_one_edge_gamma2_positive_n9.json`](../../tests/fixtures/regressions/delaunay_one_edge_gamma2_positive_n9.json) vérifie que la réduction n'est pas seulement exacte lorsque $\mathcal{C}_{1}$ coïncide accidentellement avec Gamma. Sur neuf points, elle omet exactement deux cofaces sans arête de Delaunay, conserve 82 triplets sur 84 et reproduit 168 états stricts ou fermés. Le checker réénumère rationnellement tous les circumballs de quatre points, exige l'égalité entre les tétraèdres stockés et le catalogue exact des sphères vides, puis rejoue toutes les miniboules avec `Fraction`.

## 4. Triangles de Gabriel et lot wedge nécessaire

Notons $\mathcal{W}_{2}=\left\lbrace Q\in\binom{X}{3}:\lvert E(Q)\cap E(G_{\mathrm{Del}})\rvert\geq2\right\rbrace$ la famille des wedges historiques. Sous position générale, sans égalité extérieure sur la miniboule et avec le 1-squelette complet du nerf de Voronoï, tout triangle de Gabriel appartient à $\mathcal{W}_{2}$.

Si le support minimal de la miniboule a cardinal trois, son centre appartient aux trois cellules de Voronoï des sommets : le triangle est une face du complexe de Delaunay et ses trois arêtes sont de Delaunay. Si le support a cardinal deux, les deux points supports sont sur la sphère et le troisième sommet est strictement intérieur. La vacuité de Gabriel et l'absence d'égalité extérieure donnent alors $X\cap B=Q$. Le lemme 1 impose que le graphe de Delaunay induit par ces trois sommets soit connexe; il contient donc au moins deux arêtes. Un support non unique, un angle droit, un point extérieur sur la sphère ou une triangulation SoS qui ne restitue pas le 1-squelette complet sort de ce domaine certifié.

Cette inclusion autorise un gate massif plus faible que l'exactitude de Gamma$_2$. Pour tout triangle de Gabriel $Q$ de niveau exact $a=\beta(Q)$, le critère demandé possède exactement deux issues sûres :

- `necessary`, donc conservé explicitement, si ses trois facettes ne sont pas déjà dans une même composante stricte;
- `strictly_lower_connected`, donc omissible du lot explicite, si ses trois facettes appartiennent déjà à une même composante construite à des niveaux exacts strictement inférieurs à $a$.

Le deuxième cas ne change aucune composante au niveau $a$. La stricte antériorité est obligatoire : une connexion obtenue seulement sur le plateau $a$ ne permet pas d'écarter $Q$ avant la contraction atomique du lot. Le diagnostic `gabriel-coverage-only` implémente cette décision pour les propositions `gabriel_binary64` : il trie leurs niveaux, évalue toute la DSU avant les unions du plateau courant, puis impose `accepted = necessary + strictly_lower_connected`. Toute proposition ambiguë est conservée dans un lot de sécurité séparé et ne peut connecter aucun autre triangle. Ce rejeu ne réduit pas Gamma$_2$; comme le classifieur et les niveaux restent binary64, il constitue un sidecar conditionnel et non un certificat exact de Gabriel. Une même valeur exacte peut notamment être évaluée en deux binary64 distincts; sans intervalles ou rejeu exact, le deuxième record pourrait alors être déclaré à tort strictement postérieur.

Ce lot ne matérialise ni le produit « arête de Delaunay fois tous les points », ni Gamma exhaustif. Il énumère seulement les wedges depuis les listes d'adjacence de Delaunay, par plages bornées de sommets, et peut être représenté par la liste d'arêtes canonique, la règle de propriété des wedges et des segments de classification vérifiables. L'inclusion des triangles de Gabriel dans cet univers est exacte uniquement sous les hypothèses précédentes; elle ne rend pas la restriction wedge exacte pour `hgp_reduced`, puisque des cofaces non-wedge peuvent encore porter des incidences nécessaires. En l'absence de classifieur et d'ordre exacts, cet univers wedge complet est le grand lot conservateur; le sous-lot binary64 n'est qu'une optimisation à recertifier. Les campagnes G4 à 50 000, 10 000 001 et 30 000 001 points, leurs digests indépendants du découpage et leurs limites sont consignés dans [`PHASE15_GABRIEL_COVERAGE_G4.md`](../validation/PHASE15_GABRIEL_COVERAGE_G4.md). Le run 50 k emploie une graine distincte du benchmark canonique historique et ne le recertifie pas.

## 5. Deux réfutations permanentes

La restriction historique à deux arêtes est fausse dès six points génériques. La fixture [`delaunay_two_edge_gamma2_counterexample.json`](../../tests/fixtures/regressions/delaunay_two_edge_gamma2_counterexample.json) possède vingt cofaces Gamma$_2$; dix-huit seulement sont des wedges. Les cofaces omises $124$ et $245$, toutes deux au niveau carré $281/4$, doivent activer la facette $24$. À la coupe fermée correspondante, la couverture de points reste correcte mais l'ensemble actif et la composante de facettes sont faux.

Les fermetures locales de rayon graphe deux sont également insuffisantes. La fixture générique [`delaunay_local_gamma2_counterexample_n8.json`](../../tests/fixtures/regressions/delaunay_local_gamma2_counterexample_n8.json) montre que l'étoile fermée d'un sommet, les cliques du carré du graphe et le fan construit depuis les faces du link gardent chacun cinquante cofaces sur cinquante-six mais omettent simultanément $014$, $016$ et $017$. La facette $01$, dont les extrémités sont à distance trois dans le graphe de Delaunay, manque au niveau carré exact $13\,956\,479\,554$. La couverture de points ne détecte toujours pas cette erreur.

Ces deux cas sont audités à chaque exécution de `tools/check_phase14_geogram_low_order.py`. Ils imposent de comparer les facettes, leurs composantes et les coupes ouvertes comme fermées; un digest de la seule union de points est insuffisant.

## 6. Coût et frontière produit

Si la triangulation ordinaire possède $m$ arêtes, l'énumération directe « une arête plus un troisième site » produit jusqu'à $m(n-2)$ occurrences avant déduplication. Sur le nuage 50 k déjà mesuré, $m=385\,152$, soit $19\,256\,829\,696$ occurrences potentielles. Le seul univers final des facettes possède $\binom{50\,000}{2}=1\,249\,975\,000$ éléments. À 10 000 001 points, les 77 589 517 arêtes observées donneraient 775 895 092 410 483 occurrences potentielles.

La restriction $\mathcal{C}_{1}$ est donc une preuve de suffisance de l'information Delaunay ordinaire, pas un algorithme produit compatible avec 100 ms. La matérialiser violerait l'invariant d'architecture de MorseHGP3D aussi sûrement qu'un renommage de Gamma exhaustif. Le chemin produit doit exploiter implicitement la connexité radiale, ne découvrir que les attaches ou événements utiles à la réduction hiérarchique, recertifier chaque absence par branch-and-bound et conserver l'oracle exhaustif borné uniquement pour la falsification.

En conséquence :

- l'ordre un reste l'EMST exact extrait du graphe de Delaunay, avec recertification des niveaux et traitement explicite des dégénérescences;
- l'ordre deux possède désormais une réduction combinatoire exacte à $\mathcal{C}_{1}$ sous position générale;
- le radix sort GPU des seuls wedges à deux arêtes n'est plus un jalon scientifique valide pour prétendre reconstruire Gamma$_2$; l'énumération wedge reste utile au sidecar conditionnel de couverture Gabriel;
- aucune variante Delaunay explicite testée ne qualifie le p95 `warm_e2e` 50 k/$K=10$;
- `deployment_status=architecture_only` et `public_status=not_claimed` restent inchangés.
