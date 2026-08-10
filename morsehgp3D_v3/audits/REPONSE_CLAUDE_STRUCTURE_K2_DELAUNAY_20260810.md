# Réponse à Claude — garde-fou Delaunay léger pour $k=2$

Date : 10 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`, `backend=diagnostic_delaunay_plus_reference_cpu`, `profile=hgp_reduced_order_2`, `mode=recall_guardrail_only`, `public_status=not_claimed`.

## Décision pratique

Pour le besoin précisé par Louis — un **garde-fou de rappel**, pas une seconde implémentation exacte — le lot des triangles formés par deux arêtes Delaunay incidentes est une bonne sentinelle à 50 k. Il a déjà une masse praticable, autour de 4,4 millions de wedges sur le nuage mesuré, et chaque triplet proposé fournit une vraie échéance Gamma$_2$ indépendamment de l'exactitude de ses deux étiquettes Delaunay.

En effet, pour **n'importe quel** triplet distinct $Q=\lbrace a,b,c\rbrace$, le simplexe de Čech $Q$ est présent au niveau de sa miniboule. Ses trois facettes $ab$, $ac$ et $bc$ doivent donc être actives et dans une même composante Gamma$_2$ à cette coupe fermée. La Delaunay ne sert ici qu'à choisir quelques millions de triplets géométriquement pertinents parmi $\binom{n}{3}$; une fausse arête Delaunay ne crée pas une fausse obligation Gamma, et une arête manquante réduit seulement la couverture diagnostique.

Le test recommandé est unilatéral : pour chaque wedge unique $Q$, calculer son niveau $a_Q$ puis le premier niveau $a_{\mathrm{sujet}}(Q)$ auquel les trois facettes sont réunies dans la forêt $k=2$ du sujet.

- `connected_before` si $a_{\mathrm{sujet}}(Q)<a_Q$ ;
- `connected_at` si $a_{\mathrm{sujet}}(Q)=a_Q$ ;
- `late` si $a_{\mathrm{sujet}}(Q)>a_Q$ ;
- `never` si une facette n'est jamais activée ou si les trois facettes restent dans des racines distinctes ;
- `unsupported` si le niveau ou le lookup de facette est indécidable dans le mode diagnostique choisi.

`connected_before` et `connected_at` passent. `late` et `never` mesurent directement ce que le prototype manque sur cet échantillon. Le mutant `omit-all-fusions` donne presque tout `never` et ne peut donc pas passer par vacuité.

L'exactitude des **arêtes Delaunay** n'est pas une précondition de ce garde-fou. Il est néanmoins peu coûteux de calculer $a_Q$ avec la miniboule exacte u16 déjà disponible, afin que la mesure de retard ne soit pas polluée par les égalités binary64. Si même le niveau reste flottant pour le premier diagnostic, les comparaisons ambiguës sont classées `unsupported`, jamais créditées comme des succès.

## Implémentation du garde-fou sans rejeu par coupe

Il ne faut pas rejouer la DSU pour chacun des 4,4 millions de triangles. Construire une fois la forêt de fusion `k=2` et un index de premier carrier pour chaque facette de deux points, puis répondre par LCA : le niveau du plus petit ancêtre commun des trois carriers est $a_{\mathrm{sujet}}(Q)$. Une table de binary lifting donne $O(\log N)$ par wedge; une numérotation Euler plus RMQ permet $O(1)$ après prétraitement. Les lots de niveaux égaux restent multifurqués et atomiques.

Le reçu doit publier au minimum :

- digest des points, de la liste d'arêtes diagnostique, des wedges uniques et de la forêt sujet ;
- propositions brutes, triplets uniques, doublons et triplets invalides ;
- facettes trouvées/manquantes dans l'index de carriers ;
- les cinq classes ci-dessus, globalement et par quantile de niveau ;
- quantiles du retard pour `late`, plus les pires témoins exacts ;
- résultats stratifiés par degré Delaunay du sommet central et par point, afin qu'une région très dense ne masque pas une région entièrement oubliée ;
- high-water, temps d'énumération, de miniboule, de lookup/LCA et total du sidecar, tous hors `warm_e2e` produit.

Le taux brut par wedge est utile mais biaisé par les sommets de grand degré. Publier aussi la fraction de points centraux possédant au moins un `late/never`, le p95 du taux d'échec par point et le nombre de couples de racines sujet distincts violés. Aucun seuil scientifique n'est inventé ici : la première campagne fixe la distribution de référence, puis Louis peut choisir le budget de rappel acceptable. Le sidecar reste `diagnostic_recall_only` quel que soit son score.

Fixtures et mutants minimaux : triplet connecté exactement au niveau, plus tôt, plus tard et jamais; lot multifusion; facette absente; wedge dupliqué par ses trois centres; permutation des `PointId`; dernier wedge omis; LCA pris avant le lot fermé; niveau décalé; et suppression de toutes les fusions.

## Frontière mathématique à ne pas confondre avec ce diagnostic

La question d'une **reconstruction exacte** est différente et déjà tranchée par [`docs/math/DELAUNAY_ORDINAIRE_GAMMA2.md`](../../docs/math/DELAUNAY_ORDINAIRE_GAMMA2.md).

1. Les triangles ayant au moins deux arêtes Delaunay ne reconstruisent pas toute Gamma$_2$. La fixture générique permanente `delaunay_two_edge_gamma2_counterexample.json` omet les cofaces $124$ et $245$ au niveau exact $281/4$ et n'active pas la facette $24$. Cela limite l'interprétation du score; cela n'invalide pas le garde-fou demandé.
2. Sous position générale, avec le 1-squelette **complet** du complexe de Delaunay ordinaire, la famille exacte conserve les triplets ayant **au moins une** arête Delaunay :

$$\mathcal{C}_{1}=\left\lbrace Q\in\binom{X}{3}:E(Q)\cap E(G_{\mathrm{Del}})\neq\varnothing\right\rbrace.$$

Cette restriction possède exactement les mêmes facettes actives et les mêmes composantes que Gamma$_2$ exhaustive à toute coupe stricte et fermée, pour `include_isolated=false`. Elle donne donc la même généalogie horizontale réduite.
3. Ce théorème n'est pas un analogue industriel de l'EMST. Sur le nuage 50 k déjà mesuré, $m=385\,152$ arêtes Delaunay donnent jusqu'à $m(n-2)=19\,256\,829\,696$ occurrences avant déduplication, et l'univers des facettes contient $\binom{50\,000}{2}=1\,249\,975\,000$ paires. Il s'agit d'une autorité d'oracle ou d'un lemme de complétude, jamais du garde-fou compact ni d'un backend chaud.
4. Le théorème vise Gamma$_2$ complète. Il ne transforme pas le catalogue courant `smax=11` en source complète et ne borne pas le rang fermé d'un triangle par `smax`. Cela n'empêche pas de mesurer combien de deadlines wedge la forêt partielle respecte.

## Pourquoi une arête suffit mathématiquement

Pour toute boule fermée $B$, le sous-graphe Delaunay ordinaire induit par $S=X\cap B$ est connexe. Un arbre couvrant $T$ de ce sous-graphe relie toutes les paires de $S$ par des triplets contenus dans $S$ : deux arêtes adjacentes de $T$ donnent un triplet, et toute paire hors $T$ se raccorde à une arête incidente de $T$. Chacun de ces triplets contient une arête Delaunay et sa miniboule a un niveau au plus égal à celui de $B$.

C'est une preuve de suffisance de l'information Delaunay, pas une prescription d'énumérer `arête × troisième point`. La voie produit doit exploiter la connexité radialement et à la demande, sans matérialiser $\mathcal{C}_{1}$ ni les facettes globales.

En présence de cosphéricités, le lemme porte sur le 1-squelette complet du nerf de Voronoï. Une triangulation particulière issue d'un SoS peut omettre des arêtes. Un vérificateur u16 doit donc recertifier les arêtes nécessaires ou refuser `unsupported_degeneracy`; il ne peut pas hériter silencieusement de la triangulation choisie.

## Si un certificat exact est demandé plus tard

Le sidecar de rappel ci-dessus est déjà non vacue parce que sa source de wedges est indépendante de la liste des fusions du sujet. Si un certificat exact est demandé ultérieurement, une simple inégalité attachée aux seules fusions sujet ne suffira pas : `omit-all-fusions` la satisferait par vacuité. Il faudra alors recevoir les deux côtés de chaque lot exact.

Pour un lot de niveau $a$, un certificat de **solidité** peut porter :

- les représentants des composantes strictes avant le lot ;
- un forest de triplets réels, chacun contenant au moins une arête Delaunay recertifiée ;
- le niveau exact de chaque triplet, au plus $a$, et au moins une arête du forest au niveau exact $a$ pour chaque connexion nouvelle ;
- les composantes fermées obtenues après rejeu atomique du forest.

Le rejeu doit retrouver exactement les connexions publiées par le sujet. Ce certificat prouve que chaque fusion publiée existe dans Gamma$_2$, mais il ne prouve pas qu'aucune fusion n'a été omise.

La **complétude** exige en plus une autorité indépendante des coupes : soit les partitions strictes et fermées de l'oracle à une arête, soit un transcript indépendant qui engage toutes ses fusions attendues. Les obligations minimales sont alors : égalité des partitions de facettes à chaque lot, égalité du nombre pondéré de contractions et couverture de tous les niveaux critiques de l'oracle. Le mutant prioritaire supprime toutes les fusions, puis un second supprime uniquement la dernière coface nécessaire d'un lot multifusion. Comparer seulement l'union des points ne mord pas : les deux fixtures permanentes conservent cette couverture tout en donnant une histoire de facettes fausse.

Pour la porte `k=1`, la même discipline implique de comparer les partitions exactes de `PointId` après chaque lot EMST, pas seulement le multiensemble des niveaux et la somme des arités. Un mutant qui reconnecte les mauvaises composantes aux mêmes niveaux teste cette distinction.

## Conséquence pour le contrat 50 k

Le sidecar wedge peut servir de contrôle structurel hors chrono chaud et signaler rapidement une régression massive de rappel. Il ne certifie ni la source tronquée, ni les ordres $3\ldots10$, ni le SLO. La priorité produit reste un flux de seules attaches utiles et une réduction en ligne. Porter les wedges à deux arêtes sur GPU est donc un bon jalon de **garde-fou quantitatif**, simplement pas un jalon d'exactitude Gamma$_2$.

GCP non utilisé.
