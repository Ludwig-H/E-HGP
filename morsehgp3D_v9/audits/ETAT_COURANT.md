# État courant des audits v9

23 septembre 2026. Produit publié courant : **`f55ea40c`**. Le
[reçu G4 R6](../receipts/g4_tower_r6_20260923/README.md) exécute le
snapshot **`78ce9fd4`** ; ses temps ne qualifient pas encore le
sample-sort, le raccourci FULL ni la nouvelle frontière temporelle
de la sonde v9. Cadre :
`exploration_v9_hors_registre`, `reference_cpu`,
`quantized_u18_input_only`, **`not_claimed`**. Ce fichier porte le verdict
mutable. Les notes datées conservent preuves, contre-exemples et reçus.

**Session G4 interrompue le 23 septembre, 05 h 35 UTC.** L'utilisateur a
choisi l'arrêt de la session SPOT envisagée : le premier diff de la sonde
MEB v3 annonçait un schéma que le validateur v2 aurait rejeté. Relecture
GCP ciblée à ce moment : l'instance de campagne
`ehgp-v7-4fa0e0789a7d5bb06b787d35` est déjà `TERMINATED` (dernier
arrêt 04 h 50 min 54 s UTC) et aucune des autres instances SPOT du projet
n'est en cours ; **aucun stop supplémentaire n'a été lancé**. Le commit
`458fb0ed` aligne les littéraux sonde/worker/selftest en v10 et MEB v3 ;
`78e94b04` publie le levier ON/OFF et ses quatre compteurs dans le JSON
jugé, sans mesure G4. Sa
[rupture de schéma v10/v5](PROTOCOLE_TOUR_V10_V5_RUPTURE_20260923.md)
est fermée au pin `f55ea40c` : sonde v11, plan v6 et mutations des
anciens libellés dans le selftest. Sur ce pin stable, **21/21 selftests**
passent en Python normal et sous `-O` en contrelecture locale. Un nouveau
plan épinglé et ses portes restent nécessaires avant une campagne de calcul.
Le WIP v6 C6/tri observé en parallèle
est [contrelu séparément](CONTRE_AUDIT_B_WIP_V6_C6_TRI_20260923.md) :
il n'est pas raccordé à v9 ni qualifié sur u18/G4.

La [tentative G4 R7](../receipts/g4_tower_r7_stockout_20260923/README.md)
du 23 septembre à 05 h 46 UTC, sur le pin `78e94b04`, a reçu un refus
GCE `resource_availability/STOCKOUT` avant tout démarrage du worker.
Le manifeste et ses douze sommes SHA sont cohérents ; la lecture seule
retrouve la VM `TERMINATED` avec `lastStartTimestamp` inchangé depuis R6.
Le paquet `PACKAGE.json` décrit la préparation (`GCP_used=false`), le
reçu hôte la demande réellement envoyée ; **aucun chrono, aucune
ablation MEB ni résultat FULL/GPU** ne vient de R7.

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
le reçu. La première tentative R4, [préemptée avant le
worker](CONTRE_AUDIT_B_G4_R4_PREVOL_20260923.md), ne donne aucun chrono.

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
([fixture B](CONTRE_AUDIT_B_PREATLAS_ET_Q3_20260922.md)).
Le [cover commun par blocs](CONTRE_AUDIT_B_COVER_BATCH_20260923.md)
et la [saturation K−2 des seules arêtes q4](SEUIL_SATURATION_ATLAS_PAR_VOIE_20260923.md)
restent des pistes secondaires à mesurer avec le coût aval complet.

## FULL, sortie explicite et résidence

`684d8fc7` construit les ordres K en parallèle, publie les images
verticales après leurs lots et déplace les populations vers une banque
partagée. `84c74a5e` conserve le travail payé en cas d'échec et choisit
le plus petit K en échec entre les phases A/C ; deux mutants ciblés sont
tués, sans nouveau reçu G4 sur ce commit. Une phase de préparation/tri
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

Même si q3/q4 **et** la tour devenaient gratuits, la meilleure répétition
R5/K10 laisserait encore **2,50 / 3,39 / 3,44 s** de chaîne sur
000100 / 000000 / 000200 ; q2+fusion+recensus représente déjà
**1,43 / 2,02 / 2,06 s**. La queue après `tower_ms` comprend le résumé et
le digest sur les snapshots R5/R6. `50690c12` isole désormais le digest
dans `times_ms.digest` ; il reste **synchrone** dans l'appel public.
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
la tour 08/000000/K10/W8, sans reçu de calcul G4. Une
[contre-épreuve FENV](check_meb_proposed_fenv_20260923.cpp) indépendante,
compilée `-O2 -frounding-math -fno-fast-math` contre le header publié
(SHA-256 `de54655393b09182…`), compare encore **42 544** cas sous
quatre arrondis et FTZ/DAZ activés ou non, sans divergence de
clé, niveau, support, coquille ou statut ; ce sidecar ne remplace pas
une porte FENV intégrée. Voir la [preuve et ses portes
ouvertes](CONTRE_AUDIT_B_MEB_PROPOSE_WIP_20260923.md). `78e94b04`
publie maintenant les compteurs `proposals/verified/canonical/fallbacks`
dans le JSON et un levier `tower_meb_proposal` ON/OFF. Le nouveau
préflight exige une proposition vérifiée quand ce levier est actif ;
l'ablation G4 complète reste à faire. L'ordre Welzl inverse actuellement
`power_order` alors
que la récursion insère dans l'ordre du tableau ; mesurer les deux ordres
sur les mêmes facettes avant de le choisir pour le coût.

Le [filtre négatif de clés](FULL_FILTRE_ABSENCE_CLE_20260923.md) propose
d'éviter le `lower_bound` de `static_terminal` quand la clé MEB est
assurément absente du catalogue immuable. Ses collisions ne changent
jamais le résultat ; construire et sonder le filtre a cependant un coût.
R6 compte **11,309 M recherches** à K10 sur 08/000000, sans publier le
nombre de clés absentes ni leur temps : l'histogramme par K et une
ablation FULL/chaîne décident de l'intérêt réel.

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
sépare ce chemin du [tri WIP abandonné](CONTRE_AUDIT_B_TRI_FUSION_WIP_20260923.md).
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
[mutation d'une heure de lecture](CONTRE_AUDIT_B_TRI_FUSION_WIP_20260923.md).
Il classe `std::length_error` et `std::system_error` comme manque de
ressources, chronomètre fusion et recensus même en cas d'échec et vide
les résumés d'ordres sur refus ; la porte de chaîne exerce une panne de
lancement après q3/q4. Ce correctif n'a pas de reçu G4.

Prochaines mesures : mêmes octets et masque figé, trames **entières** de
plusieurs séquences sans sol puis brutes, s8/10/12, K5 et K10, W1/W24/W48,
profil float32 et grille fine **séparés**. Les sept morceaux spatiaux
1 mm déjà figés dans le [reçu v8
LiDAR](../../morsehgp3D_v8/receipts/lidar_ground_20260921/README.md)
permettent un diagnostic de croissance pré-déclaré, jamais une validation
de trame entière. Publier travail amont, formes et atlas, candidats
résiduels, coquilles, catalogue, sorties FULL, CPU/mur et RSS par phase,
y compris les échecs et les replis exacts. Une exécution GPU de toute
la tour, avec transferts, buffers résidents et retours CPU exacts, reste
entièrement à construire et à juger. Verdict public : **`not_claimed`**.
