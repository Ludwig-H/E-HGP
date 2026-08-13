# Audit du descripteur géométrique

## Conclusion

Le maximum directionnel proposé doit être scindé en objets mathématiques différents. La **fonction support** convexifie toujours. La **fonction radiale extérieure** conserve une forme exactement seulement si elle est étoilée autour du centre choisi. Ni l'une ni l'autre ne représente en général la non-convexité d'un $K$-polyèdre HGP. Elles restent des canaux utiles, mais pas une identité de forme.

Avant tout apprentissage, le pipeline doit aussi préciser ce qu'il appelle « réalisation géométrique ». Dans la source HGP, les sommets du graphe $\Gamma^{K}(X,r)$ sont les $(K-1)$-simplexes du complexe de Čech ; deux sont adjacents lorsque leur union est encore un simplexe, et un $K$-polyèdre est l'ensemble des points de $X$ apparaissant dans une composante connexe de ce graphe. Ce n'est donc pas, par définition, un solide géométrique canonique. Pour $K\geq2$, ces ensembles peuvent en outre se chevaucher. Le seul ensemble de points et un arbre parent–enfant ne déterminent ni quels simplexes réaliser, ni leurs niveaux, ni une réalisation laminaire.

## Quatre représentations à ne pas confondre

### Support : enveloppe convexe seulement

Pour un ensemble compact $P\subset\mathbb{R}^{3}$, $h_P(u)=\sup_{x\in P}\langle u,x\rangle$. On a, pour toute direction, $h_P(u)=h_{\mathrm{conv}(P)}(u)$. Si $P$ est une union de simplexes de sommets $V$, alors $h_P(u)=\max_{v\in V}\langle u,v\rangle$. Les incidences, trous, composantes, dimensions des simplexes et niveaux HGP sont invisibles.

Ce résultat est une fixture de correction : `support(realisation) == support(vertices)`. Une différence numérique ne constitue pas un gain d'information ; elle révèle un désaccord de conventions ou un défaut d'implémentation.

### Rayon extérieur : enveloppe étoilée

Pour un compact non vide $P$, un centre candidat $c\in\mathbb{R}^{3}$ et une direction unitaire $u$, définir l'ensemble des intersections radiales $I_{P,c}(u)=\left\lbrace r\geq0:c+ru\in P\right\rbrace$ et $\rho_{P,c}(u)=\sup\left(I_{P,c}(u)\cup\left\lbrace0\right\rbrace\right)$. La quantité « norme maximale dans une direction » est $\rho$, pas $h$. Un masque `ray_hit` distingue un rayon vide d'une intersection à $r=0$.

La reconstruction induite est $R_c(P)=\left\lbrace c+ru:u\in\mathbb{S}^{2},\ 0\leq r\leq\rho_{P,c}(u)\right\rbrace$. On a $R_c(P)=P$ si et seulement si $P$ est étoilé par rapport à $c$, propriété qui implique $c\in P$. Sinon, elle remplit les intervalles radiaux absents et peut supprimer trous, concavités ou disconnexions. Elle dépend fortement du choix de $c$ ; si `center_in_realization=false`, aucune reconstruction exacte n'est possible.

Contre-exemple permanent : un cube plein tétraédralisé et sa seule frontière triangulée ont le même support et le même rayon extérieur depuis le centre, mais des intérieurs, homologies et volumes différents.

### Intersections multi-segments : plus fidèles, mais fragiles

Conserver toutes les composantes de $I_{P,c}(u)$ — intervalles éventuellement dégénérés en points — distingue les trous le long des rayons échantillonnés. Cette représentation a une longueur variable et n'est pas injective avec un nombre fini de directions. Pour un complexe de dimension un ou deux plongé dans $\mathbb{R}^{3}$, un rayon générique peut ne rencontrer aucune arête ou face. Des cônes angulaires ou un épaississement corrigent ce défaut au prix d'un nouveau paramètre de bande passante.

### Transformées de masse et de topologie

Les CDF de projections de la mesure ponctuelle conservent la masse intérieure. La collection continue sur toutes les directions détermine cette mesure par Cramér–Wold, mais elle ne détermine pas les incidences simpliciales ; une grille finie reste un sketch.

Pour un complexe géométrique $P$, l'Euler Characteristic Transform peut être définie par $\mathrm{ECT}_P(u,t)=\chi\left(P\cap\left\lbrace x:\langle u,x\rangle\leq t\right\rbrace\right)$. Les transformées ECT/PHT complètes possèdent déjà des résultats d'injectivité sur des classes de formes constructibles ou complexes PL. Les variantes pondérées et différentiables sont également antérieures : ECT/WECT doivent donc être des oracles bornés et des baselines, pas une nouveauté revendiquée.

Leur emploi exige un vrai complexe plongé. Si des simplexes HGP se recouvrent sans former un complexe simplicial conforme, il faut prouver une subdivision cohérente ou distinguer explicitement l'ECT du complexe abstrait de celle de l'union géométrique.

Une autre baseline fusionnable est un embedding de mesure par fréquences de Fourier ou noyau caractéristique. La fonction caractéristique continue admet une composition exacte sous déplacement, ratio d'échelle et mélange pondéré ; un ensemble fini de fréquences n'est en général pas fermé sous le changement d'échelle et demande interpolation ou borne de discrétisation. Fonctions caractéristiques et kernel mean embeddings sont par ailleurs classiques. Une contribution devrait porter sur un sketch HGP multi-résolution, sa discrétisation et sa stabilité. Pour une topologie robuste aux outliers, la distance à une mesure fournit également un contrôle antérieur à ne pas réinventer.

## L'invariance d'échelle ne modélise pas la portée LiDAR

Une homothétie métrique d'un objet et son observation plus lointaine par un LiDAR sont deux opérations différentes. La portée modifie l'espacement angulaire, le nombre de retours, l'occultation, l'incidence et parfois la rémission ; elle ne multiplie pas simplement toutes les coordonnées relatives par un scalaire. La normalisation peut aider au partage de forme, mais elle ne justifie aucun claim de robustesse à longue portée.

Le modèle doit garder séparément taille métrique, portée, cardinalité, direction de vue et densité. Le stress test correct transporte un patch, le rééchantillonne selon un modèle capteur déclaré, puis compare arbre, descripteur et prédiction. Une simple commande `scale(points)` ne suffit pas.

## Décision de représentation

Comparer à dimension, bits, FLOPs et latence proches :

1. moments, covariance et statistiques HGP ;
2. support maximal et quantiles/CDF de points ;
3. rayon extérieur, puis intersections multi-segments ou occupation conique ;
4. distributions pondérées d'attributs simpliciaux ;
5. ECT/WECT et distance à une mesure comme contrôles topologiques ;
6. sketch de Fourier ou embedding par noyau caractéristique ;
7. Deep Sets ou mini-PointNet de même budget.

Le support est conservé comme canal d'extrêmes. Le rayon extérieur n'est promu que si la fraction de nœuds étoilés est élevée ou si son gain subsiste face aux contrôles. Une ECT finie n'est promue que si son coût et sa stabilité au thinning sont compétitifs. Le backbone local et le chemin résiduel point-wise restent obligatoires dans tous les cas.

## Théorèmes et certificats prioritaires

| Priorité | Énoncé candidat | Valeur scientifique | Condition de survie |
|---|---|---|---|
| T0 | support simplicial = support des sommets ; reconstruction radiale exacte ssi forme étoilée | hygiène mathématique, pas nouveauté | tests exhaustifs sur fixtures |
| T1 | sketch HGP fusionnable, stable au thinning dépendant de la portée, avec erreur finie directions/bins | contribution possible | borne non vacue et gain sur ECT/CDF/Deep Sets |
| T2 | raffinement adaptatif de l'attention avec borne calculable sur KL ou sortie et coût contrôlé | candidat central | certificat sous-quadratique et Pareto réel contre HSA/FMA |
| T3 | stabilité ou merge distortion de l'arbre HGP sous un modèle d'observation LiDAR corrigé | contribution forte, très difficile | hypothèses réalistes, régime de $K$ explicite et second capteur |

Il n'est pas légitime de viser un théorème « HGP est sémantiquement optimal » sans modèle joint précis de géométrie, échantillonnage et labels. Une meilleure approximation d'un arbre de densité n'implique pas, seule, un meilleur mIoU.

## Fixtures de falsification

- cube plein contre frontière, mêmes $h$ et $\rho$ ;
- forme concave contre son remplissage radial ;
- mêmes sommets, incidences simpliciales différentes ;
- centre hors forme ou hors noyau étoilé ;
- complexe $K=1$ ou surface $K=2$ avec majorité de rayons vides ;
- même objet métrique sous thinning uniforme, range-aware et occultation ;
- permutation ou duplication de simplexes sans changement du support ;
- recouvrements $K\geq2$ avant et après projection laminaire.

Toute collision qui invalide un claim devient une fixture permanente ; elle n'est pas retirée lorsque le descripteur change.
