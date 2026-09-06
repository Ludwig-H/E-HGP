# Les seuls sommets Gabriel et la vraie hiérarchie K-NN

6 septembre 2026. Question explicite de l’utilisateur, précisée par « Mon but est la vraie hiérarchie k-NN ». Cadre : `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`. Preuve mathématique et fixture rationnelle bornée ; aucune modification, compilation ou exécution du moteur C++, aucun benchmark, GCP non utilisé.

**Les minima Gabriel peuvent être les seuls sommets d’une représentation exacte de cette hiérarchie, mais les connexions doivent transporter les chemins omis. La restriction aux seuls minima avec les adjacences géométriques initiales est fausse.** Une descente de facettes de cardinal K vers ces minima donne une piste constructive pour calculer les bons parents sans conserver les autres facettes comme sommets permanents.

## 1. L’objet à conserver exactement

La cible est la filtration des composantes connexes de $L_K(r)=\left\lbrace y\in\mathbb{R}^{3}:\lvert B(y,r)\cap X\rvert\ge K\right\rbrace$, avec leurs naissances, fusions, identités et couvertures de points. Ce n’est pas la hiérarchie d’un graphe ordinaire de voisins. Le manuscrit, définitions 21–22 et théorème 2, PDF 84–87, identifie cet objet aux composantes de Γ et aux K-polyèdres. Les [définitions capturées](../receipts_gabriel_20260905/full_certificate_manuscript_pdf84_85.txt) fixent ce raccord.

Précision de vocabulaire : Γ a initialement **tous** les labels de Čech de cardinal K pour sommets ; deux labels F et G sont adjacents quand leur union est active. Le graphe dont les sommets sont les facettes des cofaces Gabriel est celui de la définition 29, PDF 115. La proposition 5, PDF 112, permet les seules adjacences élémentaires de cardinal K+1 dans Γ complet ; elle ne prouve pas que cette réduction reste vraie après suppression de sommets. Gabriel signifie ici la vacuité de l’intérieur de la **miniball**, définition 28, PDF 113.

Écrivons β(F) pour le rayon carré de la MEB. La région témoin $T_r(F)=\bigcap_{x\in F}B(x,r)$ est convexe, et $L_K(r)=\bigcup_{\lvert F\rvert=K}T_r(F)$. L’adjacence initiale équivaut à $T_r(F)\cap T_r(G)\ne\varnothing$, soit $\beta(F\cup G)\le r^2$. Supprimer les régions témoins des labels non Gabriel peut couper cette union. L’objectif de la représentation comprimée est l’égalité de la hiérarchie de composantes et de ses couvertures, pas l’égalité géométrique de cette union avec les seules régions témoins des minima.

Les preuves suivantes supposent la régularité déclarée : support essentiel unique, autres points du label strictement intérieurs, aucun point étranger sur la coquille. La restriction naïve échoue déjà dans ce domaine. Les plateaux non réguliers ne reçoivent pas implicitement cette réduction.

## 2. Contre-exemple u16 à quatre points en dimension trois

Prendre $A=(0,3,0)$, $B=(4,9,0)$, $C=(8,3,0)$ et $D=(4,0,1)$. Le déterminant affine a pour valeur absolue 48. Tous les certificats MEB sont réguliers. Les seuls minima de cardinal deux sont AD et CD, de niveau 13/2, puis AB et BC, de niveau 13.

| Label | Centre de la MEB | β | Support et vacuité |
| --- | --- | --- | --- |
| AC, ACD | (4,3,0) | 16 | Support AC ; D intérieur ; B extérieur. AC n’est pas Gabriel, ACD l’est. |
| ABC | (4,14/3,0) | 169/9 | Support ABC essentiel ; D a une puissance extérieure 4. ABC est Gabriel. |
| BD, ABD, BCD, ABCD | (4,9/2,1/2) | 41/2 | Support BD ; A et C strictement intérieurs, de puissance −2. |

À 16, ACD rattache AC et fusionne AD avec CD. À 169/9, ABC joint AB, BC et **AC**, donc fusionne trois anciennes composantes en une. La vraie hiérarchie K-NN d’ordre deux ne possède alors qu’une composante, couvrant ABCD.

Sur les seuls minima, ABC relie AB à BC, et ACD relie AD à CD. Tout lien entre ces deux groupes est trop tardif, **même en autorisant toutes les unions de la définition 21** : un label du premier groupe contient B, un label du second contient D. Leur union contient BD, donc sa MEB a un rayon carré au moins 41/2. Ce seuil est atteint par ABD ou BCD. Le graphe induit a donc encore deux composantes pendant tout l’intervalle $169/9\le r^2<41/2$, au lieu d’une. Le retard vaut 31/18 en rayon carré.

Ce défaut n’est ni un plateau exclu ni une approximation numérique. Les deux groupes couvrent respectivement ABC et ACD : leur recouvrement ne fournit pas une règle d’adjacence de remplacement. Par exemple, les minima AB et BC partagent B dès leur naissance à 13, mais ne doivent pas être fusionnés à cette date.

Le [vérificateur fixe](counterfixture.py) vérifie les certificats et les coupes, sans rechercher de MEB ni appeler un helper produit. Ses reçus [normal](normal.json) et [optimisé](optimized.json) conservent ce négatif et la réparation. La [fixture J=1 antérieure](../receipts_full_producer_20260905/lazy_alias_next_step_review.md#4-fixture-j1-réalisable-et-rejets-proposés) porte le même mécanisme dans le plan ; E5 ajoute une descente silencieuse. Aucune qualification moteur antérieure n’est transférée à ce nouveau nuage.

Les 15 certificats, 19 coupes et six variantes de descente passent en Python normal et `-O`, avec des reçus identiques. Quatre points sont minimaux dans le domaine régulier : pour n≤3, seuls K=1, K=n et n=3/K=2 sont possibles. Dans ce dernier cas, une arête non Gabriel naît avec l’unique triangle, qui relie directement tous les minima ; sa suppression ne peut retarder leur fusion.

## 3. Un graphe exact sur les seuls minima

Soit $\mathcal{M}_K$ l’ensemble des labels Gabriel de cardinal K, chacun activé à β(M). Pour deux minima distincts, définir $\mu(M,N)=\min_{P:M\leadsto N\ \mathrm{dans}\ \Gamma_K}\max_{Q\ \mathrm{connexion\ de}\ P}\beta(Q)$. Les chemins élémentaires suffisent ; les niveaux de leurs connexions dominent ceux de leurs sommets. Ce seuil est le premier niveau carré auquel M et N sont dans la même composante Γ.

Le graphe pondéré sur ces minima, avec poids μ et dates de naissance β, reproduit exactement les composantes à chaque coupe ouverte ou fermée. **En général μ(M,N) est plus petit que β(M∪N).** Dans la fixture, μ(AB,AD)=169/9 alors que β(ABD)=41/2. Un lien comprimé n’est donc pas une nouvelle coface géométrique née au poids affiché : il certifie un chemin existant au bon seuil.

Il n’est pas nécessaire de stocker le graphe complet. Pour chaque véritable multifusion à p parents, choisir un minimum dans chaque ancienne composante et relier ces p représentants par p−1 liens au niveau de la fusion. Toutes les opérations d’un niveau se lisent sur l’état strictement antérieur et se ferment atomiquement. Pour L minima et R composantes finales du préfixe, le résultat est une forêt à **L−R liens**. Son rejeu restitue les multifusions, sans imposer d’ordre binaire à l’intérieur du plateau. Dans la fixture, les trois liens AD–CD à 16, AB–BC et AD–AB à 169/9 suffisent. Les p−1 liens ne permettent pas d’ignorer un bras essentiel : ABC a trois parents distincts, dont chacun doit être retrouvé.

La preuve que toute composante contient un minimum et que ses points sont exactement couverts par ses minima est donnée par la descente suivante. L’association d’une composante Γ à ses minima commute avec les inclusions entre coupes : elle préserve ainsi la vraie hiérarchie K-NN, pas seulement le nombre de composantes d’une coupe. Elle ne reconstruit pas la liste exhaustive de ses facettes, ses adjacences ou le contour de sa région continue.

Cette existence ne calcule pas gratuitement μ. Définir le poids comme le seuil recherché, puis lancer Kruskal, déplacerait toute la difficulté dans la production de ce poids. Le procédé suivant fournit une autorité constructive pour les parents des cofaces de fusion déjà découvertes.

## 4. Descendre une facette directement vers un minimum

Soit F de cardinal K, non Gabriel. Choisir un intrus strict z et un sommet essentiel s de sa MEB, puis poser $F'=(F\setminus\lbrace s\rbrace)\cup\lbrace z\rbrace$. Alors $\beta(F')<\beta(F)$, tandis que $\beta(F\cup\lbrace z\rbrace)=\beta(F)$. La première propriété vient de l’essentialité : les points restés sur la frontière sont un sous-ensemble propre du support, dont l’enveloppe convexe ne contient plus le centre. La seconde vient de F⊆F+z⊆B(F) et de l’unicité de la MEB.

F et F′ sont donc adjacentes dans Γ au niveau β(F), et F′ est strictement antérieure. Répéter termine sur un minimum Gabriel : il n’existe qu’un nombre fini de labels de cardinal K et β diminue strictement. Pour une requête de parent β(F)<a, tout le chemin reste dans Γ à la coupe stricte a. Le minimum terminal, normalisé dans l’histoire des fusions déjà traitées, donne le bon parent. Des choix différents peuvent atteindre des minima différents ; **leur composante normalisée à a est la même**, puisque leurs chemins passent par F avant a. On ne réclame pas l’unicité du minimum terminal.

Pour conserver un point donné x∈F, choisir à chaque pas un essentiel s différent de x. Il en existe toujours un puisque le support a au moins deux points. La descente conserve alors x et termine sur un minimum le contenant, dans la même composante. Ainsi chaque point de chaque facette active est porté par un minimum de sa composante. L’union des labels des minima restitue exactement sa couverture ponctuelle. Cette preuve vaut aux deux côtés d’une coupe : β(F) est admis si F l’est.

À K=1, tous les points sont déjà des minima et les cofaces sont les arêtes ; on retrouve le Single-Linkage. À K=n, X est le seul minimum et aucune connexion n’est requise. Sans régularité, une facette Gabriel faible peut naître avec une coface, comme le [contre-exemple de coquille antérieur](../receipts_gabriel_20260905/full_proof_review.md#8-fixture-minimale-régulière-et-frontière-exclue) : il faut une convention de plateau et une preuve distinctes.

## 5. Portée concrète pour Morse HGP 3D

La [v7 courante](../../src/forest/full_gabriel.hpp) publie déjà les minima et les véritables multifusions. `lot()` ne demande que les q≤4 facettes strictes obtenues en retirant les essentiels de chaque coface Gabriel. `locate()` résout les facettes non minimales par alias ou descente de cofaces, puis `direct_anchor()` normalise une ancre historique. Le mode lazy n’installe plus les K+1 alias de chaque directe ; son cache facultatif peut être nul. La nouvelle formulation justifie cette économie, sans autoriser à supprimer `locate()`.

**Piste nouvelle à qualifier : remplacer ce resolver par une descente de facettes de cardinal K.** Catalogue complet des minima et successeurs donnent alors l’autorité de terminaison ; les ancres par coface directe ne sont plus mathématiquement nécessaires à ce resolver horizontal. Les cofaces Gabriel de cardinal K+1 restent nécessaires pour découvrir et dater les fusions. Le gain possible porte sur les ancres stockées et sur des MEB de cardinal K plutôt que K+1 ; ni la longueur de descente ni un gain de temps ne sont bornés ici. Un choix déterministe suffit, sans exiger un minimum terminal canonique.

Cette piste change le calendrier des appels et peut visiter d’autres boules. Les contrôles exacts de support, coquille, intrus, terminaison et catalogue restent requis ; une autorité limitée à une fenêtre de rang ne certifie pas automatiquement la régularité de toutes ces nouvelles visites. La fixture bornée ci-dessus ne qualifie pas le domaine, les refus ou les performances d’un nouveau resolver C++. L’ancienne qualification et ses budgets restent inchangés.

La tour inter-K exige encore ses ancres dans l’ordre inférieur après fermeture du plateau ; les enlever au resolver horizontal ne les supprime pas du contrat vertical. Les poids et le vote du §9.1 portent sur toutes les facettes contributrices, et ne sont pas remplacés par des poids uniformes sur les minima. Ces suppléments ne changent pas la cible primaire démontrée ici : la hiérarchie exacte des composantes K-NN avec leurs couvertures.

## 6. Reconstruire toute la tour jusqu’à Kmax

La précision ultérieure de l’utilisateur porte sur une méthode plus simple pour **toutes** les hiérarchies 1..Kmax. Le catalogue Gabriel global jusqu’au rang min(n,Kmax+1) possède déjà le bon partage mathématique : un label de cardinal m est un minimum à l’ordre m et une connexion à l’ordre m−1, avec une seule géométrie et un seul niveau. Même sans fusion à m−1, sa naissance reste obligatoire si m≤Kmax. Le rang Kmax+1 reste nécessaire pour les dernières fusions demandées, sans obligation de construire sa hiérarchie ; lui seul peut bénéficier d’une recherche limitée aux connexions utiles sans perdre une naissance demandée. Une telle recherche exigerait un oracle de coupe complet, non fourni ici. Le cas terminal K=n émet X sans coface supérieure.

Un algorithme commun aux ordres peut traiter chaque niveau exact en trois étapes :

1. Pour chaque connexion de rang m, résoudre ses q≤4 facettes strictes dans l’état **antérieur** de l’ordre m−1, par descente de cardinal constant jusqu’aux minima ; grouper les parents distincts de toutes les connexions de cet ordre et de ce niveau.
2. Installer les minima de rang m dans l’ordre m, puis fermer atomiquement les véritables multifusions de chaque ordre. Les minima ne sont pas des parents stricts d’une connexion du même niveau.
3. Si la tour verticale est demandée, rattacher chaque nouveau minimum de rang m à la composante **fermée** de sa connexion à l’ordre m−1. Propager ensuite cette ancre par les successeurs inférieurs.

Chaque ordre conserve ses propres identités de composantes. Pour une coface Q de rang m≥3, ses facettes strictes $F_u=Q\setminus\lbrace u\rbrace$ et $F_v=Q\setminus\lbrace v\rbrace$ partagent $H_{uv}=Q\setminus\lbrace u,v\rbrace$. Dans l’ordre m−2, elles sont déjà des cofaces actives avant β(Q), et leurs images contiennent le même Huv. **Tous les parents de cette fusion supérieure ont donc déjà la même image dans l’ordre inférieur.** Cette image commune ne peut remplacer leur résolution à l’ordre m−1. Dans la fixture, les trois parents K2 d’ABC ont la même image K1 depuis le niveau 13, mais restent distincts jusqu’à 169/9.

Le coût topologique de demandes initiales est au plus $4\sum_{k=1}^{Kmax}D_k$, où Dk compte les cofaces Gabriel de cardinal k+1. Si P demandes ne sont pas trouvées directement dans la table des minima et si leur descente totalise S pas, une variante qui cherche le minimum **avant** de calculer sa MEB effectue S appels MEB non terminaux, avec S≥P. Recalculer aussi chaque terminal donnerait P+S appels. Ce calendrier concerne le nouveau resolver ; les census, recherches, normalisations et éventuels recalculs après éviction restent à compter. Ni S ni le volume des catalogues ne sont bornés par n dans cette preuve.

La v7 partage déjà index, génération, census et géométrie des catalogues entre les ordres. La sonde réemploie même le catalogue des connexions comme catalogue des minima de l’ordre suivant. Un flux global ne supprime donc pas K générations existantes : elles ne sont pas répétées. Il peut permettre de partager les index/ordres de tri des catalogues et de construire les ancres verticales pendant la fermeture des lots, au prix de plusieurs états topologiques vivants. Pour la seule collection de forêts horizontales, le traitement séquentiel conserve un intérêt mémoire.

La nouveauté concrète est la descente vers les seuls minima, avec suppression possible des ancres directes comme autorité horizontale, puis sa composition avec ce partage déjà existant. Le [prototype borné de tour](tower_minima_descent.py) en vérifie la construction sur les certificats fixes de la fixture ; ses résultats restent un modèle d’audit, pas une architecture exhaustive à copier ni un benchmark industriel. La découverte rapide et complète du catalogue géométrique demeure un chantier distinct de son rejeu topologique.

Ce prototype reconstruit K1..4 sur six lots globaux. Il passe 76 comparaisons Γ, 70 inclusions horizontales, 45 images verticales et 42 carrés de naturalité. Ses sept ancres verticales sont des sorties, jamais des terminaux du resolver. AC peut descendre vers CD ou vers AD : les tours obtenues sont identiques, après normalisation dans la même composante. Les [reçus normal](tower_normal.json) et [optimisé](tower_optimized.json) sont identiques, codes 0. Les 15 demandes de parents comprennent une descente non vide ; aucune statistique de MEB physique n’est déduite des consultations de certificats fixes.

Deux autres économies sont séparables. Les labels et index de tri d’un catalogue peuvent être préparés une fois pour ses deux rôles, sans partager les tokens d’ordres différents. Pour une demande stricte $Q\setminus\lbrace u\rbrace$, la MEB du petit support $U\setminus\lbrace u\rbrace$ est une proposition : si elle contient toute la facette, l’inclusion et l’optimalité prouvent qu’elle en est la MEB exacte. Pour q=3 c’est une paire ; pour q=4, une MEB de trois points, éventuellement de support deux. Pour q=2 et K>1, la proposition ponctuelle ne peut contenir la facette : la sauter. Cette économie de solveur ne supprime pas le census global : dans la fixture, D est extérieur à B(ABC), de puissance +4, mais intérieur à B(AC), de puissance −6. **Les boules de facettes ne sont pas nécessairement contenues dans la boule de leur coface.** La réutilisation du seul census supérieur ne serait donc pas exacte.

Le [relevé local daté](baseline_read.json) situe les priorités sans exécuter de benchmark : dans l’unique capture constructeur n=8 000/s8/P0 close, les 159,160 s avant terminal comprennent 61,807 s de génération, 17,540 s de préfiltre+census et 73,798 s de construction FULL. Celle-ci rapporte 4 305 891 appels MEB et 503 231 458 supports de référence. Les postes internes MEB ne sont pas chronométrés séparément, et aucun résultat apparié P>0 n’est déduit. Il faut donc travailler à la fois la découverte géométrique et la reconstruction ; changer seulement le resolver laisserait le premier coût. Le cache strict ne sature pas dans ce témoin (`cache_skips=0`) : l’agrandir ne supprimerait aucun miss par capacité. Cette lecture épingle sept fichiers locaux, sans qualification indépendante de leur moteur ou de leurs catalogues.

Les deux programmes Python s’exécutent depuis la racine avec `python3 -B morsehgp3D_v7/audits/receipts_gabriel_vertices_20260906/counterfixture.py` et `python3 -B morsehgp3D_v7/audits/receipts_gabriel_vertices_20260906/tower_minima_descent.py`, puis avec l’option `-O` ajoutée. Ils écrivent uniquement leur JSON sur stdout ; aucune porte ne repose sur `assert`.
