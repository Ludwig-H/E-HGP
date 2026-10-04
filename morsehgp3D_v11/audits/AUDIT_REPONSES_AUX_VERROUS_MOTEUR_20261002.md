# Audit mathématique courant : clustering plat et FULL → points

4 octobre 2026. Source relue : **ab1a739d1**. Réponse aux [huit questions](QUESTION_CLAUDE_PREUVES_POINTS_20261003.md),
publiées en 8df2025ab, SHA 9a1003e1. Cadre :
`exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only /
not_claimed`. [Preuves détaillées et gardes reproductibles](../receipts/points_answers_20261003/README.md).
Cette note remplace le conseil antérieur ; reçus clos et Git conservent les
preuves précédentes, sans journal supplémentaire dans audits/.

**La règle intérieure à k fixé est solide ; les huit questions demandent des
réponses de portées différentes.** Stabilité et retard qualifié sont prouvés.
L'optimalité est intrinsèque, pas encore géométrique. L'égalité asymptotique n'est
pas prouvée ; une obstruction de Palm donne un déficit sous hypothèses à k=2.
Les ordres ne s'emboîtent pas automatiquement. L'impossibilité
générale de Q6, telle qu'écrite, est fausse. ER0h n'a pas de constante uniforme
pour ses dates, en rayon. Le port natif des nouvelles dates reste à qualifier.
La session F qualifie le banc exact C++/Python, pas ce port. Deux verrous
supplémentaires : choisir la métrique d'insertion et justifier le modèle infini.

## Chantier actif — choisir une partition plate

Le workflow privé `wf_fb625b66-561`, sous `build/v11-points-select/`, traite
modèle, équité, mesures et LiDAR. Il possède déjà une condensation N-aire,
un DP d'antichaîne et des politiques de bruit/racine. Aucun de ces prototypes
n'est intégré au moteur ab1a. [Sources et contre-gardes](../receipts/flat_selection_contract_20261004/README.md).

**Conseil : garder une tête simple comme référence**, masses entières des
points engagés, condensation mcs≥2, plateaux exacts, EOM à λ=1/r, racine
exclue, parent sur égalité certifiée, points non affectés en bruit. Mesurer
feuilles, λ=1/r³ et complétion comme variantes déclarées ; aucun optimum
statistique ne découle du seul arbre. Les masses de faces fractionnaires de
la thèse changent ce modèle et perdent les triangles à mcs=3.

**z est un choix de modèle, pas une simple unité.** Sur H qualifié k=2/m=3
des neuf sites A={0,2,4}, B={7,9,11}, D={17,19,21}, toutes les entrées valent
2, A/B fusionnent à 5/2 puis avec D à 4. À mcs=3, racine exclue :
λ=1/r choisit A∪B et D ; λ=1/r³ choisit A, B et D. Les scores et toutes
les coupes Γ₂ sont exacts. À une égalité parent/enfants, une perturbation
minime peut faire basculer la partition malgré H3. Publier le gain de score
parent/descendants et son erreur permet une garantie **avec marge**, sous
correspondance de la condensation. [Preuve et fixtures](../receipts/flat_selection_math_20261004/README.md),
[précision sur les sorties condensées et variante entière](../receipts/flat_selection_math_r2_20261004/README.md).

Le niveau B ne suffit pas : le vrai évaluateur peut choisir parent et enfant
pour deux objets ; témoin à six points, moyenne 5/6 contre au plus 1/2 pour
une antichaîne. La sortie doit donc être une **antichaîne globale**, puis un
appariement des objets un à un. Conserver meilleur-IoU comme diagnostic.

Deux contrats facilitent le port. Pour mcs≥2, on peut ignorer les naissances
des singletons : e_i≤u(i,j) garantit que tout bloc non trivial est engagé.
Il faut néanmoins l'arbre **de points**, avec les fusions créées par des
attaches entre niveaux FULL, pas les seules populations couvertes de FULL.
Après condensation, un point peut sortir avec sa petite branche **avant**
λ=e_i^(-z) ; ne pas le compter jusqu'à cette dernière date dans l'EOM.
La complétion par lignée ne rend pas aux enfants un point ancré dans leur
parent : publier ses gains de couverture et pertes de pureté séparément.

Le prototype flottant possède un témoin où le parent gagne à tort ; celui à
intervalles force le parent au budget avec un compteur. Une sélection n'est
certifiée optimale que si ce compteur est nul. L'ordre exact des dates ne
qualifie pas les **sommes de réciproques** de l'EOM : contrat de signe,
égalité et refus propre à la tête, puis portes de plateaux, racine/bruit,
permutation et conservation des IDs. Ces réserves concernent les prototypes
épinglés, sans défaut attribué à un port natif encore absent.

**Comparaison équitable à fixer avant G4.** Garder HDBSCAN officiel intact,
puis un bras séparé où la même tête atomique est appliquée aux deux arbres.
Le pilote privé à epsilon=0 compte 520 partitions différentes sur 2 400
configurations après normalisation des plateaux ; le total 1 015/6 752 mêle
sorties officielles et transcriptions de secours après exceptions. Versions
locales 1.9.1, contre 1.7.2 dans F : aucune qualification transférée.
[Archives, dénominateurs et lecture indépendante](../receipts/flat_selection_evidence_20261004/README.md).
La métrique hongroise vérifie sa somme et une borne par maxima de lignes ;
cela ne constitue pas le certificat dual d'optimalité annoncé. Corriger
cette revendication ou fournir un vrai certificat, sans déclarer faux les
scores observés. Le pilote EOM capturé a zéro arbitrage forcé.

## Q1 — Chaîne complète de stabilité

[Preuve complète](../receipts/points_answers_20261003/tower_math/README.md) :
identifiants appariés, sites distincts dans chaque nuage, poids unitaires,
k et m fixes, m≤n, déplacement maximal ε ; boules et coupes fermées,
nuages finis, racine prolongée après sa naissance.

Les k témoins de z∈L_k^X(r) restent témoins dans Y au rayon r+ε.
L'inclusion L_k^X(r)⊆L_k^Y(r+ε) envoie chaque composante connexe C dans
l'unique composante qui la contient. Les cartes φ,ψ commutent aux remontées
et vérifient **ψφ=Up(2ε), φψ=Up(2ε)**. Un témoin c∈C à distance≤r de x_i
reste dansφ(C), à distance≤r+ε de y_i. Tous les identifiants couverts se
transportent ainsi ; le seuil de cardinal m est conservé dans les deux sens.
Il n'y a ni bijection de nœuds critiques ni conservation exacte des cardinaux.
Les plateaux N-aires sont déjà ceux des composantes de la coupe fermée.

Avec t′ première couverture qualifiée et D la persistance rivale maximale :
**|Δt′|≤ε, |ΔD|≤2ε, |Δe|≤3ε, |Δu(i,j)|≤3ε**. Pour les réunions, le transport
des attaches finales g donne m_Y(φg_Xi,g_Yi)≤e_Xi+3ε ; l'ultramétrie par la
chaîne g_Yi→φg_Xi→φg_Xj→g_Yj puis la symétrie concluent. Pκ possède la borne
(1+2κ)ε. Cela ne garantit pas stabilité de l'IoU ni d'une sélection de labels.

La preuve mathématique s'étend aux **poids positifs fixes appariés**, en
comptant la masse déclarée ; compter des retours n'est pas compter des sites
distincts si les classes de coïncidence changent. Le moteur FULL actuel reste
unitaire. Insertion/suppression : pas de stabilité à k/m fixes déduite de P5.
Contre-exemple qualifié : {0,2,4},k=2/m=3, entrée de0 à 2 ; ajouter η∈(0,1) près
de 0 la fait entrer à1, bien que Hausdorff=η→0. Les anciens points sont fixes.

La constante 3 est atteinte **abstraitement** : profils (1,3,6) et
(5/4,11/4,25/4), entrelacementδ=1/4, entrées 4 et 19/4. Pas de réalisation
géométrique de cette égalité démontrée.77 gardes bornées normal/−O.

## Q2 — Qualification et optimalité

**Oui** si la règle est définie sur tous les profils abstraits après
qualification, avec localité au profil fixé, équivariance par isomorphisme,
entrée immédiate sans rival et Lipschitz intrinsèque. Le théorème C s'y
applique ; la monotonie donne la précocité globale point par point de P1.

**Non par simple transport de Π_m** si le domaine est seulement l'image de
nuages géométriques qualifiés. Les comparateurs C1 peuvent être hors de cette
image ou non réalisables au déplacement annoncé. La borne supérieure se
transfère ; la preuve inférieure doit respecter le domaine et sa métrique.
Garde : n=k+1,m=k+1, chaque profil qualifié est une seule remontée née à la
MEB globale ; la règle est alors **1ε-stable**. Cela ne tranche pas l'optimum
uniforme sur tous n ; cela réfute l'inférence automatique « profil transporté,
donc constante minimale 3 ».

## Q3 — Retard qualifié et temps de cœur

Le lemme L6 est démontré par le segment du témoin de couverture vers x :
une composante couvrante au rayon r rejoint la composante contenant x avant
r+d_k(x)/2. Toutes les composantes qualifiées à la même coupe rejoignent
cette composante ; restreindre le profil n'augmente pas ce temps de résolution.
Ainsi **0≤e−t′≤d_k/2**, **t′≤d_(k+1)** et
**e≤d_(k+1)+d_k/2**. Aucune borne par un facteur universel de d_k n'en découle.

Condition exacte de retard après le cœur : t′+D>d_k ; si t′≤d_k, une barre
qualifiée doit persister plus longtemps que d_k−t′. L'ancien récit « seulement
les petits amas » est réfuté par [huit sites](../receipts/hm_followup_20261003/check_qualified_delay.py) :
x est cœur, d₂=10 et t′=10 dans un amas de quatre sites, mais entre à
√250−5/2≈13,311, après une fusion parasite F=12. Le rival qualifié naît après F.

Pour une composante C contenant x à F, **t′+d_k/2≤F** garantit sa présence
à la coupe **fermée** F. Pour récupération strictement **avant** le plateau
parasite, demander **t′+d_k/2<F**. La livraison 6c88 adopte cette distinction.

**Conseil pour E1 : κ=1 contre κ=2.** À profil qualifié identique,
κ₂≥κ₁≥1 implique eκ₂≤eκ₁ et Hκ₁(C,r)⊆Hκ₂(C,r), pour chaque composante
FULL C à la même coupe. Le rappel dans C augmente ; **pas nécessairement l'IoU
ni le résultat de la sélection**. Mesurer t′, D, e−t′ et la part e>d_k, en plus
du meilleur IoU. Sur R1, κ=2 fait entrer x à √250−5<F=12, contre
√250−5/2>F pour κ=1. [Preuve et gardes](../receipts/points_math_followup_20261004/README.md).
Pour κ>1, L6 exclut les rivaux nés à h≥t′+d_k/[2(κ−1)] ; les rencontres
pertinentes ont m≤t′+κd_k/[2(κ−1)]. Cet horizon en rayon ne prouve aucune
localité spatiale ni baisse de coût de tout le pipeline.

## Q4 — Asymptotique du chapitre 7

[Lecture primaire de la thèse et théorème conditionnel](../receipts/points_answers_20261003/evidence_head/Q4_REPONSE.md).
Dans une composante C de L_k(r), H_C⊆E_C. Tous les sites x∈C avec
t′(x)+d_k(x)/2≤r sont dans H_C. La perte est majorée par deux masses :
**frontière couverte hors de C**, et **sites de C ne vérifiant pas cette condition**.
Si leur somme B_A(C_n,r_n)/|X_n∩A|→0, avec r_n<F_n et les hypothèses de
convergence FULL du chapitre 7, alors H et FULL récupèrent la même fraction limite.

Cette disparition n'est pas démontrée à K et contraste fixes : r_n,d_k,t′
ont la même échelle n^(-1/p). Un retard absolu tendant vers 0 n'est pas un
retard relatif négligeable. La définition de H sur le processus infini,
la loi de Palm et les seuils/continuités nécessaires doivent aussi être justifiés.
Une grille 1 mm fixe ne reproduit pas ce régime continu n→∞.

**Complément : obstruction locale ouverte à k=2/m=3**
([preuve et contrelecture](../receipts/palm_obstruction_20261003/README.md)).
Un patron perturbable, protégé contre tout extérieur, impose e(x)≥12,3>F=12,
alors que x est déjà core d'une primaire raccordable à une composante infinie.
Pour un Poisson homogène supercritique à intensité et rayon fixes, un vide local
et un corridor d'insertions uniformes donnent à cet événement une probabilité
de Palm positive. **Si H∞ est défini, mesurable et fidèle**, cela implique
ΘH<Θpoly aux **mêmes** λ/F. Le nerf localement fini, la protection contre
les chemins extérieurs et le raccord ont été relus indépendamment ; 28 gardes
exactes vérifient le patron, sans simulation Poisson.
Ce résultat conditionnel ne prouve ni la convergence des fenêtres finies,
ni une perte uniforme lorsque λ varie, ni les fractions avant les seuils
critiques propres des deux méthodes. Il faut donc analyser ΘH séparément.
Le motif montre le coût de la **qualification avec ancrage persistant** ; il
ne démontre pas que la qualification seule impose cette perte.
Le registre ab1a, ligne 1312, emploie encore « fraction limite » : écrire
« probabilité de Palm aux λ/F fixes » tant que la convergence des fenêtres
finies reste ouverte, conformément à sa propre réserve.

**Le prolongement infini est un vrai verrou**, pas une formalité.
[Contre-exemple déterministe localement fini](../receipts/points_math_followup_20261004/README.md) :
deux rangées symétriques de paires, plus x=0, donnent à k=2/m=3 deux
premières couvertures qualifiées au même t′=√10121/200. Elles restent séparées
à r=1, mais se rejoignent pour chaque r>1 par des ponts de plus en plus
distants. L'infimum de rencontre 1 n'est **pas atteint**. La formule Pκ publie
e=1 ; les deux choix de primaire ont pourtant deux propriétaires distincts
à la coupe fermée 1. Les propriétaires des préfixes finis sont atteints à des
rayons décroissant vers 1 ; prendre cette limite ne restaure pas H1.
Il faut prouver les atteintes/continuités nécessaires presque sûrement pour
Poisson, ou déclarer une autre convention infinie. Ce témoin n'est pas un
événement de Palm ; il ne réfute ni H1 sur nuages finis ni le théorème
conditionnel précédent. 52 gardes exactes et preuve infinie séparée.

Pour la fermeture CL, sous existence des modèles infinis et du bloc géant :
**Θpoly(λ)≤ΘCL(λ)≤Θpoly(2^pλ)** et
λcCL≤λcpoly≤2^pλcCL. Les fractions avant leurs propres fusions évaluent Θ à des
arguments différents ; aucun ordre universel des rappels n'en découle.
Si a=ρλcCL, les deux fractions sont dans[Θpoly(a),Θpoly(2^p a)]. La fixture
finie 1 contre 1/2 ne tranche pas la limite statistique.

## Q5 — Verticalité : contre-exemple qualifié

[55 gardes exactes](../receipts/hm_followup_20261003/check_vertical.py) sur les
six sites collinéaires 0,3,6,18,19,20, au même rayon 7 :

| Ordre et seuil | Blocs P1 qualifié |
|---|---|
|k=2/m=3 |{0,3,6} et{18,19,20} |
|k=3/m=4 |{6,18,19,20} |

La dernière branche traverse les deux premières. Chaque règle est laminaire
à k fixé ; leur famille commune ne l'est pas. Ce témoin suffit, sans preuve
qu'il soit minimal en nombre de sites.

Le théorème D v10 exige **aussi** l'entrée immédiate en couverture non ambiguë.
Fidélité, laminarité et verticalité seules restent compatibles. Transporter
les attaches d'un ordre supérieur donne des propriétaires compatibles aux
ordres inférieurs, au prix de leurs propres entrées précoces. Cela constitue
une piste explicite, sans optimalité statistique ni synthèse canonique acquise.

## Q6 — L'impossibilité générale proposée est fausse sous ces trois axiomes

[Construction vérifiée](../receipts/points_answers_20261003/geometry_catalogue/README.md) :
pour k≥2, N=6→P1Π3, N=4→P2Π1 ; P1Π1 ailleurs, liaison simple à k=1. N est conservé
par l'appariement. Chaque voie est fidèle et équivariante ; sa constante est
≤3 ou≤5, donc la règle entière est **uniformément 5ε-stable à N fixé**.

T0 et Q1bis ont six sites : ABC|DEF avant la racine. Q2 a quatre sites :
{x,a}|{b1,b2} sur[61,75], e_x≈54,844. Les cibles datées, leurs variantes et
les invariances ont été recoupées. Cette construction ne lit pas mcs.

Ce choix par N est une démonstration, pas un modèle proposé pour le produit.
Il ne garantit pas stabilité sous insertions, localité au profil brut ou
entrée immédiate brute sans rival. Une impossibilité intéressante doit donc
énoncer les axiomes supplémentaires qui l'excluent. Les théorèmes E/F ne
portent pas sur toutes les règles des seuls trois axiomes de Q6.

**Relance du développeur** ([6c88, axiomes renforcés](REPONSE_CLAUDE_POINTS_20261003.md)) :
la métrique d'insertion doit être déclarée. Si ajouter un site à distance η d'un
ancien coûte au plus η (Hausdorff, anciens sites fixes), la stabilité uniforme
et l'entrée immédiate sans rival qualifié sont déjà incompatibles à k=2/m=3.
Pour le site 0, {0,2,4} a une seule remontée qualifiée née à 2 ;
{0,η,2,4}, 0<η<1, une seule née à 1. La branche {2,4} meurt à 2−η/2
sans avoir été qualifiée ni avoir couvert 0 ; elle rejoint alors le parent
de la première composante, qui couvre 0 par héritage.
Toute règle à entrée immédiate doit donc donner 2 puis 1 : saut 1 pour η→0.
Le même profil vaut pour l'ancien site 2 ; la réunion u(0,2) vaut aussi 2
puis 1. Le défaut concerne donc également les réunions hors diagonale.
Ce contre-exemple ne dépend ni de la localité ni de T0/Q1bis/Q2 ; les
[gardes exactes de Q1](../receipts/points_answers_20261003/tower_math/README.md)
vérifient aussi ces dates. Si l'insertion coûte sa masse unitaire, ou si une
distance de mesures pénalise le changement de masse, elle n'est pas petite
quand η→0 : aucune impossibilité ne découle alors de ce témoin.

**Relance ab1a : distance de mesures.** Pour tout p fini, Wasserstein sur les
mesures empiriques **normalisées** ne suffit pas à contrôler uniformément le
maximum des dates des anciens IDs, si k=2/m=3 reste un compte de sites unitaires.
Prendre X_N={0,2R,4R}∪{6R+jR/N : 0≤j≤N−4}, et Y_N=X_N∪{η}, 0<η<2R.
Le profil de 0 est une seule remontée qualifiée, née à 2R puis R : l'entrée
immédiate impose un saut R. Un couplage donne W_p≤7R/(N+1)^(1/p), donc aucun
Lipschitz uniforme en N, même sur [0,7R] fixé.
[Preuve de la lentille et couplage](../receipts/measure_metric_20261004/README.md).
Cela ne tranche ni W∞, ni une erreur L¹ pondérée des dates, ni des seuils
définis sur les masses normalisées, ni une constante dépendant du domaine u21
fini. La reformulation doit choisir ensemble masse, seuils et erreur de sortie.

## Q7 — Relecture indépendante de C et S

**C confirmé**, avec les réserves de domaine de Q2 : entrelacements C1,
borne inférieure intrinsèque3, monotonie pour l'optimalité globale.

**S confirmé pour ER0h(η=1,κ=12)**, en niveaux carrés. Une famille complémentaire
prouve le défaut en rayon : pour κ>0 fixé, A entier≥max(48,4κ),
L=Aρ^4,h=Aρ^3, nuage{0,(L,h),(L,−h)} ; déplacer le dernier site de(1,0).
Le saut de date est **≥κρ²/4−1**, pour déplacement maximal 1. Aucune constante
indépendante de l'échelle/aspect n'existe pour les dates ER0h. 6113 gardes
indépendantes, normal/−O, avec géométrie et votes reconstruits.

Cette preuve concerne les entrées, ou la diagonale si elle encode l'entrée.
Elle ne fournit pas une nouvelle preuve universelle des seules réunions hors
diagonale. Une grille finie u21 admet des bornes dépendant de son étendue.
Le maximum fini 69,5 d'ER0hr ne prouve pas une absence de borne uniforme pour
cette variante : S traite le cône absolu d'ER0h. Le verdict 125/125 reste un
résultat de fixtures ; il ne qualifie pas la robustesse uniforme du modèle.

## Q8 — Contrat du port natif

[Type, algorithmes et budgets complets](../receipts/points_answers_20261003/root/Q8_CONTRAT.md).
Type proposé **PointRadiusDate**, trois rangs dans un domaine immuable ;
égalité algébrique, pas égalité des triplets. Ordre commun avec les niveaux
FULL, propriétaires de la coupe fermée, refus transactionnels, scratch compté.

| Profil | Niveau num/den | Quatre racines, produit final | Test de classe |
|---|---:|---:|---:|
|u18 |156/116 bits |2022 bits |272 bits |
|u21 |180/134 bits |2334 bits |314 bits |
|u24 |204/152 bits |2646 bits |356 bits |

Pour six racines, zéro est certifié par classes de carrés rationnelles.
Si non nul, une borne par norme algébrique donne des précisions suffisantes
45996/53097/60198 bits pour u18/u21/u24 ; **bornes extrêmes conservatrices**,
sans cas Cloud atteignant ces valeurs ni prévision de coût.8192 bits reste
un budget avec refus, sans preuve de complétude de tout le domaine.
Le noyau actuel limite Int à 1024 bits et Wide à 2048 : raccord arithmétique
spécifique nécessaire, sans élargissement global automatique. 12824 gardes
scalaires vérifient les réductions et tables ; aucun port natif exécuté.

Le format exporté à 192 bits couvre le majorant u21 ; le majorant u24 est de
204 bits. La complétude u24 exige donc un format élargi ou une réduction
prouvée. Ces majorants ne démontrent pas qu'un nuage les atteint ; aucun
transfert de qualification depuis u21.

## Suivi de l'implémentation

Le développeur a adopté rayon, égalités certifiées/refus, filtres absolus,
portée intrinsèque de H4, correction de H5, exception m=1 à k=1 et contexte
Decimal unique. Ces points ne sont plus des défauts ouverts de l'ancien WIP.
La [réponse courante](REPONSE_CLAUDE_POINTS_20261003.md) adopte Q1–Q8 et les
compléments d'insertion/Palm ; le port natif Q8 n'est pas commencé.
**Session F conforme**, commit poussé f02f91c7e, sources jouées identiques à
ab1a : export FULL natif u21 + consommateur rayon 457/oracle 2f05 exacts en
Python. 2 854 nuages, 194 520 comparaisons, 215 974 comparaisons de sites répétées,
12 fixtures dont le plateau √2+√18−√8=√8, quatre mutants causaux.
Ces mutants modifient **Python**, avec exporteur C++ inchangé ; « tués
nativement » dans la réponse du développeur est à corriger. Porte bornée à
neuf sites/k≤4/m≤n ; ce n'est pas la qualification d'un futur PointRadiusDate
natif. [Archives et 4 343 contrôles normal/−O](../receipts/points_gate_qualification_20261004/README.md).
Le correctif 2f05 et ses [20 831 gardes indépendantes](../receipts/points_answers_20261003/owner_fix_review/README.md)
sont désormais aussi exercés par ce banc G4 ; l'ancien défaut de propriétaire
Decimal est clos. E conserve son échec de dossier manquant, sans mesures.

[Limite d'API relue](../receipts/points_code_review_20261004/README.md) : m>n
refuse `jamais_qualifie`, domaine sauté par la porte. Déclarer 1≤m≤n, ou
prévoir des points inactifs sans date/propriétaire pour une API générale.
Les anciennes [campagnes pts3/pts4](../receipts/pts4_review_20261003/README.md)
restent historiques ; leurs observables et sources sont recoupés avec F dans
la note de contrats, sans transformer les juges anciens en juges exacts.
[Mesures historiques recoupées](../receipts/hm_review_20261003/README.md) ;
[état du moteur et contrats](AUDIT_CONTRATS_NUMERIQUES_ET_CAPACITE_20261002.md).
La fermeture extérieure reste une borne canonique et un témoin stable,
[avec raccord direct prouvé](../receipts/full_points_20261003/README.md),
sans remplacer les propriétaires FULL nécessaires à une pendaison fidèle.
