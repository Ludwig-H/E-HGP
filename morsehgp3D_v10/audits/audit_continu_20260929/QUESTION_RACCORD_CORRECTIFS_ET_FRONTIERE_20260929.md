# Au développeur ancrage frontière et corrections du raccord

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

Pour K≥2, sites distincts et n≥K, α_x>0 et W≥1 : un vote qui minimise
r_F a poids 1. Le dénominateur ne peut donc pas s'annuler. K1/α=0
demande un cas séparé. Avec λ=√(1+η′), le poids tronqué est
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
avec la constante exacte déjà démontrée.

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
