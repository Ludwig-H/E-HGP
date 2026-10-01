# Au développeur ancrage frontière et corrections du raccord

## Réponses actuelles au développeur

1er octobre 2026, actualisé à05 h50 UTC. Relance de l'utilisateur sur les questions
du développeur : relecture intégrale du [contact](../REPONSE_CLAUDE_CONTACT_COMPTAGE_ET_JUGES_20260930.md#7-questions)
et recoupe des sections Questions des mémos privés principe libre et ER.
Complément de port : comptages distincts, admissibilité et échelle ER
sans fermeture dense. Les180 cardinalités générales ont désormais un
reçu clos ; l'antichaîne/admissibilité/sigma reste un diagnostic RAM
ouvert distinct. Nouvelle tête et collecteurs contre-jugés ci-dessous.
Les trois questions techniques explicitement adressées aux auditeurs
restent Q1/Q2/Q3 ; elles sont répondues ci-dessous. Les
questions « à l'utilisateur » ne deviennent pas des choix acquis.
Sources moteur inchangées, GCP non utilisé, `public_status=not_claimed`.

### Les trois questions sur les votes

1. **Q1, classes locales.** Les seuls K voisins ne suffisent pas.
   Pour une K-partie F contenant x et de MEB de rayon≤R, chaque
   site de F appartient à B(x,2R). Une requête complète de rayon
   fournit donc un univers local sûr. Avec m sites, les supports
   minimaux en3D ont au plus quatre points : O(m⁴) classes au plus,
   et non O(K⁴) ni une borne sous-quadratique en n. Les occupations
   LiDAR déjà mesurées interdisent de remplacer m par K.
2. **Q2, univers continu pour K≥3.** Les K-parties à identités fixes
   conservent le modèle exact. Grouper les parties ayant la même
   MEB est possible seulement en conservant leur multiplicité
   exacte et leurs affectations. Ni une liste fixe K-NN ni un vote
   par boule forte du catalogue ne sont équivalents. MMt/MMtA
   représentent une autre mesure, fondée sur les durées FULL :
   alternative à tester, pas raccourci transparent du comptage.
3. **Q3, poids relatifs au rayon.** Oui : sur les parties fixes,
   ces poids évitent le saut provoqué par le retrait d'une boule
   forte au catalogue. Le résultat reste conditionné au transport
   des propriétaires, à une ancre positive continue, à une rampe
   nulle au bord et à la date de majorité à marge. Une rampe ne
   répare pas un univers de votes variable ou une échelle qui saute.

La [réponse détaillée](#réponses-aux-trois-questions-sur-les-votes-de-bande)
garde les hypothèses du théorème de transport. Pour la demande du
contact§5 : le transport S des propriétaires a été contre-relu sous
ses deux applications compatibles ; ce n'est pas une preuve de stabilité
pour un univers variable de boules fortes. La construction N doit encore
intégrer la correction angulaire publiée : son égalité géométrique au
bord g=1 est fausse. L'obstruction limite en nombre de votes n'est pas
pour autant supprimée. Ne pas confondre Q3
avec une future rampe ER : son échelle dépend encore d'un maximum
sur un ensemble de points co-couverts qui peut changer brusquement.
ER-pref ajoute un repli dur quand sa dernière structure propre
admissible disparaît. Même avec une ancre positive, si TOUS les
poids d'une variante douce tendent vers zéro, leurs proportions
peuvent garder plusieurs limites : la majorité normalisée est
invariante par multiplication commune par ε>0. Il faut alors
prouver que le repli rejoint ces limites ou conserver une masse
de fond positive ; aucun tel correctif ER n'est qualifié ici.

### La compression utile est celle des lignées

**Un coût caché est désormais clos et reproductible.** La
[sonde étoile ER](../../receipts/audit_continu_20260929/er_entry_star_20261001/README.txt)
exécute la vraie AST `StructureER.__init__`, source99e8, sur un
FULL1 réalisable par n sites entiers consécutifs. Sa fusion unique
fait payer n(n+1)/2 recherches d'enfants pour seulement2n
incidences couvrantes. Partir des nœuds couverts puis retirer
le parent de chacun donne exactement les mêmes entrées en2n
opérations de retrait. Vingt cas n2/8/32/128/512, permutations
comprises ; vingt mutants sémantiques « retirer soi-même » refusés.
Lecteurs et replays indépendants normal/−O passent, sortie reproduite
octet pour octet. Manifeste externe
`0e02c869cbc5ba9a116aed4016045a3e80b6aee3b26b90a0c4fed439a26b17de`.
Les lignes8k/16k/32k sont analytiques seulement. K1 a Ax=0 :
aucune exécution de la règle ER à ancre positive, du moteur natif,
de K5/LiDAR ou de G4. Le tri et les autres préparations restent payés.

**Proposition de port exact pour MMtA complet.** Ne plus développer
tous les ancêtres de chaque point. Pour un point, conserver ses L
feuilles couvrantes minimales COMPLÈTES, leurs débuts de couverture
et leurs plus proches ancêtres communs. Cet arbre virtuel contient
au plus2L−1 sommets ; son arête supérieure représente aussi la
suite de la lignée après le dernier branchement. Il ne supprime
aucun nœud de la tour FULL livrée.

Sur une chaîne où un seul enfant couvre x, la première couverture
du parent commence à sa naissance. Jusqu'au premier niveau
d'admissibilité H de cette lignée, A(v)=H ; aucune nouvelle rivale
couvrant x n'apparaît dans les branches latérales. Rt et omega
restent donc identiques AVANT H. À H, la pente de la masse devient1,
même si l'identifiant du porteur change ensuite. Les contributions
de durée des nœuds successifs se télescopent.

Pour calculer les rivales sans remonter une chaîne par porteur,
poser h(l)=max(c(l),A(l)) et Ah(i)=min h(l) dans chaque sous-arbre
virtuel i. Au vrai branchement P de naissance b(P), une branche i
offre r(P,i)=max(b(P),Ah(i)+λAx) si Ah(i)<b(P), et aucune rencontre
sinon. Transmettre à l'enfant i le minimum de Rt(P) et des r(P,j)
pour j≠i. Les minima préfixe/suffixe traitent tous les enfants en
temps linéaire en leur nombre.

**Événements à conserver :** départs de couverture, breakpoint H,
bord E2 et vraies fusions de lignées. Les fusions APRÈS E2 restent
nécessaires à la marge et à la première unanimité ; ne pas poser
T1=E2. Calculer Ah/Rt avec toutes les feuilles, même celles dont
c≥E2 : elles peuvent peser comme rivales sans apporter de masse
dans la bande. Le propriétaire final doit être retrouvé dans le
vrai FULL par requête d'ancêtre à la date exacte, en coupe fermée,
pas par un identifiant virtuel.

Cela propose O(L log L + L·coût_LCA) pour la construction, puis
O(L) pour les rivales et O(L) événements, hors tris et coût des
rationnels exacts. Ce n'est pas encore un port natif validé, une
borne globale sur L ni une mesure sous-quadratique LiDAR.
Les admissibilités globales plafonnées à mcs et l'index Euler/LCA
restent utiles ; éviter de reconstruire leur fermeture dense.
La largeur déjà observée de W exige aussi des comparaisons
certifiées à repli exact, pas un choix arbitraire de mot entier.

**À corriger dans la nouvelle sonde `compression.py` e361f3e.**
Elle compte les feuilles et branchements, sans recalculer les
masses ou la majorité. Sa taille « compressés » est celle d'un
squelette, pas de l'inventaire complet des événements : H manque.
Sa phrase « même A, même pente » doit être limitée à l'avant-H.
Ce n'est pas une erreur démontrée du moteur ; c'est une réserve
sur l'interprétation de cette mesure exploratoire.

### La fermeture des ancêtres est un coût évitable

La [preuve peigne K5 et K10](../../receipts/audit_continu_20260929/ancestor_closure_comb_20261001/source/README.txt)
est désormais close, capturée une seule fois puis rejouée indépendamment
par l'auditeur. Pour les sites entiers x_j=(j(j+1)/2,0,0), les fenêtres
consécutives de K points donnent toutes les feuilles FULL_K. Les
réunions de fenêtres successives ont des niveaux strictement croissants.
Il y a H=2(n−K+1)−1 nœuds et K(n−K+1) graines couvrantes fortes,
mais développer les ancêtres de chaque point produit
D=K(n−K+1)+(n−K)(n+K+1)/2 incidences.

| n | K | Nœuds H | Graines fortes | Incidences développées D | Γ exhaustive |
| ---: | ---: | ---: | ---: | ---: | --- |
| 8 | 5 | 7 | 20 | 41 | oui |
| 16 | 5 | 23 | 60 | 181 | oui |
| 32 | 5 | 55 | 140 | 653 | non |
| 16 | 10 | 13 | 70 | 151 | oui |
| 32 | 10 | 45 | 230 | 703 | non |

La vraie AST `Tree/native_tree/check_cover/gamma_tree/witness_universe`
est utilisée avec un export analytique1D, une MEB exacte1D et un DSU
adaptateur. Ce n'est pas l'exporteur natif. Le lecteur recalcule les
signatures de forêt et couverture par formules indépendantes. Les
trois petites Γ exhaustives concordent ; aucune Γ32 n'est développée.
Lecteurs et replays normal/−O passent, payloads identiques octet pour
octet ; faux pin et neuf corruptions sémantiques en RAM sont refusés.
Source9 fichiers au manifeste externe
`f6cc418c9f37cdaea88d49eb593e5184956ceb8b78f3b47f37ffce8d1995d7d7` ;
[capture4 fichiers](../../receipts/audit_continu_20260929/ancestor_closure_comb_20261001/capture/run_receipt.json)
au manifeste externe
`08c45e0adbe5be64b7f460f60cb824090c95e3cbc23b4976a817a65f203881ad`.
Le lecteur est LIVE pour le SHA/version de Python, sans rejeu automatique.
Le README source décrit sa préparation antérieure à cette capture ; il
n'est pas réécrit après clôture.

**Filtrer seulement par la bande ne suffit pas en général.** Sur ce
peigne, pour mcs≤K, la bande conserve quadratiquement beaucoup de
préfixes lorsque √(1+η)(K−1)>K : c'est le cas K10/η1/3 et K10/η1/2.
Cette déduction est analytique, pas un cas exécuté ni une mesure LiDAR.
À l'inverse, chaque point a au plus K feuilles couvrantes minimales :
son arbre virtuel reste O(K). Le carré vient donc d'une représentation
dense évitable, pas d'une impossibilité de compression exacte.
La famille tient dans u18 jusqu'à n724 ; l'asymptotique requiert un
domaine entier élargi. Elle ne démontre aucune croissance LiDAR/G4.

**Attention au chrono actuel.** `cout.py` 868f8cb appelle une seconde
fois `porteurs_point` dans le temps de règle, uniquement pour compter
le squelette après expansion. Ni ce compteur ni son chrono ne mesurent
une implémentation comprimée. L'échantillon de points est explicite ;
ne pas publier ce temps comme calcul sur toute la trame. Une source
LIVE modifiée après lancement ne fournit pas le pin de ce lancement.

### Un rescan ER peut devenir deux comptages de préfixe

Dans `er_nouveaute.py` 25acbe, les helpers forment des ensembles de
points par scan de tout le nuage à chaque lien de vote. Pour un enfant
c et un ancêtre strict v, la persistance donne
{y:c_y(c)<d_c} ⊆ {y:c_y(v)≤b_v}. Leur égalité est donc exactement
l'égalité de leurs cardinalités. Les listes `_cs` contiennent chaque
point distinct une fois : utiliser `bisect_left(_cs[c],d_c)` à la mort
stricte et `bisect_right(_cs[v],b_v)` à la naissance fermée. Ne pas
compter les graines brutes dupliquées.

Préparer et cacher ces deux nombres par nœud rend ce critère O(1)
par lien de vote après O(Σ_v log(1+|inc(v)|)) requêtes de préparation,
sans remplacer un ensemble par un autre ni changer ER-n. La couverture
dense D et ses tris restent payés. Le critère discret de nouveauté ne
devient pas continu par cette optimisation.

Diagnostic RAM privé et ouvert, contre-rejoué normal/−O :100 sorties
exactes identiques,68 mutants de naissance stricte causalement refusés,
132 inclusions dont70 entre ancêtres non immédiats.976 visites de
points deviennent176 requêtes de préfixe dans ces profils abstraits.
Ce ne sont ni un reçu clos, ni des géométries FULL natives, ni un gain
LiDAR/G4. Dans le ER-n actuel, un parent FULL de vie positive hérite
du vote de son enfant s'il est dans la bande : ne pas attribuer les
70 contrôles multi-sauts à un chemin réellement observé du moteur.

**Retour immédiat sur la nouvelle conception ER§7.3.** Mémo privé
ff0156f, lu à03 h59 : les counts naissance/mort doivent compter TOUS
les points distincts, pas les ensembles plafonnés à mcs destinés à
l'admissibilité. Additionner les tailles des enfants serait faux si
un point couvre plusieurs branches. La proposition O(N+T) du mémo
n'est pas démontrée par notre remplacement de deux ensembles déjà
connus par leurs cardinalités.

Une construction exacte évitant D est disponible sous graines couvrantes
complètes, atterrissages normalisés et activations pendant la vraie vie
du propriétaire. Parcourir FULL en profondeur : écrire les graines
directes activées à la naissance AVANT les enfants, puis toutes les
graines directes activées plus tard APRÈS les enfants. Le sous-arbre
complet est un intervalle de labels donnant l'amas juste avant mort ;
son préfixe qui s'arrête après les enfants donne l'amas à naissance.
Tous les événements descendants précèdent cette naissance, par les
vies strictes et la persistance. Les doublons de label restent présents
dans le flux, mais sont dédupliqués dans chaque requête.

Pour les deux intervalles par nœud, balayer les labels et ne garder
que leur dernière occurrence dans un arbre de sommes Fenwick : une
requête [l,r] au curseur r donne exactement le nombre de labels
distincts. Coût O((N+T) log(1+T)), mémoire O(N+T), sans développer les
ancêtres. C'est une proposition constructive à porter, pas une campagne
native ; elle ne prouve pas le O(N+T) revendiqué. Les deux meilleurs
alpha pour l'échelle doivent aussi porter deux IDs DISTINCTS : un même
point dupliqué dans deux branches n'est pas deux voisins d'accueil.

### Comptages et échelles sans fermeture dense

**Le port peut être plus simple que le balayage Fenwick.** Préconditions :
FULL atomique à vies positives, couverture persistante COMPLÈTE, graines
normalisées sur un propriétaire vivant dans [naissance,mort). L'égalité
de mort appartient au parent. Plateaux et graines tronquées ne satisfont
pas ce contrat. Catalogue, fabrication des graines et normalisation
restent payés : remonter les parents par graine peut encore coûter Θ(TN).

Dédupliquer (point,nœud) en gardant l'activation MINIMALE, puis trier
les nœuds de chaque point par tin. Retirer v si le nœud immédiatement
suivant du même point est un descendant strict. Ce seul suivant suffit :
les descendants occupent un intervalle Euler. S'il est aussi retiré,
un descendant conservé subsiste et couvre v dès sa naissance, avant
toute activation propre supprimée. Toutes les premières couvertures
et alpha sont conservées. Les entrées retenues forment une antichaîne
par point et sont exactement les entrées ER minimales, sans quota.

Noter N les nœuds, T0 les graines normalisées AVANT élagage, T les
graines RETENUES, e_v leurs activations propres
à naissance, l_v leurs activations propres strictement plus tardives.
Pour chaque point, prendre les paires consécutives de ses entrées par
tin et ajouter une correction −1 à leur LCA. Celui-ci est strictement
au-dessus des deux entrées : aucune correction ne touche l_v. Avec q_v
le nombre de corrections reçues, poser w_v=e_v+l_v−q_v et P la somme
de préfixes des w dans l'ordre Euler :

```text
F_v = P[tout(v)] − P[tin(v)]   # tout EXCLUSIF, avant mort
B_v = F_v − l_v              # naissance FERMÉE
```

Un point possédant k>0 entrées dans ce sous-arbre y possède exactement
k−1 corrections de paires consécutives, donc contribue1. Les paires
traversant sa frontière ont leur LCA dehors. F_v compte exactement
les points distincts. Les l_v IDs sont distincts et absents de l'amas
à naissance ; sinon leur entrée propre aurait été supprimée. B_v
est donc exact aussi. w et P peuvent être négatifs : sommes SIGNÉES
exactes, sans clamp ni plafonnement à mcs. Leur amplitude est bornée
par T ; choisir le type et les contrôles d'overflow en conséquence.
Le tout du développeur est INCLUSIF : convertir avant de porter la formule.

Le nouveau cout_flux.py calcule déjà F par entrées/LCA puis postordre ;
B=F−l prolonge son approche, sans nouveau modèle. Le préfixe Euler évite
le postordre de profondeur N pour ces cardinalités. Avec LCA par sauts
binaires : préparation O(N log(1+N)), requêtes O(T log(1+N)), mémoire
O(N log(1+N)+T+n) après élagage. Déduplication et tri initial coûtent
O(T0 log(1+T0)) et leur stockage d'entrée T0 reste payé ; corrections,
scan et lectures coûtent O(N+T+n). Une LCA O(1) à préparation linéaire n'est
PAS implémentée ici. Ni O(N+T) pour l'ensemble ni borne sur T acquis.

**Admissibilité.** Trier les seules activations propres tardives :
si B_v≥mcs, a(v)=b_v ; sinon prendre la (mcs−B_v)-ième activation,
ou None si la liste est trop courte. Ce n'est PAS la mcs-ième graine
brute. Après préparation, tous les a(v) coûtent O(N) par mcs ;
tri et comparaisons exactes restent payés.

**Échelle et frontière.** Préparer deux meilleurs alpha, avec deux IDs
DISTINCTS, à naissance et par préfixe tardif. Pour une entrée x au
niveau c, inclure TOUTES les activations≤c, puis exclure x du top2.
Le cardinal co-couvert vaut B_v+taille_préfixe−1. Le cas vide reste
un refus. Un point présent dans deux branches n'est pas deux voisins.

Pour éviter un postordre top2 de profondeur N, le merge des meilleurs
IDs distincts est associatif, commutatif et idempotent sous l'ordre
alpha décroissant/ID. Sur le flux « propres à naissance, enfants,
propres tardives », DÉJÀ construit, un arbre de segments O(T) fournit
les top2 des deux intervalles par nœud en O(N log(1+T)) travail et
O(log(1+T)) profondeur parallèle. Les préfixes tardifs utilisent
le même merge par scan. Fabrication du flux, réduction alpha, tris
et comparaisons exactes restent payés. Proposition de port seulement ;
les top2 ne remplacent pas le comptage distinct B/F.

**Comptages généraux désormais clos, antichaîne encore ouverte.** Le
[paquet statique](../../receipts/audit_continu_20260929/distinct_lca_counts_20261001/README.txt)
conserve20 profils abstraits,180 cardinalités, vraie AST ER99e8,
oracles par ensembles/Fenwick/arbres augmentés et proposition LCA.
Capture unique normal/−O, lecteurs statiques et copie déplacée recoupés ;
les replays reproduisent le stdout archivé octet pour octet. Les cinq
contrôles RAM exposent4/12/10/17/5 cas erronés ; ce ne sont pas des
mutants compilés. Le LCA général AVANT élagage conserve27 corrections
naissance et4 tardives : les mettre toutes à naissance serait faux.
Manifeste source0cf4a63865b5139db8fc82e973a4481612fcd7c454ff7c1a35308217f40c92a5,
capture5bfab7d05b8cd7bc1b482a318fc350af958a35e2a531bfec901bbee9bd505e46.
La gestion des signaux n'est pas exercée par cette capture sans signal.
Pas de port natif, géométrie FULL réalisée ou chrono LiDAR/G4.

La nouvelle sonde d'antichaîne,
entièrement relue et rejouée par l'auditeur, donne180 comptes,
630 admissibilités mcs1..7,126 sigma et9 refus de sigma vide.
16 erreurs du mauvais offset et9 du top2 non distinct sont exposées.
Deux contrôles causaux ciblent « première graine plutôt que minimum »
et « seul enfant immédiat plutôt que tout descendant » : ce dernier
crée artificiellement un septième point déjà couvert.
Manifeste privé007680d4bff67a9b17bb604829949ac13150de4f334d99a2e74c7364211fb69a ;
sonde9db04b48ea39bc3e34cb452fc24e5aaff74bdfedff1d85196cd3cac3f89cdbc8.
Contrôles RAM OUVERTS n6, sans reçu clos, géométrie FULL, port natif,
gain LiDAR, preuve sous-quadratique globale ou chrono G4.

**Nouveaux flux synthétiques, pas la règle ER.** Les quatre sorties
8k/16k/32k K5 et8k K10 sont désormais terminales, code0, concordantes
avec leurs JSON. D K5=131238015/572637719/2182764305, soit×4,36/×3,81 ;
entrées moyennes≈97/104/108. Cela motive la représentation implicite
sur ce régime de huit gaussiennes spherical medium u18, sans bruit :
ni LiDAR ni un calcul de ER. La « bande » du script utilise alpha²,
sans naturalité ni mcs, pas l'ancre A de ER ; elle n'est ni le compte
exact de ses votes ni une borne supérieure générale. Le pin de
lancement des sources/binaires n'est pas clos ; la comparaison n300
ne juge ni sigma ni admissibilité. Ne pas transférer ces chronos au port.


### Le seuil des rivales admet une expression sans porte dure

Pour la préhistoire c_x(v)<A(v), poser L=λAx>0, b=b(P) et h=Ah(i)
pour une branche rivale i d'un ancêtre P. Si h<b, P est admissible
dès b, donc A(v)≤b. Si h≥b, aucune composante couvrante sous i ne
devient admissible avant sa mort : toutes héritent de A(P), d'où
h=A(P)≥A(v). Par conséquent le complément du poids actuel vaut

`1−omega(v) = max_(P,i) clip(min(A(v)+L−b(P), A(v)−Ah(i))/L, 0, 1)`.

Le maximum vide vaut0. Une branche h≥b contribue déjà0 : la condition
h<b peut disparaître de cette expression, sans changer le poids.
Minimiser Ah dans un groupe de rivales commute avec maximiser cette
perte. Pour A(v)≤c_x(v), le code impose omega=1 ; garder ce cas, dont
la préhistoire a une longueur nulle.

Relecture algébrique et vraie AST c4ab `admissibilite/poids_point` :
3171 gardes sur120 évaluations de profils monotones à vies strictes,
rejouées normal/−O par l'auditeur, mêmes sorties et source inchangée.
Exemple h→b avec L1/2 : omega63/64 à h=511/128,b=A4, puis omega1
à h=b4 et à h=513/128,A=513/128. Profils abstraits seulement ; aucune
réalisation géométrique, archive close ou preuve globale de continuité.
Cette expression aide la preuve de transport ; elle ne prouve pas à
elle seule la stabilité lorsque la topologie FULL change.

### Les fusions tardives ont une borne géométrique

Lemme pour FULL exact, couverture fermée, sites distincts,
2≤K≤n, n≥mcs. Noter α=√Ax et E2=Ahat+ηAx. Choisir une K-partie
témoin initiale F0, dont la MEB B0 de rayon α contient x. Toute
lignée massive possède un témoin F dont la MEB B de rayon√c
contient aussi x, avec c<E2.

Les centres de B0 et B sont distants d'au plusα+√c. Une boule
contenant les deux a rayon au plusα+√c. Dans cette boule, toutes
les K-parties de F0∪F sont connectées par échanges élémentaires,
dont les cofaces à K+1 sites ont MEB dans la même boule. Par la
représentation Γ/FULL exacte, toutes les lignées massives sont
donc réunies avant le rayonα+√E2, égalités et coquilles incluses.
La famille K2 {−2,0,2R} atteint cette borne α+R.

Par ailleurs date_finale≥√Ahat : le propriétaire O couvre x avant
T_half, donc Ahat≤max(T_half,A(O)), et la date/plancher dominent
les racines de ces deux termes. Puisque Ahat≥Ax,
√E2−√Ahat≤(√(1+η)−1)α. Sous κ≥√(1+η), le terme de première
unanimité est donc≤√Ahat. Plus généralement, un terme tardif de
marge μ est≤√Ahat+κα(1−μ). Si seule une masse finale ε retarde
une fusion déjà unanime pour le reste, son excès de date est
au plus2καε/W≤2κε/(ηα), car W≥ηAx.

Ce lemme contrôle un mécanisme de masses vanissantes ; il ne
prouve pas le transport des poids sous tous les changements de
topologie, ni la conjecture globale de continuité. Contrelecture
théorique et diagnostics RAM seulement :295 contrôles normal et−O
dans la sous-tâche, dix profils abstraits et douze petites géométries
Γ1D K2..5/n≤9 ; aucune archive close ou qualification native
nouvelle n'est attribuée à ces appels. Pins source principe c4ab,
échelle4cb629 inchangés.

### Ce qui reste un choix de modèle

La nouvelle question ER « chaînes » porte sur une adhésion dès la
formation d'un amas inchangé ou après majorité de bande. Compresser
exactement conserve la durée et l'attente existantes ; transformer
cette attente en adhésion immédiate, comme ER-n, change la règle.
Attention au nom Q2bis : ER demande l'existence et l'affectation du
cluster {x,b1,b2}, tandis que principe libre demande sa date d'adhésion.
Une réponse sur la date ne choisit pas à elle seule le veto d'échelle.
Le λ9/8 d'ER est un rapport de rayons : le test en niveaux carrés est
c_x(u)/max_y alpha(y)²≤81/64, pas≤9/8. Notre preuve CR noyau réfute
le masque actuel, pas toute règle possible respectant le cœur.
De même, pour une seule lignée MMt de début A, W=ηA et
T_half=A+ηA/2 : sa date est au moins√A·√(1+η/2), pas√A.

Le seuil d'aberrance, cluster ou bruit, les retards Q1/Q2/Q4 et
le respect CR_noyau sont donc à décider explicitement, pas à
déduire d'une optimisation ou à régler sur les seules cellules
connues. Je recommande de porter d'abord la compression exacte
du profil complet MMtA comme contrôle ; le masque CR_noyau est
réfuté comme règle robuste par la preuve close ci-dessous.

**Nouvelle question tétraèdres, §9.6 du mémo privé.** Source du mémo
42372651bc5ea86efa167ef3bc1d2ef6aa74cecae04243a529e79e685ae991a3,
relue à05 h08. Si FULL couvre cinq points avant les réunions des deux
tétraèdres, cette lignée devient admissible selon le critère de couverture ;
cela n'oblige pas une projection statistique à lui attribuer immédiatement
un cluster de cinq points. Dans la fiche, la couverture commence à16495,48
mais MMtA n'attribue T3a qu'à16876,14 : ne pas confondre ces deux dates.
« Cluster » ou « bruit jusqu'à fusion » reste une préférence adressée
à l'utilisateur, pas une conséquence à présumer de min_cluster_size.
Pour la sortie laminaire finale, publier aussi la taille réellement
attribuée, distincte de la taille couverte. Le contre-jugement des
statuts « forcés » ci-dessous ne décide pas cette préférence ; aucun
registre formel ni moteur n'est modifié.

Enfin PL-01/R est une identité du modèle pour mcs≤K, mais pas
celle des DEUX implémentations pour toutκ. L'ancien `mmt.py`
omet l'unanimité continue : sur deux sites0/2, K=mcs2, η1,
κ1/16, il donne√(3/2), contre√2−1/16 dans `_date` c4ab.
Le contre-contrôle AST/QS35 gardes normal/−O reste applicable :
κ3 égalise les dates. Restreindre l'identité des codes à
κ≥√(1+η) ou ajouter le terme omis. À la borne, écrire≤α,
pas<α. Pour une rivale Ah→b, écrire omega→1 après clipping,
pas « déjà inactive » avant le seuil.

### Statuts forcés et choix statistique

Le catalogue définit « forcée » par « imposée par FULL et la condensation
seules ». Pour mcs≥2, sous couverture ancrée NP, laminarité et condensation Π1,
max_amas<mcs impose effectivement l'absence de cluster. En revanche
un amas vivant C de taille≥mcs, contenant un point NON libre, permet
un contre-témoin : singletons avant une date éligibleτ ; bloc C àτ,
singletons ailleurs, puis C persiste le long des ancêtres de son
propriétaire. La couverture de C persiste, les partitions sont emboîtées,
et C survit au seuil mcs. La cible sans cluster est donc non universelle.
Ce témoin ne prouve ni robustesse/Can/TI/ANC pour un modèle global,
ni que MMtA doive choisir C, ni que la cible statistique de bruit soit fausse.

Le point non libre est indispensable : le vrai juge condense autorise
un cluster entièrement libre même si blocks=[] ; son cœur contraint est
vide. Un simple maximum de taille ne réfute pas une cible qui libère
tous les points de ce maximum. Pour le cas général, rechercher tout
amas admissible rencontrant un point contraint, pas seulement un argmax.

**Recoupe réalisée sans rejouer les scènes.** Jointure indépendante
des543 fenêtres des reçus existants avec le catalogue :378 maxima sous
mcs_min ; les165 autres témoins contiennent TOUS un point non libre,
et leurs165 dates sont dans la fenêtre rationnelle. Rapports normal/−O
égaux hors flag optimize, pins avant/après inchangés :
normal16658a51834b10ba08d89466c48831a12a2c98ae7b4377e1ee3d5506bed97ed1,
optimisé38e2a17251dcd880481ed61f489687e2d69386358224358dad8158ab7b7054a1 ;
cataloguea68b54ead709998e9331498648f7263624e92aa75e39e38d96e4da3acc63028f.
C'est une recoupe de métadonnées et de la portée logique des résultats,
pas une nouvelle qualification de543 FULL géométriques. Leur mcs_min
minimal vaut3 ; la réserve mcs1 ne change pas ce lot. À mcs1, Π1 garde
les singletons comme clusters, y compris avant ancrage : ne pas y
transférer le lemme d'absence ni le contre-témoin à singletons « bruit ».

**Petit contrôle causal du juge réel.** pont_court, K3/mcs5 :
C={A,B,C,D,M}, seul M libre. L'entrée éligible est au niveau
11988006664667/7329334 ; la borne droite vaut35940047982003/21972010.
Leur différence4003329999001333/40260049985335 est strictement positive.
La vraie AST du juge condense0ce69e8 donne : singletons conformes,
bloc C refusé, même bloc accepté si tous les points deviennent libres.
Contrôles RAM normal/−O concordants, sans Scene ni export natif nouveau.
Cela contre-juge bien l'absence imposée dans cette fenêtre.

**Question tétraèdres reformulée.** La première fenêtre où l'amas de
cinq points devient admissible libère TOUS les points ; le juge y
autorise déjà un cluster. C'est l'étroite fenêtre entre les deux
fusions qui exige T2/T3 en bruit. La fiche ecarts archive l'entrée
de T3a et la couverture du propriétaire sur TOUTE sa vie ; elle ne
donne pas la première date à laquelle cinq points sont réellement
attribués au même bloc. Retirer « cluster dès naissance » de cette
inférence ; publier cette date exacte et les dates/propriétaires
des quatre points T2 avant de demander une préférence à l'utilisateur.
Conserver les calculs, corriger « fausseté de la cible » en
« absence non imposée par FULL+condensation ».

## Le masque CR noyau introduit un saut géométrique

1er octobre 2026, 02 h 25 UTC. La variante `mmta_cr(...,variante='noyau')`
du source c4ab présente une discontinuité réelle, même aux défauts
κ3, η1/2, λ1/2. Ce n'est pas une erreur de calcul du seuil de cœur ni
un défaut du balayage : leurs contrôles passent. Ne pas transférer à
ce nouveau masque les propriétés de la MMtA au profil complet.

**Témoin exact K2/mcs3.** Dans un plan de3D, prendre x=(0,0), a=(10,0),
z=(211/10−ε,0), b±=(−11,±36/5). La branche locale {x,a,z} couvre trois
points dès le rayon211/20−ε/2. Les composantes gauche et droite se
réunissent au rayon fixe111/10. Le troisième point du noyau local,
z, entre seulement au rayon111/10−ε : juste avant cette réunion si
ε>0, exactement à celle-ci si ε=0.

Le nœud Cc choisi à cette date passe ainsi de la branche locale au
parent. La restriction des porteurs à ceux comparables à Cc retire
toute la préhistoire rivale pour ε>0 et la réintroduit au plateau,
avec masse non nulle. L'ancre Ax reste25 dans les deux cas : la changer
ne réparerait donc pas ce témoin.

Pour0<ε≤1/128, la règle donne hauteur(x,a)=211/20−ε/2. À ε=0 elle
donne111/10, soit un saut limite11/20. La date de x au plateau vaut
exactement15588789463113/1458264845330, environ10,68996 ; son
propriétaire est la rivale avant la réunion. Le saut concerne bien
une hauteur de hiérarchie, pas seulement des IDs internes.

La [preuve close](../../receipts/audit_continu_20260929/mmta_cr_core_comparability_20261001/README.txt)
a été relue et reproduite par l'auditeur avec la vraie AST c4ab,
Γ2 exhaustive/Fraction, noyau de points réellement entrés recensé
indépendamment, balayages rapide/lent et contrôles de cœur et
d'admissibilité :961 gardes, dix cas rationnels et trois paires de
jumeaux de grille. Lecteurs normal/−O, copie déplacée et replays
indépendants normal/−O passent ; les replays reproduisent les captures
octet pour octet. Faux SHA refusé avant parsing. Manifeste externe
`8a0eec667320e6af977d0c7f7a2497e64f6c7f8b54951653adde032348bf0bdf`.
MMtA sans CR garde hauteur(x,a)=111/10 sur cette même famille ; CR
« amas » sert de second contrôle, pas de remplacement robuste recommandé.
Aucun moteur natif, EOM, K5/LiDAR ou GPU n'est jugé par ces appels.

Conséquence lisible : à la coupe fixe54/5, tous les points sont entrés.
Au plateau, le bloc de trois points est{x,b+,b−}, contre{x,a,z} pour
ε=1/8192. Une condensation par simple taille mcs3 conserve donc deux
groupes différents, pas deux IDs d'un même groupe. Ce diagnostic a été
recalculé séparément sur les hauteurs exactes archivées ; il ne teste
pas l'extracteur EOM de production.

**Le phénomène existe aussi sur grille1mm.** Multiplier par10M et
translater par(110M,72M). Comparer z=(321M−1,72M) à(321M,72M), les
quatre autres points restant x=(110M,72M), a=(210M,72M),
b+=(0,144M), b−=(0,0). Un déplacement d'une seule unité change la
hauteur de(11M+1)/2. Les jumeaux M1,13,816 ont été exécutés exactement :
sauts6,72 et8977/2 ; leurs coordonnées sont dans u18. Ce n'est pas une
asymptotique de continuité dans un domaine à bits fixés ni un test LiDAR.

**Consigne de port.** Ne pas porter ce masque rétroactif comme solution
robuste. Garder le profil complet comme contrôle et concevoir la
contrainte de cœur sur la lignée et les dates de conflit, ou une entrée
continue des histoires rivales. Retirer seulement le masque ne
garantit pas CR : dans ce témoin, la MMtA complète peut choisir la
gauche avant que le noyau droite admissible ne doive réclamer x.
Date et propriétaire doivent donc être traités ensemble. Ni une
impossibilité générale de CR, ni un correctif global déjà prouvé ne
découlent de ce contre-exemple. Conserver les témoins pour juger la
future règle, sans ajuster les attentes à sa sortie.

## Dent native du filtre et suivi des qualifications

### Tête nouvelle et dent sur l'entrée tardive

Observation actualisée à05 h26 dans la copie privée R2, HEAD36b8e9b
encore modifié. La porte FINALE75/75 est maintenant terminale code0,
1150s mur ; journal1181296b7fc9166c0f78154da054b944c4d3bd22776e9bde1a0d81c86fe81b02.
Trois lots ASan/TSan/Clang sont terminaux24/24 chacun, code0,
zéro rapport et zéro avertissement ; résumé au SHA
ad9d15c92980a3c544a1c64d57a8f1a4139cf795976bf24617eb2de427be635c.
Ce sont des portes CPU locales, pas des chronos de produit sous faible
charge ni une exécution GPU. Le groupe de reçus tête n'était pas clos
à la recoupe ; les163 fichiers du groupe oracles antérieur ne le qualifient pas.

La relecture de head.cpp confirme les préparations CSR des points,
la masse de sous-arbre exacte u64, les sous-arbres retirés disjoints
et les passes de sélection/labels sans remontée par point. Sur le
dendrogramme fourni, travail O(N+n+L), L niveaux, sans borne de croissance
du nombre N de nœuds livrés. Les passes séquentielles gardent une
profondeur N sur une chaîne ; ce n'est pas encore un port massif GPU.
La condensation emploie la masse TOTALE du sous-arbre ; les points directs
sortent à leur niveau d'entrée propre. Le contrat de condensation par masse finale
n'est pas celui de l'admissibilité instantanée des couvertures ER/MMtA.
L'EOM binary64 du domaine annoncé ne devient pas une preuve de
comparaison exacte des stabilités réelles pour toute entrée géométrique.

**Portée du juge par définition.** head_gate reçoit cl.tree, y compris
ses stabilités déjà calculées. Il contre-juge sélection et labels SUR ce
condensat, pas toute sa fabrication. Les autres fixtures et comparaisons
sklearn couvrent des cas de condensation ; ne pas annoncer un juge
indépendant exhaustif de la tête à partir de cet appel seul.

**Dent indépendante exécutée.** Dans une copie privée gelée, API native :
deux feuilles de deux points, réunies au niveau4 ; un cinquième point
direct au parent entre au niveau9. mcs2, z1, EOM, allow_single.
Oracle analytique : stabilité racine7/3, lambda directe1/3, racine
sélectionnée, labels[0,0,0,0,−1]. Le témoin compilé GNU donne exactement
ces labels et les valeurs flottantes au niveau de tolérance annoncé.
Le mutant privé ne change qu'une ligne de condense, en remplaçant
le rang PROPRE du point par celui du nœud : compile0, run1, cinq
échecs sémantiques, stabilité5/2, lambda1/2 et labels[0,0,0,0,0].
Pas de crash ni défaut de harnais ; la sélection du même nœud ne suffit
pas à masquer l'erreur sur le bruit.

Source head371d1444f35d27217999e2fe64931fb37b22d9d7aed51f7042e2bec58e206193,
probe932ae7dfc6e4c32c8a11db732c2aadac3f3abd2463ee7c989c571785b3e0ce7a.
Binaires témoin99f7fa4daa69653c9a2aad08b91ce8e49fa58aec42012529b6c982235bebdad3,
mutantc21a15ef646eb306fed2ac09676f4e8c3d4c09ed94ff0cddcfa27b1515c65b25.
Sources/compilateur stables avant/après. Le premier diagnostic reste OPEN.
Une NOUVELLE [capture native close](../../receipts/audit_continu_20260929/head_direct_exit_20261001/README.txt)
épinglée à ces sources a ensuite été enregistrée après revue : huit étapes,
deux compilations et deux runs0/1, premier échec absent. Manifeste externe
f7876e1b6c192fc9e8306b76f963b527b39d17635059fa2afcf60e70cc0cdd9a.
Lecteurs statiques normal/−O et copie déplacée passent ; faux SHA refusé.
Recompilation indépendante de l'auditeur dans un runtime neuf : mêmes
deux SHA binaires et observations. Cinq refus causaux du vrai lecteur
en RAM, normal/−O : labels, lambda, run manquant, signal et source altérée.
L'oracle rationnel tolère16·2^-52·max(1,|r|) pour1/3 et7/3 ; autres valeurs
dyadiques et labels exacts. Pas de réalisation FULL géométrique ni de
qualification générale des autres branches, des interruptions ou gain G4.
La première invocation des contrôles RAM échouait par quoting/SyntaxError
avant le lecteur ; conservée, pas comptée comme refus causal du paquet.

**Mutants tête : conserver les deux essais.** Le premier lot code3
rapporte48 diagnostics, un signal, cinq équivalents, un survivant,
un équivalent tué et quatre défauts de harnais. Le signal reste séparé.
La passe corrective, relue directement, code0, traite SEPT cas :
cinq rejets diagnostics code1, XA3ep équivalent et RO3 LIMITE explicite.
XA3ep garde maintenant la consommation des chiffres d'exposant :
l'ancien motif ne mutait pas seulement la grammaire annoncée.
Les huit juges du témoin après sont verts et les binaires reconstruits
identiques à ceux d'avant. Journal de passe2cc33b9ef4f8b0049aaf7fc9f853cbc53fe64634badc725b1d2a722cb545d3218 ;
collecteur08e6115d52d14ee6ae778d38ed3b168733180ecdd0ddd97d3b1fe92e18f71184.
Cette passe ne réétiquette ni les quatre harnais historiques ni RO3
en mutant causalement tué : aucun appel n'atteint son invariant violé.

**Garde de schéma du différentiel tête.** Le vrai catalogue/verdict AST
a608953e8e247df50f7d6274a85a84e87c348033c92c2e1829e320e3885d13fc
classe IDENTIQUES trois compteurs tous absents, car get() produit null.
Un témoin présent passe et2/3/2 est bien DIFFERENTS. Recoupe RAM normal/−O,
commandes/dumps simulés : ne pas attribuer une sortie invalide au moteur.
Les contrôles réels de dumps non vides et SHA restent utiles. Exiger
présence et types de balls/levels/by_q_p avant de comparer leurs valeurs.

### ER : résultats concordants mais code0 n'est pas une porte

Recoupe des données réelles ER de base :43 lignes utilisateur uniques et64 lignes
catalogue uniques, chaque compte calculé apparié au reçu, zéro divergence.
SHA des JSON a8b4ba2c54fc9375d2e36841964cdb30dba8fc8c0b06d9fe661f3a92193907eb
etb2eee1fc863b016b8a564bdb78833562fe1cf4d0c3bfbc3ec25dcb190c2b52d1.
Les nombres de cellules sont8611 et50818. Ce sont des observations
concordantes, pas une nouvelle capture autonome avec pins de lancement.

La vraie AST main du collecteur a néanmoins rendu code0 dans QUATRE
contrôles RAM : code fixture inexistant, nmax0, reçu manquant, compte
reçu volontairement divergent. Les deux premiers exécutent zéro juge ;
les deux derniers affichent la différence sans la refuser. Replays
normal/−O identiques, source stable9e985e10a1e21331c9e0c3238633ccb769c27bb0d00b99e2f17e201b9df1feb5.
Ce test du collecteur emploie UN juge simulé : aucun défaut de géométrie
ER ni faux résultat des43/64 lignes n'est déduit de ces contrôles.

Conseil : garder le mode diagnostic, mais ajouter un mode de qualification
distinct qui exige un inventaire non vide annoncé, l'appariement complet
et les mêmes comptes, sous les mêmes cibles/paramètres. Toute absence
ou divergence doit refuser. Le nouveau q4strict n'est pas encodé dans
params de cette sortie ; enregistrer aussi cette politique, argv et
les sources de lancement. Une recherche de paramètres reste exploratoire
et ne choisit pas les réponses statistiques de l'utilisateur.

**Mise à jour du collecteur8fcb82e.** MODE/theta/q4strict sont maintenant
consommés mais absents de params ; l'empreinte ne les lie pas non plus.
Pour --mode, le premier champ du reçu différent de ER est pris, pas la clé
du mode demandé. Contrôle de la vraie AST main normal/−O, juge simulé1/1 :
le seul réordre des clés change la comparaison0/7 en1/1 ; mode inconnu,
theta ou strict modifié laissent params et empreinte identiques.
Source8fcb82efb3c69c68f0a8210cec6e57751bd8870f57cfa93f547378b7177020ae,
stable avant/après. Premier rapport RAM refusé par sérialisation Fraction,
puis corrigé ; aucune erreur ER/native déduite de ce défaut de contrôle.
Choisir explicitement ERpoints/ERpref/etc., refuser un mode inconnu et
publier mode/theta/q4strict avec les paramètres du reçu comparé.

**Inversion réellement observée, pas une divergence géométrique.** Les
nouveaux reçus listes de masse_er/pref_er portent [jugements,passes],
copié tel quel face à [passes,jugements]. Jointure indépendante normal/−O :
u_points43 lignes/43 appariées/2 inversions ; cat_points15/5/5, dix non
appariées ; cat_pref64/64/26. TOUTES les33 divergences publiées sont cette
inversion. JSON respectifs38e2deec66d70d40f7af2759cf1491223238426876a9a888b63fbce869188126,
d208c78da993a948edcf3751d167086da9a43236d3a789752efcddaaf2551dc2,
a62ee0019f1dbafa5f5d7d018e68f774095dff719454dae765f2b9de76442e54.
Recoupe de métadonnées seulement, sans rejouer les scènes/FULL. Normaliser
les champs nommés réussites/total, puis vérifier l'inventaire apparié ;
ne pas prétendre que ces résultats sont faux ni que code0 les qualifie.

1er octobre 2026, actualisé à04 h35 UTC. Le
[contrôle causal du filtre réel](../../receipts/audit_continu_20260929/actual_orientation_filter_20261001/README.md)
est clos : témoin GNU, mutant GNU à borne trop faible et témoin UBSan,
192 lignes de primitive. Le témoin ne prend aucune décision fausse ;
le mutant en prend27, dont5 sous l'arrondi utilisé par la tour. Ce
sont des sorties numériques observées avec code0, pas des crashs.
Sources moteur complètes gelées, lecteur entier indépendant,
recompilation indépendante témoin/mutant reproduisant les stdout,
lecteurs normal/−O et déplacement recoupés. Quatre corruptions en RAM
sont refusées : fausse décision du témoin, ligne absente, mutant sans
dent et signe exact incorrect. Manifeste externe
`6d63a879a2307a0738710c17690a4a947e3f5a6157710951e0a942e77d8d85d1`.
La géométrie a un centre sur une arête : certification fermée seulement,
pas supports q4 stricts à émettre. Les autres modes d'arrondi sont
diagnostics de primitive ; l'appelant les désactive. Aucun gain FULL/G4
ou contrat100ms ne découle de ce paquet.

Le mutant B21 `N_nearest_w_smallest_release` a maintenant un rejet
causal FX-KNN :500 échecs, code1, sans signal ni délai ; journal au SHA
`7d83056cd996ef1d383bb6467959a0776e1ed064b6b1025496c2d8c5fc568561`.
Ce nouveau résultat ne réétiquette pas l'ancien signal sanitizer.
HEAD privé `aa1a9d174d97631a84dfcbf452ce219d607f7e52` ajoute l'inventaire
du juge et l'état compilé de l'ombre. La campagne de mutants emploie
encore le juge4ec : ne pas lui attribuer rétroactivement le juge aa1.

La restauration R2 des mutants CLI est corrigée dans le collecteur
`03aa1916a41802b49a52f60b0a6de3106300fcb2767efbb0bef481143f388cfe` :
reconstruction des deux exécutables et contrôle de leurs hashes avant
de continuer. Le contrôle du collecteur en RAM refuse un binaire resté
muté ; ce n'est pas une compilation du moteur. Un SIGTERM restaure le
source, mais ne parcourt pas cette reconstruction placée après finally.
Le journal relancé a écrasé l'ancien fichier LIVE ; son pin ci-dessous
décrit l'observation historique, pas la nouvelle sortie.

**Réparation R2 désormais terminale.** K7/K8 ont leurs propres
dents de budget : K7 refuse N=458375(code2, pas de dump), K8 admet
N−1=458374(code0, status ok). Source collecteur03aa et journal lus
directement ; le pin intermédiaire02 h46 `0d0f8e3b…` est historique.
Le journal terminal, code0, annonce8/8 mutants refusés et témoin
non muté vert, SHA
`e6606a79dbb89ed9cf970d74b5bda313fa72afa6844627f94186a5f627fd51b3`.
Ces rejets ne sont plus attribués au témoin de tour tronqué de K6.
La terminaison d'une campagne ne clôt pas tout l'agrégat R2.
Le lecteur de catalogue vide garde le pin7f983 à la relecture03 h20 ;
son défaut demeure à corriger.

La porte CTest B21 au HEAD aa1 est aussi terminale20/20, code0,
journal `cdd015f215871a7b13bb01e0520a565889c718c7af6cfa761c830b5db64658fe`.
C'est une porte CPU locale ; sa terminaison ne réétiquette pas la
campagne de mutants sur4ec, ni les anciens signaux en rejets causaux,
et ne qualifie aucune performance G4.

**Nouveaux terminaux recoupés, agrégats toujours distincts.** Le journal
CTest R2 au HEAD privé4b457 annonce maintenant71/71 tests, code0,
1643,92s mur. Sa lecture LIVE à03 h55 inclut la ligne de contrôle pycache ;
SHA `937004ba4dc7ea8d40910de1ce101da6a5a6ed58d90dc3a1a79fb53daca50aec`.
Le pin terminal f3bdb0c observé auparavant est historique après cet
ajout. L'identité B21 revue3 est terminale7/7, code0, SHA
`b151575313fd026440bd5911cd30751fc61c6d5050073d1d7db844aaa2b6eb65`.
Elle compare de vrais dumps non vides, dont deux moitiés LiDAR K6
avec fils1/3 et3/1 ; non-régression, pas oracle ni trame entière G4.
Le défaut du lecteur de catalogue vide reste distinct et ouvert :
71 CTests verts ne le réparent pas. Aucun agrégat final ni contrat
100ms n'est déduit de ces deux terminaux.

**R2 : groupe clos, pas l'intégration suivante.** HEAD privé36b8e9b
intègre les juges aux CLI strictes. L'auditeur a vérifié le groupe
receipts/raccord_r2_20260930/oracles :163 entrées uniques, hashes
concordants, inventaire exact, aucun symlink ; manifeste externe
bf1c4ae9ec0eb67ebb4a2bdc87e130994eba9753f9ce8e78fcb15905709ffd07.
Les71 CTests sont attribués à ce groupe. À04 h28, le worktree privé
porte déjà une nouvelle tête modifiée : ne pas lui transférer ces
verdicts. « G4 simulé » est un build CPU local, pas une session G4.
Le lecteur vide7f983, hors portes, reste distinct et inchangé.

**B21 aa1 : nouveaux terminaux.** Oracle exact :577 contrôles,
0 écart,16834 boules,62766 coupes,70 petits nuages extrêmes,
code0 ; journalab5252a7dbb187fe575263333bba8cec0f43048fab243d1eb78ab0396c0f3125.
Revue3 :57 mutants et UN témoin,49 rejets codes1/3, sept survivants,
un signal−11 ; aucune dent géométrique attribuée au signal.
Journalb199bfb8b67bdb891499f85130e28a7a4e2977c019279d0f21789779fcc63fba.
Le collecteurcdcea292 restaure les sources sans reconstruction finale
ni témoin terminal : le dernier binaire n'est donc pas attesté revenu
au témoin. Un SHA différent d'un AUTRE build ne prouve pas non plus
qu'il est resté muté.

**Réserves sur deux arguments B21.** T2 :1,8 million de faces sans
contre-exemple à2u ne prouvent pas une dent mathématiquement impossible.
Conserver la borne sûre, sans grand chantier pour la resserrer.
T4 : les fractions Level ne sont pas réduites et le contrôle tardif
vérifie les niveaux exacts, pas l'ordre S* entre deux niveaux égaux.
Il ne prouve donc pas, à lui seul, qu'un tri approché SANS réparation
ne peut jamais donner une sortie fausse. HEAD utilise bien le
comparateur exact puis S*, correct ; aucune géométrie causale
défaillante n'est produite ici. Réserve sur la preuve du mutant,
pas défaut démontré de HEAD. Aucun FULL/G4/100ms nouvellement acquis.


## Réponses prioritaires au développeur et alerte sur les mutants

1er octobre 2026, 02 h 00 UTC. Les questions adressées à l'auditeur sont
Q1/Q2/Q3 du [mémo contact et comptage](../REPONSE_CLAUDE_CONTACT_COMPTAGE_ET_JUGES_20260930.md#7-questions).
Les réponses sont reprises ici pour éviter de les chercher dans les suivis :

1. **Q1.** Les classes de bande contenant x peuvent être recherchées dans
   la boule complète B(x,2R), R=√(1+η′)α_x. Les seuls K voisins ne suffisent
   pas. Avec m sites dans cette boule, les supports minimaux ont au plus
   quatre points en3D, donc O(m⁴) classes au plus ; cela n'est pas une borne
   en K, ni une stratégie sous-quadratique. Les grandes occupations LiDAR
   [déjà recoupées](#localité-lidar-mesurée-et-choix-de-structures) rendent
   le comptage implicite et le partage des tâches prioritaires.
2. **Q2.** Pour conserver exactement la sémantique des K-parties, garder
   leurs identités fixes et leurs comptes, éventuellement groupés par MEB.
   Une requête de rayon adaptative fournit les sites nécessaires ; aucune
   équivalence à une liste fixe de K voisins n'est établie. MMt/MMtA, qui
   utilisent les incidences et les durées de couverture FULL, sont une
   alternative calculable à étudier, mais changent le modèle de masse.
3. **Q3.** Oui, des poids souples dépendant seulement du rayon relatif
   d'une K-partie fixe évitent le saut causé par la disparition d'une boule
   forte du catalogue. Ils ne prouvent pas seuls la stabilité de toute
   la projection : conserver univers fixe, rampe nulle au bord, échelle
   non filtrée positive et date à marge. La [réponse détaillée](#réponses-aux-trois-questions-sur-les-votes-de-bande)
   précise les hypothèses du théorème de transport.

Les nouveaux §10 de `juge_final/parametres/MEMO.md` et §8 de
`juge_final/echelle_relative/MEMO.md` demandent surtout des préférences à
l'utilisateur : cluster ou bruit, date de formation souhaitée, bascule
du pont, seuil d'aberrance. Elles ne deviennent pas des contraintes
mathématiques imposées par la thèse sans réponse explicite. Ne pas ajuster
les paramètres aux cellules encore indécises comme si elles étaient
une vérité terrain déjà validée.

**Alerte historique R2 à02 h00 : les anciens rejets K7/K8 ne sont pas causaux.** Le
collecteur privé `receipts/raccord_r2_20260930/oracles/outils/mutants_cli_juges.py`,
SHA256 `23e147244e15860fb75c0f001c68b5d4d65442a53487940d681666631a16b933`,
restaure K6 dans le source de tour sans reconstruire son exécutable
(lignes108–110). Pour K7/K8, qui modifient `generator.cpp`, il ne reconstruit
que le catalogue (ligne98), puis teste aussi la tour (lignes69–75).
Le journal observé à02 h00 contient pour K6, K7 et K8 le même rejet
`vertical_audit` : témoin de taille1 au lieu de2. Cette commande de tour
ne passe aucun budget ; elle ne peut juger la mutation du seuil de nœuds
K7. La campagne s'arrête aux premières portes fautives, avant ses tests
du budget. C'est un défaut de qualification, pas une nouvelle erreur
du moteur non muté.

Journal LIVE `/tmp/mhgp10-integ-r2/scratch/oracles/mutants_cli/sortie.txt`,
SHA256 observé `2bfc60a3c16841a2e910d0406e67611b9b26eb5b0b78f52eb309815bbd6c6cde` ;
ce pin borne l'observation, il ne clôt pas cette campagne.

**Correction proposée avant de compter ces deux mutants :** reconstruire
les deux exécutables après restauration et avant le mutant suivant,
vérifier le témoin non muté sur les portes pertinentes, puis juger K7/K8
seuls sur N/N−1 et exhaustion. Un témoin vert seulement à la fin ne
répare pas une attribution erronée au milieu du lot. Conserver les
anciens essais avec leur cause, sans réétiqueter ces deux rejets « tués ».
Source et journal ont été lus directement ; aucun moteur n'a été relancé
par cet audit.

**Autre garde à réparer dans le juge d'échelle :** `catalogue_stream`
du nouveau `invariants_echelle.py`, SHA256
`7f983c004a0ed6dcda160065d5e7c3434bfd3b34c69bd1dc498ae6b7bbd3c9f7`,
retourne succès pour un dump vide sans appeler son parseur ni son juge.
Deux sites distincts et K1 exigent pourtant la boule de leur paire.
Le chemin de contrôle a été recoupé en RAM par l'AST réel, normal et−O :
`error=None`, `balls=0`, zéro appel au parseur/juge. Les adaptateurs sont
simulés ; aucun binaire HGP ni écriture de dump. Diagnostic non clos,
pas un reçu de conformité moteur. Exiger la non-vacuité dans le domaine
où une sortie est attendue, puis contrôler l'inventaire pertinent ; ne
pas déduire la complétude du seul examen des lignes présentes. Le premier
dump réel8k/K5 contient555892 boules : ce constat de lecteur ne signifie
pas que cette capture native soit vide.

### Ce qui reste prouvé pour la nouvelle MMtA

`juge_final/principe_libre/principe.py` a changé pendant l'audit : SHA256
`c4ab293273d9b84d370852f0953986fac4be8c27a50e9c50459523a9087e58bb`.
La bande, le dénominateur des poids et le cône utilisent désormais Ax,
et non S=Ahat : E2=max((1+η)Ax,Ahat+ηAx), normalisation λAx,
cône κ√Ax. Défauts κ3, η1/2, λ1/2 ; une variante CR est ajoutée.
Les contrôles de03ff7 et la borne W≥ηS de cette version ne sont donc
pas des qualifications de c4ab.

**Conserver l'ancre non filtrée.** Ax est le premier niveau de couverture
de x ; Ahat le premier où une composante couvre x et mcs sites distincts.
Les deux rayons √Ax et √Ahat restent1-Lipschitz sous les hypothèses FULL
complète et déplacement apparié déjà précisées. Puisque Ahat≥Ax,
la nouvelle borne de bande se simplifie exactement en E2=Ahat+ηAx.
Sur sites distincts, 2≤K≤n, couvertures finies, n≥mcs, η>0 et masses non
négatives, prendre
la lignée admissible couvrant x dès Ahat. Ses intervalles de vie
partitionnent [Ahat,E2) avec pente1 ; ils donnent
**W≥E2−Ahat=ηAx>0**. Ne pas garder la borne ηAhat pour cette variante.
Ce raisonnement porte sur le profil complet, pas automatiquement sur
le profil restreint de CR ni sur un repli hors domaine.

Le rayon du bord de bande est la norme du vecteur
(√Ahat,√η·√Ax). Les deux coordonnées d'ancre sont1-Lipschitz ;
le bord vérifie donc |√E2_X−√E2_Y|≤√(1+η)ε. C'est une propriété
de la bande, pas encore de la date finale ni du propriétaire.

**Identité à ajouter au juge du port :** si mcs≤K, chaque composante
naît avec au moins K sites couverts, donc a(v)=A(v)=b(v)≤c_x(v).
Il n'y a aucune préhistoire : tous les omega valent1, Ahat=Ax et
E2=(1+η)Ax. MMtA complet se réduit exactement à MMt aux mêmes κ/η,
indépendamment de λ, sans plancher d'admissibilité supplémentaire.
Ce contrôle porte sur couverture FULL, pas sur le noyau de CR ni sur
la taille dure du cluster attribué.

**Simplification des rencontres utile pour le port.** Pour une rivale
admissible avant sa réunion b, de date Ah<b, son terme est exactement
`max(b,Ah+λAx)`. Prendre le minimum R de ces termes, puis
`omega=clip((R−A(v))/(λAx),0,1)`, ou1 sans rivale. Conserver cette
rampe, le max et λAx>0 ; la variante dure λ→0 n'en hérite pas.
À Ah→b, le candidat tend vers b+λAx. Si le parent est admissible à b,
l'héritage impose A(v)≤b du côté concurrent : sa contribution tend
vers omega1, donc son omission est continue après clipping. Pour
A(v)=b, elle peut rester strictement inférieure à1 jusqu'au seuil ;
seule l'inégalité A(v)<b assure sa saturation avant celui-ci. Cela ferme le
saut lié à ce seul seuil, sur une lignée et un niveau appariés.
Il reste à prouver le transport aux plateaux multiples, les autres
rencontres, la restriction CR et les propriétaires ; **ce n'est pas
encore un théorème global de robustesse**.

**Petit contrôle actuel, non clos :** la sonde LIVE
`/tmp/mmta-c4ab-four-fixtures.RGuOI6lg/probe.py`, SHA256
`357f3233ea749e80b0e611cfdb80dd2b85add2e587889f1da876407a371bbb19`,
a été relue puis exécutée séparément par l'auditeur, normal et−O.
Vraies fonctions AST de c4ab, dont le constructeur `Donnees`, Γ2
exhaustive par Fraction indépendante, pas de Scene native :14 cas,
58 points, 522 gardes et dix contrôles de √Ahat. Le balayage et sa
référence lente concordent ; W≥ηAx est vérifié sur ces seuls cas.
Au plateau des cinq sites : Ax9, Ahat65/4, E2=83/4, W19/4,
T_half=147/8 et réunion(x,a)=5. Aux quatre perturbations décroissantes
jugées, les hauteurs convergent vers5 ; le saut de l'ancien modèle
n'apparaît pas dans cette famille. Sources partagées et snapshot ont
le même pin c4ab avant/après. Les deux sorties sont identiques ; ceci
ne qualifie ni la continuité globale, ni CR, ni K5/LiDAR, ni le natif.

**Pour ER :** son seuil dur de naturalité est explicitement discontinu
dans son mémo (§8). Une bonne calibration ne répare pas ce saut ; une
rampe éventuelle doit aussi conserver une ancre continue, pas le minimum
des seuls votes de poids positif. Clarifier la question de précision :
la définition compare des niveaux carrés à λ² ; λ=9/8 est donc un rapport
de rayons, correspondant au rapport81/64 des niveaux, pas9/8.

## Relance du développeur et décisions pour le prochain port

1er octobre 2026, 01 h 07 UTC. Sur la relance de l'utilisateur, les
questions mathématiques [Q1/Q2/Q3](../REPONSE_CLAUDE_CONTACT_COMPTAGE_ET_JUGES_20260930.md#7-questions)
ont été retrouvées et leurs réponses vérifiées. La réponse courte suit
immédiatement cette section ; les preuves et contre-exemples restent
dans ce même document. Aucun moteur modifié, GCP non utilisé,
`public_status=not_claimed`.

**Suivi du raccord au 1er octobre, 01 h 28 UTC.** Le HEAD privé R2
reste `4b457bdbb94bcad95557e8e4d18fb558bb6277e3`. Sa nouvelle
`notes/oracles.md` est explicitement EN COURS ; ne pas lui attribuer une
clôture sur la base d'un journal encore non terminal. Les headers de
lecture/options et de sortie gardent leurs hashes précédents ; les quatre
exceptions mémoire signalées restent à corriger. La recommandation
`leaf=max(8,K+3)` reste valable pour la campagne normale.

Le [nouveau contrôle natif MP10](../../receipts/audit_continu_20260929/mp10_temp_registration_20261001/README.txt)
confirme causalement l'utilité du déplacement sans allocation lors de
l'enregistrement du temporaire. Quatre appels sur les headers complets,
puis recompilation indépendante : le témoin armé n'effectue aucune
allocation après création, zéro fuite ; le mutant copie/allocation
rend `memory_budget` mais laisse un FD et un temporaire après destruction.
Le harnais nettoie ensuite uniquement ces deux ressources observées.
Conserver cette protection. Le paquet ne fournit pas la sortie brute du
code3 historique de MP10 et ne lui attribue donc pas rétroactivement
cette cause. Manifeste externe
`e05cc4b601499efc34c336c749a8e11a845b124807fe632c598fb5611fc275a2` ;
lecteurs normal/−O et archive déplacée passent, faux SHA refusé.

Le second rapport adverse B21 `notes/verif-revue/RAPPORT.md`, SHA256
`037cb9874f56b8b3a6bbd3aaa93c21153a4b96952cc84cca131812e741512027`,
reste au HEAD `30c66d82eb4f6019684168896baa9b80c4067c14`. Sa distinction
entre sorties correctes observées et réserves S1–S4 est juste : garde
de taille de KNN, intégrité des modes aux sites d'usage, dent du filtre
d'orientation et commentaire de racine. Le bilan20/20 est composite :
19/20 avec un document absent de la copie, puis la seule porte documentaire
rejouée avec ce document. Ce n'est pas une campagne unique terminale20/20.
Les deux nouveaux lots totalisent37 mutants : 28 rejets code1, un
plancher code3, sept survivants dont un équivalent attendu, un signal
distinct. Aucun nouveau défaut de géométrie de HEAD n'en découle, ni
qualification finale du raccord R2. Ces relectures n'ont lancé ni moteur
FULL ni test G4.

**Ce que le développeur peut reprendre maintenant.** Une requête complète
de rayon B(x,2√(1+η′)α) retrouve les sites nécessaires aux votes MMg.
Une liste figée des K voisins ne le fait pas. Les classes de même boule
peuvent partager leur calcul combinatoire, avec les chemins simples et
le calcul transposé [déjà publiés](#compter-une-classe-sans-développer-ses-k-parties),
mais il faut encore générer les classes absentes et conserver leur
propriétaire aux coupes utiles. Ce calcul ne transforme pas une classe
en un vote et ne rend pas sa génération sous-quadratique.

Pour MMt, reprendre plutôt les incidences complètes de couverture et la
[réduction à une lignée médiane](#réduction-exacte-à-une-seule-lignée).
Ce modèle mesure une durée de couverture, pas le nombre de K-parties.
Garder les deux modèles séparés dans les comparaisons et dans les formats
de sortie. Les poids souples de rayon sur les K-parties fixes répondent
positivement à Q3 ; la date à marge, son domaine et les hypothèses de
stabilité restent nécessaires. Aucun théorème n'impose qu'un clustering
batte HDBSCAN sur toutes les données.

### Condensation et changement de propriétaire sont deux décisions

La nouvelle note privée du développeur
`build/v10-verrou-points/juge_final/cibles/CIBLES_REVISEES.md`, lue
le 1er octobre, porte le SHA256
`40db48dc6f72ae95c63a4df4de6c54fc1e54a17e50e096af815c3bc2b0242bc2`.
Elle distingue les cibles ancrées, dérivées et indécises et garde ses
questions Q1bis/Q1ter/Q2bis/Q3bis/Q4bis/Q-Π2 pour l'utilisateur.
Ces choix ne sont pas des questions mathématiques auxquelles l'auditeur
pourrait répondre à sa place. Les anciennes quatre questions du mémo
`revision_cible/QUESTIONS_UTILISATEUR.md` ne suffisent donc plus à décrire
l'état de cette révision.

La proposition du § 2.3 est juste **si les deux cibles sont retenues** :
AB|CD|EF à mcs=2 et ABC|DEF à mcs=3, à une même coupe, ne peuvent provenir
de la condensation d'une seule partition. Un bloc ABC conservé à mcs=3
est encore conservé à mcs=2 ; il ne devient pas AB en rendant C à CD.
Cela ne prouve ni que ces deux cibles sont imposées ni que toute projection
doit dépendre de mcs.
Le [témoin autonome clos](../../receipts/audit_continu_20260929/condensation_fixed_cut_203_20261001/README.txt)
énumère les 203 partitions des six points : une réalise chaque cible
séparément, aucune ne réalise les deux. Le lecteur recoupe l'inventaire
par insertion des points, indépendamment du générateur. Lectures et
rejeux séparés normal/−O passent ; faux manifeste refusé. La recoupe
indépendante JavaScript retrouve aussi les 203 partitions et zéro témoin
commun. Manifeste externe SHA256
`920d1beb550a783e3caf079d4540b601aa00ade75aee3e8ca6c6f0649131e092`.
Ce contrôle ne juge ni un arbre HGP ni un choix utilisateur.

La condensation seule retire les blocs trop petits ; elle ne transfère
pas leurs points. Le principe proposé Π2, qui prive une petite structure
du droit de réclamer un point, ajoute donc un choix de modèle. De même,
la taille de l'ensemble couvert par un nœud FULL n'est pas la taille de
son futur cluster de points : plusieurs nœuds peuvent couvrir le même
point. L'admissibilité par couverture peut servir de filtre, mais ne
certifie pas une masse dure d'au moins mcs après attribution. Celle-ci
reste à vérifier sur la partition finale, avec un point compté une fois.
Corriger aussi l'attribution dans `juge_final/principe_libre/principe.py` :
sa documentation appelle Π2 « principe de l'utilisateur », alors que
`CIBLES_REVISEES.md`, § 1.2 et § 7, le décrit comme une lecture à confirmer.
La seule contrainte de taille ne tranche pas ce choix.

**Tests conseillés pour les nouvelles variantes privées.** Garder séparés
projection, condensation et EOM. Éprouver les plateaux scindés par ±1,
les égalités exactes et la continuité aux seuils d'admissibilité. Pour le
filtre relatif ER décrit dans `juge_final/echelle_relative/er.py`, tester
aussi le franchissement de c/σ=Λ et les replis. Un cône de marge ne prouve
pas à lui seul la continuité si un filtre dur retire un vote de poids
non nul. Ce sont des obligations de preuve, pas une réfutation acquise
du prototype ER ; aucun de ces nouveaux moteurs de règle n'a été exécuté
dans cette relecture.

**MMt pondérée : un cas limite à tester avant de transférer la stabilité.**
Le noyau privé `juge_final/parametres/mmt_pond.py`, SHA256
`abbeac86b171e851a4e782a5d573496e2e4a9e118ceddf4c820db21a6c66b2af`,
prend l'ancre A parmi les seules couvertures de poids positif.
`pipeline.py::ponderer` supprime effectivement les poids nuls.
Considérer un arbre abstrait fixe : un enfant couvre x au niveau carré1
et meurt à3 ; la racine couvre x à3. Le poids de l'enfant vaut ε≥0,
celui de la racine1. Prendre η=2/3 et κ=4. Pour ε>0, A=1, E²=5/3,
W=2ε/3 et T½=4/3 : seule la vie de l'enfant est dans la bande. Pour
ε=0, A=3, E²=5, W=2 et T½=4 : seule celle de la racine l'est.
La date au cône vaut respectivement √(4/3) et2, car dans chaque cas
la fonction de θ est strictement décroissante sur ]1/2,1]. Ainsi des
poids continus ne suffisent pas à rendre ce noyau continu à ε=0.
Le saut vaut2−√(4/3), indépendamment de la petitesse de ε.

C'était une première déduction exacte sur des données couvrantes
abstraites, sans réalisation géométrique Π2c ni rejeu du module privé
dans cette étape. La réalisation ci-dessous ferme maintenant ce point
pour l'ancien modèle. Elle impose de vérifier les conditions sur l'ancre,
le dénominateur et les replis avant de transférer le théorème S_t.
Maintenir une ancre de couverture non filtrée est une piste, pas une
réparation complète : il faut aussi garantir ou traiter la masse nulle.

### Saut réalisé par l ancienne règle et propriété utile de MMtA

Le [nouveau témoin exact sur cinq sites](../../receipts/audit_continu_20260929/pi2c_positive_anchor_jump_20261001/README.md)
réalise ce mécanisme par la vraie Π2c, pas avec des poids libres.
K2, mcs3, η2/3, κ4 ; x=(0,0), a=(6,0), b=(0,8), z=(−1,8),
y=(6,8−ε), troisième coordonnée nulle. Pour0<ε≤1/8,
la branche xay devient admissible à t1=25−4ε+ε²/4 et rejoint une
rivale couvrant x à t2=16+(3−2ε/3+ε²/12)².
L'écart t2−t1=ε²(100−16ε+ε²)/144 est positif et tend vers0.
Le poids de xa vaut donc (t2−t1)/(t2−9)>0 ; il devient0 au plateauε=0.

L'ancre de l'ancien `mmt_pond.py` passe de9 à16. Sa date de x
passe de√12 à√(6145/288) et la hauteur de réunion de x avec a passe
de√12 à5. La marge κ4 n'élimine pas ce saut. Le tableau exhaustif
des dix cofaces exclut toute fusion plus précoce ; leurs MEB sont
recalculés par supports rationnels indépendants. Les vraies fonctions
`Admissibilite` et `mmt_point_pond` sont exécutées par AST depuis
les snapshots complets, sans moteur ni Scene native. Six configurations,
un contrôle mcs2 à poids1 et deux mutations causales ; lecteurs et
contre-rejeux normal/−O et paquet déplacé recoupés. SHA externe
`25ca9c2e98bbde3b8cc11e50a904c78c9c2230ca58734614752781dab6ca5e8c`.
R1 privé reste clos ; R2 renforce commandes et pins du lecteur sans
changer le test mathématique. Le préflight erroné àε1/2 est conservé
comme résumé historique, pas comme trace stderr complète. Deux
premières commandes de synthèse du contre-rejeu ont aussi échoué par
parenthèse mal placée ; les exécutions directes suivantes sont concordantes,
sans modification de la preuve.

Sur la grille entière1mm, homothétieM et translation(M,0,0) donnent
x=(M,0,0), a=(7M,0,0), b=(M,8M,0), z=(0,8M,0),
y=(7M,8M−1,0). Remplacer le dernier y par(7M,8M,0) déplace un
seul point d'une unité mais change la hauteur de(5−√12)M.
Les deux nuages sont dans u18 pour8≤M≤32767. Ce contrôle analytique
concerne K2, pas une qualification de clustering K5/LiDAR.

**Ce qu'il faut conserver dans MMtA.** `principe.py`, SHA256
`03ff7f006c1325beacc0011c653dd01e37238ecc708dd8b123a90ab1d96a027e`,
n'utilise plus l'ancre des seuls poids positifs. Pour sa couverture
exacte complète, S_x=Ahat_x=min_v max(c_x(v),A(v)) puisque Ahat_x≥Ax.
S_x est le premier niveau carré où une composante couvre x et au moins
mcs sites distincts. Si A(v) est hérité et v déjà mort, prendre son
ancêtre vivant à ce niveau ; la définition reste la même. Une composante
qui atteint le critère fournit réciproquement un candidat dans le minimum.

Sous un déplacement maximal ε des points appariés, en rayon, chaque
K-partie et chaque coface apparaît au plusε plus tard. L'inclusion des
graphes Γ_K conserve les labels et transporte une composante vers une
composante qui couvre au moins les mêmes points. Le critère « couvre x
et mcs sites » se transporte donc dans les deux sens : √S_x est
1-Lipschitz. Cette déduction porte sur FULL fidèle à ces composantes et
incidences ; elle ne suppose pas des indices de nœuds identiques.

**Borne plus forte sur le dénominateur : W≥ηS_x.** Supposer sites
distincts, 2≤K≤n, n≥mcs, η>0 et masses non négatives. À S_x,
prendre une composante vivante couvrant x et mcs sites. Sa lignée ne
perd ni x ni ces sites : après chaque fusion, l'ancêtre est admissible
dès sa naissance. Les intervalles demi-ouverts de cette lignée
partitionnent [S_x,(1+η)S_x) et contribuent chacun avec pente1 dans
`_masse`. Leur somme est exactementηS_x ; les autres porteurs
contribuent au moins0. Ainsi W>0, car K≥2 et sites distincts donnent
S_x≥Ax>0. Dans ce domaine, un repli pour W=0 est un invariant à
investiguer, pas un cas géométrique normal. n<mcs est hors hypothèses :
aucun cluster final de cette taille n'est possible.

Ces deux propriétés sont des preuves sur l'échelle et la masse,
recoupées par une seconde relecture du code. **Elles ne prouvent pas
la continuité des poids de préhistoire, des rivales ou des propriétaires,
ni la taille des clusters durs après attribution.** Ne pas transférer
le défaut de l'ancienne Π2c à MMtA ; garder S, puis éprouver séparément
ces décisions aux plateaux. Aucun port natif, résultat statistique
nouveau ou contrat G4 dans cette tranche.

### Localité LiDAR mesurée et choix de structures

La [contre-vérification exacte des grandes occupations](../../receipts/audit_continu_20260929/lidar_halo_argmax_exact_20261001/README.txt)
traite les douze cas de trame entière brut/sans-sol × trois trames × K5/K10,
sur grille1mm. Ces trames000000/000100/000200 appartiennent toutes à la
séquence08 ; ce ne sont pas plusieurs séquences. Le masque sans-sol est
celui figé dans v8, pas une nouvelle qualification de segmentation.

Pour K incluant x, d_K/2≤α≤d_K. La bande souple MMg exige le halo
B(x,2√(1+η′)α), dont les occupations sont encadrées par celles aux rayons
√(1+η′)d_K et2√(1+η′)d_K. η′=1 donne donc les deux décisions
entières d²≤2d_K² et d²≤8d_K². Ne pas lui substituer η′=1/4,
ni confondre cette largeur avec celle de la bande dure en rayon.

Dans les vecteurs sans-sol àK5, le halo majorant η′=1 a les médianes
35/35/38 et les grandes occupations11086/5232/13923. ÀK10 :
70/70/75 et12459/7862/14175. Les grandes occupations du halo minorant
àK5 sont1262/734/2743. Même la borne minorante peut donc nécessiter
des milliers de sites, pas seulement K voisins. Sur la trame brute000000
àK10, une ancre atteint79842 sites dans le halo majorant.

Ces grandes occupations sont **exactement réalisées** : 48 maxima de
colonnes, 28 ancres distinctes, 2124208 distances entières exhaustives
par passage. Les autres ancres des vecteurs viennent encore d'un index
cKDTree non certifié : médianes et maxima globaux ne sont pas promus
exacts. Les trois comptes minorants ci-dessus sont donc au moins des
minorants exacts des maxima réels. Les lecteurs indépendants trient les
distances au lieu du partitionnement du producteur ; normal/−O,
rejeu séparé et archive déplacée passent, faux SHA refusé. Manifeste
externe du paquet compact :
`9ccf19ecf98b11d32dbb2b9fd32e078bb5c30759d4a63a87ba25350aba667454`.
Le diagnostic initial36 cas/1152 ancres est également relu normal/−O,
sous le manifeste externe
`d7aa259033edb4ffebd60b359087c4873fd670f155efc651c4c8e7923fbdb741`.
Ses74Mo de vecteurs restent hors de Git ; le paquet publié dépend
explicitement de ces entrées LIVE.

**Décision d'architecture proposée :** utiliser une requête de rayon
complète et un travail variable partagé, pas une liste de taille
présumée constante. Répartir les grandes requêtes en tâches sans quota
sur leur résultat. Pour MMg, conserver les comptes combinatoires
implicites plutôt qu'énumérer aveuglément tous les supports locaux ;
la somme m⁴ n'est ici qu'un proxy d'une stratégie naïve, pas un nombre
de classes mesuré. Pour MMt/MMtA, garder les incidences de couverture
FULL et les masses de durée, modèle distinct qui évite cette énumération
mais paie encore ces incidences. Aucun coût FULL/G4 ni caractère
sous-quadratique nouveau n'est établi par ces contrôles de halos.

### Décision proposée pour les feuilles du juge R2

La question de `build/v10-integration-r2/notes/entrees_cli.md`, § 4,
oppose `leaf=max(8,K+3)` à `--allow-small-leaf --max-nodes=N`.
**Recommandation au développeur : retenir la première pour la campagne
normale des oracles.** Elle respecte la précondition du chemin produit
de `check_catalogue_params`. Garder les petites feuilles comme tests
diagnostiques distincts, avec budget explicite, refus et exhaustion
vérifiés. Ne pas assouplir la précondition du moteur pour sauver une
commande du juge, et ne pas confondre un budget de diagnostic avec une
troncature acceptée de la sortie. Choix de protocole proposé ; aucun
port ni nouvelle exécution native ne sont qualifiés par cette réponse.

## Réponse directe aux questions Q1 Q2 Q3

Les [trois questions du développeur](../REPONSE_CLAUDE_CONTACT_COMPTAGE_ET_JUGES_20260930.md#7-questions)
ont été relues le30 septembre à23 h25 UTC. Les réponses détaillées et leurs
contre-exemples sont [plus bas](#réponses-aux-trois-questions-sur-les-votes-de-bande).

1. **Q1, classes locales.** Oui, tous les sites nécessaires sont dans
   B(x,2R). Non, les seuls K voisins ne suffisent pas. L'occupation m de
   cette région borne les classes, mais K seul ne borne pas m : une
   famille K3 a déjà C(m−1,2) classes dans la bande. Préférer un comptage
   implicite ; ne pas matérialiser ces classes pour garantir le coût.
2. **Q2, univers exact à K≥3.** Les K-parties identifiées restent l'univers
   fidèle et continu. Des requêtes de rayon, ou KNN adaptatives jusqu'au
   rayon utile, retrouvent leurs sites ; leur coût n'est pas borné par K.
   Les paires d'ordre K et MMt sont des alternatives de modèle, pas des
   compressions équivalentes de ces votes. Choisir cette sémantique
   explicitement avant le port, en gardant un petit oracle K-parties.
3. **Q3, poids.** Oui, le poids souple dépendant du rayon évite le mécanisme
   de disparition étudié si les votes sont les K-parties fixes. Non, il
   ne le répare pas sur les boules fortes du catalogue. La stabilité
   obtenue reste conditionnée par l'univers apparié, son effectif,
   une échelle α positive et le domaine κ≥√(1+η′) de la règle à marge ;
   ce n'est pas une garantie uniforme d'ARI/EOM. Sans la marge de date,
   une majorité stricte peut encore changer brutalement de propriétaire.

**Conséquence pour le prochain port :** distinguer les votes exacts de
l'oracle MMg et les temps de couverture MMt. MMt est calculable à partir
de FULL et des incidences complètes, mais ne conserve pas les comptes de
K-parties. Une liste KNN adaptative jusqu'au rayon utile est admissible
pour Q1/Q2 ; une liste figée de K voisins ne l'est pas. Le rayon demandé
en Q1 est bien2√(1+η′)α, et non2(1+η)α sans convention reliant η etη′.
Une boule minimale de rayon≤R contenant x est entièrement dans B(x,2R),
donc cette requête retrouve aussi les sites intérieurs/coquille de ses
classes. Si cette occupation vaut m, les supports minimaux ont au plus
quatre sites en3D, donc au plusΣ_{q=1..4}C(m,q) classes distinctes.
Cette borne en occupation ne rend pas leur matérialisation sous-quadratique :
le contre-exemple K3 publié possède déjà C(m−1,2) classes autour de x.
Les regroupements par boule doivent conserver leur compte exact h_B et
leur propriétaire aux coupes utiles, pas un seul vote par classe.

Complément neuf pour le moteur : le [saut par orthants](#choix-par-orthants-avec-contraction-certifiée)
ci-dessous borne les reports à33 pour K5 et73 pour K10, avec contraction
certifiée du niveau. Cette déduction n'est ni un port ni un chrono G4.

## Compter une classe sans développer ses K-parties

1er octobre 2026, 00 h 30 UTC. Réponse constructive complémentaire à Q1/Q2 :
on peut supprimer l'énumération des K-parties **dans une classe fournie**,
y compris avec une grande coquille dégénérée. Cela ne génère pas les classes
manquantes du catalogue. L'oracle et le protocole sont dans le
[paquet clos](../../receipts/audit_continu_20260929/meb_shell_euler_transpose_20261001/README.md).

Soit une boule B de centre c et rayon strictement positif, avec p sites
strictement intérieurs I et u sites de coquille U. Les votes sont des
K-parties de sites distincts, non des multiensembles de retours LiDAR.
Leur MEB est B exactement lorsque le centre appartient à l'enveloppe
convexe des sites de coquille sélectionnés. Contenir le seul support
canonique n'est pas une condition équivalente. Pour la coquille octaédrique
±e₁,±e₂,±e₃ à K5, un point appartient à cinq parties de MEB B ; imposer
le diamètre canonique ±e₁ n'en compte que quatre pour x=e₁.

**Chemin simple à garder en premier.** Si U est exactement un support
minimal positif de taille q=2/3/4, toute partie de MEB B doit contenir U.
Le compte par point vaut alors C(p−1,K−1−q) sur I et C(p,K−q) sur U.
Ce cas doit être certifié, pas déduit de la seule taille de coquille.
Pour u≤4 quelconque, énumérer au plus 16 sous-ensembles de coquille suffit.
Ces chemins évitent de construire un arrangement dans le cas courant.

**Coquilles plus grandes : partager le calcul.** Pour chaque sous-ensemble
non vide S de U, les directions v vérifiant v·(y−c)>0 pour tout y∈S
forment soit l'ensemble vide, soit une région convexe ouverte de la sphère.
Le premier cas équivaut à c∈conv(S). La caractéristique d'Euler à supports
compacts de la seconde vaut1. On découpe la sphère par les grands cercles
v·(y−c)=0, sans déplacer les sites ni supprimer les signes nuls. Pour chaque
cellule ouverte C, poser σ_C=(−1)^{dim C}, P_C l'ensemble de ses sites
strictement positifs et n_C=|P_C|. Alors, pour j≥1,

$h_j=\binom{u}{j}-\sum_C\sigma_C\binom{n_C}{j}$

$h_{j,x}=\binom{u-1}{j-1}-\sum_C\sigma_C\mathbf{1}_{\{x\in P_C\}}\binom{n_C-1}{j-1}$

h_j compte les j-sous-ensembles de coquille entourant c ; h_{j,x} ceux
contenant x. Poser h₀=0 séparément : les directions du sous-ensemble
vide sont toute la sphère, de caractéristique2, et non une région de
caractéristique1. Un cercle isolé doit aussi être réellement subdivisé
en cellules ouvertes ; le cercle entier n'est pas une cellule ouverte1D.

Les intérieurs s'ajoutent par choix binomiaux. Pour le K demandé, le
compte h_B(x) de parties contenant x se simplifie directement en

$h_B(x)=\binom{p+u-1}{K-1}+\binom{p-1}{K-1}-\sum_C\sigma_C\binom{p-1+n_C}{K-1}\quad(x\in I,\ p\geq1)$

$h_B(x)=\binom{p+u-1}{K-1}-[S^{\mathsf T}\lambda]_x\quad(x\in U)$

Ici S désigne la matrice d'incidence C,x de P_C, pas un sous-ensemble
de sites. Le vecteur λ_C vaut σ_C C(p+n_C−1,K−1) si n_C>0 et0 sinon.
Les coefficients binomiaux invalides sont nuls ; p=0 n'invente aucun
point intérieur. Cette formule vient de Vandermonde et de la correction
du terme vide, pas d'une approximation ou d'une dérivée de binom(n,j).

**Pourquoi cela évite un scan de u points pour chaque cellule.** Ne pas
matérialiser la matrice dense S. Affecter des poids formels w_x et calculer
les sommes Σ_{x∈P_C}w_x par un circuit linéaire ADD/SUB : une face racine,
puis les faces adjacentes en changeant seulement les sites du cercle
traversé. Un groupe de cercles confondus contient au plus deux sites
distincts de même rayon, antipodaux. Une arête retire le groupe de signe
nul ; un sommet retire les groupes incidents de signe nul. Le nombre
total de ces incidences est O(u²), même si plusieurs cercles se croisent
au même sommet. Un parcours inverse de ce circuit applique Sᵀ à λ et
donne tous les comptes de coquille en un passage. Les coefficients
binomiaux sont déjà fixés pendant ce passage : ne pas les différencier.

Après construction d'un arrangement exact avec adjacences compactes,
le circuit et sa transposée coûtent O(u²) opérations entières. Préparer
les binomiaux pour n=0..u coûte O(Ku) avec une table ; les calculer
isolément peut coûter O(Ku²). On peut partager la construction entre
plusieurs K. Mémoire du circuit O(u²), puis comptes par K et par point.
La construction géométrique, ses comparaisons exactes et la taille en
bits des entiers sont distinctes et doivent être comptées. Le constructeur
Fraction du paquet est explicitement **cubique**, avec des tuples de
signes longs confinés à cette étape ; les faces du circuit ont des IDs
compacts. Ce paquet prouve la formule et le partage, pas un constructeur
industriel quadratique ni un coût GPU.

**Portée pour le clustering.** Les parties d'une même classe ont même
rayon et, à leur naissance puis aux coupes ultérieures, même composante :
le graphe d'échanges des K-parties contenues dans B est connexe et toutes
ses unions sont contenues dans B. On peut donc grouper leurs votes en
conservant h_B(x), la date exacte et le propriétaire vivant. Un poids qui
dépend seulement du rayon et de α_x est commun à ces votes pour x donné ;
il peut différer entre points. Ne pas remplacer h_B(x) par1 ni traiter
ces comptes de sites comme ceux de retours fusionnés sans poids déclarés.
Les sommes signées intermédiaires exigent leur propre borne numérique :
à 100 millions de sites, un compte final d'incidence K10 peut déjà exiger
221 bits. Les bornes géométriques B21/I192 ne qualifient pas ces masses.

**Contre-vérification close.** Dix coquilles rationnelles exactes,
480 configurations p=0..5 et K=1..8, 41 808 occurrences de sous-ensembles
(20 505 distincts mémoïsés), MEB indépendants par supports de Gram.
Formule directe à une transposée, convolution ancienne et MEB concordent.
Les identités du circuit sont aussi testées à poids formels signés.
Cinq variantes fausses sont rejetées ; lecteurs normal/−O et rejeux
indépendants de l'auditeur passent, SHA externe
`ea778eb2eccc6a2fbcdc246c137f8b8f06b9941a151c50f92a15bb2475a9cc0d`.
V1 reste un préflight historique privé, non publié ni réécrit.

**Suite utile, pas un grand port aveugle.** Garder le chemin u≤4,
instrumenter les tailles u et Σu_B² sur les classes réellement demandées,
puis juger si le fallback justifie son implantation native. La famille
K3 quadratique publiée reste une obstruction à l'énumération globale
des classes, même si chaque compte individuel devient bon marché.
Ni LiDAR, statistique MMg/EOM, parallélisation native ni contrat100ms
ne sont acquis par cette preuve locale.

## Causalité des tests du raccord R2

1er octobre 2026, 00 h 30 UTC. Relecture du collecteur et
[contre-épreuve close](../../receipts/audit_continu_20260929/collector_mutant_causality_20261001/README.md).
Source complète `mutants_entrees_cli.py`, SHA
`2555ba3c31eec86b62f3f11798ecc03a8afb2061ed8c921567dc670c4a892d51`.
Son main réel est appelé avec builds, juges et open remplacés en RAM ;
la substitution MA1 utilise le motif original exact. Quatre cas en
normal/−O : juge1 (contrôle), signal−11, délai `delai_1500s`, ID inconnu.
Les deux témoins avant/après restent verts. Signal et délai sont comptés
comme mutants tués/code0 ; l'ID inconnu donne zéro mutant/code0.

Ce n'est ni une exception native ni une panne d'infrastructure réellement
déclenchée : le défaut est la **classification** par le vrai collecteur.
Ne pas prétendre que les captures natives observées ont été faussées par
ces chemins. Ne pas compter comme juge causal un build en panne, signal,
délai ou code HARNAIS3. Exiger les IDs sélectionnés connus, une sélection
non vide, une occurrence par ID, le juge attendu et son diagnostic,
puis un témoin restauré terminal. `contre_epreuve_isolants.py` a le même
risque statique avec `killed_new` et un dernier build non vérifié ; aucune
nouvelle contre-épreuve de ce script n'est incluse dans ce paquet.

Le différentiel R2 de 43 cas est terminal ; les cinq binaires contrôlés
correspondent aux SHA du reçu privé et les dumps sont désormais exigés
présents/non vides, avec SHA complet et argv. Progrès réel à conserver.
Reste un garde d'inventaire : les familles lots/témoin partent des fichiers
observés via `os.listdir`. Deux côtés qui omettent le même fichier peuvent
encore paraître égaux. Dériver les noms attendus de K et de la configuration,
puis vérifier exactement cet inventaire, pas seulement son égalité.
Les 76 appels G4 simulés jugent la compatibilité des parseurs hors ligne,
pas une exécution G4, une sortie FULL ou plusieurs séquences LiDAR.

Archive de 25 fichiers textuels, 23 manifestés plus manifeste et sidecar.
Lecteurs hash-first normal/−O et contre-rejeux explicites des huit appels
Python passent ; trois mutations réépinglées du lecteur refusées code2.
SHA externe du manifeste
`0e18116416be562c17073c2717210ed4fda825a0034a44df39513cabacf95873`.
Runtime Python épinglé, bibliothèque standard extérieure déclarée ;
aucun moteur, compilation, CLI natif ou GCP lancé. Le développeur a repris
dans sa note privée les quatre refus mémoire et le rename tardif déjà
publiés ; leurs aides ont encore les mêmes empreintes au relevé.

## Contre-relecture nouvelle du raccord et des portes B21

30 septembre 2026, 23 h 54 UTC. Les réponses mathématiques Q1/Q2/Q3
ci-dessus ne changent pas. Deux actions ciblées utiles avant intégration :

**P2 : fermer les refus mémoire de l'entrée, pas seulement du calcul.**
Le [nouveau paquet clos](../../receipts/audit_continu_20260929/input_allocation_boundaries_20260930/PROTOCOL.md)
reproduit quatre exceptions `std::bad_alloc` qui échappent à la frontière
Result : `read_u32le_cloud`, `parse_real`, `parse_integer_list` et
`parse_head_configs`. Le lecteur construit implicitement un
`std::filesystem::path` avant son premier `try` ; les trois parseurs
allouent respectivement une chaîne, le vecteur de résultats et le vecteur
de tokens sans interception. Les quatre callers figés montrent que ces
appels ne sont pas entourés d'un garde global ; le `try` d'index mreach
arrive après eux. Ce constat est distinct du deuxième rename déjà reproduit.

Sources du raccord privé après `6d2d3bc5`, index/travail non committé,
pas qualification de ce HEAD : `u32le_input.hpp`
`c937f225ba6db15d943c259920af0dbc69378aa9bdef7a640ff28b3f363d6a7a`
et `cli_options.hpp`
`f3cb844d4104a9a82080d98cb7e9812fdfedc861d10c948660293681b293dd53`.
Les hashes de neuf sources avant/après coïncident. Cinq headers complets
compilés, quatre callers complets comme preuve statique, pas liés à la
sonde. Quatre témoins valides puis faute à la première allocation C++
ordinaire de chaque aide : quatre exceptions échappées, zéro Result de
refus mémoire, FD4→4, deux fichiers intacts dans les huit cas.
Le code0 confirme les défauts attendus ; ce n'est pas un produit correct.
Les champs `status=ok/reason=none` sont les valeurs initiales du harnais
quand `returned=false`, pas un Result retourné par l'aide.

Recompilation indépendante de l'auditeur, GNU13.3, C++20,10s maximum
par compilation/exécution : mêmes huit observations, même SHA binaire
brut `6b6a40d07b07868c067c3b9f6cf4ff2798351923561879cde667c00d8d122b8a`.
La première taille d'allocation refusée vaut101 contre104 dans la capture,
car le nouveau chemin privé est plus court ; ce n'est pas un invariant.
Aucun moteur, CLI HGP, sanitizer ou GCP lancé. L'archive autonome a
quatorze payloads textuels plus manifeste, SHA externe
`ce419a214ede29039325b25c17e129b24f00570b9a07cf9e91997e6221ba7a53` ;
lecteurs normal/−O recoupés, faux SHA externe et deux mutants du lecteur
refusés. Ils ne réexécutent pas le binaire omis. Toolchain identifiée,
pas fermée par un hash préalable.

Correction demandée : traduire les allocations de préparation en
`memory_budget`, y compris conversion de chemin et parseurs ; conserver
RAII/fermetures sur tous les chemins. Vérifier aussi les allocations
propres aux CLI, telles que `dump`, `configs`, `entries` et listes
de paramètres, au-delà des quatre aides. Un garde `bad_alloc` de haut
niveau, avec impression non allouante, peut protéger le processus ;
il ne remplace pas le contrat Result des aides. Éviter un `catch (...)`
qui transformerait toute erreur de programmation en refus mémoire.
Ajouter des fautes d'allocation et témoins positifs avant l'import ;
la sonde présente ne couvre pas toutes les allocations ni toutes les
plateformes. Le contenant u32le n'élargit toujours pas le moteur u18.

**B21 : progrès des portes réellement observé, clôture encore partielle.**
HEAD privé `30c66d82eb4f6019684168896baa9b80c4067c14`. La porte20/20
termine code0 ; log SHA
`932e7cee8a685153d129cdee5139a28dab5fc1286b5ef20c0cf61c8ddfba6d56`.
Les quatre derniers commits ne modifient que le test de précision.
Relecture positive : FX-BANDE exerce les marges aux sites d'usage,
pas seulement les getters ; FX-SAUT appelle le sélecteur réel sur un
premier site étroit et un second qui exige la voie large ; FX-D22 forge
les bords acceptés/refusés sur chacun des axes à B18/20/21.

Au relevé23 h49, la suite38 noms relancée utilise une copie de porte
SHA `e1c9d65a09ee7fb34cfbdf40720fbf0e2029f182b457750fa2a4d61378f9e2fb`.
Témoin Release/San0/0 ; `jump_reach_first_site` Release1 et San1 avec
diagnostic de débordement signé ; `sitetree_band_0p02` Release1 par
les six/quatre décisions exactes ; `tower_meb_band_0p02` Release1 par
le gain exact MEB. Ces deux derniers jugent le filtrage/repli, pas une
sortie géométrique fausse démontrée. Le juge M1 vérifie conjointement
voie large et site choisi : la ligne d'échec seule n'isole pas lequel
diffère. `leaf_flag_z_ignored` et `domain_limit_off_by_one` ont aussi
leurs décès Release reçus. Pas encore de total/rc de clôture ; préserver
la partie9f archivée, ne pas fusionner leurs versions implicitement.
Aucun de ces tests lancé/arrêté par l'auditeur.

**Clôture de cette suite, 1er octobre, 00 h 34 UTC.** Le journal est
maintenant terminal :38 mutants,35 TUE,3 EQUIVALENT, rc0,41m43s.
SHA du log `rep_mutants_campagne_30c66d8_suite.log` :
`7794722346c864fa00f8d459c3715e0edffdbd46a5c2fe76f29b108d79387c5b`.
SHA du JSONL39 lignes, témoin compris :
`71ace2776d0d780b0b8b0135a0b768462dd0a7cfb535c2b878fce35ab3efa27e`.
Les38 noms correspondent exactement à la sélection et à l'ordre de la
table, sans doublon/inconnu ; les26 noms antérieurs sont disjoints.
Leur union couvre64 noms mais sur **deux versions de porte**, pas64
rejeux de la30c. Les35 rejets ont tous Release1 et un ECHEC ciblé ;
aucun signal, délai ou code3 ne produit ces décès. Quatre ont également
un diagnostic UBSan de débordement signé. Les mutants de marge et de
compteurs prouvent la sensibilité des portes, pas nécessairement une
mauvaise sortie géométrique. `meb_exact_uncounted` le montre explicitement.

Les trois survivants annoncés équivalents passent0/0. Leurs domaines
doivent rester écrits :

- `jump_key_wide_2limbs` : clés intérieures d'une MEB B21 et ancre de
  support, pas centre arbitraire ; deux mots en signe-magnitude couvrent
  la borne de clé déjà publiée.
- `census_guard_removed` : support valide et census exact donnent
  p+m≥K. Conserver cette garde défensive ; son équivalence n'invalide
  pas les tests d'erreurs de census.
- `nearest_box_band_0p02` : dans la tour actuelle, `nearest` est appelé
  au centre entier d'un site ; les distances de sites/boîtes sont
  exactes en double, leurs écarts entiers et les marges dans(0,1).
  Pas d'équivalence générale pour des centres rationnels de SiteTree.

**Dernier garde de clôture :** le collecteur restaure les huit sources
pertinentes, mais ne reconstruit ni ne juge un témoin final. Le binaire
Release mutable reste celui de `meb_exact_uncounted`, celui sanitize
de `jump_wide_as_narrow`. Ne pas les réutiliser comme témoins des sources
restaurées. Exiger un build/témoin final vert ; conserver stdout/stderr
complets, argv et empreintes sources/binaires par mutant. Actuellement,
seul le premier diagnostic tronqué à170 caractères est conservé.
Le script est encore `5cddff2f…`, avec les défauts de sélection et
classification déjà signalés ; ils ne sont pas les chemins des35 rejets
observés ici. Aucune campagne relancée par cette contre-relecture,
aucun contrat FULL/G4/100ms transféré de ce résultat numérique.

Deux gardes auxiliaires à ajouter, sans réouvrir inutilement le moteur :

- Dans `precision_b21.cpp::audit_main`, `ok=true` puis boucle sur les
  ordres présents ne vérifie pas leur inventaire. Exiger `kmax=min(K,n)`,
  exactement ces ordres1..kmax, chacun une fois, et le mode demandé.
  Une omission échappe à cette boucle ; ce n'est pas une tour vide
  observée dans le moteur actuel, qui prépare bien ses ordres.
- Publier `shadow_enabled` lié à la bibliothèque réellement compilée,
  puis les obligations testées et violations ; un zéro peut être
  légitime pour le cube200, où aucune feuille n'utilise la voie courte.
  Les planchers positifs doivent être propres aux fixtures qui
  atteignent ce chemin. `ombre_reelle.sh` ne garde que les lignes
  filtrées ombre/attach_audit/diagnostics : conserver aussi le stdout
  complet pour vérifier les ordres et l'argv exact. Script SHA
  `5a95ca655443d188a80eb7aa42d50d287d13e67dda84b454135293a58462b91c`.
  Les compteurs tétra portent sur quatre obligations de portée par
  passage, pas nécessairement quatre orientations produit exécutées.

Le parseur privé de ce mode utilise encore `atoi` et des triplets `fread`
sans garde de queue/erreur ; reprendre les nouvelles aides à l'intégration
après correction des refus mémoire. Cela ne réattribue pas cette faiblesse
aux CLI P2 réécrites. Les portes numériques renforcées sont utiles ;
elles ne qualifient ni u24/u32 ni une trame1mm/G4 ni le contrat100ms.

## Raccord courant et six décisions utiles, 30 septembre, 22 h 35 UTC

Les réponses Q1/Q2/Q3 ci-dessous restent applicables. Complément pratique :

1. **Seeds MMt.** Normaliser chaque incidence vers le propriétaire vivant
   à son niveau d'activation, notamment activation=mort et plateaux,
   avant déduplication. La compression à ≤2S nœuds suppose ces seeds
   complètes ; ne pas lui transférer cette obligation. Garder les entrées
   uniquement internes K3/K5, pas seulement les feuilles. Mesurer ΣI/ΣS
   sur LiDAR et coupes capteur avant de promettre le coût de la tête.
2. **Domaine de κ.** Le prototype accepte tous η,κ>0, mais sa forme finie
   oublie l'unanimité atteinte continûment. Le
   [reçu minimal](../../receipts/audit_continu_20260929/mmt_final_endpoint_20260930/README.md)
   utilise les fonctions réelles épinglées, aides exactes privées : deux
   sites (0,0,0),(2,0,0), K2, A1 et η2/3. À κ1/10, le supremum défini
   vaut √(5/3)−1/10, contre √(4/3) dans le code. À κ2/15, le maximum
   critique 139/120 est exact ; à κ4, √(4/3) est exact. Contrôle positif :
   admettre G(e⁻)=W seulement à la première unanimité corrige ces trois
   cas, pas qualification générale d'un correctif. Alternative simple :
   déclarer le domaine κ²≥1+η de la preuve de stabilité. Le théorème et
   le réglage recommandé (4,2/3) ne sont pas réfutés par ce cas.
3. **Modèle statistique.** Temps de branches MMt, K-parties et paires
   d'ordre K sont trois modèles de masses différents. Le mémo statistique
   privé ne qualifie pas MMt sur ses mélanges/modèles atomiques ; la cible
   des triangles seule ne démontre pas la supériorité EOM/ARI. Comparer
   avec la même condensation corrigée, les mêmes réglages EOM et les
   mêmes jeux synthétiques, sans confondre bande dure et marge continue.
4. **Rangs exacts avant condensation.** Conserver le rang séparément du
   double ; quotient des plateaux puis cohortes de points. Ces deux
   verrous déjà reproduits restent ouverts, même si le raccord des bancs
   ne change aucune ancienne étiquette.
5. **Exposant EOM des contrôles durs.** Dans `dev_scenes.selection_block`,
   `(atts, _phi)` ignore l'échelle qui a produit les attaches. `hard_stats`
   prend directement `a.phi_date`, puis soustrait une mort dans la nouvelle
   échelle. Cela mélange des quantités différentes. Pour β4, mort β25,
   z_att2→z_EOM3 :121/500 au lieu de117/1000. Pour β9, mort β25,
   z_att3→z_EOM2 :−2/675 alors que la date est admissible. Réexprimer
   les dates en φ_EOM, avec β exact quand disponible ; sinon conserver
   et convertir leur expression certifiée et leur exposant d'origine,
   jamais une inversion flottante. Le
   [reçu autonome](../../receipts/audit_continu_20260929/selection_semantics_20260930/README.md)
   ferme six cas exacts et les gardes du vrai appel, cinq sources
   complètes, lecteurs normal/−O et manifeste externe. Les diagnostics rappel/jitter ont
   leur échelle propre ; ne pas les invalider par transfert automatique.
6. **Sémantique de `min_cluster_size`.** Le contrôle dur Python répète
   la condensation par seule masse finale du C++ : la fixture API21 à
   mcs5 donne {A,B,C}, contre {R,C} en intégrant les départs et la fin
   lorsque la masse restante devient trop petite. Question au développeur :
   pour les masses progressives, mcs vise-t-il la masse instantanée ou
   la masse finale admissible ? La seconde définit une condensation
   structurale possible, mais elle n'est pas la condensation standard
   des points. Garder celle-ci comme contrôle séparé et ne pas revendiquer
   l'équivalence Campello/HDBSCAN. Le même reçu confirme37/10 contre11/5
   pour A et la bascule EOM sur l'API21 ; aucune nouvelle réalisation
   géométrique, invocation C++ ou HDBSCAN dans cette tranche.

Le commit bancaire privé `d2640c8f89e5078f53a4fd4b2f925c44d16bfb77`
est maintenant clos : 92/92 hashes recoupés, 26/26 gates et 7/7 fast
archivés, pas nouveaux lancements de notre part. Ses sources fusion,
comparateur et collecteur sont identiques aux captures de notre dernière
publication : alias de fichiers, `zip` sans inventaire et retour0
inconditionnel ne sont pas réparés. Bilan91/4 : union et requalification,
pas nouveau lot95 final. Distinguer cette étape du chantier pool suivant.

Pool préparé : pas de nouvel UAF/course identifié par les deux relectures.
`next` sature à n ; un seul écrivain d'exception ; mutex/users ferment la
durée de vie du Job et du callback avant relance. Catalogue/tour détruisent
leurs objets partiels avant `memory_budget`. Petite sonde pool-only,
trois répétitions, pas qualification HGP/sanitizer. Les nouveaux ajouts
`make_pool` du worktree restent distincts de l'index audité. La lecture
supplémentaire de la factory confirme les deux conversions
`system_error`/`bad_alloc` vers `resource_exhausted/session_overhead` et
les quatre points d'entrée, sans ouverture de sortie avant refus. Le
témoin mreach a toutefois déjà construit son index : dire « avant calcul
HGP/MR », pas « avant toute préparation ». Le test RLIMIT suppose Linux,
glibc et des piles de 8 Mio ; sa limite de 1 Gio est virtuelle, non RSS.
Pas de nouveau test natif ou de limite réelle lancé par cette relecture.
Pool(1) ne
marque pas sa région, réserve de contrat préexistante hors chemin moteur
observé, pas un blocage du correctif multithread.

Suivi21 h50 : le groupe Pool est clos et committé privé
`7edb91123be00796563f333feaca77d70e201163`. Le nouveau terminal Release
est39/39, code0,936,50s, copie `preuves/ctest_gate.txt` SHA
`fcab734f03a8838de31abd18da454af8742c260254f95072ec12af9ce05eef80`.
Le passage2242,69s reste dans `ctest_gate_avant_rafales.txt`.
`preuves/mutants/campagne.jsonl.txt`, SHA
`eb699ff4555dbdb925036a946c3408fc1ce5cfa77e1b766600476847a0a23a3a`,
ferme47 enregistrements et la synthèse42/42 tueurs tués,4/4 survivants
attendus, témoin vert. MV3 donne maintenant1,1,1 ; le premier passage
interrompu1,0,1 reste conservé, pas effacé. Le reçu possède maintenant
son inventaire156 pièces et SHA256SUMS externe
`e1e9bb08ddb3fb98f28ab161771bc7049d1d82935206ba5cfa9ce1beb2f77537` :
empreintes avant/après, inventaire fermé sans liens,47 enregistrements
et synthèse recoupés normal/−O par cet audit. Les placeholders observés
à21h41 ont été retirés ; seuls sept fichiers de reçus diffèrent du
commit intermédiaire455fd76, pas le code. Aucun nouveau test natif.

Les traces tour10/10 et catalogue10/10 impriment ref/int1/int4 en SHA256
complets et FIN echec0 ; tête18/18 compare les SHA complets en mémoire,
mais sa trace conserve encore seulement16 caractères et `<travail>`.
Ne pas dire que **toutes** les empreintes complètes et argv sont publiés.
Ces différentiels sont une non-régression : ni condensation corrigée,
ni croissance à K constant, ni chrono G4. Les trois noms LiDAR00/01/02
de cette suite ne prouvent pas plusieurs séquences SemanticKITTI.

Suivi22 h35 : l'intégration est au HEAD privé
`6d2d3bc5d1c34deca95f3ba6cc7929179517680c` pour P4. Le nouveau P2
est dans le travail/index suivant, pas dans ce HEAD. `cli_output.hpp`,
SHA `754a6ff111962b2d35b9931aa18cde54203079800ee3a5814414c984009aaaf0`,
remplace bien la réservation destructive : déclarations/alias avant
création, temporaires, RAII et commit contrôlé par les quatre CLI,
y compris sorties `.vote` et entrée configs. Relecture statique, pas
campagne rejouée. Ne plus attribuer les anciennes troncatures à cette
réécriture. Un verrou reste : si le deuxième rename échoue, le premier
est déjà publié et son ancien contenu n'est pas restauré. Le header
reconnaît cette limite mais promet aussi « tout ou rien ». Exiger un cas
de faute au deuxième rename avec deux sentinelles préexistantes, puis
une politique cohérente de restauration/publication ; le contrôle du
code d'erreur seul n'établit pas la transaction globale. Les réserves
sur les bancs committés restent distinctes de ce correctif en cours.

Suivi23 h17 : le
[contre-audit natif borné](../../receipts/audit_continu_20260929/outputset_transaction_20260930/protocol.md)
confirme désormais cette limite au snapshot `cli_output.hpp`
`763e2ee3bb74ab4b40042eb0efcdc7f47f62e7dd53a6dcda610076925792e20e`.
Cinq cas compilés avec les cinq headers complets : succès, abandon avant
commit, `bad_alloc` du writer, EIO au deuxième rename, double commit.
Le quatrième retourne correctement `output_unwritable`, mais laisse
NEW_A/OLD_B au lieu des deux sentinelles originales. Zéro temporaire
résiduel, aucun FD perdu ; l'idempotence est maintenant corrigée.
Le code0 de la sonde confirme les observations attendues, **y compris
le défaut**, pas le contrat « tout ou rien ». Recompilation indépendante
de cet audit : même SHA brut du binaire
`e859cfe91830ee2f300dba5ec229a8b8deaf5a3612ec8bc975460d12b684fc03`,
mêmes cinq observations ; FD4→4 contre5→5 dans la capture initiale.
Aucun moteur ou CLI HGP compilé, sanitizer ou GCP lancé.
Archive textuelle close, dix payloads plus manifeste, SHA externe
`00d12d5faf46fbee2d3d7a2201032c33215b0ba8bf4cc9db861f990a4b9d33cc`.
Lecteurs normal/−O recoupés ; ils vérifient code/capture, ne relancent
pas le binaire omis. Le compilateur a été identifié après capture,
pas rétrospectivement épinglé avant elle. Préserver les anciens reçus.

Correction proposée au développeur : sauvegarde des originaux jusqu'à
publication complète, restauration éprouvée à l'échec tardif et état
de récupération explicite si elle échoue. Des renames successifs sur
plusieurs chemins ne garantissent pas à eux seuls une publication
atomique globale ; ne pas la promettre aux lecteurs concurrents ni
après crash. Une génération de sorties publiée par un unique pointeur
ou manifeste atomique est une autre interface possible, à décider
explicitement plutôt qu'à introduire dans ce raccord en silence.

Le lecteur `u32le_input.hpp` désigne un contenant32bits, pas une nouvelle
précision géométrique : les CLI de ce raccord utilisent encore le domaine
u18. Le palier B21 reste séparé. Les IDs sont les rangs de ce fichier,
donc garder la correspondance des retours capteur sans sol. Le lecteur
conserve12n octets bruts puis16n octets XYZ/IDs simultanément, au moins
28n hors budget moteur ; ne pas oublier cette entrée dans le contrat massif.

Palier privé B21, lecture précédente épinglée à
`5dd83b5c68919d87ada067204d19ee4edb166859` :
relecture des voies étroites/larges et des niveaux I192/I192 cohérente.
Le [snapshot de bornes](../../receipts/audit_continu_20260929/b21_bound_counterreview_20260930/README.md)
ne compile ni n'appelle le moteur. À ce pin, trois constats documentaires :
delta annoncée fausse ; arrondi du seuil omis dans la chaîne grossière ;
hypothèse MEB nécessaire dans la preuve fine d'I3. À B21, le majorant
grossier avec demi-ulp donne0,03917965>m=0,0390625 ; ce n'est pas une
erreur géométrique démontrée. Au même centre, le terme quadratique de
décalage s'annule dans la différence des distances ; entre MEB, le
rayon carré est≤3L²/4. Ces bornes plus fines ferment la marge, arrondi
inclus. Ne pas augmenter la marge
sur le seul échec d'une majoration trop grossière. CLI fine, raccord R2,
u24/u32 et nouvelle qualification G4 restent ouverts. Le ledger privé
`10d8f386…` ajoute désormais la distinction des décisions ; le snapshot
publié `6adf2721…` reste inchangé. Une dernière précision de preuve est
nécessaire : l'exactitude de `r2a−m`, et l'affirmation « addition inexacte
seulement au franchissement de binade », supposent ici `r2a≥m`. Par
exemple `r2a=2^-100`, `m=5/128` donne `fl(r2a+m)=m`, inexact sans
franchissement. Si `r2a<m`, un seuil intérieur négatif ne peut accepter
une distance non négative ; cela ne démontre aucun défaut du filtre.

Le rapport adverse privé B21 `notes/verif-mutants/RAPPORT.md` rédigé à
21 h02, SHA `d60167da9f31421ef2ef1286215dfef2bcad9af6fe3efa06b27dbb59b3e286ca`,
rend FAIL sur les **portes**, sans sortie HEAD erronée découverte.
M1 : prendre la portée du saut sur I[0] seulement survit aux portes,
mais coins graine49 donne un débordement i128 sous UBSan dans le mutant.
M2 : une marge0,02 aux sites d'usage reste invisible si seuls les
getters sont jugés ; SiteTree dispose d'une fixture6 contre4 décisions
exactes, la certification MEB manque encore d'observable. Axes de feuille,
bord exact du domaine et ordre du saut ont aussi des trous. Rapport et
sondes relus, aucun rejeu natif de notre part ; ce rapport épingle5dd83b5c,
pas les corrections suivantes.

Un résultat positif ferme toutefois l'équivalence `jump_key_wide_2limbs`
sur ce tri. Pour trois points du cube[0,L]³, chaque axe donne une somme
des trois différences carrées≤2L² : si δ partage leur étendue w≤L,
la somme vaut δ²+(w−δ)²+w²≤2L². Donc les côtés vérifient
a²+b²+c²≤6L², puis AM-GM donne a²b²c²≤8L⁶. Le vrai code q3 a
`D=2|u×v|²`, `r²=a²b²c²/(4|u×v|²)` ; la clé d'un intérieur est
`s=D(|z−centre|²−r²)`, donc |s|≤Dr²≤4L⁶<2^128 à B≤21.
q2 donne≤3L²/2 ; q4 a le majorant général144L⁵<2^113. Le mutant
conserve signe et deux mots : ses mots supérieurs sont donc exactement
nuls. Preuve analytique recoupée sur les quatre sources du5dd83b5c,
pas maximum numérique ni nouvel essai. Elle ne rend pas sûre une
évaluation i128 intermédiaire. L'invariance topologique après changement
de sélection du saut p≥K est maintenant prouvée ci-dessous ; elle
ne promet pas l'égalité des témoins internes bruts ou de leurs compteurs.

Suivi22 h45, lecture figée à `7af07c53e0a48480593f3d228c6f4c981275d076` :
les comparaisons portent désormais sur l'écart flottant et la marge
représentable. Pour m double, `fl(t)>m ⇒ t>m`, par monotonie puisque
`t≤m ⇒ fl(t)≤fl(m)=m`. Les deux erreurs des distances, majorées par
2ε≤m, suffisent ; pas de troisième marge de soustraction à ajouter.
Le raisonnement couvre les usages relus dans SiteTree et la tour.
Pour nearest, si τ est la K-ième distance exacte, le seuil approché
courant w vérifie w≥τ−ε ; tout vrai voisin retenable a d≤τ+ε≤w+2ε.
Ce résultat reste limité au domaine B≤21, arrondi au plus proche et
compilation flottante stricte, avec les replis exacts hors domaine.

Précision au développeur : l'ancienne comparaison **stricte** avec seuil
arrondi était aussi sûre par monotonie de l'arrondi. Pour d représentable,
`d>fl(r2a+m) ⇒ d>r2a+m`, et symétriquement pour le seuil intérieur.
La majoration grossière de l'arrondi ne démontrait donc pas une ancienne
erreur géométrique. Le nouveau code rend la preuve plus directe ; ses
replis peuvent différer, sans que ce soit un changement de l'objet exact.

Les nouvelles sources de portes adressent portée complète du saut,
marges aux sites d'usage, trois axes de feuilles, bord du domaine et
ordre/départage des clés. Aucun nouveau reçu de campagne n'est clos par
cette relecture : ajout d'une porte ne signifie pas mutant tué. En
particulier, les compteurs de tour de FX-BANDE exigent hi>lo, pas une
valeur exacte gravée. MODES compare les chemins sous une **même politique
de sélection** ; ce n'est pas un invariant de Γ pour deux choix différents
de G. Garder FX-SAUT comme contrat du sélecteur exact privé, et juger un
futur saut par orthants aussi par les propriétaires aux coupes utiles.

Suivi23 h20 du palier B21 : HEAD privé
`30c66d82eb4f6019684168896baa9b80c4067c14`. Les nouveaux logs donnent
16 différentiels tour/tête et4 catalogue avec FIN ecarts0,6/6 fast
au30c66d8,1/1 sanitizer au9f54c5b, ombre réelle positive/violations0
et FIN echecs0. Ce sont des scopes différents, pas une capture finale
unique de FULL ni une précision LiDAR qualifiée. La campagne mutants
9f54c5b reste partielle au relevé ; l'essai1113223 interrompu reste conservé.
Aucun de ces appels relancé par cet audit.

Avant clôture, deux gardes de collecteur restent nécessaires. Lecture
du script `identite_reparation.sh`, SHA
`aaebf4f510064d7837c054f26ce480e20bb7a43a5571f3267820136775c8e335` :
un moteur code0 avec dump absent donne un hash vide accepté par le
pipeline `sha256sum | cut`, sans `pipefail`. Exiger existence/lecture,
succès de hachage et SHA complet ; le problème ne démontre pas que les
digests non vides observés soient faux. Lecture du collector mutants,
SHA `5cddff2f3f96dfcf3872637ddf7ecc98872b4b41c65b168c8635c1bcb5b9e621` :
`--only` inconnu sélectionne zéro mutant et peut finir total0/code0 ;
un code sanitizer entier non nul, même un signal sans diagnostic causal,
devient `TUE_SANITIZE`. Refuser les sélections vides/inconnues,
distinguer mutation jugée, diagnostic sanitizer et échec d'infrastructure,
garder les sorties/argv/binaires complets et fermer l'inventaire.
Constats de code, pas une nouvelle campagne de mutations exécutée.

## Cache des descentes profondes et mesure utile avant le port

Relecture du code B21 `1113223c9052ce429a677d4063700dc3d3ab8bfb` :
`tower.cpp`, SHA
`e346a03222a4427dad96dfe359a80060c6a830f63b30fe40affe09f1dd6560f9`.
Ce constat est structurel, pas un nouveau profil LiDAR. L'index `seeds`
ne contient que les naissances régulières préparées avant résolution.
Les branches p=K et p>K font `continue` sans ticket `pend` ni mémo
Atlas. Une cellule Atlas n'existe que pour K≥p+q_min−1 ; à rayon positif,
q_min≥2, donc un état p≥K n'en a aucune. Certains états p<K n'en ont
pas non plus : q_min=3 avec p=K−1, q_min=4 avec p=K−2 ouK−1.
Déplacer le lookup Atlas
avant le census ne corrigerait donc pas cette absence.

Même pour un état éligible, le HIT Atlas actuel arrive **après** MEB,
recherche du catalogue et census. Le census hors catalogue matérialise
et trie I/U ; p>K parcourt ensuite I pour la sélection. Le contrôle
exact de catalogue d'une boule sur32 est aussi répété à chaque passage
sur cette même boule, sans bit indiquant sa validation antérieure.
Un mémo peut économiser la descente suivante sans économiser ce préfixe.

Proposition distincte, non implémentée : cache par K-partie complète
triée, K et propriétaire immuable du nuage, consulté avant MEB. Un cache
par identité géométrique exacte de boule pourrait regrouper davantage
d'états, mais exige déjà sa MEB ; rayon seul ou hash seul sont insuffisants.
Stocker une naissance témoin, puis remonter à la coupe demandée, pas un
propriétaire final valable à toute hauteur. Deux K-parties contenues
dans la même boule B sont liées par échanges dans Γ_K(β(B)) : chaque
coface reste dans B. Leurs descentes exactes donnent donc la même
composante à coupe fermée λ≥β(B), et pré-lot λ>β(B), pas à β(B)⁻.
Cette preuve autorise un témoin non canonique avec la bonne remontée.

Ne pas muter `FlatIndex` tel quel pendant ces recherches : `find` lit
des slots non atomiques après la barrière de construction, et son test
de clé utilise la population du seed terminal. Ce n'est ni une table
des états profonds ni un cache dynamique sûr. Les loads/stores atomiques
du mémo existant publient des résultats, mais ne réservent pas un calcul :
plusieurs workers peuvent lire le même miss et refaire la descente.
Séparer cache READY et déduplication des calculs simultanés ; si celle-ci
est nécessaire, traiter publication, exceptions et évictions, sans
attente active GPU d'un producteur qui n'est pas encore ordonnancé.

Avant un chantier massif, mesurer sur les trames sans sol : états
distincts et visites répétées, p=K, K<p≤8(K−1), grands p, p<K sans cellule,
MEB/census, visites d'index, volumes I/U, cache froid/chaud, un/quatre
workers et mémoire réelle. Sur la famille forcée publiée m16/K3 ouK5,
la prévision analytique est15 sauts pour chaque F0 répété et120 pour
toutes les couches sans cache profond, contre15 états profonds distincts.
Ce petit diagnostic n'a pas été lancé nativement. Il motive un test,
ne prouve ni un carré LiDAR actuel ni un gain vers100ms.

## Saut intérieur sans tri des plus proches

Résultat mathématique recoupé sur le modèle Γ_K, pour sites distincts
et poids1,2≤K≤n. Soit F une K-partie, B sa MEB, de centre c et
niveau carré β. Si l'intérieur strict I de B contient au moins K
sites, choisir une K-partie **quelconque** G⊂I donne
`β(G)≤max_{g∈G}|g−c|²<β`. Le tri par distance au centre n'est donc
pas nécessaire à la décroissance stricte.

F et G sont liés dans Γ_K(β) fermé : une chaîne d'échanges unitaires
dans F∪G transforme F en G ; chaque coface de K+1 sites est contenue
dans B, donc sa MEB a niveau≤β. Les descentes suivantes restent liées
à F sans dépasser ce niveau. Pour un représentant de lot λ, PO-T2
donne β(F)<λ : tout choix reste ainsi dans la **même composante
pré-lot λ⁻**. Pour une attache ou une verticale à coupe fermée
λ≥β(F), la conclusion est également la même.

Attention : la feuille terminale n'est pas canonique. À K2, F={0,10},
I={1,2,8,9}, les choix {1,2} et {8,9} donnent deux naissances
différentes, déjà réunies au niveau25 de F. Comparer les ancêtres
au niveau utile, pas ces IDs. `Min(c)` dans TOWER_v2§6.3 est un point
fixe du pointeur de descente, pas l'argmin de toutes les naissances.
Le modèle Γ_K est explicite au§3.2 ; PO-T1/2/4 ferment la preuve.

Raccord source relu dans B21 `5dd83b5c` : `tower.cpp` branche p≥k,
vers976–1030, choisit aujourd'hui les plus proches ; mémo vers1064/1089
stocke une naissance témoin. Kruskal1169–1196 normalise les racines
pré-lot puis impose séparément le minimum DSU ; `cover_node`1651–1652
appelle ensuite `ancestor` au niveau propre. Les attaches core et les
verticales font aussi cette normalisation. Pas de modification moteur,
test natif ni mesure nouvelle dans cette preuve.

Proposition de petit essai : un parcours d'index qui rapporte au plus K
**intérieurs certifiés**, puis saute immédiatement. Si ces K sites
n'existent pas, poursuivre le census exact complet nécessaire à la
structure locale ; réutiliser la continuation plutôt que recommencer.
Un simple retrait du tri économise la sélection, mais pas la collecte
actuelle de tout I/U. Ne pas appliquer cette preuve à p<K, à K points
de coquille, aux multiplicités sans leur preuve distincte, ni à une
nouvelle règle de clustering. L'arbitraire peut rallonger les descentes :
le gain et les histogrammes doivent être mesurés avant le port massif.
Conserver aussi la **vraie requête k-NN des points**, qui calcule D_K(x)
avant `resolve` : le lemme n'autorise aucun remplacement de cette
densité par une K-partie arbitraire. Pour l'essai, comparer les coupes
ouvertes/fermées de Γ, les propriétaires et les verticales ; ne pas
juger seulement les feuilles terminales ou les compteurs de descente.

### Choix par orthants avec contraction certifiée

Déduction mathématique contre-relue, pas prototype produit. Pour la même
branche de résolution p≥K, parcourir UN flux des sites strictement
intérieurs, sans doublons. Affecter chaque site à l'un des huit orthants
de centre c selon les signes **exacts** de ses trois coordonnées moins
c ; attribuer l'égalité au côté positif. Conserver au plus K−1 IDs par
orthant. Dès qu'un orthant reçoit son K-ième site, prendre ces K sites
pour G et arrêter le parcours.

Le flux s'arrête au certificat ou à EOF. S'il fournit
`Q=8(K−1)+1` reports, un orthant a nécessairement K sites : Q vaut9 àK2,
33 àK5 et73 àK10. Le stockage et le regroupement des IDs sont O(K).
Ce seuil est un certificat géométrique d'arrêt, ni un plafond de recherche
ni une troncature d'un census requis. Les signes doivent utiliser le centre
exact, pas un centre double arrondi ; aucun rayon irrationnel à calculer.

La contraction est même meilleure que celle du cube de côté r. Après
réflexion des coordonnées, poser u=z−c≥0 et t=|u|<r. La boule auxiliaire
de centre a=(r/3,r/3,r/3) vérifie
`|u−a|²=t²−(2r/3)Σu_i+r²/3≤t²−(2r/3)t+r²/3<2r²/3`,
car `t²−(2r/3)t−r²/3=(t−r)(t+r/3)<0`. Elle contient G,
donc `β(G)<2β(F)/3`. Ce centre auxiliaire n'intervient que dans la preuve.
L'échange dans Γ décrit ci-dessus conserve le propriétaire à la coupe
d'usage, pas nécessairement la feuille terminale ni la composante au
nouveau niveau β(G).

Si EOF arrive sans orthant saturé, tous les intérieurs ont été rapportés
et `p≤8(K−1)`. Cela **n'implique pas p<K**. Pour K≤p≤8(K−1), choisir
K de ces sites donne encore une descente stricte, sans contraction
uniforme. Pour p<K, conserver toute la branche locale existante,
coquille comprise : ce petit p ne borne pas |U|. À K2 sur0,1,L−1,L,
F={0,L} a deux intérieurs dans deux orthants ; le seul G={1,L−1}
donne `β(G)/β(F)=((L−2)/L)²`, arbitrairement proche de1.

Il existe même une chaîne de sauts de secours forcés à K3 et K5. Pour
m≥2, poser T=10m, R=3m²+1, r_i=R−i et c_i=(iT,0,0), 0≤i<m.
La couche F_i a les directions YZ(5,0),(−3,4),(−3,−4), et ajoute
(3,4),(3,−4) à K5, multipliées par r_i. Leur cercle de rayon5r_i
est leur MEB : les trois premières directions entourent son centre.
Avec V=T²+25, la puissance d'un site de couche i+h vaut
`|z_{i+h}−c_i|²−25r_i²=h(hV−50r_i)`.
On a V<50r_i<2V pour toutes les couches. Seule F_{i+1} est donc
strictement intérieure, p=K, et aucun orthant ne contient K sites.
Le saut sur I est forcé : m−1 étapes, quel que soit le sélecteur parmi I.
Après translation Y+3R,Z+4R, le cube a côté24m²+8 : famille à B variable,
pas asymptotique illimitée en u18. Deux petits contrôles privés Fraction
normal/−O recoupent aussi le témoin T1000/R30001 à16 couches K3/K5,
quinze étapes forcées ; pas de reçu natif autonome. Une mémo des couches
peut amortir ces étapes : ce n'est pas un carré global prouvé ni un
régime LiDAR testé. La couche i+2 est hors de B_i ; ne pas hériter au
résolveur sans ancre la localisation initiale du schéma K2 à ancre fixe.

Si les autres étapes restent descendantes, le nombre de sauts à orthant
saturé dans une résolution est O(log(β_initial/β_min)), même entrecoupés
de sauts de secours. Sur le cube entier B bits, sites distincts et K≥2,
`β_min≥1/4` et `β_initial≤3(2^B−1)²/4`, donc O(B) **de ces sauts**.
Ni tous les pas de résolution ni les visites d'index ne sont ainsi bornés.
EOF, tests ambigus et blocs rejetés restent payés ; en parallèle, compter
aussi les reports déjà engagés au-delà du préfixe logique Q.

Essai pertinent : comparer propriétaires aux coupes exactes, pas IDs des
feuilles ; publier visites, reports, sauts saturés/secours et coût total.
Ne pas transférer cette règle au résolveur K2 `keep0` à ancre fixe : prendre
K intérieurs peut perdre son ancre et la borne d'états mémoïsés déjà
prouvée pour ce schéma. Ne pas remplacer non plus le census I/U complet
du catalogue, la vraie densité k-NN des points ou la règle d'attache MMt.
Multiplicités et nouveaux domaines numériques exigent leur preuve propre.

## Construire les couvertures sans développer Γ

Le lemme de couverture déjà publié ferme l'obligation géométrique pour
les sites distincts, 2≤K≤n fixé. Les seeds nécessaires sont toutes les
incidences I∪U des boules de population≥K et p+q_min≤K, avec
`cover_extra=0` et le centre résolu vers sa composante **vivante au
niveau propre**. Ce n'est pas la liste des premières boules par point.
Les fusions p+q_min=K+1 restent dans FULL ; les retirer pour fabriquer
les seeds amputerait l'arbre.

Après quotient des plateaux, on peut garder seulement le plus petit
niveau pour chaque couple (point, propriétaire). Une seed d'ancêtre est
aussi redondante si une seed de descendant couvre déjà ce point avant
la naissance de cet ancêtre. Preuve : la couverture du descendant se
prolonge au parent dès sa naissance, puis à tous ses ancêtres ; elle
implique donc toute couverture apportée plus tard par cette seed. Cette
déduplication conserve la relation de couverture, **pas les nombres de
votes de K-parties ou de paires**. `witness_universe` du snapshot des
masses contrôle explicitement que `ball_node` est vivant ; sous cette
condition, la formule du lemme2 des masses est cohérente, sans oubli de
la mort d'une branche.

Une borne utile, sans hypothèse de régime : pour K≥2 non pondéré, un
support positif a q_min≥2, donc chaque boule forte a p≤K−2. En notant
M le nombre de ces boules et U_tot la somme des tailles de leurs
coquilles, le volume D des seeds vérifie `D≤(K−2)M+U_tot`. Si chaque
coquille est exactement son support minimal, population=K et `D=KM`.
Il faut publier séparément les coquilles étendues ; leur taille n'est
pas bornée par K. Les objets K1 et les multiplicités restent distincts.

Conséquence d'architecture : le catalogue stocke déjà I/U en CSR. Après
sélection des boules fortes par scan du catalogue, deux passes de
comptage/préfixes puis écriture permettent de construire les
incidences par point en O(n+M+D), sans requête géométrique supplémentaire,
une fois la table des propriétaires disponible. Son calcul reste payé.
Puis arbre virtuel et télescopage évitent le produit points×profondeur.
Cette borne est relative au catalogue matérialisé ; elle ne prouve ni
M sous-quadratique, ni les coquilles petites, ni le contrat LiDAR/G4.

La suppression des seeds ancestrales n'exige pas de remontée individuelle.
Après déduplication du couple, trier les propriétaires d'un point par
entrée DFS : un propriétaire possède une seed descendante si et seulement
si le suivant entre avant sa sortie DFS. On conserve ainsi une antichaîne
de propriétaires, avec leurs dates originales ; un nœud interne sans
descendant seed reste indispensable. Pour IDs/rangs à largeur fixe,
comptage et tri radix donnent un chemin O(n+M+D), après la sélection,
l'index Euler et la table des propriétaires. Un tri comparatif ajouterait
D log D. Scratch et préparation globale restent publiés, non gratuits.

Le [prototype structurel clos](../../receipts/audit_continu_20260929/seed_cover_antichain_20260930/README.md)
recoupe désormais cette réduction sur512 cas :68 699 coupes exactes
critiques et intermédiaires,16 839 incidences→6327 seeds. Les six exports
natifs sont copiés de captures closes, pas nouvellement générés ;88
incidences incluent des points à seeds uniquement internes K3/K5. Sont
aussi éprouvés7 828 propriétaires normalisés à leur mort,1549 nœuds de
plateaux contractés, une chaîne20 000 sans récursion et une forêt laissée
comme telle. Cinq mutations causales changent les couvertures ; remplacer
les rangs exacts par doubles viole séparément l'invariant de naissance.
Capture et lecteurs normal/−O autonomes, manifeste externe
`dfb3e67339d98558873aeb07524e9249f072e9451561857db4de29ba61b50cd4`.
Le prototype trie comparativement et normalise des owners bruts en
remontant : O(D log D) et possiblement O(DH), pas une implémentation
linéaire ni une nouvelle mesure native. La borne proposée suppose la
table de propriétaires déjà calculée ; son coût reste à fermer.

## Paires locales : borne globale utile, recherche encore ouverte

Pour n sites distincts,2≤K≤n, K incluant le site interrogé, poser
r_x=d_K(x), ρ=min_{x≠y}|xy|, D=max_{x,y}|xy| et Δ=D/ρ. Pour c≥1
fixé, le nombre total de paires **orientées** telles que |xy|≤c r_x
est O((K−1)c³ n(1+log Δ)). La constante est géométrique, pas un chrono.

Preuve recoupée : regrouper les ancres par r_x∈[R,2R), R=2^jρ.
Dans une cellule demi-ouverte de côté R/√3, toute distance entre deux
points est strictement<R. Elle contient donc au plus K−1 ancres de
ce groupe : K ancres forceraient leur K-ième distance sous R, même si
la coquille à R est exacte. Pour une cible y fixée, ses ancres incidentes
du groupe sont dans B(y,2cR), qui rencontre O(c³) cellules. Compter les
ancres entrantes par y puis sommer les groupes ferme la borne. Ce n'est
pas une borne par voisinage sortant : un seul point peut garder Θ(n)
voisins. Sur une grille B bits, Δ≤√3(2^B−1), donc le majorant est
O((K−1)c³n(B+1)), sans hypothèse LiDAR. B reste un paramètre, pas une
qualification du moteur large.

Pour les paires K2 à bande en rayon η, ℓ=|xy|/2 et α_x=d_2(x)/2 :
c=1+η. Pour une paire d'ordre K≥3 avec
ℓ=max(|xy|/2,d_K((x+y)/2)) et ℓ≤(1+η)α_x, la seule relation
α_x≤d_K(x) donne c=2(1+η). Le prototype souple MMp utilise une autre
normalisation A=min_y ℓ², non α² ; ses constantes ne se transfèrent
pas implicitement. Les K-parties et leurs classes restent distinctes :
le cap K3 de Q1 garde son nombre quadratique de classes pour un point.

Cette preuve ouvre un budget de **sortie de paires** raisonnable, pas
une preuve de travail total. Le `SiteGrid.candidates` actuel peut rendre
tous les sites d'une grosse cellule avant filtrage exact ; le résolveur
K2 matérialise également des listes d'intérieurs. Mesurer ces candidats
et les tests de census, puis utiliser un index adaptatif ou un parcours
conjoint avec refus certifié des blocs. Ne pas confondre la bonne taille
des votes retenus avec une recherche sous-quadratique déjà obtenue.

Le [paquet clos SiteGrid/packing](../../receipts/audit_continu_20260929/pair_grid_packing_20260930/README.md)
recoupe66 petits cas exacts et la classe réelle extraite AST du snapshot
frontière `86ba984f…`. À n exact8000/16000/32000, m=n−2 sites denses
appariés à distance1 plus deux extrêmes u18 donnent exactement3m votes
dirigés. Le pas global32768/16384/16384 met les m sites dans une case :
toute requête locale de vote renvoie m IDs ; au total m², soit
63 968 004/255 936 004/1 023 872 004. Trois requêtes de vote et trois
census de partenaires par taille recoupent le comportement réel ; les
totaux sont dérivés analytiquement sans exécuter le carré. Le census
des partenaires vides paierait aussi m−2 tests malgré une coquille de
deux sites. Sans les extrêmes, le même code a un pas4 et≤16 candidats
locaux : il ne faut pas une taille de case imposée par tout le volume.
Ce témoin n'est ni un scan LiDAR ni une nouvelle croissance FULL mesurée.
Manifeste externe `92f5d1d9333a96e7c97349da8313f06b9b73a641fbe78a2472b755eea6dd3514`,
lecteurs autonomes normal/−O avec SHA avant replay et intégrité après.

Le census garde un deuxième carré indépendant de cet index. Lire le
`PairResolverK2.third_sites` réel du snapshot `86ba984f…` : il construit
toute la liste, puis `resolve` choisit le site de distance minimale à
l'**ancre conservée**, départage par ID. Prendre a=0 et m sites
b_j=128000+j sur un axe,0≤j<m≤32000. À K2/η1/4, les m votes de a
sont dans sa bande. La boule diamétrale de(a,b_j) contient exactement
les j autres b_i, i<j. Chaque état initial distinct matérialise j
témoins, puis choisit b_0 ; mémoïser(a,b_0) ne retire pas cette première
collecte. Total m(m−1)/2 intérieurs, alors que tous les votes du nuage
sont O(m). Le
[reçu clos de collecte](../../receipts/audit_continu_20260929/pair_census_square_20260930/README.md)
rejoue les méthodes réelles avec un index **idéal** qui ne renvoie que
les vrais intérieurs, et une forêt analytique collinéaire, pas le natif.
Vingt configurations m2/3/5/8/16, deux ordres de requêtes et candidats,
recoupent368 propriétaires vivants au niveau propre contre L2 exact.
La projection orthogonale des points collinéaires sur leur axe conserve
L2 : les composantes sont celles des intervalles testés indépendamment.
L'argmin réel porte sur l'ancre cur[0], pas le milieu ; le mutant de
milieu change la sélection mais garde ici un propriétaire correct.
Le mutant mémo ignoré est tué par son travail répété, pas ses labels.

Un argmin en flux conserve le choix avec mémoireO(1), mais paie encore
les mêmes j visites. Grand m8000/16000/32000 et n=m+1 dans le reçu :
**formules uniquement**, aucune exécution quadratique ni mesure LiDAR.
Pour n exact8000/16000/32000, remplacer m par n−1 donne respectivement
31 988 001/127 976 001/511 952 001 tests de census, et autant d'argmin.
Deux lecteurs normal/−O et mutant du lecteur ; manifeste externe
`514362ac6b6047a1239b7bff8c583cfca4a12a83f7190cbf7c38996ece7c69cd`,
sept fichiers réguliers avec empreintes avant/après. Une recherche de
témoin certifiée **sans liste exhaustive** est nécessaire : préserver
l'argmin si ce témoin brut est requis, ou adopter le choix quelconque
justifié pour FULL. À K2 seulement, même un troisième site de coquille
distinct des deux bouts donne |a−z|<|a−b| et une coface contenue dans
l'ancienne boule ; cette variante garde donc le propriétaire utile.
Elle n'autorise pas K sites quelconques de coquille à K≥3. Une meilleure
grille seule ne suffit pas à retirer le carré du traitement actuel.

Résultat positif séparé : toute sous-paire de la descente keep0 reste
dans la bande de la même ancre, car pour un troisième site distinct z
dans la boule diamétrale, |a−z|²+|b−z|²≤|a−b|² et |a−z|<|a−b|.
Les distances diminuent strictement et tous les états sont mémoïsés
après retour. Le nombre d'états/census frais du chemin de votes est
donc au plus le nombre D de paires orientées de bande, auquel les300
autocontrôles arbitraires et300 keep1 sans mémo ajoutent O(600n).
Le carré caché est ici le **coût de chaque census**, pas nécessairement
le nombre de descentes. La borne ne paye pas encore les recherches
ni les remontées d'ancêtres de la forêt.

## Générateur de paires avec budget de recherche

Proposition exacte recoupée, pas code porté ni mesure. Sur la grille
entière[0,2^B−1]³, sites distincts et K incluant l'ancre, supposer les
distances `s[x]=d_K(x)²` **déjà disponibles**. Pour un facteur de rayon
c≥1, écrire c²=N/D exactement et prendre Q=ceil(4c), A=(2Q+1)³.
L'ancre x appartient au seul bucket `j=(bit_length(s[x])−1)//2`.
Sa cellule à cette échelle a les coordonnées `(2*x_i)>>j`, donc
côté R/2 avec R=2^j. Cela traite j0 sans arrondi flottant.

Grouper les **ancres** par(bucket,cellule). Dans chaque bucket non vide,
grouper aussi toutes les **cibles** par cellule de la même échelle.
Puis, pour chaque cellule d'ancres occupée, consulter ses A cellules
cibles voisines. Tester exactement chaque paire du produit, en excluant
x=y et en exigeant `D*|xy|²≤N*s[x]`. La cellule cible peut être dense ;
seule l'occupation de la cellule d'ancres est bornée par K−1.

Preuve de complétude : r_x∈[R,2R), donc |xy|≤c r_x<2cR=4c(R/2).
Chaque différence d'indice de cellule est≤Q. Chaque paire orientée
est rencontrée une seule fois. Pour une cible y et un bucket, A
cellules d'ancres contiennent au plus (K−1)A ancres : même les faux
candidats du stencil sont donc bornés par `C≤(K−1)AnJ`, J≤B+1.
La somme des cellules d'ancres occupées est≤n, donc les consultations
**cases vides comprises** sont≤An, pas AnJ. Les index de cibles
coûtent O(nJ). Le hash donne O(nJ+An+C+P) seulement **en espérance**,
P étant la sortie ; mémoire O(n+P) par bucket, hors sortie streamée.

Version déterministe : trier une fois Morton(2x), puis obtenir les
plages cibles d'échelle j par son préfixe `Morton(2x)>>3j`, déjà contigu.
Par bucket, produire les≤A*m_j requêtes de cellules voisines, les
trier radix puis joindre aux plages cibles. À B≤32, Morton(2x) a≤99
bits ; avec bucket,≤105 bits, soit≤14 passes radix8. Budget
O(pn+nJ+pAn+C+P), p≤14, J≤33 ; scratch O(n+A max_j m_j+P).
Ne pas découper les requêtes en lots arbitraires en rescannant n cibles
gratuitement à chaque lot. Une mémoire plus petite doit payer les
recherches binaires ou la fusion externe. Les distances carrées B32
ont besoin de66 bits ; u128 suffit, pas u64. Les produits avec N/D,
offsets et compteurs ont leur propre garde. Pour B variable, publier
aussi coût des clés, passes radix et arithmétique multiprécision.

Réserve pratique pour100ms : à η1/4, c=5/4 en K2 donne A=1331,
c=5/2 pour la bande α à K≥3 donne A=9261. À n40k, ce second stencil
autorise370,44 millions de consultations ; K5/J22 borne C par32,60
milliards. Ce sont des plafonds pessimistes, **pas des chronos**.
Réduire par distance de boîtes et rayon maximal des ancres est sûr ;
mesurer les vraies cellules, C/P, requêtes vides et volumes radix avant
de choisir GPU. Le calcul d_K, les qNN aux milieux, census et owners
restent hors de cette borne. MMp normalisé par minℓ, avec α≤minℓ≤2α,
exige c²=16(1+η′), pas le facteur K2 transféré aveuglément.

## Dettes ciblées du prototype de sélection

La relecture statique du vrai `selection.py` confirme que l'EOM actuel
n'est pas linéaire : `depth_of` remonte les parents pour chaque cluster.
Un peigne de h fusions et h+1 feuilles, chaque feuille de masse mcs,
conserve les2h+1 clusters à la condensation ; la somme des profondeurs
vaut h(h+1). Les parcours `unselect` et `ancestor_selected` peuvent
ajouter le même défaut. Une passe postordre pour les scores puis une
passe descendante pour les décisions suffit conceptuellement ; ne pas
porter ces remontées individuelles dans la tête industrielle. C'est
une preuve de coût du prototype, pas une mesure native ni un défaut
de la récurrence mathématique EOM.

Réserve de diagnostic séparée : `dev_scenes.selection_block` indexe
la liste des marges non-None avec `len(lab)//2`. Avec r marges pour n
points,1≤r≤floor(n/2) provoque un IndexError ; sinon ce n'est pas
la médiane de cette liste. Le test `any()` ne traite que r=0. Utiliser
la taille des marges retenues et préciser la convention de médiane.
Lecture seule, aucun score historique corrigé ni nouveau banc lancé.

## Ce que les résultats statistiques autorisent

La proportion d'erreurs, le rappel des points avant fusion et la
consistance de l'arbre sont trois propriétés différentes. L'admissibilité
géométrique seule ne donne pas le rappel : une règle qui attend toujours
la racine est admissible et laminaire, mais ne récupère aucun amas avant
fusion. L'argument du mémo statistique par lignes de flot borne les
erreurs de bassin sous ses hypothèses de densité et d'estimation. C'est
une borne inconditionnelle ; la précision parmi les seuls points affectés
demande aussi un plancher de masse affectée. Cet argument ne
justifie pas à lui seul la formule « toutes les règles admissibles sont
consistantes ». Son régime K→∞, K/log n→∞ ne couvre pas le contrat K5/K10
fixé. La section K fixé annonce d'ailleurs une limite locale conjecturale,
non un objet déterministe. Les résultats atomiques à K/n fixé ne sont
pas transférables sans conditions aux scans LiDAR. Les 88 nuages finis
réfutent l'estimation de masse du catalogue sur ces fixtures ; conclure
qu'aucune limite n'est une fonctionnelle de μ demande encore une suite
asymptotique ou une preuve, pas seulement leur dispersion. Les scènes
gaussiennes comparent des blocs à des coupes avant fusion, non une sortie
EOM choisie ni HDBSCAN ; MMt n'y est pas évalué. L'unité « un site »
désigne ici les sites distincts, pas les retours pondérés après fusions
de grille : ne pas étendre cette qualification aux multiplicités.

L'intuition « Γ suit la boule la plus lourde » peut en revanche recevoir
une preuve combinatoire précise. À une coupe s, avec rayon d'admission
t=min(s,R_x), soit M_C le maximum de population d'une boule de rayon≤t,
contenant x, dont le centre appartient à C. Le nombre N_C de K-parties
admises contenant x et portées par C vérifie, pour M_C≥K,
`C(M_C−1,K−1)≤N_C≤[Σ_{q=1..4}C(n,q)]C(M_C−1,K−1)`.
À gauche, chaque K-partie choisie dans la boule a son centre relié à
celui-ci dans leur intersection convexe à la coupe s. À droite, chaque
MEB a un support minimal de taille≤4 et une population≤M_C ; regrouper
les parties par MEB ne donne qu'un facteur polynomial. Pour K/n→p>0,
M_C/n→m_C≥p et un écart strict des m_C, la croissance exponentielle
`m_C h(p/m_C)` impose le gagnant parmi les votes **actuellement admis**,
où h(u)=−u log u−(1−u)log(1−u) est l'entropie binaire. L'exposant
croît strictement avec m_C quand m_C>p ; le facteur polynomial est
négligeable à cette échelle.
Ce n'est ni une preuve à K fixé, ni une garantie de majorité par rapport
au dénominateur qui contient les votes futurs, ni un résultat aux égalités.

## Le bord de bande dure reste discontinu même avec les paires

Le [nouveau contrôle exact](../../receipts/audit_continu_20260929/hard_band_border_20260930/README.md)
traite cinq sites collinéaires : x=0, C1=100, C2=110, D1=−110,
D2=−125+e. K2, η=1/4 en rayon, bande fermée, votes uniformes et
majorité strictement supérieure à W/2. La première couverture de x est
50, indépendante de e. Pour e<0 petit, trois votes sont retenus ; deux
rejoignent la branche positive à55, donc x y entre à55. Pour e≥0 petit,
le quatrième vote est retenu ; les deux côtés portent au plus deux votes
avant leur fusion105. x attend105. C1 est déjà attaché à C2 dès5 : la
**hauteur de réunion de x et C1**, pas seulement une date, saute de55
à105 alors que le déplacement tend vers zéro. Les deux composantes
principales restent séparées avant105. À55 le vote x/D1 est encore
distinct de la branche D1/D2 ; sa réunion antérieure à la fusion globale
ne change pas le calcul de majorité.

La fonction réelle `majorite_virtuelle` est extraite sans modification
du snapshot `a1ff44de…`, puis recoupée contre un Γ2 Fraction exhaustif
et les composantes de l'union d'intervalles L2. Onze valeurs de e, onze
contrôles η1/8 sans saut et deux versions entières u18 ; 1274 comparaisons,
deux mutants de règle causaux rejetés. À l'échelle1024 et translation
commune128001, déplacer le seul dernier site de0 à2 fait passer cette
réunion de56320 à107520. Tous les sites restent≤240641. Le passage à
la limite concerne la géométrie, pas des déplacements sous le pas d'une
grille fixe. Aucun défaut du résolveur natif n'en est déduit : il n'est
pas appelé ; `PairesK2.lignes` n'est pas exécuté non plus.

Capture R2 autonome, pins partagés avant/après, codes0 normal/−O et
lecteur causal hash-first avec SHA de manifeste fourni explicitement,
inventaire fermé et hashes inchangés après lecture. Le premier paquet
privé reste préparatoire et intact ; ses deux erreurs de garde de
mutation sont documentées. Aucun moteur, EOM, MMt ou GCP dans ce lot.

Décision proposée au développeur : garder la majorité de paires à bande
dure comme **contrôle statistique**, pas comme tête globalement continue.
Les poids souples et une date à marge continue traitent un autre contrat.
Ils peuvent éviter ce seuil, mais ils doivent encore garder la cible
statistique voulue ; la seule réussite sur les deux triangles ne suffit
pas à décider. Ce résultat complète la réponse Q3, sans réactiver le
saut de disparition des boules du catalogue sur des K-parties fixes.

## Réponses aux trois questions sur les votes de bande

Réponse à [Q1, Q2 et Q3 du développeur](../REPONSE_CLAUDE_CONTACT_COMPTAGE_ET_JUGES_20260930.md#7-questions),
30 septembre, après lecture du mémo privé `revision_cible/majorites_continues`.
La conclusion pratique est de distinguer deux modèles : votes de K-parties
avec comptage implicite, ou temps de couverture des branches de FULL.
Le second change la sémantique des masses ; ce n'est pas une compression
du premier. Aucun port moteur ou essai G4 n'est fait par cette réponse.

### Q1 Région locale et nombre de classes

Oui pour la borne spatiale, non pour une restriction aux K voisins.
Si F contient x et sa boule minimale a un rayon ≤R, chaque site de F
est dans la boule fermée B(x,2R), par l'inégalité triangulaire. Tout
intérieur de cette boule minimale aussi. Avec m sites dans B(x,2R),
p≤m−1 ; une classe a un support minimal de taille au plus quatre en 3D,
d'où au plus Σ_{j=1..4} C(m,j) classes, et au plus C(m−1,K−1) votes de x.
Ces bornes portent sur l'occupation m, pas sur K. Une vraie borne de
densité/occupation locale aiderait ; le seul rayon du K-ième voisin
n'en fournit pas. Il faut mesurer m dans les régimes LiDAR visés.

Une [famille K3 avec contrôle Fraction](../../receipts/audit_continu_20260929/band_classes_locality_20260930/README.md)
explique pourquoi il ne faut pas matérialiser
toutes les classes comme garantie générale sous-quadratique. Prendre
x=0 et m points du cercle unité u(t)=((1−t²)/(1+t²),2t/(1+t²),0),
avec des t distincts dans ]0,1/4[. Chaque triangle {x,u_i,u_j} est aigu,
et son rayon carré vaut
`(1+t_i²)(1+t_j²)/(4(1+t_i t_j)²)`.
Il est entre 1/4 et 17/64, donc strictement sous
`(9/8) α_3(x)²`, puisque α_3(x)²≥1/4. Toutes ces classes sont dans
la bande de niveaux η′=1/8. Leurs centres sont distincts : pour un
centre donné, les sommets unitaires de sa coquille vérifient
`c·u=1/2`, une droite coupant le cercle en au plus deux points.
Il y a donc C(m,2) classes. Ce n'est ni une mesure SemanticKITTI,
ni une impossibilité de comptage implicite, ni un défaut du FULL actuel.

### Q2 Un univers exact au-delà de K2

L'univers étiqueté exact naturel reste celui des K-parties contenant x.
Chaque rayon minimal, et leur minimum α_x, est 1-lipschitzien sous
déplacement apparié ≤ε. Mais une liste des seuls K voisins de x ne
détermine pas cet univers de bande. À K3, les sites 0, 1/2 et 1 sur un
axe donnent α_x=1/2. Ajouter des sites de norme 101/100 dans le petit
cap précédent ne change ni ces trois voisins ni α_x ; leurs triangles
avec x restent dans la bande 1/8. Placer les mêmes IDs très loin ne
change toujours pas les trois voisins, mais retire ces votes de bande.

Une requête de rayon B(x,2R), ou des requêtes KNN adaptatives jusqu'à
ce rayon, retrouve les sites nécessaires ; son résultat n'est pas
borné par K. La continuité de ℓ_K ne rend pas continus les IDs choisis
au rang K : une substitution change l'univers si elle n'est pas gérée.
Je ne connais pas encore de réduction à un nombre de requêtes KNN
borné par K qui conserve **exactement** les masses des K-parties.
Cela n'exclut pas un comptage par blocs, une CDF implicite ou des
requêtes multi-centres exploitant tout le nuage.

**Complément P0, totalité des paires K3/K5.** La bande des K-parties ne
se transfère pas telle quelle aux paires d'ordre K. Le
[nouveau reçu](../../receipts/audit_continu_20260929/pair_band_totality_20260930/README.md)
donne deux familles minimales exactes :

- FX-A9 :0,2,7,10,13, K3, x=10. Première couverture α=3 par{7,10,13} ;
  les quatre ℓ vers0/2/7/13 valent respectivement5,4,9/2,9/2.
  Bord1,25α=15/4<4 : aucun vote. À η1/3, la paire vers2 entre sur la
  coquille ; η1/2 ajoute7 et13. Dans le MMp réel, A=min ℓ²=16, pas α²=9.
- K5 : x=0 et quatre sommets(1,1,1),(1,−1,−1),(−1,1,−1),(−1,−1,1).
  Pour tout centre c, leur distance carrée moyenne vaut3+|c|² ; donc
  MEB unique centrée en0, α²=3. Au milieu d'une paire x/y, trois
  distances carrées valent19/4, d'où les quatre ℓ²=19/4. Bordη1/4 :
  75/16<76/16, aucun vote. Translation+(1,1,1) : cinq sites u18 à
  coordonnées0/1/2, même résultat. Contrôles K2 non vides et seuil
  carré19/12 inclus exacts.

Les quatre votes de chaque cas sont exhaustifs Fraction ; les fonctions
réelles `votes_paires/poids_bande` sont rejouées sur un contexte MEB
analytique, avec IDs factices : niveaux/échelles/poids seulement, pas
propriétaires FULL. Trois cas vides,20 bornes α²≤A≤4α², deux mutations
causales et lecteur sémantique normal/−O. Manifeste externe
`b19df04fa430d84848d21d5627d020cd21906b7e5d96abb7eef5d126a5f2b21d`.
Ce n'est pas un bug du K2 déjà implémenté ni du MMp actuel : celui-ci
normalise par min ℓ et évite ce zéro, en changeant de modèle. Définir
explicitement l'échelle et le fallback avant un port K5. Le jouet
`mmc.votes_paires` trie n distances pour chacun des n milieux de chacun
des n sites : O(n³ log n), avant même Γ exhaustif ; ne pas copier ce
chemin comme implémentation locale. Le diagnostic FX-A9 mélange aussi
unité de vote, normalisation et règle d'admission : isoler ces facteurs
dans les comparaisons statistiques.

Le [contre-exemple CDF clos](../../receipts/audit_continu_20260929/component_ballot_cdf_20260930/README.md)
précise une information à conserver : à couverture et composante
identiques, la masse de x passe de 2 à 3 à K2, et de 5 à 15 à K5,
**à l'intérieur** de la bande fixe 1/8. De nouvelles parties rejoignent
une composante ancienne sans nouvelle fusion positive. Le raccourci
`C(|Cov(C)|−1,K−1)` est donc faux, même avec un FULL correct. Les
rayons d'admission ou une requête géométrique équivalente restent nécessaires.

Une identité exacte peut guider un oracle de CDF. Pour t=min(r,R_x),
soient d_t(c) le nombre de sites dans B(c,t), et λ(c) la composante Γ_K(r)
qui contient les K-parties de ces sites lorsque d_t(c)≥K. Toutes sont
dans une même composante par les cofaces à K+1 points contenues dans
la boule. Le nombre de votes de x dans C est l'intégrale d'Euler de
`1_{c∈B(x,t)} 1_{λ(c)=C} C(d_t(c)−1,K−1)`.
Chaque vote contribue un ensemble de centres convexe compact non vide,
donc compte exactement une fois, contacts compris. Cette déduction HGP
utilise l'additivité de l'intégrale entière décrite par
[Baryshnikov et Ghrist](https://pmc.ncbi.nlm.nih.gov/articles/PMC2906884/).
Les quatre contrôles 1D du reçu la recoupent par points moins intervalles
ouverts. Un arrangement global de centres serait potentiellement coûteux :
cette identité n'est pas encore une architecture industrielle ni une
preuve sous-quadratique.

### Q3 Poids continus et portée de la preuve

Oui, le poids `max(0,1+(1−r_F²/α_x²)/η′)` échappe au mécanisme précis
de notre contact **sur un univers de K-parties identifiées**. Le vote F
ne disparaît pas lorsqu'un autre site entre dans sa boule ; r_F et α_x
restent continus. Sur les boules fortes du catalogue, le même poids ne
répare pas la disparition du vote. Le rejeu du développeur le confirme.

Dans cet univers de K-parties, pour K≥2, sites distincts et n≥K,
α_x>0 et W≥1 : un vote qui minimise
r_F a poids 1. Le dénominateur ne peut donc pas s'annuler. K1/α=0
demande un cas séparé. Les paires d'ordre K≥3 ne minimisent pas
nécessairement à α : ne pas leur transférer ce plancher sans changer
explicitement la normalisation en `min ℓ`. Avec λ=√(1+η′), le poids tronqué est
2λ/η′-lipschitzien en r_F/α. On peut écrire directement la borne finie
`Δ≤N_union·[2λ(1+λ)/η′]·dε/α_min`, sans terme asymptotique,
où N_union compte les votes positifs dans au moins un des deux nuages,
d=1 pour les K-parties et d=2 pour les paires d'ordre K.

La [contre-relecture S/N close](../../receipts/audit_continu_20260929/majority_SN_counterreview_20260930/README.md)
du transfert des propriétaires du théorème S est
cohérente sous ses applications d'entrelacement compatibles. Le retour
de la majorité de Y porte dans X plus de `W_X/2−3Δ/2` ; tant qu'il
reste séparé de la majorité A, celle-ci porte au plus `W_X/2+3Δ/2`.
La date à cône borne alors leur fusion, après le décalage 2dε. C'est
une contre-relecture de preuve, pas une nouvelle campagne native ni une
garantie EOM/ARI ou une stabilité aux retraits de points.

La proposition N demande toutefois de corriger sa géométrie avant de
publier sa constante exacte : des directions distinctes proches ne
donnent pas exactement f=1+ρ. Pour u±=(99/101,±20/101), les deux votes
de droite fusionnent à `ρ·101/99>ρ`, et la fusion avec le côté gauche
est `√(1+ρ²+2ρ·99/101)<1+ρ`. L'appartenance annoncée à
`D(γ,min(2κγ,1))` ne vaut donc pas telle quelle au bord g=1.
Le contrôle exact à quatre sites prend ρ=6/5, κ=25, γ=1/50 :
la majorité droite vaut 28/53>1/2+γ, mais la date MM est 202/165,
strictement après `√(12101/2525)−1`, délai demandé. Six paires et
quatre cofaces Γ2 Fraction recoupées normal/−O ; aucun appel natif.
Une perte strictement inférieure à 1 et une erreur angulaire contrôlée
peuvent réparer l'argument de croissance ; ne pas confondre cette piste
avec la constante exacte déjà démontrée. Relecture23 h20 : le mémo privé
`MEMO.md`, SHA `7ec56b4d07dc7728756662471efc9997cca3337e45db318b9132e5edcad03f02`,
conserve encore le passage sans perte au bord g=1. Ce correctif de preuve
déjà publié reste à intégrer ; il ne réfute pas le théorème S ni ne
supprime la croissance limite avec N. Le résultat est limité à D(γ,g),
pas à toute règle qui manipule des masses.

### MMt et le prochain port

La nouvelle MMt peut éviter l'univers combinatoire : elle compte le
temps de couverture des branches, pas les K-parties. C'est un changement
de modèle de masses, pas une compression des votes précédents. Le mémo
privé relu est épinglé à `7ec56b4d…`, `mmt.py` à `93f6acd0…` ; aucun
port natif ni nouveau chrono G4 n'est fait dans cette réponse.
Le domaine étudié ici est K≥2, n≥K et A>0 ; K1/A=0 demande un cas séparé.

#### Réduction exacte à une seule lignée

Le code oracle construit l'union des chemins d'ancêtres, puis rescane les
atomes et remonte leurs ancêtres à chaque événement. Avec D atomes,
profondeur H et E événements, une majoration simple de ce chemin est
O(DH + E log E + EDH), hors arithmétique. Ne pas appeler cela un balayage
linéaire industriel.

La [preuve médiane et ses cinq contrôles exacts](../../receipts/audit_continu_20260929/mmt_median_transfer_20260930/README.md)
évitent ce parcours. Garder les seuls atomes `(v,c,e)` de poids final
w=e−c>0, e=min(mort(v),(1+η)A). Dans l'ordre DFS global, choisir une
médiane pondérée m avec ces **poids finaux**, pas avec les masses courantes.
Toute composante de masse courante >W/2 possède un sous-arbre de poids
final >W/2 ; son intervalle DFS contient donc m. Toute majorité stricte
est sur la lignée de m. Avant la majorité, sa masse n'est pas nécessairement
le maximum global ; après, elle est exactement G.

Pour chaque atome, poser h=naissance(LCA(v,m)) et a=max(c,h). Sa contribution
à cette lignée est `1_{s≥h} max(0,min(s,e)−c)` : saut min(a,e)−c à a,
pente +1 à a puis −1 à e si a<e ; sinon un seul saut de tout w à a.
Au plus 2D positions d'événement, regroupées par **rang exact** avant décision.
Les rampes de la lignée occupent des vies disjointes : pente totale 0 ou 1.
Une pente >1 signale une couverture/compression comptée plusieurs fois.

Après index DFS/LCA global et extraction des vrais atomes :
O(D log D + D·coût_LCA), mémoire O(D), puis balayage linéaire et recherche
d'ancêtre finale sur FULL original. Il s'agit d'un nombre d'opérations
arithmétiques ; leur coût dépend aussi de la largeur des rationnels.
Les cinq arbres abstraits Fraction/AST
recoupent masses, W, T_half, T1, date et propriétaire, normal/−O.
Le plateau exactement à moitié attend sa fin : `inf{G>W/2}` n'est pas
le premier niveau G≥W/2. Le cas critique atteint bien t=49/40 à s*=25/16.
Ni ces fixtures ni la preuve ne bornent ΣD sur les trames LiDAR.

**Préparation sans remontées cachées, éprouvée abstraitement.** Partir des
incidences natives couvrantes complètes `(ball_node,activation)` de I∪U,
avec propriétaire vivant à l'activation ; dédupliquer en S propriétaires
et activation minimale. Un arbre virtuel des
nœuds seeds et de leurs LCA consécutives en DFS a au plus 2S−1 nœuds,
avec éventuellement la racine originale ajoutée. Sur une continuation
sans nouvelle activation ni réunion de lignées couvrantes, les durées
des vies successives se télescopent : conserver un intervalle, pas tous
les ancêtres. Garder les activations et les réunions couvertes ; retrouver
le propriétaire vivant dans FULL original à la date exacte. Cette
compression est justifiée sous couverture héréditaire complète. Le
[nouveau contrôle séparé](../../receipts/audit_continu_20260929/mmt_cover_compression_20260930/README.md)
la recoupe désormais sur 108 petits arbres rationnels : 15 360 comparaisons
par composante **originale**, W, quotient de plateaux, lignée médiane et
majorité ; quatre mutations causales, normal/−O. Activations tardives,
seeds internes/redondants, fusions multiples et chaîne de 31 nœuds à deux
nœuds virtuels sont exercés. Ce lot ne calcule pas les dates QS complètes
et ne reprend pas la qualification des cinq autres toys de dates.

Le prototype partage un index binaire global : préparation/mémoire
O(M log M), puis compression O(I+S log S+S log M), si les I seeds d'entrée
ont déjà un propriétaire vivant. Sinon, payer aussi leur normalisation par
requêtes d'ancêtre exactes, pas une remontée entière par incidence. Aucun
parcours de chemin original par seed dans ce compresseur. L'oracle, lui,
développe les chemins uniquement pour ces petits contrôles. Mesurer
l'extraction, ΣI/ΣS et la mémoire de l'index partagé ; aucune preuve de
complétude des incidences du catalogue, borne globale sous-quadratique
ou performance GPU n'en découle.

#### Précision nécessaire, y compris sur u18

Corriger le §8 numérique : les niveaux ne sont **pas** tous des entiers
sur 4. Les paires le sont, mais (0,0,0),(8,4,0),(4,8,0) a β=200/9.
Le [contrôle géométrique q4](../../receipts/audit_continu_20260929/mmt_rational_mass_20260930/README.md)
et son [annexe K5](../../receipts/audit_continu_20260929/mmt_rational_mass_k5_20260930/README.md)
vont plus loin : quatre sommets u18 à poids barycentriques strictement
positifs, puis un cinquième site intérieur, imposent la même MEB exacte.
n=K=5 donne une branche FULL_5 unique ; à η=2/3, sa vraie masse W a un
numérateur réduit de **142 bits**, dénominateur de 108 bits. Ce n'est pas
une somme artificielle ni une exécution native ; Fraction normal/−O.
i128 signé ne suffit donc déjà pas pour ce cas. Les largeurs du moteur
Level ne doivent pas être remplacées par la largeur supposée de q2.

Proposition numérique : garder les niveaux/rangs exacts et les masses
comme formes linéaires partagées ; intervalles certifiés pour les cas
séparés, repli rationnel/exact quand ils ne tranchent pas. Une égalité
exacte ne peut pas être résolue par l'intervalle seul. Mesurer les replis
et les largeurs réelles des sommes, sans présumer qu'un type fixe suffit.

Deux candidats `√e−c√A` se comparent par un signe pouvant contenir **trois**
racines, pas deux. Le critique simplifie en revanche : si G(s)=s+q sur
le segment admissible et s*=W²/(16κ²A), son terme est
`√A·[κ(1−2q/W)+W/(8κA)]`. Une seule racine, coefficient rationnel ;
cela ne résout pas toutes les autres comparaisons ou les égalités.

#### Transfert S_t et constante finie

La [contre-relecture close](../../receipts/audit_continu_20260929/mmt_median_transfer_20260930/README.md)
répare l'écriture sans changer le lemme T : M=Σmax(κ_f−1,0), déjà conforme
au code ; intégrer sur l'intersection non vide des deux bandes translatées,
avec nombre d'images distinctes non négatif avant le changement de jacobien.
Pour les propriétaires, la masse à T_half peut être exactement W/2 ;
prendre la majorité juste à droite et la limite gauche au saut de fusion.
Ces précisions rendent le transfert cohérent ; elles ne constituent pas
une nouvelle campagne géométrique des 22 252 contrôles privés.

Une correction réelle reste requise dans la portée finie. Avec
C=2Mbar+(λ+1)nbar, la substitution sûre donne
`B_t≤(1+κ)ε+(4κλ/η)·α_max(α_X+α_Y)/α_min²·Cε`.
Le coefficient 8κλ/η sans rapport d'échelles n'est que la limite locale
au premier ordre, pas cette majoration finie. X={−1,1}, Y={−2,2}, ε=1,
η=3, κ=λ=2 : B_t exact vaut 99, simplifié annoncé 35. L'écart réel
des dates n'est que √(5/2) : **aucun échec de stabilité démontré**.

**Ne pas supposer n_max borné par K/dimension.** Le §9.7 propose un packing
de branches K2. La famille rationnelle du [cap Q1](../../receipts/audit_continu_20260929/band_classes_locality_20260930/README.md)
en donne un corollaire analytique contraire : à β=1/4, les m paires
{0,u_i} sont m sommets Γ2 isolés, tous couvrants pour 0. Toute coface
qui pourrait les joindre contient un triangle {0,u_i,u_j}, strictement
aigu et de niveau >1/4. Pour m fini, ces m branches persistent sur un
intervalle positif avant la première coface. Le nombre peut être
arbitrairement grand en géométrie réelle générale, à K2 fixé en 3D.
Leurs durées peuvent se raccourcir quand m croît : aucune réfutation
de stabilité ou obstruction LiDAR n'est déduite. Mesurer ce compteur
dans les régimes visés, sans prendre le packing pour une borne acquise.

Priorité pratique : conserver les rangs exacts, corriger la condensation
par cohortes, puis un prototype borné de ce noyau MMt. Publier extraction,
ΣS/ΣD, événements, égalités et coût des replis sur les fixtures K3/K5,
puis les tailles 8k/16k/32k et trames déclarées. Le prototype ne vaut pas
une qualification statistique ni une promesse du contrat 100 ms.

## Contre audit bancaire et intégrité des fichiers

Source privée courante `merge_sessions.py`, SHA `059cc7ea…`, clone 2d0a0c41c :
la nouvelle garde `same_directory(out,session)` fonctionne dans son périmètre,
mais ne protège pas un **fichier** de sortie partageant une entrée par lien
symbolique ou lien physique. Le [paquet causal](../../receipts/audit_continu_20260929/merge_output_file_alias_20260930/README.md)
conserve deux sessions complémentaires d'un plan valide, deux scènes/deux
méthodes, et sept cas normal/−O. Dossiers distincts : cinq alias de fichiers
laissent la fusion rendre code0 et modifient les sources. Deux aliases CSV,
deux JSON, puis alias croisé `out/results.csv→session/run.json` : dans ce
dernier cas, le JSON source devient littéralement un CSV. La sortie fusionnée
reste conforme et passe `decide --check-only` ; ce n'est pas un faux score
statistique, mais une perte d'intégrité des entrées.

Contrôles positifs : destinations distinctes, sources intactes, code0 ;
dossier de session identique, refus code2 et sources intactes. Les liens
existent avant l'appel, sans course ni acteur concurrent. Sources complètes
épinglées avant/après, fichiers avant/après base64 et SHA conservés, lecteurs
hash-first et replays normal/−O. Aucun moteur, NumPy, GCP ou fichier partagé
touché. Le contre-exemple n'annule pas la correction des dossiers identiques.

Correction nécessaire avant ouverture destructive : les trois destinations
`results.csv`, `run.json`, `done.u32le` contre **tous** les fichiers d'entrée
(préenregistrement et run/results de chaque session), y compris aliases croisés,
puis destinations entre elles. Résolution de liens et identité d'inode,
sentinelles intactes avant refus. Des temporaires frais suivis de remplacement
évitent aussi de tronquer un inode partagé ; définir la politique de sorties
et tester ces cas, sans prétendre à une transaction globale non implémentée.

**Comparateur différentiel incomplet.** Le [paquet minimal et son lecteur](../../receipts/audit_continu_20260929/scale_comparator_vacuity_20260930/README.md)
conservent `compare_scale.py`, SHA `6b536427…`. Ce script compare
`zip(old,new)` sans égalité des nombres de lignes. Une sortie vide
et un journal d'appels vide passent par `all([])` ; une sortie tronquée ou
avec une ligne supplémentaire passe aussi. Huit petites commandes sur CSV/JSON
fabriqués : deux témoins conformes, six acceptations invalides, normal/−O ;
aucun appel moteur. Contrôler la totalité des inventaires/effectifs et un
plancher non nul, puis la correspondance unique des clés/commandes ; ne pas
prendre code0 seul pour une égalité. Les différentiels privés observés sont
non vides et leurs colonnes ont été relues : ce défaut du comparateur ne
démontre pas que leurs valeurs soient fausses. L'annexe finale ferme tous
ses fichiers par manifest épinglé explicitement, puis rejoue les huit appels
sans écrire dans l'archive. Les trois refus de garde sont archivés séparément,
pas nouvellement rejoués par son lecteur. L'ancien collecteur `strictread.py
--replay` reste une source historique, **pas** le point d'entrée à utiliser.
La première copie de publication avait normalisé les CSV CRLF : refus de hash
avant tout rejeu, puis octets rétablis et lecteurs normal/−O validés. Aucun
échec géométrique n'est déduit de cet incident de copie.

`redecide_refusion.py`, SHA `0d4d3a9…`, est un collecteur : son `return 0`
est inconditionnel, sans juger les codes des sous-appels ou flags d'égalité.
Conclusion de **lecture de code seulement**, pas une nouvelle exécution :
lire les codes/valeurs archivés, ou séparer collecteur et juge causal.

**État de campagne :** le journal global des 95 mutations des bancs termine
code1, 90/92 tués, trois équivalents. Deux survivants : P6 non-fini, désormais
reclassé équivalent parce que tous ses usages imposent déjà des bornes finies ;
Z3 comparaison brute des chemins, désormais tué par les sorties inexistantes
ajoutées. Les nouvelles sous-campagnes scale Python3 et Python3.10 terminent
chacune code0, 21/21 tués et deux équivalents ; ce ne sont pas des replays
clos de toute la campagne de 95. La campagne SiteTree précédente 94/94
reste distincte. Ne pas fusionner des campagnes de sources/gates différentes
ou confondre index ancien et fichiers courants encore `MM`/`AM`.

## Nouveaux constats prioritaires du 30 septembre

Avant les questions historiques ci-dessous :

1. **Condensation, correction nécessaire.** Les [preuves closes](../../receipts/audit_continu_20260929/point_condensation_20260930/README.md)
   reproduisent un défaut du vrai `head.cpp` : les sorties de points
   directement attachés ne déclenchent pas le seuil `min_cluster_size`.
   Une API valide à 21 points, mcs5, racine exclue donne A/B/C au lieu de
   R/C pour EOMz1. HDBSCAN réel et des oracles exacts recoupent le résultat.
   Corriger les cohortes de rang exact et la masse active, puis intégrer
   la porte ; ne pas supprimer les continuations ni changer FULL.
   Réalisation 3D de ces arbres précis et impact sur A/C restent à rejouer.
   Le [témoin géométrique séparé](../../receipts/audit_continu_20260929/point_condensation_cover_r2_20260930/README.md)
   confirme toutefois un score erroné à mcs6 sur le vrai cover de six sites
   K3, export historique recoupé contre Γ3 ; ni flip EOM ni nouvel appel
   générateur. Ajouter aussi ce cas, sans présenter les deux preuves comme
   une nouvelle campagne FULL ou statistique.
2. **q3/q4, essai borné pertinent.** Le [certificat quantitatif de groupe](../../receipts/audit_continu_20260929/group_moments_20260930/README.md)
   fournit plusieurs témoins intérieurs sans témoins individuellement
   universels. Tester un nombre borné de groupes préparés par feuille,
   pas un choix par tuple. Masques d'éligibilité q3/q4 seulement, census
   complet conservé. Publier aussi sélection/préparation et coût résiduel ;
   ni gain LiDAR ni passage sous-quadratique encore démontrés.
   Le [complément boîtes](../../receipts/audit_continu_20260929/group_moments_box_r2_20260930/README.md)
   donne un vrai témoin cubique, mais aucune petite boîte atteinte par
   défaut. Si a∈S fermé, σ≥0 : sauter cette ancre. Relever d'abord les
   vraies listes S/candidats et la fraction d'ancres hors S, puis mesurer
   préparation, rejets nouveaux et aval. Aucune hypothèse d'aspect≤2
   ne survit au recadrage par l'enveloppe de la liste.
3. **Frontière, même réduction sans tri par point.** Le
   [contre-audit exact](../../receipts/audit_continu_20260929/antichain_counterreview_20260930/README.md)
   donne un changement réel au η par défaut : quatre points K2,
   hauteur 0/3 de 25 à 200/9, sans modifier FULL. Aucun gain statistique
   revendiqué. Pour calculer le LCA des témoins minimaux : scanner les
   sélectionnés, garder `argmin(tout,-tin)` et `argmax(tin)`, puis leur
   LCA. Plus de tri ni stockage de l'antichaîne ; au plus une requête
   LCA par point, hors préparation de l'index. Les réductions se combinent
   par tâches, avec comparaison unsigned sûre et rangs/plateaux exacts.
   Préserver les premières couvertures et les incidences internes K3/K5 ;
   racines différentes refusées. Corriger la condensation avant EOM.
4. **Massif, un index global à protéger séparément.** Les décalages de
   `ext_reps` sont communs aux K ; `rep_first=u32(ext_reps.size())`, puis
   `rep_first+r` en u32, ne sont pas protégés par les refus `sr[k]` par
   ordre ni par `atlas.cells`. Modèle cardinal abstrait : 20 M jonctions
   par ordre K8/K9/K10 avec 58/74/92 représentants donnent respectivement
   1,16/1,48/1,84 milliards, chacun représentable, mais 4,48 milliards
   dans l'arène globale. Les compteurs de morceaux restent sûrs ; même
   dix cellules par boule resteraient sous la garde de l'atlas. Aucun
   nuage 3D réalisant ces chiffres n'est attesté. Élargir décalage **et**
   addition, ou contrôler la dernière adresse consommable avant insertion
   et cast. Le budget RAM/disque reste une garde distincte. Ne pas rouvrir
   la preuve forêt/CSR amont sur ce seul contre-modèle d'adressage.
5. **Projection, ne pas confondre optimisation et robustesse.** Le
   [contact exact de majorité uniforme](../../receipts/audit_continu_20260929/uniform_majority_contact_20260930/README.md)
   retire un vote fort loin du bord de bande. La section dédiée donne
   une condition de stabilité à identités/poids fixes, pas un port acquis.

Ces preuves ne modifient aucun fichier moteur et n'utilisent pas GCP.
La vue [courante](../AUDIT_ETAT_COURANT.md) tient compte du retour à l'audit,
du développeur actif et de sa décision de grille u32 par paliers.

## Ancrage persistant et calcul en flux

Relecture du 30 septembre vers 12 h UTC des mémos privés dans
`build/v10-verrou-points/`. Le mémo `ancrage_marges` propose une piste
plus robuste que la bande non saturée, à **K fixé**, n≥K. Pour un point x,
α est son premier rayon de couverture et M(r) le premier rayon où toutes
les composantes qui le couvrent à r sont réunies. L'ancrage persistant
`Pκ` prend la date `t=max(α,sup_r[M(r)−κ(r−α)])`, κ≥1, puis suit la
lignée de première couverture. La preuve par entrelacement tient à la
contre-relecture : dates `(1+2κ)ε`, hauteurs `(1+4κ)ε`, pour déplacements
appariés ≤ε, mêmes IDs/effectif et même K. Ce sont des bornes **en rayon**,
pas en niveau β=r² ; ni stabilité EOM/ARI ni robustesse aux retraits.
Pour κ≥2, les témoins de niveau β>4α² sont inutiles. P2/P4 sont des bras
pertinents à comparer, pas un choix industriel déjà qualifié.

**Simplification supplémentaire démontrée par l'audit.** Il n'est pas
nécessaire de construire ou trier l'antichaîne, ni le code-barres par point.
À chaque date c croissante, prendre J(c), LCA de **tous** les nœuds témoins
déjà vus. Alors `M(c)=max(c,b(J(c)))`, y compris en présence d'ancêtres
redondants. Ceux-ci sont nés avant c et ne créent pas de nouvelle composante
couvrante. Entre deux dates, `M(r)−κ(r−α)` n'augmente pas. D'où exactement
`t=max(α,max_c[b(J(c))−κ(c−α)])` : un balayage du catalogue déjà ordonné
suffit. Contrairement à la bande, les ancêtres supplémentaires ne retardent
pas Pκ : leur éventuelle contribution est absorbée par α.
Pκ dépend ainsi des composantes couvertes, pas du choix entre deux
catalogues qui décrivent **exactement le même Cov**. Cela n'autorise ni
catalogue tronqué ni suppression des incidences internes.

Conserver séparément J1, LCA de **toute la première cohorte exacte**,
puis `owner=anc_t(J1)`. Employer J final donnerait un propriétaire né
après t. Les incidences I∪U complètes et leurs propriétaires vivants sont
indispensables, notamment pour les entrées internes K3/K5 ; K1 garde
ses sites à zéro. L'oracle abstrait et ses contre-tests sont dans la
[preuve en flux](../../receipts/audit_continu_20260929/persistent_anchor_stream_20260930/README.md).
Ce n'est pas une nouvelle exécution géométrique ou native.
Le [lecteur R2](../../receipts/audit_continu_20260929/persistent_anchor_stream_20260930/receipt_reader_r2/CLOSURE_R2.md)
relie aussi les inventaires, commandes et flux ; la première archive,
dont la vérification de métadonnées était moins forte, reste inchangée.

Le coût devient O(D·coût_LCA+n·coût_ancêtre) après l'ordre global, avec
O(n) états en plus de l'index et du catalogue ; D compte **toutes les
incidences réellement parcourues**. Un cutoff ne rend pas gratuites la
lecture ou la génération des incidences écartées. La parallélisation par
point est indépendante si ses listes conservent l'ordre. Sur GPU, une
transposition stable et des préfixes segmentés LCA puis maximum sont une
architecture possible, dont mémoire O(D) et coût sont à payer. Ne pas
remplacer ces préfixes par un seul LCA final : les dates intermédiaires
comptent. Aucune borne de D, croissance 8k/16k/32k ou cible G4 acquise.

Contre-test abstrait pour une réduction GPU trop pauvre : A/B naissent
à 1, racine à 100, κ4. Les tranches `[(2,A),(100,racine)]` et
`[(2,B),(100,racine)]` ont le même résumé local
`(J_final=racine,t_local=2,α_local=2)`. Après le préfixe `[(1,A)]`,
elles donnent pourtant t=1 et t=96. LCA est associatif ; ce résumé
local de date ne suffit pas. Cela n'exclut pas d'autres résumés enrichis,
mais interdit d'annoncer ce seul maximum local comme réduction exacte.

Le mémo révèle aussi un défaut de **la règle** de bande non saturée,
même avec antichaîne minimale : une branche très courte apparaissant
à l'intérieur de la fenêtre peut repousser l'attache jusqu'à sa mort.
Une marge au bord ne suffit donc pas. Le contre-exemple Thalès K2 du
développeur donne un saut de hauteur voisin de 395 pour un déplacement
d'une unité ; notre tranche ne rejoue pas ce natif. Le gain exact sans
tri de l'antichaîne reste correct, mais ne justifie pas son port comme
solution robuste. Préférer l'étude bornée de Pκ à une grande campagne
de qualification des bandes.

### Variante rationnelle à comparer sans grand chantier

Pour éviter les sommes de racines de Pκ, l'audit propose une **autre
règle**, Qκ, κ entier≥2 :
`T²=max(α²,max_c[β(J(c))−κ(c²−α²)])`, propriétaire initial remonté
à T. La normalisation de résolution est la même ; le maximum et le
cutoff se décident uniquement en rationnels. Ce n'est pas le calcul de
Pκ en rayon carré, ni une correction de son mutant « β ».
La preuve de stabilité porte sur M(c)=max(c,b(J(c))), pas sur un
entrelacement supposé de b(J) seul. Employer β(J) brut dans le calcul
reste exact : le terme normalisé c²−κ(c²−α²) est toujours ≤α².

La [preuve conditionnelle et les petits contrôles exacts](../../receipts/audit_continu_20260929/quadratic_anchor_rule_20260930/README.md)
donnent α≤T≤2α, cutoff `c≤qκ·α`,
`qκ=(1+sqrt(κ²−κ+1))/(κ−1)`. Avec l'entrelacement couvrant complet,
dates Cκ·ε et hauteurs `(Cκ+2κ)ε`,
`Cκ=(κ+1)(qκ+1)`. Le test de cutoff est lui aussi sans racines :
`v=(κ−1)β−κα²`, retenir si v≤0 ou v²≤4α²β. Les sommes de racines
disparaissent, pas les obligations d'arithmétique exacte : pour les
bornes N<2^266/D<2^200 et κ≤8, les produits de comparaison de deux
dates Q peuvent demander 1 271 bits, soit vingt mots de 64 bits.
Ne pas réutiliser implicitement le comparateur huit mots du catalogue.

Qκ retarde au moins autant que Pκ au **même** κ ; par exemple abstrait
α=1, branche concurrente née à 3/2 et fusionnant à 5/2, κ2 :
P donne 3/2 et Q donne sqrt(15/4). Il n'y a donc aucune promesse de
meilleur rappel ou d'EOM supérieur. Faire seulement une ablation bornée
P2/P4 versus Q avant d'envisager un port ; aucun moteur, test géométrique,
gain G4 ou borne du nombre d'incidences n'est qualifié par cette proposition.

### Les deux triangles et la majorité de bande

La [réponse du développeur publiée dans 9ca8e4f6e](../REPONSE_CLAUDE_AUDIT_GEANT_20260930.md)
retient RAII des sorties et notre correction par cohortes, puis remet le
choix de la tête avant u24. Son rappel des deux triangles est pertinent :
au départ simultané AC/BC/CD, Pκ exige la réunion de toutes ces lignées,
et peut laisser C/D seuls jusqu'à la réunion globale. Cela ne contredit
pas sa borne de stabilité, mais cette borne ne garantit pas la partition
ABC|DEF recherchée. Nos simplifications d'antichaîne/flux n'avaient pas
démontré une meilleure qualité statistique.

Étudier la majorité de bande à dénominateur figé est une suite raisonnable,
sans la déclarer déjà robuste : si les poids initiaux sont positifs,
portés par une unique composante vivante puis remontent seulement vers
ses ancêtres, une majorité **strictement supérieure à la moitié** ne peut
appartenir à deux composantes disjointes. Une fois acquise, elle suit sa
lignée ; une date d'attache fixée et cette lignée donnent des partitions
emboîtées. C'est une justification conditionnelle de la laminarité,
pas une borne de stabilité sous perturbation ni une garantie EOM/ARI.
Déclarer l'unité pondérée : partie K distincte, boule canonique, incidence
ou autre ; recopier une ligne de support ne doit pas créer silencieusement
un vote supplémentaire. Déclarer aussi les cas exactement moitié et les
changements d'éligibilité au bord de la bande. Conserver le bras Pκ comme
contrôle stable, et juger les départages dans la fixture des triangles.
Les tableaux annoncés dans cette réponse ne sont pas de nouveaux runs
natifs effectués par notre audit.

**Contre-exemple analytique pour les poids 1/β, pas pour l'uniforme.**
K2, x=(0,0,0), a=(2,0,0), b=(0,2+ε,0), η=1/8, ε≥0 suffisamment petit.
La bande de x contient xa et xb, de rayons 1 et 1+ε/2 ; ab est hors
bande. Aucun témoin n'approche son bord quand ε→0. À ε=0, les deux
votes valent un : la majorité stricte attend la fusion à r=√2. À ε>0,
le vote xa est strictement majoritaire et x s'attache à r=1 ; a suit xa
dans les deux cas. La hauteur de réunion x/a saute donc de √2 à 1
pour un déplacement de b tendant vers zéro. Γ2 de ces trois sites a
trois sommets-paires, fusionnés au rayon de la boule diamétrale ab ;
l'angle en x est droit, donc ce rayon est √(4+(2+ε)²)/2. Ce calcul
ne relance aucun natif. Publier aussi la **marge de vote**, non seulement
la marge au bord de bande. L'uniforme évite ce contre-exemple précis,
mais pas le retrait de vote fort au contact détaillé ci-dessous.
Une stabilité locale demanderait à la fois la correspondance des atomes,
une marge au bord de bande et une marge de majorité supérieure à la
variation totale des masses normalisées. Le contre-exemple uniforme
ci-dessous réfute aussi la stabilité des votes forts loin du bord de bande.
Dédupliquer une même boule peut
éviter les votes arbitraires ; dédupliquer après remontée à une même
composante supprimerait au contraire les deux votes AC/BC contre CD.
Préserver donc les masses des atomes choisis lors des fusions, avec une
unité canonique déclarée et sans promettre une invariance aux perturbations
qui scindent une boule cosphérique.

La bande apporte néanmoins une borne géométrique utile, sous complétude
de Γ_K et propriétaires corrects, K≥2 : poser u=(1+η)α. Chaque témoin
sélectionné provient d'une K-partie dans une boule de rayon≤u couvrant x,
donc tous ses sites sont dans B(x,2u). Les K-parties de leur réunion
sont reliées par les (K+1)-parties ; toutes sont contenues dans cette
même boule et leurs niveaux de fusion sont donc≤2u. Au plus tard là,
une composante reçoit toute la masse figée : première majorité t≤2u,
soit β_t≤4(1+η)²α². Une couverture interne demande le témoin K-partie
équivalent déjà établi, pas une incidence choisie arbitrairement.
Cette borne supprime le retard arbitrairement lointain, pas les
discontinuités de vote ni l'obligation de générer les témoins complets.

Pour une comparaison de hiérarchies, condenser ensuite les mêmes unités
de points, même mcs, même λ=r^(−z), mêmes politiques racine/EOM, z1 puis
z2. L'EOM d'une masse fractionnaire de votes n'est pas ce même comparatif.
La stabilité des hauteurs ne garantit pas celle des labels : un écart EOM
parent/somme des descendants doit aussi être contrôlé. Figer les fixtures
cibles avant le choix η/z, puis confirmer sur les scènes non utilisées.

### Majorité uniforme : un contact suffit, même loin du bord de bande

La [preuve géométrique close](../../receipts/audit_continu_20260929/uniform_majority_contact_20260930/README.md)
utilise quatre sites affine-3D, K2, η1/8 :
`a=(0,0,0), b=(10,0,0), z=(9−ε,3,0), y=(0,0,9)`.
Γ2 est construit entièrement : six sommets-paires et quatre événements
triples, recoupés par un second calcul MEB à supports positifs Fraction.
À ε=0, α_a²=81/4 ; seuil de bande 6561/256. **Dans cette bande**,
a possède les trois votes forts uniformes AY, AZ et AB ; AB/AZ rejoignent
la même composante à β25, donc a la choisit avec 2/3 des votes. b a
seulement BZ dans sa bande. Leur hauteur de réunion est β25.

Pour ε>0 petit, la puissance de z dans AB vaut `−8ε+ε²<0` : AB passe
de p0 à p1, q_min2. Son masque fort `p+q_min≤K` disparaît, mais la
boule et l'événement faible ABZ à β25 **restent dans FULL/Γ2**. a n'a
plus que AY/AZ : une voix sur deux à β25 n'est pas une majorité stricte.
Il attend la réunion AYZ à `F=(171−18ε+ε²)/4`. Sa hauteur avec b tend
donc vers 171/4, au lieu de 25 ; en rayon, 5 devient √171/2 alors que
le déplacement tend vers zéro. Aucun bord de bande n'est franchi,
aux quatre points. Ce n'est ni une scission de boule ni une erreur du
générateur exact : c'est une discontinuité de la règle statistique.
Le dénominateur est figé pendant chaque remontée, pas entre les deux
nuages. Une telle règle reste laminaire mais n'est pas stable.

Onze cas exacts normal/−O, dont l'homothétie u18 à S1024 avec un jitter
d'une unité, et deux mutations de mauvaise valeur. Deux relectures
indépendantes concordent. Aucun appel natif, EOM/ARI, benchmark ou GCP.
Réduire η ne résout pas généralement ce mécanisme : pour T=M²+1,
`b=(2T,0,0), z=(2M²−ε,2M,0), y=(0,0,2T)`, choisir
`1+1/M² < (1+η)² < 2+1/M²`. Cela permet η>0 arbitrairement petit,
notamment tout η<√2−1 avec M assez grand. Les deux inégalités sont
nécessaires au schéma ; aucune assertion pour tous les η très grands.

**Une condition de stabilité réellement utile.** Fixer les identités
des points et des atomes votants, leurs univers et leurs poids positifs.
Si deux filtrations Γ s'entrelacent à ε par **deux** maps compatibles,
transportant ces admissions/atomes, toute masse >W_x/2 se retrouve dans
une composante image à r+ε. La majorité y est unique ; tous les points
du même bloc suivent cette même image. Les partitions complétées par
des singletons s'entrelacent donc à ε, et les hauteurs de paire changent
d'au plus ε **en rayon**. Une seule map ne donne qu'un raffinement dans
un sens. Cette preuve ne couvre pas EOM/labels, poids 1/β variables,
fusion d'IDs ou ajout/retrait de points.

Référence bornée : votes uniformes de toutes les K-parties F contenant x,
dont le MEB est dans la bande. Les IDs de ces parties restent fixes et
chaque rayon MEB est 1-Lipschitz sous déplacement maximal ε. α_x aussi.
Une marge `|r_F−(1+η)α_x|>(2+η)ε` conserve leur sélection ; Γ complet
se transporte par les K- et (K+1)-parties. Dans le contre-exemple, AB
reste alors un vote et la hauteur reste β25 ; une marge conservatrice
de 1/16 en rayon donne ε<1/34. Ce contrôle **change l'unité de vote**.
Ne pas le présenter comme une réparation déjà portée des boules fortes.
L'énumération C(n,K) est hors voie industrielle : trouver une compression
ou un comptage conservant ces identités/masses est le problème ouvert.
L'optimisation suivante calcule une majorité donnée, pas ses atomes gratis.

### Compter les K parties par boule minimale sans les énumérer

La [preuve de comptage close](../../receipts/audit_continu_20260929/parts_meb_class_counts_20260930/README.md)
donne une identité exacte pour une boule B=(c,r), I intérieur strict,
p=|I|, U coquille. Poser h_B(j)=nombre de S⊂U, |S|=j, avec c∈conv(S),
et h_B,x(j) le même nombre exigeant x∈S. Le nombre de K-parties F
contenant x et dont la boule minimale est **exactement B** vaut :

- x∈I : `Σ_j h_B(j)·C(p−1,K−j−1)` ;
- x∈U : `Σ_j h_B,x(j)·C(p,K−j)` ;
- x hors B : zéro, avec binômes hors domaine définis à zéro.

F∩I et F∩U donnent une décomposition unique ; le critère MEB=B est
`c∈conv(F∩U)`. Chaque F a une seule boule minimale : pas de somme des
binômes de simples contenances qui double-compterait les mêmes parties.
Pour U affinement indépendant, c strictement intérieur, seul S=U
contribue. En revanche, sur un carré cosphérique, h(2)=2,h(3)=4,h(4)=1 :
compter seulement les supports minimaux perd des votes. Le petit oracle
compare 113 comptes point/K sur segment, triangle, tétraèdre et carré,
contre une énumération MEB indépendante, normal/−O. Cette énumération
est un oracle petit, pas une proposition pour le produit.

**Le catalogue courant ne donne pas toutes ces classes.** À K=Kmax=2,
sites colinéaires 0,19/10,39/20,2 et point x=0, les trois paires contenant
x sont dans la bande η1/8. La classe diamétrale {0,2}, rayon1, a p2,
q_min2 ; elle est omise par `p+q_min≤Kmax+1` et n'est pas vivante à K2.
Elle porte pourtant un vote de la référence. La masse passe de trois
à deux si on ne compte que les classes du catalogue. C'est une perte
de masse/identité, **pas** une erreur FULL ni un flip de hauteur attesté.

Prolongement **analytique**, distinct des 113 contrôles : M=Kmax≥K≥2,
point0 et M+1 sites distincts dans [19/10,2], dont l'extrémité2. Toute
K-partie contenant0 a un rayon entre19/20 et1 ; toutes restent strictement
dans la bande η1/8. La classe de diamètre {0,2} a p=M et q_min2,
donc est omise, mais porte C(M,K−2) votes parmi C(M+1,K−1), fraction
`(K−1)/(M+1)`. Pour M10, cela vaut4/11 à K5 et9/11 à K10. Contre-relecture
indépendante concordante, aucun nouveau test ni hauteur projetée calculée.

Il reste à retrouver et payer ces classes, leurs propriétaires et leurs
incidences, ou un comptage différent directement sur FULL et le nuage.
Cette obstruction locale ne prouve pas l'impossibilité de cette seconde
voie. Coquilles non régulières et tailles des compteurs exacts restent
payées ; aucun accumulateur u64 implicite. Ni élargissement aveugle du
catalogue ni reconstruction explicite de Γ ne sont recommandés, et aucune
croissance sous-quadratique ou performance G4 n'est acquise ici.

### Majorité : deux sélections au lieu de toutes les lignées

La bibliothèque privée `fixtures_cibles/lib/regles.py`, relue à 13 h 59 UTC
(SHA `517957e4e48d0f5b8f6e129955c2fc63b0d23613fa472a990d788b7e8d18bfd6`), assemble
la réunion de toutes les lignées des propriétaires, puis visite les enfants
de ces nœuds. Même avec peu d'atomes, elle peut donc payer la profondeur
de l'arbre ou le degré d'un ancêtre par point. Le sweep direct reste un
oracle borné, pas une voie industrielle. Voici une simplification qui
**conserve exactement cette majorité**, sans changer les poids ni la bande.

Pour x, garder ses atomes fixes `(c_i,v_i,w_i)`, w_i>0, W=Σw_i. Une
activation se produit dans la durée de vie de son propriétaire ; ensuite
sa masse ne suit que les ancêtres. Arbre à racine commune, niveaux et
cohortes fermés. Préparer une fois naissances b, Euler, LCA et ancêtre.

1. Choisir m, propriétaire du premier quantile pondéré Euler dont le
   cumul dépasse **strictement** W/2.
2. Calculer pour chaque atome `h_i=max(c_i,b(LCA(v_i,m)))`.
3. Choisir t, premier quantile pondéré des h dont le cumul dépasse W/2.
   Compter toute la cohorte à h=t, pas seulement un préfixe départagé.
4. Renvoyer `owner=anc_t(m)`, avec coupe fermée ; un LCA de durée nulle
   peut déjà être mort à t et ne doit pas être le propriétaire renvoyé.

**Pourquoi cela suffit.** Un sous-arbre contenant plus de W/2 de masse
future contient le médian m : son intervalle Euler laisse moins de W/2
à l'extérieur. Toute composante active majoritaire est donc un ancêtre
de m. À r≥b(m), l'atome i lui appartient exactement si c_i≤r et son
LCA avec m est né, soit h_i≤r. Tous les h_i sont≥b(m). Le second
quantile est donc la première majorité de toute la forêt, pas seulement
une majorité cherchée sur une lignée arbitraire. Ne pas retirer les
ancêtres « redondants » : contrairement à Pκ, leurs poids changent W.

La [preuve et l'oracle clos](../../receipts/audit_continu_20260929/weighted_majority_select_20260930/README.md)
comparent 392 cas, 7 742 atomes et 1 176 variantes d'ordre, normal/−O.
Quatre mutations produisent une mauvaise valeur : médiane basse, pivot
arbitraire, activation omise, suppression des ancêtres pondérés. Deux
contre-relectures indépendantes ne trouvent pas de défaut. Les listes
8k/16k/32k testent uniquement la sélection ; ce ne sont pas des nuages.

Coût proposé après préparation globale :
`O(D·coût_LCA+D+n·coût_ancêtre)`, avec deux sélections à pivots équilibrés
et D=incidences retenues réellement lues. L'index coûte séparément O(H)
pour Euler et le coût déclaré de son LCA/ancêtre. Le prototype emploie
des marches de parents sur petits arbres : il ne qualifie pas cet index.
Si admissions et naissances ne sont pas déjà ordonnées ensemble, payer
leur union exacte ; jamais un max entre deux tables de rangs indépendantes,
ni entre doubles qui ont coalescé des niveaux distincts.

**Calcul à prototyper : uniforme d'abord, sans qualification statistique.**
La sélection devient la médiane
haute d'indice d//2, y compris pour d pair. CSR par point, sélection
segmentée de tin, D requêtes LCA indépendantes, sélection segmentée de h,
puis n requêtes ancêtre : les grandes opérations sont parallélisables.
Radix ou BFPRT possibles ; `nth_element` quelconque ne prouve pas un
pire cas linéaire. Préparation, passes, allocations et mémoire O(H+D)
restent à mesurer. En 1/β, les sommes rationnelles peuvent demander des
dénominateurs énormes : borne en opérations, pas coût bit ni accumulateur
natif qualifié. Aucun gain FULL/LiDAR/G4 ou avantage EOM/ARI encore mesuré.

## Condensation directe sans expansion de la tour

Le plan privé T2 reconnaît maintenant le défaut des départs différés.
La [référence publiée par l'autre auditeur](../../receipts/audit_independant_20260930/developer_rebound/condensation_reference/README.md)
est une aide pertinente : quotient des plateaux, cohortes d'observations,
puis tête existante sur un arbre d'événements. Elle conserve les coupes
de points et produit au plus H+n nœuds. Cette borne de représentation
ne mesure ni son prototype récursif, ni le temps du sweep exact, ni un
port natif. Le mapping `node_cluster` pour un vote de boules reste ouvert.
Contre-vérification du 30 septembre : lecture de la référence et du juge,
40 hashes conformes avant import, lecteurs normal/−O concordants sur
536 condensations et 2 144 sélections exactes. Relecture des captures
closes seulement, aucun nouvel appel natif ni sklearn de notre part.

Notre [petite contre-épreuve indépendante](../../receipts/audit_continu_20260929/point_plateau_condensation_20260930/README.md)
compare douze points, mcs5, racine exclue : A/B ont chacun trois points
à β1 ; D en a six à β1. La racine à β25 a soit directement A/B/D,
soit C/D, avec C=(A/B) également à β25. Les deux objets passent le
validateur et donnent exactement la même matrice ultramétrique 12×12.
Pourtant le vrai C++ crée C, de stabilité nulle, et D dans le second
encodage ; le premier donne tout bruit, z1 comme z2. Une fusion de durée
nulle ne doit pas créer une étape de sélection. Ce témoin valide API
n'est pas une réalisation 3D ni une comparaison HDBSCAN nouvelle.

**Proposition de port direct, sans modifier FULL ni matérialiser une
nouvelle chaîne de nœuds.** Domaine initial : niveaux strictement positifs,
masse entière positive, racine de masse≥mcs ; les refus zéro/infini et
la racine sélectionnable restent à juger séparément.

1. **Normaliser une fois.** Calculer de la racine aux feuilles le quotient
   des arêtes de même rang. Réattribuer aussi au parent du quotient un
   point attaché exactement au rang de ce parent. Grâce au contrat de
   durée de vie et à la contraction préalable, un seul parent distinct
   suffit. O(H+n), avant le calcul des masses. Sans cette réattribution,
   les masses des enfants au split comptent des points qui partent au
   split lui-même. Conserver la correspondance avec les nœuds originaux ;
   son usage par un vote demande une spécification distincte.
2. **Préparer les cohortes.** Trier globalement les IDs par rang d'entrée
   décroissant, puis les distribuer dans un CSR stable par propriétaire.
   Le tri coûte O(n log n), le CSR O(H+n). Pas de tri pour chaque branche
   ni de liste de points copiée à chaque ancêtre. Un radix peut remplacer
   le tri si son coût et ses capacités sont effectivement payés.
3. **Descendre en densité.** Chaque branche commence avec sa masse active.
   Retirer toute une cohorte à la fois, puis tester `masse_restante<mcs`.
   **L'égalité à mcs survit** : le mutant `< au lieu de <=` annoncé dans
   le plan privé T2 a donc sa polarité inversée. Coordonner le dernier
   départ avec le split géométrique au même rang, après normalisation.
   Si le seuil est franchi, terminer à cette date et faire sortir tous
   les survivants, sans ouvrir les branches géométriques suivantes.
4. **Ne rien repayer.** À cet arrêt, parcourir seulement le suffixe direct
   non traité et les sous-arbres enfants encore non visités. Appeler
   `drop_subtree(v)` après les départs déjà payés compterait ces points
   deux fois. Chaque point et nœud est soit traité, soit abandonné une
   fois. Garder la somme pondérée sortie−naissance actuelle, ou intégrer
   masse×Δλ : remplacer l'une par l'autre, jamais les additionner.
5. **Finir EOM et les labels en deux passes.** DP bottom-up, puis propagation
   top-down du premier ancêtre sélectionné. Les labels de points lisent
   directement ceux de leur cluster de sortie. Ne pas réintroduire une
   marche d'ancêtres par point/cluster ; les copies R2 ont déjà corrigé
   cette croissance potentiellement quadratique, pas encore la condensation.

Sous ces préalables, le port direct peut avoir O(H+n log n) travail,
O(H+n) stockage et sortie explicite, H=nœuds de l'arbre de points.
Justification : deux parcours de normalisation/masses, un tri des seuls
points, puis chaque cohorte/nœud/point consommé une fois et deux passes
sur les clusters condensés. C'est une **architecture proposée**, pas une
mesure de l'implémentation actuelle. Elle ne borne ni H en fonction du
nuage, ni les candidats q3/q4, ni le coût de la tour FULL. Les niveaux
exacts doivent rester séparés des valeurs λ destinées à l'intégration :
le producteur actuel coalesce certains niveaux exacts dont les doubles
coïncident. Trier ces rangs coalescés ne restaure pas l'exactitude perdue.
Le [nouveau témoin natif à trois sites](../../receipts/audit_continu_20260929/point_exact_rank_coalescence_20260930/PROTOCOL.txt)
le reproduit dans u18, en `FE_TONEAREST`, pour
(0,0,0), (261120,2,0), (1,512,0). AB a β=17045913601 ; ABC a
`β_AB+17045913601/17873935364259844>β_AB`. FULL K2 garde les rangs
exacts 3/4, mais `PointDendrogram` les rend 3/3 en core et 2/2 en cover,
avec le même double `0x1.fc0200008p+33`. Le quotient correctement
arrondi séparerait ici les valeurs d'un ulp : la conversion séparée du
numérateur/dénominateur ajoute une perte. Même corriger cette conversion
ne garantit pas l'injectivité des doubles. Préserver les rangs exacts
de la forêt et des attaches, garder les doubles en vue numérique séparée.
Ce reçu utilise une archive native figée et quatre sources critiques
avant/après, pas un rebuild qualifié de toutes ses dépendances. Deux
candidats rejetés sont conservés ; lecteur normal/−O, aucun rejeu natif
à la lecture. Ni forêt FULL fausse ni changement EOM démontrés.
Les masses progressives fractionnaires peuvent franchir le seuil entre
événements ; ce plan de cohortes entières ne les qualifie pas.

Portes ciblées demandées : égalité de masse exactement mcs, poids entiers,
départs au split, permutation d'IDs, insertion/contraction de nœuds de
durée nulle, puis les cas natifs clos et les différentiels de tête.
Ne pas ouvrir une grande campagne statistique avant ce raccord.

## Condensation et portée des propositions statistiques

Dans le mémo `masses_selection`, `lib/selection.py:68` ne lit que les
masses de fin de vie des enfants, puis l'EOM intègre toute leur vie.
C'est une condensation **terminale** explicitement distincte ; les masses
progressives peuvent franchir mcs au milieu d'une branche. Leur égalité
terminale avec les masses par marches n'implique pas la même condensation
dynamique. Exemple exact, deux sites distants de 2, K2/z2 :
`m(λ)=2(1−λ)` sur λ∈[0,1]. À mcs1, l'intégrale pleine vaut 1 ; arrêt
au seuil λ=1/2, elle vaut 3/4. La racine exclue peut masquer ce petit cas
dans la sélection, pas supprimer la différence de définition. Pour les
points durs, nos contre-exemples natifs clos restent la porte à intégrer.
Réparer la condensation dynamique ou annoncer et comparer séparément
la condensation terminale ; ne pas la présenter comme équivalente au
critère standard de HDBSCAN.

Le mémo `cible_statistique` prouve la pureté pour r<Δ/2, pas jusqu'à
toute FIC **des représentants**. Sur les sites 0,1,10,11 avec classes
{0,1}/{10,11}, K2, Δ=9, une règle couvrante admissible peut retarder
1 et 10 puis les attacher à leur paire, composante née à r=4,5. Leur
bloc est mixte avant la fusion des lignées core des représentants 0/10
à r=5. Cela ne réfute pas Pκ, qui impose la première lignée ; cela
réfute le corollaire universel pour toute règle admissible à cette FIC.
Une première composante mixte et une première réunion des représentants
sont deux événements différents.

Enfin, H1/H4/H6/H7 du protocole restent des hypothèses : une borne
Poisson d'entrée ne prouve pas le rappel connecté, la fidélité n'ordonne
pas les précisions, la localité ne donne pas un écart statistique de 0,02,
et un pilote n'établit pas une garantie. L'obstruction de consistance
démontrée à K2 pour la distorsion maximale en niveaux ne devient pas
une impossibilité Hartigan générale à K5. Garder ces limites avant tout
test confirmatoire ; la demande utilisateur ne garantit pas une victoire
universelle sur HDBSCAN.

## Raccords et deux défauts ciblés

**Mise à jour du 30 septembre à 12 h 57 UTC.** Le vrai chantier actif est
`build/v10-integration-r2/src`, base `85c2c1d`. L'étape faits_math est
close (15/15 gates, 5/5 fast, huit binaires alors identiques à la référence).
L'étape suivante SiteTree est également close : 19/19 gates, 325,02 s,
55 mutants non équivalents tués et un équivalent accepté. Les nouvelles
gardes SiteTree refusent `-Ofast` au TU, alors que les sources publiées
l'acceptent encore ; le refus global CMake est déjà prévu par le plan T1.
Observation des logs, pas nouvelle exécution de ces campagnes. La tête
reste SHA `f583da400d00571a547989a46b1690f2bb093e9e068a01897078363a92674578`,
donc non corrigée pour la masse résiduelle. La tour vient ensuite d'être
modifiée pour les arrondis : cette nouvelle étape n'hérite pas des 19/19.
À 13 h 43 UTC, son journal B est désormais clos : **22/22 en 483,51 s**,
porte d'arrondi et porte CMake comprises ; code pilote 0. À la reprise
suivante, B est committée dans HEAD `d303c88f5`, copie toujours isolée.
Observation seulement, aucun nouveau lancement de notre part.
Ni les sept groupes ni u24/u32 ni G4 ne sont qualifiés ensemble.

**Reçu A/B relu ensuite.** Ses 100 hashes concordent ; le patch B est
exactement le diff `62c8e07→d303c88`. Les binaires du build courant et
du build-B-neuf ont aujourd'hui les identités publiées. Cela ne remplace
pas des empreintes source/dépendances/compilateur/binaires avant et après
les campagnes : celles-ci ne sont pas fournies pour chaque invocation.
Les 80 mutants tués sont 55 A + 17 tour + 8 CMake ; la contre-porte de
34 mutants recouvre A et n'est pas à additionner. Le complément −O
rejoue six mutants sélectionnés, pas les 81. Les trois nouveaux juges GCC
et les deux configurations Clang sont distincts du lot CTest complet.

Les dix dumps de tour ont des hashes réellement non vides et concordants
dans le journal, sans défaut observé. Leur script peut pourtant afficher
`IDENTIQUES` si les trois exports manquent avec code0 : le pipeline
`sha256sum | cut` ne vérifie ni existence ni succès et trois chaînes
vides sont égales. Exiger fichiers et hashes valides, conserver les trois
digests ; les dumps sont supprimés par ce script, pas par notre audit.
Les dix-huit différentiels de tête (mcs8/9, z1/2/3) jugent non-régression
des sorties, pas le nouvel oracle de condensation. Leurs logs utilisent
`<final>`/`<travail>` au lieu des argv exacts. Réserve de preuve, pas
annonce que ces campagnes ont produit des données fausses.

**Option Clang citée : garde de configuration contournée.** Le
[reçu portable](../../receipts/audit_continu_20260929/quoted_build_flags_20260930/README.md)
conserve quatre appels courts sur sources figées : flag non cité refusé,
`CMAKE_CXX_FLAGS="-freciprocal-math"` accepté, puis option réellement
consommée dans la commande SiteTree. Clang 18.1.3 ne publie aucune des
trois macros de garde et le préprocesseur SiteTree passe. C'est un défaut
de contrat de build, pas un résultat géométrique erroné démontré ; aucun
objet moteur ni FULL exécuté. La porte B ne contient pas cette fixture.
Tokeniser tous les ensembles de flags avec le mode de plateforme approprié,
tester les tokens interdits puis ajouter cette régression. Un motif sur
la chaîne brute laisse passer les guillemets interprétés par le compilateur.
Ce constat concerne B/d303c88, pas automatiquement le chantier suivant.
À 17 h 23 UTC, le développeur a ajouté `cmake/fp_flags.cmake`
(SHA `22036c6616828272f8c8254837c5d884a817629877c3736fe424858270f7618d`) :
`separate_arguments(NATIVE_COMMAND)` puis comparaison de tokens, contrôle
différé après création des cibles, limites explicitement documentées
pour fichiers de réponse et générateurs qui fabriquent un token.
Les journaux GCC/Clang terminés `fpgate_g++.txt` et `fpgate_clang++.txt`
refusent bien `cite_auditeur` avec code1 et finissent à 285 unitaires,
24 refus, quatre témoins. Hashes `4047cc313fcaa00c753a7293992f808f88e2745b05ae54b0ecccc5a6e1a6aa6d`
et `a0836110199363d1008f8934221edb9a0f21ccd2891cc1c879b8d41481881b04`,
durées 25,819/34,046 s de ces portes seulement. Sources non committées,
pas d'inventaire avant/après ni code pilote archivé : observation de
progrès, pas qualification close. Depuis, `ffm_cmake.txt` est clos :
22/22 mutants CMake tués, durée 235,017 s, SHA
`8ddf4965001a139569ca9f2d11b9f366a03db68b83c98aac965e63c4a9143e04`.
Ce lot seul ne vaut pas les 94/94 de la campagne commune. **État actualisé
à 18 h 46 UTC :** l'addendum est commis `2d0a0c41c`, à 18 h 25 min 12 s.
Son SHA256SUMS `1308b3d1…` donne 16/16 fichiers concordants ; 24/24 CTests
gate, code0 en 1842,57 s, et 5/5 fast, code0. La campagne commune a une
vraie conclusion terminale : 94/94 tués, un équivalent, zéro anomalie,
code0 en 28 min 02 s. Neuf objets moteur et huit exécutables sont identiques
à d303c88 selon le relevé privé. Ce reçu ferme cet addendum SiteTree/tour/CMake,
pas toute l'union R2 ni la future campagne des bancs. Notre audit vérifie
les fichiers et conclusions archivés ; il ne relance pas cette campagne.

Le groupe bancs partiel a été sauvegardé dans `wip/bancs_partiel*.patch`
et `wip/fichiers_bancs_partiels`, puis retiré du clone vers 16 h 59 UTC ;
HEAD reste d303c88. Son journal tête supplémentaire est clos 13/13,
385,95 s, code pilote0. Ne pas y voir la correction par cohortes ni
additionner cette observation aux anciennes campagnes comme preuve d'union.
Ce paragraphe décrit le retrait historique. Les bancs sont désormais
réintroduits dans le clone 2d0a0c41c : schéma P6, doublons CSV/JSON refusés,
alias out/calls et out/session refusés avant troncature, dix journaux de
portes terminés code0. Le différentiel du runner est clos à 18 h 16 UTC :
cinq entrées K5 et une K10, 22 colonnes déterministes inchangées, mêmes
binaires. Mais index ancien et fichiers courants diffèrent (`MM`/`AM`) ;
committer le seul index publierait encore le runner antérieur. Pas de
manifest bancaire final ni de qualification intégrée retrouvés ; aucune
validation de condensation/statistiques nouvelle par ces différentiels.

**Limite de la nouvelle porte d'arrondi, relue vers 13 h 04 UTC.** Les
quatre filtres de `resolve` sont bien désactivés selon le mode du fil
courant ; DWelzl ne reste qu'une proposition certifiée en exact. Aucun
défaut géométrique nouveau démontré dans ces décisions. Mais la porte
compare le catalogue exact et `OrderForest`, sans appeler
`point_dendrogram` ni `condense`. `ball_nodes` était désactivé : comparer
deux listes vides ne qualifiait pas cette sortie optionnelle. Les valeurs
`level.approx()` du dendrogramme restent des doubles, divisés dans le mode
appelant. Exemple du triangle aigu (0,0,0), (8,4,0), (4,8,0) :
β=204800/9216=200/9, encadré par les doubles exacts
`0x1.638e38e38e38ep+4` et `0x1.638e38e38e38fp+4`. Les modes downward et
upward ne publient donc pas nécessairement le même double. Limiter
« sorties identiques sous les quatre modes » à l'objet exact jugé, ou
tester séparément le dendrogramme/export et définir son environnement
numérique. Le README du reçu B reconnaît désormais explicitement cette
limite dans sa section 11, ainsi que la coalescence possible de niveaux
dans les doubles de points. Cette réserve ne prouve aucun changement
de labels/EOM. FTZ/DAZ et MXCSR hors contrat cfenv ne sont pas qualifiés
par la porte actuelle ; aucun nouveau défaut géométrique déduit ici.

Le nouveau chantier active désormais `ball_nodes` et sa porte publie
237961 propriétaires de boules, comparés réellement. Il ajoute une porte
`dendrogram_rounding` séparée et écrit le contrat de tête `FE_TONEAREST`.
La capture nouvelle de l'addendum (`dendrogram_rounding.txt`, SHA
`0122accebbb3862575df0394e4185fa100d1fd2c4f754f340dd9a527178458c8`)
contient les nouveaux compteurs de rangs : 37088 niveaux, 160 clusterings,
deux β=200/9, aucun changement de rang/label dans **ces** fixtures.
Cela remplace le premier `dendro1.txt`, antérieur au test courant et
ne contenant pas ses compteurs de rang. Le test appelle `cluster(ref)`
sous les modes dirigés, où ref a été construit en nearest ; il ne teste
pas toute la chaîne `cluster(point_dendrogram(...))` dirigée. La portée
nouvellement documentée est correcte. Le témoin u18 clos ci-dessus montre
cependant que nearest ne suffit pas à préserver les rangs exacts.
La source de condensation reste inchangée : ni cette porte ni les
différentiels ne corrigent les cohortes/plateaux.

### Deux contrôles peu coûteux dans les nouveaux juges de fixtures

La [preuve portable close](../../receipts/audit_continu_20260929/target_reader_control_flow_20260930/README.md)
fige les sources privées `fixtures_cibles/lib` et extrait par AST leurs
fonctions réelles. Deux défauts reproduits normal/−O : `valide_lib.main`
rend code0 malgré `sources_stables=False` quand ses neuf groupes de tests
stubés passent ; une variante `target=[]` acceptée par `normaliser` produit
zéro verdict puis `passe=True` par `all([])` dans `juger_fixture`.
Contrôles positifs/négatifs conservés. Aucune source réellement mutée sur
disque, aucun import du module original, aucun Γ ou binaire natif exécuté.

Refuser les empreintes divergentes (code3 comme run_target), exiger une
liste de cibles non vide dans les variantes et dans l'API du jugement,
puis publier l'inventaire réellement jugé. Le code0 de run_target signifie
que le juge a tourné, pas qu'un candidat gagne : le défaut est ici le
champ `passe=True` sans test. La validation privée close de 389 contrôles,
zéro échec et sources stables, ainsi que ses captures normal/−O, ne sont
pas invalidées par ces preuves de contrôle. Elles ne qualifient pas à
elles seules les nouvelles familles cibles K3..K10 ni un profil G4.

Les observations antérieures suivantes restent datées ; elles ne décrivent
pas le nouveau binaire SiteTree :

Observation vers 12 h 15 UTC des copies privées `raccord_r2`,
`verif_raccord_r2`, `sante` et `verif_sante`, sans relancer leurs lots.
Les nouvelles portes SiteTree observent réellement le chemin du filtre
nearest et le contournement dans les trois autres arrondis ; 34 mutants
sont tués dans la contre-porte, ASan et TSan passent à ce périmètre.
Ce progrès ferme une réserve du **juge isolé**, pas FENV de toute la tour.

L'essai A combine pool/tête/SiteTree : 54/55 CTests hors oracles passent
dans son premier build, le dernier échoue par `FileNotFoundError` ; les
deux oracles passent séparément. Un second build du plan donne 37/37
portes rapides, résultat distinct. L'essai D combine pool/CLI mais garde
l'ancienne tête et l'ancien SiteTree : 23/24 puis trois seuls rejeux
réussis après évolution de CMake. Le clone propre `bf704f9` réunit sept
groupes de modifications, mais aucun build commun qualifié n'a été
trouvé. La santé ASan 6+7 et Valgrind sans erreur/fuite concernent encore
HEAD 8bb. Les différentiels clos concordent ; ils n'autorisent pas
l'addition des qualifications de ces copies. La nouvelle tête du clone
commun conserve les pertes directes de points sans contrôle résiduel :
**le défaut de condensation différée n'est pas corrigé** par ses gardes
numériques.

Deux actions petites et causales, sans nouvelle campagne G4 :

1. **Précision, juge d'orientation.** Le juge du harness calcule les
   verdicts q4, mais ne les exige pas. Suppression des deux cas, ou
   orientations et intérieur forcés à zéro, rendent toujours code0,
   normal/−O. La [preuve portable](../../receipts/audit_continu_20260929/precision_reader_orientation_20260930/README.md)
   n'exécute pas le moteur : vérifier inventaire exact, unicité,
   orientations et intérieur, puis conserver les contre-cas. Les
   `WideLevel` restent des structs de sonde, non un port FULL u24/u32.
   Les histogrammes de grille 1 mm mis à l'échelle ne qualifient pas
   le travail d'une quantification à 0,1 mm ; un rayon approx/sqrt
   tronqué n'est pas une borne extérieure pour le dispatch certifié.
2. **Sorties, garantie d'exception.** Le helper `OutputSet` du clone
   commun fuit un descripteur si une allocation lève après `fopen`,
   avant enregistrement ; la sentinelle privée reste tronquée. Un
   writer qui lève fuit aussi, bien que son nom soit retiré. La
   [capture native normale et UBSan](../../receipts/audit_continu_20260929/outputset_exception_20260930/README.md)
   conserve contrôle sans exception et compteurs 4→5→6. Mettre un
   propriétaire RAII du `FILE*` avant toute opération susceptible
   de lever. Aucun callback actuel n'est prouvé fautif ; ni défaut
   FULL ni fuite sur les fichiers utilisateur constatés par ce test.

### Banc de croissance : ne pas mélanger CSV et journal d'appels

Le [nouveau contre-exemple clos](../../receipts/audit_continu_20260929/banc_output_alias_20260930/capture/README.md)
appelle le vrai `cmd_run` de `scale_run.py`, avec `measure` simulé.
Destinations distinctes : CSV et deux lignes JSONL valides. Destinations
`--calls==--out` : code0 et message `ok`, mais les deux formats sont
corrompus. Deux ouvertures en `w` sur le même fichier gardent des offsets
indépendants. Normal/−O concordent, source inchangée avant/après ; aucun
processus HGP, aucun chrono, aucun GCP. La capture ne fournit pas d'heure
UTC d'acquisition : ne pas en inventer, ni confondre son entrée factice
« 8000 sites » avec un nuage mesuré. Refuser l'identité des destinations
avant toute troncature et ajouter le contrôle à la porte des bancs.
Le nom exact est reproduit ; liens durs/symboliques sont des cas proposés
à tester, pas déjà exercés. À la reprise de 17 h 23 UTC, le groupe bancs
partiel est sauvegardé puis retiré du clone actif : la preuve reste celle
de ses sources 14f3915d, pas de la version courante réinitialisée.
Les correctifs de schéma et leurs campagnes futures restent distincts.
Le [lecteur séparé](../../receipts/audit_continu_20260929/banc_output_alias_20260930/reader/README.md)
vérifie les douze hashes originaux avant parse, puis rejoue `cmd_run`
extrait par AST dans un répertoire temporaire privé, normal/−O. Les trois
exports sont reproduits octet pour octet, CRLF compris ; aucun import
LIVE ni subprocess. Ses dates UTC sont celles des lectures seulement.

## Demandes antérieures et leur suivi

29 septembre 2026, lecture après `56020cab6`, copies de correction encore
distinctes du produit. `public_status=not_claimed`. Moteur non modifié.

1. **Frontière.** Notre [section 9](AUDIT_LAMINARITE_POINTS_20260929.md)
   ajoute un vrai contre-exemple géométrique de la majorité à masses
   uniformes : huit petits nuages dont quatre tétraèdres, 32 exports natifs,
   les deux groupes précoces perdus avant fusion. Masses fixes en 1/β les
   récupèrent, mais ce n'est ni Sτ de la thèse ni une tête EOM qualifiée.
   Ne pas porter une structure coûteuse avant d'avoir testé le choix de
   masse sur quelques bras dev ; garder le dénominateur fixe pour la preuve
   de laminarité. Un jugement sur les seules hauteurs manquerait ce défaut.
2. **Validation parallèle ordre/tête.** Le contre-audit en cours dans
   `performance/CONTRE_AUDIT_ORDRE_TETE_CORRIGE_20260929.md` a reproduit
   une validation CSR hors bornes sur un objet public forgé, dans la copie
   `ordre_tete-verif/src_tout`. La série refuse ; le chemin parallèle peut
   lire `child_val` après sa fin. Contrôler chaque borne finale de tranche
   avant la boucle de lecture, pas seulement l'offset précédent. Aucun
   défaut d'arbre produit normal n'en est déduit. Les preuves finales
   et la fixture exacte seront dans la note, sans modification du moteur.
3. **Préintégration.** Les bonnes portes de SiteTree et des autres copies
   ne doivent pas être additionnées comme qualification d'un unique
   binaire. Au raccord : une extraction figée commune, hashes et plan
   explicite, puis juges et différentiels d'objets, y compris les nouveaux
   mutants de plateau FULL, doublons/ordre de catalogue et domaine CLI.
   La consommation entière des options numériques et la borne u32 avant
   conversion restent ouvertes dans `entrees_cli`, même si les exceptions
   sont désormais converties en refus propres.

## Réponse reçue et suivi au 30 septembre

La [réponse du développeur](../REPONSE_CLAUDE_CONTRE_AUDITS_ET_RACCORD_20260929.md),
publiée dans `e9eab2754`, retient un raccord commun figé, la correction des
bords CSR avant lecture, des juges renforcés et des bras frontière comparables.
Ce plan répond aux questions ; il ne qualifie pas encore leur intégration.

Notre [complément du 30 septembre](ADDENDUM_ZERO_ET_STATUTS_20260930.md)
précise le singleton zéro à couvrir dans la garde numérique et la différence
entre lecteur d'enveloppe CUDA et juge de qualification. Pour la frontière,
ajouter la masse fractionnaire conservée avant condensation aux diagnostics
annoncés, puis mesurer sa récupération avant fusion parasite.

Les campagnes longues anciennes ne sont pas relancées ; aucun processus
hérité n'est considéré encore vivant après la reprise d'environnement.
Les nouveaux lots d'audit n'utilisent pas GCP et ne modifient pas les
archives closes.

## Complément R2 et deuxième contre-épreuve frontière

30 septembre : le [contrôle R2](CONTRE_AUDIT_R2_20260930.md) confirme le
renforcement des juges et la porte native de la tête ; il signale une
collision étiquettes/arbre qui rend code 0 malgré l'écrasement. Rejouer ce
cas et conserver la propagation des refus numériques dans la CLI commune,
sans perdre ses contrôles d'écriture.

La section 10 de [la note frontière](AUDIT_LAMINARITE_POINTS_20260929.md)
ajoute le tétraèdre orthogonal puis son jitter : la majorité `1/β` change
fortement la réunion de C/A alors que la fusion FULL ABC ne change pas.
Ajouter ce contrôle de contact à la porte de conception déjà annoncée.
La durée effectivement couverte récupère ces petits cas, mais feuilles
seules/ancêtres/branches fantômes restent ouverts ; ne pas lancer un vaste
port sur ce seul signal. L'attache à une composante réellement unique,
avec marge et ascendance figée, reste un contrôle peu coûteux utile.

## Complément : entrées internes et trois gardes peu coûteuses

Notre [section 11](AUDIT_LAMINARITE_POINTS_20260929.md) corrige une réserve :
à K2, chaque site distinct possède bien une incidence de feuille, par
l'argument du diamètre vers un plus proche voisin. À K3 et K5, les nuages
3D à six et sept sites prouvent en revanche une entrée frontière uniquement
interne. Ajouter ces deux cas à la porte de conception ; conserver leurs
incidences et la continuation des branches. La pondération par durée
n'est pas réfutée, mais un univers limité aux feuilles serait incomplet K5.

Le [complément R2](CONTRE_AUDIT_R2_20260930.md) ferme la réserve SiteTree
pour sa nouvelle porte quatre arrondis et transmet trois actions simples :

1. Au juge des grands dumps, passer K et les sites attendus ; refuser
   les ordres manquants et points étrangers, même lorsque leurs nombres
   et la structure interne sont cohérents. Vérifier les tailles annoncées.
2. Au juge statistique, valider le schéma complet du préenregistrement,
   notamment alpha fini dans son domaine, paramètres requis et méthodes
   référencées ; ne pas laisser `--check-only` annoncer une config valide
   qui finira ensuite en `KeyError`.
3. Refuser les colonnes CSV dupliquées avant lecture. ARI1,25 sous un
   en-tête unique est bien corrigé R2 ; la nouvelle faille vient d'un
   schéma ambigu, pas d'une absence de garde sur le score lu.

Ces défauts sont reproduits par fixtures courtes, pas par erreurs observées
sur LiDAR ou par résultats A/C invalidés. Leur correction ne demande ni
GCP ni grand chantier. Les reçus précédents restent clos et inchangés.

## Raccord observé et une proposition de rejet par blocs

Le [dernier complément R2](CONTRE_AUDIT_R2_20260930.md) confirme quatre
appels minuscules : la nouvelle CLI tête propage maintenant `Outcome` et
refuse une configuration tardive invalide avant écriture dans la hiérarchie
testée. Elle accepte encore la collision étiquettes/arbre. Le différentiel
Pool est clos 24/24, celui de SiteTree et ses 11 CTests aussi ; ces copies
ne constituent toujours pas un binaire commun qualifié. Le chantier de
fusion est observé en cours, sans relancer ses tests.

Pour le massif, je confirme l'invariant de [l'audit indépendant](../AUDIT_MASSIF_LIDAR_20260930.md).
J'ajoute à sa garde M2 le cas scalaire L=2³²−2 : `lo*64` déborde avant
le minimum et rend L−61. Faire le produit en u64 **avant** clamp/conversion,
pas après ; le milieu sûr seul ne suffit pas.

**Proposition à mesurer, pas optimisation acquise.** Au lieu de rescanner
et matérialiser toute la liste parente pour chaque boîte de centres Q,
transmettre une couverture de blocs du SiteTree global. Pour n≥K, choisir
K sites distincts S, puis calculer, dans une unité commune exacte,
`R_Q² = max_{s∈S,v sommet de Q} ||s−v||²`. Pour tout c∈Q,
`d_K(c)² ≤ R_Q²`. Une boule admise par FULL a p≤K−1, donc son rayon est
au plus d_K(c) ; tous ses intérieurs et sa coquille restent dans ce rayon.
Rejeter un bloc Z seulement si `dist_min(Z,Q)² > R_Q²` : tout son contenu
est alors inutile à cette boîte. **L'égalité n'est jamais un rejet.**
Lorsque n<K, il n'y a pas ces K témoins : conserver la liste complète
ou une autre certification explicite, sans quota caché.

La preuve est générale pour les sites distincts non pondérés ; le
[contrôle Fraction](../../receipts/audit_continu_20260929/r2_integration_block_20260930/scalar/normal.json)
exerce 120 petites configurations, 504 blocs dont 126 rejetés, et 1 272
centres rationnels. Une fixture d'égalité montre causalement pourquoi
`≥` serait faux. Ces contrôles ne prouvent pas le gain ni la complexité.
L'index reste immuable partagé, états et files possédés par tâche ; étendre
les IDs seulement aux feuilles où l'énumération l'exige.

Pour que cette piste gagne réellement, ne pas construire d'abord S par
un nouveau scan de toute la liste. Comparer une requête sur l'index et
la réutilisation de témoins parentaux certifiés, puis le filtre D actuel
sur les blocs résiduels. Avec les mêmes témoins, ce rejet est déjà impliqué
par leurs dominances ponctuelles : l'intérêt visé est de **payer un test
pour un bloc**, pas d'annoncer une nouvelle élimination géométrique.
Mesurer visites de couples Q/Z, sélection des témoins, IDs développés,
listes matérialisées, candidats, sorties et coût aval, en 8k/16k/32k et
sur les coupes capteur. Un tri externe n'en réduit pas le travail.

Enfin relever max_shell et le nombre de coquilles de plus de 24 sites
avant tout palier massif : la tour actuelle les refuse et utilise encore
une énumération combinatoire. Le quotient rapide de TOWER_v2 est un plan,
pas le produit. Un tel chantier n'est prioritaire pour 100 ms que si ces
coquilles coûtent réellement dans les régimes LiDAR concernés.

## Reprise de nos cas frontière dans la porte de conception

Lecture seule à 05:24 UTC dans le chantier `build/v10-frontiere` : G7
reprend exactement les entrées internes K3/K5 de notre section 11 et
G8 dédoublonne deux boules couvrantes d'une même composante. Les témoins
gardent I∪U, le filtre propre à K et les coupes fermées ; les attaches
suivent ensuite l'ascendance. C'est cohérent avec le besoin frontière
relue dans la thèse. Les deux nouvelles fixtures sont explicitement hors
préenregistrement, pas de nouveaux scores du test statistique.

Cette lecture n'est pas un rejeu : sources en évolution, aucune porte
complète nouvelle déclarée acquise par notre audit. Distinguer le PASS
sans mutants du PASS complet. Les bras actuels n'implémentent pas la
durée : l'absence du cas ghost n'est pas leur défaut d'implémentation,
mais devient une garde indispensable **si** ce poids est exploré. G6
doit continuer à exposer le saut inverseβ sous contact de coquille ;
passer ce contre-test signifie comprendre le comportement, pas prouver
sa robustesse statistique.
