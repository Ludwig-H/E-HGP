# Programme du résultat théorique

## Décision

Le résultat candidat porte sur une **attention hiérarchique conditionnée par la requête**, notée provisoirement `QC-HSA`. Il ne cherche pas à prouver que HGP est sémantiquement optimal. Il caractérise l'attention qui minimise la reverse-KL par rapport à une attention Softmax plate, parmi toutes celles qui respectent les partitions cibles imposées par l'arbre tout en conservant une requête distincte pour chaque feuille.

Le nom `QC-HSA` est provisoire : cet opérateur relâche la famille de contraintes de HSA et n'est pas un cas automatiquement couvert par le théorème HSA.

Cette cible est pertinente pour la segmentation dense : HSA impose la même interaction à tous les couples de feuilles appartenant à deux branches sœurs. Deux points d'une même branche, dont l'un est près d'une frontière sémantique, peuvent pourtant devoir interroger différemment une branche distante. `QC-HSA` retire ce couplage côté requête sans abandonner la compression hiérarchique côté clés/valeurs.

L'énoncé ci-dessous est un **résultat technique à démontrer et à auditer**, pas encore le théorème central du papier. Sa preuve élémentaire paraît solide, mais Fast Multipole Attention et l'appendice du papier HSA rendent sa nouveauté probablement insuffisante seul. Il ne deviendra central qu'avec un certificat HGP spécifique, non vacu et calculable.

## Priorités théoriques révisées

L'idée géométrique impose d'abord deux résultats négatifs. Pour une union simpliciale, le support est celui des sommets ; pour un centre donné, le rayon extérieur reconstruit exactement seulement les ensembles étoilés. Ces propositions et leurs contre-exemples sont nécessaires à la correction du papier, mais trop classiques pour en être la nouveauté. ECT/PHT possèdent déjà des résultats d'injectivité ; les employer ne crée pas non plus un théorème central.

Les cibles réellement susceptibles de porter une soumission sont, par ordre de faisabilité :

1. **Certificat adaptatif d'attention.** Construire, depuis les résumés d'un nœud et la requête, une borne calculable $U_{iB}$ sur l'oscillation des scores du bloc $B$. Raffiner les blocs dont $U_{iB}$ dépasse une tolérance, prouver monotonie, puis convergence vers l'attention dense lorsque tous les blocs atteignent les singletons avec les mêmes scores et masques, et enfin une borne sur KL puis sortie. La complexité doit être output-sensitive et mesurée, pas seulement annoncée sous degré borné.
2. **Sélection fidélité–coût.** Pour une requête ou une famille de requêtes, choisir une antichaîne sous budget avec optimum exact dans le cas additif, ou garantie d'approximation dans le cas partagé. La nouveauté doit dépasser l'élagage d'arbre classique et inclure le vrai coût des kernels.
3. **Stabilité HGP corrigée de la portée.** Sous un modèle explicite de thinning LiDAR $p(r,\theta)$, borner la merge distortion entre l'arbre estimé et un arbre latent, puis composer cette borne avec le certificat d'attention. Une asymptotique populationnelle doit utiliser un régime $K_n$ déclaré ; les seuls ordres fixes $K\in\left\lbrace1,2,3\right\rbrace$ ne l'établissent pas.
4. **Extension recouvrante.** Si les $K$-polyèdres d'ordre supérieur restent un DAG, définir des poids d'incidence formant une partition de l'unité, puis construire et prouver séparément conservation de masse, stochasticité de l'attention, absence de double comptage et réduction exacte au cas laminaire ; la partition de l'unité seule ne suffit pas.

Le candidat 1 est le meilleur compromis actuel. Le candidat 3 serait le plus fort pour ICML/NeurIPS, mais aussi le plus risqué. Aucune priorité n'autorise un énoncé « HGP est sémantiquement optimal » : cela demanderait un modèle joint réaliste des labels et du capteur.

## Partition canonique vue par une feuille

Soit $T$ un arbre fini laminaire enraciné dont les feuilles sont $N\geq2$ points ou micro-tokens. Une forêt doit soit être traitée composante par composante, soit recevoir une famille racine explicite si l'attention plate globale entre composantes fait partie de la cible. Le résultat porte sur cet arbre laminaire livré : il ne s'applique pas directement aux unions de $K$-polyèdres chevauchantes pour $K\geq2$, sauf après une projection laminaire déterministe et auditée. Pour une feuille requête $i$, suivre le chemin de $i$ à la racine. À chaque ancêtre, collecter les sous-arbres frères de la branche qui contient $i$. Leurs ensembles de feuilles forment une partition disjointe $\Pi_T(i)$ du domaine $\Omega_i=\left\lbrace1,\ldots,N\right\rbrace\setminus\left\lbrace i\right\rbrace$.

Dans le cas HGP, chaque bloc $B\in\Pi_T(i)$ représente une branche rencontrée à une échelle de l'estimateur de densité. Deux cibles $j,k\in B$ sont indiscernables pour cette requête, mais cette égalité n'est pas imposée à une autre requête $i'$.

Définir $\mathcal{R}_T$ comme l'ensemble des matrices $Q$ telles que $Q_{ii}=0$, $Q_{ij}\geq0$, $\sum_{j\in\Omega_i}Q_{ij}=1$ et $Q_{ij}=Q_{ik}$ pour tout $B\in\Pi_T(i)$ et tous $j,k\in B$. La diagonale est masquée dans $P$ et $Q$, comme dans le papier HSA ; toute comparaison expérimentale flat/HSA/QC-HSA utilise ce même masque.

## Proposition candidate — projection KL conditionnée par la requête

Soit $P$ une matrice d'attention plate strictement positive hors diagonale et stochastique par ligne. Pour chaque $i$ et $B\in\Pi_T(i)$, poser $g_{iB}=\exp\left(\frac{1}{|B|}\sum_{j\in B}\log P_{ij}\right)$ et $Z_i=\sum_{B\in\Pi_T(i)}|B|g_{iB}$.

Alors la solution unique de $Q^{\star}=\arg\min_{Q\in\mathcal{R}_T}\sum_i D_{\mathrm{KL}}\left(Q_i\,\Vert\,P_i\right)$ est donnée par $Q^{\star}_{ii}=0$ et $Q^{\star}_{ij}=\frac{g_{iB}}{Z_i}$ pour $j\in\Omega_i$ et l'unique $B\in\Pi_T(i)$ contenant $j$. La distorsion optimale par feuille vaut $D_i^{\star}=D_{\mathrm{KL}}\left(Q_i^{\star}\,\Vert\,P_i\right)=-\log Z_i$, les distributions étant restreintes à $\Omega_i$.

L'inégalité arithmético-géométrique donne $0<Z_i\leq1$. On a $D_i^{\star}=0$ si et seulement si $P_i$ est constant sur chaque bloc de $\Pi_T(i)$.

Si $P_{ij}=\exp(S_{ij})/\sum_{k\ne i}\exp(S_{ik})$, la normalisation plate s'élimine dans les poids projetés. En posant $\bar S_{iB}=\frac{1}{|B|}\sum_{j\in B}S_{ij}$, on obtient $Q^{\star}_{ij}=\frac{\exp(\bar S_{iB})}{\sum_{C\in\Pi_T(i)}|C|\exp(\bar S_{iC})}$ et $D_i^{\star}=\log\left(\sum_{j\ne i}\exp(S_{ij})\right)-\log\left(\sum_{B\in\Pi_T(i)}|B|\exp(\bar S_{iB})\right)$.

La masse d'un bloc est donc un Softmax sur $\bar S_{iB}+\log|B|$. Le terme de cardinalité est indispensable : l'omettre ne donne plus la projection KL.

### Extension massique pour des feuilles agrégées

L'énoncé précédent est une projection au niveau des feuilles. Pour une **requête atomique** $i$, si une feuille cible $j$ représente $w_j$ clés/valeurs répétées, poser $M_B=\sum_{j\in B}w_j$ et $\bar S_{iB}^{w}=M_B^{-1}\sum_{j\in B}w_jS_{ij}$, puis remplacer $|B|$ par $M_B$ et les moyennes par leurs versions massiques. Cela ne rend pas une feuille requête de masse $w_i>1$ exactement équivalente à l'attention point-level : les requêtes internes restent distinctes et le masque diagonal laisse $w_i-1$ cibles intra-feuille. Il faut alors conserver des requêtes atomiques, masquer tout le bloc intra-feuille, ou modéliser explicitement un bloc interne de masse $w_i-1$ sous une hypothèse de clones identiques. Aucun claim point-level exact n'est fait pour des requêtes micro-tokens génériques.

### Exactitude des noyaux de fusion sur un arbre

Si $S_{ij}=f_i(a_{ij},b_{ij})$, où $a_{ij}$ et $b_{ij}$ sont les deux plus hauts ancêtres distincts de $i$ et $j$ sous leur LCA, alors $S_{ij}$ est constant sur chaque $B\in\Pi_T(i)$. Dans ce cas $P\in\mathcal{R}_T$, $Q^{\star}=P$ et $D_i^{\star}=0$ pour tout $i$, y compris sur un arbre déséquilibré. La forme plus restrictive $f_i(\mathrm{LCA}_T(i,j))$ est aussi exacte, mais ne distingue pas les différents frères d'un nœud multifurqué.

Cette exactitude vaut pour tout arbre laminaire. HGP la rend potentiellement utile en décorant les branches de fusion par niveau de densité, persistance et sketch géométrique. Un score appris à partir de la feuille requête et de ces deux branches admet une attention Softmax plate exacte par agrégation hiérarchique. Le résultat ne couvre pas un biais ultramétrique simplement ajouté à un terme point–point arbitraire.

### Certificat par oscillation intra-branche

Si l'oscillation des scores dans chaque bloc vérifie $\max_{j\in B}S_{ij}-\min_{j\in B}S_{ij}\leq\delta_i$, alors $0\leq D_i^{\star}\leq\delta_i^2/8$. Cette borne de Hoeffding transforme la dispersion intra-branche en certificat de fidélité. Une version pratique devra majorer $\delta_i$ à partir de résumés de nœuds calculables, potentiellement des largeurs/supports directionnels et des bornes Lipschitz des projections, sans parcourir toutes les paires.

### Monotonie par raffinement

Scinder un bloc cible en sous-blocs relâche des égalités et ne peut pas augmenter la distorsion optimale. Cette monotonie fournit une courbe fidélité–coût naturelle le long de HGP. Pour une requête isolée et un budget de blocs, la somme $\sum_B|B|g_{iB}$ permet une DP additive ; la difficulté réapparaît pour une coupe partagée entre requêtes/têtes, les contraintes HSA ou un coût GPU non additif. Le tree pruning générique reste un précédent ancien.

## Domination de HSA

Soit $\mathcal{B}_T^0$ la famille non vide et fermée des matrices HSA à blocs avec le même masque diagonal. Toute matrice de $\mathcal{B}_T^0$ est constante, pour une requête $i$, sur chaque sous-arbre frère de son chemin. Par conséquent, $\mathcal{B}_T^0\subseteq\mathcal{R}_T$ et $\min_{Q\in\mathcal{R}_T}\sum_i D_{\mathrm{KL}}\left(Q_i\,\Vert\,P_i\right)\leq\min_{Q\in\mathcal{B}_T^0}\sum_i D_{\mathrm{KL}}\left(Q_i\,\Vert\,P_i\right)$.

L'inégalité est stricte si et seulement si le minimiseur dans $\mathcal{R}_T$ ne satisfait pas les égalités supplémentaires entre lignes imposées par HSA. Ce cas est attendu lorsque deux feuilles d'une même branche ont des contenus ou des contextes sémantiques différents. La comparaison universelle porte sur les deux familles optimales ; elle porte sur l'opérateur HSA effectif seulement sous les hypothèses exactes de son théorème — même énergie/cible $P$, Q/K LayerNormés, température, rescaling et masque.

`QC-HSA` est ainsi la projection reverse-KL optimale sur les égalités définies par $\Pi_T(i)$, sans égalité supplémentaire entre requêtes. Cette baisse de KL est achetée par davantage d'interactions : sur un arbre binaire équilibré, QC-HSA en utilise $N\log_2N$, contre environ $2(N-1)$ blocs dirigés pour HSA. La comparaison pertinente est donc un Pareto fidélité–coût, pas une domination gratuite.

## Pont conditionnel vers l'arbre populationnel de Hartigan

Ce pont répond à la question « pourquoi un meilleur arbre HGP aiderait-il ? », mais seulement après avoir défini **meilleur**.

Soit $T_f$ l'arbre des composantes connexes des ensembles de niveau d'une densité latente $f$, et $m_f(x_i,x_j)$ la hauteur à laquelle $x_i$ et $x_j$ fusionnent dans cet arbre. Pour un arbre estimé $\widehat T$ sur des feuilles bijectivement appariées — ou sur leur restriction commune — muni de hauteurs $\widehat m_{ij}$ dans la **même coordonnée calibrée**, définir sa distorsion de fusion empirique par $\delta(\widehat T,T_f)=\max_{i\ne j}|\widehat m_{ij}-m_f(x_i,x_j)|$. Une log-densité tronquée peut éviter une singularité ; la valeur de $\delta$ dépend de cette paramétrisation.

La consistance de Hartigan seule est insuffisante ici : elle ne garantit pas conjointement séparation et minimalité et, a fortiori, ne fournit aucune erreur uniforme sur les hauteurs de fusion. La quantité pertinente pour l'attention est une merge distortion calibrée, plus forte.

Supposons un score idéal $S^f_{ij}=\phi_i(m_f(x_i,x_j))$, où chaque $\phi_i$ est $L_m$-Lipschitz, et le score estimé $\widehat S_{ij}=\phi_i(\widehat m_{ij})$. `QC-HSA` calcule exactement le Softmax de $\widehat S$ car ce score dépend du nœud de fusion estimé. De plus, $D_{\mathrm{KL}}\left(\mathrm{softmax}(\widehat S_i)\,\Vert\,\mathrm{softmax}(S_i^f)\right)\leq L_m^2\delta^2/2$ et la même borne vaut dans le sens opposé.

Avec les mêmes valeurs de diamètre $D_V$, les sorties d'attention diffèrent donc d'au plus $D_VL_m\delta/2$. Pour une tête dont chaque logit est $L_h$-Lipschitz, une prédiction idéale de marge $m_i$ est préservée si $m_i>L_hD_VL_m\delta$. Sur les $N$ feuilles appariées, la conclusion rigoureuse immédiate est la borne empirique $\widehat R_N(\widehat T)\leq\widehat R_N(T_f)+\frac{1}{N}\sum_{i=1}^{N}\mathbf{1}\left\lbrace m_i\leq L_hD_VL_m\delta\right\rbrace$ pour le risque point-wise du modèle fixé.

Une borne populationnelle demande une étape supplémentaire. Par exemple, pour un scan aléatoire $Z$, une feuille aléatoire $I$ du scan et une distorsion $\delta_Z$ définie sur les mêmes feuilles, les hypothèses vérifiées presque sûrement donnent $R(\widehat T)\leq R(T_f)+\Pr\left[m_I\leq L_hD_VL_m\delta_Z\right]$. Une généralisation hors des feuilles appariées ne découle pas de la projection KL.

Ainsi, définir l'ensemble certifié $\mathcal{C}(\widehat T)=\left\lbrace i:m_i>L_hD_VL_m\delta(\widehat T,T_f)\right\rbrace$. Si HGP a une distorsion de fusion plus petite que celle d'une hiérarchie RSL appariée, alors $\mathcal{C}(\widehat T_{\mathrm{HGP}})\supseteq\mathcal{C}(\widehat T_{\mathrm{RSL}})$ sous les mêmes scores idéaux, valeurs, tête et marges. L'argument s'étend à un objet HDBSCAN une fois sa version hiérarchique précisément fixée. Cela ne prouve pas que son mIoU réalisé sera supérieur : ordonner deux bornes supérieures, ou des ensembles certifiés relativement à un modèle idéal, n'ordonne pas nécessairement deux erreurs réelles.

Le lien devient sémantique seulement sous une hypothèse de cluster formelle : le posterior de classe $\eta_c(x)=\Pr(Y=c\mid X=x)$ doit varier faiblement à l'intérieur des branches pertinentes, ou ses transitions doivent suivre des vallées de la densité, et il doit exister un score pairwise idéal $S^f_{ij}=\phi_i(m_f(x_i,x_j))$ dont l'attention suivie de la tête fixée possède un faible risque et des marges non vacues. Les proportions de labels sont des **marques** agrégées sur l'arbre ; la merge distortion ne les contrôle pas à elle seule. Cette hypothèse est forte sur SemanticKITTI. Sans correction du processus d'échantillonnage LiDAR, $f$ peut surtout décrire la portée, l'angle et l'occultation ; mieux estimer son arbre peut alors ne rien apporter aux classes.

Enfin, sous ses hypothèses, notamment de position générale, le papier HGP actuel établit une correspondance sur l'échantillon fini avec son estimateur de densité $K$-NN, pas une convergence de MorseHGP3D/HGP vers $T_f$ en distorsion de fusion. Une preuve populationnelle doit spécifier un régime de lissage asymptotique ; les résultats classiques pour une densité $K_n$-NN demandent généralement que $K_n$ croisse tout en restant négligeable devant $n$, alors que les expériences $K\in\left\lbrace1,2,3\right\rbrace$ utilisent un ordre fixe. On ne peut pas transférer silencieusement le résultat fini au modèle de Hartigan. DBSCAN à paramètres fixés est une partition plate. RSL est le contrôle théorique naturel ; pour HDBSCAN, il faut distinguer l'arbre brut de mutual reachability, l'arbre condensé et la sélection plate, sans lui attribuer automatiquement les garanties de RSL. La garantie suppose en outre valeurs et tête fixes ; elle ne s'applique pas directement au thinning, aux occultations ou à tout changement de cardinalité sans une correspondance partielle et des termes d'erreur supplémentaires.

La version susceptible de devenir centrale serait donc un théorème en deux étages : prouver avec probabilité au moins $1-\alpha$ une borne $\delta(\widehat T_{\mathrm{HGP}},T_f)\leq r_n(\alpha)$ sous un modèle LiDAR corrigé de la portée, puis obtenir un excès de risque contrôlé par $\alpha+\Pr\left[m_I\leq L_hD_VL_m r_n(\alpha)\right]$. Une supériorité théorique sur RSL, puis éventuellement sur une variante HDBSCAN précisément définie, exigerait en plus une classe de distributions et une comparaison de taux ou de constantes ; elle ne découle ni du nom HGP ni de l'inclusion des hiérarchies.

## Calcul sans matrice dense

Pour des scores $S_{ij}=q_i^{\top}k_j/\sqrt{d}+b_{iB}$ dont le biais $b_{iB}$ est constant sur $B$, on a $\bar S_{iB}=q_i^{\top}\bar k_B/\sqrt{d}+b_{iB}$, où $\bar k_B=|B|^{-1}\sum_{j\in B}k_j$. Les moyennes de clés et de valeurs se calculent bottom-up une fois par nœud.

La sortie projetée se calcule exactement par $y_i^{\star}=\sum_{B\in\Pi_T(i)}\frac{|B|\exp(\bar S_{iB})}{\sum_{C\in\Pi_T(i)}|C|\exp(\bar S_{iC})}\bar v_B$, où $\bar v_B=|B|^{-1}\sum_{j\in B}v_j$. Aucun coefficient point–point n'est matérialisé.

Le nombre d'interactions agrégées est $C_T=\sum_i|\Pi_T(i)|=\sum_{v\in T_{\mathrm{int}}}|\ell(v)|(d_v-1)$, où $\ell(v)$ est l'ensemble des feuilles descendantes et $d_v$ le nombre d'enfants. Le temps d'une tête est $\mathcal{O}((N+|T|)d+C_Td)$. Pour un arbre $b$-aire équilibré, $C_T=\mathcal{O}(Nb\log_b N)$ ; pour une étoile, une chaîne ramifiée ou un arbre peigne, il peut redevenir quadratique. La condensation, le degré et la profondeur sont donc des conditions expérimentales, pas des détails d'implémentation.

Le calcul exact de la **valeur** $D_i^{\star}$ demande aussi la constante de normalisation de l'attention plate. Il peut rester quadratique même si les poids et sorties projetés sont calculés rapidement. Un certificat sous-quadratique exigera une borne supplémentaire sur les scores à l'intérieur des nœuds ; il ne doit pas être supposé gratuitement.

## Corollaire de fidélité de la décision

Ce corollaire est utile, mais standard ; il ne constitue pas à lui seul la nouveauté.

Supposons que les valeurs aient un diamètre au plus $D_V$ dans la norme choisie. Si $y_i=\sum_jP_{ij}v_j$ est la sortie plate, alors $\left\Vert y_i^{\star}-y_i\right\Vert\leq D_V\sqrt{D_i^{\star}/2}$ par Pinsker. Si chaque logit d'une tête point-wise fixe est $L_h$-Lipschitz et si la marge top-1 plate au point $i$ vaut $m_i$, la classe est certifiée inchangée dès que $D_i^{\star}<\frac{m_i^2}{2L_h^2D_V^2}$.

Cette garantie préserve la décision de l'attention plate, qu'elle soit correcte ou fausse. Elle ne borne pas directement le mIoU et ne couvre pas automatiquement les changements de V, les gates, les MLP ou plusieurs couches. Ces termes devront être ajoutés avant tout claim de stabilité du réseau complet.

## Esquisse de preuve

La contrainte se sépare par ligne et par bloc. Pour une ligne $i$, écrire une valeur commune $q_{iB}$ sur chaque $B$ et imposer $\sum_B|B|q_{iB}=1$. Le Lagrangien est $\sum_B\sum_{j\in B}q_{iB}\log(q_{iB}/P_{ij})+\lambda_i\left(\sum_B|B|q_{iB}-1\right)$. L'annulation de sa dérivée donne $\log q_{iB}=|B|^{-1}\sum_{j\in B}\log P_{ij}-1-\lambda_i$, donc la moyenne géométrique puis la normalisation $Z_i$ ; la stricte convexité du reverse-KL donne l'unicité. La valeur $-\log Z_i$ suit par substitution. L'inclusion $\mathcal{B}_T^0\subseteq\mathcal{R}_T$ donne la comparaison. La formule de sortie vient du regroupement exact des valeurs par bloc. L'exactitude des noyaux de fusion vient de la constance du couple de branches sur chaque sous-arbre frère, et la borne d'oscillation du lemme de Hoeffding appliqué à chaque bloc.

Pour le pont Hartigan, la Lipschitzianité donne $|\widehat S_{ij}-S^f_{ij}|\leq L_m\delta$, donc une oscillation d'au plus $2L_m\delta$ par ligne. Le même argument de Hoeffding, appliqué dans chaque sens aux deux Softmax, donne le KL $L_m^2\delta^2/2$. Pinsker, le diamètre des valeurs et la marge donnent les deux dernières inégalités.

## Pourquoi les autres théorèmes envisagés sont écartés

- **« HGP est l'arbre optimal »** : aucune distribution sémantique ni fonction de risque ne permet aujourd'hui un tel énoncé.
- **« Un score ultramétrique donne une attention HSA exacte »** : faux en général, car la normalisation Softmax par ligne peut rompre les égalités entre lignes d'un même rectangle.
- **« Merge distortion HGP borne l'erreur sémantique »** : faux sans contrôler aussi la géométrie, les décorations, Q/K/V et les changements combinatoires de l'arbre. La merge distortion seule ne voit pas ces quantités.
- **« La meilleure coupe globale partagée sous budget se trouve toujours par une DP simple »** : l'optimal tree pruning est ancien ; une DP existe pour une requête QC isolée et un coût additif, mais les normalisations/couplages d'une coupe partagée, de HSA ou d'un coût GPU réel demandent une analyse distincte.
- **« KL implique mIoU »** : Pinsker et une marge contrôlent une sortie ou une décision locale, pas le mIoU agrégé ni la vérité terrain.

## Conditions pour en faire un résultat central du papier

1. formaliser complètement l'énoncé avec masque diagonal, forêts et arbres multifurqués ;
2. vérifier par brute force sur petits arbres la projection, la sortie groupée et le cas d'inégalité stricte ;
3. mener une recherche d'antériorité ciblée, notamment HSA, Fast Multipole Attention, H-Transformer, MRA, HKT et projections KL/Bregman ;
4. démontrer sur SemanticKITTI que la relaxation côté requête réduit effectivement les erreurs de frontière ou augmente le mIoU par rapport à HSA au même arbre ;
5. montrer un Pareto utile entre distorsion, mIoU, $C_T$, VRAM et latence ;
6. obtenir une borne ou un estimateur calculable de $D_i^{\star}$ sans attention dense si le papier revendique un certificat pratique.

L'audit actuel conclut déjà que la projection QC-HSA seule est trop élémentaire et trop proche de Fast Multipole Attention pour porter le papier. Elle ne redeviendra centrale que si le pont HGP produit un certificat calculable, non vacu et validé, ou si un résultat fidélité–coût réellement nouveau est démontré. Sinon, elle reste une proposition technique et l'architecture est jugée empiriquement.
