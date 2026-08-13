# Architecture proposée

## Principe

L'architecture de départ est hybride : une représentation locale forte protège les détails de surface et les frontières, tandis que la hiérarchie HGP apporte du contexte à plus grande échelle. Cette décision évite de demander à la fonction support de reconstruire toute l'information du nuage et suit le signal négatif du papier HSA : remplacer indiscriminément toutes les couches basses d'un modèle plat peut dégrader fortement les résultats.

Nom de travail : **HGP-Hybrid Transformer**. Ce nom n'est pas un claim de nouveauté définitif.

## Contrat d'entrée de la hiérarchie

On suppose que chaque scan fournit :

- les points dans l'ordre original, avec `xyz` et rémission ;
- une forêt enracinée laminaire ;
- `parent`, `first_child`/`children`, profondeur et ordre topologique ;
- le mapping point–feuille et, si possible, des intervalles contigus de feuilles par nœud ;
- si une géométrie $K$-simpliciale est utilisée, la liste canonique des simplexes actifs, leur orientation/convention, leurs niveaux de filtration et leur mapping vers les nœuds ;
- cardinalité, centre, rayon et statistiques HGP par nœud ;
- niveau de naissance, niveau de mort et persistance/stabilité lorsqu'ils sont définis ;
- l'ordre HGP $K$, la distance géométrique $d_{\mathrm{geo}}$, les paramètres de reconstruction et la version du reconstructeur ;
- un identifiant déterministe du scan et de la hiérarchie.

Les labels SemanticKITTI ne participent jamais à la construction. Une hiérarchie par classe prédite serait une expérience aval différente et n'est pas admise dans l'encodeur sémantique principal.

HSA a besoin d'un arbre. Si les objets HGP d'ordre supérieur ont des appartenances ponctuelles chevauchantes, le producteur doit fournir soit une projection laminaire auditée, soit une structure DAG explicitement prise en charge. Dupliquer silencieusement les points ou arbitrer l'appartenance par ordre d'itération rendrait le résultat non interprétable. Un ensemble de sommets HGP ne doit pas être présenté comme une réalisation géométrique complète si les simplexes et niveaux correspondants n'ont pas été sérialisés.

## Représentation des feuilles

Deux granularités seront comparées avec, dans les deux cas, une sortie finale par point :

1. **points comme feuilles**, solution de référence scientifique qui conserve directement la localisation point-wise ;
2. **micro-voxels ou micro-clusters comme feuilles**, seulement si les diagnostics de composition et le décodeur point-wise valident cette réduction ; la baseline majoritaire dure n'est pas à elle seule une porte sur cette granularité.

Le premier prototype doit partir d'une recette SemanticKITTI réellement reproductible et épinglable, par exemple MinkUNet/Cylinder3D ou une recette publique SphereFormer auditée. PTv3 reste le porteur Transformer ambitieux, mais son dépôt officiel ne fournit pas à ce jour une recette SemanticKITTI complète avec config, poids et résultat reproductible ; il ne doit donc pas bloquer WP0. Un second backbone fort devra vérifier que l'effet HGP n'est pas propre au porteur choisi. Le backbone reçoit exactement les mêmes entrées et la même recette dans toutes les comparaisons appariées.

Pour chaque feuille $i$, le backbone produit $f_i^{0}\in\mathbb{R}^{d}$. Les projections Q/K/V et les normalisations suivent la définition de la baseline HSA testée. Les coordonnées absolues ou cylindriques ne sont pas supprimées : le repère ego et la gravité sont sémantiquement utiles.

## État sémantique multiscale

Chaque nœud $v$ porte une **distribution de proportions sémantiques**, jamais un label dur. Soient $V_v$ ses points dont le label n'est pas ignoré, $n_v^{\mathrm{lab}}=|V_v|$ et $n_{v,c}=\sum_{i\in V_v}\mathbf{1}\left\lbrace y_i=c\right\rbrace$. Sa cible est $\pi_v(c)=n_{v,c}/n_v^{\mathrm{lab}}$ pour $c\in\left\lbrace1,\ldots,19\right\rbrace$. Une feuille point valide a donc une cible one-hot, tandis qu'un cluster traversant une frontière conserve explicitement son mélange de classes. Un nœud sans label valide est masqué pour cette supervision.

À l'inférence, aucune tête indépendante n'est nécessaire dans l'architecture minimale. Les distributions point-wise sont agrégées sans label par $\widehat\pi_v^{\mathrm{all}}=n_v^{-1}\sum_{i\in C_v}p_i$, donc $\widehat\pi_p^{\mathrm{all}}=\sum_{v\in\mathrm{children}(p)}\frac{n_v}{n_p}\widehat\pi_v^{\mathrm{all}}$. Pour la loss seulement, $\widehat\pi_v^{\mathrm{lab}}=(n_v^{\mathrm{lab}})^{-1}\sum_{i\in V_v}p_i$ est comparé à $\pi_v$ ; ce masque GT n'entre jamais dans le forward de validation ou de test. Le backbone local fournit les $p_i^{(0)}$ initiaux ; après chaque bloc, les agrégats peuvent être recalculés. Un readout appris depuis l'état du nœud n'est qu'une ablation auxiliaire. Cet agrégat ne transforme pas silencieusement le nœud en token HSA.

Lorsque les enfants forment une partition laminaire exacte du parent et $n_p^{\mathrm{lab}}>0$, les cibles vérifient $\pi_p=\sum_{v:n_v^{\mathrm{lab}}>0}\frac{n_v^{\mathrm{lab}}}{n_p^{\mathrm{lab}}}\pi_v$, tandis que les agrégats prédits utilisent les cardinalités géométriques $n_v/n_p$. Pour des $K$-polyèdres chevauchants, une moyenne naïve double-compte les points : la laminarisation ou des poids d'incidence formant une partition de l'unité doivent être définis avant toute loss. Si une feuille représente plusieurs points, les agrégats emploient leurs sorties point-wise originales, ou pondèrent explicitement sa distribution par sa masse.

Une distribution de proportions préserve la masse de chaque classe dans un cluster, mais pas la localisation des classes à l'intérieur. La sortie principale reste donc point-wise et conditionnée par les features de feuille ; aucun broadcast uniforme du vecteur de nœud n'est utilisé comme prédiction finale. En particulier, comparer seulement $\pi_v$ à la moyenne des $p_i$ est invariant à une permutation des prédictions entre points et ne remplace pas la supervision point-wise.

Conserver aussi la cardinalité et deux incertitudes distinctes : l'entropie moyenne $n_v^{-1}\sum_iH(p_i)$ mesure l'incertitude des feuilles, tandis que $H(\widehat\pi_v^{\mathrm{all}})-n_v^{-1}\sum_iH(p_i)$ mesure leur désaccord. Une même proportion `50/50` n'a ainsi pas le même état si toutes les feuilles sont incertaines ou si deux sous-populations sont chacune confiantes.

Le triplet `proportions + entropie moyenne + désaccord`, accompagné de $\log n_v$, est projeté comme **contenu sémantique** dans la passe top-down et le décodeur. Dans la variante HSA fidèle, il ne remplace pas l'embedding positionnel $\epsilon_p(v)$. L'ablation obligatoire compare géométrie seule, proportions seules et combinaison des deux, toujours à partir de proportions prédites.

## Descripteur des nœuds

### Canal de support

Pour un nœud non dégénéré $v$, $s_v(u)=\max_{x\in C_v}\left\langle u,\frac{x-c_v}{R_v}\right\rangle$ est échantillonné sur une grille déterministe de la sphère. Comparer séparément des grilles Fibonacci de cardinalité exacte 20/42/80/162 et des grilles de sommets d'icosphères de cardinalité réelle 12/42/162/642. Chaque direction est normalisée ; si des largeurs sont utilisées, chaque paire $u,-u$ est explicitement présente. La construction, la cardinalité après ajout des antipodes et le rayon de couverture mesuré sont enregistrés. La sélection finale dépend d'une courbe erreur–mémoire–latence.

Le support maximal est stable vis-à-vis de petites perturbations de l'enveloppe convexe au sens de Hausdorff, mais statistiquement fragile à un point aberrant. Il ne reçoit des gradients que par les points extrêmes des directions. Il reste donc un canal parmi plusieurs.

### Audit de la réalisation géométrique du K-polyèdre

Soit $|P_v|=\bigcup_{\sigma\in\mathcal{C}_v}\mathrm{conv}(\sigma)$ la réalisation géométrique de la composante de simplexes HGP et $X_v$ l'union de ses sommets. Pour toute direction $u$, $h_{|P_v|}(u)=\max_{\sigma\in\mathcal{C}_v}\max_{x\in\mathrm{conv}(\sigma)}\langle u,x\rangle=\max_{x\in X_v}\langle u,x\rangle=h_{X_v}(u)$. Le maximum d'une forme linéaire sur un simplexe étant atteint sur un sommet, ce descripteur est **exactement identique** au support des sommets sérialisés. Il coïncide avec celui du nuage du cluster seulement si ce nuage est exactement $X_v$. Il est calculable par `max` et très HGP-friendly, mais il ne voit ni l'ordre $K$, ni les incidences, ni le nombre ou la forme des simplexes, ni les recouvrements, ni les niveaux de filtration. Il ne peut donc pas être le descripteur suffisant revendiqué.

La variante qui exploite réellement HGP doit agréger une **mesure sur les simplexes** : centres, volumes ou aires, spectres de longueurs, niveaux de naissance $\beta(\sigma)$ et multiplicités d'incidence. Des CDF projetées ou moments de ces attributs sont fusionnables par sommes/comptes et distinguent certaines réalisations ayant les mêmes sommets extrêmes. Le support de la réalisation reste un canal d'extrêmes et le contrôle `support des sommets`, auquel il doit être numériquement identique. Distinguer la mesure de l'union géométrique, sans multiplicité, de la mesure sur enregistrements simpliciaux, où les multiplicités sont intentionnelles. Pour $K\geq2$, les recouvrements doivent être résolus ou pondérés avant d'interpréter une somme comme masse géométrique.

### Canal radial et intersections de rayons

Pour un centre $c_v$, le rayon extérieur de la réalisation est $\rho_{v}(u)=\sup\left(\left\lbrace r\geq0:c_v+ru\in|P_v|\right\rbrace\cup\left\lbrace0\right\rbrace\right)$. Ce n'est pas une fonction support. Si $c_v\in|P_v|$, la fonction continue reconstruit exactement la réalisation si et seulement si celle-ci est étoilée autour de $c_v$ ; tout échantillonnage fini reste un sketch. Le noyau étoilé est l'ensemble des centres $c\in|P_v|$ tels que le segment $[c,x]$ soit inclus dans $|P_v|$ pour tout $x\in|P_v|$. Le prototype stocke `center_in_realization`, `center_in_kernel` et un masque directionnel `ray_hit`, car $\rho=0$ ne distingue pas un rayon vide d'une intersection réduite au centre ; il mesure aussi le nombre de composantes d'intersection par rayon, points isolés inclus.

Une variante multi-segments encode les extrémités ou une occupation binaire le long du rayon. Pour une réalisation de dimension intrinsèque inférieure à trois, préférer des cônes angulaires ou un épaississement explicite et ablater leur bande passante : un rayon exact générique peut manquer une arête ou une surface. Le cube plein et sa frontière, de mêmes support et rayon extérieur depuis le centre, forment une fixture permanente.

### Contrôle topologique ECT/WECT

Une ECT à directions et seuils finis, éventuellement augmentée par masse, rémission ou niveau HGP, sert de contrôle pour les incidences et trous. Elle n'est pas annoncée comme nouvelle : injectivité de la transformée complète, variantes pondérées et versions différentiables sont déjà publiées. Les garanties WECT citées ne sont transférées à aucun poids réel de rémission ou niveau HGP sans vérifier leurs hypothèses d'admissibilité. Si les simplexes HGP se recouvrent sans former un complexe géométrique conforme, distinguer l'ECT du complexe abstrait de celle de l'union plongée et ne transférer aucun théorème entre les deux sans preuve.

### Canal robuste directionnel

Pour chaque direction, calculer une petite pile de quantiles des projections normalisées $\left\langle u,\frac{x-c_v}{R_v}\right\rangle$, par exemple `q50`, `q90`, `q95`, `q99` et `max`. Les quantiles ne sont pas des fonctions support convexes au sens strict ; ils sont utilisés comme features de distribution. Une variante log-mean-exp multi-température doit être testée pour distribuer les gradients, sans la présenter comme robuste aux outliers.

### Canal de masse projetée, ajout prioritaire

La correction la plus directe au support consiste à conserver la **distribution** de chaque projection, pas seulement son maximum. Pour $R_v>0$ et des seuils fixes $t_b$, définir $F_v(u_j,t_b)=n_v^{-1}\sum_{x\in C_v}\mathbf{1}\left\lbrace\left\langle u_j,(x-c_v)/R_v\right\rangle\leq t_b\right\rbrace$. Ce tenseur direction × seuil a une dimension fixe et peut distinguer certains clusters ayant la même enveloppe mais des masses intérieures différentes. Avec toutes les directions et toute la CDF projetée, la mesure normalisée est déterminée par le théorème de Cramér–Wold ; avec une grille finie, il s'agit seulement d'un sketch à auditer. Le cas $R_v=0$ est la masse de Dirac dégénérée traitée par le masque ci-dessous.

Pour le prototype, préférer des histogrammes/CDF à bins fixes, éventuellement complétés par quelques quantiles robustes et par le max exact. Les comptes d'histogramme sont additifs dans un repère commun, contrairement à une liste de quantiles qui ne se fusionne pas exactement. Une normalisation indépendante par nœud exige cependant de transporter le déplacement et le ratio d'échelle, puis de rééchantillonner les bins ; sinon le canal est calculé directement depuis les points lors du prétraitement.

Le premier descripteur à tester est donc : moments/covariance + attributs HGP + side channels métriques, puis support maximal + CDF projetées. Rayon extérieur, multi-segments, mesure simpliciale et ECT/WECT sont ajoutés une variable à la fois et comparés à Deep Sets de même budget. Les CDF décrivent la masse, le support les extrêmes, et les canaux HGP la position du nœud dans la filtration de densité.

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

Pour l'ablation artificielle où une même distribution $p_v$ est diffusée à tous les points valides du cluster, l'identité exacte est $\sum_{i\in V_v}\mathrm{CE}(e_{y_i},p_v)=n_v^{\mathrm{lab}}\mathrm{CE}(\pi_v,p_v)=n_v^{\mathrm{lab}}\left[H(\pi_v)+D_{\mathrm{KL}}\left(\pi_v\,\Vert\,p_v\right)\right]$. Le coût irréductible $n_v^{\mathrm{lab}}H(\pi_v)$ mesure le mélange du cluster, mais ne concerne pas le décodeur point-wise proposé. Sommer cette loss sur tous les ancêtres répète chaque label ; c'est un régularisateur assumé, sauf si des poids d'incidence $\alpha_{iv}$ vérifient $\sum_{v:i\in C_v}\alpha_{iv}=1$.

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
