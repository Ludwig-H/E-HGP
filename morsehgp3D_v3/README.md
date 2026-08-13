# MorseHGP3D v3

MorseHGP3D v3 explore une construction exacte et industrielle de la hiérarchie
Morse/HGP en dimension trois, sans matérialiser de mosaïque de Delaunay d'ordre
supérieur. Le profil candidat est le nuage quantifié u16; aucune conclusion
n'est étendue au nuage réel antérieur à la quantification. Les preuves Yao-1
supposent en plus des positions distinctes. Une admission exacte exige une
porte régulière suffisante, un quotient reçu des plateaux pertinents ou un
refus explicite `unsupported_degeneracy`; `RelevantGP` seul ne ferme pas toutes
les incidences silencieuses. Cette fermeture du domaine n'est pas encore reçue.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

## Verdict

Le contrat n'est pas rempli. À 50 000 points et `K=10`, sur une famille
volumique favorable dont le certificat reste sparse, le p95
`warm_e2e<100 ms` est la cible principale et `warm_e2e<1 s` la cible
secondaire. Aucun chemin exact actuel n'est qualifié sous l'une ou l'autre;
aucun échantillon SLO ni producteur de `BenchmarkOutputContract-v1` n'existe.

Le verdict lié au `HEAD` et au worktree est tenu uniquement dans
[`AUDIT_ETAT_COURANT.md`](audits/AUDIT_ETAT_COURANT.md). Ne déduire aucun état
live d'une note datée, d'un message de commit ou du seul passage d'un CTest.

Au pin `5113ff2`, la disjonction du masque central et du fallback est
mathématiquement sûre parce que le second remplace seulement un verdict
`MIXED`; leurs populations ne doivent jamais être additionnées. Le juge live
ne rejoue toutefois que le masque central. Surtout, la grille annoncée en `K`
mesure toujours `sum_N=2*residual_pair_mass_q2`, non la fenêtre projective q4 :
deux seuils sur une famille ne reçoivent ni `Theta(K)`, ni indépendance de `s`.
Même une vraie fenêtre d'arêtes `E_4` linéaire ne borne pas le shallow : il faut
encore mesurer `M=sum_(a,b in E_4)m_ab`, la relation factorisée arête ouverte ×
site actif. Réponse directe et directive :
[`AUDIT_REPONSE_RETRACTATION_S2_K_5113FF2_20260813.md`](audits/AUDIT_REPONSE_RETRACTATION_S2_K_5113FF2_20260813.md).

Au pin `f02d5ed`, la « chaîne complète » annoncée est corrigée. Le producteur
par arête est un falsificateur différentiel utile, mais il parcourt encore
`C(n_lens,2)` en q4 et refait le census par support ; sur
`eight_clusters,n=500`, le dépôt reçoit déjà `191538784` paires q4 et `33,53 s`
après mort anticipée. Son `kept(a,b)` est un ensemble de sites ambigus par paire
et un maximum, tandis que l'ancien `sum_N` vaut deux fois la masse q2 ouverte :
les deux scalaires ne sont ni le même objet, ni une preuve `O(n)`. La directive
est désormais : petit pont `BallForm -> BallKey -> RLE -> census I_B/U_B`, vrai
reporter projectif des arêtes maximales, niveaux shallow locaux sur les seules
arêtes ouvertes, puis fold streamé. Le script G4 CPU du pin reste impropre à une
qualification produit. Contre-audit et ordre exact :
[`AUDIT_CONTRE_CHAINE_COMPLETE_ET_G4_736F5BC_20260813.md`](audits/AUDIT_CONTRE_CHAINE_COMPLETE_ET_G4_736F5BC_20260813.md).

Le successeur `3d07be1` rétracte correctement le refus prématuré de `s=2`, mais
sa loi `Theta(Kn)` n'est pas reçue. L'OR de deux certificats `ALL` est sûr ; sa
mesure reste pourtant `2*residual_pair_mass` q2, pas `E_4`, et le fallback
multiplie le temps CPU par `2,8..3,4` pour quelques dizaines de fermetures q3/q4.
La taille d'une fenêtre de crédits est une fonction en escalier du seuil et peut
sauter jusqu'au quadratique sans hypothèse de distribution. `s=2` reste donc une
ablation non réfutée. Réponse mathématique et directive :
[`AUDIT_REPONSE_RETRACTATION_S2_K_3D07BE1_20260813.md`](audits/AUDIT_REPONSE_RETRACTATION_S2_K_3D07BE1_20260813.md).

Au pin `dba8961`, la fourche « source par record ou source par paire » est
tranchée : aucune des deux descriptions n'est le contrat v3. La source doit
être factorisée et sortie-sensible ; elle paie ses couples de nœuds visités,
ses blocs et ses vraies sorties, jamais tous les PairIds d'un bloc. La colonne
alors appelée « masse résiduelle » était seulement le complément du taux de
records q2 fermés. Le successeur `af08b0e` calcule désormais la vraie masse q2, mais
cette masse reste un ledger et non un temps aval. Les données physiques
disponibles dominent `s=4`; `s=1`, `3/2`, `2` et désormais `3` restent des
ablations à départager seulement sur les trois lanes et le vrai reporter
projectif. Aucune séparation n'est figée par la masse q2.

Le micro-jalon central actuel est `Central-VWave`. Pour chaque terminal,
il classifie l'arbre témoin par les extrema exacts du score
`S=Vhi(singleton)` : `S<Dlo`, `3S<Dlo` et
`209S<=56Dlo`. `ALL` crédite une antichaîne, `CENTRAL_DEAD` élague seulement ce
certificat et `MIXED` descend en vagues `count--scan--fill`. Cette construction
est complète pour les crédits du masque central et supprime fenêtre Morton,
top-`L` et heap par rectangle. Son travail `J` peut encore être dense : il est
mesuré avant le join global `QueryTree×PointTree`, les carriers par marges et
les niveaux shallow q4. Réponse, formules, fixtures et ordre d'implémentation :
[`AUDIT_REPONSE_FOURCHE_SOURCE_CENTRAL_VWAVE_DBA8961_20260813.md`](audits/AUDIT_REPONSE_FOURCHE_SOURCE_CENTRAL_VWAVE_DBA8961_20260813.md).

Le pin `dfa9e1b` corrige déjà un faux crédit découvert par l'audit : le masque
des lanes appartient désormais à chaque `CNode`, si bien qu'un parent `ALL` ne
peut plus être recompté dans ses enfants lorsque l'autre lane est `MIXED`.
Cette réparation réduit de `17 %` les records q2 annoncés fermés sur le cas
mesuré. Le successeur `7b58fc3` ajoute un CTest nominal et tue le mutant du
masque global ; le rejeu ciblé rend `18/18` portes CPU en `26,48 s`.
`Central-VWave` reste néanmoins diagnostique : le juge ne certifie pas les IDs
crédités, son plancher n'est pas séparé par lane, et un cap compte seulement
`tronques` sans sérialiser les tâches restantes. Aucun `proof_id`, compactage
résiduel, ABI ou kernel device n'existe. Le prochain jalon est donc le ledger
d'antichaîne et la continuation `count--scan--fill`, pas un nouveau score.

Le pin `75f16db` clôt l'ablation locale. Son mode `--climb` réduit les
classifications de `30,42 M` à `27,65 M` sur
`uniform,n=8000,s=2`, mais il omet la feuille Morton localisée, traite d'abord
les gros frères proches de la racine et ferme moins de masse q2
(`65,22 %` contre `66,43 %`). Trois runs CPU bruités ne donnent qu'environ
`1,1 %` de gain médian. Le mode reste un proposer fail-open ; il ne justifie
plus d'optimiser une recherche indépendante par rectangle. Le prochain levier
est le partage par ancre/spans, pas une autre pile.

Cette vague ne ferme toutefois que le préfixe central. La réponse source plus
forte n'est pas un join développé de carriers : des groupes projectifs
disjoints ferment des paires entières. Pour se raccorder à l'owner déjà reçu,
on oriente chaque paire `a<b` et note `E_q(a)` les seconds endpoints non fermés.
L'invariant exact est que l'arête maximale canonique de tout vrai support reste
dans cette fenêtre ; ses autres sommets sont générés ensuite par la lentille.
Le prochain falsificateur source est donc
`PWC0-A/MaxEdgeSuffixReporter-q4-v0`, qui mesure `sum_a |E_4(a)|`, tâches,
octets et HWM. S'il passe, `EdgeActiveFormCounter-v0` mesure ensuite
`M=sum m_ab`, tâches arête×site, octets et HWM sans développer le produit. Deux
verts seulement autorisent l'arrangement shallow **par arête ouverte**, RLE des centres par `BallKey`, census
global une fois par boule, puis expansion tardive des `SupportKey`. Tous les
points restent obligatoires au census. Cette ordonnance évite tout join
`PairId×carrier` et toute mosaïque globale d'ordre supérieur :
[`AUDIT_REPONSE_FOURCHE_SOURCE_AF08B0E_20260813.md`](audits/AUDIT_REPONSE_FOURCHE_SOURCE_AF08B0E_20260813.md).

Un triple projectif plein rang ferme désormais un `BNode` par un test uniforme
exact : trois formes coniques faibles et
`F(d)=|Delta|*||d||^2-p dot d>0`, dont le minimum entier est séparable sur la
boîte. Les trois H2 membre par membre restent un fast path `i64` sûr mais
incomplet ; `F` demande environ 87 bits et reste l'ablation large. Le P0 du
compteur commence donc par les suffixes `i64` des 48 chambres, puis compare le
triple exact et les 432 sous-cellules. Directive et fixtures :
[`AUDIT_DIRECTIVE_BNODE_PROJECTIF_ET_ARRET_CLIMB_75F16DB_20260813.md`](audits/AUDIT_DIRECTIVE_BNODE_PROJECTIF_ET_ARRET_CLIMB_75F16DB_20260813.md).

Le parent `32589ad` n'est pas encore ce compteur : son `sum_N` vaut
identiquement deux fois la masse PairId q2 centrale résiduelle, avec les deux
orientations. Il n'a ni crédits projectifs, ni owner d'arête, ni q3/q4 ; son
calcul est hors chrono et sa pente n'entre pas dans la gate. Les annonces
« `s=2` refusé » et « `s=3` vert » sont donc rétractées. Le `HEAD=a5c8251`
reçoit séparément les marges carrier : `ALL` exact sur le produit AABB,
`NONE` sûr mais incomplet. Le vrai reporter, la définition de `E_q(a)` et la
réponse 48/432 sont dans
[`AUDIT_CONTRE_COMPTEUR_FENETRE_32589AD_20260813.md`](audits/AUDIT_CONTRE_COMPTEUR_FENETRE_32589AD_20260813.md).

La directive d'implémentation est maintenant unique : écrire d'abord
`PWC0-A/MaxEdgeSuffixReporter-q4-v0`, pas un nouveau scan de carriers. Il
conserve les vrais `PointId`, l'orientation canonique, les preuves et les
continuations, mesure les spans ouverts q4 sur 48 chambres, puis raffine
uniquement chaque chambre ouverte dans ses neuf sous-cellules. Une banque
bornée reste propositionnelle : un résultat dense à `P=96` refuse cette
configuration, pas tous les certificats. Si `sum_a|E_4(a)|`, les tâches ou les
octets restent rouges après ablation de `P`, la route s'arrête avant le shallow.
Si la fenêtre est sparse mais les `n` graines racine dominent, `PWC0-B`
universalise sur `ANode×BNode`. Un vert autorise le shallow q4 puis q3/q2 ; il
ne qualifie encore aucun SLO.

Au pin `96be8e0`, le nouveau front apporte deux briques utiles sans changer ce
verdict. L'intervalle entier de `H=(z-a) dot(b-z)` sur trois AABB certifie
`ALL/NONE/MIXED` pour q2 et des `ALL` suffisants pour q3/q4. Arrêter la
récursion `A×B` dès la bonne séparation peut en outre produire un front WSPD de
cardinal `O(s^3 n)` à séparation fixe, sous split-tree canonique et politique
des doublons. Cette borne porte sur les records du front, pas sur la descente
témoin, la source résiduelle ni l'aval.

La nouvelle conclusion `cred+pending<h_q` est reçue comme support pour q2 sous
endpoints distincts. Pour q3/q4, elle signifie seulement
`KEEP_ANCHOR/DELEGATED_TO_SOURCE` : un nuage collinéaire peut satisfaire ce
majorant sans contenir aucun triangle ou tétraèdre propre. Le probe repart
encore de la racine témoin pour chaque rectangle. Les CTests du pin rendaient
`1/4`; le successeur `c77227c` répare leur câblage et un rejeu frais rend
`5/5`, mais ni la gate de pente ni le script G4 n'activent le mode WSPD. Le
prochain jalon recommandé partage un seul front et un masque de lanes, propose
en lot une petite banque de points proches du milieu de chaque rectangle,
recertifie chaque `PointId` exactement, puis conserve une continuation
persistante pour les seuls résidus. Preuves, mesures et gates :
[`AUDIT_REPONSE_WSPD_DESCENTE_JOINTE_96BE8E0_20260813.md`](audits/AUDIT_REPONSE_WSPD_DESCENTE_JOINTE_96BE8E0_20260813.md).

Les successeurs appliquent déjà `KEEP_ANCHOR`, le `NONE` propre à chaque lane,
un enum de CLI fermé et le budget exact. Au pin `a7f061b`, le cœur entier
`Dlo/Vhi` partagé fait passer la couverture diagnostique `terrain/8k` à
`87,27 %` en q3 et `83,93 %` en q4. Le parent logiciel `62cea17` corrige le
faux juge du cœur et reçoit une première partition WSPD CPU ; un rejeu Release
au `HEAD=90aa941` rend `14/14` CTests ciblés en `28,44 s`. Ce vert reste
diagnostique : le juge `ALL` échantillonne quatre triplets par nœud, le WSPD
repart de `C=root` par terminal, et aucun `RectKey`, owner ou consommateur exact
du résiduel n'est reçu.

La route longue proposée ne prolonge plus cette file diagnostique. Elle construit
d'abord une WSPD entière/canonique à faible séparation, puis classifie une seule
fois ses terminaux avec un masque commun. Deux fast paths entiers — cœur central
`Dlo/Vhi` et corridor d'ordre unimodulaire — ferment les rectangles denses ;
tout le reste devient un vrai front de carriers. Le raccord q3 est caractérisé
par `H<0` et les deux inégalités d'arête maximale ; q4 conserve toute la
lentille car un seul de ses deux porteurs doit être aigu. Le jalon
`WspdFrontLowerBound-v1` peut ainsi réfuter la seconde avant d'écrire census et
fold, sans jamais transformer un cap en résultat :
[`AUDIT_DEBLOCAGE_WSPD_PREFIX_CARRIERS_20260813.md`](audits/AUDIT_DEBLOCAGE_WSPD_PREFIX_CARRIERS_20260813.md).

La fenêtre Morton initialement proposée pour `RF-GPU-P0` reste une ablation
positive bornée, pas le prochain chemin produit. Elle inspecte `W=32`, garde au
plus `L=16` `PointId` distincts et délègue tout échec. Aucun top-`L` exact
n'était requis pour sa sûreté, mais les mesures et la descente qui lui a
succédé ne reçoivent ni son rappel ni un coût device. `Central-VWave` la
remplace comme baseline scientifique ; la fenêtre peut encore comparer son
temps et sa couverture au même compactage. La preuve historique du double
cœur, les fixtures et l'ABI positive restent dans :
[`AUDIT_REPONSE_CLAUDE_DOUBLE_COEUR_RF_GPU_P0_A7F061B_20260813.md`](audits/AUDIT_REPONSE_CLAUDE_DOUBLE_COEUR_RF_GPU_P0_A7F061B_20260813.md).

La première banque du pin `360ea7c` confirmait la baisse de constante à
`n=2000` (`2,607 s` vers `0,109 s` local), mais son encodeur 2D faisait
collisionner `(2,0,0)` et `(0,0,1)`. Le successeur `4f4b463` répare Morton3D.
Restent à recevoir : une porte dédiée contre l'oracle bit à bit, la fenêtre
haute recadrée, zéro allocation par rectangle, la mutualisation réelle de
`Dlo/Vhi`, les preuves sérialisées et le compactage. Une banque rapide sans
signal spatial ne débloque pas la source.

La réponse à la mesure `--bank-strong` est un repli q2 ciblé : pour chaque ID,
`Hmin_singleton>0` est le test q2-ALL exact sur `box(A)×box(B)` et ne coûte que
douze produits `i64`. C'est un certificat sûr, mais incomplet sur les ensembles
de points corrélés. Le bit central q2 est inclus dans `Hmin>0` : après le masque
commun, ce repli remplace la disjonction générale en q2. Il conserve le rappel
q2 annoncé (`31,37 %` à `s=2`) sans
les trois classifieurs ni les carrés larges ; q3/q4 restent centraux. `s=8`
n'est pas retenu globalement : il multiplie le front mesuré par `11,8`. Le
choix ne monte pas globalement au-delà de `s=2`; `s=1`, `3/2` et `2` sont
comparés sur `P0 + source + aval`, pas sur la seule masse fermée. La réponse
complète, y compris `Vbest`, l'héritage
des preuves et le traitement des endpoints relatifs, est dans
[`AUDIT_REPONSE_BANQUE_MORTON_360EA7C_20260813.md`](audits/AUDIT_REPONSE_BANQUE_MORTON_360EA7C_20260813.md).

Le pin `4f4b463` réparait l'entrelacement Morton3D, mais sa banque P0 n'a pas
été reçue. La suite ne doit pas recréer une source indépendante. Avec
`D=||b-a||^2`,
`V=||2z-a-b||^2` et `T=(b-a) dot(2z-a-b)`, un même calcul classe les témoins
centraux, la lentille et les carriers aigus. Le jalon recommandé est désormais
`Central-VWave`, puis une `DVT-CWave` factorisée, sans redémarrage racine.
q4 consomme la relation factorisée `Acute×Lens`, jamais toutes les paires de
la lentille. ABI, bornes et fixtures :
[`AUDIT_DIRECTIVE_DVT_CWAVE_4F4B463_20260813.md`](audits/AUDIT_DIRECTIVE_DVT_CWAVE_4F4B463_20260813.md).

Au pin live `81d24d0`, les mutants omission/doublon et le refus `leaf>1`
renforcent le ledger, mais FNV-64 reste un digest et ne peut servir d'identité
scientifique. `NodeKey` doit être injectif dans un arbre pincé et `RectId` la
paire ordonnée de deux telles clés. Surtout, aucune paire nue ne peut établir
`owner=max_edge_canonical` : cette propriété dépend des autres arêtes du
support. P0 émet donc `CLOSED_PAIR_SHARD` en q2 et un
`PRUNED_OWNER_SHARD` conditionnel en q3/q4 ; la source exacte partitionnée par
owner saute ensuite ce shard sans développer ses supports. La preuve et l'ABI
sont dans
[`AUDIT_REPONSE_OWNER_SHARD_P0_81D24D0_20260813.md`](audits/AUDIT_REPONSE_OWNER_SHARD_P0_81D24D0_20260813.md).
Le rejeu Release ciblé frais rend `18/18` CTests en `12,36 s`; ce vert reçoit
les portes CPU bornées, pas le microkernel ni le handoff owner-shard.

La seule rampe WSPD rouge est `scanline_single_pass,s=4`, pas la famille
d'amas. Elle a été générée à `coord=65535` fixe, alors que l'emprise canonique
scanline varie comme `sqrt(40n)` ; chaque taille est un préfixe de lignes, pas
une scène homothétique. Le refus fini reste valable, mais sa cause ne l'est
pas. Le rejeu doit pincer une emprise par taille, `m==n`, plusieurs graines,
bbox/lignes/profondeur/splits, temps et HWM. Le tape garde une séparation
L-infini explicitement nommée `s_inf`; une séparation euclidienne entière est
une ablation distincte, pas une substitution silencieuse.

Le script G4 du `HEAD=21a7a63` ne doit pas être lancé pour qualifier cette
tranche. Sa rampe principale passe maintenant correctement
`--budget-depth=4 --core`, mais son étape WSPD mesure quinze gros runs du probe
CPU, pas le kernel P0, masque leurs codes par `wait || true` et ne publie ni
temps, ni octets, ni HWM. Son sweep budget lit des fichiers différents de ceux
qu'il écrit et le sweep feuille emploie encore `--budget=24`; la cible CUDA
construite reste une autre qualification. Ce script ne peut donc ni recevoir
la banque, ni répondre au contrat d'une seconde.

Le producteur expérimental par arête maximale apporte quatre lemmes exacts :
borne mono-ancre `ext/4`, face positive adjacente d'un q4 positif, disque q4
mutualisable pour le filtre q3 et identité entière du shell diamétral q2. Son
owner choisit exactement une occurrence de chaque support propre, mais pas une
activation unique par `BallKey`. Il n'est pas reçu : son différentiel partage
la géométrie du sujet, son ABI ne transporte pas encore `I_B/U_B`, et sa boucle
q4 forme toujours toutes les paires de la lentille. Le contre-audit et la route
de remplacement par niveaux mono-ancre sont dans
[`AUDIT_CONTRE_AUDIT_PRODUCTEUR_ANCRE_LENTILLE_AIGUE_20260812.md`](audits/AUDIT_CONTRE_AUDIT_PRODUCTEUR_ANCRE_LENTILLE_AIGUE_20260812.md).
La reprise la plus récente simplifie encore ce chemin : le filtre global
`theta` est redondant sur tout domaine vivant, tandis qu'une cutting signée
locale peut tuer des patches ; les concurrences se traitent par bundles
pondérés et dominance exacte avant toute microtuile. Voir
[`AUDIT_REPONSES_CLAUDE_CHAMBRES_NIVEAUX_CUTTING_20260812.md`](audits/AUDIT_REPONSES_CLAUDE_CHAMBRES_NIVEAUX_CUTTING_20260812.md).
La rampe amas suivante révèle deux murs conjoints, boucle q4 et census, mais
ses colonnes de front ne sont pas reproductibles. La réparation mathématique
proposée est un classifieur des huit coins AABB dans le spindle complet **avant
la liste**, relevé ensuite par un certificat entier fail-open sur
`A_endpoint times B_partner times C_witness`. À endpoint/témoin fixes, le
domaine partenaire est exactement un cône convexe : huit coins suffisent sans
trigonométrie. Vient ensuite le replay local des conflits de la cutting. Voir
[`AUDIT_REPONSES_MUR_AMAS_CENSUS_SPINDLE_20260812.md`](audits/AUDIT_REPONSES_MUR_AMAS_CENSUS_SPINDLE_20260812.md).

Le commit `3d4c598`, ancêtre du `HEAD` courant, implémente cette primitive
ponctuelle et passe ses
`30/30` portes ciblées, mais il n'est pas reçu : un `smax` hors largeur ferme
faussement toute la masse avec l'accord du juge, les décisions q2/q3/q4 ne sont
pas jugées séparément, la cardinalité demandée peut être réduite silencieusement
et les rampes banques 48/96 gardent deux pentes rouges. Le résiduel sous cap
n'est pas rejouable. À ce pin, l'ABI CUDA anchor était incohérente avec
`density_guard`; `24cc1a2` a depuis supprimé cette garde du chemin partagé,
sans pour autant recevoir un producteur device ou le SLO.
Le successeur logiciel `519ddfb` ferme ces quatre dettes locales : son nouvel
ELF Release, CUDA désactivé, rend `39/39` portes ; `smax` et cardinalité sont
refusés avant calcul, les lanes sont jugées séparément et le mutant d'héritage
est porté. Cela reçoit le juge ponctuel, pas la DFS industrielle : caps,
résiduel, pentes, CUDA et payload restent ouverts. Le pin et le transcript local
sont tenus dans
[`AUDIT_ETAT_COURANT.md`](audits/AUDIT_ETAT_COURANT.md).
La DFS par endpoint reste donc un oracle borné. Deux reprises peuvent partager
le résiduel : un fast path de fenêtre k-NN certifiée pour les supports locaux,
puis un domaine collectif `A_endpoint times B_partner times C_witness` ou
directionnel complet pour tout ce qui n'est pas certifié, avant toute mesure
G4. Voir
[`AUDIT_CONTRE_AUDIT_SPINDLE_CONE_WORKTREE_20260813.md`](audits/AUDIT_CONTRE_AUDIT_SPINDLE_CONE_WORKTREE_20260813.md).

## Contrat visé

Le contrat courant est `n=50 000`. L'objectif industriel qui le suit est de
traiter sur G4 des nuages de **dizaines de millions de points**. Ce second
horizon n'est pas une extrapolation du premier : il élimine dès aujourd'hui des
routes qui sembleraient acceptables à 50 000. Une ordonnance en `n^{1,8}` y est
absurde de plusieurs ordres de grandeur, et toute structure résidente
proportionnelle à la sortie totale — environ `480` supports par point, donc
quelques milliards à `10^7` — y devient impossible. Le `DensePointIndex:u16`
est de même une limite d'implémentation, pas de mathématiques, et doit être
nommée comme telle. Les décisions d'architecture se jugent donc aux deux
horizons ; la route proposée et son séquencement sont dans
[`NOTE_CLAUDE_ROUTE_G4_50K_PUIS_10M_20260813.md`](audits/NOTE_CLAUDE_ROUTE_G4_50K_PUIS_10M_20260813.md).
Son contre-audit accepte `4R^2<delta_out^2` comme certificat suffisant d'une
sous-source localement complète, mais refuse trois raccourcis : Source S ne
borne pas `|U_B|`, les seuls candidats locaux refusés ne couvrent pas les
supports jamais proposés, et un halo k-NN ne rend pas les sous-nuages
indépendants. La réponse aux cinq questions, le protocole de fold global et le
ledger mémoire sont dans
[`AUDIT_REPONSES_ROUTE_G4_50K_PUIS_10M_20260813.md`](audits/AUDIT_REPONSES_ROUTE_G4_50K_PUIS_10M_20260813.md).
Le complément collectif évite de choisir prématurément entre fenêtre et
`A×B×C` : les 432 sous-cônes rationnels donnent des cutoffs q4/q3 décidables en
entiers, puis top-h et report du seul résiduel deviennent des requêtes de
dominance ; des groupes coniques de trois témoins ciblent le mur des amas. Une
gate `counter-only` doit comparer cette voie à un cœur WSPD et au relation-tree
sur le même univers et le même schéma de ledger avant tout port CUDA ; les
bitsets peuvent différer selon la force des certificats. Voir
[`AUDIT_DEBLOCAGE_COLLECTIF_APRES_FENETRE_20260813.md`](audits/AUDIT_DEBLOCAGE_COLLECTIF_APRES_FENETRE_20260813.md).
Le successeur `ffe5b69` raccorde le probe de fenêtre et rend `21/21` portes
ciblées. Il reçoit l'ensemble des `SupportKey` certifiables sur quatre petits
nuages et mesure de nombreux supports globaux jamais proposés, mais pas encore
les identités complètes : le juge partage le constructeur top-M/coupure, compare
les comptes plutôt que les membres `I_B/U_B`, ne construit aucune `BallKey` et
juge après déduplication des ancres. Il reste une baseline/oracle bornée, jamais
une mesure de la source produit. Voir l'audit historique puis sa réception
successeur :
[`AUDIT_WORKTREE_WINDOW_SOURCE_20260813.md`](audits/AUDIT_WORKTREE_WINDOW_SOURCE_20260813.md) et
[`AUDIT_SUCCESSEUR_WINDOW_SOURCE_FFE5B69_20260813.md`](audits/AUDIT_SUCCESSEUR_WINDOW_SOURCE_FFE5B69_20260813.md).

Claude a ensuite demandé quelles propriétés propres à la dimension trois
pouvaient réellement débloquer cette gate. Les réponses ferment sept raccourcis :
une chambre peut déjà porter treize q2 propres sans plateau, le groupe
octaédrique partage le code mais pas un tri absolu, le lift 4D reste seulement
un backend de requête, et un cœur entre amas ne ferme un bloc qu'après le compte
strict de huit ou neuf IDs. En revanche, le cutoff par sous-cône se réduit
exactement à neuf paires de rayons et peut fermer un intervalle de hauteurs.
Voir
[`AUDIT_REPONSES_CLAUDE_GEOMETRIE_3D_20260813.md`](audits/AUDIT_REPONSES_CLAUDE_GEOMETRIE_3D_20260813.md).

Le premier probe dominance 432 confirme que cette primitive mord, mais pas
encore avec une ordonnance admissible : il balaie toujours toutes les paires,
matérialise trois bitsets quadratiques et ses séries mêlent deux cutoffs. Son
rejeu ciblé rend `15/15`, mais `smax=34` produit un faux prune parce que les
seuils `10/9/8` restent figés ; le mutant cible--témoin ajoute en réalité une
hauteur zéro hors du préfixe valide. Ce probe demeure un diagnostic CPU borné,
pas la route 50 k/G4. Voir la note Claude et son contre-audit :
[`NOTE_CLAUDE_DOMINANCE_432_MESURES_20260813.md`](audits/NOTE_CLAUDE_DOMINANCE_432_MESURES_20260813.md) et
[`AUDIT_CONTRE_DOMINANCE_432_5DDF4A3_20260813.md`](audits/AUDIT_CONTRE_DOMINANCE_432_5DDF4A3_20260813.md).

Le successeur `2270077` raccorde le microprobe de groupes coniques et rend
`11/11` portes ciblées. Il reçoit le théorème pour un triple fourni, pas la
seconde voie collective : `smax` est ignoré, le juge partage H2 et ne suit pas
les seuls IDs crédités, le packing est incomplet et le probe reste
`O(n^3 log n)`. La proposition de déblocage exploite la dimension trois sans
`C(m,3)` : un crédit peut avoir plus de trois membres. Par cellule, les témoins
s'activent à un seuil entier de hauteur ; leur enveloppe convexe projective 2D
fournit un crédit de taille au plus neuf couvrant la cellule entière. Huit à dix
crédits disjoints ferment alors directement un suffixe de nœuds cibles. Le tri
angulaire transverse et les régions polyédriques restent un raffinement lorsque
la cellule entière échoue. Tout échec est fail-open et factorisé. Voir le
contre-audit et la construction remise à Claude :
[`AUDIT_CONTRE_GROUPES_CONIQUES_2270077_20260813.md`](audits/AUDIT_CONTRE_GROUPES_CONIQUES_2270077_20260813.md) et
[`AUDIT_REPONSE_DOMINANCE_GROUPES_5DDF4A3_20260813.md`](audits/AUDIT_REPONSE_DOMINANCE_GROUPES_5DDF4A3_20260813.md).

Le successeur `ec2fbab` commet aussi le microprobe de cœur commun et la note
Claude qui compare les trois voies. Cette comparaison reste diagnostique : les
tailles `12 500/150/600` et les univers diffèrent, aucune pente physique n'est
publiée. Le cœur n'est pas abandonné mathématiquement, mais rétrogradé en fast
path positif optionnel d'un rectangle déjà visité ; il ne justifie ni WSPD ni
index propre. Le prochain jalon recommandé est la dominance bloc d'abord puis
les crédits coniques cellulaires par enveloppes 2D, avec le cœur seulement si
une occupation plausible est disponible sans scan supplémentaire. La note, le
contre-audit du probe et la réponse sont :
[`NOTE_CLAUDE_GATE_TROIS_VOIES_20260813.md`](audits/NOTE_CLAUDE_GATE_TROIS_VOIES_20260813.md),
[`AUDIT_WORKTREE_COEUR_COMMUN_20260813.md`](audits/AUDIT_WORKTREE_COEUR_COMMUN_20260813.md) et
[`AUDIT_REPONSE_GATE_TROIS_VOIES_20260813.md`](audits/AUDIT_REPONSE_GATE_TROIS_VOIES_20260813.md).

Le successeur `d3329fe` retire correctement deux faux mutants, le retrait de
deux occupants et l'identité scalaire insuffisante. Il reste un diagnostic :
`smax=34` ferme encore avec un seuil q4 figé à huit et l'accord du juge, le bord
inclus est une transformation sound, l'arrondi supérieur unsafe n'a pas sa
fixture et quatre matrices byte `n*n` resteraient environ `10 GB` à 50 k.

Claude a ensuite ouvert le premier probe de crédits cellulaires. Son événement
H2 et le certificat par les trois rayons sont admis, mais le snapshot initial
est vacueux : un pool de `16` IDs ne peut contenir huit crédits 3D disjoints,
qui exigent au moins `24` IDs. `smax` reste figé, le « juge » ne juge que les
témoins ponctuels et le ledger reboucle sur toutes les paires. Le premier raccord
rend `8/8` portes, toutes sans fermeture ; doubler le pool de `16` à `32` multiplie les tests coniques
de `43,96 M` à `350,27 M` sans fermer une q4. Le contre-audit et l'ordre de
réparation sont dans
[`AUDIT_WORKTREE_CREDITS_CELLULAIRES_20260813.md`](audits/AUDIT_WORKTREE_CREDITS_CELLULAIRES_20260813.md).
Le pin logiciel `01a3a3f`, inchangé par le successeur documentaire `88eb36d`,
remplace les triples par Jarvis mais reste P0 rouge : `h=2`, duplicats, cycle de
pivot et tie-break débordant permettent notamment huit faux crédits q4 face à
une sphère sans intérieur. Le successeur `090f752` commet Andrew et corrige ce
noyau sur son domaine borné : `37 752/37 752` accords, `471` couvertures, quatre fixtures et trois
contradictions tueuses ; un checker indépendant ajoute `1 533` cas avec
`fp=fn=bad_carrier=bad_id=0`. `smax` pilote aussi enfin `h=smax+1-q`. Ce vert
local reste à rendre transactionnel sur `false` — union et compteurs de rang ne
doivent être fusionnés qu'après couverture des trois rayons — et à raccorder : la fixture
« un seul rayon » live bifurque encore vers l'ancien oracle au lieu d'exercer
`cell_covered(rays_one)`, et les anciens planchers de rang deux ne décrivent
plus le ledger d'un crédit cellulaire complet.
La question de Claude sur les trois mutants survivants est close : une
différence de marge seule est une ablation, pas un mutant tué. Quatre fixtures
entières et leurs sphères fautives sont données dans
[`AUDIT_REPONSE_CLAUDE_ENVELOPPE_PROJECTIVE_20260813.md`](audits/AUDIT_REPONSE_CLAUDE_ENVELOPPE_PROJECTIVE_20260813.md),
en réponse à
[`NOTE_CLAUDE_ENVELOPPE_PROJECTIVE_20260813.md`](audits/NOTE_CLAUDE_ENVELOPPE_PROJECTIVE_20260813.md).
Le progrès local ne suffit pas encore pour 50 k : le live retrie le hull à
chaque préfixe et conserve un pire cas cubique, puis reboucle sur toutes les
cibles. La prochaine primitive partage l'ordre projectif, conserve les
duplicats en piles `(X,PointId)`, rend des `CreditKey` rejouables et confie le
suffixe à un `CellSuffixReporter` ancre-feuille × LBVH cible. Celui-ci émet les
nœuds `ALL` maximaux et garde un front `MIXED` authentifié, sans `PairId`.
Pour éviter de retomber à une tâche par ancre, une ancre de bloc peut proposer
un carrier d'au plus neuf IDs, puis les huit coins de l'AABB le recertifient :
les déterminants coniques sont affines en l'ancre et la marge H2 est concave.
Le représentant propose, les coins font autorité ; l'échec scinde le bloc.
Chaque ID proposé peut ensuite classifier exactement un rectangle par
`L_z(A,B)=sum_i min_{a_i,b_i}(z_i-a_i)(b_i-z_i)>0`. Le prochain jalon positif
est donc Andrew à la feuille, ce reporter de suffixes, puis sorties
`RectKey/BankKey/CreditKey` et ce test H2 bloc — pas une nouvelle boucle de
mesure `n(n-1)`.

La nouvelle question de Claude ferme ici un vrai étage q2. Pour trois AABB
`A/B/C`, la somme des minima des huit produits
`(z_i-a_i)(b_i-z_i)` par coordonnée est le minimum continu exact sur
`A times B times C`. Sa stricte positivité crédite tous les IDs d'un nœud `C`
pour toutes les boules diamétrales de `A times B`; une antichaîne de nœuds `C`
disjoints se somme jusqu'à `h_2=smax-1`. L'échec reste `MIXED`, car les coins
fictifs d'une AABB peuvent masquer un crédit discret. Ce reçu
`DIAMETRAL_BOX_CREDIT` ferme q2 seulement : q3/q4 conservent leurs crédits
spindle. L'identité `RectId` reste canonique par `TreeDigest/ANodeKey/BNodeKey`,
mais l'ordonnance peut choisir entre ses enfants canoniques à l'aide d'un score
entier `L_z`, avec tie-break fixe et visites comptées.

Sur la contre-famille `125 times 200`, quatre crédits logiques de dix IDs
ferment `624 990 000/625 000 000` paires q2. Un raisonnement orthogonal affine
le front à exactement `55` supports q2 croisés, sous forme d'un escalier de dix
rectangles ; les `499 945` supports et l'absence de positifs q3/q4 cités dans
une note appartiennent à l'autre famille, à deux droites, et ne se transfèrent
pas. Preuve, ABI, fixtures et politique de split :
[`AUDIT_REPONSE_CLAUDE_LZ_RECTANGLE_20260813.md`](audits/AUDIT_REPONSE_CLAUDE_LZ_RECTANGLE_20260813.md).

Le diagnostic nœud--nœud suivant confirme le levier mais pas encore le front :
`16 466` appels à `Lambda`, soit jusqu'à `395 184` produits entiers, ferment
`600 M` paires q2 sur trente-deux tranches logiques. Ces tranches ne sont pas
encore des `NodeKey` du même arbre, et les lignes q3/q4 n'ont pas de source
reproductible dans le reçu. La masse résiduelle reste publiée pour la
conservation et le taux de compression. Elle ne devient non bloquante qu'après
réception transitive d'une source et d'un aval dont le coût ne dépend pas de
cette masse. La cible est `W<=c_I I_n+c_F F+c_K K`, avec `F` records et `K`
sorties réellement matérialisées ; `O(n)` par rectangle reste quadratique. Les
gates physiques portent sur records, multiplicité de visite des points/nœuds,
travail réel de la source, sorties, octets et HWM, puis sur le raccord
`SupportKey -> BallKey -> census -> fold`. Aujourd'hui cette indépendance n'est
pas reçue : GO pour construire le walking skeleton, NO-GO produit/G4.

Claude a demandé la famille capable de réfuter la parcimonie de cette voie ;
elle existe en u16. Deux grilles parallèles de `m` points, à `x=0` et
`x=60000` avec un décalage transverse borné, placent toutes les relations
croisées dans `U00` aux hauteurs égales `tau_d=tau_h=60000`. Le garde direct
vaut alors `9 tau_d-11 tau_h<0` dans les deux orientations : les trois lanes
laissent exactement `m^2=n^2/4` `PairId` croisés au résiduel. Le même argument
d'activation donne `X_s>=60001`, donc aucun crédit uniforme n'est actif à la
hauteur de ces cibles. La dominance et les crédits cellulaires ne peuvent plus
être présentés comme sparsifieur terminal distribution-indépendant.

Cette réfutation ne condamne pas le front factorisé : la masse quadratique est
la relation unique `A x B`. La gate distingue désormais la masse sémantique,
qui peut rester quadratique, des compteurs physiques `node_visits`,
`front_records`, octets, copies et high-water, dont deux pentes doivent rester
au plus `1,35`. Le prochain reporter reçoit tous les seuils `X[432][3]` dans
une seule DFS par ancre, sans reprise de racine par cellule, puis lève les
`StarKey` en `RectKey`. Un juge borné seul développe les rectangles. Ce front
est raccordé dans le même jalon à une tranche q4 régulière
`SupportKey -> BallKey -> census -> fold` ; ni le filtre seul, ni tout l'aval
sur des stubs ne constitue le prochain jalon.

Le résultat classique de Chazelle donne en outre `Omega(n^2)` vraies arêtes
Gabriel dans `R^3`, mais sa réalisation `50 k` u16 n'est pas reçue. Une famille
u16 à deux droites montre la distinction nécessaire : le résiduel des
certificats universels q3/q4 vaut au moins `n^2/4`, alors qu'il n'existe aucun
support positif q3/q4 et que Source S vaut `499 945` à `50 k`. `U_q` mesure
donc l'impuissance de cette classe de certificats ; le plancher littéral `L_q`
compte les `PairId` owners d'un support positif pertinent. Une borne
quadratique sur `L_q` signifie catalogue de supports dense, donc traitement
output-sensitive ou `resource_exhausted`, pas victoire automatique d'une
source générative. Elle ne borne pas sans preuve séparée un quotient H0 direct.
Verdict, coordonnées et ordre des jalons :
[`AUDIT_REPONSE_CLAUDE_TUER_LA_VOIE_20260813.md`](audits/AUDIT_REPONSE_CLAUDE_TUER_LA_VOIE_20260813.md),
en réponse à
[`QUESTIONS_CLAUDE_TUER_LA_VOIE_20260813.md`](audits/QUESTIONS_CLAUDE_TUER_LA_VOIE_20260813.md).

Deux sorties sont distinctes :

| sortie | contenu | portée actuelle |
| --- | --- | --- |
| Gamma exhaustif enregistré | facettes, cofaces, incidences silencieuses, lots, `coverage_log` et ses `coverage_delta`, et verticales | oracle borné; l'implémentation exhaustive actuelle n'est pas une route 50 k |
| `hgp_reduced_normalized_h0_v3` | composantes horizontales exactes, niveaux exacts et unions des `PointId`, après quotient certifié des blocs H0 inertes | candidat non reçu et non revendiqué publiquement |

Une boule H0-inerte peut porter de vraies incidences Gamma. Une tombstone du
quotient horizontal ne prouve ni l'absence d'un support, ni l'absence d'une
incidence, ni une application verticale. Les verticales sont hors du contrat
horizontal et demandent leur propre spécification.

Le SLO officiel de la section 14.4 du
[`TEST_PLAN_MORSEHGP3D.md`](../docs/TEST_PLAN_MORSEHGP3D.md) porte sur
`BenchmarkOutputContract-v1` : dix forêts, applications verticales, lots et
certificat minimal sont matérialisés avant la fin de `warm_e2e`. Une mesure du
seul payload horizontal v3 appartient donc à une série diagnostique distincte;
même sous une seconde, elle ne ferme pas ce SLO.

## Faits établis

- À `k=1`, les partitions strictes et fermées sont celles du single linkage.
  Tout EMST est contenu dans le Gabriel fermé de rang deux, donc q2 profond est
  mathématiquement inutile à cet ordre. Énumérer Gabriel resterait toutefois
  potentiellement quadratique en 3D.
  Sur le profil initial à positions 3D deux à deux distinctes, les plus proches
  voisins exacts dans les 48 chambres Yao forment un graphe de taille au plus
  `48n` qui contient un EMST; la réduction sparse évite tout catalogue Morse
  d'ordre supérieur. Une future politique `duplicate_policy=aggregate`
  applique Yao-1 aux sites agrégés distincts; elle ne conserve pas une étoile
  nulle de `PointId` dans ce graphe.
- Pour une boule avec `p` points strictement intérieurs et un support propre
  positif de taille `q`, les ordres `1<=k<=p+q-2` sont des continuations H0
  sans fusion ni nouveau `PointId`.
- À `K=10`, les seuils de témoins des supports q2/q3/q4 sont `10/9/8`. Cette
  preuve autorise seulement une tombstone horizontale avec resolver latent.
- Pour une ancre q4 admise et `c` témoins toujours intérieurs, les centres
  géométriques distincts du sous-arrangement de `m` lignes carriers à profondeur
  au plus `k=7-c` sont en nombre `O(m(k+1))`, avec la borne explicite
  `<e(k+1)m` pour `k>=1`. Cette borne fixe la boucle locale q4, pas la masse des
  ancres, le census, ni le développement d'une cosphère lourde.
- `q_min` est la plus petite arité de provenance Morse prouvée. `q_cert` est le
  maximum des arités effectivement exhibées et rejouées pour la même boule,
  sans preuve d'absence d'un support plus grand.
- Le fast principal d'un lot multiple exige `q<=k+1`, une vraie
  `CarrierClosure` et des carriers stricts résolus dans le snapshot pré-lot.
  `q>k+1` reste au fallback.
- `prefix-all` est exact relativement à la `GeneratorTable` fournie; il ne
  prouve jamais que cette table est géométriquement complète.

Les contre-fixtures exactes du dépôt réfutent le K-graphe de Gabriel brut du
manuscrit : une coface non-Gabriel peut installer silencieusement une facette
réutilisée plus tard. La conclusion se répare localement par `G_k^+`, qui
ajoute une étoile entre la composante stricte et les facettes simultanées de
chaque coface non-Gabriel. Sous la porte régulière globale, un MSF de `G_k^+`
préserve exactement les composantes de Gamma; sa construction littérale reste
exhaustive en cofaces et n'est qu'un oracle. La route sparse candidate emploie
les cofaces directes, tous les co-minimiseurs nécessaires et, dans la branche
régulière à au moins deux intrus, une gateway par facette du cœur avec un
resolver strict. Elle ne vise que le H0 horizontal normalisé : un journal de
classes de carriers, racines, parents et deltas de couverture, pas le payload
facetté de Gamma ni une partition des points. La preuve, les hypothèses et les
non-claims sont dans
[`AUDIT_REPARATION_K_GABRIEL_K_MST_20260812.md`](audits/AUDIT_REPARATION_K_GABRIEL_K_MST_20260812.md).
Un support proposé n'est pas encore une source directe complète. Pour une boule
d'intérieur strict `I_B`, de shell global `U_B` et de support minimal `S`, le
record `(S,B)` ne représente que `I_B union S`; si `U_B` contient d'autres
labels, la boule porte potentiellement d'autres cofaces directes. Les comptes
actuels deviennent une source régulière seulement après owner exact-once, RLE
par `GeometricBallKey`, census unique `I_B/U_B` par boule et porte `U_B=S`.
Les réponses d'implémentation — plateaux, niveaux exacts, déduplication,
resolver et fold parallèle par lot — sont dans
[`AUDIT_REPONSES_ROUTE_SPARSE_GABRIEL_20260812.md`](audits/AUDIT_REPONSES_ROUTE_SPARSE_GABRIEL_20260812.md).
Le pipeline sparse complet sous ces prémisses est décrit dans
[`NOTE_IMPLEMENTATION_SPARSE_COMPLETE_GABRIEL_GATEWAYS_20260812.md`](audits/NOTE_IMPLEMENTATION_SPARSE_COMPLETE_GABRIEL_GATEWAYS_20260812.md).

Le graphe brut, un RNG d'ordre fini, une cascade low-rank ou le seul résiduel
q2 ne constituent donc aucune source complète des supports q3/q4.

Le catalogue des supports `S` vérifiant `|I_B|+|S|<=11` est génératif pour les
cofaces de cardinalité au plus onze, mais il n'est ni bijectif ni équivalent au
rang fermé `|I_B|+|U_B|<=11`. Le front inverse actuel est un témoin q4, pas une
autorité q2/q3/q4 : sa transition vise désormais le premier croisement dans les
deux sens et transporte les lots, mais son germe ne rejoue pas les deux directions
du premier contact et refuse encore de vrais nuages affines-3. Sa récolte q2/q3
n'a ni juge de complétude, ni owner/BallRecord complet et filtre encore par rang
fermé. Le pivot itératif historique montrait en outre que la baisse du rayon
n'ordonne pas le nombre d'intérieurs; il ne décrit pas le comparateur du pinceau
courant. Le plein arrangement relevé reste interdit et sans borne
sortie-sensible. Les niveaux peu profonds d'une **ancre fixe** ont en revanche
une borne linéaire à profondeur fixée ; ils restent conditionnels au coût du
front et du census et ne donnent aucun droit de matérialiser une mosaïque
d'ordre supérieur sous un autre nom. Les preuves et fixtures sont dans
[`AUDIT_REPONSES_SOURCE_FRONT_INVERSE_20260812.md`](audits/AUDIT_REPONSES_SOURCE_FRONT_INVERSE_20260812.md)
et l'audit courant du volume/pinceau est dans
[`AUDIT_REPONSES_VOLUME_PINCEAU_PROJECTIONS_20260812.md`](audits/AUDIT_REPONSES_VOLUME_PINCEAU_PROJECTIONS_20260812.md).

La séparation arrangement/source est exacte, pas seulement empirique. Une
famille u16 à `n=50 000` possède `34 364 000 715` sommets relevés à shell quatre
jusqu'au niveau neuf, tous transits non positifs, mais seulement `499 945`
supports q2--q4 de Source S.
Les supports utiles doivent donc être produits avant les transits. Le théorème de
propriétaire donne alors les plafonds distincts `q2/q3/q4=9/8/7`; ces plafonds
préservent `K_max=10` et ne rendent pas le sweep **global** sortie-sensible. Ils
bornent en revanche les centres shallow distincts d'une ancre fixe, ce qui
autorise le producteur local décrit dans le contre-audit de la lentille aiguë.
Le théorème GPU par listes imbriquées de cellules de centres évite le census
global par tuple. Il ne prouve pas une source sparse : les cliques
d'intervalles peuvent rester `Theta(m^4)` et une subdivision peut répéter plus
de quadruplets que l'exhaustif global. La génération corrigée utilise des
arités q3/q4 indépendantes, une partition terminale commune à leurs budgets
d'intérieurs, un indice d'entrée immuable et une promotion par buckets. Dans
la variante BallKey-first, le RLE par clé géométrique précède l'unique census
par boule; dans SupportKey-first, il ne porte que la side queue. `U_B` reste un
certificat post-census. Après resserrement `tight`, la preuve exacte est une conservation
relative au pool hérité de `I_B union U_B`, pas l'identité avec les listes
globales d'un rescan. Un snapshot CPU historique reste `NO-GO` pour son
ordonnance mesurée : quatre compteurs avaient deux pentes successives
supérieures à 1,35 et 85,7 % des lifts mouraient à l'owner à `n=400`. Le
successeur courant n'hérite ni de ce NO-GO chiffré ni d'un GO : il doit publier
sa propre rampe contractuelle. La note
restructurée et le contre-audit pincé sont dans
[`NOTE_ARCHITECTURE_GPU_LISTES_CELLULES_CENTRES_20260812.md`](audits/NOTE_ARCHITECTURE_GPU_LISTES_CELLULES_CENTRES_20260812.md)
et
[`AUDIT_REPONSES_CELLULES_CENTRES_20260812.md`](audits/AUDIT_REPONSES_CELLULES_CENTRES_20260812.md).

La décision expérimentale par arité est explicite; elle ne constitue aucune
admission produit :

| tranche | voie activement explorée | comparateur ou voie suspendue |
| --- | --- | --- |
| `k=1` | Yao-1 exact puis EMST sparse | Borůvka point--LBVH borné |
| q2 profond | cellules de centres, lane `D_9`, à comparer avant tout port | cascade Yao--banque affine--dual et self-join conservés comme diagnostics/falsificateurs |
| q3/q4 | front de Jung coalescé, lentille fermée avec bit/certificat aigu et niveaux mono-ancre exacts, en exploration | cellules de centres et exhaustif borné comme comparateurs d'identités/coût |
| quotient H0 | fusion device vers activations, gateways et token Johnson | catalogue exhaustif exigé seulement par Gamma/verticales tant que leur reconstruction n'est pas prouvée |

Le transcript Yao-1 de `k=1` n'est donc pas abandonné avec la cascade q2.
Pour q2 profond, aucune des deux voies concurrentes n'est aujourd'hui une
source produit admise.

Le read-off k1 ajouté au snapshot `e6f1ef3` ne change pas cette matrice : il
calcule d'abord toute la source cellules q2/q3/q4, puis compare seulement les
poids d'une MST. Il ne publie ni endpoints ni multifusions et n'est pas une
voie sparse 50 k; son contre-audit appartient à
[`AUDIT_JUGE_CELLULES_INDEPENDANT_90C06B0_20260812.md`](audits/AUDIT_JUGE_CELLULES_INDEPENDANT_90C06B0_20260812.md).

Sur un Poisson homogène continu sans bord, les formules publiées des mosaïques
de Delaunay d'ordre k donnent environ `480,340886` supports positifs de Source S
par point jusqu'à `smax=11`, soit environ **24,017 millions** en bulk à 50 000
points. Cette baseline ne vaut directement ni pour une boîte u16 finie ni pour
du LiDAR, mais elle condamne un catalogue hôte de supports : la source doit
compter son trafic et fusionner vers le fold sur device. Le calcul, ses
hypothèses et ses références primaires sont dans l'audit cellules-centres.

Ces vingt-quatre millions de tâches utiles ne sont pas tenus pour le verrou
GPU. Le point gelé `uniform,n=50 000` produit `21 395 212` supports mais
`839 582 666` géométries, soit `39,242` occurrences par support, dont
`81,555 %` meurent à l'owner. La réparation prioritaire est donc un
`count/scan/fill` de clés compactes, un radix/RLE par `SupportKey` **avant** le
lift, puis une point-location directe du centre dans la feuille owner et un
rejeu du pool de cette feuille. Une face shallow arbitraire n'est pas une
source q2/q3 : il faut le minimum auto-centré de la fonction rayon sur son flat
d'égalité. Le contre-exemple et la preuve sont dans
[`AUDIT_DEBLOCAGE_GPU_SUPPORTKEY_TOP12_20260812.md`](audits/AUDIT_DEBLOCAGE_GPU_SUPPORTKEY_TOP12_20260812.md).

À cette taille, les occurrences q2/q3/q4 occupent exactement `6,33 Go`, ou
`12,66 Go` en double buffer radix, **si** leurs identifiants chauds sont des
`DensePointIndex:u16`. Ce layout `u32/u64/u64` exige une bijection immuable,
liée à `cloud_epoch`, vers les `PointId` durables; il n'encode pas directement
des `PointId` ABI arbitraires. La table de remap, les listes, le workspace et
les sorties restent hors de ce compte. La capacité et le high-water complet,
le nombre de clés uniques et le trafic mesuré restent donc tous des portes. Le
collecteur CPU non commité `UniqueKeyReceipt-v1` compte toutes les arités dans
des `u64`, trie q4 en colex, et son cap par worker ne borne pas ce high-water;
il diagnostique les uniques mais ne reçoit pas encore ce layout ni le SLO.

Pour un support validé `U` d'arité `q`, la sentinelle fixe minimale pour
`smax=11` retourne les `12-q` vrais plus proches `PointId` de `X minus U`, soit
top-10/top-9/top-8 pour q2/q3/q4. Si leur distance maximale `delta` est
strictement au-dessus du rayon `beta`, l'intérieur et l'extra-shell sont
complets; seul `E=U` publie directement. Si `delta<beta`, alors `p+q>=12` et le
support est rejeté. Si `delta=beta`, tous les intérieurs et au moins un contact
hors `U` sont connus : range-report, quotient ou refus fermé. Pour moins de
douze points, scanner `X minus U`. Le top-12 global reste sûr, mais n'est pas
minimal une fois `U` connu; top-`(11-q)` ne distingue pas le dernier intérieur
ou contact caché.

Un producteur reçu peut éviter cette requête terminale. Sur un domaine q3/q4
encore vivant, le filtre global au top-`(smax-2)` est toutefois redondant : si
moins de `smax-2` bornes inférieures sont positives, son seuil est non positif
et `U<theta` est déjà couvert par `U<0`; sinon les lanes sont mortes. Le top-k
sert donc à tuer un domaine ou un patch, pas à réduire la lentille d'un domaine
vivant. Une cutting signée half-open transporte les identités
`always_inside`, les conflits et le bit aigu, puis le census exact `I_B/U_B` au
centre owner. L'arête maximale canonique vise une émission par `SupportKey`,
mais plusieurs supports peuvent encore partager une `BallKey`. Ces propriétés
restent des gates contre la sentinelle hors support et l'oracle borné.

Dans le modèle continu, ou dans une famille de précision croissante, une sortie
exhaustive n'est même pas universellement linéaire : quatre amas de sites sur
une même sphère peuvent porter `Theta(m^4)` supports q4 positifs ayant une
seule clé de boule. Le profil u16 fixé est fini : cette construction y motive
une gate de plateau, mais n'y constitue ni une asymptotique ni une borne sans
fixture finie dédiée. Le RLE du census ne compresse pas ces
`SupportKey`. Le chemin H0 sous une seconde exige donc soit un quotient de
plateau reçu, soit un certificat de famille excluant cette sortie. Pour les
cellules, un certificat local falsifiable borne la liste : si le diamètre du
domaine vaut au plus `alpha*rho`, où `rho` est la distance au `(H+1)`-ième
voisin, et si la boule dilatée contient au plus `Lambda*(H+1)` sites, alors la
liste terminale en contient au plus autant. Sans ce certificat, choisir un
bitset fixe ou atteindre `max_depth` ne prouve aucune parcimonie.

`smax=11` borne une activation admise de rang fermé au plus onze sous le
contrat `RelevantGP`; il ne borne ni une coquille fermée arbitraire, ni le
nombre d'arêtes Gabriel incidentes à un point. Une coquille plus grande doit
être diagnostiquée puis traitée par un quotient saturé reçu ou refusée, jamais
tronquée. Le kissing number 12 ne
s'applique pas : dans l'espace euclidien, le degré est arbitraire même dans un
bucket de rang fermé fixé. Sur la grille u16 finie, les seules bornes
universelles immédiates disponibles ici sont les caps triviaux `n-1` et
`2^48-1`; aucune preuve ne permet d'affirmer qu'il n'existe pas de meilleure
borne finie. Deux constructions à treize voisins réfutent déjà le cap 12 aux
rangs exacts 2 et 11. Leur preuve est durable; le statut de leur
porte exécutable appartient exclusivement à l'audit live. Sous un modèle de
Poisson homogène 3D sans bord, le degré moyen jusqu'à `smax=11` vaut 80; c'est
une baseline, pas un cap ni une garantie de temps. La preuve est dans
[`AUDIT_DEGRE_GABRIEL_KISSING_SMAX11_20260811.md`](audits/AUDIT_DEGRE_GABRIEL_KISSING_SMAX11_20260811.md).

## Architecture candidate

```text
points u16 + LBVH exact résidents
  |-> k=1 : Yao-1 exact mutualisé -> EMST sparse
  |-> q2 : lane cellules D_9 en comparaison avec Yao--affine--dual suspendu
  `-> q3/q4 : front Jung + lentille fermée factorisée, bit/certificat aigu
       -> center-cover persistant + cutting signée half-open; top-k tue le patch
       -> q3 intrinsèque + niveaux q4 P/P, N/N, P/N ou cutting certifiée
       -> owner génératif exact-once ou RLE SupportKey -> une géométrie/owner
       -> census I/U complet + identités always-inside et support explicite
       -> side queue H!=empty/plateau ou second RLE BallKey A/B
       -> gate régulière / plateau / inertie de haut rang
       -> facettes du cœur, gateways et resolver strict
       -> MSF de carriers ou fold direct par lots atomiques
       -> composantes et coverage horizontal normalisés
       -> verticales séparément reçues et payload officiel nommé
```

Cette architecture possède un prior art mécanique dans la ligne enregistrée :
LBVH Morton/Yao48 CUDA tuilé, classifieur `count--scan` multi-rang sous son
ancien contrat fermé et falsificateur P1a q4. Les décisions q2 ne sont pas
compatibles : l'ancien prune admet une égalité radiale et son classifieur peut
s'arrêter sur dix contacts, tandis que v3 exige dix intérieurs stricts et un
census fermé complet. Les motifs structurels et transactionnels d'ownership,
de tuiles, d'epochs, de lease/reprise/backpressure, de ledger et de
`count--scan` à offsets 64 bits sont des différentiels à réécrire puis à
requalifier. Les décisions sémantiques, layouts, ABI et juges enregistrés ne
sont ni une autorité v3 ni une preuve de SLO. Leur inventaire est dans
[`AUDIT_REEMPLOI_YAO48_P1A_LIGNE_ENREGISTREE_20260811.md`](audits/AUDIT_REEMPLOI_YAO48_P1A_LIGNE_ENREGISTREE_20260811.md).

La ligne enregistrée contient aussi le théorème Yao-1/EMST, son oracle
quadratique borné et un prototype LBVH/Kruskal hôte. Ce dernier est un
blueprint rejeté comme chemin CPU produit, pas une preuve de débit. La route
v3 ne mutualise son premier voisin par chambre avec q2 que si la complétude et
les ex æquo canoniques sont certifiés; un budget épuisé ne prouve jamais une
chambre vide. Le contrat est détaillé dans
[`AUDIT_REEMPLOI_EMST_YAO48_LIGNE_ENREGISTREE_20260811.md`](audits/AUDIT_REEMPLOI_EMST_YAO48_LIGNE_ENREGISTREE_20260811.md).

Le self-join q2 de diagnostic reste un falsificateur borné ou un second prune
tant que ses compteurs complets ne battent pas la route Yao/LBVH. Son prune q2
ne retire jamais une ancre q3/q4.

Le déblocage recommandé ne change pas le prédicat q2 : il rend le produit
adaptatif sur cible/témoin, conserve une antichaîne immuable, choisit librement
le split qui résout le plus de masse et garde des masques
accepté/rejeté/ambigu dans les feuilles partielles. Un triple-tree
`(P,Q,W)` devient l'alternative si la route par ancre reste rouge. Une seconde
piste emploie l'inversion en l'ancre et des seuils de calottes par cellule
angulaire. Le certificat full-sphere demeure impossible sur les ancres du bord
convexe, mais le mode directionnel courant n'en dépend plus. Le signe q4 et la
condition top-M ont été corrigés; la voie cône n'est toutefois exercée par
aucun CTest et ses compteurs CPU croissent trop vite pour 50 k/1 s. Le détail
mathématique initial, les non-claims et les gates sont dans
[`AUDIT_DEBLOCAGE_Q2_ET_LOCALITE_20260812.md`](audits/AUDIT_DEBLOCAGE_Q2_ET_LOCALITE_20260812.md).

La rampe CPU Yao48/LBVH du snapshot `2e49dcf`, pincée à
`12 500/25 000/50 000`, ferme douze ledgers, mais classe l'ordonnance
état--nœud mesurée `NO-GO` avant G4 : `terrain` et les deux
familles scanline ont deux pentes chargées successives supérieures à `1,35`.
`uniform` seul n'a aucun compteur de travail publié rouge; la télémétrie
incomplète interdit d'en déduire un `GO`. Les temps étaient contaminés et ne
sont pas un benchmark; le verdict porte sur les compteurs. L'audit et la
distinction top-`K`/réservoir arbitraire `K+1` sont dans
[`AUDIT_RECU_YAO48_ECHELLE_2E49DCF_20260811.md`](audits/AUDIT_RECU_YAO48_ECHELLE_2E49DCF_20260811.md)
et la route de réduction exacte vers la seconde est dans
[`AUDIT_ROUTE_50K_1S_DF9DC77_20260812.md`](audits/AUDIT_ROUTE_50K_1S_DF9DC77_20260812.md).

Le successeur dual persistant du snapshot `c70974e` réduit fortement le
résiduel : sur les trois familles structurées complètes, les survivantes et le
classifieur passent sous la pente `1,35`. Il ne ferme pourtant pas la gate de
travail : `dual_witness_visits` reste rouge sur les deux doublements de chacune
de ces familles, et la série `uniform` est incomplète. La matrice v2 reçoit
ensuite le pointwise-leaf sur les mêmes trois familles : les visites restent
doublement rouges et le nouveau `dual_point_tests` n'est pas publié, donc elle
ne peut pas prouver un GO. Aucun de ces reçus ne qualifie le cap/clear courant,
un temps ou le payload produit. Voir
[`AUDIT_RECU_YAO48_DUAL_C70974E_20260811.md`](audits/AUDIT_RECU_YAO48_DUAL_C70974E_20260811.md).

La preuve locale q2 combine un supremum `U4`, un infimum `L4`, des témoins
distincts et une partition exacte des paires. Sa réception logicielle, ses
mutants et ses insuffisances ne sont pas dupliqués ici : voir le verdict live.
Les compteurs historiques à 50 k atteignent déjà 53 à 724 millions de visites
`L4` et 86 millions à 1,36 milliard de tests ponctuels pour q2 seul. Le reçu
brut est dans
[`scale_counters_raw.txt`](receipts/selfjoin_q2_20260811/scale_counters_raw.txt).
Ces compteurs refusent l'ordonnance mesurée avant census, q3/q4 et fold; les
chronos sous charge ne permettent aucune conclusion de latence.

Le cœur universel de Jung fournit une suppression supérieure exacte, distincte
de q2 : pour une paire distincte certifiée arête maximale d'un support propre
positif, neuf `PointId` q3 ou huit q4 distincts satisfaisant le prédicat strict
certifient toutes les sphères admissibles dans le disque de centres. Pour une
ancre et un témoin fixes, une borne entière par les huit coins certifie
uniformément un nœud AABB de cibles sans rescan par paire. Cette propriété est
prouvée; la banque, son parcours et sa gate restent à construire. Le certificat
ponctuel de Helly exploite les offsets des
demi-plans sur le disque : chaque crédit possède un sous-groupe de trois
identifiants au plus, et neuf ou huit groupes disjoints ferment la lane
correspondante. Contrairement au certificat plus étroit par enveloppe convexe,
Helly n'exige pas que chaque membre soit diamétral strict. Un greedy qui échoue
reste fail-open.

La profondeur fermée de demi-boule et son noyau angulaire partagé restent des
falsificateurs exacts complémentaires. Aucun reçu courant n'établit qu'une
collecte complète par paire paie son coût dans le chemin chaud. Cœur, groupes
de Helly, profondeur et center-cover par patches gardent des sorts et des
compteurs séparés. Les preuves et limites sont dans
[`NOTE_COEUR_UNIVERSEL_JUNG_ANCRES_Q3_Q4_20260811.md`](audits/NOTE_COEUR_UNIVERSEL_JUNG_ANCRES_Q3_Q4_20260811.md)
et
[`NOTE_CERTIFICAT_HELLY_DISQUE_JUNG_20260811.md`](audits/NOTE_CERTIFICAT_HELLY_DISQUE_JUNG_20260811.md).
Le statut précis des composants et de leurs portes reste exclusivement dans
[`AUDIT_ETAT_COURANT.md`](audits/AUDIT_ETAT_COURANT.md).

Le self-join d'ancres couvre implicitement toutes les paires et redémarre ses
recherches de témoins à la racine. Son snapshot pincé reste un oracle et un
falsificateur : le résiduel est plus mince que le travail, mais les visites
croissent trop vite pour en faire la route CUDA. Ce résultat historique ne
qualifie aucun successeur et ne remplace pas la porte contractuelle aux tailles
`12 500/25 000/50 000`; ses valeurs et hashes sont archivés dans
[`AUDIT_Q2_SELFJOIN_8A39C53.md`](audits/AUDIT_Q2_SELFJOIN_8A39C53.md) et les
reçus associés.

Le premier probe P1a q4 mass-only du snapshot `b312638` n'a montré aucune
fausse coupe dans ses campagnes bornées; sa condition géométrique est sûre sous
ses hypothèses. Il redémarre toutefois la recherche témoin à la racine pour
chaque bloc. Sur les deux doublements `terrain` de 2 000 à 8 000
points, les visites témoins ont des pentes `2,104` puis `1,896`; `uniform`
expire à 8 000. Le port littéral est donc `NO-GO` avant G4. Une ordonnance
persistante munie des bornes dirigées `L/U` doit d'abord subir un court
diagnostic structurel CPU qui exclut ce régime; ce diagnostic n'ajoute aucun
palier au protocole P1a direct. Le théorème, les trous de juge et les compteurs
pincés sont dans
[`AUDIT_P1A_CENTER_COVER_B312638_20260811.md`](audits/AUDIT_P1A_CENTER_COVER_B312638_20260811.md).

## Invariants industriels

- Aucun atlas global **persistant** de paires, tuples, cellules d'arrangement,
  faces, cofaces ou incidences n'est construit dans le chemin produit. Une
  frontière transitoire `count--scan--radix`, éventuellement logique sur toutes
  les clés mais segmentée et streamée, n'est pas un atlas si elle ne matérialise
  ni cellules/cofaces/incidences, si ses octets et son high-water sont
  préflightés et si elle est évincée après réduction. Une CSR transitoire de
  cellules de centres reste autorisée seulement si son coût complet passe la
  gate.
- Un oracle exhaustif borné falsifie ou recertifie le produit; il ne devient
  jamais son architecture par défaut. Le sujet cellules-centres, dont le juge
  partage encore des primitives géométriques, n'est pas lui-même cet oracle.
- Le chemin industriel exact n'a aucun budget configurable : il produit
  l'objet complet ou échoue sur une ressource physique réelle.
- Count, fill et consommation portent la même identité. Une insuffisance de
  ressource refuse atomiquement; elle ne tronque aucune sortie.
- Toute égalité géométrique reste dans la branche conservée. Pour le sujet de
  cellules, la partition exacte est `beta>R_q(C)` contre `beta<=R_q(C)`.
- La pertinence ne s'hérite jamais d'une arité à la suivante : une lane q3 ne
  dépend pas des q2 retenus, et une lane q4 ne dépend pas des q3 retenus.
- Une proposition flottante peut ordonner le travail; seul un prédicat exact
  et rejouable autorise un prune.
- Exactitude, réduction hiérarchique, performance et statut public sont quatre
  décisions séparées.

La section 1.1 de la spécification fixe le chemin produit sans budget
configurable. Un cap diagnostique peut refuser, mais ne peut jamais publier un
préfixe comme objet complet.

## Prochain ordre de travail

1. Installer immédiatement le squelette de `BenchmarkOutputContract-v1`, son
   payload et l'interface verticale avec producteurs `incomplete`, puis taguer
   chaque chantier `slo_critical_path=yes/no`. Conserver le générateur, les
   self-joins, le sidecar borné et les ancres comme portes locales ou oracles.
   Fermer les identités persistantes et les juges vraiment indépendants encore
   ouverts, sans promouvoir le rescan en route 50 k. Conserver aussi le front
   inverse comme falsificateur. Sa prochaine
   porte compare chaque premier successeur local `(cellule,flat ferme,sens)` et
   son lot à un oracle rationnel, tue un mutant d'ordre et couvre ex æquo,
   `lambda=0`, transport, cap et fallback; l'accord sur l'ensemble final des
   cellules ne suffit pas.
2. Conserver le Borůvka point--LBVH courant comme diagnostic borné. Pour la
   route `k=1`, extraire le premier voisin exact et canonique de chaque chambre
   pendant le parcours q2, avec reçu `candidate` ou `empty` complet; dédupliquer
   au plus `48n` arêtes, réduire ce graphe sparse et trier les `n-1` arêtes par
   niveau avant les lots atomiques.
3. Seulement si la comparaison q2 rouvre la voie suspendue, réemployer les motifs de lease, ledger et `count--scan` de la ligne
   enregistrée, sans copier ses layouts binary64 ni ses décisions de rang
   fermé. Garder `K=10` pour une banque certifiée des plus proches; réserver
   `K+1=11` aux réservoirs arbitraires qui doivent exclure au plus un membre de
   la boîte avant d'engager dix témoins. Appliquer en cascade la coupe
   cône--boîte, le certificat affine direct des banques chaudes, puis le
   dual-tree seulement aux plages non résolues. Employer le maximum entier
   exact pour `U`, trois masques accepté/rejeté/ambigu dans les feuilles,
   rollback de l'arène et microtuiles cibles. La frontière persiste sans rescan
   racine ni matrice cible--témoin. Elle doit authentifier ses crédits, compter
   tout le travail dual et réduire ses deux pentes rouges avant tout port
   device. Seul le résiduel finit dans le census résident multi-ordre avec
   offsets 64 bits. Les bornes et reçus sont spécifiés dans
   [`NOTE_SOLUTION_SOURCE_Q2_YAO48_LBVH_U16_20260811.md`](audits/NOTE_SOLUTION_SOURCE_Q2_YAO48_LBVH_U16_20260811.md).
4. Évaluer d'abord le fast path de fenêtre locale : top-`M+1` exact par ancre,
   coupure au premier site omis, génération complète indépendante q2/q3/q4
   dans la fenêtre, census `I_B/U_B`, owner après découverte et comparaison des
   identités à l'oracle borné. L'égalité et toute fenêtre ouverte rejoignent un
   domaine résiduel complet ; une file des seuls tuples refusés ne suffit pas.
   Mesurer séparément k-NN, propositions, positivité, census, certifiés et
   résiduels. Le shell n'est pas borné par `smax` et `M=128/256` n'est jamais
   une borne universelle.
5. Conserver le probe q4 mass-only `P15-HOCUDA-P1a` comme falsificateur : son
   port littéral à rescan racine est déjà refusé. Appliquer d'abord le cœur de
   Jung, puis remplacer le résiduel par une wavefront témoin persistante avec
   les bornes dirigées `L/U`; fermer les trous de bijection et de rejeu avant
   de le requalifier. La
   partition triangulaire reste implicite, les 64 patches de centres ont des
   coins de dénominateur quatre, le prédicat quadratique exact est évalué à
   l'échelle seize et le seuil q4 reste huit. Le ledger attendu est
   `pruned_mass+microtile_mass=C(n,2)`, sans arène globale de paires. Cette
   tranche n'émet aucune ancre et ne prouve pas la complétude de P1; elle est
   spécifiée dans
   [`NOTE_SOLUTION_P1A_CENTER_COVER_MASSONLY_20260811.md`](audits/NOTE_SOLUTION_P1A_CENTER_COVER_MASSONLY_20260811.md).
6. Sur les seuls blocs encore admis, mesurer d'abord la lentille aiguë
   `NONE/ALL/UNKNOWN` : `NONE` ferme une masse avant `PairId`, `ALL` reste
   factorisé et `UNKNOWN` se subdivise. Mesurer ensuite séparément cœur de
   Jung, Helly, composition cœur--profondeur et profondeur terminale. Le gain
   marginal doit payer collecte et tri ; toute ambiguïté retombe fail-open et
   aucun rescan racine n'est admis comme route.
7. Construire le front de Jung coalescé et sa cutting signée q3/q4 sans
   parcourir le plein arrangement ni remplacer le transcript Yao-1 de `k=1`.
   Graver `theta_only_prunes_on_live=0` et supprimer la sélection globale après
   ablation ; conserver le top-`(smax-2)` seulement comme certificat de mort
   d'un patch. Recevoir l'arête maximale canonique, puis remplacer la boucle
   `C(nlens,2)` par les niveaux mono-ancre `P-P/N-N/P-N` sur leurs segments
   actifs ou par une shallow cutting certifiée. Le rang restreint génère des
   centres, jamais le census publié. Recevoir le patch half-open avec
   `occurrences=SupportKey_unique`; rejouer un census complet `(I_B,U_B)` et
   transporter les identités `always_inside`. Un site shell écarté par `theta`
   implique déjà un support hors budget et ne peut appartenir à une sortie. Garder les
   lanes q2/q3/q4 et budgets `h` comme comparateur support-first. Pour cette
   baseline, employer une partition terminale commune, émettre les occurrences
   compactes puis faire un premier RLE par `SupportKey` **avant** tout lift.
   Calculer une seule géométrie et chercher au plus un contexte owner : zéro
   rejette le tuple, la complétude garantit l'existence pour tout support
   pertinent et plusieurs signalent un invariant rompu. Deux layouts restent
   à comparer. Des lots spatiaux de feuilles terminales atomiques paient au plus
   un lift par `(SupportKey,lot)`; sous arbre et epoch communs, tous les supports
   d'une même boule ont leur occurrence owner dans la même feuille et le même
   lot. Un RLE local par clé primitive de sphère est alors exact-once si le
   contexte owner certifie `b_cert>=H_run`. Des shards radix par `SupportKey`
   réunissent au contraire toutes les occurrences d'un support et peuvent ne
   payer qu'un lift par clé, mais deux supports de la même boule peuvent tomber
   dans des shards différents. Comparer après le lift deux ordonnances exactes :
   redistribuer les pending par `(cloud_epoch,GeometricBallKey)` vers leur
   `OwnerCellId` avant un second RLE, ou employer le census reçu du producteur
   puis top-`(12-q)` hors `U` en fallback. Dans ce second cas, seuls
   `delta>beta` et `E=U` publient directement; toute extra-shell, toute égalité
   et toute demande Gamma rejoignent la side queue. L'owner est une destination
   exacte, pas une
   colocalisation initiale. Lorsqu'il existe, un représentant par boule promeut
   ensuite le curseur `h` par nouveaux buckets et matérialise une seule fois
   `I_B/U_B`. Gamma
   conserve les provenances nécessaires; le H0 normalisé emploie un support
   canonique et le token Johnson. Graver les contre-fixtures q3-sans-q2,
   q4-sans-q3, pool-relative, budgets indépendants et shell 30. Fermer le domaine
   dégénéré et le cas terminal `k=n`, puis recevoir `BallActivation`, source
   directe, gateways, resolver strict, MSF/fold et reconstruction des verticales
   contre Gamma exhaustif borné. Une extra-shell pertinente exige un générateur
   saturé avec join de postings reçu, sinon un refus fermé. Installer le harness
   du payload officiel avant toute qualification GPU.
8. Pour P1a seulement, fermer le différentiel hôte à `n=32`, puis, dans la même
   session G4 gardée, exécuter la parité native, `n=32` sous Compute Sanitizer
   et le profil 50 k direct, sans taille intermédiaire ni retry. Pour les autres
   routes de source, appliquer la gate de compteurs à
   `12 500/25 000/50 000`. Toute route produit complète admise se mesure ensuite
   sur G4 avec build, transferts, source, certification, dix forêts,
   verticales, lots, certificat minimal et retour hôte dans le même p95
   `warm_e2e`.

## Construction des juges

```bash
cmake -S morsehgp3D_v3 -B build/v3 -DCMAKE_BUILD_TYPE=Release
cmake --build build/v3 --parallel
ctest --test-dir build/v3 --output-on-failure
```

Ces commandes valident des portes locales. Elles ne qualifient ni la source,
ni la performance, ni le statut public.

## Autorités

- [`PROPOSITION.md`](PROPOSITION.md) : architecture, preuves conditionnelles et
  conditions d'admission.
- [`audits/README.md`](audits/README.md) : index des audits et reçus.
- [`audits/AUDIT_ETAT_COURANT.md`](audits/AUDIT_ETAT_COURANT.md) : seul verdict
  live.
- [`../docs/SPECIFICATION_MORSEHGP3D.md`](../docs/SPECIFICATION_MORSEHGP3D.md) :
  contrat.
- [`../docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md`](../docs/math/STATUT_PREUVES_ET_HEURISTIQUES.md) : statut des preuves.

GCP non utilisé.
