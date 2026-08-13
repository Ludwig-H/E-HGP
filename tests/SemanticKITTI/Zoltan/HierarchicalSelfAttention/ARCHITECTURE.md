# Architecture proposée

## Principe

L'architecture de départ est hybride : une représentation locale forte protège les détails de surface et les frontières, tandis que la hiérarchie HGP apporte du contexte à plus grande échelle. Chaque branche reçoit deux canaux complémentaires : le support normalisé, raccourci global de son enveloppe convexe, et un **objet HGP marqué complet relativement au contrat d'export**, dont les incidences sont effectivement traitées. Ce payload est un certificat fini ; c'est son carrier déclaré qui peut être non convexe ou troué. Le second canal n'est ni une seconde fonction support, ni une réduction radiale. Cette décision suit aussi le signal négatif du papier HSA : remplacer indiscriminément toutes les couches basses d'un modèle plat peut dégrader fortement les résultats.

Nom de travail : **HGP-Hybrid Transformer**. Ce nom n'est pas un claim de nouveauté définitif.

## Contrat d'entrée de la hiérarchie

On suppose que chaque scan fournit :

- les points dans l'ordre original, avec `xyz` et rémission ;
- une forêt enracinée laminaire ;
- `parent`, `first_child`/`children`, profondeur et ordre topologique ;
- le mapping point–feuille et, si possible, des intervalles contigus de feuilles par nœud ;
- pour le canal marqué, les points, les facettes actives, les cofaces élémentaires de connexion, toutes leurs incidences requises, leurs niveaux de filtration et leur mapping vers les nœuds ;
- cardinalité, centre, rayon et statistiques HGP par nœud ;
- niveau de naissance, niveau de mort et persistance/stabilité lorsqu'ils sont définis ;
- l'ordre HGP $K$, la distance géométrique $d_{\mathrm{geo}}$, les paramètres de reconstruction et la version du reconstructeur ;
- un identifiant déterministe du scan et de la hiérarchie ;
- la version de `cut_policy`, `cut_level`, `cut_side` et les deltas ordonnés de cellules ou d'appartenances qui déterminent chaque état consommé.

Les labels SemanticKITTI ne participent jamais à la construction. Une hiérarchie par classe prédite serait une expérience aval différente et n'est pas admise dans l'encodeur sémantique principal.

Pour un ordre HGP $K$, les sommets de $\Gamma_K^{\mathrm{full}}(a)$ sont les facettes actives $F$ de cardinal $K$, et $F,F'$ sont adjacentes si $\beta(F\cup F')\leq a$. Le sous-graphe $\Gamma_K^{\mathrm{elem}}(a)$ ne conserve que les arêtes telles que $|F\cup F'|=K+1$ ; elles sont représentées par les cofaces élémentaires $\mathcal{Q}_v^{\mathrm{elem}}$. Toute arête complète se remplace par une chaîne d'échanges d'un sommet dans le simplexe actif $F\cup F'$. Les deux graphes ont donc les mêmes composantes $H_0$, mais ni les mêmes arêtes ni la même géométrie. Les cofaces élémentaires seules ne reconstruisent pas $\Gamma_K^{\mathrm{full}}$.

Pour une branche $p\leftarrow v$, l'objet contractuel est noté $\mathfrak{P}_{p\leftarrow v}=(V_v,\mathcal{F}_v,\mathcal{Q}_v^{\mathrm{elem}},\partial_v,\beta_v,\mu_v)$. Ici $V_v$ contient les identifiants de points et leurs coordonnées, $\mathcal{F}_v$ les facettes actives de cardinal $K$, $\mathcal{Q}_v^{\mathrm{elem}}$ les cofaces de cardinal $K+1$, $\partial_v$ les incidences point–facette et facette–coface, $\beta_v$ les niveaux exacts ou leur représentation sérialisée et $\mu_v$ les multiplicités ou poids de couverture explicitement définis. « Complet » signifie complet **relativement à ce contrat versionné** : aucun enregistrement exigé par le profil n'est omis. Cela ne signifie pas matérialiser tout le complexe de Čech, toute la mosaïque de Delaunay d'ordre supérieur ou toutes les cofaces ambiantes. Le consommateur refuse un payload qui ne permet pas de distinguer une omission d'une absence certifiée.

Le schéma fixe trois axes indépendants et n'emploie aucun alias :

- `payload_kind=marked_incidence` ;
- `carrier_kind=source_points|facet_pl|coface_pl|witness_union` ;
- `authority=incidence_complete|pl_complete|witness_exact|witness_approx|h0_only`.

`witness_exact` certifie toutes les facettes actives, leurs coordonnées et la coupe nécessaires à la reconstruction de l'union témoin. `witness_approx` enregistre obligatoirement la méthode et $\varepsilon_W$ ; `h0_only` ne satisfait pas le loader de la branche marquée. Les combinaisons incohérentes sont rejetées plutôt que réparées implicitement.

Un nœud persistant évolue entre sa naissance et sa mort : lui associer un unique carrier sans coupe est ambigu. La baseline sérialise donc un état **conditionné par l'arête**. Pour $p\leftarrow v$, `cut_policy=pre_parent`, `cut_level=a_p` et `cut_side=strict` sélectionnent les événements $\beta<a_p$, juste avant le lot de fusion du parent. Une racine utilise `cut_policy=explicit`, le dernier niveau fini sérialisé et `cut_side=closed`. Le format conserve naissance, mort, plateaux, deltas ordonnés et convention de lot pour les égalités ; un état strict vide ou un événement de persistance nulle reçoit un record dégénéré explicite ou est rejeté. La politique et sa version font partie du hash de run.

Le contrat distingue quatre carriers, qui ne sont jamais nommés tous « polyèdre » sans qualificatif :

- le $K$-polyèdre source HGP $C_v=V_v=\bigcup_{F\in\mathcal{F}_v}F$, qui est l'ensemble des observations du nœud ;
- le carrier PL des facettes $C_v^{F}=\bigcup_{F\in\mathcal{F}_v}\mathrm{conv}(F)$ ;
- le carrier PL des cofaces $C_v^{Q}=\bigcup_{Q\in\mathcal{Q}_v^{\mathrm{elem}}}\mathrm{conv}(Q)$, éventuellement augmenté des facettes isolées si le profil le déclare ;
- l'union témoin $W_v(a)=\bigcup_{F\in\mathcal{F}_v}\bigcap_{x\in F}\overline{B}(x,\sqrt{a})$, dont les composantes correspondent à celles de $\Gamma_K^{\mathrm{full}}(a)$ et donc, seulement au niveau $H_0$, à celles de $\Gamma_K^{\mathrm{elem}}(a)$.

Les facettes, leurs coordonnées et la coupe permettent de reconstruire implicitement $W_v(a)$ sous `authority=witness_exact`. Les cofaces élémentaires et incidences portent $\Gamma_K^{\mathrm{elem}}$ et sa filtration, pas toutes les arêtes de $\Gamma_K^{\mathrm{full}}$. Aucun théorème ni descripteur calculé pour un `carrier_kind` n'est transféré aux autres par simple changement de vocabulaire.

HSA a besoin d'un arbre. La baseline part donc de $K=1$ ou d'une laminarisation déterministe déjà spécifiée et auditée. Pour $K\geq2$, aucune projection de recouvrements n'est réputée prête tant qu'une fonction d'ownership $w_{iv}$ n'a pas un domaine explicite, ne vérifie pas $w_{iv}\geq0$ et $\sum_{v\in\mathcal{A}(i)}w_{iv}=1$, et ne passe pas les tests de conservation de masse, d'absence de double comptage et d'invariance à l'ordre. Une structure DAG demande un opérateur dédié. Dupliquer silencieusement les points ou arbitrer l'appartenance par ordre d'itération rendrait le résultat non interprétable. Un ensemble de sommets HGP ne doit pas être présenté comme le payload $\mathfrak{P}_{p\leftarrow v}$ si ses facettes, cofaces élémentaires, incidences et niveaux n'ont pas été sérialisés.

## Représentation des feuilles

Deux granularités seront comparées avec, dans les deux cas, une sortie finale par point :

1. **points comme feuilles**, solution de référence scientifique qui conserve directement la localisation point-wise ;
2. **micro-voxels ou micro-clusters comme feuilles**, seulement si les diagnostics de composition et le décodeur point-wise valident cette réduction ; la baseline majoritaire dure n'est pas à elle seule une porte sur cette granularité.

Le premier prototype doit partir d'une recette SemanticKITTI réellement reproductible et épinglable, par exemple MinkUNet/Cylinder3D ou une recette publique SphereFormer auditée. PTv3 reste le porteur Transformer ambitieux, mais son dépôt officiel ne fournit pas à ce jour une recette SemanticKITTI complète avec config, poids et résultat reproductible ; il ne doit donc pas bloquer WP0. Un second backbone fort devra vérifier que l'effet HGP n'est pas propre au porteur choisi. Le backbone reçoit exactement les mêmes entrées et la même recette dans toutes les comparaisons appariées.

Pour chaque feuille $i$, le backbone produit $f_i^{0}\in\mathbb{R}^{d}$. Les projections Q/K/V et les normalisations suivent la définition de la baseline HSA testée. Les coordonnées absolues ou cylindriques ne sont pas supprimées : le repère ego et la gravité sont sémantiquement utiles.

## État sémantique multiscale

Chaque nœud $v$ porte une **distribution de proportions sémantiques**, jamais un label dur. Soit $C_v^{\mathrm{lab}}\subseteq C_v$ le sous-ensemble des points dont le label n'est pas ignoré, $n_v^{\mathrm{lab}}=|C_v^{\mathrm{lab}}|$ et $n_{v,c}=\sum_{i\in C_v^{\mathrm{lab}}}\mathbf{1}\left\lbrace y_i=c\right\rbrace$. Sa cible est $\pi_v(c)=n_{v,c}/n_v^{\mathrm{lab}}$ pour $c\in\left\lbrace1,\ldots,19\right\rbrace$. Une feuille point valide a donc une cible one-hot, tandis qu'un cluster traversant une frontière conserve explicitement son mélange de classes. Un nœud sans label valide est masqué pour cette supervision.

À l'inférence, aucune tête indépendante n'est nécessaire dans l'architecture minimale. Les distributions point-wise sont agrégées sans label par $\widehat\pi_v^{\mathrm{all}}=n_v^{-1}\sum_{i\in C_v}p_i$, donc $\widehat\pi_p^{\mathrm{all}}=\sum_{v\in\mathrm{children}(p)}\frac{n_v}{n_p}\widehat\pi_v^{\mathrm{all}}$. Pour la loss seulement, $\widehat\pi_v^{\mathrm{lab}}=(n_v^{\mathrm{lab}})^{-1}\sum_{i\in C_v^{\mathrm{lab}}}p_i$ est comparé à $\pi_v$ ; ce masque GT n'entre jamais dans le forward de validation ou de test. Le backbone local fournit les $p_i^{(0)}$ initiaux ; après chaque bloc, les agrégats peuvent être recalculés. Un readout appris depuis l'état du nœud n'est qu'une ablation auxiliaire. Cet agrégat ne transforme pas silencieusement le nœud en token HSA.

Lorsque les enfants forment une partition laminaire exacte du parent et $n_p^{\mathrm{lab}}>0$, les cibles vérifient $\pi_p=\sum_{v:n_v^{\mathrm{lab}}>0}\frac{n_v^{\mathrm{lab}}}{n_p^{\mathrm{lab}}}\pi_v$, tandis que les agrégats prédits utilisent les cardinalités géométriques $n_v/n_p$. Pour des $K$-polyèdres chevauchants, une moyenne naïve double-compte les points. Jusqu'à spécification et validation des poids $w_{iv}$ ci-dessus, les losses massiques emploient seulement $K=1$ ou une laminarisation auditée ; l'expression « partition de l'unité » ne vaut pas preuve de conservation. Si une feuille représente plusieurs points, les agrégats emploient leurs sorties point-wise originales, ou pondèrent explicitement sa distribution par sa masse.

Une distribution de proportions préserve la masse de chaque classe dans un cluster, mais pas la localisation des classes à l'intérieur. La sortie principale reste donc point-wise et conditionnée par les features de feuille ; aucun broadcast uniforme du vecteur de nœud n'est utilisé comme prédiction finale. En particulier, comparer seulement $\pi_v$ à la moyenne des $p_i$ est invariant à une permutation des prédictions entre points et ne remplace pas la supervision point-wise.

Conserver aussi la cardinalité et deux incertitudes distinctes : l'entropie moyenne $n_v^{-1}\sum_iH(p_i)$ mesure l'incertitude des feuilles, tandis que $H(\widehat\pi_v^{\mathrm{all}})-n_v^{-1}\sum_iH(p_i)$ mesure leur désaccord. Une même proportion `50/50` n'a ainsi pas le même état si toutes les feuilles sont incertaines ou si deux sous-populations sont chacune confiantes.

Le triplet `proportions + entropie moyenne + désaccord`, accompagné de $\log n_v$, est projeté comme **contenu sémantique** dans la passe top-down et le décodeur. Dans la variante HSA fidèle, il ne remplace pas l'embedding positionnel $\epsilon_p(v)$. L'ablation obligatoire compare géométrie seule, proportions seules et combinaison des deux, toujours à partir de proportions prédites.

## Descripteur des nœuds

### Canal de support

Pour un nœud non dégénéré $v$, $s_v(u)=\max_{x\in C_v}\left\langle u,\frac{x-c_v}{R_v}\right\rangle$ est échantillonné sur une grille déterministe de la sphère. Comparer séparément des grilles Fibonacci de cardinalité exacte 20/42/80/162 et des grilles de sommets d'icosphères de cardinalité réelle 12/42/162/642. Chaque direction est normalisée ; si des largeurs sont utilisées, chaque paire $u,-u$ est explicitement présente. La construction, la cardinalité après ajout des antipodes et le rayon de couverture mesuré sont enregistrés. La sélection finale dépend d'une courbe erreur–mémoire–latence.

Ce canal porte par défaut sur le $K$-polyèdre source $C_v$, donc aussi sur le carrier PL des facettes, et non sur $W_v(a)$. Un éventuel support de l'union témoin reçoit un nom et un kernel distincts. Le support maximal est stable vis-à-vis de petites perturbations de l'enveloppe convexe au sens de Hausdorff, mais statistiquement fragile à un point aberrant. Il ne reçoit des gradients que par les points extrêmes des directions. Il reste donc un canal parmi plusieurs.

### Canal marqué et incidence-aware

Le second canal est $\mathfrak{P}_{p\leftarrow v}$ lui-même, pas une fonction scalaire calculée sur une de ses réalisations. `payload_kind=marked_incidence` désigne les données finies ; selon `carrier_kind`, le carrier reconstruit peut être non convexe ou troué. Deux payloads ayant les mêmes sommets, le même support et les mêmes statistiques marginales de cellules peuvent rester distincts par $\partial_v$, $\beta_v$, $\mu_v$ ou par leur coupe.

La voie implémentable commence par un encodeur d'incidences sparse de profondeur faible :

1. initialiser chaque sommet par sa feature de backbone, sa coordonnée normalisée et ses canaux métriques non normalisés ;
2. initialiser chaque facette et coface élémentaire par une agrégation symétrique de ses sommets, son barycentre, sa matrice de Gram, son contenu dimensionnel, son niveau $\beta$ et sa multiplicité déclarée ; pour `carrier_kind=witness_union`, ajouter les paramètres normalisés de l'intersection de boules à `cut_level` au lieu de la confondre avec $\mathrm{conv}(F)$ ;
3. effectuer deux à quatre passages sommet–facette–coface puis coface–facette–sommet le long de $\partial_v$, avec MLP partagés par dimension et résidu ;
4. lire un état de nœud par somme/attention massique sur les cellules, puis construire les embeddings parent–enfant utilisés par HSA ;
5. redescendre le contexte jusqu'aux sommets et le fusionner avec le skip point-wise.

Le prototype de référence est un **hypergraphe typé** point–facette–coface élémentaire : il utilise des incidences non signées et des agrégations invariantes à l'ordre canonique des sommets. Il ne s'agit pas d'un complexe de cochaînes, car les rangs intermédiaires peuvent manquer. Une variante Hodge/cochaîne exige d'exporter tous les rangs de $0$ à $K$, de définir chaque matrice de bord et de vérifier $\partial_{r-1}\partial_r=0$ avant toute propriété d'orientation. Le payload reste disponible pendant le calcul ; le readout de dimension fixe n'est pas déclaré injectif sans théorème. Cette architecture est une adaptation directe des réseaux de complexes à l'objet HGP : sa nouveauté éventuelle doit venir du marquage HGP, de son couplage multi-échelle ou d'une garantie nouvelle, pas du seul message passing d'incidence.

Le support source est informationnellement redondant conditionnellement au complexe complet, puisque $V_v$ appartient déjà au contrat et suffit à le recalculer. Il est néanmoins conservé comme **shortcut d'optimisation** peu coûteux vers les extrêmes globaux. La fusion recommandée concatène l'état incidence-aware, le support normalisé et les side channels métriques, puis applique un gate résiduel. Les ablations `complexe seul` et `support + complexe` déterminent si ce raccourci aide l'apprentissage ; un gain du second ne prouve pas qu'il contient davantage d'information sur l'objet.

La mise en œuvre ne reconstruit jamais un Čech ou Delaunay global dans le réseau. Elle consomme les seuls enregistrements sparsifiés et certifiés par le contrat. Les cellules sont stockées une fois par identifiant canonique et les nœuds HGP référencent leurs occurrences ou deltas de filtration. En notant $N_V$, $N_F$, $N_Q$, $N_I$ et $N_A$ les nombres uniques de sommets, facettes, cofaces élémentaires, incidences et appartenances cellule–nœud, une couche sparse de largeur $d$ vise un coût $\mathcal{O}\left(d(N_V+N_F+N_Q+N_I+N_A)\right)$ et un stockage du même ordre, hors projections quadratiques en $d$. Il faut mesurer ces cinq compteurs : répéter matériellement chaque sous-complexe à tous ses ancêtres peut rendre $N_A$ dominant et annuler tout intérêt système. Pour `carrier_kind=witness_union`, rapporter aussi $N_W$, nombre de requêtes exactes ou éléments d'approximation effectivement évalués, et $\varepsilon_W$, écart au carrier ou à l'oracle borné selon la métrique versionnée ; ces coûts restent hors de la formule sparse précédente.

Pour $K\geq2$, le stockage d'incidences peut rester un DAG global partagé même lorsque les carriers se recouvrent, mais sa projection vers HSA reste une obligation ouverte. La baseline n'utilise cette voie qu'après définition explicite de $w_{iv}$, de son domaine et des masses induites, puis validation de conservation et de non-double comptage ; jusque-là elle reste à $K=1$ ou à une laminarisation documentée. La voie théorique ambitieuse remplace cette projection par une attention directement définie sur le DAG de recouvrement et doit prouver conservation de masse, stochasticité, équivariance aux réindexations et réduction exacte au cas laminaire. Cette extension n'hérite pas automatiquement du théorème HSA.

### Contrôle du raccourci de support

Pour toute direction $u$, $h_{C_v^{F}}(u)=\max_{F\in\mathcal{F}_v}\max_{x\in F}\langle u,x\rangle=h_{C_v}(u)$. Pour un carrier des cofaces non vide, l'identité exacte est $h_{C_v^{Q}}(u)=\max_{Q\in\mathcal{Q}_v^{\mathrm{elem}}}\max_{x\in Q}\langle u,x\rangle$ ; elle égale $h_{C_v}(u)$ lorsque les cofaces conservées couvrent $V_v$, mais pas pour une facette isolée omise. Le profil encode séparément le cas vide au lieu de lui attribuer un support fini. Ces tests ne réduisent pas $\mathfrak{P}_{p\leftarrow v}$ à son support. Ils ne s'étendent pas à l'union témoin : $h_{W_v(a)}(u)=\max_{F\in\mathcal{F}_v}h_{\bigcap_{x\in F}\overline{B}(x,\sqrt{a})}(u)$ n'est en général pas égal à $h_{C_v}(u)$.

### Ablation radiale lossy

Pour un carrier déclaré $A_v\in\left\lbrace C_v^{F},C_v^{Q},W_v(a)\right\rbrace$ et un centre $c_v$, son rayon extérieur est $\rho_{A_v,c_v}(u)=\sup\left(\left\lbrace r\geq0:c_v+ru\in A_v\right\rbrace\cup\left\lbrace0\right\rbrace\right)$. Ce n'est pas une fonction support. Si $c_v\in A_v$, la fonction continue reconstruit exactement $A_v$ si et seulement si celui-ci est étoilé autour de $c_v$ ; tout échantillonnage fini reste un sketch. Le prototype stocke le carrier choisi, `center_in_realization`, `center_in_kernel` et un masque directionnel `ray_hit`, car $\rho=0$ ne distingue pas un rayon vide d'une intersection réduite au centre ; il mesure aussi le nombre de composantes d'intersection par rayon, points isolés inclus.

Une variante multi-segments encode les extrémités ou une occupation binaire le long du rayon. Pour une réalisation de dimension intrinsèque inférieure à trois, préférer des cônes angulaires ou un épaississement explicite et ablater leur bande passante : un rayon exact générique peut manquer une arête ou une surface. Le cube plein et sa frontière, de mêmes support et rayon extérieur depuis le centre, forment une fixture permanente. Ces variantes sont des ablations compressées face au complexe complet, jamais son substitut dans l'hypothèse principale.

### Contrôle topologique ECT/WECT

Une ECT à directions et seuils finis, éventuellement augmentée par masse, rémission ou niveau HGP, sert de contrôle pour les incidences et trous. Elle n'est pas annoncée comme nouvelle : injectivité de la transformée complète, variantes pondérées et versions différentiables sont déjà publiées. Les garanties WECT citées ne sont transférées à aucun poids réel de rémission ou niveau HGP sans vérifier leurs hypothèses d'admissibilité. Si les simplexes HGP se recouvrent sans former un complexe géométrique conforme, distinguer l'ECT du complexe abstrait de celle de l'union plongée et ne transférer aucun théorème entre les deux sans preuve.

### Canal robuste directionnel

Pour chaque direction, calculer une petite pile de quantiles des projections normalisées $\left\langle u,\frac{x-c_v}{R_v}\right\rangle$, par exemple `q50`, `q90`, `q95`, `q99` et `max`. Les quantiles ne sont pas des fonctions support convexes au sens strict ; ils sont utilisés comme features de distribution. Une variante log-mean-exp multi-température doit être testée pour distribuer les gradients, sans la présenter comme robuste aux outliers.

### Canal de masse projetée, ajout prioritaire

La correction la plus directe au support consiste à conserver la **distribution** de chaque projection, pas seulement son maximum. Pour $R_v>0$ et des seuils fixes $t_b$, définir $F_v(u_j,t_b)=n_v^{-1}\sum_{x\in C_v}\mathbf{1}\left\lbrace\left\langle u_j,(x-c_v)/R_v\right\rangle\leq t_b\right\rbrace$. Ce tenseur direction × seuil a une dimension fixe et peut distinguer certains clusters ayant la même enveloppe mais des masses intérieures différentes. Avec toutes les directions et toute la CDF projetée, la mesure normalisée est déterminée par le théorème de Cramér–Wold ; avec une grille finie, il s'agit seulement d'un sketch à auditer. Le cas $R_v=0$ est la masse de Dirac dégénérée traitée par le masque ci-dessous.

Pour le prototype, préférer des histogrammes/CDF à bins fixes, éventuellement complétés par quelques quantiles robustes et par le max exact. Les comptes d'histogramme sont additifs dans un repère commun, contrairement à une liste de quantiles qui ne se fusionne pas exactement. Une normalisation indépendante par nœud exige cependant de transporter le déplacement et le ratio d'échelle, puis de rééchantillonner les bins ; sinon le canal est calculé directement depuis les points lors du prétraitement.

L'hypothèse principale à tester est donc `support normalisé + objet HGP marqué complet + side channels métriques`. Elle est comparée à `support seul`, `objet marqué seul`, un multiensemble de cellules sans incidences, $\Gamma_K^{\mathrm{elem}}$ seul, $\Gamma_K^{\mathrm{full}}$ lorsqu'il est effectivement disponible, et un encodeur Deep Sets de même budget. Rayon extérieur, multi-segments, CDF, ECT/WECT et moments restent des ablations compressées ou des contrôles. Le support décrit directement les extrêmes ; le payload porte les incidences et les niveaux HGP, tandis que le `carrier_kind` déclaré détermine la géométrie, potentiellement non convexe.

### Convention dégénérée

Si $R_v=0$, fixer supports et quantiles normalisés à zéro et activer un masque `degenerate`. Le canal de taille utilise $\log\left(\max\left(R_v,\varepsilon_{\mathrm{metric}}\right)\right)$ accompagné du même masque. Le plancher métrique est fixé dans la configuration ; il sert à la stabilité numérique et rompt l'invariance d'échelle exacte du cas dégénéré. Pour un parent de rayon nul, le déplacement et le ratio d'échelle sont fixés à zéro et masqués.

### Canaux non normalisés

Le modèle reçoit séparément :

- $\log\left(\max\left(R_v,\varepsilon_{\mathrm{metric}}\right)\right)$, le masque dégénéré et les dimensions métriques du nœud ;
- `log(1 + n_v)`, masse et estimation de densité ;
- centre en coordonnées cartésiennes et `(range, azimuth, elevation)` ;
- hauteur minimale, maximale et moyenne ;
- valeurs propres normalisées de covariance et dimension intrinsèque estimée ;
- moyenne, dispersion et quantiles de rémission ;
- naissance, mort, persistance/stabilité et ordre HGP $K$ ;
- profondeur, fraction de masse dans le parent et rapports de masse entre enfants ;
- drapeaux singleton, rayon nul, faible dimension et troncature/condensation.

Le dataset standard ne fournit pas explicitement l'identifiant d'anneau laser. Aucun canal `ring` ne doit être inventé ; un indice reconstruit depuis l'élévation doit être déclaré comme feature dérivée.

### Géométrie relative parent–enfant

Une normalisation indépendante rend deux formes semblables identiques même si leur position dans le parent diffère. Pour une arête $p\rightarrow v$, l'embedding doit au minimum inclure $\Delta c_{pv}=\frac{c_v-c_p}{R_p}$ et $r_{pv}=\frac{R_v}{R_p}$, avec la convention masquée ci-dessus pour $R_p=0$. Sur une grille de directions globales commune, l'identité exacte à tester est $s_p(u)=\max_{v\in\mathrm{children}(p)}\left[\left\langle u,\frac{c_v-c_p}{R_p}\right\rangle+\frac{R_v}{R_p}s_v(u)\right]$. Si un repère local tourné est utilisé, sa rotation et le rééchantillonnage des directions deviennent aussi contractuels.

S'ajoutent la différence de niveau HGP, le ratio de cardinalité et l'orientation principale relative. Un MLP partagé par domaine du parent produit alors **un embedding $\epsilon_p(v)$ par enfant**. Dans la variante fidèle, l'énergie positionnelle entre frères $v,w$ utilise le produit scalaire $\epsilon_p(v)^{\top}\epsilon_p(w)$ attendu par HSA. Un MLP pairwise arbitraire ou l'injection d'un état appris aux nœuds internes définit une variante hors théorème. Un attribut de recouvrement n'est permis que dans une future variante DAG/multi-appartenance, pas dans la forêt laminaire HSA.

## Bloc hiérarchique de référence

### Passe bottom-up

Chaque nœud agrège les statistiques suffisantes Q/K/V de ses feuilles descendantes et déduit son agrégat $\widehat\pi_v^{\mathrm{all}}$. Son descripteur géométrique produit les embeddings par enfant décrits ci-dessus ; ni cet agrégat sémantique ni le descripteur ne le transforme silencieusement en token Q/K/V interne. Les réductions sont groupées par profondeur et concaténées entre scans d'un batch.

La concaténation entre scans est **block-diagonal et masquée**. Un éventuel dummy root ne sert qu'à l'ordonnancement des kernels et sa famille est exclue du calcul d'attention : aucun scan ne peut influencer un autre. Une forêt à l'intérieur d'un scan reçoit un root synthétique propre au scan ; l'interaction entre ses composantes racines est un choix de modèle explicite et ablaté. Test obligatoire : la sortie d'un scan reste bit-identique, ou égale dans la tolérance numérique déclarée, quels que soient les autres scans du batch.

### Interaction entre enfants

Pour chaque famille, HSA calcule une interaction entre sous-arbres frères avec des coefficients partagés par blocs. Le coût structurel de l'algorithme et de l'énergie HSA spécifiques est mieux décrit par $\mathcal{O}\left(\sum_{v} d_v^{2}\right)$, où $d_v$ est le nombre d'enfants, que par le seul slogan linéaire. La borne $\mathcal{O}(M b^{2})$ n'est favorable que si le degré maximal $b$ reste borné et si le nombre de familles $M$ est linéaire. Ces expressions omettent dimension des têtes, projections, descripteurs, transferts et batching ; elles ne s'étendent pas automatiquement aux scores pairwise libres.

La reproduction fidèle ré-établit les équations depuis la définition de la projection au lieu de recopier le pseudo-code : signes de normalisation, indices positionnels et facteurs de cardinalité font l'objet de mutants. Le théorème HSA ne couvre que les poids sous Q/K post-LayerNormés, énergie, température, rescaling et masque exacts ; il ne couvre pas automatiquement V, le gate, le MLP ni la qualité de l'arbre.

Les nœuds de degré élevé, les chaînes profondes et les arbres déséquilibrés sont mesurés. La binarisation ou le rééquilibrage ne sont pas gratuits : ils introduisent une structure artificielle et doivent être ablatés contre l'arbre natif.

### Opérateur expérimental : QC-HSA

HSA partage un coefficient sur tout rectangle entre deux branches sœurs, donc aussi entre plusieurs feuilles requêtes de la même branche. `QC-HSA` conserve au contraire chaque feuille requête. Pour une feuille $i$, elle agrège les clés/valeurs des sous-arbres frères rencontrés sur le chemin vers la racine et calcule un poids propre à $i$ pour chacun de ces groupes.

Les clés et valeurs moyennes des nœuds sont construites bottom-up. Pour un score bilinéaire et un biais HGP constant dans le groupe cible, chaque sortie s'obtient sans matrice dense par un Softmax sur score moyen + log-cardinalité. La [proposition candidate](THEOREM_PROGRAM.md) en fait la projection reverse-KL optimale sur ces contraintes conditionnées par la feuille, montre que sa famille contient celle de HSA et donne l'exactitude des scores constants sur chaque couple de branches de fusion.

Son coût structurel est $C_T=\sum_i|\Pi_T(i)|$, soit $\mathcal{O}(N\log N)$ sur un arbre équilibré de degré borné mais $\mathcal{O}(N^2)$ au pire. Le prototype doit donc rapporter simultanément $C_T$, $\sum_v d_v^2$, la profondeur, le degré et la latence. `QC-HSA` n'est retenue que si sa fidélité point-wise compense ce surcoût face à HSA.

### Passe top-down

Le contexte agrégé redescend vers chaque feuille. Pour la segmentation, la sortie hiérarchique $g_i$ est fusionnée avec la feature locale par un gate résiduel : $z_i=f_i^{0}+\alpha_i\,W_g g_i$, avec $\alpha_i\in[0,1]$ prédit à partir des features locales et hiérarchiques.

Le gate sert de voie de secours lorsque l'arbre traverse une frontière sémantique. Une variante concaténation + MLP est un contrôle nécessaire ; le gate ne doit pas devenir une source de gain non isolée.

## Placement des blocs

La configuration initiale contient :

- un encodeur local bas niveau ;
- un premier bloc HGP aux features intermédiaires ;
- éventuellement un second bloc HGP à plus grande portée ;
- un décodeur local haute résolution ;
- une tête sémantique point-wise.

Tester zéro, un, deux et quatre blocs, ainsi que leur placement tôt/tard. La recommandation initiale est tardive : la géométrie locale doit être apprise avant d'imposer les blocs d'attention hiérarchiques.

## Frontières et adjacency spatiale

Les interactions HSA entre branches sont contraintes par l'arbre. Une erreur de branche lie alors tous les couples concernés. Trois mécanismes de correction sont autorisés, chacun ablaté :

- skip local du backbone ;
- petites arêtes $k_{\mathrm{local}}$-NN ou de frontière entre feuilles de branches voisines ;
- une fraction de têtes locales non hiérarchiques.

Le modèle final ne doit pas cacher un Transformer plat complet sous le nom HSA. Le budget et la portée des voies de correction seront rapportés.

## Sortie et pertes sémantiques

La tête produit 19 logits pour chaque point original. Le chemin feuille–point doit conserver l'ordre exact des points du fichier `.bin`.

Le protocole minimal réutilise l'objectif point-wise exact de la baseline reproduite. Les ajouts possibles, testés seulement ensuite, sont :

- loss propre sur les proportions de tout nœud supervisé, par exemple cross-entropy molle ou $D_{\mathrm{KL}}\left(\pi_v\,\Vert\,\widehat\pi_v^{\mathrm{lab}}\right)$ ;
- cohérence massique parent–enfants entre proportions prédites ;
- loss de frontière ;
- calibration ou pondération des classes rares.

Ces pertes ne sont ouvertes qu'après preuve de l'effet de l'arbre avec une recette identique. Leur pondération par profondeur/taille doit empêcher qu'un point soit surcompté une fois par ancêtre. Une supervision majoritaire forcée est exclue : les nœuds mixtes sont supervisés par leurs proportions exactes.

Pour l'ablation artificielle où une même distribution $p_v$ est diffusée à tous les points valides du cluster, l'identité exacte est $\sum_{i\in C_v^{\mathrm{lab}}}\mathrm{CE}(e_{y_i},p_v)=n_v^{\mathrm{lab}}\mathrm{CE}(\pi_v,p_v)=n_v^{\mathrm{lab}}\left[H(\pi_v)+D_{\mathrm{KL}}\left(\pi_v\,\Vert\,p_v\right)\right]$. Le coût irréductible $n_v^{\mathrm{lab}}H(\pi_v)$ mesure le mélange du cluster, mais ne concerne pas le décodeur point-wise proposé. Sommer cette loss sur tous les ancêtres répète chaque label ; c'est un régularisateur assumé, sauf si des poids d'incidence $\alpha_{iv}$ vérifient $\sum_{v:i\in C_v}\alpha_{iv}=1$.

## Baselines architecturales appariées

Avec exactement le même backbone et le même budget de dimension :

1. aucun module global ;
2. pooling global ou par voxel ;
3. bottom-up/top-down par `mean/max + MLP` ;
4. message passing parent–enfant/frères ;
5. HSA avec arbre aléatoire contrôlé ;
6. HSA avec octree/voxel tree ;
7. HSA avec HGP $K=1$, qui doit être identique au single-linkage et sert de fixture de cohérence ;
8. HSA avec HGP $K=2,3$, RSL/HDBSCAN et les autres arbres de contrôle ;
9. `QC-HSA` avec HGP, puis avec les arbres de contrôle retenus ;
10. attention plate seulement sur sous-échantillon, comme contrôle de qualité et de coût.

Cette matrice empêche d'attribuer à HGP un gain provenant seulement d'un chemin global supplémentaire.

## Ordre HGP et multi-hiérarchie

Commencer par $K=1,2,3$. Le cas $K=1$ doit reproduire exactement le single-linkage et devient une fixture permanente ; il n'est pas compté deux fois dans les comparaisons. L'étude HGP existante sur SemanticKITTI, faite avec masques sémantiques de vérité terrain et pour le regroupement d'instances, observait $K=2$ supérieur à $K=1$ et $K=3$ ; ce résultat ne prouve rien pour la classification sémantique, mais justifie $K=2$ comme candidat initial plutôt qu'une croissance aveugle de $K$.

Une fusion multi-$K$ ou plusieurs arbres par tête ne sera testée qu'après établissement d'un gain pour un $K$ unique. Sinon, elle multiplierait les hypothèses et le coût sans identifier la source du résultat.

## Interface conservée pour l'instance, sans tête active

Le modèle sauvegarde :

- logits et features finales par point ;
- mapping stable point–feuille–ancêtres ;
- topologie et attributs HGP ;
- distributions de proportions prédites $\widehat\pi_v^{\mathrm{all}}$ par nœud, avec pureté, entropie et incertitude dérivées.

Cela permettra plus tard de comparer ALPINE, une coupe HGP fixe et une coupe HGP apprise avec les mêmes prédictions sémantiques. Aucun objectif PQ n'entre dans la décision de la phase sémantique.
