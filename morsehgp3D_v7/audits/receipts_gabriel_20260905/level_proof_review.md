# Les niveaux Gabriel graduent les composantes réduites et leurs points

5 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

**Conclusion indépendante : la proposition du constructeur est correcte sur le domaine déclaré. Un plateau sans coface directe ne produit ni naissance réduite, ni multifusion, ni changement de couverture en points. Les niveaux Gabriel suffisent donc à graduer cet objet, à condition de conserver les rattachements nécessaires à sa construction.** E5 réfute le fold des seules cofaces Gabriel, pas la suffisance de leurs valeurs pour cette graduation. Aucune contradiction géométrique n'a été trouvée. La preuve ci-dessous traite le plateau entier, les coupes strictes et fermées, puis les boules éventuellement irrégulières hors fenêtre ; elle ne demande aucun Gamma exhaustif comme architecture.

## 1. Objet et autorités de lecture

Pour un nuage fini $X\subset\mathbb{R}^3$ de positions distinctes, une K-facette est un sous-ensemble de cardinal K et une coface Q a cardinal K+1. On écrit $\beta(Q)=\rho(Q)^2$. Une coface est Gabriel si aucun point de $X\setminus Q$ n'est strictement intérieur à sa miniball. Dans le domaine régulier pertinent, cela équivaut à $Q=X\cap B_Q$ ; c'est alors une coface directe du catalogue v7.

Pour K≥2, $\mathcal{C}_K^-(a)$ et $\mathcal{C}_K^+(a)$ désignent les composantes de Gamma élémentaire **non triviales**, aux coupes respectivement $\beta<a$ et $\beta\leq a$. Leurs sommets sont les facettes incidentes à au moins une coface active. Pour une composante C, U(C) est l'union de ses PointId. Une facette isolée n'est pas une racine de cet objet réduit. Les singletons normatifs K1 sont traités séparément au §5.

Les sources relues sont la [proposition constructeur](../../docs/AUDIT_NIVEAUX_GABRIEL_20260905.md), les dernières précisions de la [coordination](../../../audits/COORDINATION_MORSEHGP3D_V7.md), le [certificat horizontal E](../CERTIFICAT_HORIZONTAL_COURANT.md), la [composition horizontale](../../docs/PREUVE_HORIZONTALE_COMPOSITION.md), et les [attaches silencieuses transverses](../../../docs/math/INCIDENCES_SILENCIEUSES_GAMMA.md). Les SHA256 des octets lus sont dans [level_proof_review.json](level_proof_review.json).

Les pages PDF 110–117 du [manuscrit](../../../docs/references/MANUSCRIT_THESE_HAUSEUX.pdf) ont été relues directement : définitions 25–28, fait12 et proposition5, puis théorème4 aux PDF114–115 et proposition6 au PDF116. Le théorème4 fournit l'argument de remplacement strict. La proposition6 n'est pas prise comme prémisse : son passage de l'inertie locale à la suppression permanente des incidences est précisément celui que réfute E5.

## 2. Domaine régulier suffisant et remplacement strict

Fixons $2\leq K<n$. Supposons d'abord que chaque miniball pertinente possède pour coquille globale exactement son support minimal essentiel U : U est affinement indépendant, de cardinal s≥2, et son centre appartient à l'intérieur relatif de son enveloppe convexe. Tous les autres points de la boule sont strictement intérieurs. La position générale de la définition26 du manuscrit est une hypothèse globale suffisante ; l'extension moins forte utilisée par E est donnée au §6.

Un fait élémentaire justifie la stricte décroissance sans supposer qu'un intrus appartient déjà à une boule réduite. Soit B la miniball d'un support essentiel U, de niveau a et centre c. Tout ensemble H contenu dans B et dont les seuls points de frontière appartiennent à un sous-ensemble propre de U vérifie $\beta(H)<a$. Sinon B serait aussi sa miniball, par unicité à rayon minimal égal ; la condition d'optimalité placerait c dans l'enveloppe convexe de ce sous-ensemble propre, ce qui contredit l'essentialité. On peut donc retirer un essentiel et ajouter un ou plusieurs points strictement intérieurs en gardant une décroissance stricte.

Soit maintenant Q non-Gabriel de niveau a, U son support et z un intrus strict extérieur à Q. Pour u∈U, posons $F_u=Q\setminus\lbrace u\rbrace$ et $Q_u=F_u\cup\lbrace z\rbrace$. On a $\beta(Q_u)<a$, donc F_u est déjà incidente dans la coupe stricte. Pour u≠v, Q_u et Q_v partagent la K-facette $H_{uv}=(Q\setminus\lbrace u,v\rbrace)\cup\lbrace z\rbrace$. Toutes les F_u appartiennent donc à une même composante antérieure non triviale P(Q).

Ce sont exactement les facettes de Q de niveau strictement inférieur à a : supprimer un point intérieur laisse U et garde la même miniball. De plus, $\bigcup_{u\in U}F_u=Q$, car s≥2. Ainsi $Q\subseteq U(P(Q))$. Toute facette de Q déjà incidente avant a appartient à P(Q), et les autres facettes de Q ne portent aucun PointId absent de P(Q).

Cela ferme le lemme2 transverse coface par coface. Il reste indispensable de vérifier qu'une suite d'attaches simultanées ne relie pas deux anciens P différents.

## 3. Confluence exacte du plateau

Considérons deux cofaces distinctes Q=F∪{x} et Q'=F∪{y}, non-Gabriel, de même niveau a et partageant une facette F.

Si $\beta(F)<a$, F est l'une des facettes strictes de chacune ; elle est déjà incidente dans P(Q) et P(Q'), donc ces deux composantes antérieures sont égales.

Si $\beta(F)=a$, la boule de Q contient F et possède le même rayon minimal que F. L'unicité impose $B_F=B_Q=B_{Q'}$. Le support essentiel commun U est inclus dans F ; x et y sont strictement intérieurs. Pour u∈U, la coface $R=(F\setminus\lbrace u\rbrace)\cup\lbrace x,y\rbrace$ a cardinal K+1 et niveau strictement inférieur à a par le fait du §2. Elle relie les facettes strictes Q\{u} et Q'\{u}, donc à nouveau P(Q)=P(Q'). Ce raisonnement est celui de la confluence transverse §5.2, étape2 ; il ne suppose aucun ordre favorable dans le lot.

Contractons maintenant toutes les composantes de la coupe stricte avant de traiter le plateau. Chaque coface non directe touche exactement une ancienne racine P(Q). Deux telles cofaces connectées par une facette nouvelle ont le même P par la confluence ; si elles se rencontrent par une ancienne composante, leur P est déjà cette même composante. Par transitivité, tout groupe connexe composé uniquement de cofaces non directes a **exactement un parent pré-lot**. Tous ses points appartiennent déjà à ce parent, par le §2 appliqué à chaque coface du groupe.

Le schéma combinatoire « ancien parent A — facette nouvelle F — ancien parent B » montrerait pourquoi le seul lemme2 ne suffit pas : deux insertions individuellement à un parent pourraient créer ensemble une multifusion. La confluence prouve que ce schéma impose A=B dans le domaine géométrique considéré. Ce n'est donc ni un contre-exemple réalisable ici ni une justification pour séquentialiser les unions.

## 4. Théorème de graduation et lots mixtes

Soit $\mathcal{G}_K=\lbrace\beta(Q):Q\text{ est une coface Gabriel de cardinal }K+1\rbrace$. Si a n'appartient pas à cet ensemble, toutes les cofaces du plateau sont non directes. Le §3 montre que l'inclusion induit une bijection naturelle $i_a:\mathcal{C}_K^-(a)\longrightarrow\mathcal{C}_K^+(a)$, avec $U(i_a(C))=U(C)$. Les composantes non touchées persistent aussi par cette inclusion. Une nouvelle facette sans coface active reste isolée et n'entre pas dans cet objet réduit.

Il n'y a donc aucune naissance, aucune multifusion et aucun gain de point à publier à a. Par composition sur le nombre fini de valeurs intermédiaires, les composantes abstraites et leurs couvertures sont constantes, au sens de ces bijections canoniques, entre deux valeurs Gabriel consécutives. Cela inclut toute coupe ouverte ou fermée située dans cet intervalle ; aux valeurs Gabriel elles-mêmes, les états strict et fermé restent distingués.

La conclusion est une **suffisance**, pas l'affirmation que chaque valeur Gabriel est indispensable. Une valeur Gabriel qui ne porte elle-même qu'une continuation sans point nouveau peut aussi être contractée dans l'objet annoncé. Inversement, si une naissance, une multifusion ou un gain de point existe, sa date exacte appartient à l'ensemble des valeurs Gabriel. Omettre cette date changerait les coupes de l'objet demandé.

Le raisonnement traite aussi les lots mixtes. Une coface directe D et une coface non directe Q distincte de même niveau ne peuvent partager une facette F avec $\beta(F)=a$ : les miniballs coïncideraient ; leur coquille est le même support inclus dans F, et le point de Q\D serait un intrus strict de D. Tout contact direct–silencieux passe donc par une facette stricte appartenant à l'apex du groupe silencieux. Les morceaux directs que relie ce groupe touchent déjà le même parent pré-lot. Après que ces rattachements sont connus, le groupe silencieux n'ajoute ni relation entre parents, ni point au quotient atomique du lot direct.

Tous les parents doivent être lus dans la coupe strictement antérieure, puis les directes de même niveau contractées ensemble. Une facette latente de niveau inférieur à a, mais dont la première incidence vaut a, n'est pas un parent ancien. La trier plus tôt dans un dictionnaire ne la rend pas incidente. Cette distinction empêche les fausses naissances et fusions binaires créées par un ordre d'insertion artificiel.

## 5. K1 et interprétation de l'arbre réduit

À K1, les PointId sont les racines initiales normatives, avec le contrat explicite de coupe à zéro. Pour une paire non-Gabriel {x,y}, un intrus strict z dans sa boule diamétrale vérifie $\beta(\lbrace x,z\rbrace)<\beta(\lbrace x,y\rbrace)$ et $\beta(\lbrace z,y\rbrace)<\beta(\lbrace x,y\rbrace)$. Ses deux extrémités sont donc déjà connectées. Supprimer ce niveau comme modification de composantes est correct ; les singletons initiaux restent conservés. Le graphe Gabriel contenant un EMST donne le même single-linkage.

Pour K≥2, les feuilles abstraites du journal réduit ne sont pas toutes les K-facettes isolées de Gamma. Le journal suffisant pour les composantes et leurs couvertures garde les identités distinctes, les niveaux et parents des naissances/multifusions, et les points ajoutés par les continuations utiles. Une continuation à un parent et sans point nouveau conserve l'identité abstraite. La mise à jour par union des couvertures parentales et du delta reconstruit U à toute coupe, par induction sur les lots.

Une égalité ou une intersection des ensembles U(C) n'est jamais un critère de fusion. Le quotient conserve les identités de composantes et leurs flèches d'inclusion, même lorsque leurs couvertures se recouvrent. Il ne promet pas les anciennes clés publiques ou les batch_ids d'un autre payload.

## 6. Extension précise au domaine de fenêtre accepté par E

La régularité globale du §2 n'est pas nécessaire pour appliquer cette conclusion au domaine du certificat horizontal E. Reprenons son autorité S : pour toute miniball B représentée par un support positif de cardinal minimal s, si $p+s\leq r_{\max}$, son census global fermé est complet et sa coquille est exactement ce support ; p est le nombre global de points strictement intérieurs. Les boules au-dessus de la fenêtre peuvent être irrégulières. Cette autorité est une fermeture mathématique du domaine, pas l'absence de diagnostic sur les seuls candidats arbitrairement reçus.

Fixons K demandé, donc K+1≤rmax, et une coface Q non-Gabriel de niveau a, de miniball B. Si p+s≤rmax, la boule est régulière : Q contient s points de support et K+1−s points intérieurs ; l'intrus strict ajoute au moins un autre intérieur, donc p+s≥K+2. Si p+s>rmax, on a aussi p+s≥K+2. **Dans les deux cas, K≤p+s−2.**

Le [théorème transverse4.2](../../../docs/math/INCIDENCES_SILENCIEUSES_GAMMA.md#531-inertie-h_0-exacte-des-blocs-saturés-au-dessus-de-la-fenêtre-de-rang) s'applique alors au saturé $S_B=X\cap B$, même irrégulier. Ses K-facettes de niveau strictement inférieur à a forment déjà un graphe connexe couvrant tous les points de S_B. Pour K≥2, la couverture de S_B, de cardinal au moins K+2, impose plusieurs facettes dans ce graphe connexe ; son apex est donc non trivial. L'activation de son bloc fermé ne crée ni composante, ni fusion, ni point.

Cette présentation par blocs ferme également les contacts simultanés hors fenêtre. Deux blocs qui partagent une K-facette stricte possèdent le même apex antérieur. Si la facette commune a niveau a, l'unicité de sa miniball impose que les deux boules soient la même ; on ne peut ainsi relier deux apex distincts par une nouvelle facette de plateau. Ce raisonnement vaut pour toutes les cofaces non directes du plateau, sans supposer que leurs facettes ont été matérialisées par le candidat.

Enfin, une coface Gabriel D contient tous les intérieurs de sa boule et un support minimal, donc p+s≤|D|=K+1≤rmax. Dans le domaine accepté, elle est régulière et son saturé fermé est exactement D. Le catalogue direct de l'autorité S est donc bien l'ensemble Gabriel utilisé dans le théorème. Un contact égal avec un bloc inertiel distinct imposerait la même boule, incompatible avec les deux cardinalités de saturé.

Ainsi, la graduation est également fermée sur le domaine accepté E. Les contrôles de cœur et de chaîne du certificat horizontal raccordent séparément son sous-flot matériel aux composantes de référence. Un futur producteur de portails doit établir l'autorité qu'il consomme ; il n'hérite pas de contrôles locaux qu'il aurait cessé d'exécuter. Cela ne demande ni régularité globale supplémentaire, ni catalogue Gamma.

## 7. Portail avant une première consommation directe

Soit F une K-facette régulière réutilisée par une coface directe D au niveau a. Notons $J_F=(X\cap B_F^{\circ})\setminus F$ et $\lambda(F)=\min_{x\notin F}\beta(F\cup\lbrace x\rbrace)$. La distinction entre naissance géométrique β(F) et première incidence λ(F) est essentielle.

Si |J_F|=0, alors λ(F)>β(F), et tout minimiseur est direct : un minimiseur non direct devrait avoir son nouveau point essentiel ; le remplacer par un intrus diminuerait le niveau tout en conservant F, contradiction. Si |J_F|=1, l'unique extension de niveau β(F) est directe. Si |J_F|≥2, les premières extensions ont niveau λ(F)=β(F), sont non directes et confluent vers un même apex strict ; un intrus reste extérieur à chacune.

Dans ce dernier cas, **β(F)<a**. En effet, l'égalité imposerait $B_F=B_D$ ; puisque D=F∪{x}, D ne peut absorber qu'un des deux intrus distincts de F. L'autre contredirait le caractère Gabriel de D. Une attache silencieuse utile à D est donc strictement antérieure à son lot, et non un parent fabriqué dans ce lot.

Une conséquence constructive est disponible sous un invariant précis de table : toutes les utilisations directes strictement antérieures à a ont déjà été traitées. À la première consommation directe d'une F encore inconnue, les cas |J_F|=0 ou1 ne peuvent cacher un minimiseur direct de niveau <a : ce minimiseur aurait déjà consommé F. Ils sont donc latents jusqu'au lot ou incidents dans ce même lot. Le cas |J_F|≥2 demande au contraire un portail vers son apex ancien. Ce critère ne s'applique pas si la table a oublié une utilisation directe antérieure sans conserver d'alias.

En particulier, une directe qui ne produit qu'une continuation sans point doit quand même enregistrer toutes ses facettes dans cette table de construction. La table des parents reste figée à a− : les clés découvertes dans le lot a sont des sommets temporaires du DSU atomique, pas de nouveaux parents stricts. Les deltas de points se calculent pour chaque groupe par rapport à l'union de ses propres parents, jamais par rapport à un ensemble global de points déjà vus dans d'autres composantes.

Un certificat de descente choisit une première extension non directe, remplace un essentiel par un intrus et conserve un suffixe strict jusqu'à un terminal direct, sous les contrôles réguliers de chaque maillon. La stricte décroissance et la finitude des cofaces prouvent la terminaison mathématique sans les énumérer. Elle ne borne pas le coût d'une recherche ou d'un certificat dans un budget donné. Un cache doit être lié à un terminal validé ; l'identifiant de ce terminal est ensuite normalisé dans la composante courante à a−. L'ancre ancienne seule n'est pas un parent valide après une fusion intermédiaire sans ce transport.

Dans le domaine E autorisant des boules irrégulières hors fenêtre, un changement de choix d'intrus ou d'essentiel peut visiter une MEB absente de l'ancienne descente contrôlée. Le nouveau chemin doit conserver son contrôle et son refus éventuel, ou réutiliser un suffixe déjà certifié. L'inertie de son bloc prouve la graduation mais ne rend pas ce chemin particulier régulier par héritage.

Ce report n'autorise pas une activation anticipée de F dans un contrat de membership : avant λ(F), F peut être isolée. Pour le seul objet réduit avec points, l'apex couvre déjà F ; le portail sert à prendre la bonne décision lors de sa consommation future. Il peut être conservé comme une ancre certifiée, sans republier toutes les cofaces du chemin ou leur niveau comme des événements de sortie.

## 8. Portée d'E5 et informations non conservées

Au niveau33/2, E5 ajoute AC par ACD/ACE, alors que la composante antérieure couvre déjà ACDE. L'intervalle jusqu'à la directe ABC ne voit aucun changement de composante abstraite ou de points dû à cette attache. La contraction de cet événement dans le journal annoncé est donc exacte. Effacer aussi le rattachement de AC créerait en revanche une fausse naissance à la directe ABC, puis une fausse fusion à BCE : ce sont les deux conclusions distinctes que la fixture sépare.

Les dates omises ne sont pas inutiles pour tout objet. Elles peuvent modifier le membership des facettes, leurs incidences, des carriers marqués, ou une masse de feuilles affectée à la première incidence. La graduation Gabriel ne suffit pas à ces contrats sans information additionnelle. Les poids du catalogue Gabriel du manuscrit n'obligent pas à ajouter les cofaces de rattachement comme contributions ; la date d'affectation d'une feuille pondérée reste une question distincte.

Pour les applications entre ordres, les bijections horizontales et la naturalité permettent une graduation commune par l'union des valeurs Gabriel des ordres retenus, avec des ancres verticales certifiées et transportées aux mêmes coupes. Ce n'est ni une carte déduite du recouvrement des points ni une intégration verticale déjà faite par cette note.

La conclusion ferme la question mathématique de graduation et de suffisance du journal réduit annoncé. Restent à qualifier le producteur concret des portails, son autorité terminale, ses lots atomiques, ses contrôles et budgets, puis son contrat de payload. Aucun gain de temps ou de mémoire, aucun nouveau résultat produit et aucun statut public ne sont déduits de la petitesse possible du journal. Aucun C++, Git ou GCP utilisé par cette sous-tâche.
