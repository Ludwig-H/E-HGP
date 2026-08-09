# Audit de la voie `reverse search` pour `order_k` — snapshot simple `cf9374`

Date : 9 août 2026 UTC.

Phase revendiquée : M3. Backend : CPU exact. Profil : grille u16. Mode audité : architecture expérimentale sous arrangement simple.

> [!IMPORTANT]
> **Il existe un vrai remplacement de `seen` et `visited` par un parent canonique local.** Sous arrangement simple en dimension quatre, le graphe des sommets de niveau au plus $k$ admet une arborescence de reverse search vers un germe de niveau zéro. Le parent se calcule par les voisins du sommet et un census exact du nuage ; il ne demande ni table globale de sommets, ni LP, ni mosaïque matérialisée. Une pile DFS coûte $O(\text{profondeur})$, et la variante classique de reverse search peut revenir au parent avec un état de parcours borné.
>
> **Ce résultat ferme seulement la mémoire du parcours simple.** Avec l'oracle de voisin actuel, le temps reste en $\Theta(nV)$, les sommets non critiques restent parcourus, les arités basses demandent un propriétaire unique, et le tri exact des événements et de leurs lots impose soit la mémoire de la sortie, soit un tri externe. `RelevantGP` ne fournit pas la simplicité globale requise ici.

## 1. Snapshot et séparation des résultats

| objet | empreinte |
| --- | --- |
| HEAD observé pour le snapshot antérieur | `7fa39b1d8c9d3b566bcd098bb4bdd2dbc107d7af` |
| `prototype/order_k_bfs.hpp` simple avec harvest | SHA-256 `cf9374b64fdc6428625a1e8f72ecb6e19e6d66a80d3249361c694ea064c6d256` |
| preuve de connectivité utilisée | [`AUDIT_CONNECTIVITE_ORDER_K_A8111F0.md`](AUDIT_CONNECTIVITE_ORDER_K_A8111F0.md) |

Ce rapport ne qualifie que le modèle où chaque sommet porte exactement quatre hyperplans et où une arête est le pinceau de trois hyperplans indépendants. La réécriture ultérieure à coquille variable est auditée séparément dans [`AUDIT_ORDER_K_DEGENERESCENCES_C1548B3.md`](AUDIT_ORDER_K_DEGENERESCENCES_C1548B3.md). Les deux snapshots ne doivent pas être confondus.

Probes éventuels et raisonnements auxiliaires conservés sous `/tmp`. Aucun source v3, commit, branche ou état GCP modifié par cet audit.

## 2. Coordonnées de l'arrangement

Pour $x=(c,t)\in\mathbb{R}^{4}$ et chaque site $p_i$, posons

$$L_i(x)=t-2c\mathbin{\cdot}p_i+\lVert p_i\rVert^{2}.$$

Sur la sphère représentée par $x$, $L_i(x)=\lVert p_i-c\rVert^{2}-r^{2}$. Ainsi :

- $L_i<0$ signifie intérieur strict ;
- $L_i=0$ signifie coquille ;
- $L_i>0$ signifie extérieur.

Pour un sommet $v$, notons

$$B(v)=\lbrace i:L_i(v)<0\rbrace,\qquad S(v)=\lbrace i:L_i(v)=0\rbrace.$$

Sous simplicité, $\lvert S(v)\rvert=4$ et le niveau vaut $\ell(v)=\lvert B(v)\rvert$. La fermeture de la chambre située verticalement juste au-dessus de $v$ est

$$P_B=\lbrace x:L_i(x)\leq0\ \text{pour}\ i\in B,\quad L_j(x)\geq0\ \text{pour}\ j\notin B\rbrace.$$

Le sommet $v$ est un sommet du polyèdre pointé $P_{B(v)}$. Parmi les deux directions de chaque pinceau incident, exactement une appartient localement à $P_B$ : après abandon d'un hyperplan de $S(v)$, cet ancien point de coquille doit devenir extérieur, jamais intérieur.

## 3. Parent canonique pour un niveau strictement positif

Supposons $B=B(v)\neq\varnothing$. Définissons la marge verticale la plus proche de la coquille par

$$M(v)=\max_{i\in B}L_i(v)<0,$$

et choisissons le plus petit identifiant $h(v)$ qui atteint ce maximum.

Cette sélection évite le LP que demanderait la recherche d'une facette non redondante. En effet, le segment vertical

$$x_s=(c_v,t_v+s),\qquad 0\leq s\leq-M(v),$$

reste entièrement dans $P_B$. À son extrémité, $L_{h(v)}=0$. Le maximum de la forme linéaire $L_{h(v)}$ sur $P_B$ vaut donc zéro et est atteint, alors que sa valeur en $v$ est strictement négative.

Le théorème de monotonie du graphe d'un polyèdre donne une arête incidente de $P_B$ sur laquelle $L_{h(v)}$ augmente strictement. Une telle arête ne peut pas être un rayon non borné de pente positive, puisque $L_{h(v)}\leq0$ sur tout $P_B$ ; elle possède donc un sommet voisin fini. Parmi les voisins améliorants, choisissons canoniquement celui dont le support trié est lexicographiquement minimal. Ce voisin est le parent $\pi(v)$.

L'arête étant contenue dans $P_B$, on a

$$B(\pi(v))\subseteq B(v).$$

Deux cas sont possibles :

1. l'inclusion est stricte, donc le niveau diminue ;
2. l'ensemble $B$ est inchangé, et alors

$$M(\pi(v))\geq L_{h(v)}(\pi(v))>L_{h(v)}(v)=M(v).$$

Le couple « cardinal de $B$, puis marge $M$ » interdit donc tout cycle : le cardinal baisse, ou la marge augmente strictement dans un ensemble fini de sommets portant le même $B$. Tout sommet de niveau positif atteint finalement un sommet de niveau zéro, sans jamais augmenter son niveau.

Ce potentiel est le point utile nouveau : la cible $h(v)$ peut changer à chaque pivot sans casser la preuve. Il n'est pas nécessaire de calculer une facette canonique globale de $P_B$.

## 4. Parent canonique au niveau zéro

Fixons un germe exact et déterministe $r$ de niveau zéro. Sa construction et sa certification restent une porte indépendante ; le germe coplanaire du snapshot `cf9374` n'est pas réparé par le reverse search.

Tous les sommets de niveau zéro sont les sommets du même polyèdre

$$P_{\varnothing}=\lbrace x:L_j(x)\geq0\ \text{pour tout}\ j\rbrace.$$

Pour le support simple $S(r)$, définissons

$$Q_r(x)=\sum_{s\in S(r)}L_s(x).$$

Chaque terme est non négatif sur $P_{\varnothing}$. L'égalité $Q_r(x)=0$ impose les quatre égalités $L_s(x)=0$ ; leur indépendance donne $x=r$. Le germe est donc l'unique minimum de $Q_r$ sur $P_{\varnothing}$.

Depuis tout autre sommet de niveau zéro, il existe une arête du polyèdre qui diminue strictement $Q_r$. Le parent est le voisin diminuant de support lexicographiquement minimal. Cette seconde orientation est acyclique et termine uniquement en $r$.

En combinant les sections 3 et 4, chaque sommet shallow possède exactement un parent, sauf $r$, et tous les parents restent shallow. On obtient une arborescence couvrante du vrai 1-squelette induit par $\ell\leq k$.

## 5. Énumération sans `seen`

À un sommet simple, le graphe complet de l'arrangement possède au plus huit voisins finis : deux directions pour chacun des quatre triples porteurs. Pour énumérer les enfants de $v$ :

1. énumérer ses voisins $w$ de niveau au plus $k$ ;
2. recalculer canoniquement $\pi(w)$ ;
3. descendre dans $w$ exactement lorsque $\pi(w)=v$.

Un DFS ordinaire ne conserve que le chemin courant, soit $O(\text{profondeur})$ sommets. Le schéma de reverse search d'Avis--Fukuda peut aussi retrouver l'indice du fils en réénumérant les voisins au retour ; il conserve alors le sommet courant, un indice de voisin et la profondeur, sans table proportionnelle à $V$.

Il faut distinguer trois mémoires :

- l'entrée et son index spatial éventuel, qui restent en $O(n)$ ;
- le travail de navigation, qui peut devenir indépendant de $V$ ;
- la sortie scientifique, qui coûte nécessairement au moins la taille du catalogue et de ses membres si l'API retourne un objet résident.

Le reverse search retire donc bien `seen`, `visited` et la frontière globale. Il ne rend pas un `Catalogue` non vide possible en mémoire constante.

## 6. Décisions exactes nécessaires

La quantité $L_i(v)$ est exactement la puissance du point $p_i$ par rapport à la sphère de $v$. Dans la représentation actuelle, `sphere_side` calcule un numérateur

$$N_i(v)=\mathrm{den}(v)L_i(v).$$

Comme le dénominateur est positif et commun à tous les points testés contre une même sphère, $h(v)$ se choisit en prenant le plus grand numérateur strictement négatif. En revanche, comparer deux sommets exige les produits croisés

$$N_i(v)\mathrm{den}(w)\quad\text{et}\quad N_i(w)\mathrm{den}(v).$$

La même obligation vaut pour $Q_r$. Ces comparaisons doivent employer une largeur démontrée ou les `BigInt` déjà disponibles ; un `double`, `beta` ou produit `i128` non borné ne peut pas orienter le parent.

Pour reconnaître les quatre arêtes de $P_B$ parmi les huit directions de l'arrangement, le test local est exact : l'ancien apex abandonné doit être strictement extérieur dans la sphère du voisin. Parmi ces quatre voisins, la hausse de $L_{h(v)}$ ou la baisse de $Q_r$ se décide ensuite par produit croisé.

## 7. Arity harvest sans tables de doublons

Supprimer `seen_edge` et `seen_face` demande un propriétaire unique pour chaque support bas. Une règle « premier rencontré par le DFS » recréerait précisément la mémoire globale retirée. Sous dimension affine trois, arrangement simple, support propre et arrangement restreint essentiel, un propriétaire géométrique local existe.

### 7.1 Triangles

Pour un triangle $U$, paramétrons son pinceau par $\lambda=0$ à la sphère minimale, avec orientation canonique de la normale obtenue depuis les identifiants triés. Prenons le premier sommet rencontré pour $\lambda>0$ s'il existe, sinon le premier pour $\lambda<0$. Entre le minimum et cet événement, aucun point ne change de côté. Si le rang fermé du triangle vaut $r$, le propriétaire a donc un rang fermé au plus $r+1$.

Un sommet contenant $U$ peut vérifier qu'il est ce propriétaire à l'aide de son paramètre et des prédécesseur et successeur consécutifs du même pinceau. Le scan de voisinage déjà requis fournit cette décision ; aucune table globale de triangles n'est nécessaire.

### 7.2 Paires

Pour une paire distincte $U=\lbrace a,b\rbrace$, le flat de centres est le plan orthogonal à $b-a$, centré au milieu. Construisons deux directions rationnelles canoniques $e_1,e_2$ de ce plan. Parmi les rayons $+e_1,-e_1,+e_2,-e_2$ dans cet ordre, choisissons le premier qui rencontre un hyperplan restreint, puis son premier lot d'événements. Sous simplicité, ce lot contient un ou deux hyperplans : avec deux, le point atteint est déjà le sommet propriétaire ; avec un seul, il appartient au flat d'un triangle, sur lequel on choisit une direction canonique et le premier événement fini. Le sommet final est le propriétaire de $U$.

Le premier segment augmente le rang fermé d'au plus deux s'il atteint directement un sommet, sinon d'au plus un ; le second segment éventuel ajoute au plus un. Une paire utile de rang $r$ possède donc un propriétaire de rang au plus $r+2$. L'arrangement restreint essentiel garantit l'existence du sommet final ; les petites tailles $n=2$ et $n=3$ gardent une voie directe.

Tester ce propriétaire demande deux sélections exactes parmi les points, donc encore $O(n)$ sans index. La règle résout les doublons et justifie le plafond simple $s_{\max}+2$ ; elle ne résout pas le temps.

### 7.3 Limites de cette propriété

Les preuves de rang $r+1$ et $r+2$ utilisent un événement à la fois. Un lot de points cosphériques peut faire sauter le rang fermé de plus d'une unité. `RelevantGP` interdit l'extra-shell de la miniboule utile, mais pas les multiplicités non critiques rencontrées plus tard par la route de propriété. Cette construction ne doit donc pas être transposée telle quelle à la réécriture à coquille variable.

## 8. Tri canonique, membres et lots de niveaux

L'ordre du reverse search est un ordre de navigation, pas un ordre public. Le contrat existant exige un support canonique, des tranches contiguës de membres et des sources de forêt stables. L'[audit du contrat catalogue--forêt](AUDIT_CONTRAT_CATALOGUE_FORET_ORDER_K_CF9374.md) montre en outre que le comparateur par arité du snapshot ne coïncide déjà pas avec l'ordre lexicographique des chemins existants.

Deux stratégies exactes sont possibles :

- conserver en mémoire les $C$ événements publiés et leurs $L$ occurrences de membres, trier par la clé canonique scellée, puis reconstruire le pool ; la mémoire de sortie vaut déjà $\Omega(C+L)$ ;
- écrire des runs externes bornés, les trier exactement, puis effectuer une fusion multi-voies vers le catalogue final.

La forêt impose un second ordre scientifique : les valeurs de rayon doivent être triées et les égalités regroupées atomiquement par `sphere_cmp_beta`. Traiter un événement dès sa découverte par le DFS serait faux, car des événements égaux éloignés dans l'arbre appartiennent au même lot. Là encore, il faut stocker la sortie ou employer un tri externe exact. Le reverse search ne supprime pas cette barrière informationnelle.

## 9. Le facteur $nV$ reste entier

Le snapshot recherche chaque voisin par un rescan du nuage. Même en regroupant les deux directions d'un pinceau en un seul passage, chaque sommet paie quatre scans, auxquels s'ajoutent :

- le census qui trouve $B(v)$ et $h(v)$ ;
- le calcul du parent des voisins candidats pour reconnaître les enfants ;
- les censuses des propriétaires d'arités deux et trois ;
- le census fermé de chaque événement publié.

Le temps reste donc en $\Theta(nV)$, avec une constante probablement supérieure au DFS avec `seen`. $V$ compte toujours les sommets shallow non bien centrés et non publiés.

Préconstruire l'ordre de tous les pinceaux de triples déplacerait ce coût vers une structure globale en $\binom{n}{3}$ et violerait directement l'invariant architectural. La seule fermeture produit crédible combine le parent ci-dessus avec un oracle de voisinage **à la demande**, sous-linéaire et certifié : listes de conflits bornées, cutting shallow ou index spatial exact, sans cache proportionnel à l'ensemble du squelette.

## 10. Verdict architectural

Le reverse search est une amélioration réelle et démontrée de la voie `order_k` simple :

- il retire la mémoire $O(V)$ de `seen`, `visited` et `frontier` ;
- il ne matérialise aucune incidence globale ;
- son parent est local, exact et ne demande pas de LP ;
- des propriétaires canoniques peuvent remplacer les tables de doublons des arités deux et trois sous les mêmes hypothèses simples.

Il ne transforme pas le prototype en voie produit :

1. le germe et les constantes coplanaires restent des portes indépendantes ;
2. `RelevantGP` ne certifie pas l'arrangement simple ;
3. le facteur $\Theta(nV)$ demeure ;
4. aucun reçu ne borne $V$, la profondeur ou le travail de parent ;
5. les sorties et les lots exacts conservent leur coût incompressible ;
6. les multiplicités demandent le traitement séparé du rapport suivant.

Décision : **GO comme expérience de mémoire sur l'oracle simple ; NO-GO comme générateur M3 produit tant qu'un oracle de voisinage sous-linéaire et la voie multiplicitaire ne sont pas prouvés.**

GCP non utilisé.
