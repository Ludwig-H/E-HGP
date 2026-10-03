# Réponses au développeur : FULL → points, Q1–Q8

3 octobre 2026. Réponse aux [huit questions](QUESTION_CLAUDE_PREUVES_POINTS_20261003.md),
publiées en 8df2025ab, SHA 9a1003e1. Cadre :
`exploration_v11_hors_registre / cpu_reference / quantized_u21_input_only /
not_claimed`. [Preuves détaillées et gardes reproductibles](../receipts/points_answers_20261003/README.md).
Cette note remplace le conseil antérieur ; reçus clos et Git conservent les
preuves précédentes, sans journal supplémentaire dans audits/.

**La règle intérieure à k fixé est solide ; les huit questions demandent des
réponses de portées différentes.** Stabilité et retard qualifié sont prouvés.
L'optimalité est intrinsèque, pas encore géométrique. La récupération asymptotique
est conditionnelle ; les ordres ne s'emboîtent pas automatiquement. L'impossibilité
générale de Q6, telle qu'écrite, est fausse. ER0h n'a pas de constante uniforme
pour ses dates, en rayon. Le port natif des nouvelles dates reste à qualifier.

## Q1 — Chaîne complète de stabilité

[Preuve complète](../receipts/points_answers_20261003/tower_math/README.md) :
identifiants appariés, sites distincts dans chaque nuage, poids unitaires,
k et m fixes, m≤n, déplacement maximal ε ; boules et coupes fermées,
racine prolongée après sa naissance.

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
parasite, demander **t′+d_k/2<F**. La correction de H5 est adoptée dans le WIP ;
sa formulation « avant F » doit encore distinguer cette égalité.

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
Le nouvel oracle compare aussi dates symboliques et propriétaires : progrès
utile, encore à qualifier sur la source effectivement jouée sur G4.
[Contre-garde exacte](../receipts/points_answers_20261003/owner_plateau/README.md) :
sur quatre sites collinéaires, k=2/m=1, la date √2+√18−√8=√8 tombe exactement
sur une fusion. L'oracle ab200 en Decimal120 la place 1E−119 avant et garde
l'enfant mort ; le helper exact 457 prend correctement le parent fermé.
42 gardes normal/−O. Le contrôle m3 est correct ; aucun défaut général de
la règle retenue n'est démontré par ce cas.
[Correctif 2f05 vérifié séparément](../receipts/points_answers_20261003/owner_fix_review/README.md) :
rival et propriétaire désormais exacts, indépendants de Decimal ; 20 831
gardes normal/−O, dont égalité et voisins stricts, transformations du nuage
et trois précisions d'affichage. **Réserve soldée dans ce périmètre Python**.
Une augmentation de précision seule n'aurait pas fermé le contrat.

La campagne pts3 est close sur **échéance globale**, arrêt ciblé certifié :
son échec ne démontre pas un défaut natif. pts4 joue le correctif 58952 et
reste non close à notre dernière lecture. Le 457 ne change que sa docstring ;
en revanche le juge exact et l'oracle 2f05 ne sont pas ceux du paquet pts4.
Les campagnes pts1/2 portent sur
la marge carrée ; leurs succès ciblés ne se transfèrent pas à la règle rayon.
[Mesures historiques recoupées](../receipts/hm_review_20261003/README.md) ;
[état du moteur et contrats](AUDIT_CONTRATS_NUMERIQUES_ET_CAPACITE_20261002.md).
La fermeture extérieure reste une borne canonique et un témoin stable,
[avec raccord direct prouvé](../receipts/full_points_20261003/README.md),
sans remplacer les propriétaires FULL nécessaires à une pendaison fidèle.
