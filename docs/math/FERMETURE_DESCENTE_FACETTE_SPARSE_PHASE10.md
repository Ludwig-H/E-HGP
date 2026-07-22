# Fermeture mémoïsée des descentes de facettes sparse — Phase 10.5c

## Portée et statut

Phase 10, backend `reference_cpu`, profil `hgp_reduced`, mode `certified`, sémantique interne `partial_refinement`. La porte d'entrée de la phase est satisfaite et le jalon 10.5c est implémenté puis validé sur hôte dans la portée bornée décrite ici. Le `public_status` reste `not_claimed`.

Le jalon 10.5c compose exclusivement des résultats du pas exact 10.5b pour construire une forêt fonctionnelle multi-source de descentes strictes. Il partage les suffixes par clé complète de facette, propage les seules résolutions positives relatives fournies par le locator gelé et ne publie encore aucun `GatewayAttach` ni aucune mutation de forêt d'ordre supérieur.

Le chemin ne construit ni coupe Gamma, ni catalogue global de facettes, ni coface globale, ni incidence Gamma globale, ni cellule top-$m$, ni mosaïque de Delaunay d'ordre supérieur. Gamma exhaustif reste uniquement un oracle de falsification sur le domaine borné.

## Entrées et autorité commune

Un appel traite un unique ordre $k$, un unique niveau fermé exact $a$ et une famille de références de graines. Toutes les graines sont des facettes canoniques de même cardinal $k$ avec $1\leq k\leq10$. Les doublons d'entrée restent des références distinctes mais partagent le même sommet interne.

La clé complète d'une facette est

$$\kappa(F)=(k,p_0,\ldots,p_{k-1}),\qquad p_0<\cdots<p_{k-1}.$$

Les autorités fiables de l'appel sont :

- le nuage canonique ;
- le LBVH validé pour ce nuage ;
- le niveau fermé exact $a$ ;
- l'ordre de parcours LBVH ;
- un budget 10.5b commun ;
- un locator positif certifié ;
- un témoin de requête commun dont `external_authority_id` et `replay_token` sont non nuls ;
- un `LocatorSnapshotStamp` commun à toute la fermeture.

Le stamp v1 est le tuple sans allocation formé de la version de schéma, de l'identifiant d'autorité externe, des compteurs monotones de lots committés, clés insérées, unions de composantes et requêtes de liaison, puis d'un `CanonicalId` SHA-256. Ce digest est une chaîne séparée par domaine : l'état initial engage le schéma, les capacités, la configuration et le nombre de handles; chaque lot engage le digest précédent, son index, ses compteurs, ses faits durables, ses unions ordonnées et ses nouvelles liaisons ordonnées avec leurs clés complètes, handles et témoins. Dans un locator donné, tout `apply_batch` committé, même vide, avance le compteur de lots et calcule le maillon suivant de la chaîne. Le vérificateur structurel rejoue cette chaîne depuis les arènes durables. Sous l'hypothèse standard d'absence de collision SHA-256 sur les transcriptions canoniques considérées, le stamp distingue deux états de mêmes autorités et compteurs mais de contenus sémantiques différents. Sans cette hypothèse, cette divergence n'est pas prouvée. La valeur SHA-256 tout-zéro est valide et n'est jamais une sentinelle. Le stamp ne rejoue pas le sens géométrique de l'autorité externe.

Le locator, ses sondes, `snapshot_stamp` et `apply_batch` ne sont pas synchronisés. L'appelant doit sérialiser les writers et maintenir une exclusion externe de toute mutation du locator pendant **toute** la construction, depuis avant la première lecture du stamp jusqu'au retour du builder. Il doit maintenir la même exclusion pendant **toute** la vérification, depuis avant la lecture du stamp courant jusqu'à la fin du rejeu frais et au retour du vérificateur. En particulier, aucun `apply_batch` ne peut chevaucher l'une de ces deux opérations. L'emprunt `const` du builder ne fournit pas cette exclusion.

Sous cette précondition, le builder capture le stamp avant la première géométrie, le compare après chaque appel 10.5b et le compare encore immédiatement avant le retour. Le vérificateur exige l'égalité entre le stamp courant et celui du résultat observé avant tout rejeu scientifique. Ces comparaisons sont uniquement des gardes de mutation séquentielle : elles refusent notamment un résultat si un commit s'est achevé après sa construction et avant une vérification ultérieure correctement sérialisée. Elles ne créent pas de snapshot et ne détectent jamais sûrement un writer concurrent; un tel chevauchement sort du contrat certifié et peut constituer une data race.

Le locator ne rejoue pas l'autorité externe des liaisons positives. Même avec un stamp valide, le sens HGP d'un hit reste `conditional_on_caller_external_replay`.

## Objet combinatoire

Notons $R$ le nombre de références de graines, doublons compris, $S$ le nombre de graines distinctes, $V$ le nombre de sommets distincts internés, $B$ le nombre de sommets effectivement soumis à 10.5b, $E$ le nombre d'arêtes strictes admises et $T$ le nombre de sommets sans successeur dans le graphe publié.

Chaque sommet possède une clé complète unique. Toute cible d'une arête est internée comme sommet, y compris lorsqu'elle est déjà positive dans le locator. Une empreinte peut accélérer l'internement, mais toute identité exige l'égalité du cardinal et de chacun des au plus dix `PointId`.

$$\kappa(F)=\kappa(F')\Longrightarrow\mathrm{node}(F)=\mathrm{node}(F').$$

Un sommet est évalué par 10.5b au plus une fois. Le résultat local 10.5b reste immuable. Plusieurs graines ou plusieurs arêtes entrantes peuvent ensuite référencer le même sommet et partager son suffixe.

Chaque arête $F\to G$ conserve une projection compacte du témoin strict 10.5b et satisfait

$$F\to G\Longrightarrow\beta(G)<\beta(F)\leq a.$$

La forêt est orientée des graines vers ses terminaux. Son degré sortant vérifie

$$\deg^{+}(v)\in\left\lbrace 0,1\right\rbrace.$$

## Décisions 10.5b admissibles

Une arête scientifique est admise exclusivement à partir de l'une des deux décisions complètes suivantes :

- `complete_unresolved_strict_successor_not_bound` : le successeur strict a subi un miss locator complet ; sa clé est internée et le sommet est placé dans la frontière s'il n'est pas déjà fermé ou planifié ;
- `complete_relative_strict_successor_positive_hit` : le successeur strict a subi un hit locator complet ; sa clé est internée comme terminal positif et aucun appel 10.5b supplémentaire n'est lancé pour ce terminal.

Le source hit `complete_relative_source_positive_hit` ferme directement un terminal positif sans requête top-$k$ et sans arête.

Les décisions suivantes ferment un terminal `unresolved` sans arête :

- `complete_unresolved_source_above_closed_batch_level` ;
- `complete_unresolved_source_is_canonical_top_k_choice` ;
- `complete_unresolved_non_strict_canonical_successor`.

Un choix canonique identique à la source ne prouve pas ici que la facette est active. Un successeur distinct de niveau non strict n'est ni contracté ni renommé en plateau pris en charge. Ces deux branches restent `unresolved`.

Les décisions suivantes ferment un terminal opérationnel `budget_exhausted` sans arête :

- `no_resolution_source_locator_probe_budget_exhausted` ;
- `no_resolution_top_k_budget_exhausted` ;
- `no_resolution_successor_locator_probe_budget_exhausted` ;
- tout épuisement du plafond de sommets ou d'appels de la fermeture.

Le témoin strict éventuellement présent après l'épuisement de la sonde locator du successeur reste un diagnostic exact individuel. Dans le contrat v1, il ne devient pas une arête et ne permet pas de contourner silencieusement ce budget par un nouvel appel sur la cible.

Une décision 10.5b de contradiction, une couture géométrique incohérente ou un cycle empoisonne le résultat entier : les vecteurs scientifiques sont vidés et seul le diagnostic de contradiction demeure. Un désaccord de stamp observé séquentiellement sous le contrat d'exclusion suit la règle plus forte précédente et interdit tout retour de résultat.

## Absence de demi-arête et budgets transactionnels

Une arête n'est engagée que si sa source existe et si sa cible est déjà internée ou peut l'être sous le plafond de sommets dans le même commit logique. L'arête et une éventuelle nouvelle cible sont préparées puis engagées ensemble. Comme $E<V$ pour tout résultat non vide, le plafond de sommets borne aussi le vecteur d'arêtes sans capacité globale séparée.

Si la capacité manque après un résultat 10.5b strict complet, la source devient un terminal `graph_budget_exhausted` et le témoin reste diagnostique. Aucune arête sans cible et aucun sommet cible sans provenance ne sont publiés.

Les graines distinctes constituent les points d'entrée de la couverture demandée ; une graine peut aussi être atteinte depuis une autre graine et n'est donc pas nécessairement une racine du graphe. Après validation des références et déduplication par clé complète, le préflight exige $S$ inférieur ou égal au plafond de sommets. Le builder interne ensuite les $S$ graines distinctes, dans l'ordre canonique des clés, avant le premier appel géométrique 10.5b. Un plafond inférieur à $S$ produit un épuisement de préflight avec graphe vide, jamais une forêt partielle.

Un sommet déjà interné mais non encore évalué devient un terminal `graph_budget_exhausted` lorsque le budget global d'appels est épuisé. Après tout arrêt budgétaire d'exécution, toutes les graines pré-internées encore `unseen` sont fermées de cette manière avant la projection des références. Ce statut ne signifie ni absence, ni isolation, ni naissance.

Les plafonds de confiance de la fermeture, le budget commun de chaque pas et le plafond du nombre d'appels bornent le travail total. Les arêtes et la frontière n'ont pas de caps ABI redondants : $E<V$ borne les arêtes par `maximum_node_count`, et chaque entrée de frontière référence un nœud interné unique. Les sept dimensions du top-$k$ et les deux dimensions de chaque sonde locator restent séparément visibles. Les additions d'agrégats scientifiques emploient une arithmétique vérifiée. Le callback de cache imposé `noexcept` utilise une addition non levante qui mémorise tout débordement; le builder contrôle ce verrou immédiatement après le retour du noyau 10.5b et échoue fermé avant toute publication.

## Théorème de forêt fonctionnelle

### Énoncé

Supposons que chaque clé possède une unique miniball exacte factorisée, que chaque arête est issue d'une des deux décisions complètes admissibles et que toutes les coutures utilisent cette miniball unique. Alors le graphe publié est une forêt fonctionnelle convergente. Pour $V>0$, il vérifie $E=V-T<V$.

### Preuve

Chaque sommet est évalué au plus une fois par le choix canonique 10.5b ; il possède donc au plus une arête sortante. Sur toute arête $F\to G$, le témoin rejoué impose $\beta(G)<\beta(F)$. Un cycle dirigé donnerait une chaîne finie d'inégalités strictes revenant au même niveau exact, ce qui est impossible.

Un graphe fini de degré sortant au plus un et sans cycle possède exactement un sommet sans successeur dans chacune de ses composantes connexes sous-jacentes. La somme des degrés sortants compte les arêtes, tandis que chaque sommet non terminal contribue exactement un. Ainsi

$$E=\sum_{v\in\mathcal{V}}\deg^{+}(v)=V-T.$$

Pour une forêt non vide, $T\geq1$, donc

$$V>0\Longrightarrow 1\leq T\leq V,\qquad 0\leq E=V-T<V.$$

Pour l'entrée vide autorisée, $V=E=T=0$. La borne uniforme est donc $E\leq V$, avec l'inégalité stricte dès que la forêt contient un sommet. La représentation normative doit publier l'identité plus forte $E=V-T$.

Chaque arête possède une source évaluée distincte, et chaque sommet est évalué au plus une fois. Par conséquent

$$E\leq B\leq V.$$

## Cache factorisé de miniballs

10.5c appelle le noyau privé certifié de 10.5b, qui accepte une miniball source déjà préparée et un lookup sans allocation pour la cible. Une miniball fraîchement construite comme source est placée dans le cache indexé par les sommets. Toute miniball fraîchement construite comme successeur est placée dans un second cache plat, indexé par clé complète, que le successeur soit strict, non strict, interné ou non interné.

Le lookup consulte ces deux caches avant toute construction. Ainsi une cible qui devient ensuite source réemploie exactement sa miniball certifiée ; la couture ne la reconstruit plus. Plusieurs arêtes entrantes vers une même cible retrouvent le même centre, le même niveau et le même support. Une seconde construction fraîche pour la même clé ou une couture dont le centre ou le niveau diverge est un échec fermé.

Si $C_{\mathrm{cache}}$ désigne `distinct_cached_miniball_count`, les compteurs certifient l'identité

$$C_{\mathrm{cache}}=N_{\mathrm{source\_build}}+N_{\mathrm{successor\_build}}\leq2B\leq2V.$$

Dans les noms du schéma, cette identité est `distinct_cached_miniball_count = source_miniball_build_count + successor_miniball_build_count`. Les réemplois source et successeur sont comptés séparément et ne contribuent pas une seconde fois à $C_{\mathrm{cache}}$. Le résultat public ne persiste aucune miniball complète : un sommet conserve seulement sa clé, son centre et son niveau exacts éventuels, sa projection locale et ses références de graphe. Les caches de miniballs sont un scratch détruit avec le builder.

Chaque sommet source distinct déclenche au plus une requête top-$k$ 10.5b. La `TopKPartition`, son shell et sa frontière restent transitoires et sont évincés avant le traitement du sommet suivant. Ni partition top-$k$, ni shell, ni frontière ne sont persistés dans le graphe.

## Partage des suffixes et propagation

Les références de graines sont d'abord remises dans l'ordre de leurs identifiants durables, puis leurs clés sont validées, triées et dédupliquées. Toutes les clés distinctes sont internées avant la géométrie. Le parcours des points d'entrée et la frontière des sommets non évalués suivent ainsi l'ordre canonique des clés. La permutation physique des records d'entrée ne change ni les sommets, ni les arêtes, ni les terminaux ; elle ne change pas non plus la projection triée par identifiant durable.

Lorsqu'une arête atteint un sommet déjà interné, aucun second appel 10.5b n'est effectué pour cette clé. L'événement est compté comme `memoized_suffix_reuse`, pas comme un hit locator. Le suffixe déjà certifié est partagé par toutes les provenances entrantes.

Le résultat local et le résultat de fermeture sont distincts :

- `local_step_decision` est la décision 10.5b immuable du sommet ;
- `closure_disposition` est déduite du terminal unique de sa composante fonctionnelle.

Un terminal positif fournit un handle de composante et un témoin de liaison relatifs. Une passe en ordre topologique inverse propage exactement ce couple à tous ses ancêtres. Un terminal `unresolved` ou `budget_exhausted` propage la même absence de résolution, sans handle inventé.

Deux chemins qui rejoignent le même sommet partagent nécessairement tout leur suffixe et la même disposition de fermeture. Deux payloads positifs différents pour le même terminal exact constituent une contradiction du locator ou de son autorité.

## Hits locator et compteurs top-k

Un source hit est un terminal évalué par 10.5b. Il compte une sonde source, zéro requête top-$k$, zéro sonde successeur et zéro arête.

Un successor hit est contenu dans l'appel 10.5b du prédécesseur. Il compte la requête top-$k$ de cette source, une sonde successeur et une arête vers un terminal positif interné. Le terminal cible ne déclenche aucun appel 10.5b ni aucune requête top-$k$ supplémentaire.

Un partage de suffixe ne compte aucun hit locator supplémentaire. Les compteurs séparent au minimum :

- `step_call_count` ;
- `source_locator_probe_count` ;
- `top_k_query_count` ;
- `successor_locator_probe_count` ;
- `source_positive_hit_count` ;
- `successor_positive_hit_count` ;
- `distinct_positive_terminal_count` ;
- `memoized_suffix_reuse_count` ;
- `strict_edge_count` ;
- `diagnostic_strict_witness_without_edge_count` ;
- `distinct_cached_miniball_count`, `source_miniball_build_count`, `source_miniball_reuse_count`, `successor_miniball_build_count` et `successor_miniball_reuse_count` ;
- les terminaux `unresolved`, `budget_exhausted` et `graph_budget_exhausted`.

Si $Q_{\mathrm{topk}}$ est le nombre de requêtes top-$k$ lancées, on a

$$E\leq Q_{\mathrm{topk}}\leq B\leq V.$$

Si $H_{\mathrm{source}}$ et $H_{\mathrm{successor}}$ comptent les hits positifs, leur somme compte les opérations et non les terminaux distincts :

$$H_{\mathrm{locator}}=H_{\mathrm{source}}+H_{\mathrm{successor}}.$$

Plusieurs hits successeur peuvent viser le même terminal positif. Le nombre de terminaux positifs distincts peut donc être strictement inférieur à $H_{\mathrm{locator}}$.

## Théorème de composition des chemins

### Énoncé

Soit une graine $F_0$ dont le suffixe finit en un terminal positif $F_L$. Alors les témoins 10.5b se composent en un chemin géométrique depuis le germe source vers la composante positive de $F_L$, relatif à l'autorité externe du locator. Après retrait du premier point du premier segment, ce chemin reste strictement sous le niveau fermé $a$.

### Preuve

Pour chaque arête $F_i\to F_{i+1}$, 10.5b certifie un segment source-ouvert strictement sous $\beta(F_i)$ et sous $a$. La suite des niveaux vérifie

$$\beta(F_L)<\cdots<\beta(F_1)<\beta(F_0)\leq a.$$

La cible géométrique d'une arête est la source géométrique de la suivante par l'égalité exacte des clés, centres et miniballs factorisées. Tous les segments après le premier sont donc entièrement sous $\beta(F_0)$, et le premier l'est après retrait de son extrémité source. Le terminal positif fournit ensuite la composante relative atteinte. Le partage d'un suffixe ne modifie aucun segment ni aucune couture. $\square$

Ce théorème certifie seulement les graines dont le terminal est positif. Un terminal `unresolved` ou `budget_exhausted` ne fournit qu'un préfixe exact et aucune attache de hiérarchie.

## Bornes de stockage

Le résultat persiste :

- $R$ projections de références de graines vers des sommets ;
- $V$ sommets, au plus $KV$ références de `PointId` avec $K\leq10$, et leurs centres et niveaux exacts éventuels ;
- $E$ arêtes et leurs témoins compacts ;
- $T$ terminaux, dont seuls les terminaux positifs portent un handle et un témoin externe.

Le stockage logique propre à la fermeture vérifie

$$M_{\mathrm{logique}}=O(R+KV+E+T),\qquad K\leq10.$$

Comme $E=V-T$ et $T\leq V$, on obtient

$$M_{\mathrm{logique}}=O(R+KV).$$

Cette borne compte les enregistrements et références du résultat public, qui ne contient aucune miniball complète. Elle ne borne pas à elle seule le nombre de limbs des rationnels exacts ; les octets, limbs, frontières top-$k$ transitoires et pics d'allocation restent des compteurs obligatoires.

Le scratch contient deux tables ouvertes de taille `required_memo_slot_count`, les caches factorisés de $C_{\mathrm{cache}}\leq2V$ miniballs, la frontière canonique, les couleurs de cycle, les tableaux de propagation et une seule exécution 10.5b transitoire. Les tables sont dimensionnées depuis le plafond fiable de sommets, pas depuis $\binom{n}{k}$. Il n'existe jamais une partition top-$k$ par sommet, une coface globale ou une arène indexée par $\binom{n}{k}$.

## Vérificateur frais

Le vérificateur reçoit séparément les autorités, graines, budgets et caps fiables. Aucun champ du graphe observé ne choisit les sommets à développer, les arêtes à suivre ou les allocations du rejeu frais. L'exclusion externe décrite plus haut est une précondition de l'appel complet; le vérificateur n'acquiert aucun verrou et `apply_batch` reste interdit jusqu'à son retour.

Il effectue les opérations suivantes :

1. vérifier le nuage, le LBVH, le niveau, l'ordre de parcours, le témoin commun, le locator et les budgets fiables ;
2. borner `observed.nodes` par `maximum_node_count`, `observed.edges` par ce même plafond grâce à $E<V$, et `observed.seed_projections` par `maximum_seed_count` ;
3. retourner immédiatement si l'un de ces trois vecteurs observés dépasse son cap, avant d'en scanner le moindre élément ;
4. vérifier que le stamp courant du locator est exactement le stamp enregistré par le résultat observé ;
5. reconstruire et trier les graines distinctes depuis les entrées fiables, puis les interner toutes avant la géométrie ;
6. reconstruire indépendamment toute la forêt avec le noyau privé certifié 10.5b et les mêmes budgets fiables ;
7. ne créer des arêtes attendues que pour les deux décisions complètes admissibles ;
8. rejouer fraîchement les sondes locator, requêtes top-$k$, caches de miniballs, comparaisons de niveaux et témoins de segment ;
9. reconstruire l'internement par comparaison de clés complètes et vérifier chaque couture factorisée ;
10. vérifier qu'aucune arête n'est publiée après un budget, un résultat non strict ou une contradiction ;
11. vérifier les identités de sommets, arêtes et terminaux, la baisse stricte, $E=V-T$, $E<V$ pour $V>0$ et $B\leq V$ ;
12. reconstruire les dispositions de fermeture et les payloads positifs par propagation inverse ;
13. comparer les compteurs 10.5b, les réemplois, le nombre de miniballs distinctes et la projection canonique complète ;
14. confirmer qu'aucune partition top-$k$, mutation du locator, isolation, naissance, singleton, attache, coface Gamma ou structure globale interdite n'a été inventée.

Le booléen `observed_storage_within_budget` atteste seulement ce préflight des trois vecteurs observés. Un checksum ou un stamp recopié depuis le résultat ne remplace jamais le rejeu depuis les autorités fiables.

Le vérificateur ne rejoue pas encore l'autorité externe des liaisons du locator. Son succès établit donc une fermeture géométrique et combinatoire exacte relativement au locator fourni, pas l'exactitude HGP absolue des handles terminaux.

## Décisions et portée implémentées

Les dispositions globales restent :

- `relative_positive` si toutes les références de graines atteignent un terminal positif ;
- `unresolved` si au moins une graine atteint un terminal non résolu et qu'aucun budget ni contradiction n'intervient ;
- `budget_exhausted` si au moins une graine dépend d'un terminal budgétaire ;
- `contradiction` si un pas local certifié, une couture partagée, un cycle ou un payload positif est incohérent.

Une autorité d'entrée invalide est rejetée avant construction. Un désaccord du stamp commun observé dans une exécution correctement sérialisée interrompt la construction ou la vérification avant toute publication ; ces deux cas ne sont pas convertis en forêt `contradiction`. Aucune propriété n'est revendiquée pour un writer concurrent.

Un résultat peut conserver individuellement les graines positivement résolues lorsque d'autres sont `unresolved` ou `budget_exhausted`, mais il ne peut appliquer aucun quotient HGP de lot tant que les graines requises par ce lot ne sont pas toutes fermées.

La base de preuve active est `fresh_10_5b_core_steps_full_facet_key_interning_exact_level_acyclic_suffix_sharing_common_frozen_locator_snapshot_v1`.

La portée active est `bounded_multi_source_memoized_strict_functional_forest_relative_positive_sink_propagation_only`.

## Tests de falsification validés

- la fixture canonique à six points ferme `F0 -> F1 -> F2` aux niveaux exacts $52$, $85/4$ et $325/16$, avec $V=3$, $E=2$, $T=1$, $B=2$ et deux requêtes top-$k$ ;
- la fixture permanente à cinq points ferme `AC -> DE` avec niveau source $33/2$, cutoff $31/2$, niveau cible $9/2$, un unique pas et le terminal positif relatif attendu ;
- les deux graines `F0` et `F1` partagent le suffixe déjà fermé, construisent exactement trois miniballs et réemploient `F1` comme source ;
- la permutation physique des graines et les parcours `near_first` et `far_first` conservent la forêt scientifique canonique ;
- un masque d'empreinte nul force les collisions, chaque collision compare la clé complète, et les références de graines dupliquées restent distinctes ;
- une source déjà positive ferme un terminal avec une sonde, zéro top-$k$, zéro miniball et zéro arête ;
- le choix canonique identique et le successeur distinct non strict ferment chacun un terminal `unresolved` sans arête ;
- les préflights de références, de slots de mémo et de graines distinctes échouent avec graphe vide ;
- les budgets de sommets, d'appels et de sonde locator successeur conservent un préfixe certifié sans demi-arête ;
- les épuisements de sonde source et de top-$k$ restent terminaux sans arête, et une source de niveau $33/2$ dans un lot fermé à $16$ reste `unresolved` sans requête top-$k$ ;
- deux clés positives distinctes de cardinal dix restent deux terminaux sous collisions forcées, avec comparaison de clé complète et sans développement combinatoire ;
- l'ordre de parcours invalide et l'autorité externe divergente sont rejetés avant construction ;
- un commit séquentiel du locator après construction et avant vérification rend le stamp observé périmé, puis le vérificateur exécuté sous exclusion externe refuse le résultat ;
- un vecteur observé surdimensionné est refusé avant stamp et avant scan, puis les mutations d'arête, terminal, niveau, compteurs, projection et portée échouent au rejeu frais ;
- le garde statique et symbolique interdit Gamma, cofaces globales, cellules top-$m$, mosaïque d'ordre supérieur et partition top-$k$ persistée dans le target isolé.

Les six CTests GCC Release fonctionnels et statiques ciblés passent en 0,99 seconde dans l'environnement local. Ils restent courts et bornés. Gamma exhaustif n'entre ni dans le builder 10.5c, ni dans son vérificateur, ni dans son exécutable unitaire.

## Limites scientifiques et produit

10.5c ne prouve pas :

- que toutes les facettes-portes nécessaires ont été proposées ;
- la complétude du premier niveau d'incidence $\lambda(F)$ ;
- l'absence de co-minimiseurs de même niveau omis ;
- la génération complète des gateways silencieux non directs ;
- la contraction des plateaux ou des dégénérescences ;
- `attachments_complete_by_order` ;
- la propriété M.1 ;
- les morphismes verticaux ;
- un `public_status=exact`.

La porte suivante reste un oracle spatial de première incidence qui propose tous les minimiseurs pertinents et certifie par branch-and-bound qu'aucune coface moins chère n'a été omise. Les co-minimiseurs de même niveau doivent être traités dans un lot atomique.

Pour environ 50 000 points, 10.5c évite le travail top-$k$ répété sur les suffixes partagés, mais une requête LBVH, un shell ou une longue chaîne peuvent encore être linéaires. Aucun SLO inférieur à une seconde n'est revendiqué.

Pour dix millions de points ou davantage, l'absence de terme $\binom{n}{k}$ et la borne $O(R+KV)$ préservent l'architecture sparse, mais elles ne qualifient ni le débit, ni la mémoire en octets des rationnels, ni la reprise durable, ni l'absence d'OOM. Aucun SLO 10 M+ n'est revendiqué.

Le jalon 10.5c est donc fermé comme `proved_here` et `validated_host_software` dans sa portée relative bornée. La réduction globale reste `partial_refinement` et le `public_status` reste `not_claimed`.
