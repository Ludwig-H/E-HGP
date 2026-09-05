# Contrat vertical courant : descendre en ordre à coupe fixée

5 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`. Travail mathématique et petits rejeux sous `audits/` ; aucun moteur exécuté pendant la fenêtre de chronométrage E/F.

**La verticale est constructible depuis les seuls deltas horizontaux E qualifiés : scanner `born` à chaque vraie naissance fournit une ancre inférieure, puis `parents` et les successeurs inférieurs propagent les cartes.** La preuve ci-dessous établit la totalité de cette construction, sa naturalité et son accord avec Gamma, sans MEB supplémentaire ni resolver géométrique général. Le produit ne livre pas encore cette reconstruction ou son export. Une facette absente de la table inférieure ne signifie jamais que la composante source n'a pas d'image.

L'autorité horizontale est le [certificat CPU E](CERTIFICAT_HORIZONTAL_COURANT.md), aux octets E épinglés dans `61f72a6805e27f1bc216b5d7444164b31fc970b6`. La variante F publiée dans `71895104` garde sa qualification distincte. Cette note ne rouvre aucune obligation horizontale close.

## 1. Sources et sens des deux paramètres

Les passages relus du [manuscrit](../../docs/references/MANUSCRIT_THESE_HAUSEUX.pdf) sont les définitions 6–8, pages PDF 44–47, pour la densité et la couverture discrète ; les définitions 20–22 et le théorème 2, pages PDF 83–87, pour Čech, Gamma et les régions témoins ; la définition 25 et la proposition 5, pages PDF 110–112, pour les rayons et les adjacences élémentaires. Le hash du manuscrit reste celui du certificat horizontal. La présente preuve verticale utilise ces définitions et la proposition 5 ; elle n'utilise pas la suppression Gabriel brute réfutée.

La [spécification transverse, §§3–4](../../docs/SPECIFICATION_MORSEHGP3D.md) définit la même tour. Les [incidences silencieuses, §6](../../docs/math/INCIDENCES_SILENCIEUSES_GAMMA.md#6-niveaux-égaux-verticales-et-dégénérescences) en donnent déjà le principe combinatoire. Le [certificat local historique, §6.1](../../docs/math/INCIDENCES_SILENCIEUSES_GAMMA.md#61-carré-vertical-normalisé-local) et la [tour directe de Phase 11](../../docs/math/TOUR_VERTICALE_DIRECTE_PHASE11.md) décrivent des composants d'une autre lignée ; leurs reçus ne sont pas hérités par la v7.

Pour un ensemble fini X de points distincts, on utilise le niveau carré exact $a=r^2$ et $\beta(S)=\rho(S)^2$. Les régions de multicoverture sont :

$$L_k(a)=\left\lbrace y\in\mathbb{R}^{3}:\#\left\lbrace x\in X:\lVert x-y\rVert^2\leq a\right\rbrace\geq k\right\rbrace.$$

Les deux inclusions sont $L_k(a)\subseteq L_k(b)$ lorsque $a\leq b$, et $L_{\ell}(a)\subseteq L_k(a)$ lorsque $k<\ell$. La flèche verticale descend donc de l'ordre supérieur vers l'ordre inférieur **au même rayon géométrique**. Les flèches horizontales vont vers les rayons croissants. Il n'existe pas d'inverse vertical général.

Le seuil de densité normalisée du manuscrit vaut $\lambda_k=k/(n\omega_3 r^3)$. Garder une même valeur numérique de $\lambda$ en changeant k change le rayon ; ce n'est pas la coupe verticale ci-dessus. Une autre paramétrisation doit enregistrer ce changement de variable.

Une coupe c désigne un niveau et un côté : $a^-$ signifie $\beta<a$, $a^+$ signifie $\beta\leq a$. À niveau identique, $a^-\preceq a^+$ ; à niveaux différents, les deux côtés de a précèdent les deux côtés de b si $a<b$. Pour le raccord à E, on considère les coupes positives et l'état initial fermé $0^+$ : cela conserve les racines ponctuelles normatives sans ajouter de convention de coupe ouverte avant zéro.

## 2. Définition sur Gamma et preuve de fonctionnalité

À une coupe c, les sommets de $\Gamma_k(c)$ sont les ensembles de cardinal k actifs à cette coupe. Les arêtes élémentaires sont portées par les ensembles actifs de cardinal k+1. Le passage du graphe d'intersection complet au graphe élémentaire conserve les composantes par la proposition 5 ; leurs adjacences ne sont pas identifiées.

Soit C une composante de $\Gamma_{k+1}(c)$. Chacun de ses labels S, de cardinal k+1, devient une coface active de $\Gamma_k(c)$. Posons :

$$\partial_k C=\left\lbrace F:\lvert F\rvert=k,\ F\subset S\ \text{pour un }S\in C\right\rbrace.$$

**Lemme de cible unique.** Toutes les facettes de $\partial_k C$ appartiennent à une même composante de $\Gamma_k(c)$.

Pour un label S fixé, ses k-faces forment une clique de l'ordre inférieur, puisque leur union est S et que $\beta(F)\leq\beta(S)$. Deux labels source adjacents S et T ont une intersection de cardinal k. Leurs cliques inférieures partagent donc ce sommet. Un chemin dans C relie ainsi toutes ces cliques. Cet argument garde le même côté de coupe : une inégalité stricte reste stricte après passage à un sous-ensemble. Il ne demande aucune régularité géométrique.

On définit alors :

$$v_{k+1\to k,c}(C)=\text{l'unique composante de }\Gamma_k(c)\text{ contenant }\partial_k C.$$

L'application est fonctionnelle et totale sur les composantes source. Elle ne dépend ni du label S choisi, ni de la k-face choisie dans S, ni d'un chemin ou d'un ordre de parcours.

Pour le modèle continu fermé, la région témoin $T_r(S)=\bigcap_{x\in S}B(x,r)$ est incluse dans $T_r(F)$ dès que $F\subset S$. La correspondance du théorème 2 transporte donc exactement cette application vers celle induite par $L_{k+1}(a)\subseteq L_k(a)$. On conserve l'identité de la composante de témoins ou de facettes ; on ne la remplace pas par un ensemble de points supposé identifier cette composante.

## 3. Réduction non triviale, K1, couverture et naturalité

Notons $\mathcal C_k(c)$ les composantes non triviales de Gamma pour $k\geq2$, et toutes les composantes pour k=1. Ce domaine est fermé sous les inclusions horizontales : une composante déjà incidente à une coface ne redevient pas isolée.

La cible verticale est elle aussi dans ce domaine. Un seul label S source, même isolé dans Gamma complet, engendre k+1 facettes distinctes reliées en bas. Pour $k\geq2$, sa cible est donc non triviale ; pour k=1, sa cible est une composante ordinaire contenant ses points. Restreindre la source aux composantes non triviales ne rend aucune flèche partielle.

À K1, les PointId constituent les racines initiales et le niveau d'une paire est $\lVert x-y\rVert^2/4$. La cible de K2 vers K1 peut être retrouvée depuis **n'importe quel PointId couvert par la composante source**, car K1 partitionne les points. Cette simplification ne s'étend pas aux cibles d'ordre au moins deux.

L'ordre terminal k=n doit être distingué selon le profil : Gamma complet possède le label X à partir de $\beta(X)$, tandis que sa réduction non triviale est vide. E calcule jusqu'à $K_{\mathrm{eff}}=\min(s_{\max},n)-1$ ; le présent raccord produit ne demande des cartes qu'entre ordres disponibles, soit $1\leq k<K_{\mathrm{eff}}$. Il ne prétend pas exporter le terminal de la tour complète transverse. Si $K_{\mathrm{eff}}=1$, la famille verticale demandée est vide.

Écrivons $U(C)=\bigcup_{S\in C}S$. Puisque chaque point de S appartient à l'une de ses k-faces :

$$U(C)=U(\partial_k C)\subseteq U\bigl(v_{k+1\to k,c}(C)\bigr).$$

L'inclusion peut être stricte. La verticale n'impose donc ni égalité des couvertures, ni conservation du cardinal ponctuel, ni bijection des composantes. Elle ne crée pas non plus de compatibilité automatique entre des masses recalculées indépendamment aux deux ordres ; le [contrat des masses et du vote](CONTRAT_MASSES_VOTE_COURANT.md) distingue ce calcul d'une mesure poussée par sommes sur les fibres.

**Naturality horizontale.** Pour $c\preceq d$, soit $h_{k,c,d}$ l'inclusion sur les composantes. Toute facette utilisée à c demeure à d ; les deux chemins suivants aboutissent à la composante contenant les mêmes facettes initiales :

$$v_{k+1\to k,d}\circ h_{k+1,c,d}=h_{k,c,d}\circ v_{k+1\to k,c}.$$

Ce carré couvre aussi $a^-\to a^+$ et un niveau où seul l'ordre cible change. Une application ancrée une fois n'est pas une permission de conserver une ancienne racine cible sans la faire avancer horizontalement.

**Composition des ordres.** Pour $k<\ell<m$, définir $v_{m\to k,c}$ à partir de tous les sous-ensembles de cardinal k des labels source. Les cliques inférieures restent connectées. La projection via l'ordre intermédiaire contient ces mêmes faces ; l'unicité de leur composante donne :

$$v_{m\to k,c}=v_{\ell\to k,c}\circ v_{m\to\ell,c}.$$

Les carrés horizontaux et ces identités donnent toutes les compositions ordre–rayon. Les égalités simultanées demandent l'état cible correspondant au côté de coupe, jamais une séquentialisation des événements égaux.

## 4. Transport au sous-flot E et certificat d'une seule ancre

Le certificat horizontal fournit, pour chaque ordre disponible et chaque coupe, une bijection $\Phi_{k,c}:\mathcal R_k(c)\to\mathcal C_k(c)$ par inclusion des facettes, préservant U et naturelle en c. À K1, cette autorité est celle des racines ponctuelles et du single-linkage. La définition verticale sur le résultat réduit E est :

$$\widehat v_{k+1\to k,c}=\Phi_{k,c}^{-1}\circ v_{k+1\to k,c}\circ\Phi_{k+1,c}.$$

La fonctionnalité, la totalité abstraite, l'inclusion des points, la naturalité et la composition des ordres en découlent. Cette preuve ne demande pas de reconstruire Gamma dans le produit : Gamma décrit l'objet auquel le résultat est identifié.

**Certificat suffisant d'ancre.** Soit A une composante retenue source. Choisir un label effectivement retenu $S\in A$, puis une k-face $F\subset S$. Si une autorité certifie une composante retenue cible T telle que $F\in\Phi_{k,c}(T)$, alors $\widehat v_{k+1\to k,c}(A)=T$. En effet, F appartient à la projection verticale de $\Phi_{k+1,c}(A)$, qui possède une seule cible ; l'injectivité de $\Phi_{k,c}$ identifie T.

Ainsi, une fois la fidélité horizontale E acquise, il suffit d'une ancre résolue par composante source. L'énumération exhaustive des labels Gamma ou d'un arbre couvrant externe, requise comme prémisse dans certaines coutures historiques conditionnelles, n'est pas une obligation supplémentaire ici. Le membership du label S dans la composante retenue et la résolution inférieure de F doivent en revanche avoir une autorité réelle.

Si F est déjà matérialisée et active dans la table inférieure, le lookup de sa racine suffit. Sinon, le certificat horizontal garantit l'existence de la cible, mais ne fournit pas le chemin permettant de la trouver. Un resolver peut fournir une attache certifiée vers une facette retenue de la même composante Gamma, ou une autre preuve de membership relative à cette composante. Un échec de lookup demeure une absence de matérialisation ; ce n'est ni une facette Gamma isolée, ni une composante source sans image.

Scanner plusieurs suppressions d'un label S arbitraire est une accélération correcte dès qu'une résolution réussit. La terminaison de ce scan pour tout label E n'est pas prouvée ici. Le résultat plus fort utile au produit porte sur un autre quantificateur : **scanner une seule suppression de chacun des labels `born` d'une vraie naissance E termine avec succès**. La structure géométrique d'une naissance, ajoutée à la fidélité horizontale, le prouve au §5. Les contre-fixtures du §6 ne réfutent pas cette propriété.

## 5. Construction totale depuis `born` et `parents`

### 5.1 Une naissance supérieure fournit une directe inférieure

Considérons une vraie naissance réduite A à l'ordre k+1, au niveau a, donc un delta dont `parents` est vide dans l'histoire horizontale qualifiée depuis son état initial. Une entrée dans une vue filtrée par taille n'est pas cette naissance. Choisissons une coface retenue Q de son groupe atomique, avec $\lvert Q\rvert=k+2$ et $\beta(Q)=a$. Sur E, sa miniball possède un support essentiel de cardinal q au moins deux ; retirer un essentiel u donne une facette stricte $S=Q\setminus\lbrace u\rbrace$, avec $\beta(S)<a$.

**Lemme de naissance.** Aucun point de $X\setminus S$ n'appartient à la boule fermée $B_S$.

En effet, si un tel point z existait, l'ensemble $S\cup\lbrace z\rbrace$ serait une coface de l'ordre supérieur k+1, de niveau exactement $\beta(S)<a$. La facette S aurait donc déjà une incidence supérieure et appartiendrait à une composante Gamma non triviale strictement antérieure. Q toucherait cette composante au niveau a, et le groupe ne pourrait pas être une naissance à zéro parent. Le certificat horizontal transfère cette contradiction aux parents du vrai delta E, même si une facette de contact n'était pas représentée auparavant dans le sous-flot.

S est donc une coface Gabriel à l'ordre inférieur k. Sa boule vérifie $p(B_S)+q(B_S)\leq\lvert S\rvert=k+1\leq r_{\max}$. Le census de la route E terminée exclut les extra-shells pertinents, donc cette coface est régulière et appartient au catalogue direct inférieur complet. Chaque k-face de S est dès lors matérialisée dans le sous-flot inférieur au plus au niveau $\beta(S)$, **strictement avant a**.

Ce lemme ne suppose pas l'hérédité Gabriel pour une coface directe arbitraire. E5 la réfute : ABC est directe alors que sa face AC a les intrus D et E ; l'événement ABC est une continuation dans la vraie histoire K2, pas une naissance. La condition « zéro parent dans l'histoire horizontale qualifiée » est indispensable.

Si l'événement Q est encore disponible, deux essentiels distincts u et v donnent directement l'ancre $F=Q\setminus\lbrace u,v\rbrace$ ; elle appartient aux deux directes inférieures strictes $Q\setminus\lbrace u\rbrace$ et $Q\setminus\lbrace v\rbrace$. Le support déjà livré suffit, sans nouveau calcul de MEB. La variante suivante élimine même ce besoin de provenance d'événement.

### 5.2 Pourquoi `born` contient une ancre qui réussit

Le [fold normalisé](../src/forest/fold.hpp#L1045) fige `seen` avant les unions du lot. Une facette vue possède déjà une incidence retenue et une racine réduite. Le [remplissage de `born`](../src/forest/fold.hpp#L1098) ajoute chaque facette touchée dont `seen` était faux, y compris les facettes géométriquement strictes mais jusque-là isolées.

À une naissance sans parent, aucune facette incidente du groupe ne peut appartenir à une racine antérieure. Toutes les facettes d'une coface Q de ce groupe figurent donc dans son `born`, dont les q facettes strictes obtenues en retirant un essentiel. Le regroupement simultané et la déduplication conservent ces facettes même lorsque plusieurs cofaces contribuent au même groupe.

Voici alors un resolver **total sur les naissances E qualifiées** : parcourir les labels S de `born`, former une seule face canonique $F(S)=S\setminus\lbrace\max S\rbrace$, puis chercher F(S) dans la table inférieure après fermeture complète du niveau a. Pour chacun des labels stricts du lemme précédent, toutes ses k-faces sont déjà matérialisées ; sa face canonique réussit donc. Le parcours termine après au plus `|born|` lookups.

Toute réussite plus précoce est également correcte, même si le label essayé n'était pas strict : S appartient à la nouvelle composante source et F(S) appartient à S. Le lemme d'une seule ancre du §4 identifie sa cible. Aucun support, coface, centre, rayon nouveau, census, intrus ou Gamma n'est consulté par ce resolver de tokens.

Épuiser tout `born` sans succès est donc une contradiction avec les autorités de ce domaine ou un échec d'exécution explicite. Ce résultat ne devient jamais « aucune cible ». Un cap qui interrompt le parcours reste un refus, et ne transforme pas la preuve de totalité mathématique en succès d'une exécution incomplète.

### 5.3 Propagation et traitement des coupes

À la naissance, conserver l'ancre F trouvée et la racine inférieure qui la contient. Lors d'une continuation à un parent, porter cette même ancre vers la composante source résultante. Son label persiste ; seule sa racine cible doit avancer horizontalement. Aucun nouveau scan de `born` n'est requis. Lors d'une multifusion, normaliser toutes les ancres des parents dans le même état inférieur courant, exiger une racine commune, puis conserver une de ces ancres pour le résultat.

Par induction sur les naissances et les transitions source, chaque composante possède une ancre correcte. La naturalité du §3 justifie les propagations ; l'unicité de la cible justifie les comparaisons à la fusion. On obtient donc toutes les cartes réduites E sans resolver géométrique général. La descente du §8 garde son intérêt pour d'autres requêtes de labels, mais n'est pas une obligation de cette construction.

La référence stable peut être une facette témoin, résolue par le DSU inférieur à la coupe demandée, ou un identifiant historique muni de son successeur. On ne doit pas supposer que le token `output` ou le contenu d'une composante reste constant. Au côté fermé, tous les lots cibles de niveau inférieur ou égal sont appliqués ; au côté ouvert, seuls les lots strictement inférieurs le sont. Les `batch` propres à chaque ordre ne sont pas une horloge globale. Il n'est pas nécessaire de réécrire toutes les ancres entrantes lorsqu'un groupe inférieur fusionne : leur normalisation peut se faire lors de la consultation.

Un rejeu hors ligne des deux payloads adjacents, ou un merge de leurs niveaux exacts, suffit à organiser ce travail. Le nombre de propositions d'ancre est au plus la somme des cardinalités `born` des seuls deltas de naissance source. Les lookups, insertions, unions et normalisations gardent les coûts de la structure de dictionnaire et du DSU effectivement choisis ; aucune borne de temps mural n'en est inférée. Il n'y a ni catalogue Gamma, ni matrice ordres–coupes, ni nouvelle recherche géométrique.

### 5.4 Autorité et champs à transporter

| Information | Invariant |
| --- | --- |
| Entrée et autorité | Identité du nuage canonique avec ses PointId, hashes des payloads source/cible et certificats horizontaux de même entrée ; résultats terminaux réussis |
| Ordres | `source_K=k+1`, `target_K=k`, dans la fenêtre disponible |
| Coupe | `cut_level` rationnel exact et `cut_side` ouvert/fermé ; même coupe des deux côtés |
| Naissance | `parents=[]`, ancre $F(S)\subset S$ pour un label S de `born`, lookup inférieur réellement réussi |
| Continuation/fusion | Ancres entrantes présentes et normalisées ; une seule racine inférieure après le lot requis |
| Résultat | Carte liée à ces autorités ; une absence de token intermédiaire n'est jamais une absence mathématique de cible |
| Publication | Succès terminal ; un préfixe, une exception ou un cap interrompant la reconstruction ne certifie pas une famille complète |

Ces champs décrivent la future reconstruction et son export, pas une API verticale déjà publiée par v7.

## 6. Contre-fixtures et reçus bornés

### 6.1 Le point commun n'identifie pas la cible

Voici le schéma minimal de deux composantes non triviales de Gamma2 partageant un point : $A=(2,2,2)$, $B=(3,2,2)$, $C=(2,3,2)$, $E=(1,1,2)$, $F=(1,2,1)$, à la coupe fermée $a=2/3$. Les triangles ABC et AEF ont respectivement les niveaux $1/2$ et $2/3$. Tout triangle mixte contient une paire entre $\{B,C\}$ et $\{E,F\}$ de distance carrée au moins 3, donc son niveau est au moins $3/4>a$.

Gamma2 possède donc les deux composantes d'arêtes de ABC et AEF, dont les couvertures partagent A. Gamma1 n'a qu'une composante : les deux images verticales K2→K1 coïncident. Choisir un point puis chercher « sa composante » n'est pas une opération univoque à l'ordre deux.

Ajouter $D=(2,2,3)$ donne le cas K3→K2 : le tétraèdre ABCD est actif au niveau $2/3$, sa miniball étant celle de BCD. La composante source de ses quatre faces se projette sur la composante inférieure des six arêtes d'ABCD, tandis que l'autre composante AEF couvre aussi A. Choisir A seul pour résoudre cette verticale autoriserait la mauvaise cible. Ces exemples sont des calculs Gamma sans hypothèse de régularité ; leurs triangles droits ne constituent pas une entrée produit E qualifiée.

### 6.2 H0 et couverture ne garantissent pas une face matérialisée

La [fixture permanente à six labels](receipts_vertical_20260905/math/combinatorial_anchor_counterexample.py) est purement combinatoire. Au premier niveau, le complexe est engendré par AXY, BXY, CXY, DXY et toutes leurs faces. Au second niveau, ajouter ABCD **et le triangle de liaison ABX**. Le sous-flot inférieur conserve les quatre triangles initiaux ; le sous-flot supérieur conserve le tétraèdre.

Gamma2 complet et le sous-flot inférieur ont chacun une composante, avec les mêmes six points couverts. L'inclusion donne leur bijection aux deux niveaux. Au second niveau, le composant complet possède quinze arêtes et le composant retenu les neuf arêtes $XY,AX,AY,BX,BY,CX,CY,DX,DY$. La composante Gamma3 est formée des quatre triangles d'ABCD. Aucune de leurs six arêtes distinctes n'est matérialisée dans l'ordre inférieur : les douze requêtes de suppression échouent malgré l'existence de la carte verticale.

Le triangle ABX est indispensable à cette version : il relie le K4 à l'étoile dans Gamma2, tout en demeurant un sommet isolé à l'ordre trois. Sans ABX, Gamma2 possède deux composantes distinctes ; le sous-flot contenant seulement l'étoile ne serait plus horizontalement fidèle.

Cette variante sans liaison fournit une autre dent : les deux composantes inférieures couvrent chacune les quatre points ABCD, mais seule celle des six arêtes du tétraèdre est l'image verticale. Même la contenance de **tous** les points source ne certifie donc pas la cible sur un complexe simplicial arbitraire.

La fixture ne possède pas la structure d'une naissance régulière E : ses quatre facettes d'ABCD apparaissent au même niveau que le tétraèdre, sans facette stricte. Or le support essentiel d'une coface E donne au moins deux facettes strictes, qui deviennent les directes inférieures du §5. Cette distinction explique pourquoi le scan de `born` est total sur E sans contredire cette non-implication depuis les seules données H0 et ponctuelles.

Les reçus [normal](receipts_vertical_20260905/math/combinatorial_normal.json) et [Python optimisé](receipts_vertical_20260905/math/combinatorial_optimized.json) vérifient les composantes par parcours de graphe et les confrontent aux listes littérales : douze lookups positifs dans Gamma complet, douze absences dans le sous-flot, rejet attendu `shortcut.no_retained_anchor`. Les portes n'utilisent pas `assert`. La portée est `combinatorial_only` : aucune réalisabilité Čech de ce complexe, aucune production par E et aucun défaut du moteur ne sont revendiqués. La fixture réfute précisément la déduction depuis les seules bijections horizontales et couvertures.

### 6.3 Rejeu vertical des payloads E déjà scellés

Le [juge vertical](vertical_replay.py) consomme les sorties déjà scellées du pipeline E. Il construit Gamma uniquement comme oracle borné, projette tous les labels source Gamma puis conjugue par les bijections horizontales. Il vérifie séparément les faces effectivement retenues, les compositions de deux ordres et les carrés aux niveaux globaux, y compris les niveaux sans événement source.

Par provenance O2 et UBSan, les reçus [normal](receipts_vertical_20260905/vertical/normal.json) et [optimisé](receipts_vertical_20260905/vertical/optimized.json) comptent 16 cas, 432 coupes, 1 608 couples coupe–ordres adjacents, 764 cartes de composantes, 720 carrés de naturalité et 400 compositions sur deux ordres. Ils observent 248 inclusions strictes de points et 144 changements du contenu de la composante cible. Ces derniers ne signifient pas nécessairement un changement de token `output`.

Sur la courbe des moments à sept points, à $a=819/4$ fermé, le label source `[3,19,65537]` contient la face inférieure `[3,65537]`, absente de la table retenue K2 ; la vraie cible porte le représentant `[3,19]`. Les reçus comptent 2 296 occurrences de faces absentes sur 15 688 requêtes. Chaque label examiné possède néanmoins au moins une autre face retenue : cette observation ne prouve pas la totalité générale du scan des suppressions.

Le premier exemple de cible à avancer est à K1, du côté ouvert au côté fermé de $a=9$ : le label source `[211,65537]` persiste, tandis que sa composante cible gagne le PointId 3. Ignorer le côté de coupe ou conserver l'ancien contenu cible contredirait le carré de naturalité.

Ces cartes conjuguées sont des objets d'audit. Aucun moteur n'a été relancé pour ce rejeu, aucun resolver produit n'est appelé et aucun export vertical v7 n'est qualifié par ces nombres.

### 6.4 Reconstruction effective par les seuls tokens

Le [lecteur d'ancres](vertical_anchor_replay.py) réalise désormais le §5 sous audit : sa classe `TokenVertical` reçoit seulement les PointId et les deltas. Elle ne consulte ni coordonnées, ni MEB, ni cofaces géométriques, ni Gamma. Le juge séparé confronte ses cartes à la projection Gamma par les bijections horizontales, puis vérifie les carrés et les compositions. Les appels à `advance` aux niveaux d'audit sans delta sont sans effet sur les décisions du lecteur.

Les [reçus normaux](receipts_resolver_20260905/anchors/normal.json) et [Python optimisé](receipts_resolver_20260905/anchors/optimized.json) séparent trois provenances, pour chacun des deux payloads scellés O2 et UBSan :

| Provenance | Contrôle exercé |
| --- | --- |
| 16 sorties E originales | 764 cartes, 720 carrés de naturalité, 400 compositions ; 44 naissances résolues par 44 lookups, 104 continuations, aucune multifusion source d'ordre au moins deux |
| Un réindexage explicite du certificat de la courbe des moments | Les deux extrémités reçoivent les deux plus petits PointId ; parents et représentants sont recanonisés à chaque lot. À K6, au niveau 11997, cinq misses précèdent le succès au sixième label de `born` |
| Un flux mathématique synthétique distinct | Gamma complet sur les six points $(t,t^2,t^3)$ pour $t\in\lbrace0,1,2,10,11,12\rbrace$ fournit une multifusion source, 54 cartes, 50 carrés et 21 compositions ; ce flux n'est pas une sortie E |

Le réindexage conserve le certificat horizontal comme objet mathématique, mais ne prétend pas être un nouveau run du moteur sur ces identifiants. La fixture synthétique exerce la branche de propagation des ancres à une multifusion ; sa construction par Gamma ne qualifie aucun producteur géométrique. Au total par provenance de payload scellé : 919 cartes, 866 carrés, 485 compositions, 54 naissances, 59 lookups de naissance, cinq misses, 128 continuations et une multifusion. Le maximum observé de six essais n'est pas une nouvelle borne universelle ; la borne prouvée demeure `|born|`.

Les fautes injectées appartiennent au lecteur d'audit : limiter le scan au premier label est refusé par `anchor.birth_not_resolved` ; interpréter le premier miss comme une absence de carte publie une entrée manquante que le juge externe rejette par `judge.vertical_totality` ; conserver une cible périmée échoue par `anchor.target_not_current`. Un budget nul de lookups refuse une naissance par `budget.birth_lookup`, tandis que la famille verticale vide n=2/K1 accepte ce même budget nul. Aucun de ces essais n'est présenté comme un mutant produit CTest.

Le [premier essai invalide](receipts_resolver_20260905/anchors/initial_nonvacuity_rejection.json) est conservé : son plancher `nonvacuity.multifusions` échouait parce que les multifusions horizontales du corpus E étaient toutes à K1, qui n'est jamais un ordre source vertical. La fixture mathématique séparée comble cette dent du lecteur. Aucun défaut produit n'est déduit de ce premier échec de corpus. Aucun moteur ni build n'a été lancé pour ces rejeux.

## 7. Ce qui est disponible et ce qui reste à construire

La preuve mathématique des cartes Gamma, leur transport à E et leur reconstruction totale par les tokens de naissance et les parents sont fermés ci-dessus. K2→K1 possède en outre la résolution ponctuelle particulière. Le certificat horizontal et les payloads horizontaux qualifiés nécessaires à cette construction sont disponibles.

Le produit actuel transporte seulement les champs horizontaux de [`ComponentDelta`](../src/forest/fold.hpp#L64) : `batch`, `level`, `output`, `parents`, `born`. La sortie de [`run_pipeline`](../src/pipeline/run.hpp#L1142) et le [manifeste d'archive](../src/io/archive.hpp#L303) déclarent explicitement `vertical_maps=none`. Aucun resolver ni sérialisation de cartes verticales n'est fourni par cette API. Le flag `normalized_reduced` porte la demande de route, y compris à K1 ; il ne change pas son comportement normatif et ne constitue pas un flag vertical.

Le prochain livrable est donc concret : implémenter le scan borné par `|born|` à chaque naissance, la propagation par successeurs aux coupes exactes, les contrôles de multifusion, puis un export lié aux identités source/cible et à leur succès terminal. La preuve ne laisse plus le resolver géométrique général comme verrou de cette route. La qualification doit tuer les raccourcis de première candidate seulement, d'absence interprétée comme aucune cible, de cible périmée et de mauvais côté de coupe. Le catalogue Gamma exhaustif de l'audit ne devient pas son architecture.

Le domaine des plateaux, les identités publiques du quotient, les masses et votes entre ordres, ainsi que le coût du traitement de bout en bout conservent leurs contrats propres. Le statut public reste `not_claimed`. GCP non utilisé.

## 8. Descente géométrique : resolver auxiliaire à domaine local explicite

Pour une requête portant sur un label source S arbitraire, considéré comme coface à l'ordre inférieur k, la descente envisagée reste mathématiquement valide sous essentialité locale. Poser $Q_0=S$, de cardinal k+1 et actif à la coupe demandée. Tant qu'un intrus strict $z\in X\setminus Q_i$ existe dans $B_{Q_i}$, choisir un sommet essentiel u et poser $Q_{i+1}=(Q_i\setminus\lbrace u\rbrace)\cup\lbrace z\rbrace$.

**Baisse stricte.** Le retrait essentiel donne une boule de rayon strictement plus petit pour $Q_i\setminus\lbrace u\rbrace$. Déplacer légèrement le centre courant vers le centre de cette boule rend strictement intérieurs les points conservés ; l'intrus z reste intérieur par sa marge initiale. Ainsi $\beta(Q_{i+1})<\beta(Q_i)$. Une faible inclusion de z sur le shell ne fournit pas cette preuve.

**Raccord à la coupe.** Les cofaces successives partagent la k-face $Q_i\setminus\lbrace u\rbrace$. Toutes leurs faces sont connectées dans Gamma au niveau initial $\beta(S)$, et donc à toute coupe où S est actif, ouverte ou fermée selon le même critère. La descente ne requiert pas que ces faces intermédiaires soient matérialisées dans le sous-flot inférieur. À l'arrivée, la racine d'une face directe identifie la cible par le certificat horizontal.

**Terminaison conditionnelle.** Si chaque étape non terminale dispose d'un essentiel et d'un intrus strict, les niveaux diminuent dans l'ensemble fini des cofaces de cardinal k+1. Aucun état ne se répète : il y a au plus $\binom{n}{k+1}-1$ remplacements. Cette borne prouve la terminaison sans proposer d'énumérer cet ensemble ; elle n'est pas une borne pratique de longueur ou de latence.

**Autorité du terminal.** Une coface terminale Q sans intrus strict étranger est Gabriel faible, avec $p(B_Q)+q(B_Q)\leq\lvert Q\rvert=k+1\leq r_{\max}$. Sur une entrée E acceptée, une extra-shell de cette boule serait pertinente et aurait provoqué le refus du census. Le terminal est donc régulier, figure dans le catalogue direct inférieur complet et toutes ses k-faces ont été matérialisées au plus à $\beta(Q)$. Un cache ne peut remplacer ce terminal que s'il lie une descente déjà certifiée à une ancre inférieure, au même nuage et au même ordre ; sa racine doit ensuite être avancée à la coupe demandée.

Les outils locaux existent dans [`silent_incidence.hpp`](../src/forest/silent_incidence.hpp) : `miniball` vérifie le shell local essentiel, `intruders` poursuit le contrôle de bord global, et `Builder::run` vérifie la baisse stricte puis la présence du terminal dans son catalogue. Le label source possède au plus dix points sur cette route ; ses MEB locales restent dans le domaine de taille déjà qualifié. Ces outils construisent actuellement des incidences horizontales et ne constituent pas une API de résolution verticale. Un resolver dédié pourrait ne conserver qu'une coface courante, un témoin de remplacement et une ancre terminale, sans ajouter ses étapes au payload horizontal.

Enfin, le succès horizontal E ne garantit pas la régularité de toute nouvelle MEB que visiterait une descente arbitraire : des irrégularités hors fenêtre restent autorisées lorsqu'elles n'ont pas été rencontrées par les contrôles horizontaux. Le helper existant refuse un shell local non essentiel ou extérieur ; il faudrait conserver ce refus, ou prouver une autre politique de plateau. L'essentialité du sommet retiré est nécessaire à la preuve de baisse, tandis que l'absence de shell extérieur à chaque étape est une condition plus forte du helper actuel. **Cette restriction ne réduit pas la totalité de la construction par `born` du §5, qui n'effectue aucune nouvelle descente.**
