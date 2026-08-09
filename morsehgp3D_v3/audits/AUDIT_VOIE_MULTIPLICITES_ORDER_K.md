# Voie mathématique pour `order_k` avec multiplicités : flats fermés, lots et propriétaires shallow

Date : 9 août 2026 UTC.

Phase visée : M3. Backend étudié : CPU exact. Profil : grille u16. Mode : proposition d'architecture, non implémentée et non promue.

> [!IMPORTANT]
> **Une voie exacte existe sans couper les sommets par rang fermé et sans matérialiser la mosaïque globale.** Le vrai 1-squelette multiplicitaire se décrit par les flats fermés de rang trois de la coquille, pas par ses triplets bruts. Une transition suit un tel flat jusqu'au prochain lot simultané ; la coquille suivante est l'union du flat constant et du lot entrant.
>
> **Résultat nouveau utile pour la récolte.** Dans un nuage de dimension affine trois, tout support indépendant $U$ d'arité $q$ est contenu dans un sommet d'arrangement dont le niveau strict ne dépasse pas celui de la sphère minimale de $U$, même si ce sommet possède une coquille arbitrairement grande. Par conséquent, pour publier toutes les sphères de rang fermé au plus $s_{\max}$, les singletons étant directs, il suffit de naviguer selon le niveau strict $\ell\leq s_{\max}-2$. Ce plafond remplace le faux plafond multiplicitaire en rang fermé `s_max + 2`.
>
> Cette voie ferme un problème de correction et de mémoire, pas le contrat 50 k / 1 s. L'énumération locale des flats, la récolte des paires, les requêtes de voisin et le test de parent peuvent encore avoir un coût combinatoire. Ils doivent être mesurés et bornés avant toute promotion produit.

## 1. Snapshot et portée

| objet | empreinte |
|---|---|
| HEAD de référence | `5a6cdb1af030a264ce07adddd312be2c458459b4` |
| header commité étudié par les audits de dégénérescences | SHA-256 `c1548b3ce5336a423ceb7f069ba3311749efdca057025bbde1c63333be193457` |
| header live observé au début de cette note | SHA-256 `08b9ba1182445d281758e994b4ed7a8e96fa27c4ca3af0cb83021cdbbd62dc2c` |
| oracle inchangé | SHA-256 `927809a35e0356a29e81dc6ed23ee9363655a4b3e4af2d12974edb8fe3ce6078` |

Cette note prolonge les preuves et contre-exemples de [`AUDIT_CONNECTIVITE_ORDER_K_A8111F0.md`](AUDIT_CONNECTIVITE_ORDER_K_A8111F0.md), [`AUDIT_REVERSE_SEARCH_ORDER_K_CF9374.md`](AUDIT_REVERSE_SEARCH_ORDER_K_CF9374.md), [`AUDIT_COQUILLES_RANK_CUT_C1548B3.md`](AUDIT_COQUILLES_RANK_CUT_C1548B3.md) et [`AUDIT_ORDER_K_DEGENERESCENCES_C1548B3.md`](AUDIT_ORDER_K_DEGENERESCENCES_C1548B3.md). Elle propose un modèle mathématique et ses portes ; elle n'audite pas comme correction le chemin rapide ajouté dans le delta live.

Seuls ce rapport et son entrée d'index ont été ajoutés au dossier d'audits. Aucun code produit, commit, branche ou ressource externe n'a été modifié. GCP non utilisé.

## 2. Les quatre objets qu'il faut cesser de confondre

Pour $x=(c,t)\in\mathbb{R}^{4}$, posons

$$L_i(x)=t-2c\mathbin{\cdot}p_i+\lVert p_i\rVert^{2}.$$

À un sommet $v$, les ensembles exacts sont

$$B(v)=\lbrace i:L_i(v)<0\rbrace,\qquad S(v)=\lbrace i:L_i(v)=0\rbrace.$$

Le contrat multiplicitaire doit conserver quatre champs séparés :

| objet | définition | usage autorisé |
|---|---|---|
| niveau de navigation | $\ell(v)=\lvert B(v)\rvert$ | coupe du graphe et potentiel du parent |
| coquille complète | $S(v)$ | clé géométrique du sommet et lots d'incidence |
| rang fermé | $\rho(v)=\ell(v)+\lvert S(v)\rvert$ | filtre de publication seulement |
| support HGP | $U^\star\subseteq S(v)$, $1\leq\lvert U^\star\rvert\leq4$ | arité, forêt et sérialisation canonique |

Le rang fermé n'est ni un niveau de graphe ni un potentiel de parcours. La fixture du pont à coquille cinq prouve qu'une coupe sur $\rho$ retire un sommet de niveau zéro indispensable à des sorties de rang deux.

Il existe encore un cinquième lot, indépendant : plusieurs sphères géométriquement distinctes peuvent avoir le même rayon exact. Ce lot de valeur `beta` appartient au tri scientifique et à la réduction de forêt ; il ne doit pas être confondu avec le lot de points qui atteint simultanément un même sommet le long d'un pinceau.

## 3. Le vrai 1-squelette multiplicitaire

### 3.1 Sommets : flats de rang quatre

Un sommet d'arrangement est un point $v\in\mathbb{R}^{4}$ où les hyperplans de $S(v)$ ont un rang normal égal à quatre. Sa coquille complète détermine alors $v$ de façon unique. Une sous-coquille de quatre indices n'est qu'une base possible ; elle n'est pas la clé du sommet.

### 3.2 Arêtes : flats fermés de rang trois

Une droite porteuse $F$ du 1-squelette est l'intersection de trois hyperplans indépendants. Sa fermeture est

$$C(F)=\lbrace i:F\subseteq H_i\rbrace.$$

À un sommet $v$, tout $i\in C(F)$ appartient à $S(v)$. Inversement, pour un triple indépendant $T\subseteq S(v)$, la fermeture locale est l'ensemble des indices de $S(v)$ dont l'hyperplan contient la droite $F_T$. Deux triples donnent le même pinceau exactement lorsqu'ils ont la même fermeture.

Dans la géométrie des points, $C(F)$ est le lot maximal de points de la coquille qui se trouvent dans un même plan et sur un même cercle. Le code courant traite tous les triples de ce lot comme des arêtes différentes ; le quotient correct les traite comme une seule droite.

Une représentation canonique locale peut employer :

1. la plus petite base indépendante de trois indices de $C(F)$, obtenue gloutonnement dans l'ordre des identifiants ;
2. la fermeture triée $C(F)$ comme certificat ;
3. une direction primitive exacte $d_F$ de la droite, normalisée par le signe de sa première coordonnée non nulle.

Comme toutes les droites considérées passent par le même sommet $v$, leur direction et leur fermeture les distinguent localement. Pour une clé globale, il faut ajouter le flat affine, par exemple ses trois formes affines canoniques ; la seule direction ne suffit pas entre deux sommets différents.

### 3.3 Cellules ouvertes et événements consécutifs

Sur $F$, paramétrons $x(\tau)=v+\tau d$ avec $\tau>0$ dans une direction fixée. Pour chaque hyperplan qui ne contient pas $F$, $L_i(x(\tau))$ est affine non constant en $\tau$. Le prochain sommet est le plus petit zéro strictement positif ; tous les indices qui atteignent ce même paramètre forment un lot entrant $A$.

Si $C=C(F)$ et $D=S(v)\setminus C$, la transition exacte est

$$S(w)=C\mathbin{\cup}A.$$

Cette formule conserve tous les points constants du cercle et retire tous les anciens membres de coquille qui ne contiennent pas la droite. Elle corrige précisément la transition `tri + tied` réfutée par la fixture à coquille constante.

Les hyperplans parallèles à $F$ qui ne le contiennent pas ont une valeur $L_i$ constante non nulle. Ils restent intérieur ou extérieur pendant toute la transition. Ils ne doivent être ni ignorés au germe, ni ajoutés au lot.

### 3.4 Transport exact du niveau par lots

Écrivons $a_i=\nabla L_i=(-2p_i,1)$ et orientons $d$ vers le voisin. Les anciens membres qui deviennent intérieurs sur l'arête ouverte sont

$$D_-(d)=\lbrace i\in S(v)\setminus C:a_i\mathbin{\cdot}d<0\rbrace.$$

L'ensemble intérieur de l'arête ouverte vaut donc

$$B_e=B(v)\mathbin{\cup}D_-(d).$$

Un membre $i$ du lot entrant $A$ était intérieur juste avant le sommet terminal exactement lorsque $a_i\mathbin{\cdot}d>0$. Ainsi

$$B(w)=B_e\setminus\lbrace i\in A:a_i\mathbin{\cdot}d>0\rbrace.$$

Ces trois identités donnent la coquille, l'ensemble intérieur et le niveau sans supposer qu'un seul point change d'état. Pendant la phase prototype, un census exact de tous les points doit néanmoins vérifier systématiquement ces identités ; le transport ne doit devenir autorité qu'après certification des fixtures permanentes.

## 4. Parent canonique sur le vrai graphe non simple

Le parent à marge de [`AUDIT_REVERSE_SEARCH_ORDER_K_CF9374.md`](AUDIT_REVERSE_SEARCH_ORDER_K_CF9374.md) ne dépend pas de la simplicité du sommet. Pour $B(v)\neq\varnothing$, choisissons

$$M(v)=\max_{i\in B(v)}L_i(v),\qquad h(v)=\min\lbrace i\in B(v):L_i(v)=M(v)\rbrace.$$

Le sommet $v$ est un sommet du polyèdre de chambre

$$P_{B(v)}=\lbrace x:L_i(x)\leq0\ \text{si}\ i\in B(v),\quad L_j(x)\geq0\ \text{si}\ j\notin B(v)\rbrace.$$

Le théorème de monotonie du graphe d'un polyèdre fournit une arête incidente sur laquelle $L_{h(v)}$ augmente. Dans le modèle multiplicitaire, cette arête est exactement une orientation d'un flat fermé de rang trois. Une direction $d$ appartient localement à $P_{B(v)}$ si et seulement si aucun ancien membre de coquille ne devient intérieur, soit $D_-(d)=\varnothing$.

Parmi les voisins finis admissibles qui augmentent $L_{h(v)}$, choisir la clé de flat puis la coquille terminale lexicographiquement minimales définit le parent. Sur cette arête,

$$B(\pi(v))\subseteq B(v).$$

Le potentiel déjà prouvé reste valide : la cardinalité de $B$ diminue, ou elle reste constante et $M$ augmente strictement. Aucun cycle n'est possible et aucun sommet de niveau supérieur n'est traversé.

Au niveau zéro, on peut conserver le potentiel $Q_r$ du rapport reverse search. Si le germe $r$ est multiple, prendre la plus petite base indépendante de quatre indices de $S(r)$ et sommer seulement ses quatre formes suffit : leur annulation commune identifie encore uniquement $r$.

La reverse search visite donc tous les sommets $\ell\leq k$ sans `seen`, à condition que l'oracle local énumère chaque flat fermé de rang trois et chacun de ses deux voisins consécutifs. Elle ne doit jamais filtrer un voisin par $\rho$.

## 5. Germe multiplicitaire exact sans pinceau fragile

Le germe peut lui aussi être défini sans supposer une face simple. Considérons

$$P_{\varnothing}=\lbrace x:L_i(x)\geq0\ \text{pour tout}\ i\rbrace.$$

Si le nuage a dimension affine trois, les normales $a_i$ engendrent $\mathbb{R}^{4}$. Le polyèdre $P_{\varnothing}$ est non vide et pointé. Sur tout rayon de récession non nul, au moins une dérivée $a_i\mathbin{\cdot}d$ est strictement positive ; la fonction linéaire $G_0(x)=\sum_iL_i(x)$ est donc strictement croissante sur ce rayon.

Minimiser d'abord $G_0$, puis lexicographiquement les quatre coordonnées sur la face optimale compacte, sélectionne un unique sommet de niveau zéro. Il peut être non simple, mais son niveau et sa coquille sont obtenus par un census exact. Cette définition évite les deux dettes du germe courant : témoins coplanaires intérieurs oubliés et choix dépendant d'un seul pinceau.

Une résolution exacte de programmation linéaire en dimension fixe suffit. Cette proposition est une porte d'implémentation à certifier, pas une invitation à utiliser un flottant comme décision.

## 6. Théorème de propriétaire shallow pour les arités basses

### 6.1 Énoncé

Soit $U$ un ensemble de $q$ points affinement indépendants, avec $1\leq q\leq4$. Le flat

$$F_U=\bigcap_{u\in U}H_u$$

a dimension $4-q$. Soit $x_U$ le point de $F_U$ qui minimise le rayon carré ; il représente la sphère minimale passant par $U$. Notons

$$B_U=\lbrace i:L_i(x_U)<0\rbrace,\qquad d_U=\lvert B_U\rvert.$$

> **Théorème de propriétaire.** Si le nuage a dimension affine trois, il existe un sommet d'arrangement $o(U)\in F_U$ tel que $B(o(U))\subseteq B_U$. En particulier, $\ell(o(U))\leq d_U$. La conclusion ne dépend ni de la simplicité de $o(U)$, ni de la taille de sa coquille.

L'indépendance de $U$ exclut les doublons, les paires confondues et les triangles collinéaires. Un nuage de dimension affine inférieure à trois doit être envoyé vers une voie dimensionnelle dédiée ou rejeté explicitement ; il ne satisfait pas la preuve ci-dessous.

### 6.2 Preuve

Dans $F_U$, considérons le polyèdre de signes fermé

$$P_U=F_U\cap\lbrace x:L_i(x)\leq0\ \text{pour}\ i\in B_U,\quad L_j(x)\geq0\ \text{pour}\ j\notin B_U\cup U\rbrace.$$

Il est non vide car $x_U\in P_U$. Une direction de linéalité de $P_U$ annulerait les restrictions de toutes les formes $L_i$, y compris celles de $U$. Or la dimension affine trois du nuage équivaut au fait que les normales $a_i=(-2p_i,1)$ engendrent $\mathbb{R}^{4}$. La seule direction de linéalité est donc zéro : $P_U$ est pointé.

Tout polyèdre non vide et pointé possède un sommet. À un sommet de $P_U$, les égalités de $U$ et les contraintes actives supplémentaires ont ensemble un rang quatre ; ce point est donc un vrai sommet de l'arrangement contenant $U$. Enfin, toute forme autorisée à être négative dans $P_U$ est indexée par $B_U$. Par conséquent, aucun nouvel intérieur strict ne peut apparaître et $B(o(U))\subseteq B_U$.

### 6.3 Propriétaire unique et calcul local

Le théorème d'existence devient une règle sans table globale. Écrivons les contraintes de $P_U$ sous la forme $G_i(x)\geq0$ et posons $G_U(x)=\sum_iG_i(x)$. Sur tout rayon de récession non nul, chaque dérivée est positive ou nulle et au moins une est positive ; $G_U$ atteint donc son minimum sur une face compacte.

Minimiser ensuite lexicographiquement les coordonnées de $x$ sur cette face choisit un unique sommet, déclaré propriétaire canonique $o(U)$. Pour une paire, le problème vit dans un plan ; pour un triangle ou, plus exactement, un flat de rang trois, il vit sur une droite. Une programmation linéaire exacte en dimension deux ou un tri exact de bornes en dimension un calcule ce propriétaire à la demande, sans mosaïque ni table de sommets.

### 6.4 Plafond correct pour un catalogue de rang $s_{\max}$

Si la sphère de $U$ a un rang fermé au plus $s_{\max}$, sa coquille contient au moins les $q$ points de $U$. Donc

$$d_U\leq s_{\max}-q.$$

Le propriétaire satisfait la même borne. Les singletons se publient directement. Les paires sont alors le cas dominant, et toutes les arités deux à quatre possèdent un propriétaire dans

$$\ell\leq k_{\mathrm{nav}},\qquad k_{\mathrm{nav}}=s_{\max}-2.$$

Pour $s_{\max}<2$, aucune navigation n'est nécessaire. Ce plafond est un plafond de **niveau strict**. Un propriétaire qui vérifie cette borne peut avoir une coquille de taille huit, cinquante ou davantage ; il doit être traversé même si son rang fermé dépasse largement $s_{\max}$.

Le cube et le pont à coquille cinq cessent ainsi d'être des cas spéciaux : à `s_max=2`, la navigation se fait au niveau zéro et conserve précisément leurs sommets-ponts multiples.

### 6.5 Falsification rationnelle indépendante

Un probe créé uniquement sous `/tmp`, SHA-256 `e462861105052a5bd89a1fc555e2d934d5d3f5006484dc76d4d95f3e159f9cb2`, a :

1. énuméré exactement par fractions rationnelles tous les sommets issus des quadruplets indépendants ;
2. regroupé les quadruplets par centre et rayon exacts afin de former les coquilles multiples ;
3. calculé la boule diamètre de chaque paire et le circumcentre de chaque triangle non collinéaire ;
4. vérifié qu'au moins un sommet contenant le support avait un niveau strict inférieur ou égal à celui de cette sphère.

La campagne couvre les fixtures cube et pont à coquille cinq, 500 nuages entiers aléatoires et 300 nuages enrichis en points d'une même sphère entière. Parmi eux, 797 nuages de dimension affine trois ont décidé 51 139 supports d'arité deux ou trois, sans contre-exemple. Cette campagne teste l'énoncé sur des multiplicités réelles ; elle ne remplace pas la preuve de la section 6.2 et ne qualifie ni le propriétaire canonique, ni son coût.

## 7. Récolte sans tables globales de doublons

### 7.1 Arity trois : publier un flat, pas chacun de ses triplets

Un flat fermé de rang trois $C$ représente un unique pinceau de sphères et possède un unique point $x_C$ de rayon minimal. Tous les triples qui forment une base de $C$ décrivent le même cercle et le même événement d'arité trois.

La voie correcte est donc :

1. énumérer chaque flat fermé $C$ une fois à un sommet ;
2. calculer $x_C$ et son census exact ;
3. ne le traiter qu'au propriétaire canonique $o(C)$ construit comme en section 6 ;
4. publier seulement si la sphère est la miniboule de sa coquille, si son support canonique a arité trois, si ce support engendre $C$ et si son rang fermé est au plus $s_{\max}$.

Cette règle retire les doublons dus aux triples cocirculaires avant le scan du nuage. Elle ne retire pas le nombre intrinsèque de flats distincts.

### 7.2 Arity deux : propriétaire du plan de bisecteurs

Pour une paire distincte $U=\lbrace a,b\rbrace$, $x_U$ est sa boule diamètre et $F_U$ est le plan des sphères passant par les deux points. Le propriétaire $o(U)$ est un sommet de ce plan de signes, de niveau au plus celui de la boule diamètre.

Au sommet visité, la paire est testée seulement si le propriétaire recalculé coïncide avec le sommet courant. La décision dépend uniquement de $U$ et du nuage ; elle ne dépend ni de l'ordre DFS, ni d'un `seen_edge`. Les cas `n=2` et `n=3` gardent une voie directe avant toute navigation.

Cette règle supprime la mémoire globale des paires déjà vues, mais pas nécessairement tout travail répété : une même paire peut appartenir aux coquilles de plusieurs sommets avant d'atteindre son propriétaire. Une condition locale plus économique ou un index shallow reste à trouver pour le contrat temporel.

### 7.3 Support canonique d'une sphère multiple

La coquille seule déduplique la géométrie, mais l'arité publique exige une convention. Pour une sphère qui est la miniboule de sa coquille $S$, une convention mathématique déterministe est : choisir d'abord la plus petite cardinalité $q$, puis le plus petit sous-ensemble lexicographique $U^\star\subseteq S$ affinement indépendant tel que le centre appartienne à l'intérieur relatif de $\mathrm{conv}(U^\star)$.

La publication peut alors être partitionnée sans hash global :

- $q=1$ par la voie directe ;
- $q=2$ uniquement par la paire $U^\star$ à son propriétaire ;
- $q=3$ uniquement par le flat fermé engendré par $U^\star$ à son propriétaire ;
- $q=4$ uniquement au sommet d'arrangement lui-même.

La convention exacte doit être alignée avec le contrat HGP et l'oracle avant implémentation. La recherche naïve de $U^\star$ parmi tous les sous-ensembles jusqu'à quatre éléments serait combinatoire ; un algorithme exact de miniboule ou de Carathéodory en dimension fixe doit fournir le certificat canonique.

## 8. Coût réel et invariant d'architecture

Notons $m_v=\lvert S(v)\rvert$ et $f_3(v)$ le nombre de flats fermés de rang trois incidents à $v$.

Le modèle élimine la mauvaise duplication `un triplet = une arête` : le nombre de requêtes de voisin devient proportionnel à $f_3(v)$, pas directement à $\binom{m_v}{3}$. Néanmoins :

- $f_3(v)$ peut lui-même atteindre $\binom{m_v}{3}$ ; un sommet multiple peut avoir un degré réellement combinatoire ;
- énumérer les flats distincts en générant encore tous les triples ne retire que les scans répétés, pas le coût local de génération ;
- un hash local de flats coûte $O(f_3(v))$ mémoire, tandis qu'un test de base canonique en streaming économise cette mémoire mais peut augmenter le temps ;
- avec un scan global par direction, la navigation coûte encore de l'ordre de $n\sum_v f_3(v)$ ;
- recalculer le parent de chaque voisin pour reconnaître les enfants peut ajouter un facteur de degré dans une implémentation naïve ;
- la récolte des paires peut payer $\sum_v\binom{m_v}{2}$ tests d'incidence avant toute optimisation ;
- la sortie et les lots exacts de `beta` demandent toujours une mémoire proportionnelle au catalogue ou un tri externe.

La mémoire de navigation peut en revanche respecter l'invariant v3 : entrée et éventuel index en $O(n)$, coquille et ensemble intérieur courants, itérateur local de flats, puis pile de reverse search ou retour par parent. Aucune table globale de sommets, d'arêtes, de cofaces ou de cellules d'ordre supérieur n'est nécessaire.

Il faut donc publier séparément :

1. une **preuve de complétude** sur le vrai graphe $\ell\leq s_{\max}-2$ ;
2. une **preuve de non-duplication** par propriétaires et support canonique ;
3. une **borne ou mesure de travail** sur $V_k$, $m_v$, $f_3(v)$, les paires candidates et le coût du parent ;
4. un **oracle de voisin à la demande** certifié, éventuellement accéléré par index, qui ne reconstruit pas une mosaïque globale sous un autre nom.

## 9. Séquence de mise en œuvre falsifiable

1. Remplacer la clé `triplet` par un objet `Rank3Flat` exact : base canonique, fermeture et direction.
2. Construire un voisin uniquement par `constant_flat + entering_batch`, puis vérifier coquille et niveau par census complet.
3. Remplacer toute coupe `shell.size() + level` de navigation par $\ell\leq s_{\max}-2$ ; conserver le rang fermé dans `try_emit` seulement.
4. Introduire le germe canonique de $P_{\varnothing}$ ou certifier une construction équivalente sur toutes les constantes coplanaires.
5. Exerciser d'abord un BFS avec tables sur petits nuages afin de comparer le vrai 1-squelette à une énumération exhaustive indépendante.
6. Ajouter ensuite le parent multiplicitaire et vérifier que chaque sommet hors germe possède exactement un parent shallow et que le parent termine.
7. Ajouter les propriétaires 1D des flats de rang trois, puis les propriétaires 2D des paires ; conserver les petites dimensions en voie directe.
8. Comparer catalogue, membres, support canonique et lots de `beta` à un oracle qui accepte réellement les coquilles multiples.
9. Seulement après cette porte, remplacer les scans globaux par un index certifié et mesurer 50 k.

Fixtures permanentes minimales : cube à coquille huit, pont RelevantGP à coquille cinq, témoin coplanaire intérieur du germe, transition à cercle constant, plusieurs lots entrants au même paramètre, deux flats de rang trois distincts dans une même coquille et plusieurs supports minimaux d'une même miniboule.

## 10. Verdict

La voie multiplicitaire cohérente est désormais assez précise pour être prototypée :

- **GO mathématique conditionnel** pour le vrai 1-squelette quotienté par flats fermés de rang trois ;
- **GO mathématique** pour la séparation `niveau strict / coquille / rang fermé / support` ;
- **GO mathématique conditionnel** pour le plafond de navigation $s_{\max}-2$ et les propriétaires shallow, sous dimension affine trois et supports indépendants ;
- **GO expérimental** pour un BFS exact avec census systématique, puis reverse search ;
- **NO-GO produit** tant que l'énumérateur local de flats, la récolte des paires, le parent multiplicitaire, l'oracle dégénéré et les coûts 50 k ne sont pas fermés.

Le point décisif est négatif et positif à la fois : une grande coquille ne peut jamais être coupée parce qu'elle peut porter la connectivité, mais elle n'oblige pas à augmenter le niveau shallow nécessaire à la récolte. Elle doit être traversée comme un nœud de lot, quotientée par ses vrais flats, puis filtrée seulement lors de la publication.
