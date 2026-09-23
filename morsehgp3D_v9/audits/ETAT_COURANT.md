# État courant des audits v9

23 septembre 2026. Dernier port publié du lecteur LiDAR et de la CI :
**`4b6e3aa6`** ; le moteur et la sonde v12 mesurés restent **`4530644b`**.
La sonde publie les parcours q3/q4 jusque-là cachés et le runner local v2
durcit la capture des pentes ; aucun nouveau reçu G4 n'en découle. Le dernier
[reçu G4 R7b](../receipts/g4_tower_r7b_20260923/README.md) exécute
le paquet **`8e8b83a3`**, antérieur au Welzl move-to-front, aux
séparateurs pseudo-aléatoires du tri FULL et à la libération précoce de
l'index de clés. Le [reçu R6](../receipts/g4_tower_r6_20260923/README.md)
exécute `78ce9fd4` ; comparer ses temps avec R7b exige d'ajouter le
digest séparé au périmètre mural ancien. Cadre :
`exploration_v9_hors_registre`, `reference_cpu`,
`quantized_u18_input_only`, **`not_claimed`**. Ce fichier porte le
verdict mutable ; les notes et reçus gardent les preuves.

La [première tentative R7](../receipts/g4_tower_r7_stockout_20260923/README.md)
a subi un `STOCKOUT` avant démarrage, sans chrono. La rupture de schéma
de la sonde v10/plan v5 est [close](PROTOCOLE_TOUR_V10_V5_RUPTURE_20260923.md)
depuis `f55ea40c`. Le lecteur `8e8b83a3` impose l'identité MEB exacte
`proposals=verified_proposals+proposal_fallbacks` trouvée par la
[contrelecture](MEB_PROPOSITION_EXACTE_20260923.md) : **21/21** selftests
normal et `-O`, et **31/31** mutants tués dans chacune des deux portes
réelles. Le WIP v6 C6/tri est [contrelu à part](CONTRE_AUDIT_B_WIP_V6_C6_TRI_20260923.md)
et ne qualifie pas v9.

## Contrat et objet effectivement construit

Le jalon v9 vise toute la tour HGP **K=1..10 en moins de 1 s sur GCP G4**,
avec repli K=1..5, puis 100 ms. Le sans-sol LiDAR u18/grille 1 mm est le
premier régime de travail. Décision utilisateur ultérieure : **la grille
1 mm est le profil de qualification prioritaire de v9 ; le float32 exact
est secondaire**. Ce choix de précision ne remplace pas le contrat
principal sur **trames brutes entières** par le sous-nuage sans sol ;
aucune hypothèse d'alignement entre passages LiDAR n'est admise. Les trois
trames 08/000000, 000100 et 000200 sont d'une seule séquence, pas une
qualification multi-séquence. Les coupes capteur servent au diagnostic de
croissance, jamais à remplacer une trame entière.

Pour ce diagnostic LiDAR, les **deux moitiés** sont séparées par le plan
capteur `x=0` et les **quatre quarts** par `x=0` et `y=0` : deux plans
verticaux contenant le capteur, sans sous-échantillonnage ; les points sur
un plan vont au côté non négatif. Il faut publier ces découpes pour la
trame brute entière aussi bien que pour le régime sans sol.
Les sept fichiers chronométrés v9 découpent la **géométrie sur grille 1 mm** : leurs
IDs sont disjoints et reforment exactement chaque trame sans sol. Une
coordonnée float32 brute de 08/000200, négative à 0,09 mm du plan, devient
`x=0` sur grille et change de moitié ; la
[contrelecture des découpes](DECOUPES_CAPTEUR_LIDAR_BRUT_ET_GRILLE_20260923.md)
sépare donc explicitement coupe brute et coupe quantifiée. Les six
préparations float32 v8 des trames brutes et sans sol contiennent déjà des
coupes par les signes originaux, vérifiées par IDs, coordonnées et hashes ;
elles ne sont pas des chronos de tour v9. Les disques
emboîtés 8k/16k/32k du runner de pente sont un **autre** diagnostic ; ils
ne remplacent ni ces découpes par plans ni la trame entière.

La chaîne produit des candidats de miniballes k-Gabriel locales, recoupe
leurs `BallKey`, recense exactement chaque clé **émise**, puis construit
la tour FULL relative à ce catalogue. Elle vérifie le support minimal
`q_min` des coquilles étendues et refuse sans troncature une coquille de
plus de 12 sites. Les petits oracles T2 comparent inventaire de boules et
tour Γ publique, y compris W1/W4 ; ils ne prouvent pas qu'aucune clé
entièrement omise ne manque à une grande trame. `complete_relative`
signifie précisément cela, et un lecteur de contrat doit exiger
`run_tower=true` et les ordres K=1..Kmax. Voir les [contrelectures du
moteur](CONTRE_AUDIT_A_MATH_MOTEUR_20260922.md), de
[FULL](CONTRE_AUDIT_B_FULL_COUTS_ET_INTERFACES_20260922.md) et de la
[porte T2](CONTRE_AUDIT_B_PORTE_PUBLIQUE_T2_20260922.md).

La [note d'Euler par ordre](NOTE_C_INVARIANT_EULER_20260923.md),
[démontrée aussi par le nerf sans position générale](CONTRELEC_EULER_PAR_NERF_20260923.md),
donne un contrôle global **nécessaire** du catalogue : `E_K=1` pour
`K≤Kmax−2`. Les 18 coupes LiDAR 8k/16k/32k publiées le satisfont et neuf
mutants d'omission échappant à la chaîne le violent à K5. La formule ne
certifie pas les clés une à une : à K5 elle ne juge que K1..3, et le
contrat K10 exigerait un générateur K12 pour juger ses dix ordres. Le
protocole Kmax+2 ajoute des témoins, mais une omission commune aux deux
exécutions peut encore passer.

Le port Euler v13 est au commit **`e76886af` du worktree constructeur**,
non encore publié sur `origin/main` lors de cette lecture. La sonde écrit
v13 et le lecteur G4 en vérifie la borne, le vecteur entier et les nouveaux
champs `q34_occupancy`/`tower_phases_ms`. Le lecteur local LiDAR accepte
v13 mais reste moins strict : sur une vraie réponse de 360 sites, K5/s8/W2,
il accepte encore chacun des trois mutants *vecteur `euler.by_k` raccourci
de 5 à 3*, *`q34_occupancy` absent*, *`tower_phases_ms` absent* ; le lecteur
G4 les refuse. `bench/run_lidar_scaling.py:175–181` ne vérifie que le
préfixe d'Euler et son selftest CMake relit toujours un cas v12. Aligner
les deux validateurs et ajouter ces trois corruptions au selftest v13 avant
de qualifier une nouvelle pente LiDAR.

Le calcul Euler reste inclus dans `census_ms` (`tower_chain.cpp:486,539–601`)
et un `E_K` faux refuse **avant** FULL (`:602–612`), alors que la décision
du constructeur prévoit son coût séparé et un refus après tour FULL réussie.
Les nouvelles portes mathématiques passent sur leurs cas actuels, sans
contre-exemple trouvé à la formule ; leur test de mutants accepte cependant
`cause=euler.chain_refused` pour **toute** erreur de chaîne
(`chain_euler_gate.cpp:60–61`, `CMakeLists.txt:171–172`). Exiger la raison
`chain_catalogue_euler_violated` et le statut Euler `fails` rendra le mutant
causal. Pour `run_tower=false`, expliciter dans la preuve la positivité des
supports réguliers, que les fabriques exactes q2/q3/q4 imposent déjà.

Les certificats exacts actuellement raccordés comprennent la saturation
profonde de l'atlas, le census q3 sur fragment complet, la preuve de voies
q3/q4 mortes et le cache de nœuds témoins. Le propriétaire d'arête et les
tests stricts gardent les contacts dans la coquille. La porte propriétaire
de `84c74a5e` tue causalement l'ancien cache d'index par adresse nue et
vérifie la réutilisation après `bad_alloc`. `e5688680` exige désormais le
réemploi effectif de la même adresse hors ASan ; la quarantaine ASan
empêche ce plancher sans invalider les autres contrôles. Voir la
[contrelecture du chargement des
formes](CONTRE_AUDIT_B_CHARGEMENT_FORMES_Q34_WIP_20260923.md) et la
[preuve conjointe](CONTRE_AUDIT_B_Q34_PREUVE_CONJOINTE_WIP_20260923.md).

`a78664d4` tente le certificat de voies mortes d'abord sur les sites de la
boule diamétrale de l'arête, sous-ensemble du cover complet ; toute voie
ouverte repasse par celui-ci. Le sous-ensemble ne peut ajouter un faux
témoin intérieur. Les portes locales comparent les candidats aux petits
oracles et exercent fermeture puis repli ; R6 isole désormais ce levier
**ON par défaut** en ablation FULL sur G4. `e5688680` protège
aussi les consommateurs complets contre un cœur passé par erreur ;
`cc4664e5` place le même refus avant les retours q4 à K1/2. La porte
FULL actuelle
compare les cinq leviers ensemble, donc couvre le raccord sans attribuer
une égalité au seul cœur. Corriger aussi le commentaire de profondeur
« exacte » dans `q34_dead_lanes.cpp` : sur le cœur, le compte ponctuel est
un minorant et ne sert qu'à abandonner une preuve. Voir la
[contrelecture du cœur](CONTRE_AUDIT_B_NOYAU_DIAMETRAL_WIP_20260923.md).

## Ce que mesurent les reçus G4

Le [reçu R3](../receipts/g4_tower_r3_20260923/README.md) donne six paires
FULL CPU on/off du certificat de voies mortes, même entrée et sorties :
la chaîne passe de **12,93→8,89 / 10,12→6,19 / 19,71→10,93 s** à K5
et **48,33→35,56 / 36,29→25,96 / 62,99→37,75 s** à K10
(08/000000, 000100, 000200). Générateur, catalogue, ordres et digest
coïncident dans chaque paire ; les masses de travail expliquent le gain
aval. La [contrelecture indépendante](CONTRE_AUDIT_B_G4_R3_20260923.md)
vérifie les sorties brutes, le préflight et l'arrêt ciblé. Cette ablation
ne donne ni complétude exhaustive, ni borne sous-quadratique.

Le [reçu R4b](../receipts/g4_tower_r4b_20260923/README.md) ajoute six
paires cache de témoins ON/OFF : la chaîne gagne **0,12 à 0,89 s** selon
la paire, en une seule exécution par configuration. Les rejets par cache
représentent **53,36–69,76 % des paires résiduelles développées**, pas
de toutes les paires du nuage. Les sorties demeurent égales ; ni preuve
conjointe ni coût de croissance ne sont isolés par cette ablation.
La [contrelecture R4b](CONTRE_AUDIT_B_G4_R4B_CACHE_20260923.md) recoupe
le reçu. La première tentative R4, [arrêtée avant le
worker](../receipts/g4_tower_r4_preempted_20260923/README.md), ne donne
aucun chrono. Son README annonce `compute.instances.preempted`, mais ne
joint pas la trace GCE brute de cet événement ; les traces hôte suffisent
à prouver l'absence de sonde et l'arrêt ciblé, pas la cause exacte.
Le [contrôle d'antichaîne du cache](CACHE_TEMOINS_COUT_VALIDATION_20260923.md)
paie aussi du travail non inclus dans `node_tests` : les six cas R5
impliquent au moins **1,090 milliard** de tours de validation et
**424,330 millions** de comparaisons d'intervalles. Ce sont des
minorants de comptage, pas des durées ; un ticket interne possédé
pourrait réutiliser l'antichaîne certifiée sans assouplir l'API publique.

R5 exécute **13/13 cas FULL CPU `complete_relative`** : deux répétitions
W48 des trois trames sans sol à K5/K10, plus 000000/K10/W24. Les 245
hashes et sept comparaisons d'objet concordent ; l'arrêt ciblé
`TERMINATED` est attesté. À s8 avec les quatre leviers actifs, les
intervalles de mur entre les deux répétitions W48 sont :

| 08/ | K1..5 | K1..10 |
| --- | ---: | ---: |
| 000000 | 5,96–5,98 s | 17,00–17,37 s |
| 000100 | 4,26–4,33 s | 12,07–12,09 s |
| 000200 | 7,38–7,40 s | 19,28–19,57 s |

La tour seule prend 0,84–1,10 s à K5 et 4,01–5,37 s à K10 ; q3/q4
reste le premier poste (65–70 % du total à K5, 45–57 % à K10).
Face à R4b, le temps de tour K10 descend de 11,42–14,89 s à
4,15–5,37 s sur la même cible, mais plusieurs changements séparent les
sessions : ce n'est **pas** une ablation causale des seuls ordres K
concurrents. La [contrelecture R5](CONTRE_AUDIT_B_G4_R5_20260923.md)
détaille les limites. Le cas 000000/K10 à W24 prend 21,59 s contre
17,00–17,37 s à W48, pour moins de CPU·s ; un seul cas ne sépare pas
déséquilibre, contention et taille des lots.

R5 mesure **uniquement le CPU** d'un G4, sur u18/grille 1 mm sans sol de
la séquence 08. Le meilleur mur est encore 4,263 s à K5 et 12,067 s à
K10 : aucun contrat de 1 s, 100 ms, GPU ou croissance sous-quadratique
globale n'est acquis. Le [premier reçu local](../receipts/first_tower_20260922/README.md),
[R1](../receipts/g4_tower_r1_20260922/README.md) et
[R2 refusé par son validateur](CONTRE_AUDIT_B_G4_R2_PREFLIGHT_20260923.md)
restent des témoins historiques ; leurs temps ne remplacent pas R5.

Le [reçu R6](../receipts/g4_tower_r6_20260923/README.md),
[contrelu indépendamment](CONTRE_AUDIT_B_G4_R6_20260923.md), a exécuté
le snapshot `78ce9fd4`
sur G4 24 cas FULL CPU (trois trames entières sans sol, K5/K10,
W48/s8, deux répétitions ON/OFF du seul cœur), tous
`complete_relative` ; la relecture indépendante de la capture rend
`completed`, avec arrêt ciblé certifié et 390/390 empreintes intègres.
Dans les douze paires, émissions q2/q3/q4, résumés de catalogue,
ordres, travail FULL et digest coïncident ; R6 ne publie pas le flux
complet des clés/supports/coquilles. Le mur de chaîne gagne
**0,51–8,66 %** par paire (**1,8–7,7 %** sur les moyennes des deux
répétitions par scène/K) ; le CPU·s gagne **8,09–21,18 %** par paire.
Le cœur ferme **54,9–58,9 %**
des arêtes qui arrivent au cover et réduit les formes chargées de
**72,85–81,87 %**, mais augmente les visites d'index core+cover de
**21,0–45,6 %**. Meilleurs cas ON : **4,151 s à K5** et **11,726 s à
K10**, toujours hors contrat. L'[erratum
R6](../receipts/g4_tower_r6_20260923/ERRATUM.md) corrige « générateur
identique » : `q34_cover_builds` change par construction
(000000/K5 : 900 377 ON, 2 043 612 OFF), tandis que les émissions et
masses de candidats restent identiques. Il ne corrige pas encore les
plages arrondies du README, qui omettent 0,51 %, 8,66 % et 72,85 %.
Les trois compteurs du cache témoin varient légèrement entre
répétitions, sans changer l'objet.

À travail CPU mesuré inchangé, même une répartition idéale sur les
**48 fils logiques** du G4 laisserait, pour le meilleur cas R6 ON,
**1,89 s à K5** et **5,81 s à K10** de CPU·s/48 avant tout coût de
synchronisation. Ce calcul ne borne ni un nouvel algorithme ni une
voie GPU ; il montre que le seul ordonnancement CPU de l'existant ne
peut atteindre 1 s.

La reprise du cover complet à partir des nœuds terminaux du cœur est
exacte en principe, mais R6 borne son bénéfice en visites : sur
08/000100 et 000200, à K5 **et** K10, les visites du cœur seul excèdent
déjà les visites du cover OFF (de **0,2 à 12,0 %** selon le cas). Même
un second parcours gratuit ne ferait pas descendre ce compteur sous OFF.
Sur 08/000000, il faudrait économiser au moins **81,3 %** des visites
du cover ON à K5 et **72,6 %** à K10 pour atteindre seulement le niveau
OFF. Cette borne ne condamne pas la reprise : elle peut réduire CPU et
mur en conservant la baisse des formes chargées, mais impose de mesurer
son coût mémoire et de viser ensuite un certificat **avant** l'expansion.

Le [reçu R7b](../receipts/g4_tower_r7b_20260923/README.md) reprend sur
G4 le paquet `8e8b83a3` : **24/24 cas FULL CPU
`complete_relative`**, deux répétitions entrelacées ON/OFF du seul
MEB proposé pour trois trames 08, K5/K10, s8/W48, dix-huit comparaisons
de **projection d'objet** égales, arrêt ciblé certifié et **388/388**
empreintes du paquet vérifiées par la
[contrelecture indépendante](CONTRE_AUDIT_B_G4_R7B_20260923.md)
en Python normal et sous `-O`. Le reçu ne publie pas les flux
intégraux de clés/supports/coquilles/parents. Les trois compteurs du cache témoin
`witness_cache_queries/node_tests/rejected_pairs` varient légèrement
entre les paires malgré l'identité de l'objet ; la phrase du README
« travail hors MEB identique » doit être lue comme **travail de tour
hors MEB**, pas comme égalité intégrale du ledger générateur.
Toutes les propositions MEB sont
vérifiées. À K10, le MEB ON réduit la tour de **5 à 8,4 %** et la chaîne
de **2 à 4 %** ; à K5, le gain de tour reste dans le bruit et la chaîne
est parfois plus lente. Meilleurs cas ON : **3,67 s à K5** et **9,58 s
à K10** pour `chain_total`, plus respectivement **0,16 s** et
**0,80 s** de digest synchrone. Ce sont encore des secondes, sur CPU
seul et une seule séquence. R7b inclut d'autres changements depuis R6 :
leur effet ne se déduit pas de cette ablation. Il n'exécute ni Welzl
move-to-front, ni le nouvel échantillonnage FULL, ni la libération
précoce de `key_slots`, et ne prouve pas la complétude des clés omises.

## Verrou q3/q4 : réduire le travail avant l'expansion

Le certificat de voies mortes a un vrai bénéfice aval, mais construit
encore le cover avant de charger les formes de tous ses sites. R5/K10
compte **4,15–9,28 milliards** de `dead_form_sites` sur 35–46 k sites ;
`load()` écrit en plus deux formes nulles par cover. Sur 000000/K10,
les 7,805 milliards d'incidences site–cover logiques montrent pourquoi
un certificat qui relit chaque cover ne ferme pas le verrou structurel.
Le [reçu exploratoire par
arête](CONTRE_AUDIT_B_RECU_VOIES_MORTES_20260923.md) a corrigé les
pourcentages et covers moyens mal définis dans la provenance initiale ;
ses prototypes sans FULL ne sont pas une mesure R5.

Diagnostic local du commit `a78664d4` sur **un quart spatial seulement de
la trame brute** 08/000000 à 1 mm (sol conservé)
([entrée u32le](../../morsehgp3D_v8/receipts/float32_precision_20260921/release_r2/precision_a1drpf9i/scene_00_000000_grid/quarter_x_neg_y_neg.u32le),
30 263 sites, K5/s8/W8, tour statique W8, quatre autres leviers ON) : deux paires
OFF/ON du cœur gardent catalogue, ordres, travail FULL et digest égaux.
Le cœur ferme 443 495 des 990 559 arêtes qui survivent au filtre ; les
incidences site–cover payées passent de 454,08 M à 120,56 M en additionnant
**cœur et cover complet**. Le CPU de chaîne baisse de 72,05/71,88 à
68,72/68,56 CPU·s, tandis que le mur de chaîne vaut 14,81/17,04 s OFF
et 16,08/24,06 s ON. Sur cet hôte partagé, aucune amélioration murale
stable n'est établie ; les sorties JSON et le binaire local ne constituent
pas un reçu G4. Entrée SHA256 `2632c86e…6c516e`, binaire
`267dbed7…b7734` ; les sept sources principales ont les mêmes empreintes
que le commit publié. Les mesures G4 doivent inclure les visites et tests
du **core_cover**, publiés par la sonde v8 de `e5688680` et maintenant
reçus sur G4 dans R6. Les temps et masses par worker restent nécessaires
pour diagnostiquer le chemin critique. R6 a réalisé l'ablation appariée
sur trames sans sol entières avec préflight ON ; l'expansion `A×B`
demeure entière.

Une [ablation locale complémentaire sur deux coupes **sans
sol**](CORE_LIDAR_LOCAL_20260923.md), K5/K10/s8/W8 et sans FULL,
compare aussi le **flux entier** des présentations q3/q4 : OFF et ON
coïncident sur 117 196, 565 007 puis 2 032 711 lignes, avec clé,
support, profondeur et coquille exacts. Le coût reste dépendant du
régime : à K10, les visites core+cover montent de 83,1 % sur 8 225
sites mais baissent de 10,1 % sur 13 055 sites. Le CPU de chaîne
diminue dans les deux cas ; le mur q3/q4 régresse sur la petite coupe
et gagne sur la grande, une répétition sur hôte partagé. Construire
le cover complet puis filtrer le cœur exigerait de scanner jusqu'à
1,63 milliard d'incidences sur la grande coupe, contre 115 millions
pour le cœur actuel : cette variante ne mérite pas de port sans
prototype favorable. Aucune paire développée n'est supprimée.

La priorité constructive est de prouver un masque q3/q4 **avant**
l'expansion de produits résiduels `A×B`, puis avant le cover pour les
arêtes restantes. Le filtre actuel de rectangle ne voit que des témoins
universels ; un filtre exact de ligne `a×B_node` peut écarter `|B|`
paires d'un coup, à condition de transporter le vrai nœud B et sa boîte.
Le sidecar trouve 79–88 % de masse développée dans des rectangles
éligibles, mais paie 15,7–29,8 visites DFS par paire évitable sur son
échantillon ; le prototype local de ligne indépendante régresse en CPU
total. Tester d'abord un ticket borné de nœuds témoins réemployés, un
seuil de déclenchement et le repli exact. Ne jamais compter deux fois
un témoin : le ticket porte masque, antichaîne, compte et curseur, et
le DFS ne visite que le complément de ses plages. Voir le
[contrat de coûts](CONTRAT_COUTS_ET_PARALLELISATION.md) et les
[certificats par rectangles](PISTE_B_Q34_RECTANGLES_AVANT_EXPANSION_20260923.md),
[nœuds avant cover](PISTE_B_Q34_NOEUDS_AVANT_COVER_20260923.md) et
[gardes par blocs](DOMINATION_Q4_PARESSEUSE_PAR_BLOCS_20260923.md).
Leurs bornes locales sont exactes ; aucun gain net LiDAR de ces
certificats par blocs ni majorant global sous-quadratique n'est démontré.

Le [shadow LiDAR de la palette par ancre](SHADOW_HA_Q34_LIDAR_20260923.md)
isole une proposition plus légère qu'un nouveau DFS de ligne. À K10/s8
sur 08/000000 sans sol, il ferme **3 499 305 / 30 777 213** paires
résiduelles avant expansion ; les **30 777 213 masques de paire** du
replay mono restent identiques, comme les **4 507 278 covers potentiels**.
L'économie CPU locale indicative vaut environ **1,29 s** après préparation
et tests de palette, sur hôte partagé et sans FULL/G4. Le cache de paire
rejette déjà la plupart de ces paires à faible coût ; ne porter la palette
qu'après une ablation de chaîne ON/OFF, identités complètes et coût par
worker inclus. Une palette des seuls proches peut manquer les témoins
dans la direction de B, comme le montre la contre-fixture B.

Le [shadow orienté par octant](SHADOW_HA_OCTANT_Q34_LIDAR_20260923.md)
emploie les mêmes voisins proposés et le prédicat entier exact sur
l'octant pointant vers B. Sur 08/000000 sans sol K10/s8, il ferme
**5 177 835** paires avant filtre, contre **3 499 305** pour les
proches ; les **30 777 213 masques** et **4 507 278 covers** restent
identiques. Le replay mono gagne environ **3,85 CPU·s locaux nets**
après préparation et lignes, sur hôte partagé ; 08/000000 K5 gagne
**1,82 CPU·s**, mais 08/000100 K5 seulement **0,19 CPU·s** et régresse
en mur. Les passes K10/s10 et 08/000100 K10 ne jugent que les lignes,
pas le replay. Conserver l'octant en SHADOW jusqu'à une ablation de
chaîne entière, mémoire comprise, sur plusieurs séquences et sol brut ;
aucun cover ni travail aval n'est supprimé ici.

Le [contre-audit du grand-livre q3/q4](LEDGER_VISITES_CACHEES_Q34_20260923.md)
montre que six parcours d'index déjà comptés par le générateur et les
sites balayés par le sweep q4 n'étaient pas projetés dans la sonde
FULL v11 de R7b. Le port v12 `4530644b` les publie maintenant ; ses
identités/bornes sont cohérentes avec les options actuelles de chaîne.
Ils incluent les témoins par rectangle/paire, l'accès aux graines q3
par arête et trois parcours q4 par arête. Sur un index à `2n−1` nœuds,
leurs bornes par appel restent linéaires en `n` ; les comptes R7b publiés
ne permettent donc pas d'écarter un coût caché
`(rectangles + paires recherchées + arêtes q3/q4)×n`. Le reçu v12
ci-dessous publie leur croissance sur trois trames LiDAR appariées.

Le [reçu local v12](../receipts/lidar_scaling_local_20260923/README.md),
[contrelu indépendamment](CONTRE_AUDIT_LIDAR_SCALING_V12_LOCAL_20260923.md),
archive trois trames sans sol à K5/K10 : **60 cas de sonde et six résumés**,
soit 66 JSON ; la formule « 66 cas » du README compte aussi les résumés.
Sur 08/000200/K10, de 16k à 32k, les paires développées font ×4,27 et la
population logique des cœurs ×7,25, alors que le catalogue de boules fait
×1,86. C'est un signal local de coût caché à attaquer avant le cœur,
pas une borne asymptotique. Les temps viennent d'un hôte CPU partagé ; le
reçu n'est ni G4 ni une preuve de contrat ou de complétude absolue.
La [lecture des demi-scènes et quarts](CROISSANCE_LIDAR_PLANS_ET_DENSITE_20260923.md)
compare les 42 cas spatiaux du reçu : 0/36 pentes CPU parent→morceau
dépassent 2, mais **13/36** pentes de population logique du cœur le
dépassent. Les coupes changent la géométrie. Le
[premier reçu de densité](lidar_density_scene02_20260923/README.md)
garde chaque secteur de 08/000200 fixe et ajoute **28** sondes à 1/4 et
1/2 des sites ; au K10, `dead_core_form_sites` a une pente d'au moins 2
sur **4/28** relations adjacentes, concentrées dans `x≥0` et
`x≥0,y<0`, malgré des pentes de paires et CPU sous 2.
L'extension aux [trames entières 000000/000100](lidar_density_full_3scenes_20260923/README.md)
et à [leurs six secteurs](lidar_density_sectors_00_01_20260923/README.md)
ajoute **56 sondes**. Sur les trois scènes et sept secteurs, **14/84**
relations de densité adjacentes atteignent `p_formes≥2` ; le quart
`x≥0,y<0` le fait dans chacune des trois trames à K5 ou K10. Paires,
visites/bornes des nœuds et CPU restent sous 2 dans ces essais, sans
preuve asymptotique. Les trois trames appartiennent à une seule séquence,
sans sol et sans mesure G4. Une ablation appariée du seul site qui étend
fortement `z` dans le quart chaud de 000200 laisse la pente K10 des formes
à **2,042528** : cet extrême ne porte pas à lui seul le signal. Le secteur
est fixe, mais l'étendue des sites sélectionnés varie avec la densité.
Le [crédit exact par nœuds du certificat de cœur](../receipts/dead_node_credit_negative_20260923/README.md)
a été essayé hors produit : mêmes voies et digest, mais CPU de chaîne
**+27 % à K5 et +32 % à K10** sur la coupe 16k de 000000 ; cette variante
est fermée. La réduction des paires longues avant le cœur reste ouverte.

Le [reçu local de pente LiDAR v11](CONTRE_AUDIT_PENTE_LIDAR_LOCALE_PARTIELLE_20260923.md)
est intègre (15/15 SHA et entrées vérifiées) mais partiel : une seule
trame sans sol, K5 aux tailles 8k/16k/32k et K10 seulement à 8k/16k.
Les chronos internes K5 croissent d'environ ×2 à chaque doublement,
mais `core_sites` croît ×7,66 au premier ; à K10, ×5,69 de 8k à 16k.
Ce signal ne qualifie ni le travail total sous-quadratique, ni G4/GPU.
Le lecteur de `06f71037` corrige les quatre omissions du runner v2 : FNV
recalculé sur les octets fournis, format, grille 1 mm et threads statiques
vérifiés ; 25/25 mutations sont refusées sous Python normal et `-O`.
L'[addendum de revalidation](../receipts/lidar_scaling_local_20260923_revalidation/README.md)
retrouve les 60 cas archivés, sans nouveau calcul HGP. Les anciens JSON
gardent `grid=unspecified` dans la sonde ; la grille 1 mm est attestée par
les manifestes. `4b6e3aa6` lie désormais le nom du résumé à scène/K/s/W/
répétition et K/s/W de chaque commande au résumé ; il réancre les chemins
historiques du selftest (27/27), sans nouvelle sonde HGP. `--revalidate` ne
recoupe pas encore `record.case`, le chemin d'entrée dans `argv[0]` ni les
empreintes/commit du résumé, et un sous-ensemble non vide de groupes peut
retourner code 0. Ne pas employer
ce seul code de sortie comme preuve de couverture de la campagne entière.

Le [shadow de scission des rectangles q3/q4](Q34_BLOCS_LIDAR_SHADOW_20260923.md)
sur 08/000000 sans sol ferme 3,68 M paires à K5 et 4,12 M à K10 avant
leur filtre individuel, mais paie 40,9 M et 62,5 M visites supplémentaires
de nœuds témoins. Ces paires auraient déjà été rejetées par le filtre
individuel : aucun cover aval n'est économisé. Ne pas porter cette scission
naïve sans bilan apparié du travail complet ; le cache de témoins par boîte
reste une hypothèse distincte. La contrelecture indépendante juge les
certificats exacts sur la grille 1 mm, mais le reçu ne conserve pas les
sorties brutes de cette sonde pour un rejeu indépendant.

Pour q3, seuls les fragments d'atlas **complets** fournissent un compte
réutilisable ; un certificat profond incomplet n'est qu'un minorant.
Partager les graines d'une cellule exige un ticket possédé
`(X,compte,curseur Z)`, la recollecte de **toute** la coquille et un
relais EOF valide même en présence de contacts. Les 171 444 arêtes q3
seules du reçu v8 1 mm ne bénéficient pas du fragment q4. Des
[oracles u18](check_q3_shared_u18_20260922.py) couvrent les cas
ambigus ; la [note q3](Q3_STRUCTURE_ET_BORNES.md) et la
[contrelecture feuille](CONTRE_AUDIT_B_Q3_FEUILLE_WIP_20260923.md)
donnent aussi la descente rationnelle aux coupures dyadiques et une
palette privée de témoins. L'essai feuille non apparié augmentait
fortement les tests ponctuels q3 : mesurer rejets tardifs, fragments
`Leaf`/`Deep`, coquilles et octets avant de porter un collectif.

Pour q4, la [note de structure](Q4_STRUCTURE_ET_BORNES.md) et les
[niveaux orientés](CONTRE_AUDIT_B_Q4_NIVEAUX_ORIENTES_20260922.md)
bornent localement les centres peu profonds et proposent une sélection
de racines exactes. Ni construction/census global ni coût de sortie
sous-quadratique n'en découlent : les incidences site–cover dépassent déjà
`n²` sur la trame 1 mm, donc lire tous les covers déplacerait le coût.
Les contacts, le propriétaire et `centre∈conv(coquille)` restent
obligatoires. Une q4 peut survivre quand toutes ses faces q3 sont
rejetées : ne pas limiter ses graines aux q3 finalement émises
([fixture B](CONTRE_AUDIT_B_PREATLAS_ET_Q3_20260922.md)). La
[revue d'induction de l'atlas](Q4_INDUCTION_ATLAS_EVENEMENTS_20260923.md)
explicite, pour une arête propriétaire q4 déjà transmise avec cover complet,
pourquoi le centre positif échappe aux refus `Outside`/`Deep` et atteint un
groupe émis, même si les faces q3 sont rejetées. La
[preuve amont q3/q4](Q34_PROPRIETAIRE_PASSAGE_AMONT_20260923.md) établit
conditionnellement que les filtres du front, des rectangles, des paires et
du cœur ne perdent pas l'arête propriétaire d'un support positif admissible ;
elle ne borne pas le coût global. Le
[gate de flux entier à douze sites](q4_global_12sites_20260923/README.md)
compare **108 sorties complètes** à l'oracle rationnel sur deux permutations,
`s=8/10/12`, `Local28`/`Window30` et mono/W1/W4. Il ferme cette fixture
précise, pas la preuve des filtres WSPD ni la complétude globale d'une trame.
Un rejeu indépendant sans réécrire le reçu, sur les mêmes SHA de
bibliothèque et de gate produit, reproduit les **108/108** flux et
**3 580** contrôles ; le HEAD local a avancé à `28f0c284` sans modifier
ces dépendances.
Le [cover commun par blocs](CONTRE_AUDIT_B_COVER_BATCH_20260923.md)
et la [saturation K−2 des seules arêtes q4](SEUIL_SATURATION_ATLAS_PAR_VOIE_20260923.md)
restent des pistes secondaires à mesurer avec le coût aval complet.

## FULL, sortie explicite et résidence

`684d8fc7` construit les ordres K en parallèle, publie les images
verticales après leurs lots et déplace les populations vers une banque
partagée. `84c74a5e` conserve le travail payé en cas d'échec et choisit
le plus petit K en échec entre les phases A/C ; deux mutants ciblés sont
tués. R5/R6/R7b exécutent ensuite cette voie sur G4, sans l'isoler de
tous les autres changements. Une phase de préparation/tri
peut garder deux buffers de requêtes coexistants, et les dix ordres
gardent simultanément leurs états : demander RSS et capacités **au même
instant**, par phase et K. Voir l'[audit des ordres
parallèles](PREFETCH_GEOMETRIE_FULL_PAR_K_20260923.md) et sa
[contrelecture B](CONTRE_AUDIT_B_FULL_PARALLELE_WIP_20260923.md).

La sortie explicite est déjà un verrou matériel. R5/K10 publie
**5,95–7,47 millions de nœuds**, autant moins dix références de parents
et **3,55–4,47 millions de contributions** par trame. À l'ABI courant,
les seuls tableaux nœuds/parents/contributions/successeurs/images
verticales représentent **770–968 Mio** retenus, hors banque de
populations, catalogue et capacités. Le catalogue compte
**4,38–5,51 millions de boules** et **0,982–1,235 Go décimaux** ;
le RSS K10 est **3,8–4,8 Gio**. Le [calcul de
résidence](CONTRE_AUDIT_B_RESIDENCE_CHAINE_20260922.md) établit déjà
`216n` octets publiés à K1 ; le cache direct
`48·nextpow2(16n)` appartient au résolveur séquentiel et n'est pas
configuré sur la voie statique R5. Une sortie exacte adressable par
fenêtres, CSR/arène ou externe, avec coût d'export et identité conservée,
est requise avant de promettre des dizaines de millions de points.
La [borne R5 détaillée](CONTRAT_COUTS_ET_PARALLELISATION.md) n'est ni une
borne asymptotique sur d'autres LiDAR ni un RSS de phase isolé.

Dans le meilleur cas R7b ON, 08/000100/K10, q3/q4 prend **5,226 s**,
FULL **3,199 s** et les autres postes de chaîne **1,154 s**. Rendre
q3/q4 et FULL gratuits sur ce chemin séquentiel mesuré laisserait donc
encore 1,154 s, sans borner un nouvel algorithme ni un chevauchement GPU.
`50690c12` isole le digest dans `times_ms.digest` ; il reste
**synchrone** dans l'appel public.
La destruction des temporaires déclarés dans le `try` est comprise
dans `chain_total` ; celle du résultat retourné intervient après l'appel.
Les présentations ont seulement 2–13 doublons pour 4,38–5,51 M clés
sur ces trames ; ce ratio ne se transfère pas à des passages LiDAR
superposés. Le catalogue arrive déjà strictement trié par `BallKey`
dans l'appel de chaîne. `50690c12` le certifie par balayage et évite
le tri redondant de `by_key` ; l'API générale trie toujours un catalogue
non ordonné et le tri distinct `by_level` reste nécessaire. Mesurer le
gain FULL et le pic de résidence simultanée. L'[analyse FULL](CONTRE_AUDIT_B_FULL_COUTS_ET_INTERFACES_20260922.md)
et la [piste de préfixe d'intrus](INTRUS_FULL_PREFIXE_EXACT_20260923.md)
documentent les autres postes ; le préfixe ne mérite un cache qu'après
mesure des répétitions par clé et worker.

`458fb0ed` porte désormais `anchor_meb_proposed` : la proposition Welzl
flottante est vérifiée par les formes et puissances entières, puis le
support de référence est repris sur le bord exact. Le gate différentiel
juge **28 956 ensembles** et tue le mutant sans canonisation ; le port
rapporte localement **154 → 70 Gcycles MEB** et **24,7 → 18,9 s** pour
la tour 08/000000/K10/W8 ; R7b mesure ensuite l'ablation de sa version
`8e8b83a3` sur G4. Une
[contre-épreuve FENV](check_meb_proposed_fenv_20260923.cpp) indépendante,
compilée `-O2 -frounding-math -fno-fast-math` contre le header publié
(SHA-256 `de54655393b09182…`), compare encore **42 544** cas sous
quatre arrondis et FTZ/DAZ activés ou non, sans divergence de
clé, niveau, support, coquille ou statut ; ce sidecar ne remplace pas
une porte FENV intégrée. Voir la [preuve du support
canonique](MEB_PROPOSITION_EXACTE_20260923.md). `78e94b04`
publie maintenant les compteurs `proposals/verified/canonical/fallbacks`
dans le JSON et un levier `tower_meb_proposal` ON/OFF. Le nouveau
préflight exige une proposition vérifiée quand ce levier est actif ;
R7b en donne désormais l'ablation G4, résumée plus haut, sur le paquet
`8e8b83a3`. L'ordre Welzl inverse
`power_order` dans le paquet `8e8b83a3`, alors que la récursion insère
dans l'ordre du tableau. `8fa03046` passe à une proposition
move-to-front avec les extrêmes en tête : la coordination rapporte
**43,1 → 15,8 Gcycles** locaux pour Welzl seul, sans reçu G4 sur ce
nouveau code. La [contre-épreuve FENV
actualisée](MEB_PROPOSITION_EXACTE_20260923.md) compare **42 544** cas
au header publié sous quatre arrondis et FTZ/DAZ, zéro divergence ;
aucune borne de coût générale ni garantie « espérée linéaire » ne suit
d'un ordre déterministe sur les facettes.

`02d55856` remplace les recherches binaires de clé FULL par une table
exacte à adressage ouvert, construite en parallèle puis lue après jonction.
Les collisions sont départagées par la clé entière ; aucune case occupée
n'est effacée. À 5,51 M boules, la capacité de 16 777 216 identifiants
`u32` ajoute **64 Mio** à `by_key`, toujours conservé. L'essai local
alterné 08/000000/K10/W8 passe de **18,5/17,7 s à 16,9/16,4 s** pour la
tour, avec digest égal ; la
[contrelecture indépendante](CONTRE_AUDIT_B_INDEX_CLES_FULL_20260923.md)
rejoue quatre portes ciblées, dont le mutant « clé absente » tué
causalement. Une seconde archive complète du pin `02d55856` inscrit
**128** tests `gate` et passe les portes FULL séquentielle et statique
même avec hachage forcé constant ; le mutant échoue. Le **127/127**
annoncé dans la coordination n'est pas le décompte de cette archive
complète. `ec6d1b74` vide désormais `key_slots` au début de
`Builder::finish()`, après la dernière recherche et les jonctions :
**64 Mio** de stockage logique sont libérés à 5,51 M boules avant banque
et encodage ; l'effet RSS réel reste à mesurer. R7b exécute le paquet
antérieur et ne mesure pas ce changement. Publier construction, sondes réussies et
absentes, longueurs de chaînes, RSS de pointe et ablations W1/W48 :
à plusieurs dizaines de millions de points, le coût total et la
résidence décident de la pertinence de cette table.

`75f27eee` remplace dans FULL la fusion série du tri des requêtes par
des seaux répartis et triés en parallèle, et parallélise concaténation
et détection des groupes. La [contrelecture du tri
FULL](CONTRE_AUDIT_B_SAMPLE_SORT_PUBLIE_20260923.md) confirme l'ordre
total, **400/400** cas en Release et la porte Clang ASan/UBSan, avec
mutant tué ; R7b exécute ce port sans ablation du tri. Le prélèvement à
positions fixes ne garantit pas un partage utile : un témoin W48
strict de 200 003 clés met **96,94 %** des éléments dans un seul seau.
Le nombre de workers créés ne borne donc pas le temps du plus gros
tri. `ec6d1b74` remplace les positions périodiques par un tirage
pseudo-aléatoire déterministe et ajoute ce témoin à la porte d'identité ;
il ne publie pas encore l'occupation des seaux ni un gain G4 apparié.
Le risque de déséquilibre en pire cas subsiste sans borne de taille
des seaux. Publier tailles non vides/maximales des seaux et CPU/mur par étape
sur les requêtes FULL LiDAR, W1/W8/W48, avec RSS réel : le second
tampon, `bucket_of` et les offsets coexistent au-delà du seul
`2×capacity×sizeof(Request)` annoncé pour les requêtes.
Une contre-épreuve isolée du paquet périodique `8e8b83a3` sur
08/000000 entier sans sol K10/s8 ne voit **aucun** déséquilibre massif :
les neuf tris de requêtes ont 32/32 seaux utiles à W8 et 192/192 à
W48, maxima **1,27–1,70×** la moyenne ; les objets et digests sont
égaux. Cette mesure locale n'est ni un reçu G4 ni une mesure des
séparateurs pseudo-aléatoires de `ec6d1b74`.

La [réduction de la phase A en graphe
temporel](PHASE_A_GRAPHE_TEMPOREL_20260923.md) retrouve exactement
composantes, parents et IDs des lots par coupes de niveau, si les égalités
sont fermées ensemble. Elle ouvre une voie de composantes parallèles dans
un ordre K, sans coût ni gain prouvé. La sonde locale 08/000000/K10
compte seulement **1,72 %** des blocs dans les lots groupés : le seul
parallélisme *au sein d'un lot* toucherait donc peu de blocs sur cette
trame, sans que cette fraction mesure sa part des cycles. Une variante
à têtes physiques garde les ancres historiques, mais son ablation
appariée sur le même catalogue rend la même tour octet par octet sans
gain stable et avec **54–159 Mio** de RSS supplémentaire : ne pas la
porter sur ce seul régime.

Le [lemme du maximum d'ID](PHASE_A_MAX_ID_COMPOSANTE_20260923.md) donne
une reconstruction hors des barrières de niveau : dans une composante
au seuil ouvert, la racine canonique est le plus grand ID de création
qu'elle contient. Quatre fixtures et 3 000 historiques abstraits
reproduisent groupes, parents et IDs, mais ne testent pas le produit.
La construction quasi linéaire d'une hiérarchie de composantes pondérée,
avec requêtes aux seuils ouvert/fermé, reste le verrou avant tout port.

## Portes de preuve encore ouvertes

Le lecteur des reçus a progressé : `e5688680` impose les identités
physiques des voies, du cache, des coquilles et du cœur ; `cc4664e5`
lie la marque et le calendrier archivés aux **valeurs exactes** déjà
vérifiées par l'hôte. `a5872918` répare les appels du selftest à cette
nouvelle interface. Sur contenu figé, les **21/21 selftests** passent en
Python normal puis sous `-O`, avec les mutations de date future et de
calendrier plausible refusées. Ces portes hors GCP ne certifient pas
encore un reçu R6 ; la contrelecture des sorties brutes R5 reste
positive et R2 demeure refusé. Les [contre-fixtures
v5](CONTRE_AUDIT_B_G4_RECEPTION_V5_20260923.md), les [identités
v6](RECEPTION_V6_IDENTITES_MANQUANTES_20260923.md) et la
[relecture v8](CONTRE_AUDIT_B_RECEPTION_V8_GARDES_WIP_20260923.md)
gardent l'historique sans être des défauts du lecteur publié courant.

`78ce9fd4` ferme les deux derniers écarts protocolaires suivis : le
premier cas doit activer les cinq leviers que le préflight exercera,
et le lecteur contrôle aussi le résumé exact de chaque cas tué dans
une réception `partial`. Un fichier absent rend un refus typé. Sur
ce commit figé, **21/21 selftests normal et 21/21 sous `-O`** passent,
dont les mutations ON/OFF de plan et suppression/altération du résumé.
R6 apporte maintenant cette preuve de réception pour le snapshot.

`50690c12` remplace la fusion série par un **sample-sort parallèle** :
tris des slots, splitters de clés, puis tris de plages possédées ; une
clé de frontière entière va dans la même plage par `lower_bound`.
Ce n'est **pas** le `parallel_sort(all)` à tampon unique d'un chantier
mutable précédent. Les slots et plages restent simultanément résidents
durant la collecte ; mesurer le maximum par plage, la crête RSS, le temps
et les échecs de ressources sur le chemin de chaîne. Le fast-path FULL
rejuge strictement toutes les clés. Les portes locales passent, sans
reçu G4 ni ablation FULL de ces changements. Le [contre-audit du
sample-sort publié](CONTRE_AUDIT_B_SAMPLE_SORT_PUBLIE_20260923.md)
sépare ce chemin du tri global WIP abandonné avant publication.
Le prélèvement de splitters prend jusqu'à `16×4W` clés **par slot non
vide**, sans pondérer par sa taille : une répartition très inégale des
présentations peut laisser une plage beaucoup plus grosse que la
moyenne malgré `presentation_ranges>1`. Un mutant compilé produit deux plages,
dont une vide et l'autre contenant tout, et passe encore la porte :
ce compteur n'est pas un plancher de parallélisme utile. Publier le
nombre de plages non vides, le maximum et
la distribution des tailles de plages sur LiDAR ; comparer, si ce
déséquilibre apparaît, un échantillon pondéré ou une partition en deux
passes suivie d'un seul tri par plage. L'ordre exact ne dépend pas de
la qualité des splitters, seul le coût en dépend.
Le [shadow de distribution](AUDIT_DISTRIBUTION_SAMPLE_SORT_20260923.md)
mesure sur 08/000000 sans sol K5/W8 **32/32 plages non vides** et un
maximum de **1,52 à 1,59 fois la moyenne** malgré des slots déséquilibrés
par un facteur 5,40 : le déséquilibre redouté n'est donc pas observé
sur cette trame. Il révèle en revanche le coût du double tri : avec les
**mêmes** séparateurs et une suite finale de 1 306 699 présentations
égale élément par élément, l'histogramme/scatter des slots bruts suivi
d'un seul tri par plage enlève **31 077 584 comparaisons complètes**, au
prix de **6 533 495 comparaisons de clés** de classification et d'un
tampon/scatter. Le choix d'un échantillon global pondéré reste non
mesuré ; les comparateurs ne sont pas un temps G4 ou un gain FULL.
Pour comparer un futur reçu à R6, ajouter `times_ms.digest` à
`chain_total` sur le périmètre mural ancien. Le nouveau `chain_cpu_s`
exclut également le condensé, mais aucun `digest_cpu_s` n'est publié :
une comparaison CPU·s R6/R7 brute serait trompeuse. `6200bb5a` borne
désormais `read + chain_total + digest` par le mur externe et tue la
mutation qui plaçait `read=3 600 000 ms` sous un mur externe de 60 s.
Il classe `std::length_error` et `std::system_error` comme manque de
ressources, chronomètre fusion et recensus même en cas d'échec et vide
les résumés d'ordres sur refus ; la porte de chaîne exerce une panne de
lancement après q3/q4. Ce correctif n'a pas de reçu G4.

Prochaines mesures : mêmes octets et masque figé, trames **entières** de
plusieurs séquences sans sol puis brutes, s8/10/12, K5 et K10, W1/W24/W48,
profil float32 et grille fine **séparés**. Les sept morceaux spatiaux
1 mm du [reçu v8 LiDAR](../../morsehgp3D_v8/receipts/lidar_ground_20260921/README.md)
ont maintenant des chronos v12 locaux sur les trois scènes sans sol de
la séquence 08 ; reproduire sur les trames brutes et d'autres séquences.
La décimation emboîtée 1/4–1/2–1 est maintenant mesurée sur les sept
secteurs de chacune des trois scènes, avec une seule graine ; répéter
sur d'autres graines et séquences, puis sur les trames brutes avec sol.
Ni les morceaux ni les décimations ne valident le contrat de trame entière.
Publier travail amont,
formes et atlas, candidats
résiduels, coquilles, catalogue, sorties FULL, CPU/mur et RSS par phase,
y compris les échecs et les replis exacts. Une exécution GPU de toute
la tour, avec transferts, buffers résidents et retours CPU exacts, reste
entièrement à construire et à juger. Verdict public : **`not_claimed`**.
