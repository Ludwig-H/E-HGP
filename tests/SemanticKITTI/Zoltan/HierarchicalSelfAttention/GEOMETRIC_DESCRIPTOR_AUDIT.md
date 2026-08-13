# Audit du descripteur géométrique

## Conclusion corrigée

La proposition `support normalisé + payload HGP marqué d'un carrier non convexe` n'est pas réfutée par la convexification de la fonction support. Le premier canal décrit volontairement l'enveloppe convexe ; le second conserve les cellules, incidences, coordonnées et niveaux qui reconstruisent le carrier choisi. Le payload fini n'est lui-même ni convexe ni non convexe. Le rayon extérieur n'est pas ce second canal : il devient seulement une ablation compressée et lossy.

L'évaluation doit toutefois déclarer quel objet HGP est encodé. Le [contrat de la branche complète](POLYHEDRAL_COMPLEX_BRANCH.md) distingue le $K$-polyèdre discret du manuscrit, deux carriers PL et l'union témoin canonique. Les identités de support ne se transfèrent pas de l'un à l'autre.

## Objet source HGP

À l'ordre $K$ et au niveau carré $a$, les sommets de $\Gamma_{K}^{\mathrm{full}}(a)$ sont les facettes $F\subseteq X$ de cardinal $K$ telles que $\beta(F)\leq a$, et $F,F'$ sont adjacentes si $\beta(F\cup F')\leq a$. Le sous-graphe $\Gamma_{K}^{\mathrm{elem}}(a)$ ne garde que $|F\cup F'|=K+1$ et représente ces arêtes par des cofaces élémentaires. Il est strictement moins informatif en général, mais possède les mêmes composantes connexes par une chaîne de remplacements d'un sommet. Pour une composante commune $v$, le $K$-polyèdre défini dans le manuscrit est seulement l'union discrète $V_v=\bigcup_{F\in\mathcal{F}_v}F$ des observations qui apparaissent dans ses facettes.

Une branche incidence-aware doit recevoir au minimum les points, facettes, cofaces élémentaires de connexion, relations point--facette--coface, coordonnées et niveaux exacts. Ces cofaces reconstruisent $\Gamma_{K}^{\mathrm{elem}}$, pas l'adjacence full ; celle-ci demande ses propres arêtes ou un oracle certifié. Pour $K\geq2$, plusieurs composantes peuvent partager des observations. Cette multiplicité fait partie de l'objet ; une laminarisation la modifie.

La branche persistante évolue avec $a$. Le schéma versionne donc `cut_policy`, `cut_level`, `cut_side` et les deltas d'événements. La baseline est attachée à l'arête $p\leftarrow v$ et utilise `cut_policy=pre_parent`, `cut_side=strict` à $a_p$, avant le lot de fusion ; une racine utilise `cut_policy=explicit`, `cut_side=closed` au dernier niveau fini. Une politique parmi `pre_parent|post_birth|explicit` avec d'autres niveau ou côté est une autre expérience, pas une variation silencieuse du même nœud.

## Carriers à ne pas confondre

### Carrier PL des facettes

Définir $C_v^{F}(a)=\bigcup_{F\in\mathcal{F}_v(a)}\mathrm{conv}(F)$. Cet ensemble peut être non convexe, mais ses cellules droites ne forment pas nécessairement un complexe géométrique conforme dans $\mathbb{R}^{3}$. On a exactement $h_{C_v^{F}}(u)=h_{V_v}(u)$ : le support du carrier PL est celui de ses sommets.

Cette égalité signifie que le **canal support** est dérivable du carrier complet, pas que le carrier est équivalent à son support. Deux carriers construits sur les mêmes sommets peuvent avoir supports identiques et incidences différentes.

### Carrier PL des cofaces

Définir $C_v^{Q}(a)=\bigcup_{Q\in\mathcal{Q}_v^{\mathrm{elem}}(a)}\mathrm{conv}(Q)$ et $U_v^{Q}=\bigcup_{Q\in\mathcal{Q}_v^{\mathrm{elem}}}Q$. Il représente les cellules qui connectent les facettes et peut différer de $C_v^{F}$. Une facette isolée n'apparaît dans aucune coface ; la variante qui la réinjecte doit être nommée explicitement. Hors cas vide, $h_{C_v^{Q}}=h_{U_v^{Q}}$ et l'identité avec $h_{V_v}$ vaut exactement lorsque $\mathrm{conv}(U_v^{Q})=\mathrm{conv}(V_v)$. La couverture $U_v^{Q}=V_v$ est suffisante, pas nécessaire.

### Union témoin canonique

La région témoin d'une facette est $T_a(F)=\bigcap_{x\in F}\overline{B}\left(x,\sqrt{a}\right)$ et l'union de la composante est $W_v(a)=\bigcup_{F\in\mathcal{F}_v(a)}T_a(F)$. Puisque $T_a(F)\cap T_a(F')=T_a(F\cup F')$, $\Gamma_{K}^{\mathrm{full}}(a)$ est littéralement le graphe d'intersection des régions. Son sous-graphe élémentaire a seulement les mêmes composantes, par la chaîne de remplacements. En posant $L_K^{X}(a)=\left\lbrace y:\left|X\cap\overline{B}(y,\sqrt{a})\right|\geq K\right\rbrace$, on a $L_K^{X}(a)=\bigcup_FT_a(F)$ et $W_v(a)$ est exactement la composante correspondante.

$W_v$ est un objet curviligne dans l'espace des centres de boules, pas un polytope ni une surface LiDAR reconstruite. Son support n'est en général **pas** celui de $V_v$. La fixture `support(realisation) == support(vertices)` s'applique donc à $C_v^{F}$, jamais à $W_v$.

## Rôle correct du support

Pour un compact $P\subset\mathbb{R}^{3}$, $h_P(u)=\sup_{x\in P}\langle u,x\rangle$ et $h_P=h_{\mathrm{conv}(P)}$. Le support normalisé est un résumé global interprétable, stable en distance de Hausdorff et peu coûteux. Il ne voit seul ni incidences, trous, densité intérieure ni niveaux HGP.

Dans le couple $(\widetilde h_v,\mathcal{P}_v)$, il joue un rôle de **shortcut** : le chemin complet pourrait théoriquement le recalculer, mais le réseau n'a pas à redécouvrir les directions extrêmes par plusieurs couches de message passing. L'ablation correcte est donc `objet complet seul` contre `support + objet complet`, et non `support` contre `rayon`.

## Fonction radiale : ablation seulement

Pour un compact $P$, un centre $c$ et une direction unitaire $u$, poser $I_{P,c}(u)=\left\lbrace r\geq0:c+ru\in P\right\rbrace$ et $\rho_{P,c}(u)=\sup\left(I_{P,c}(u)\cup\left\lbrace0\right\rbrace\right)$. La reconstruction par segments radiaux est exacte si et seulement si $P$ est étoilé autour de $c$. Sinon elle remplit des intervalles absents.

Un cube plein et sa frontière ont le même support et le même rayon extérieur depuis leur centre. Ce contre-exemple invalide `support + rayon` comme représentation complète ; il ne concerne pas `support + incidences/cellules complètes`. Rayon extérieur, intersections multi-segments et occupation conique restent des baselines de compression du carrier déclaré.

## Transformées de masse et de topologie

Les CDF de projections de la mesure ponctuelle conservent la masse mais pas les incidences. Leur collection continue détermine la mesure par Cramér--Wold ; une grille finie reste un sketch.

Pour un complexe PL conforme $P$, $\mathrm{ECT}_P(u,t)=\chi\left(P\cap\left\lbrace x:\langle u,x\rangle\leq t\right\rbrace\right)$ peut conserver de l'information topologique. ECT/PHT, WECT et leurs variantes différentiables sont des antériorités. Elles sont des contrôles, pas la définition du canal HGP complet. Aucun théorème ECT n'est transféré à un carrier aux intersections non conformes ou à $W_v$ sans vérifier ses hypothèses.

## Invariance et portée LiDAR

Sous une similitude $x\mapsto\lambda Rx+t$ avec $\lambda>0$ et $R\in\mathrm{O}(3)$, et $a\mapsto\lambda^2a$, les facettes actives, leurs incidences, les points source et les trois carriers dérivés se transforment de façon équivariante. Une normalisation cohérente fournit donc une invariance de forme à translation et échelle, et une équivariance à rotation si l'encodeur la respecte.

Voir le même objet plus loin n'est toutefois pas une homothétie métrique : thinning angulaire, occultation, incidence et rémission changent. Le modèle conserve taille, portée, cardinalité, direction de vue et densité comme side channels. Le stress test transporte puis rééchantillonne un patch selon un modèle capteur déclaré.

## Complétude et certificats sparse

Deux sérialisations ne sont équivalentes pour l'encodeur que si elles reconstruisent le même carrier marqué après canonicalisation. Un certificat `h0_only`, qui préserve seulement la composante et son union de points, ne prouve ni l'égalité des incidences, ni celle des carriers PL, ni celle de $W_v$.

Le contrat enregistre trois axes indépendants : `payload_kind=marked_incidence`, `carrier_kind=source_points|facet_pl|coface_pl|witness_union` et `authority=incidence_complete|pl_complete|witness_exact|witness_approx|h0_only`. Une approximation de $W_v$ est toujours `witness_approx` avec erreur $\varepsilon_W$ ; elle n'est jamais rangée sous `witness_exact`. Une variante Hodge/cochaîne exige en plus tous les rangs de $0$ à $K$, des orientations contractuelles et $d_{j+1}d_j=0$ ; le payload point--facette--coface seul est un hypergraphe typé, pas une chaîne complète.

Le produit v3 courant ne persiste pas encore ce payload composante-local complet. L'étude doit créer un exporteur sparse certifié ou rester sur un oracle borné ; elle ne peut pas déduire le canal 2 de la seule forêt réduite et ne doit pas matérialiser le complexe de Čech ambiant.

## Décision expérimentale

Comparer à budget et backbone identiques :

1. statistiques de points et support seul ;
2. multiensemble de facettes sans incidences ;
3. graphe $\Gamma_{K}^{\mathrm{elem}}$ seul, puis adjacence full explicite comme contrôle distinct ;
4. composante marquée complète `point ↔ facette ↔ coface` ;
5. carrier des facettes, carrier des cofaces et union témoin ;
6. objet complet seul contre support + objet complet ;
7. CDF, ECT/WECT, Deep Sets et encodeurs simpliciaux comme contrôles ;
8. rayon extérieur uniquement comme compression ablatée.

Apparier dimension, paramètres, FLOPs, bits, prétraitement et latence. Rapporter les collisions entre objets de labels différents, la sensibilité au thinning et le coût par nombre de cellules/incidences. Pour `witness_union`, ajouter $N_W$, nombre de requêtes ou patches effectivement consommés, $\varepsilon_W$, temps et mémoire ; l'appartenance à une composante teste $D_{K,v}(y)=\min_{F\in\mathcal{F}_v}\max_{x\in F}\left\Vert y-x\right\Vert\leq\sqrt{a}$, pas seulement le $K$-ième voisin global.

## Théorèmes et certificats prioritaires

| Priorité | Énoncé | Rôle |
|---|---|---|
| T0 | $\Gamma^{\mathrm{full}}$ intersection des témoins, $\Gamma^{\mathrm{elem}}$ même $\pi_0$, polyèdre discret--union témoin et support PL | correction |
| T1 | invariance à toute présentation sparse certifiée du même carrier marqué | reproductibilité |
| T2 | séparation de carriers ayant mêmes points/support mais incidences différentes | expressivité |
| T3 | similitude équivariante et stabilité filtrée sous perturbation, avec marge aux événements | contribution possible |
| T4 | union canonique et condensation fusionnable exacte ou à erreur bornée | contribution possible |
| T5 | raffinement d'attention piloté par un certificat calculable issu du carrier | candidat central |
| T6 | opérateur sur recouvrements conservant masse et stochasticité | candidat central difficile |

## Fixtures permanentes

- mêmes observations et même support, incidences différentes ;
- singleton $K=1$ à $a>0$ : $C_v^{F}=\left\lbrace x\right\rbrace$ mais $W_v(a)=\overline{B}(x,\sqrt{a})$, donc supports différents ;
- carrier des facettes contre carrier des cofaces ;
- carrier PL contre $W_v$, avec supports différents ;
- coface absente, facette isolée et politique d'augmentation ;
- même composante $H_0$ mais couvertures témoins différentes ;
- deux certificats sparse équivalents contre un certificat seulement `h0_only` ;
- replay `cut_policy=pre_parent`, `cut_side=strict` des deltas, racine explicite fermée et plateaux à égalité ;
- cube plein contre frontière pour l'ablation radiale ;
- recouvrements $K\geq2$ avant et après laminarisation ;
- même patch sous homothétie, puis sous transport et rééchantillonnage LiDAR.

Toute collision qui invalide un claim devient une fixture permanente ; elle n'est pas retirée lorsque l'encodeur change.
