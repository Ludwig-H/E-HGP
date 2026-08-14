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

HSA a besoin d'un arbre laminaire, et cet arbre existe déjà. L'arbre de fusion HGP n'est pas construit sur les points mais sur $F_K$, l'ensemble des $(K-1)$-simplexes effectivement construits — les sommets du graphe dual, les simplexes de Gabriel dans la version standard — dont il forme une partition à chaque niveau. Le manuscrit l'écrit explicitement au § 9.1 de la Partie II : « pour $K\geq2$, l'objet naturel n'est pas une partition de $X$, mais un recouvrement de $X$ (ou bien une partition des $(K-1)$-simplexes) ». Le recouvrement n'apparaît donc que dans la projection vers les points, un point appartenant à plusieurs facettes ; il ne touche pas la structure de l'arbre.

La fonction d'ownership que ce document exigeait est fournie par le même paragraphe. À chaque facette $\tau$ est associé un score local positif $S_{\tau}=\sum_{\sigma\supseteq\tau,\left|\sigma\right|=K+1}\psi\left(\rho(\sigma)\right)$, où $\rho(\sigma)$ est le rayon de naissance du $K$-simplexe $\sigma$ dans la filtration et $\psi$ une fonction de poids décroissante ; le choix uniforme $\psi=1$ est admissible, mais $\psi(t)=1/t^{p}$ reflète plus exactement la densité locale. Chaque point normalise par $T_{x}=\sum_{\tau\ni x}S_{\tau}$, avec la convention $1/T_{x}=0$ lorsque $T_{x}=0$, et l'on pose $w_{x\tau}=S_{\tau}/T_{x}$. On a alors $w_{x\tau}\geq0$ et $\sum_{\tau\ni x}w_{x\tau}=1$ : « lorsqu'un point appartient à au moins une face, il distribue une masse totale égale à 1 entre les faces qui le contiennent ». La conservation de masse en découle en une ligne : pour toute antichaîne, en posant $w_{x\rightarrow v}=\sum_{\tau\in v}w_{x\tau}$, on a $\sum_{v}w_{x\rightarrow v}=1$, donc aucun double comptage. Le comptage brut $n_v$ doit être remplacé partout par cette masse pondérée.

L'objet existe ; les obligations de test restent. Une implémentation n'est réputée prête que si le domaine de $w$ est explicite et si les tests de conservation de masse, d'absence de double comptage et d'invariance à l'ordre passent effectivement sur les données réelles — ce sont désormais des vérifications, non des conditions d'existence. Dupliquer silencieusement les points ou arbitrer l'appartenance par ordre d'itération rendrait toujours le résultat non interprétable. Une structure DAG consommée telle quelle demande, elle, un opérateur dédié. Un ensemble de sommets HGP ne doit pas être présenté comme le payload $\mathfrak{P}_{p\leftarrow v}$ si ses facettes, cofaces élémentaires, incidences et niveaux n'ont pas été sérialisés.

La laminarisation a un coût, mais il n'est pas là où ce document le plaçait. Rien n'est détruit au niveau de l'arbre : le lemme de sous-structure optimale de HSA porte sur une partition des feuilles, et les facettes en forment une. Le coût est concentré dans le durcissement final du vote pondéré en une partition stricte des points, et il est mesurable : la marge $V_{x}^{(1)}-V_{x}^{(2)}$ entre les deux premiers clusters et la fraction de points à vote contesté sont le prix exact de la laminarisation et doivent être rapportées comme diagnostic. Il reste vrai qu'à $K=1$ HGP est le single-linkage, donc que la configuration la plus simple n'a aucune nouveauté structurelle. La discussion de ce coût et la conclusion qui en découle sur le placement du budget de nouveauté sont dans [ORDRE_DES_PREUVES.md](ORDRE_DES_PREUVES.md) ; elles ne sont pas reprises ici.

## Représentation des feuilles

Trois granularités seront comparées avec, dans tous les cas, une sortie finale par point :

1. **points comme feuilles**, solution de référence scientifique qui conserve directement la localisation point-wise ;
2. **micro-voxels ou micro-clusters comme feuilles**, seulement si les diagnostics de composition et le décodeur point-wise valident cette réduction ; la baseline majoritaire dure n'est pas à elle seule une porte sur cette granularité ;
3. **facettes comme feuilles**, seule granularité qui rende l'arbre nativement laminaire. Les feuilles sont les éléments de $F_K$, les $(K-1)$-simplexes effectivement construits, et l'arbre de fusion en est une partition à chaque niveau ; le lemme de sous-structure optimale de HSA porte précisément sur une partition des feuilles, donc son hypothèse est satisfaite sans aucun bricolage et le recouvrement n'est pas détruit, puisqu'il subsiste intact dans la projection facettes vers points par les poids $w_{x\tau}$. Le coût est de taille : les facettes sont plus nombreuses que les points, donc profondeur, degré et nombre de feuilles augmentent et doivent être mesurés avant de faire de cette granularité la baseline. Pour $K=1$ les faces sont les points eux-mêmes et cette option coïncide avec la première.

### Choix de la baseline

Dépôts vérifiés le 14 août 2026. Chiffres val SemanticKITTI, une trame à l'inférence, LiDAR seul, sans TTA, entraînement mono-jeu. La recommandation précédente, MinkowskiNet OpenPCSeg en principal, est **corrigée sur preuve** : elle sous-estimait le coût d'installation, qui est le vrai facteur de reproductibilité.

| Candidat | val sans TTA | Extension CUDA à compiler | Ce qui est publié |
|---|---|---|---|
| **WaffleIron-48-256** (principal) | 68,0 | **aucune** ; pip simple, PyTorch 2.2 | config unique `configs/WaffleIron-48-256__kitti.yaml`, checkpoint vivant (HTTP 206), chiffre garanti par le README, 6,8M paramètres |
| **MinkUNet** (second porteur) | 69,3 (mmdet3d spconv) ; 70,3 (torchsparse annoncé) ; 70,04 (OpenPCSeg) | oui : spconv ou torchsparse ou MinkowskiEngine | config, `.pth` et log par ligne (mmdet3d) ; régime OpenPCSeg déclaré verbatim « trained with merely train split », « without employing any Test Time Augmentation or ensembling », 2 A100 et environ 12 h |
| SphereFormer | 67,8 | oui | config et poids, table `Val mIoU (tta) 69.0 / Val mIoU 67.8` |
| PTv3, Pointcept | — | oui | **exclu**, voir ci-dessous |

**Baseline principale : WaffleIron.** L'argument décisif n'est pas le mIoU, c'est qu'aucune extension CUDA n'est à compiler (ni torchsparse, ni MinkowskiEngine, ni spconv, ni flash-attn) : en 2026, c'est ce qui rend un dépôt réellement reproductible. S'y ajoutent une chaîne fermée config → poids → commande → chiffre, le README qui dit littéralement « This should give you a final mIoU of 68.0% », et un mainteneur qui répond précisément (issue #19 : 68,0 en val, 70,8 en test avec `--trainval` et 12 votes ; issue #5 : 4x RTX 2080 Ti et environ 2 jours pour 45 époques, un seul V100 32 Go suffit). Ablation val du papier : 62,5 → 66,8 (cutmix + polarmix) → 67,6 (features 5D) → 68,0 (stochastic depth). **Le 68,0 est nu** : aucun chiffre val avec TTA n'a jamais été publié pour ce dépôt, donc rien n'autorise à le comparer à un chiffre TTA.

**Second porteur : MinkUNet**, famille d'architecture radicalement différente (convolution sparse voxelisée contre MLP et convolutions 2D denses sur projections). C'est précisément le point : un effet HGP qui survit aux deux n'est pas propre au backbone. Avertissement à lever avant toute citation : OpenPCSeg annonce 70,04 en liant `minkunet_mk34_cr10.yaml` alors que le fichier de poids s'appelle `mk34_cr16` ; l'appariement config/poids doit être confirmé, sinon on cite un chiffre pour une autre capacité.

**Exclusion maintenue : PTv3/Pointcept.** Issue #556, ouverte le 13 janvier 2026, donne trois reproductions indépendantes à 69,30 / 66,52 / 66,53 contre 70,8 annoncé ; l'issue #186 est ouverte depuis mars 2024 ; dans l'issue #481 le mainteneur admet « I already forgot where I got this number during paper writing ». Si Pointcept est employé malgré tout, seule sa propre baseline réentraînée est citable, jamais le 70,8 du papier.

**Le facteur dominant est la recette, pas l'architecture.** LaserMix/PolarMix valent +3,5 mIoU sur MinkUNet (66,9 → 70,4) ; instance cutmix + polarmix valent +4,3 sur WaffleIron (62,5 → 66,8), soit plus que l'écart entre la plupart des architectures publiées. Toute évaluation de HGP active donc **exactement les mêmes augmentations sur les deux bras**, faute de quoi le gain mesuré est un gain d'augmentation déguisé. Corollaire : Pointcept n'implémente ni LaserMix ni PolarMix (0 occurrence), ce qui explique probablement une part de l'écart annoncé/reproduit.

| Règle de protocole | Fait qui la fonde |
|---|---|
| Plancher de bruit d'environ 1,5 mIoU | mmdetection3d avertit que le backend TorchSparse fluctue d'environ 1,5 mIoU selon la graine ; l'issue Pointcept #556 va de 66,5 à 69,3 selon le nombre de GPU et le batch |
| Trois graines minimum par bras | tout gain revendiqué sous environ 1,5 point sur un run unique n'est pas distinguable du bruit |
| Jamais un chiffre sans TTA contre un chiffre avec TTA | TTA vaut +1,4 (MinkUNet 70,4 → 71,8), +2,4 (Cylinder3D), +1,2 (SphereFormer) ; celle de mmdet3d coûte 36 passes avant |
| Régime déclaré ligne par ligne | « mono-trame » ne vaut qu'à l'inférence : les recettes de tête mélangent des scans à l'entraînement (instance CutMix, LaserMix, PolarMix, PillarMix). Déclarer entraînement, TTA, ensemble, `train+val`, trames vues à l'inférence |

Pour chaque feuille $i$, le backbone produit $f_i^{0}\in\mathbb{R}^{d}$. Les projections Q/K/V et les normalisations suivent la définition de la baseline HSA testée. Les coordonnées absolues ou cylindriques ne sont pas supprimées : le repère ego et la gravité sont sémantiquement utiles.

## État sémantique multiscale

Chaque nœud $v$ porte une **distribution de proportions sémantiques**, jamais un label dur. Soit $C_v^{\mathrm{lab}}\subseteq C_v$ le sous-ensemble des points dont le label n'est pas ignoré, $n_v^{\mathrm{lab}}=|C_v^{\mathrm{lab}}|$ et $n_{v,c}=\sum_{i\in C_v^{\mathrm{lab}}}\mathbf{1}\left\lbrace y_i=c\right\rbrace$. Sa cible est $\pi_v(c)=n_{v,c}/n_v^{\mathrm{lab}}$ pour $c\in\left\lbrace1,\ldots,19\right\rbrace$. Une feuille point valide a donc une cible one-hot, tandis qu'un cluster traversant une frontière conserve explicitement son mélange de classes. Un nœud sans label valide est masqué pour cette supervision.

À l'inférence, aucune tête indépendante n'est nécessaire dans l'architecture minimale. Les distributions point-wise sont agrégées sans label par $\widehat\pi_v^{\mathrm{all}}=n_v^{-1}\sum_{i\in C_v}p_i$, donc $\widehat\pi_p^{\mathrm{all}}=\sum_{v\in\mathrm{children}(p)}\frac{n_v}{n_p}\widehat\pi_v^{\mathrm{all}}$. Pour la loss seulement, $\widehat\pi_v^{\mathrm{lab}}=(n_v^{\mathrm{lab}})^{-1}\sum_{i\in C_v^{\mathrm{lab}}}p_i$ est comparé à $\pi_v$ ; ce masque GT n'entre jamais dans le forward de validation ou de test. Le backbone local fournit les $p_i^{(0)}$ initiaux ; après chaque bloc, les agrégats peuvent être recalculés. Un readout appris depuis l'état du nœud n'est qu'une ablation auxiliaire. Cet agrégat ne transforme pas silencieusement le nœud en token HSA.

Lorsque les enfants forment une partition laminaire exacte du parent et $n_p^{\mathrm{lab}}>0$, les cibles vérifient $\pi_p=\sum_{v:n_v^{\mathrm{lab}}>0}\frac{n_v^{\mathrm{lab}}}{n_p^{\mathrm{lab}}}\pi_v$, tandis que les agrégats prédits utilisent les cardinalités géométriques $n_v/n_p$. Pour des $K$-polyèdres chevauchants, une moyenne naïve double-compte les points ; la correction est celle du § 9.1 rappelée plus haut. Pour $K\geq2$, la masse d'un nœud n'est pas son cardinal $n_v$ mais la masse pondérée $m_v=\sum_{x}w_{x\rightarrow v}=\sum_{\tau\in v}m_{\tau}$, où $m_{\tau}=S_{\tau}\sum_{x\in\tau}1/T_{x}$ est la masse de facette du manuscrit ; les poids d'agrégation parent–enfants deviennent $m_v/m_p$ au lieu de $n_v/n_p$, et le canal de masse additif redevient exact dès que chaque point est pondéré par $w_{x\rightarrow v}$. C'est aussi ce poids $m_{\tau}$, et non le simple comptage des faces, qui alimente le seuil `min_cluster_size` dans l'arbre condensé. L'expression « partition de l'unité » ne vaut toujours pas preuve de conservation : la conservation est ici démontrée, mais elle doit encore être vérifiée par test sur l'implémentation. Si une feuille représente plusieurs points, les agrégats emploient leurs sorties point-wise originales, ou pondèrent explicitement sa distribution par sa masse.

Une distribution de proportions préserve la masse de chaque classe dans un cluster, mais pas la localisation des classes à l'intérieur. La sortie principale reste donc point-wise et conditionnée par les features de feuille ; aucun broadcast uniforme du vecteur de nœud n'est utilisé comme prédiction finale. En particulier, comparer seulement $\pi_v$ à la moyenne des $p_i$ est invariant à une permutation des prédictions entre points et ne remplace pas la supervision point-wise.

Conserver aussi la cardinalité et deux incertitudes distinctes : l'entropie moyenne $n_v^{-1}\sum_iH(p_i)$ mesure l'incertitude des feuilles, tandis que $H(\widehat\pi_v^{\mathrm{all}})-n_v^{-1}\sum_iH(p_i)$ mesure leur désaccord. Une même proportion `50/50` n'a ainsi pas le même état si toutes les feuilles sont incertaines ou si deux sous-populations sont chacune confiantes.

Le triplet `proportions + entropie moyenne + désaccord`, accompagné de $\log n_v$, est projeté comme **contenu sémantique** dans la passe top-down et le décodeur. Dans la variante HSA fidèle, il ne remplace pas l'embedding positionnel $\epsilon_p(v)$. L'ablation obligatoire compare géométrie seule, proportions seules et combinaison des deux, toujours à partir de proportions prédites.

## Descripteur des nœuds

### Canal de support

Pour un nœud non dégénéré $v$, $s_v(u)=\max_{x\in C_v}\left\langle u,\frac{x-c_v}{R_v}\right\rangle$ est échantillonné sur une grille déterministe de la sphère. Comparer séparément des grilles Fibonacci de cardinalité exacte 20/42/80/162 et des grilles de sommets d'icosphères de cardinalité réelle 12/42/162/642. Chaque direction est normalisée ; si des largeurs sont utilisées, chaque paire $u,-u$ est explicitement présente. La construction, la cardinalité après ajout des antipodes et le rayon de couverture mesuré sont enregistrés. La sélection finale dépend d'une courbe erreur–mémoire–latence.

Ce canal porte par défaut sur le $K$-polyèdre source $C_v$, donc aussi sur le carrier PL des facettes, et non sur $W_v(a)$. Un éventuel support de l'union témoin reçoit un nom et un kernel distincts. Le support maximal est stable vis-à-vis de petites perturbations de l'enveloppe convexe au sens de Hausdorff, mais statistiquement fragile à un point aberrant. Il ne reçoit des gradients que par les points extrêmes des directions. Il reste donc un canal parmi plusieurs.

Ce choix n'est pas arbitraire, et il n'est pas non plus une contribution. Sous cinq hypothèses faibles — localité de repère, agrégation exacte sur l'arbre de fusion, monotonie d'extension, recentrage en forme close et régularité — tout canal scalaire est une reparamétrisation continue strictement croissante d'une valeur de la fonction support. L'énoncé complet, sa démonstration et son antériorité, qui est du folklore recombiné du côté des mesures maxitives et un PointNet à première couche linéaire du côté de l'apprentissage, sont dans [DESCRIPTEURS_DE_NOEUD.md](DESCRIPTEURS_DE_NOEUD.md). Ce résultat sert de lemme justificatif pour l'algorithme de remontée exact ; il n'est jamais annoncé comme un apport.

Deux conséquences contraignent directement l'architecture et doivent être écrites plutôt que subies en review. La première est limitative : sous ces hypothèses, $D(A;c)=D\left(\mathrm{conv}(A);c\right)$, donc un canal exactement fusionnable et exactement recentrable ne voit de $A$ qu'un hyperplan d'appui de son enveloppe convexe, et rien de la non-convexité, des trous ou de la masse intérieure. Tout descripteur réellement sensible à ces trois choses doit abandonner l'une des hypothèses, ce que font explicitement les canaux radiaux et le canal de masse ci-dessous. La seconde est négative : le support d'un nœud interne est une fonction déterministe de ses enfants, donc de ses feuilles, et n'apporte aucun signal propre aux nœuds internes. L'ablation `objet marqué seul` contre `support + objet marqué` mesure alors un effet d'optimisation et non un effet d'information ; elle doit être rapportée comme telle.

Si l'on abandonne la seule idempotence — l'arbre de fusion ne fusionne jamais que des enfants disjoints — tout en conservant recentrage et continuité, la classe admissible ne s'élargit pas indéfiniment : elle se referme exactement sur les fonctions support adoucies par log-sum-exp, $D(A;c)=\psi\left(\beta^{-1}\log\sum_{x\in A}e^{\beta\left\langle a,x-c\right\rangle}\right)$ avec $\beta\in\left(0,+\infty\right]$, le support dur étant le membre $\beta=+\infty$ et l'unique membre idempotent. La version à température finie est donc elle aussi exactement fusionnable sur des enfants disjoints et exactement recentrable, tout en distribuant le gradient sur plus d'un point par direction là où le $\max$ dur n'en alimente qu'un seul. C'est la variante différentiable à tester en priorité contre le support dur, à budget de directions apparié, $\beta$ et $\psi$ devenant des éléments contractuels consignés au même titre que la grille.

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

Pour $K\geq2$, le stockage d'incidences peut rester un DAG global partagé même lorsque les carriers se recouvrent, et sa projection vers HSA est spécifiée : les facettes sont les feuilles, l'arbre de fusion en est une partition à chaque niveau, et les poids $w_{x\tau}$ redescendent le résultat vers les points. La baseline vérifie sur cette projection la conservation de masse, l'absence de double comptage et l'invariance à l'ordre, mais elle n'attend plus une définition manquante. La voie théorique ambitieuse remplace cette projection par une attention directement définie sur le DAG de recouvrement, sans passer par les facettes comme feuilles ; elle doit prouver conservation de masse, stochasticité, équivariance aux réindexations et réduction exacte au cas laminaire. Ce n'est plus une condition d'existence du programme mais une extension, et elle reste le seul endroit où un budget de nouveauté d'opérateur serait bien placé. Cette extension n'hérite pas automatiquement du théorème HSA.

### Contrôle du raccourci de support

Pour toute direction $u$, $h_{C_v^{F}}(u)=\max_{F\in\mathcal{F}_v}\max_{x\in F}\langle u,x\rangle=h_{C_v}(u)$. Pour un carrier des cofaces non vide, l'identité exacte est $h_{C_v^{Q}}(u)=\max_{Q\in\mathcal{Q}_v^{\mathrm{elem}}}\max_{x\in Q}\langle u,x\rangle$ ; elle égale $h_{C_v}(u)$ lorsque les cofaces conservées couvrent $V_v$, mais pas pour une facette isolée omise. Le profil encode séparément le cas vide au lieu de lui attribuer un support fini. Ces tests ne réduisent pas $\mathfrak{P}_{p\leftarrow v}$ à son support. Ils ne s'étendent pas à l'union témoin : $h_{W_v(a)}(u)=\max_{F\in\mathcal{F}_v}h_{\bigcap_{x\in F}\overline{B}(x,\sqrt{a})}(u)$ n'est en général pas égal à $h_{C_v}(u)$.

Ces identités ont une lecture qu'il faut assumer : calculer le support sur le carrier PL des facettes ne donne strictement rien de plus que sur les points, puisque $h_{C_v^{F}}=h_{V_v}$ exactement. Le seul carrier dont le support diffère est l'union témoin, et la grandeur nouvelle n'est pas $h_{W_v(a)}$ pris seul mais l'écart $\Delta_v(u)=h_{V_v}(u)-h_{W_v(a)}(u)\geq0$, mesure directionnelle de la densité au bord : de combien faut-il rentrer depuis le plan d'appui avant que la condition d'ordre $K$ soit satisfaite. Il se calcule sans énumérer les facettes, par dichotomie sur la cote $t$ du plan avec un test d'appartenance par requête $K$-NN — $r_K(y)\leq\sqrt{a}$ et les $K$ voisins appartenant à $C_v$ — pour un coût de $\mathcal{O}\left(D\log(1/\varepsilon)\right)$ requêtes par nœud ; la recette et ses conditions d'usage sont dans [DESCRIPTEURS_DE_NOEUD.md](DESCRIPTEURS_DE_NOEUD.md). Le statut obligatoire est `authority=witness_approx` avec son $\varepsilon_W$ déclaré, jamais `witness_exact`.

Ce canal ne rejoint pas pour autant la passe ascendante à monoïde exact, car $W_v$ dépend du niveau : on n'a que $h_{W_p}(u)\geq\max_{v\in\mathrm{children}(p)}h_{W_v}(u)$, l'écart provenant des facettes nées entre $a_p^{-}$ et $a_p$ et de la croissance des boules. Il doit donc être recalculé par nœud à sa propre coupe, et son coût est compté séparément.

### Canaux radiaux et choix du centre

Pour un carrier déclaré $A_v\in\left\lbrace C_v^{F},C_v^{Q},W_v(a)\right\rbrace$ et un centre $c$, le rayon extérieur est $\rho_{A_v,c}(u)=\sup\left(\left\lbrace r\geq0:c+ru\in A_v\right\rbrace\cup\left\lbrace0\right\rbrace\right)$ et le rayon d'entrée $\rho_{\mathrm{in}}$ en est l'infimum. Ce n'est pas une fonction support. Si $c\in A_v$, la fonction continue reconstruit exactement $A_v$ si et seulement si celui-ci est étoilé autour de $c$ ; tout échantillonnage fini reste un sketch. Le prototype stocke le carrier choisi, `center_in_realization`, `center_in_kernel` et un masque directionnel `ray_hit`, car $\rho=0$ ne distingue pas un rayon vide d'une intersection réduite au centre ; il mesure aussi le nombre de composantes d'intersection par rayon, points isolés inclus.

Le centre n'est pas un détail de mise en œuvre : c'est lui qui décide si le canal existe. Autour du barycentre du nœud, $\rho_{\mathrm{in}}$ est vacu ou instable. Vacu d'abord, puisqu'il est identiquement nul dès que le centre appartient au carrier, ce qui est le régime des nœuds volumiques. Instable ensuite, car ni $\rho_{\mathrm{in}}$ ni $\rho_{\mathrm{out}}$ ne sont lipschitziens en distance de Hausdorff, contrairement au support : percer dans le carrier un cylindre mince de rayon $\delta$ le long du rayon déplace $\rho_{\mathrm{in}}$ d'une quantité fixe, $0{,}5$ dans la fixture de [DESCRIPTEURS_DE_NOEUD.md](DESCRIPTEURS_DE_NOEUD.md), pour une perturbation de Hausdorff au plus $\delta$, sans qu'aucun multiple de $\delta$ ne borne l'écart. L'amincissement angulaire d'un LiDAR à longue portée retire exactement les retours qui définissent le premier contact dans un bin.

Le centre recommandé est donc l'origine du capteur, unique pour tout le scan, et non le barycentre du nœud. Les canaux redeviennent alors exactement fusionnables en une seule passe ascendante — $\max$ pour le support et le rayon extérieur, $\min$ pour le rayon d'entrée, disjonction pour le masque, somme pour les comptes — sans recalcul par nœud ni approximation. Ils reçoivent en outre un sens physique : dans le repère capteur, $\rho_{\mathrm{in}}$ est la surface visible du nœud dans la direction considérée, $\rho_{\mathrm{out}}$ sa surface la plus lointaine, et $\rho_{\mathrm{out}}-\rho_{\mathrm{in}}$ son épaisseur en profondeur, la quantité qui sépare un mur d'un buisson et que la fonction support ne voit pas. La vacuité disparaît, l'origine capteur n'appartenant jamais au carrier d'un nœud. La condition d'étoilement, hypothèse forte autour d'un barycentre, devient automatique pour le carrier source : un scan mono-retour ne fournit au plus qu'un point par faisceau, donc le nuage est déjà exactement étoilé autour du capteur.

Ce choix a un prix et une antériorité, tous deux à déclarer. Le prix est la perte de l'invariance par translation : deux véhicules identiques à deux endroits n'ont plus le même canal radial. C'est une décision, pas un oubli, et elle impose de conserver en parallèle le canal support normalisé par nœud, qui reste invariant par translation et par échelle ; la proposition devient alors un canal invariant exactement fusionnable dans le repère du nœud plus un canal capteur exactement fusionnable dans le repère global. L'antériorité est directe : dans le repère capteur, le couple $\left(\rho_{\mathrm{in}},\rho_{\mathrm{out}}\right)$ binné est une image de portée min/max restreinte au nœud, et la littérature d'images de portée doit être citée comme telle. La nouveauté éventuelle porterait sur le couplage à la hiérarchie, jamais sur la représentation.

En notant $c_0$ l'origine du capteur, $D$ le nombre de directions, $B$ le nombre de bins azimut $\times$ élévation et $b$ le cône d'un bin, la version implémentable comporte six canaux, chacun rempli par un monoïde exact :

| Canal | Définition | Fusion sur l'arbre | Recentrage | Dimension |
|---|---|---|---|---|
| support | $h_v(u_j)=\max_{x\in C_v}\left\langle u_j,x-c_0\right\rangle$ | $\max$ | exact, additif | $D$ |
| dernière sortie | $\rho_{\mathrm{out}}(b)=\max\left\lbrace\left\Vert x\right\Vert:x\in C_v\cap b\right\rbrace$ | $\max$ | sans objet | $B$ |
| première entrée | $\rho_{\mathrm{in}}(b)=\min\left\lbrace\left\Vert x\right\Vert:x\in C_v\cap b\right\rbrace$ | $\min$ | sans objet | $B$ |
| épaisseur | $\rho_{\mathrm{out}}(b)-\rho_{\mathrm{in}}(b)$ | dérivée des deux précédents | sans objet | $B$ |
| masque de bin | $\mathbf{1}\left\lbrace C_v\cap b\neq\emptyset\right\rbrace$ | disjonction | sans objet | $B$ |
| comptage par bin | $\left|C_v\cap b\right|$ | somme | sans objet | $B$ |

Le masque de bin et le comptage par bin ne sont pas optionnels. Sans eux, un bin vide et un bin contenant un seul point sont indiscernables, et c'est la source d'erreur la plus probable de cette famille de descripteurs ; un canal radial livré sans son masque est refusé au chargement, comme l'est un support sans sa convention dégénérée.

Une variante multi-segments encode les extrémités ou une occupation binaire le long du rayon. Pour une réalisation de dimension intrinsèque inférieure à trois, préférer des cônes angulaires ou un épaississement explicite et ablater leur bande passante : un rayon exact générique peut manquer une arête ou une surface. Le cube plein et sa frontière, de mêmes support et rayon extérieur depuis le centre, forment une fixture permanente. Ces variantes sont des ablations compressées face au complexe complet, jamais son substitut dans l'hypothèse principale.

### Contrôle topologique ECT/WECT

Une ECT à directions et seuils finis, éventuellement augmentée par masse, rémission ou niveau HGP, sert de contrôle pour les incidences et trous. Elle n'est pas annoncée comme nouvelle : injectivité de la transformée complète, variantes pondérées et versions différentiables sont déjà publiées. Les garanties WECT citées ne sont transférées à aucun poids réel de rémission ou niveau HGP sans vérifier leurs hypothèses d'admissibilité. Si les simplexes HGP se recouvrent sans former un complexe géométrique conforme, distinguer l'ECT du complexe abstrait de celle de l'union plongée et ne transférer aucun théorème entre les deux sans preuve.

### Canal robuste directionnel

Pour chaque direction, calculer une petite pile de quantiles des projections normalisées $\left\langle u,\frac{x-c_v}{R_v}\right\rangle$, par exemple `q50`, `q90`, `q95`, `q99` et `max`. Les quantiles ne sont pas des fonctions support convexes au sens strict ; ils sont utilisés comme features de distribution. Une variante log-mean-exp multi-température doit être testée pour distribuer les gradients, sans la présenter comme robuste aux outliers.

### Canal de masse projetée, ajout prioritaire

La correction la plus directe au support consiste à conserver la **distribution** de chaque projection, pas seulement son maximum. Pour $R_v>0$ et des seuils fixes $t_b$, définir $F_v(u_j,t_b)=n_v^{-1}\sum_{x\in C_v}\mathbf{1}\left\lbrace\left\langle u_j,(x-c_v)/R_v\right\rangle\leq t_b\right\rbrace$. Ce tenseur direction × seuil a une dimension fixe et peut distinguer certains clusters ayant la même enveloppe mais des masses intérieures différentes. Pour $K\geq2$, le comptage uniforme et le facteur $n_v^{-1}$ sont remplacés par la masse pondérée : chaque point compte pour $w_{x\rightarrow v}$ et la normalisation devient $m_v=\sum_{x\in C_v}w_{x\rightarrow v}$, faute de quoi un point partagé entre plusieurs facettes est compté plusieurs fois et la CDF projetée cesse d'être additive sur l'arbre. Avec toutes les directions et toute la CDF projetée, la mesure normalisée est déterminée par le théorème de Cramér–Wold ; avec une grille finie, il s'agit seulement d'un sketch à auditer. Le cas $R_v=0$ est la masse de Dirac dégénérée traitée par le masque ci-dessous.

La priorité de cet ajout repose sur une séparation d'expressivité, pas sur une intuition. Le support, le rayon extérieur et le rayon d'entrée sont tous trois des extrema et ne voient aucune masse intérieure ; un agrégateur $\max$ à la PointNet ne peut pas approcher une moyenne de fonction continue, le centre de masse en étant le contre-exemple canonique, alors qu'un agrégateur additif à la DeepSets le peut. La fusion par $\max$ et la fusion par somme sont l'une et l'autre exactes sur l'arbre, mais elles ne sont pas d'expressivité comparable, et c'est ce canal qui répond à la fixture `cube plein contre frontière`. La borne de Wagstaff *et al.* (ICML 2019) reste opposable aux deux : un latent de dimension $D$ ne représente fidèlement que les ensembles de cardinal au plus $D$, donc tout descripteur de cette famille est prouvablement lossy dès que $n_v>D$, ce qui est le régime de tous les gros nœuds.

Le budget de directions n'est pas le même que pour le support. Pour la fonction support, $u$ et $-u$ ne sont pas redondantes, puisque $h(-u)=-\min_{x\in C_v}\left\langle u,x\right\rangle$ borne l'objet de l'autre côté. Pour la CDF elles le sont, via $F(-u,t)=1-F\left(u,(-t)^{-}\right)$, la valeur limite à gauche traitant les masses ponctuelles au seuil. La CDF ne vit donc que sur le plan projectif $\mathbb{RP}^{2}$ et demande, à résolution angulaire égale, deux fois moins de directions que le support. Le budget épargné se redéploie en seuils $t_b$ ou en directions supplémentaires, mais la convention antipodale doit être déclarée dans le contrat de grille pour ne pas compter deux fois la même information.

Pour le prototype, préférer des histogrammes/CDF à bins fixes, éventuellement complétés par quelques quantiles robustes et par le max exact. Les comptes d'histogramme sont additifs dans un repère commun, contrairement à une liste de quantiles qui ne se fusionne pas exactement. Une normalisation indépendante par nœud exige cependant de transporter le déplacement et le ratio d'échelle, puis de rééchantillonner les bins ; sinon le canal est calculé directement depuis les points lors du prétraitement.

Un piège de mesure doit être écrit ici, car il est invisible dans la notation. Si la CDF est calculée non pas sur les points mais sur le carrier PL, elle est identiquement dégénérée dans le régime prévu : dans $\mathbb{R}^{3}$, $\mathrm{conv}(F)$ pour $\left|F\right|=K$ est de dimension $K-1$, soit un segment pour $K=2$ et un triangle pour $K=3$, donc de mesure de Lebesgue nulle. Une CDF de volume du carrier vaudrait alors zéro partout pour les ordres $K=2,3$ effectivement prévus. La version correcte pondère chaque facette par la mesure de Hausdorff de sa propre dimension, longueur pour $K=2$ et aire pour $K=3$. La projection d'un segment est une densité uniforme sur un intervalle, celle d'un triangle est affine par morceaux, et la quantité reste additive sur les facettes, donc exactement fusionnable. Le choix `points` ou `carrier` et la dimension de la mesure employée deviennent des champs contractuels du canal.

L'hypothèse principale à tester est donc `support normalisé + objet HGP marqué complet + side channels métriques`. Elle est comparée à `support seul`, `objet marqué seul`, un multiensemble de cellules sans incidences, $\Gamma_K^{\mathrm{elem}}$ seul, $\Gamma_K^{\mathrm{full}}$ lorsqu'il est effectivement disponible, et un encodeur Deep Sets de même budget. Rayon extérieur, multi-segments, CDF, ECT/WECT et moments restent des ablations compressées ou des contrôles. Le support décrit directement les extrêmes ; le payload porte les incidences et les niveaux HGP, tandis que le `carrier_kind` déclaré détermine la géométrie, potentiellement non convexe.

### Convention dégénérée

Si $R_v=0$, fixer supports et quantiles normalisés à zéro et activer un masque `degenerate`. Le canal de taille utilise $\log\left(\max\left(R_v,\varepsilon_{\mathrm{metric}}\right)\right)$ accompagné du même masque. Le plancher métrique est fixé dans la configuration ; il sert à la stabilité numérique et rompt l'invariance d'échelle exacte du cas dégénéré. Pour un parent de rayon nul, le déplacement et le ratio d'échelle sont fixés à zéro et masqués.

### Canaux non normalisés

Le modèle reçoit séparément :

- $\log\left(\max\left(R_v,\varepsilon_{\mathrm{metric}}\right)\right)$, le masque dégénéré et les dimensions métriques du nœud ;
- `log(1 + n_v)`, masse et estimation de densité ; pour $K\geq2$, $n_v$ y est remplacé par la masse pondérée $m_v=\sum_{x}w_{x\rightarrow v}$ ;
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

Sous LayerNorm, l'énergie d'interaction ne dépend que des moyennes de requêtes et de clés de chaque sous-arbre, pondérées par sa taille. Une HSA fidèle est donc mécaniquement une attention sur des moyennes de sous-arbres, ce qui la rapproche beaucoup de la baseline `bottom-up/top-down mean + MLP` de la matrice appariée : cette baseline n'est pas un contrôle faible, c'est le concurrent le plus proche, et elle doit être entraînée avec exactement le même soin que le modèle principal. Les auteurs indiquent par ailleurs que leur cadre n'ajoute aucun paramètre apprenable le long de la hiérarchie ; un gain ne peut donc provenir que de la structure de l'arbre et du contenu des descripteurs, jamais d'une capacité supplémentaire.

Le point de passage du descripteur est étroit, et cela conditionne l'ordre des investissements. Dans la variante fidèle, la géométrie n'entre dans HSA que par le produit scalaire $\epsilon_p(v)^{\top}\epsilon_p(w)$ entre frères, soit un seul scalaire de biais par couple de frères. Raffiner le descripteur — davantage de directions, canaux radiaux, CDF projetée — sans toucher à ce goulot ne peut produire qu'un effet borné par ce scalaire ; l'ablation qui fait varier la richesse du descripteur à goulot constant doit donc précéder tout investissement dans le descripteur. Faire entrer la géométrie dans la voie des valeurs, ou conditionner $V$ et le gate sur elle, sort du théorème HSA : c'est une variante à annoncer comme telle, avec ses propres garanties et sa propre colonne dans la matrice appariée.

L'algorithme demande enfin autant de produits matrice creuse–vecteur strictement séquentiels que l'arbre a de niveaux. La condensation de l'arbre de fusion, qui contracte les chaînes unaires, est donc une condition d'existence sur GPU et non une optimisation ajoutée après coup : elle est versionnée, journalisée avec la profondeur avant et après, et ablatée comme un composant du modèle. Lorsqu'une condensation au sens de l'arbre condensé est également employée, son seuil `min_cluster_size` s'applique à la masse pondérée $m_{\tau}$ et non au comptage de faces. Les mesures rapportent la profondeur effective au même titre que $\sum_v d_v^{2}$, puisque c'est elle qui fixe la latence irréductible.

Le cadre lui-même n'a été validé que sur du texte : le papier ne contient aucune expérience 3D ni dense, et le remplacement zéro-shot d'une attention plate par HSA dégrade fortement certaines tâches, QNLI tombant à $0{,}5072$, soit le niveau du hasard. Le transfert au nuage de points est une hypothèse à tester dans cette architecture, pas un acquis hérité.

Les nœuds de degré élevé, les chaînes profondes et les arbres déséquilibrés sont mesurés. La binarisation ou le rééquilibrage ne sont pas gratuits : ils introduisent une structure artificielle et doivent être ablatés contre l'arbre natif.

### Opérateur expérimental : QC-HSA

HSA partage un coefficient sur tout rectangle entre deux branches sœurs, donc aussi entre plusieurs feuilles requêtes de la même branche. `QC-HSA` conserve au contraire chaque feuille requête. Pour une feuille $i$, elle agrège les clés/valeurs des sous-arbres frères rencontrés sur le chemin vers la racine et calcule un poids propre à $i$ pour chacun de ces groupes.

Les clés et valeurs moyennes des nœuds sont construites bottom-up. Pour un score bilinéaire et un biais HGP constant dans le groupe cible, chaque sortie s'obtient sans matrice dense par un Softmax sur score moyen + log-cardinalité. La [proposition candidate](THEOREMES.md) en fait la projection reverse-KL optimale sur ces contraintes conditionnées par la feuille, montre que sa famille contient celle de HSA et donne l'exactitude des scores constants sur chaque couple de branches de fusion.

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

Lorsque les feuilles sont les facettes, ce chemin est la relaxation différentiable de la Proposition 7 du manuscrit. Celle-ci convertit des étiquettes de facettes en partition stricte par le vote pondéré $V_{x}(c)=\sum_{\tau\ni x,\ell(\tau)=c}S_{\tau}/T_{x}$ puis $\mathrm{label}(x)\in\arg\max_{c}V_{x}(c)$, ce qui garantit une partition disjointe avec une classe $-1$ pour les points non classés, sous une règle déterministe de départage des égalités. Pour un réseau, l'argmax est remplacé par la combinaison convexe $p(x)=\sum_{\tau\ni x}w_{x\tau}\,p_{\tau}$, où $p_{\tau}$ est la distribution prédite sur la facette : c'est la Proposition 7 avant durcissement, donc différentiable. Chaque point conserve une prédiction propre, puisque les poids $w_{x\tau}$ dépendent de lui et non du seul nœud. L'argmax redevient la version d'inférence si une partition stricte est exigée ; la marge $V_{x}^{(1)}-V_{x}^{(2)}$ et la fraction de points à vote contesté mesurent alors ce que ce durcissement coûte. Pour $K=1$ les faces sont les points eux-mêmes, le vote est trivial et l'on retrouve exactement le single-linkage.

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
