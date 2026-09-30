# Au développeur ancrage frontière et corrections du raccord

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
la marge au bord de bande. L'uniforme évite ce contre-exemple précis ;
sa robustesse générale reste à établir, notamment lorsque la bande change.
Une stabilité locale demanderait à la fois la correspondance des atomes,
une marge au bord de bande et une marge de majorité supérieure à la
variation totale des masses normalisées. Dédupliquer une même boule peut
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
Ni les sept groupes ni u24/u32 ni G4 ne sont qualifiés ensemble.

**Limite de la nouvelle porte d'arrondi, relue vers 13 h 04 UTC.** Les
quatre filtres de `resolve` sont bien désactivés selon le mode du fil
courant ; DWelzl ne reste qu'une proposition certifiée en exact. Aucun
défaut géométrique nouveau démontré dans ces décisions. Mais la porte
compare le catalogue exact et `OrderForest`, sans appeler
`point_dendrogram` ni `condense`. `ball_nodes` reste désactivé : comparer
deux listes vides ne qualifie pas cette sortie optionnelle. Les valeurs
`level.approx()` du dendrogramme restent des doubles, divisés dans le mode
appelant. Exemple du triangle aigu (0,0,0), (8,4,0), (4,8,0) :
β=204800/9216=200/9, encadré par les doubles exacts
`0x1.638e38e38e38ep+4` et `0x1.638e38e38e38fp+4`. Les modes downward et
upward ne publient donc pas nécessairement le même double. Limiter
« sorties identiques sous les quatre modes » à l'objet exact jugé, ou
tester séparément le dendrogramme/export et définir son environnement
numérique. Cette réserve ne prouve aucun changement de labels/EOM.

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
