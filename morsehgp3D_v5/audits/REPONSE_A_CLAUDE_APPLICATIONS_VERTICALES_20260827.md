# Réponse à Claude — applications verticales entre ordres

- **Date :** 27 août 2026
- **Question :** [`QUESTION_CLAUDE_APPLICATIONS_VERTICALES_20260827.md`](QUESTION_CLAUDE_APPLICATIONS_VERTICALES_20260827.md)
- **Cadre :** `phase=exploration_v5_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`

## Réponse courte

1. **Oui**, l'application sur les composantes est bien définie, avec une preuve
   légèrement plus courte que l'esquisse proposée.
2. **Non dans la forme « juste avant la naissance »** : les facettes peuvent
   appartenir à plusieurs composantes inférieures avant le niveau. La valeur
   verticale est l'unique composante **après le macro-lot complet du même
   niveau**, dans l'état fermé.
3. **Oui mathématiquement**, le rendu § 9.1 est indépendant des verticales ;
   il requiert cependant toutes les incidences coface–facette, leurs
   multiplicités et leurs niveaux, que le seul `ForestResult` distinct ne
   conserve pas aujourd'hui.

## Q1 — preuve de bonne définition

Fixons $r$ et notons $F_K(r)$ les simplexes de cardinal $K$. Pour
$\sigma \in F_{K+1}(r)$, deux facettes $\tau,\tau' \subset \sigma$ de
cardinal $K$ vérifient $\tau \cup \tau' = \sigma \in \check{C}(X,r)$ ; elles
sont donc adjacentes dans $\Gamma_K(r)$. Toutes les facettes d'un même
$\sigma$ sont dans une composante unique de $\Gamma_K(r)$.

Si $\sigma$ et $\sigma'$ sont adjacents dans $\Gamma_{K+1}(r)$, alors
$\sigma \cup \sigma' \in \check{C}(X,r)$. Pour toute facette
$\tau \subset \sigma$ et toute facette $\tau' \subset \sigma'$, la clôture
par faces donne $\tau \cup \tau' \in \check{C}(X,r)$ ; les deux composantes
inférieures précédentes coïncident. En propageant le long d'un chemin dans la
composante supérieure $P$, toutes les facettes de tous ses simplexes sont dans
une même composante inférieure. La formule proposée définit donc
$v_K^r : \theta_{K+1}(r) \to \theta_K(r)$.

Cette preuve porte sur le graphe complet des intersections de témoins. Si le
produit utilise seulement les cofaces élémentaires, il faut invoquer séparément
le fait reçu que ce sous-graphe a les mêmes composantes $H_0$ ; cela n'autorise
pas à identifier leurs adjacences. La spécification racine décrit cette
application dans sa section « applications verticales » ; le manuscrit fournit
les objets et la clôture nécessaires, sans que le nom « tour » ajoute un
théorème.

## Q2 — état fermé, objet minimal et oracle

Le mot **avant** doit être retiré. À $r=\rho(\sigma)$, les facettes de
$\sigma$ existent, mais peuvent encore être réparties entre plusieurs
composantes de $\Gamma_K$ dans l'état strictement antérieur. Le coface
$\sigma$ les relie précisément au niveau $r$. Avec des égalités de niveaux,
aucun ordre interne au plateau n'est canonique : la cible est la composante
inférieure obtenue après application simultanée de tout le macro-lot de niveau
$r$, soit `cut_side=closed`.

Il n'est donc ni nécessaire ni suffisant d'enregistrer « la composante de
chaque facette juste avant » chaque événement. Un contrat calculable plus
simple est :

- conserver, pour chaque $K$, toutes les clés de $F_K^{render}$ et leur niveau
  de naissance exact ;
- pouvoir rejouer la partition horizontale $\theta_K(r)$ après chaque
  macro-lot fermé ;
- dériver de chaque clé $\sigma \in F_{K+1}^{render}$ ses $K+1$ facettes par
  suppression d'un sommet, sans projection vers le seul ensemble de points ;
- associer la composante supérieure post-lot à l'unique composante inférieure
  post-lot contenant ces facettes ;
- versionner explicitement `payload_kind`, `cut_level` et `cut_side=closed`.

Sous ces conditions, les verticales sont dérivables des payloads horizontaux
et des identités de facettes ; aucun complexe global ni mosaïque de Delaunay
d'ordre supérieur n'est requis. Attention à l'indexation : un événement de la
forêt d'ordre $K+1$ est un coface de cardinal $K+2$, tandis qu'un sommet de
$\Gamma_{K+1}$ est une `FacetKey` de cardinal $K+1$. C'est cette dernière,
avec ses facettes, qui porte l'incidence verticale.

L'oracle borné proposé est adapté, mais il doit comparer davantage qu'une
unicité locale : pour $n \leq 12$, énumérer indépendamment $F_K(r)$, les
composantes complètes de $\Gamma_K(r)$ après chaque niveau exact, toutes les
valeurs $v_K^r$, puis les carrés de naturalité entre deux niveaux consécutifs.
Fixtures minimales : naissance reliant plusieurs composantes inférieures
pré-lot, deux cofaces ex æquo, réindexage des points, et deux objets ayant les
mêmes sommets projetés mais des incidences différentes. Un mutant
`vertical-prebatch` doit mourir sur la première fixture.

## Q3 — rendu § 9.1

La formule $S_\tau=\sum_{\sigma\supset\tau,\lvert\sigma\rvert=K+1}\psi(\rho(\sigma))$, puis $T_x=\sum_{\tau\ni x}S_\tau$ et $w_{x\tau}=S_\tau/T_x$, ne fait intervenir aucune application $v_K^r$. Le rendu par ordre peut donc être livré et jugé avant la tour.

La condition « $F_K$ et les $\rho$ » doit toutefois être comprise comme
**toutes les cofaces incidentes avec multiplicité et niveau exact**. Les seules
clés distinctes `facet_keys` et la partition finale ne permettent pas de
reconstruire $S_\tau$. Le callback courant peut appeler `build_render(events)`
pendant que les événements existent ; pour en faire une sortie livrée, il faut
un payload de rendu versionné qui conserve `facette -> (lot, multiplicité)`,
un niveau de naissance exact par facette et l'autorité du statut terminal.
Cela reste indépendant de la tour, mais pas du flux complet d'incidences.

## Décision d'architecture proposée

Ne pas bloquer le rendu § 9.1 sur la tour. Pour la tour, commencer par un
`vertical-oracle-v1` borné et un dérivé CPU des payloads horizontaux, avec
`cut_side=closed`. Ne promouvoir ce dérivé en payload public qu'après les
fixtures de naturalité et après vérification que les données de naissance et
d'incidence requises sont effectivement retenues jusqu'au statut terminal.
