# Dendrogramme laminaire de points à partir des \(K\) forêts HGP

Date : 10 août 2026 UTC.

Cadre : phase **exploration v3 hors registre**, backend **CPU de référence
mathématique**, profil **candidat hgp_reduced**, mode **conception de projection
ponctuelle**, statut public **non revendiqué**.

Snapshot v3 final repincé après le commit concurrent de Claude :
**HEAD = origin/main = 3cefcd0a403a99e3dd1dc2acfd34dc6653767901**.
Les sources forestières et les deux références de projection citées ci-dessous
sont inchangées depuis le pin initial 37139de.
Empreintes utiles :

- [prototype/saturated_fold.hpp](../prototype/saturated_fold.hpp) :
  72e248c1b28d032138242f4437a432069d7a5c8362b2650688b7666e3682a271 ;
- [ancienne API de hiérarchie de points](../../morsehgp3d/include/morsehgp3d/api/point_hierarchy.hpp) :
  cb543db087ecaec79f084afa1ef6bb41ba221b40f2c7de6b3f31c2ef79c6acd2 ;
- [ancien réducteur CPU correspondant](../../morsehgp3d/src/cpu/api/point_hierarchy.cpp) :
  aa2a4fa8eaec3863e07b41459149239b78358f20fa4d15f7fafaea7b928046e1.

## Verdict

La bonne transformation n'est ni l'union brute des points de chaque nœud, ni
un MST reconstruit sur les points. Elle comporte deux étages séparés :

1. construire un **arbre maître de carriers** depuis les \(K\) forêts et leurs
   applications verticales, sur une échelle de densité commune ;
2. affecter chaque point à **un terminal unique** de cet arbre selon un objectif
   de représentativité déclaré.

Je recommande comme premier objectif scientifique le profil
**tree_l1_median** : chaque \(k\)-ensemble verse une masse conservée à ses
points, puis chaque point est placé à la médiane pondérée de ses carriers dans
l'arbre maître. Ce terminal minimise globalement la somme des désaccords
hiérarchiques. Les clusters d'un nœud sont les points dont le terminal est dans
son sous-arbre : ils sont donc laminaires et sans chevauchement par
construction.

Le claim exact est : **optimum global parmi tous les routages de points sur la
topologie de tour certifiée fixée**. Il ne s'agit pas d'un optimum parmi tous
les dendrogrammes imaginables, ni d'une preuve de complétude des \(K\) forêts.

Le routeur descendant glouton déjà présent dans l'ancienne implémentation reste
un profil défendable, mais seulement sous le nom
**lexicographic_plurality** : il privilégie la première majorité relative
rencontrée. Il n'est pas un optimum additif global et ne doit pas être présenté
comme « le plus représentatif » sans cette qualification.

## 1. Les \(K\) GammaForest live ne suffisent pas

Le [GammaForestNode courant](../prototype/saturated_fold.hpp) conserve
seulement :

~~~text
(level_representative, witness, parent, kind)
~~~

Il ne conserve pas :

- la couverture de points de chaque état vivant de composante ;
- les deltas de couverture portés par les continuations ;
- les facettes et cofaces silencieuses et leur premier carrier ;
- les applications verticales entre deux ordres adjacents ;
- un niveau rationnel autonome, indépendant de l'indice du catalogue ;
- un reçu liant ces informations à la même source certifiée.

Il est notamment interdit de prendre « l'union des witness » comme cluster de
points. Le témoin identifie une composante, il n'en énumère pas la couverture.
Une continuation peut ajouter une facette et un nouveau point sans créer de
nœud dans build_gamma_forest.

Contre-exemple informationnel : deux exécutions possèdent le même arbre d'ordre
deux, avec les mêmes naissances de témoins \(\{0,1\}\) et \(\{2,3\}\) et la
même fusion finale. Dans la première, une continuation ajoute la facette
\(\{0,4\}\) à la première branche ; dans la seconde, elle ajoute \(\{2,4\}\)
à la seconde. Les GammaForest sont identiques, mais toute projection fidèle du
point \(4\) doit différer.

Les marking_saturations du transcript constituent une preuve positive utile,
mais build_gamma_forest les jette et elles ne remplacent pas le journal de
toutes les facettes, continuations et couvertures.

Deux sidecars streamables sont donc obligatoires :

~~~text
CoverageContribution(
  order, simplex_id, point_ids, first_carrier_id,
  exact_level, incident_coface_levels, source_digest)

VerticalAssignment(
  source_order, target_order, exact_cut,
  source_component_id, target_component_id,
  source_digest, naturality_receipt)
~~~

Une continuation dont la couverture ou les contributions changent crée un
marqueur de degré deux dans la projection, même lorsqu'elle ne crée aucun nœud
topologique dans la forêt horizontale. Ces sidecars doivent être scellés
pendant le fold ; les reconstruire après coup depuis les seuls témoins est
impossible.

En leur absence, une sortie ponctuelle peut rester une expérimentation
**relative_to_partial_payload**, jamais une hiérarchie HGP exacte.

## 2. Pourquoi une décision est inévitable

Considérons deux couvertures de points valides provenant de deux composantes :

$$A=\left\lbrace 0,1\right\rbrace,\qquad B=\left\lbrace 1,2\right\rbrace.$$

Elles se croisent sans inclusion. Une famille laminaire ne peut contenir à la
fois \(A\) et \(B\). Le point \(1\) doit rejoindre une branche, rester à leur
ancêtre commun ou devenir bruit. Sans poids ni perte, aucune décision n'est
mathématiquement « la plus représentative ».

Optimiser librement la topologie n'est pas une solution produit simple. Pour
un graphe pondéré arbitraire \(G=(V,E)\), associons à \(v\in V\) l'ensemble
\(S_v=\{p_v\}\cup\{e_{uv}:uv\in E\}\). Deux ensembles \(S_u,S_v\) se croisent
exactement lorsque \(uv\in E\), et aucun n'en contient un autre. La meilleure
sous-famille laminaire pondérée résout alors l'ensemble indépendant de poids
maximal. La recherche de la topologie optimale contient donc déjà un problème
NP-difficile.

La séparation saine est :

1. la topologie vient de la tour HGP certifiée ;
2. le routage des points est optimisé exactement sur cette topologie.

## 3. Construire l'arbre maître de la tour

Cette partie peut reprendre la construction déjà spécifiée dans
[HIERARCHIE_DE_POINTS_MULTI_ORDRES.md](../../docs/math/HIERARCHIE_DE_POINTS_MULTI_ORDRES.md)
et implémentée dans l'ancien
[point_hierarchy.cpp](../../morsehgp3d/src/cpu/api/point_hierarchy.cpp).

### 3.1 Échelle commune

Pour \(z=p/q>0\), un événement d'ordre \(k\) et de rayon carré exact \(a>0\)
reçoit le niveau de densité :

$$\lambda_z(k,a)=\frac{k}{n\omega_3a^{z/2}}.$$

Le facteur commun \(1/(n\omega_3)\) peut être omis lors du tri. La comparaison
reste exacte sans calcul de racine :

$$\lambda_z(k_i,a_i)\geq\lambda_z(k_j,a_j)\Longleftrightarrow k_i^{2q}a_j^p\geq k_j^{2q}a_i^p.$$

Le facteur \(k\) est essentiel. Si tous les ordres et toutes les verticales
sont placés au seul niveau \(r^{-z}\), l'ordre un absorbe immédiatement la
tour et les ordres supérieurs ne produisent plus de structure propre.

Tous les rayons nuls appartiennent à un lot formel unique
\(\Lambda_{\infty}\). Aucun epsilon, plafond fini ou ordre artificiel entre
\(k/0\) et \(k'/0\) n'est autorisé.

### 3.2 Graphe de carriers

Construire un graphe compact \(\mathcal{G}\) contenant :

- les naissances et multifusions des \(T_k\) ;
- les marqueurs de continuation dont la couverture, les contributions ou une
  verticale changent ;
- les arêtes horizontales enfant--parent ;
- les verticales certifiées \(T_{k+1}\rightarrow T_k\), activées au niveau de
  leur extrémité d'ordre inférieur ;
- une racine virtuelle de niveau zéro reliant les racines terminales.

Balayer les niveaux \(\lambda\) en ordre décroissant. À chaque égalité exacte,
activer tous les sommets et toutes les arêtes du lot, calculer les composantes
temporaires, puis publier une seule multifusion multifurquée par composante.
Une séquence binaire dépendant de l'ordre des unions est interdite.

Le merge-tree des composantes de \(\mathcal{G}\) est noté
\(\mathcal{M}\). Les cycles éventuels de \(\mathcal{G}\) ne gênent pas : la
filtration de ses composantes possède toujours un arbre de fusion.

\(\mathcal{M}\) est encore un arbre de carriers. Il ne devient un dendrogramme
de points qu'après l'affectation définie ci-dessous.

## 4. Transformer les preuves HGP en une masse par point

L'idée saine de HGP-old est la normalisation conservatrice des votes des
facettes. Pour une facette projectable \(\tau\) d'ordre \(k\), posons :

$$S_{k,\tau}(z)=\sum_{\substack{\sigma\supset\tau\\\lvert\sigma\rvert=k+1}}\rho(\sigma)^{-z}.$$

Une pondération uniforme exacte peut être proposée comme profil de contrôle.
Pour un point \(x\) et un ordre \(k\) :

$$T_{k,x}(z)=\sum_{\substack{\tau\ni x\\\lvert\tau\rvert=k}}S_{k,\tau}(z).$$

À l'ordre un, le singleton certifié reçoit une contribution directe positive
conventionnelle, inscrite dans le profil. Sans cet ancrage, un point isolé de
toute coface pourrait disparaître du routage au lieu de rester un singleton ou
du bruit explicitement décidé.

Soit \(A_x=\{k:T_{k,x}>0\}\). Des poids rationnels scientifiques
\(\eta_k>0\) donnent :

$$\alpha_{k,x}=\frac{\eta_k}{\sum_{j\in A_x}\eta_j}.$$

La masse de l'incidence \((k,x,\tau)\) est :

$$\pi_{k,x,\tau}=\alpha_{k,x}\frac{S_{k,\tau}}{T_{k,x}}.$$

Elle vérifie la conservation :

$$\sum_{k\in A_x}\sum_{\tau\ni x}\pi_{k,x,\tau}=1.$$

Cette double normalisation empêche un ordre contenant combinatoirement plus de
facettes, ou un point possédant beaucoup d'incidences, de gagner par simple
multiplicité de représentation.

Chaque facette possède un premier carrier certifié \(b(k,\tau)\) dans
\(\mathcal{M}\). Elle induit donc, pour chaque point, une mesure de probabilité
sur les carriers :

$$\mu_x(v)=\sum_{\substack{k,\tau\\x\in\tau\\b(k,\tau)=v}}\pi_{k,x,\tau},\qquad \sum_v\mu_x(v)=1.$$

Les ordres sans contribution pour \(x\) sont absents de \(A_x\), plutôt que de
créer des zéros qui modifieraient artificiellement la normalisation.
Si \(A_x\) est vide, le point est attaché au terminal de bruit de la racine
virtuelle et aucune mesure \(\mu_x\) normalisée n'est fabriquée.

### Rayon nul dans les poids

La topologie symbolique à rayon nul ne suffit pas à définir
\(\rho^{-z}\). Pour un premier backend exact, deux choix honnêtes existent :

1. employer le profil **uniform** dès qu'un atome de rayon nul est présent ;
2. définir une normalisation lexicographique explicite.

Pour la seconde, écrire à ordre fixé
\(S_{k,\tau}=c_{k,\tau}\Lambda_{\infty}+\bar S_{k,\tau}\) et
\(C_{k,x}=\sum_{\tau\ni x}c_{k,\tau}\). Si \(C_{k,x}>0\), définir la part
normalisée par \(c_{k,\tau}/C_{k,x}\) ; si \(C_{k,x}=0\), utiliser le quotient
fini certifié. Cela correspond à la limite commune du terme dominant et évite
tout \(\infty/\infty\).

L'ancienne API morsehgp3d refuse inconditionnellement tout atome de rayon nul
dans le profil inverse_radius. La convention choisie doit donc être une
nouvelle porte, pas un claim hérité.

## 5. Routage recommandé : médiane pondérée de l'arbre

Munissons chaque arête de \(\mathcal{M}\) d'une longueur strictement positive.
La longueur unité mesure un désaccord structurel par arête ; une longueur de
persistance peut pondérer les échelles, à condition d'être certifiée. Notons
\(d_{\mathcal{M}}\) la distance d'arbre.

Pour chaque point \(x\), choisir un terminal :

$$t(x)\in\mathop{\mathrm{argmin}}_{t\in V(\mathcal{M})}\sum_{v\in V(\mathcal{M})}\mu_x(v)d_{\mathcal{M}}(t,v).$$

L'objectif total est :

$$\mathcal{L}_{\mathrm{tree}}=\sum_{x\in X}\sum_v\mu_x(v)d_{\mathcal{M}}(t(x),v).$$

Il se sépare par point : les \(n\) terminaux peuvent donc être optimisés en
parallèle sans conflit.

### Théorème 5.1 — caractérisation de la médiane

Un nœud \(t\) est optimal pour \(x\) si et seulement si chaque composante
connexe \(C\) de \(\mathcal{M}\setminus\{t\}\) porte une masse au plus égale à
\(1/2\) :

$$\mu_x(C)\leq\frac{1}{2}.$$

**Preuve.** Traverser une arête de longueur \(\ell>0\) vers un côté de masse
\(m\) diminue la distance à cette masse de \(\ell m\) et augmente la distance à
la masse complémentaire de \(\ell(1-m)\). La variation de perte est donc
\(\ell(1-2m)\). Le déplacement améliore strictement la perte exactement lorsque
\(m>1/2\). L'absence d'un tel côté caractérise le minimum sur un arbre.

L'algorithme en découle : partir de la racine virtuelle ; tant qu'un unique
enfant contient strictement plus de la moitié de la masse, descendre dans cet
enfant ; sinon s'arrêter. Une fois une branche majoritaire prise, la masse du
côté parent est strictement inférieure à \(1/2\), donc aucun retour n'est
nécessaire.

Une égalité \(1/2\)--\(1/2\) forme un segment de médianes. Le contrat choisit le
nœud le plus haut, c'est-à-dire **stay**, afin d'obtenir l'optimum le plus
grossier et le moins arbitraire. Entre deux clés encore égales, utiliser le
digest canonique, jamais l'indice d'allocation ni le représentant DSU.

Le terminal médian peut être un nœud sans contribution directe
\(\mu_x(t)=0\). Il représente alors une ambiguïté réelle entre plusieurs
branches. Le dendrogramme doit autoriser ce stay structurel et attacher le
point directement au nœud, au même niveau exact, sans epsilon.

### Optimalité annoncée

Le routage médian minimise exactement \(\mathcal{L}_{\mathrm{tree}}\) parmi
tous les routages à un terminal par point sur \(\mathcal{M}\). Cette propriété
est indépendante du choix des longueurs tant qu'elles sont strictement
positives ; les longueurs changent la valeur de la perte, pas l'ensemble des
médianes.

Avec des longueurs unité, \(d_{\mathcal{M}}(t,v)\) est la taille de la
différence symétrique entre les chaînes d'ancêtres de \(t\) et de \(v\). Le
profil est donc exactement un minimum \(L^1\) de désaccords d'appartenance
hiérarchique.

## 6. Pourquoi le routeur glouton existant n'est pas cet optimum

L'ancienne spécification et
[point_hierarchy.cpp](../../morsehgp3d/src/cpu/api/point_hierarchy.cpp)
agrègent la masse de chaque sous-arbre enfant, choisissent le maximum, puis
recommencent uniquement dans cet enfant.

Supposons qu'une branche \(A\) possède deux descendants de masses \(3\) et
\(3\), tandis qu'une branche sœur \(B\) porte une masse \(5\). Le glouton
choisit \(A\), car \(6>5\), puis choisit arbitrairement l'un des deux
descendants de masse \(3\). La médiane descend bien dans \(A\), dont la masse
\(6/11\) est majoritaire, mais s'arrête ensuite à \(A\) : ses trois côtés
portent les masses \(3/11\), \(3/11\) et \(5/11\), toutes inférieures à
\(1/2\).

Autre fixture : trois feuilles portent les masses \(0{,}34\), \(0{,}33\) et
\(0{,}33\). Le glouton choisit la première ; la médiane reste à leur parent,
car aucune branche ne possède de majorité.

Le glouton est toutefois optimal pour un autre ordre : comparer deux terminaux
à leur premier embranchement différent, choisir la sortie de masse maximale,
puis ne comparer plus bas que si les deux passent par le même enfant. C'est un
optimum lexicographique **coarse_first**, pas un optimum additif.

La recommandation est donc :

- conserver ce comportement sous le profil explicite
  **lexicographic_plurality** pour compatibilité ;
- employer **tree_l1_median** comme profil global conservateur ;
- publier les deux pertes et leur taux de désaccord pendant la qualification.

## 7. Variante si les couvertures sont la vérité à approximer

Si le contrat déclare les couvertures complètes comme cible, on peut optimiser
directement leur fidélité au lieu de la distance entre carriers.

Soit \(J\) la famille des états canoniques. L'état \(j\) possède une couverture
\(C_j\subseteq X\), un carrier \(b_j\in\mathcal{M}\) et un poids rationnel
\(w_j>0\). Pour un routage \(t\), posons :

$$P_t(u)=\left\lbrace x:u\in\mathrm{Anc}(t(x))\right\rbrace.$$

La perte est :

$$\mathcal{L}_{\mathrm{cov}}(t)=\sum_{j\in J}w_j\left\lvert P_t(b_j)\mathbin{\triangle}C_j\right\rvert.$$

Elle pénalise à la fois un point couvert envoyé ailleurs et un point absent
envoyé dans la branche. Pour un point \(x\), définir :

$$s_x(u)=\sum_{j:b_j=u}w_j\left(2\mathbf{1}_{x\in C_j}-1\right).$$

La meilleure valeur après entrée dans \(u\) vérifie :

$$F_x(u)=s_x(u)+\max\left(0,\max_{c\in\mathrm{children}(u)}F_x(c)\right).$$

Le zéro est stay. Une induction depuis les feuilles prouve que cette DP
minimise exactement \(\mathcal{L}_{\mathrm{cov}}\) sur la topologie fixée. Un
oracle hors dépôt l'a comparée à l'énumération de tous les terminaux sur
20 000 arbres aléatoires, sans désaccord :

~~~text
DP_BRUTE_FORCE_OK=20000
~~~

Ce profil **coverage_l1_global** est plus exigeant : l'absence d'un point dans
un état est interprétée comme une preuve négative. Il est approprié si les
couvertures de coupe sont l'autorité scientifique choisie. Le profil médian,
fondé uniquement sur les contributions positives HGP, reste plus proche du
vote historique. Les deux profils ne doivent pas être mélangés sous le même
digest.

## 8. Preuve de laminarité et de disjonction

Pour tout nœud \(u\) de l'arbre augmenté, définir :

$$P(u)=\left\lbrace x\in X:u\text{ est ancêtre de }t(x)\right\rbrace.$$

Chaque point possède exactement un terminal. Pour deux nœuds \(u,v\) :

- si \(u\) est ancêtre de \(v\), alors \(P(v)\subseteq P(u)\) ;
- si \(v\) est ancêtre de \(u\), alors \(P(u)\subseteq P(v)\) ;
- sinon leurs sous-arbres sont disjoints et \(P(u)\cap P(v)=\varnothing\).

La famille \(\{P(u)\}\) est donc laminaire. Toute antichaîne produit des
clusters de points deux à deux disjoints. Une coupe qui rencontre chaque
trajet terminal donne une partition de \(X\), bruit compris.

Cette preuve dépend seulement de l'unicité du terminal ; elle vaut pour le
profil médian, la DP de couverture et le profil lexicographique.

Après le routage :

- supprimer les nœuds vides ;
- contracter les chaînes parent--enfant portant le même ensemble de points,
  tout en conservant l'intervalle de niveaux et la provenance ;
- conserver les multifurcations d'un lot exact ;
- n'introduire une binarisation que pour l'affichage, avec des arêtes de
  longueur nulle explicitement marquées **presentation_only**.

## 9. Stabilité et mesure de représentativité

Pour le terminal médian \(t(x)\), définir :

$$\gamma_x=\frac{1}{2}-\max_{C\in\mathrm{cc}(\mathcal{M}\setminus\{t(x)\})}\mu_x(C).$$

Si \(\gamma_x>0\), la médiane est unique. Une perturbation de la mesure de
variation totale strictement inférieure à \(\gamma_x\) ne change pas le
terminal.

Si l'ordre \(k\) porte la masse normalisée \(\alpha_{k,x}\), sa suppression
puis la renormalisation modifie \(\mu_x\) d'au plus \(\alpha_{k,x}\) en
variation totale. La condition suivante certifie donc l'invariance
leave-one-order-out :

$$\gamma_x>\alpha_{k,x}.$$

Le reçu doit publier au minimum :

- la perte totale et la perte par ordre ;
- la distribution des \(\gamma_x\) ou, pour **coverage_l1_global**, la marge
  entre les deux meilleurs terminaux ;
- la fraction de points invariants au retrait de chaque ordre ;
- le nombre d'égalités exactes arbitrées ;
- la conservation de masse par point ;
- le taux de désaccord entre profils pendant la qualification ;
- le digest du terminal de chaque point et de l'arbre quotienté.

Une moyenne globale ne doit jamais masquer le sacrifice complet d'un ordre.

## 10. Condensation et rendu plat

Le dendrogramme complet précède toute sélection plate. La masse d'un nœud est
le nombre de PointId distincts dans \(P(u)\), ou une masse ponctuelle déclarée.
Un seuil min_cluster_size masque une branche de rendu ; il ne supprime aucune
preuve de la tour source.

La sélection EOM reprend la programmation dynamique HDBSCAN sur l'arbre de
points : choisir un parent ou une antichaîne de descendants selon la stabilité.
Les nœuds sélectionnés sont donc disjoints. En cas d'égalité exacte, préférer
le parent produit la sélection la plus grossière.

Les terminaux médians situés à un nœud sans contribution directe doivent être
représentés explicitement au niveau de ce nœud avant EOM. Leur inventer une
durée positive ou un epsilon modifierait la stabilité.

## 11. Ce que montrent réellement les anciennes implémentations

### HGP-old

[core.py](../../HGP-old/src/hgp_clusterer/core.py) construit un MST et un arbre
condensé sur les faces. Son rendu standard calcule les votes facette--point
puis un argmax sur des clusters déjà sélectionnés. Il rend une partition
plate, mais l'affectation est recalculée à chaque coupe et n'est pas
nécessairement emboîtée.

Le mode whole_tree de
[clustering.py](../../HGP-old/src/hgp_clusterer/clustering.py) agrège les poids
puis route top-down, mais :

- il ne compare pas la masse directe du parent comme un vrai stay ;
- numpy.argmax tranche les égalités par position ;
- chaque racine sélectionnée est traitée séparément ;
- un point présent dans deux racines peut donc être recopié.

La formule \(S/T\) est à conserver ; la sémantique des racines et des ex æquo
ne l'est pas.

### morsehgp3D_v2

[forest.cpp](../../morsehgp3D_v2/src/forest.cpp) possède des forêts
horizontales par lots et multifusions, mais pas le coverage_log ni les
verticales nécessaires. Il ne permet donc pas, à lui seul, une projection
ponctuelle fidèle ; ces absences sont aussi déclarées dans le
[README v2](../../morsehgp3D_v2/README.md) et le
[DESIGN v2](../../morsehgp3D_v2/DESIGN.md).

### Ancien morsehgp3d

L'ancienne
[point_hierarchy.hpp](../../morsehgp3d/include/morsehgp3d/api/point_hierarchy.hpp)
et son réducteur CPU constituent le meilleur blueprint : graphe de tour,
échelle \(k/r^z\), racine virtuelle, masses conservées, routage irréversible,
condensation et EOM. Les deux tests locaux ciblés
morsehgp3d.api_point_hierarchy et
morsehgp3d.point_hierarchy_sklearn_differential passent 2/2 sur les artefacts
existants. Cette exécution diagnostique n'est ni un rebuild scellé, ni une
mesure de performance.

Ils ne ferment pas la source v3 : l'ancienne API reçoit une tour déclarée
certifiée sans la construire depuis le nuage. Sa fixture unitaire principale
n'exerce pas encore deux composantes d'ordre supérieur partageant un point.
Son routage reste le profil lexicographique décrit plus haut.

Le point-MST archivé dans
[archive/surrogates/point_mst_v6](../../morsehgp3d/archive/surrogates/point_mst_v6/README.md)
est un négatif utile : chaque point-MST par ordre induit une hiérarchie
laminaire surrogate, mais il ne construit pas le dendrogramme multi-ordres et
remplace la hiérarchie Hartigan portée par les facettes par un autre objet.

## 12. Alternatives rejetées comme chemin exact

### Fermeture dure des recouvrements

À une coupe fixée, prendre les composantes connexes de l'hypergraphe des unions
de points donne la partition la plus fine qui ne scinde aucune couverture.
Mais la chaîne \(\{0,1\}\), \(\{1,2\}\), \(\{2,3\}\) fusionne aussitôt les
quatre points. Avec toutes les verticales au même rayon, cette fermeture est
dominée par \(T_1\). Elle est exacte pour un objectif de fermeture dure, pas
représentative de la tour HGP.

### MST de points sur une coévidence

Il garantit une ultramétrique de Single-Linkage, mais hérite du chaînage et
change l'objet scientifique. Une matrice de coévidence peut aussi coûter
\(\Theta(n^2)\). Ce MST peut être un diagnostic, jamais la source exacte.

### Vote indépendant à chaque coupe

Il peut donner une partition à chaque rayon, mais un point peut passer entre
deux branches sœurs lorsque le seuil change. Les partitions ne sont alors pas
emboîtées. L'affectation terminale doit être irréversible.

### Sous-famille laminaire gloutonne

Elle peut être arbitrairement mauvaise. Dans un graphe de conflits en étoile,
un cluster central de poids \(1+\varepsilon\) bloque un nombre arbitraire de
feuilles disjointes de poids \(1\). L'oracle exact est NP-difficile comme
montré en section 2.

## 13. Complexité et architecture

Soient :

- \(H=\lvert\mathcal{M}\rvert\) ;
- \(V\) le nombre de coutures verticales compactes ;
- \(I\) le nombre d'incidences point--facette émises ;
- \(d_x\) le nombre de carriers incidents à \(x\) ;
- \(B\) la taille maximale d'un chunk.

Les carriers incidents à un point, leurs ancêtres et leurs plus proches
ancêtres communs forment un arbre virtuel. Après tri Euler des carriers, la
médiane se calcule en \(O(d_x\log d_x)\), ou presque \(O(d_x)\) si le flux est
déjà ordonné. La somme est :

$$O\left(I\log d_{\max}\right).$$

Le profil de couverture se calcule aussi sans matrice \(n\times H\).
Pré-calculer \(B(u)=\sum_{j:b_j=u}w_j\), puis utiliser :

$$s_x(u)=-B(u)+2\sum_{\substack{j:b_j=u\\x\in C_j}}w_j.$$

Des sommes préfixes sur l'arbre compressent les segments sans incidence
positive. La mémoire résidente visée est \(O(H+n+B)\), hors spool authentifié.

La projection ne construit ni mosaïque de Delaunay d'ordre supérieur, ni
matrice point--point, ni catalogue résident de toutes les facettes. Le flux de
contributions peut être trié extérieurement ou par GPU, mais il ne devient pas
une structure globale permanente.

## 14. Portes minimales avant implémentation GPU

### Fixtures permanentes

1. \(K=1\), avec singleton ancré et hiérarchie source retrouvée selon le profil ;
2. croisement \(\{0,1\}\) contre \(\{1,2\}\), poids gauche, droit puis égaux ;
3. mêmes GammaForest, continuation de couverture différente ;
4. mêmes forêts horizontales, deux verticales différentes ;
5. branche \(6=3+3\) contre branche \(5\) : médiane au nœud \(6\), pas à une
   feuille ;
6. étoile \(0{,}34/0{,}33/0{,}33\) : médiane au parent ;
7. égalité \(1/2\)--\(1/2\) : terminal supérieur stable sous permutation ;
8. continuation silencieuse modifiant le gagnant sans fusion ;
9. biais combinatoire d'un ordre, neutralisé par \(\alpha_{k,x}\) ;
10. multifusion de niveau exact permutée ;
11. plusieurs racines partageant un point, arbitrées par la racine virtuelle ;
12. rayon nul en profil uniforme, puis profil inverse conforme à la convention
    choisie ou refus explicite ;
13. point sans évidence, envoyé au bruit ;
14. oracle petit énumérant tous les terminaux et vérifiant la perte minimale ;
15. retrait de chaque ordre et vérification du certificat \(\gamma_x\).

### Mutants

- union des witness utilisée comme couverture ;
- continuation ou contribution silencieuse omise ;
- verticale échangée ;
- niveaux égaux séquentialisés ;
- retour au glouton sous le nom tree_l1_median ;
- seuil \(>1/2\) changé en \(\geq1/2\) ;
- canal stay interdit lorsque sa masse directe est nulle ;
- normalisation par nombre brut de facettes ;
- tie-break par index d'allocation ;
- epsilon introduit au rayon nul ;
- terminal recalculé indépendamment à chaque coupe.

### Invariants reçus

- exactement un terminal par PointId, bruit inclus ;
- conservation de la masse \(\sum_v\mu_x(v)=1\) ;
- critère de médiane vérifié autour de chaque terminal ;
- optimum comparé à l'énumération exhaustive sur petites fixtures ;
- enfants, stay et bruit partitionnent leur parent ;
- toutes les paires de nœuds sont imbriquées ou disjointes ;
- lots égaux indépendants de leur permutation ;
- pertes totale et par ordre recalculées depuis le payload ;
- digests identiques entre chemins résident, chunké et GPU ;
- toute ambiguïté non certifiée échoue avant publication.

## 15. Plan directement actionnable pour Claude

1. **Ne pas étendre GammaForestNode avec une simple union de points.** Émettre
   plutôt le sidecar authentifié des contributions/couvertures et les
   verticales adjacentes.
2. **Fixer le profil source** : hgp_reduced est recommandé pour retrouver la
   sémantique historique ; full_pi0 reste une sortie distincte.
3. **Porter ou adapter l'arbre maître** de l'ancienne API point_hierarchy, avec
   niveaux exacts et lots multifurqués.
4. **Écrire d'abord l'oracle CPU tree_l1_median**, plus l'énumération de tous
   les terminaux sur petites fixtures.
5. Conserver lexicographic_plurality seulement comme profil comparatif, puis
   ajouter coverage_l1_global si les couvertures complètes sont scellées.
6. Quotienter les nœuds vides ou de même ensemble de points, sans perdre leurs
   intervalles ni leur provenance.
7. Construire ensuite condensation, coupes et EOM sur la hiérarchie ponctuelle.
8. Paralléliser/GPU uniquement après réception byte-à-byte du CPU : le routage
   se parallélise naturellement par PointId, mais la source, les verticales et
   les lots restent des autorités séparées.

Le reçu final doit porter le digest de la tour, le profil de routage, \(z\), les
\(\eta_k\), le profil de rayon nul, les terminaux, les marges, les pertes par
ordre et le statut amont. La hiérarchie ponctuelle reste une **vue aval
irréversible** ; elle ne remplace jamais les \(K\) forêts recouvrantes, qui
conservent l'information scientifique originale.

## Conclusion

La solution mathématique recommandée est donc :

~~~text
K forêts + coverage/contributions + verticales certifiées
    -> graphe de tour sur l'échelle exacte k/r^z
    -> merge-tree maître par lots atomiques
    -> masses HGP conservées et normalisées par point et par ordre
    -> un terminal médian exact par point
    -> quotient laminaire, condensation, coupes et EOM
~~~

Elle garantit simultanément :

- un seul chemin et un seul terminal par point ;
- une hiérarchie laminaire et sans chevauchement ;
- un optimum global explicite de représentativité sur la topologie HGP fixée ;
- une mesure de stabilité multi-ordres ;
- une architecture sparse compatible avec l'interdit de mosaïque globale.

Le verrou immédiat n'est donc pas le GPU. Il est le contrat
coverage/contributions + verticales absent des GammaForest live. Tant que ce
sidecar n'est pas reçu, aucune projection ponctuelle ne peut être qualifiée
d'exacte ou de « la plus représentative » relativement à la tour complète.

GCP non utilisé.
