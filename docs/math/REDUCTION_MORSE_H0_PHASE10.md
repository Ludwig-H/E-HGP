# Réduction horizontale minimale de Phase 10

## Décision corrigée

Phase 10, backend `reference_cpu`, profil `hgp_reduced`, mode `certified`, statut interne `conditional_h0_candidate` et `public_status=not_claimed`. Sa fermeture est administrative et logicielle : elle autorise l'architecture de Phase 14, mais ne transforme pas l'obligation de fidélité globale ci-dessous en théorème.

Les minima directs et les racines réduites sont deux objets différents. Pour $k\geq2$, une facette minimale isolée est un **carrier latent** de la composante complète; elle n'est pas un nœud de `hgp_reduced`. Un groupe de simplexes de Gabriel crée ou conserve une racine réduite selon le nombre $q_R$ de composantes strictes non triviales absorbées :

- $q_R=0$ : naissance réduite sans enfant;
- $q_R=1$ : continuation de l'unique racine;
- $q_R\geq2$ : multifusion des $q_R$ racines gelées.

À l'ordre un seulement, chaque singleton est immédiatement une racine publique. Cette distinction empêche le faux arbre du triangle aigu : ses arêtes restent latentes, puis le triangle crée une naissance réduite, pas une multifusion ternaire.

## Base géométrique

Soit un simplexe de Gabriel $S=I\cup U$ de cardinal $k+1$ et de niveau $a=\beta(S)$. Son support positif minimal $U$ vérifie, dans le domaine générique tridimensionnel :

$$2\leq\lvert U\rvert\leq4.$$

Pour chaque $u\in U$, la facette stricte $F_u=S\setminus\lbrace u\rbrace$ vérifie :

$$\beta(F_u)<\beta(S)=a.$$

Les suppressions $S\setminus\lbrace x\rbrace$ avec $x\in I$ restent au niveau $a$. Elles ne sont ni des bras stricts, ni des composantes pré-lot. De plus, l'union des bras stricts contient déjà tous les points de $S$ puisque $\lvert U\rvert\geq2$.

Le Théorème 4 justifie le rôle séparateur des simplexes de Gabriel, mais la Proposition 6 et le Théorème 5 du manuscrit ne peuvent pas justifier à eux seuls la suppression de toutes les incidences non Gabriel : la fixture exacte E5 les contredit pour le graphe élagué brut. Une facette peut être attachée silencieusement par une coface non Gabriel, puis être réutilisée par une selle Gabriel tardive. La chaîne candidate énumère donc les simplexes de Gabriel sans matérialiser Gamma, mais récupère l'information d'attache par la descente stricte de chaque bras vers un carrier antérieur. Les requêtes de première incidence $\lambda(F)$ et leurs co-minimiseurs $M(F)$ restent des audits de promotion; elles ne remplacent ni le catalogue des selles tardives, ni la preuve globale que les carriers paresseux représentent toutes les composantes strictes de Gamma.

Une facette stricte encore isolée ne peut pas devenir non triviale pour la première fois par une coface non Gabriel dans le domaine régulier. Soient $F$ cette facette et $Q=F\cup\lbrace x\rbrace$ une coface de première incidence. Si un intrus strict $z$ appartient à la miniball de $Q$, deux cas sont possibles : si $x$ n'est pas essentiel, alors $\beta(F)=\beta(Q)$, contrairement à la stricte antériorité de $F$; si $x$ est essentiel, alors $\beta(F\cup\lbrace z\rbrace)<\beta(Q)$, contrairement à la minimalité de cette première incidence. Ainsi, toute première promotion d'un carrier isolé est portée par un simplexe de Gabriel. Ce lemme local ne prouve toutefois pas que toutes les attaches silencieuses déjà survenues sont fidèlement représentées lorsqu'une selle tardive réutilise leur facette.

Cette conclusion utilise la position générale, un support essentiel unique et l'absence de shell extérieur. Par exemple, un plateau cosphérique peut activer simultanément facettes et cofaces sans événement de rang intermédiaire; il appartient à la Phase 13 et doit échouer fermé en Phase 10.

## Carriers complets et racines réduites

Chaque minimum direct de rang $k$ reçoit un handle stable dans le locator. Ce handle représente une composante complète issue de la réduction Morse :

- pour $k=1$, il porte dès sa création le nœud public du singleton;
- pour $k\geq2$, il ne porte initialement aucun nœud réduit;
- après un groupe non différé, tous les handles absorbés sont unis et la composante résultante porte la racine réduite créée ou conservée par ce groupe.

Un bras strict est fermé par 10.5c sous le snapshot pré-lot :

$$F_u=F_0\longrightarrow F_1\longrightarrow\cdots\longrightarrow F_L,\qquad\beta(F_L)<\cdots<\beta(F_1)<\beta(F_0)<a.$$

Le terminal doit être lié à un minimum direct strictement antérieur. Cette liaison identifie le carrier complet; elle ne prétend pas à elle seule que ce carrier possède une racine réduite.

## Quotient simultané corrigé

Pour un lot exact $(k,a)$ :

1. reconstruire les bras stricts de tous les simplexes de Gabriel catalogués dans le lot;
2. fermer toutes les clés distinctes sous le même snapshot;
3. remplacer chaque bras par le carrier complet gelé de son terminal;
4. typer chaque carrier comme $R(r)$ s'il porte la racine réduite antérieure $r$, ou comme $L(h)$ s'il est encore latent avec le handle $h$;
5. calculer sans mutation les composantes de l'hypergraphe sur l'union disjointe des sommets $R$ et $L$;
6. dans chaque composante, dédupliquer uniquement les identifiants $r$ des sommets $R(r)$ et obtenir $q_R$;
7. préparer la naissance, la continuation ou la multifusion selon $q_R$;
8. unir tous les carriers du groupe, y compris ceux encore latents;
9. attacher la racine résultante au carrier uni;
10. engager ensuite les minima du niveau courant comme nouveaux carriers, publics seulement pour $k=1$.

Les hyperarêtes du quotient portent les composantes complètes, pas seulement les racines publiques. Les sommets $L$ ne sont surtout pas supprimés avant la fermeture transitive : deux selles peuvent partager uniquement un carrier latent et appartenir au même groupe. La fixture miroir 6.18 impose alors une naissance commune $q_R=0$, et non deux naissances séquentialisées. Les facettes isolées absorbées ne deviennent jamais des enfants.

## Obligation horizontale de fidélité des carriers

**Proposition conditionnelle, non close.** Fixons un ordre $k$ dans le domaine générique et supposons :

1. la façade terminale de Phase 9 énumère complètement les simplexes de Gabriel pertinents, y compris les selles tardives, et leurs minima directs;
2. chaque simplexe fournit exactement les bras $F_u$ pour $u\in U$;
3. avant chaque lot $(k,a)$, les carriers actifs sont en bijection avec les composantes pertinentes de la coupe stricte de Gamma, et 10.5c associe chaque bras au carrier de sa composante;
4. le snapshot des carriers et de leurs éventuelles racines réduites reste gelé pendant tout le lot;
5. toutes les hyperarêtes du même niveau sont quotientées avant mutation;
6. la porte de régularité exclut supports multiples, shells extérieurs et plateaux non résolus;
7. chaque cas dégénéré, autorité divergente ou budget épuisé échoue fermé.

Alors la règle $q_R=0,1,\geq2$ ci-dessus produit la même généalogie de composantes non triviales, vues comme ensembles de points, que Gamma et donc que les composantes de Čech dans le domaine annoncé.

### Induction conditionnelle

Par l'hypothèse de fidélité, la fermeture stricte place chaque bras dans sa composante complète pré-lot. Les hyperarêtes de carriers reconstruisent donc les contractions simultanées imposées par les simplexes de Gabriel. L'ordre physique est sans effet puisque le quotient est la fermeture transitive d'une relation symétrique sur un snapshot commun.

À l'ordre $k\geq2$, une composante à une seule facette ne porte pas de racine réduite. Un groupe sans racine antérieure crée la première composante non triviale et donc une naissance réduite. Un groupe à une racine conserve cette composante. Un groupe à plusieurs racines fusionne exactement ces composantes; les carriers isolés absorbés n'ajoutent aucun parent mais leurs points sont déjà contenus dans l'union des bras stricts.

Une coface non Gabriel régulière est une attache silencieuse $q=1$ sans nouveau point couvert; elle ne crée donc aucun nœud réduit, mais peut modifier l'appartenance future d'une facette. Une coface Gabriel contracte les carriers de ses bras stricts, et les suppressions intérieures égales n'ajoutent aucun point au-delà de l'union de ces bras. La fermeture transitive du lot traite les niveaux égaux sur un snapshot commun. Sous l'hypothèse de fidélité, ces trois faits préservent l'invariant par induction sur les niveaux exacts.

Le point non démontré est précisément la surjectivité inductive suivante : tous les carriers terminaux appartenant à une même composante stricte de Gamma doivent déjà être unis au même représentant, même lorsque le chemin de Gamma a utilisé auparavant des cofaces non Gabriel non matérialisées. De façon équivalente, le graphe sparse des minima et selles enrichi par les descentes doit être un certificat $H_0$ complet. E5 vérifie localement le cas discriminant `AC` vers la composante de `DE`, puis la continuation `ABC` au niveau $83886/3563$; ce témoin logiciel ne clôt pas la propriété globale.

La proposition conditionnelle ne certifie pas la famille complète des facettes d'une composante, `full_pi0`, les deltas de couverture détaillés, M.1, la tour verticale ou un statut public exact.

## Échec fermé

Avant le quotient, chaque bras doit posséder :

- sa clé complète;
- une chaîne stricte fraîchement rejouée;
- le stamp pré-lot commun;
- un terminal `relative_positive`;
- un handle dont le minimum direct, l'ordre et le niveau sont recertifiés;
- une racine réduite optionnelle lue uniquement dans le snapshot gelé.

Un miss, un terminal non strict, une autorité non rejouée ou un budget épuisé vide toutes les actions scientifiques du lot. Une absence de racine réduite est en revanche un carrier latent valide : elle contribue à l'incidence du groupe mais pas à $q_R$.

## Audits facultatifs

10.6--10.15 et Gamma exhaustif restent des falsificateurs :

- vérifier que $\lambda(F)$ et tous les co-minimiseurs $M(F)$ retrouvent exactement les premières promotions des minima latents, sans les employer comme générateur de toutes les selles;
- comparer les carriers aux composantes complètes sur petits nuages;
- contrôler la classification `prior_nontrivial_reduced_root` ou `omitted_isolated_facet`;
- détecter une coface de Gabriel manquante ou une contraction incorrecte;
- conserver toute contradiction comme fixture permanente.

Ils ne deviennent ni l'architecture produit, ni une condition de performance, ni une promotion de statut.

## Coûts et structures évitées

Pour $J$ simplexes de Gabriel et $A$ bras stricts :

$$A=\sum_{S}\lvert U_S\rvert\leq4J.$$

Si $V$ est le nombre de clés distinctes visitées par les descentes et $T$ le nombre de terminaux, alors :

$$E=V-T<V\qquad\text{pour }V>0.$$

Le stockage transitoire des descentes vaut $O(A+KV)$ pour $K\leq10$. Le journal persistant reste proportionnel aux minima directs, bras, groupes, enfants et racines utiles. Il ne construit pas :

- les suppressions intérieures comme graines;
- un catalogue global de gateways;
- l'univers des $\binom{n}{k}$ facettes ou des $\binom{n}{k+1}$ cofaces;
- Gamma, ses coupes ou son histoire;
- une cellule top-$m$ ou une mosaïque de Delaunay d'ordre supérieur;
- un snapshot DSU complet par lot;
- une partition top-$K$ persistante par bras.

Une requête LBVH peut rester linéaire au pire cas; cette réduction ne qualifie donc encore aucun SLO.
