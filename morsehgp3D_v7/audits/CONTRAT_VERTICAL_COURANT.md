# Contrat vertical courant : descendre en ordre à coupe fixée

5 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`. Travail mathématique et petits rejeux sous `audits/` ; aucun moteur exécuté pendant la fenêtre de chronométrage E/F.

**La verticale mathématique est une application totale et naturelle sur les composantes réduites disponibles. Le certificat horizontal E permet de la transporter au sous-flot retenu ; il ne fournit pas encore son resolver ni son export.** Une seule ancre correctement résolue suffit par composante source. Une facette absente de la table inférieure ne signifie jamais que cette composante n'a pas d'image.

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

Scanner plusieurs suppressions de S est une accélération correcte dès qu'une résolution réussit. La terminaison de ce scan avec succès pour tout label E n'est pas prouvée ici. Le §6 sépare une absence réellement observée dans E d'une non-implication combinatoire plus forte ; cette dernière ne réfute pas le producteur E.

## 5. Contrat constructif du futur resolver et de la propagation

Une liaison verticale doit engager les informations suivantes. Il s'agit d'un contrat à implémenter, pas de champs déjà livrés par v7.

| Information | Invariant |
| --- | --- |
| Entrée et autorité | Identité du nuage canonique avec ses PointId, hashes des payloads source/cible et certificats horizontaux de même entrée ; résultats terminaux réussis |
| Ordres | `source_K=k+1`, `target_K=k`, dans la fenêtre disponible |
| Coupe | `cut_level` rationnel exact et `cut_side` ouvert/fermé ; même coupe des deux côtés |
| Source | Référence de composante active, label S effectivement membre, puis k-face F de S |
| Cible | Référence de composante active à la coupe, preuve de $F\in\Phi_{k,c}(T)$ ; un ancien identifiant doit être normalisé vers son successeur courant |
| Résultat | Résolution certifiée ou dette de résolution/refus explicite ; une table sparse vide ne constitue pas une réponse mathématique « aucune cible » |
| Publication | Statut terminal lié à ces autorités ; un préfixe, une exception ou un cap interrompant la résolution ne certifie pas une famille complète |

À une naissance source réduite, une première ancre fournit la carte. Lors d'une continuation à un parent, cette ancre demeure valable : son label persiste et sa cible avance par les deltas inférieurs. Aucun nouveau nœud source n'est nécessaire. Lors d'une multifusion, les ancres entrantes sont toutes avancées jusqu'à la même coupe cible et leurs racines doivent coïncider avant d'ancrer le résultat.

Cette propagation peut conserver une facette témoin stable et une référence cible historique, puis appliquer son successeur horizontal. Elle doit consommer les continuations et les changements de représentant, et ne peut supposer que le token `output` ou le contenu d'une composante reste constant. Au côté fermé, tous les lots cibles de niveau inférieur ou égal sont appliqués ; au côté ouvert, seuls les lots strictement inférieurs le sont. Les `batch` propres à chaque ordre ne sont pas une horloge globale.

Un rejeu hors ligne des deux payloads adjacents, ou un merge de leurs niveaux exacts, suffit à organiser ce travail. Il faut conserver les cartes des composantes demandées et leurs ancres, pas un catalogue Gamma ou une matrice ordres–coupes. Aucune borne de coût du resolver géométrique sur miss n'est fournie par la seule preuve de fonctionnalité.

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

Les reçus [normal](receipts_vertical_20260905/math/combinatorial_normal.json) et [Python optimisé](receipts_vertical_20260905/math/combinatorial_optimized.json) vérifient les composantes par parcours de graphe et les confrontent aux listes littérales : douze lookups positifs dans Gamma complet, douze absences dans le sous-flot, rejet attendu `shortcut.no_retained_anchor`. Les portes n'utilisent pas `assert`. La portée est `combinatorial_only` : aucune réalisabilité Čech de ce complexe, aucune production par E et aucun défaut du moteur ne sont revendiqués. La fixture réfute précisément la déduction depuis les seules bijections horizontales et couvertures.

### 6.3 Rejeu vertical des payloads E déjà scellés

Le [juge vertical](vertical_replay.py) consomme les sorties déjà scellées du pipeline E. Il construit Gamma uniquement comme oracle borné, projette tous les labels source Gamma puis conjugue par les bijections horizontales. Il vérifie séparément les faces effectivement retenues, les compositions de deux ordres et les carrés aux niveaux globaux, y compris les niveaux sans événement source.

Par provenance O2 et UBSan, les reçus [normal](receipts_vertical_20260905/vertical/normal.json) et [optimisé](receipts_vertical_20260905/vertical/optimized.json) comptent 16 cas, 432 coupes, 1 608 couples coupe–ordres adjacents, 764 cartes de composantes, 720 carrés de naturalité et 400 compositions sur deux ordres. Ils observent 248 inclusions strictes de points et 144 changements du contenu de la composante cible. Ces derniers ne signifient pas nécessairement un changement de token `output`.

Sur la courbe des moments à sept points, à $a=819/4$ fermé, le label source `[3,19,65537]` contient la face inférieure `[3,65537]`, absente de la table retenue K2 ; la vraie cible porte le représentant `[3,19]`. Les reçus comptent 2 296 occurrences de faces absentes sur 15 688 requêtes. Chaque label examiné possède néanmoins au moins une autre face retenue : cette observation ne prouve pas la totalité générale du scan des suppressions.

Le premier exemple de cible à avancer est à K1, du côté ouvert au côté fermé de $a=9$ : le label source `[211,65537]` persiste, tandis que sa composante cible gagne le PointId 3. Ignorer le côté de coupe ou conserver l'ancien contenu cible contredirait le carré de naturalité.

Ces cartes conjuguées sont des objets d'audit. Aucun moteur n'a été relancé pour ce rejeu, aucun resolver produit n'est appelé et aucun export vertical v7 n'est qualifié par ces nombres.

## 7. Ce qui est disponible et ce qui reste à construire

La preuve mathématique des cartes Gamma et leur transport abstrait à E sont fermés ci-dessus. Les ancres retenues offrent un fast path certifié ; K2→K1 possède la résolution ponctuelle particulière. Le certificat horizontal et les payloads horizontaux qualifiés nécessaires à la propagation sont disponibles.

Le produit actuel transporte seulement les champs horizontaux de [`ComponentDelta`](../src/forest/fold.hpp#L64) : `batch`, `level`, `output`, `parents`, `born`. La sortie de [`run_pipeline`](../src/pipeline/run.hpp#L1142) et le [manifeste d'archive](../src/io/archive.hpp#L303) déclarent explicitement `vertical_maps=none`. Aucun resolver ni sérialisation de cartes verticales n'est fourni par cette API. Le flag `normalized_reduced` porte la demande de route, y compris à K1 ; il ne change pas son comportement normatif et ne constitue pas un flag vertical.

Le prochain livrable est donc concret : une liaison certifiée pour chaque naissance source, un resolver total sur son domaine annoncé ou des dettes explicites, la propagation par successeurs aux coupes exactes, les contrôles de multifusion, puis un export lié aux identités source/cible et à leur succès terminal. Sa qualification doit tuer les raccourcis de face absente, de point seul, de cible périmée et de mauvais côté de coupe. Le catalogue Gamma exhaustif de l'audit ne devient pas son architecture.

Le domaine des plateaux, les identités publiques du quotient, les masses et votes entre ordres, ainsi que le coût du resolver et du traitement de bout en bout conservent leurs contrats propres. Le statut public reste `not_claimed`. GCP non utilisé.
