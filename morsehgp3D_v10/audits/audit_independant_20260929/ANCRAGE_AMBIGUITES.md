# Ancrage des ambiguïtés et mesure des hauteurs de fusion

29 septembre 2026. Réponse à
[`REPONSE_CLAUDE_ADDENDA_ET_RAPPORT_INDEPENDANT_20260929.md`](../REPONSE_CLAUDE_ADDENDA_ET_RAPPORT_INDEPENDANT_20260929.md),
après lecture de l'[addendum de l'audit continu](../audit_continu_20260929/ADDENDUM_ANCRAGE_ET_CERTIFICAT_20260929.md).
`phase=exploration_v10_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u18_input_only`, `mode=audit_math_projection`,
`public_status=not_claimed`. Note mathématique ; aucun changement du moteur,
aucune nouvelle campagne, aucune utilisation de GCP.

Actualisation du 30 septembre : relecture des sections 7–9 de
[`AUDIT_LAMINARITE_POINTS_20260929.md`](../audit_continu_20260929/AUDIT_LAMINARITE_POINTS_20260929.md),
du [lemme renforcé de couverture](../audit_continu_20260929/catalogue/ADDENDUM_COUVERTURE_CATALOGUE_20260929.md)
et de la [réponse de raccord](../REPONSE_CLAUDE_CONTRE_AUDITS_ET_RACCORD_20260929.md),
sur `e9eab2754f3d9f2a9e16d8c542b908c19b725c9a`. La petite contre-épreuve
ci-dessous est distincte de leurs deux paires éloignées.

## Relecture de la thèse : préserver les points frontière

À la demande de l'utilisateur, l'audit a relu les deux premières parties du
[manuscrit](../../../docs/references/MANUSCRIT_THESE_HAUSEUX.pdf), avec l'introduction :
chapitres 1–5, pages imprimées 1–50, puis chapitres 6–9, pages 53–107.
Le SHA-256 est `579f83671ebca34cd810f350820074eb42672411713160f9c9c2a458ff4f4fef`.
Les références ci-dessous sont les pages imprimées ; ajouter 26 pour la page PDF.

**Le modèle discret visé par la thèse est la couverture complète.** La définition 8
(p.21), puis le théorème 2 (pp.60–61), donnent
`C_discret(r) = {x∈X : dist(x,C)≤r} = X∩δ_r(C)`.
Une observation peut participer à la formation d'une région dense sans être elle-même
dans cette région. L'appartenance à plusieurs composantes est une information à conserver.
La restriction core `C∩X` reste un comparateur exact et stable ; sa seule qualité
géométrique ne suffit pas à la promouvoir en cible finale fidèle à cet objectif discret.
Core du dépôt utilise les composantes exactes de L_K ; il ne se confond pas avec le
graphe des cœurs du Robust Single-Linkage.

Une déduction élémentaire illustre la perte : X={0,2,4}, K2, r=11/10.
L2 possède les composantes `[9/10,11/10]` et `[29/10,31/10]`, qui couvrent
respectivement `{0,2}` et `{2,4}`. Aucun des trois sites n'est encore core ; leurs dates
core sont r=2, précisément la fusion des deux composantes. Une tête limitée à core
perd donc tous leurs points avant fusion. Reporter le point partagé au LCA réduit
également les masses des descendants. C'est une déduction de l'audit, pas une expérience
ni une fixture attribuée à l'auteur.

**Le critère de récupération avant fusion doit compléter celui des hauteurs.** Le
chapitre 7 sépare deux bénéfices : connecter correctement les régions denses, puis
récupérer leurs observations frontière sans faire de celles-ci des relais de connexion.
Le théorème 3 (pp.71–72), sous son modèle à deux densités et ses hypothèses, exprime la
fraction récupérable avant percolation du fond par `Θ_cc(λ_c_cc ρ1/ρ0)`.
Une projection stable peut perdre cette fraction en différant les points jusqu'à une
fusion. Les résultats de simulations à petits K (pp.73–76) restent pertinents pour cette
récupération partielle ; l'absence de consistance uniforme à K fixé ne les annule pas.
La formule pour densité générale p.70 est heuristique, le passage à la percolation du
champ gaussien p.79 est admis, et le haut rappel p.80 est présenté comme conjectural.
Ces statuts ne deviennent pas des garanties nouvelles du moteur.

**La masse frontière intervient avant condensation.** Le §9.1 (pp.96–97) définit
`S_τ=Σ_{σ⊃τ, |σ|=K+1} ψ(ρ_σ)`, avec ψ(r)=r^(−p) par défaut,
`T_x=Σ_{τ∋x} S_τ`, puis `m_τ=Σ_{x∈τ} S_τ/T_x`.
Chaque point représenté distribue au total une unité entre ses faces incidentes.
Ces masses alimentent `min_cluster_size` et la condensation, avant la sélection et le
vote final. Ce vote pondéré conserve une participation multiple jusqu'à la décision.
Un remplissage après sélection ne restaure pas une branche déjà perdue à cause d'une
masse insuffisante. Ces coefficients normalisés ne sont pas des probabilités a posteriori
calibrées. La compression en boules ne démontre pas qu'elle conserve les scores S_τ :
un poids natif demande une preuve d'agrégation ou une déclaration de nouvelle politique.

**La partition finale et l'arbre de points restent deux contrats.** La proposition 7
prouve une partition pour une sélection fixée. Refaire les votes à chaque coupe peut
faire changer un point de lignée : une préférence A de poids 0,4 peut être dépassée
lorsque deux branches B1 et B2 de poids 0,35 et 0,25 fusionnent, sans que A fusionne avec
elles. Une hiérarchie stricte de points doit imposer un rattachement qui suit ses
ancêtres. Le LCA différé est un bras conservateur à comparer ; son coût de récupération
doit être publié, et il n'est pas privilégié sur le seul motif de sa laminarité.

Pour aider le développeur, la prochaine comparaison doit donc conserver en amont la
relation complète point→composantes couvertes, vérifier la masse totale par observation,
puis mesurer **avant EOM** couverture, ambiguïtés et masse récupérée/perdue avant fusion.
Les hauteurs de fusion et leur stabilité complètent ces mesures. La construction native
ci-dessous peut préserver les incidences sans énumérer toutes les K-parties ; elle ne
résout pas à elle seule la politique de masse ni la projection stricte multi-K.

## Majorité fixe : preuve valide, limite de récupération même avec 1/β

La preuve de la section 8 de l'audit continu est correcte : à K fixé, des
témoins de poids positifs finis, activés à des dates explicites et suivant
uniquement leurs ancêtres, donnent au plus un propriétaire lorsque
`M_x(C)>θ W_x`, avec `1/2≤θ<1`. Le dénominateur `W_x` comprend **tous**
les témoins fixés, y compris les futurs. Une majorité acquise ne se perd
pas. Les points sans propriétaire restent des singletons de complétion ;
les réunir dans un bloc collectif « bruit » casserait cette preuve.

Pour les atomes positifs du catalogue, l'univers propre à K est
`population≥K` et `p+q_min≤K`, avec les incidences fermées complètes.
Fixer aussi les poids en fonction de la géométrie, pas des rangs ou de Kmax,
rend cet univers et ses masses indépendants de la profondeur demandée,
pour un catalogue complet et canonique. K1 réclame des témoins sites de
masse finie. Les boules `p+q_min=K+1` restent dans la construction FULL :
le filtrage des témoins n'autorise pas leur suppression comme fusions.

Cette laminarité ne garantit pas l'entrée précoce d'une frontière dont
la première couverture est unique. Le choix `1/β` corrige la fixture des
deux paires éloignées, mais ne donne pas cette garantie en général.
Ici **β est le rayon carré** et le test est strictement `M>W/2`.

Considérons K2 et les cinq sites u18 distincts, réellement tridimensionnels :

`x0=(1,1,0), x1=(2,1,0), x2=(0,2,0), x3=(0,0,0), x4=(0,1,1)`.

L'univers propre comporte sept boules. Pour x0, les cinq incidences sont :

| Population fermée | β | Poids 1/β |
| --- | ---: | ---: |
| {0,1} | 1/4 | 4 |
| {0,2} | 1/2 | 2 |
| {0,3} | 1/2 | 2 |
| {0,4} | 1/2 | 2 |
| {0,2,3,4} | 1 | 1 |

Les deux autres boules, {2,4} et {3,4}, ont β=1/2 et ne portent pas x0.
La boule de population {0,2,3,4} a une coquille complète de quatre sites
mais `q_min=2` : ce sont quatre incidences d'un seul atome canonique.

À β=1/4, **{0,1} est l'unique composante couvrante** de x0. Pourtant
`W_0=11` et sa masse locale 4 ne dépasse pas 11/2 : x0 est différé.
À β=2/3, les trois témoins de poids 2 rejoignent la composante couvrant
{0,2,3,4}. Leur masse 6 dépasse 11/2 ; x0 entre alors dans **cette autre
branche**, tandis que x1 reste dans la branche locale {0,1}. Ces deux
branches ne fusionnent qu'à β=5/4. La paire projetée {0,1} n'existe donc
pas avant cette fusion. Le mode uniforme a la même décision sur cette
fixture. La majorité reste laminaire et géométriquement admissible :
le défaut démontré concerne la récupération et le choix d'affectation,
pas le calcul de FULL ou la preuve d'emboîtement.

Le [script exact](../../receipts/audit_independant_20260930/tower_math/check_majority_five_sites.py)
énumère tous les diamètres vides, déduplique par centre et β, garde chaque
point de coquille et contre-vérifie l'univers par toutes les sphères
critiques. Le nerf K2 est calculé sur toutes les paires, avec les
intersections de régions témoins portées par les triples et leurs MEB
Fraction. Les activations et toutes les fusions d'un plateau précèdent
les votes. Il vérifie Kmax=2,3,4,5 et 840 implications d'emboîtement,
en normal et `−O` avec des sorties identiques.

Un seul export natif K2/W1 du build figé `6206d1d11`, sans rebuild,
confirme les deux modes core/cover, sept témoins propres, quatorze coupes
de composantes et 41 résolutions/remontées de boules. Les indices Morton
sont remis en correspondance par coordonnées exactes ; aucune égalité
d'indices entre les deux juges n'est supposée. **La majorité n'est pas
une tête native** : elle reste le consommateur rationnel du script.
Le [reçu](../../receipts/audit_independant_20260930/tower_math/receipt.json)
conserve commandes, sorties, hashes du script et de sa dépendance
`reference/hgp10_ref.py`, puis du binaire et des sources figées avant/après.
Ces calculs sont une contre-épreuve bornée, sans graines dev/test,
condensation, ARI ou mesure de performance.

### Une variante cohérente qui conserve les premières couvertures uniques

Une variante à comparer expérimentalement est : à la première couverture
α(x), remonter et dédupliquer **toutes** les composantes couvertes à cette
date. S'il n'en reste qu'une, attacher x immédiatement à α(x), puis suivre
ses seuls ancêtres. Sinon, appliquer une politique déclarée de majorité
fixe ou de LCA aux vrais conflits, puis figer le premier propriétaire.
Ne pas révoquer ensuite l'attache unique à cause de nouveaux témoins.

Cette règle conserve la laminarité par ancrage et ascendance, sans passer
le dénominateur aux seuls témoins actifs. Elle préserve par construction
les frontières initialement non ambiguës. Sur la fixture ci-dessus, ce
bras et K2-LCA à η=0 donnent la paire {0,1} dès β=1/4 : le minimum de x0
est unique. Une couverture devenue multivoque plus tard ne provoque pas
de changement de lignée. Ce sont un choix de projection et une propriété
structurelle ; ni la vérité statistique de ce choix, ni un gain de score
ou de temps ne sont démontrés. L'unicité initiale exige elle-même une
marge pour une garantie sous perturbation.

L'export d'un propriétaire doit toujours viser son ancêtre **vivant à sa
date**, après toutes les activations simultanées. Sa masse unitaire entre
alors dans la projection dure avant condensation ; les singletons de
complétion ne doivent pas donner une masse anticipée à une branche
spatiale. Comparer aussi les contributions fractionnaires inspirées de
la thèse : une masse différée peut changer `min_cluster_size` et supprimer
une branche avant EOM. Le remplissage final ne la restaure pas.

### Poids recalculés : une marge de vote est nécessaire

La stabilité conditionnelle à poids **identiques** de la section 8 ne
qualifie pas les poids 1/β recalculés. Déjà sur les trois sites collinéaires
`{0,L−δ,2L}` et `{0,L+δ,2L}`, avec δ>0 petit, l'univers propre K2 conserve
les mêmes deux paires adjacentes. La plus courte porte plus de la moitié
de la masse inverse-β du point central ; elle le reçoit dès sa naissance.
Cette majorité reproduit donc le saut de première couverture : la hauteur
avec l'extrémité gauche passe de `(L−δ)/2` à L, en **rayon**, sous un
déplacement de 2δ. Le saut n'exige ni changement d'univers ni arrondi :
des poids continûment recalculés suffisent lorsque la marge tend vers zéro.

Pour un univers apparié et un transport de branches donné, poser
`q_i=w_i/W_x` et `γ=M_x(C)/W_x−θ`. Une marge positive doit dominer la
variation de cette somme normalisée ; par exemple
`γ>Σ_i |q_i'−q_i|` suffit à conserver la majorité dans l'image lorsque
chaque témoin actif y est transporté. Cela ne certifie ni le transport
des témoins, ni leur sélection, ni toutes les premières dates de majorité.
Publier ces marges séparément de la marge de bande K2, puis mesurer rappel
avant fusion et variations appariées. Aucun réglage de θ ou des poids
n'est ici présenté comme statistiquement qualifié.

## Réponse au choix du tirage

**Conserver deux panels : un tirage uniforme pour l'estimation globale, un
tirage stratifié pour le diagnostic.** Un seul score issu de quotas égaux
« voisins / moyens / lointains » changerait implicitement la population
mesurée. Inversement, les nombreuses paires lointaines peuvent rendre un
score uniforme peu sensible aux erreurs locales de rattachement.

Sur chaque petite scène dev, choisir les panels à partir du nuage de base,
avant le calcul des variantes de projection ou le choix de η. Conserver les
IDs des deux extrémités, les graines et les probabilités de tirage. Réutiliser
exactement ces paires pour core, cover, chaque η et toutes les perturbations
appariées. Ne pas recalculer les strates après une perturbation : cela
mélangerait variation des hauteurs et variation du panel.

Une mise en œuvre concrète peut garder :

- un panel uniforme de paires non ordonnées distinctes, chacune de probabilité
  identique parmi les `n(n−1)/2` paires ; ses moyennes et distributions décrivent
  cette population, avec l'incertitude de tirage publiée ;
- un panel supplémentaire de voisins et de bandes de distance moyenne et
  lointaine, dont les seuils et quotas sont fixés sur le nuage de base. Publier
  chaque strate séparément. Les valeurs des seuils sont des choix de protocole,
  à figer avant les résultats, pas des paramètres statistiquement qualifiés.

Si les strates disjointes et exhaustives sont regroupées en une moyenne
globale, pondérer la moyenne de la strate h par `N_h / N`, où N_h est son
nombre de paires et `N=n(n−1)/2`. Pour d'autres plans, publier les probabilités
d'inclusion et utiliser leurs poids. Sans ces nombres ou probabilités, garder
les résultats stratifiés comme diagnostics et utiliser le panel uniforme
pour l'estimation globale. Il n'est pas nécessaire de payer un décompte de
toutes les paires d'une grande trame pour obtenir ces deux usages séparés.

La distance euclidienne n'identifie pas à elle seule les cols. Un troisième
panel peut viser des paires entre branches d'une **référence figée**, à des
coupes ou fusions fixées avant les variantes. Cela aide à inspecter les cols
de cette référence, avec son biais annoncé. Choisir les paires après avoir
vu les erreurs du candidat transforme ce panel en recherche de
contre-exemples ; cela ne donne plus une estimation de fréquence des erreurs.
Une référence core ou MR sert ici de comparateur déclaré, pas de vérité
physique. Aucune étiquette SemanticKITTI n'est nécessaire.

## Ce que les hauteurs doivent mesurer

Utiliser le rayon pour le certificat de perturbation : sa borne est `2ε`,
pas une constante de même taille sur les rayons carrés. Publier aussi les
valeurs exactes ou rangs de niveau lorsque disponibles. Une normalisation,
si nécessaire entre scènes, doit utiliser une échelle du nuage de base,
commune à toutes les variantes.

Pour chaque paire fixée, distinguer :

1. sa réunion dans la projection de points, en tenant compte des deux dates
   d'entrée et de l'ancêtre commun ;
2. le changement de cette hauteur sous une perturbation appariée ;
3. la différence avec un comparateur fixé, qui devient une « erreur »
   seulement si la cible est définie et justifiée.

Garder les hauteurs de la structure avant EOM ou une autre sélection. Mesurer
ensuite la sélection sur ses propres sorties. Publier séparément les dates
d'entrée, la masse différée aux coupes choisies et les paires dont la réunion
est retardée. La branche racine doit rester vivante au-delà de la dernière
fusion spatiale : une date d'attache tardive reste une date réelle. Une paire
absente ou non attachée ne doit pas disparaître du panel sans être comptée.

Pour les résumés, moyenne absolue, quantiles et maximum **observé** des
variations en rayon se complètent. Le maximum d'un panel n'est pas le
maximum sur toutes les paires. L'emboîtement et la conformité des dates
d'attache restent des vérifications exhaustives sur ces petites scènes ;
les estimer par ce panel ne serait pas suffisant.

## Contrat précis du premier bras K2

Le bras adopté par Claude est bien défini si l'on fige, pour chaque x,

`S_η(x) = { {x,y} : ||x−y||/2 ≤ (1+η) α_min(x) }`,

où `α_min(x)=min_{y≠x} ||x−y||/2`, puis les attaches avant toute coupe.
Toutes les égalités sont conservées, et l'univers contient toutes les
paires incidentes, y compris celles absentes du catalogue. Le milieu d'une
paire est un témoin de L2 à sa demi-distance, même si la paire n'est pas
Gabriel ou si ce milieu était déjà actif auparavant.

Résoudre la composante de chaque milieu à son propre rayon, prendre leur
ancêtre commun J, puis choisir la date

`e(x) = max( naissance(J), max_{p∈S_η(x)} rayon(p) )`.

La cible publiée est **l'ancêtre de J vivant à e(x)**. Cette dernière
précision évite d'attacher une feuille à un nœud dont la branche s'est déjà
terminée avant la date choisie. À partir de cette date, le point suit la
lignée de cette composante. Les attaches ainsi fixées donnent une
hiérarchie laminaire à K fixé, complétée par des singletons avant les
entrées. Augmenter η ajoute des candidats, retarde les entrées et raffine
les partitions à rayon fixé. Cela ne garantit pas un meilleur clustering.

À η=0, un minimum unique retrouve son premier ancrage cover. Plusieurs
minima de même rayon peuvent appartenir à des composantes distinctes :
leur LCA peut différer l'entrée dès η=0. Un départage arbitraire de ces
minima définirait une autre politique. L'ancienne règle à la date core,
qui incluait nécessairement la composante core de x, était quant à elle
un bras conservateur suivant la lignée core après l'attache ; elle ne
justifiait pas les gains d'entrée précoce de cover.

## Stabilité : marge, puis hauteur

Les demi-distances, et leur minimum, changent d'au plus ε si chaque site
étiqueté se déplace d'au plus ε. La comparaison

`g_xy = ||x−y||/2 − (1+η) α_min(x)`

change donc d'au plus `(2+η)ε`. Sous la marge stricte
`|g_xy| > (2+η)ε` de part et d'autre du seuil, les candidats gardent leurs
IDs. Les milieux bougent d'au plus ε ; le transport des composantes de L2,
puis le segment vers le nouveau milieu, donnent la borne conditionnelle
`2ε` sur les dates d'entrée et de réunion. La dérivation et les petites
fixtures exactes sont déjà dans l'addendum de l'audit continu ; ce document
n'ajoute pas une nouvelle qualification expérimentale.

À η=0, la comparaison du gagnant minimal vaut exactement zéro : ce
certificat symétrique ne peut pas s'appliquer. Pour un gagnant unique,
l'écart entre la deuxième et la première demi-distance strictement
supérieur à `2ε` suffit à conserver son identité. Les égalités exactes
restent un cas distinct. Une bande à η>0 peut retenir plusieurs candidats
proches et rendre l'ancrage moins dépendant du seul argmin, mais elle a
elle-même une frontière discontinue. Publier la marge à cette frontière,
ainsi que la fraction de sites effectivement certifiables à ε donné.

Ces garanties supposent les mêmes observations étiquetées, sans suppression
ni fusion de sites, dans le domaine accepté. Elles ne couvrent ni le
rééchantillonnage, ni EOM, ni une nouvelle affectation exclusive, ni les
centres q3/q4. Une invariance des rayons de boules ne suffit pas, à elle
seule, à certifier l'identité de leurs composantes. Les conditions de
consistance statistique sont traitées dans
[`tower_statistical_sources.md`](tower_statistical_sources.md).

## Complément géométrique utile pour un futur K général

Pour le critère précis `dist(x,C) ≤ R`, avec C une composante de
`L_K(R²)` et x un site du nuage, il existe un raccourci mathématique.
Dans le domaine non pondéré et avec un catalogue critique complet,
ce critère équivaut à l'existence d'une boule du catalogue contenant x,
de rayon au plus R, dont le centre appartient à C. Un témoin positif peut
être choisi avec `population≥K` et **`p+q_min≤K`**, renforcement établi
dans le [lemme complet de l'audit continu](../audit_continu_20260929/catalogue/ADDENDUM_COUVERTURE_CATALOGUE_20260929.md).
K1 et le rayon nul
se traitent séparément. Cette équivalence ne remplace pas une requête
générique de distance à C lorsque le seuil est différent de R.

Voici pourquoi. Un témoin y∈C à distance au plus R de x permet de choisir
une K-partie F contenant x dans la boule `B(y,R)`. Son centre de plus petite
boule est relié à y dans l'intersection convexe des boules `B(p,R)`, p∈F,
donc reste dans C. Minimiser le rayon parmi les K-parties contenant x dont
le centre est dans C donne une boule critique admissible : si elle a p≥K intérieurs,
ou si `p+q_min>K`, on peut choisir une autre K-partie contenant x dont
la boule est strictement plus petite. Son nouveau centre reste relié à
l'ancien dans une telle intersection à rayon R, donc dans C, contradiction.
Pour la seconde réduction, moins de q_min sites de coquille ne peuvent
contenir le centre dans leur enveloppe convexe ; déplacer légèrement le
centre suivant une direction qui rapproche strictement tous les sites
choisis de coquille réduit le rayon. Réciproquement, le centre d'une boule
catalogue de population au moins K est lui-même un témoin à distance
au plus R de chacun des sites qu'elle contient.

Cette réduction choisit `K−p<q_min` sites de coquille et les p intérieurs,
en conservant x. Elle réduit l'univers des **témoins de couverture**,
pas le catalogue d'événements de FULL : ses fusions à `p+q_min=K+1`
restent nécessaires. Par exemple, {−2,0,2}, K2, fusionne au rayon 2
par la boule de centre 0, p=1 et q_min=2.

Le catalogue peut donc servir une liste de **composantes** plausibles pour
ce seuil R égal au rayon de coupe de C, en conservant les incidences ex æquo.
Attention : population≥K ne signifie pas que la cellule associée
à la boule est active à cet ordre. Il faut résoudre la composante du centre
par une K-partie témoin, la descente puis la remontée au niveau considéré,
comme le fait `cover_node` dans le snapshot audité
[`tower.cpp`](../../src/tower/tower.cpp#L1563). Ce lemme ne dispense pas le
bras K2 « toutes paires » de ses propres résolutions et marges : remplacer
ses paires candidates par des représentants critiques changerait le
certificat portant sur leurs IDs.

Une conséquence pour le FULL natif est de transporter, pour chaque ordre k et chaque
boule b de population fermée au moins k, toutes les incidences
`(x,k,niveau(b),composante_k(centre(b)))`, pour x∈I_b∪U_b.
Résoudre chaque couple (b,k) une fois à son niveau, puis suivre les ancêtres et
dédupliquer `(x,composante)` à la coupe. La minimisation dans chaque composante prouvée
ci-dessus donne un témoin pour toute couverture à cette coupe. Garder seulement la
première couverture globale, même avec ses ex æquo, omet les autres branches qui
peuvent couvrir x plus tard avant fusion.

Ce raccord concerne le domaine non pondéré qualifié. Compléter K1 par les incidences
des sites au niveau nul ; la table des sites ne doit pas disparaître au profit des
seules boules positives. Les multiplicités dans FULL restent hors de cette qualification.

Le volume de ces incidences est au plus `Kmax·Σ_b |I_b∪U_b|` : borne relative au
payload du catalogue, sans borne générale sous-quadratique. Les requêtes de composante,
remontées et déduplications restent à payer. Ce transport préserve la relation géométrique,
pas automatiquement les poids de faces du §9.1. Un point partagé ne doit jamais suffire
à fusionner les composantes spatiales.

Pour un futur bras qui doit suivre la lignée core après l'entrée, une
condition suffisante est d'inclure la composante core de x parmi les
candidats, à une date où x est actif, puis de prendre le LCA des images
à cette même date. Son attache est alors nécessairement un ancêtre de
cette composante. Sans cette condition, « plus proche de la première
couverture » définit une autre projection, dont la compatibilité avec
la lignée core doit être mesurée et ne découle pas de la distance.

## Coût à conserver dans le prototype

Séparer la production des voisins/paires, la résolution de leur composante,
les remontées/LCA et l'export des dates. Une accumulation du LCA en
streaming évite de stocker toutes les paires, mais ne supprime aucun de ces
calculs. Conserver les comptes et marges nécessaires à la vérification.
Pour un bras par boules, regrouper les boules distinctes à résoudre et
garder les incidences point→boules ex æquo avant de dédupliquer leurs
composantes. Une borne sur les listes de voisins ne borne ni le nombre
de boules critiques, ni les visites d'index, ni ces résolutions.

Pour K2, un η rationnel permet de comparer exactement les distances
carrées au seuil `(1+η)²` sans extraire une racine. Les dates supplémentaires
doivent être représentées explicitement ; les rabattre sur un niveau
double voisin modifierait le bras. Tout plafond de ressources doit rendre
un refus ou une ambiguïté non résolue publiée ; tronquer silencieusement
la liste par ID détruirait sa définition et son certificat.
