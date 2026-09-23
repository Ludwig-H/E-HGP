# État courant des audits v9

23 septembre 2026. Code produit courant sur `origin/main` :
**`e0ae05a7`** (préparation/tri FULL statiques parallèles et erratum du
reçu q3/q4), après le certificat de voies mortes et protocole v5
`099ca784` puis le selftest `b4e480fc`. Le reçu G4 R3 exécute **ce dernier
snapshot**, pas le nouveau port FULL. Le census q3 sur feuille et la sonde
v4 venaient de `e54f727c` ; le reçu G4 R2 reste épinglé au code
antérieur `0b29b6c3` et le reçu G4 R1 au paquet `e28296bb`. Noyau MEB à
`ad2d0ebb`, atlas saturant et sonde v3 à `e6405952`, défaut FULL statique
à `0b29b6c3`. Cadre :
`exploration_v9_hors_registre`,
`reference_cpu`, `quantized_u18_input_only`, `not_claimed`. Ce fichier est le
verdict mutable du dossier ; les notes datées conservent les démonstrations et
références. Les auditeurs écrivent dans `audits/` et communiquent au
constructeur uniquement des constats utiles.

## Verdict sur le premier moteur

La chaîne générateur v8 porté → catalogue de BallKeys recoupées → tour FULL
v7 portée existe. Elle recalcule clé, niveau, intérieur et coquille de chaque
boule **émise**, vérifie `q_min` pour les coquilles étendues et refuse
transactionnellement une coquille de plus de 12 sites. La forme q4 est
indépendante de l'orientation du support. La contrelecture n'a pas trouvé de
défaut concret sur ces chemins ; voir le [contre-audit A du
moteur](CONTRE_AUDIT_A_MATH_MOTEUR_20260922.md) et la [lecture B de
FULL](CONTRE_AUDIT_B_FULL_COUTS_ET_INTERFACES_20260922.md).

`e6405952` active par défaut dans la chaîne le certificat terminal q4
`inside≥K−1` et filtre le tri des niveaux FULL en arrondi au plus proche,
avec repli exact pour les niveaux proches. La [contrelecture
B](CONTRE_AUDIT_B_PORT_V3_SATURATION_TRI_20260923.md) ne trouve pas de
défaut d'exactitude dans ces deux chemins ; le commentaire de
`ChainOptions` qui dit encore « désactivée par défaut » est périmé.
L'auteur annonce sur 08/000100 K5/W8 un temps q3/q4 97,7→72,1 s et
des digests K5/K10 inchangés, sans reçu local apparié versionné.
Le ledger v9 contient le travail physique total de partition, préfixes
saturés inclus **une fois**, mais n'exporte pas les compteurs spécifiques
`certificates`, `certificates_with_unvisited_sites`,
`unvisited_site_mass` et `discarded_frontier_ids` : publier ces nombres
pour expliquer le gain, en gardant `saturation_work.prefixes` comme
**sous-ensemble** de `work.partition`, jamais comme terme additionnel.
`0b29b6c3` choisit par défaut le résolveur FULL statique avec W fils si
W>1 ; le test de chaîne compare déjà les payloads exacts W1/W4 sur de
petits nuages. La [contrelecture B](CONTRE_AUDIT_B_PORT_V3_SATURATION_TRI_20260923.md)
montre que le plan G4 passe `--static` explicitement et ne mesure donc pas
ce nouveau défaut. Vérifier en porte le nombre **effectif** publié, le
défaut contre `--static=0`, et refuser toute valeur négative autre que
la sentinelle `-1` ; aucun nouveau chrono G4 n'est acquis.

Un rejeu indépendant local de `d2700314` passe **20/20 CTests** sans saut ;
B retrouve ces 20 portes en Release et sous Clang ASan/UBSan. Le commit
`ba762036` ajoute une porte arithmétique u18 et un CTest du mutant de census
(22 portes annoncées par le développeur). `e28296bb` annonce 28 CTests,
dont trois mutants T2 et trois refus CLI ; ces nouvelles portes n'ont pas
été rejouées indépendamment ici. La cible arithmétique reste compilée en
mode produit : son mutant `level-trunc-hi` est inactif, donc sa valeur
d'oracle numérique est distincte d'une preuve de mutant tué.

Deux coquilles exactes u13 (une mixte, une q4 pure) obtiennent le refus
`chain_shell_above_12`, sans tour ni catalogue partiel publiés. Une fixture
q4 u12 exerce l'appel public `run_tower=true` : six configurations
W1/W4 × s8/s10/s12 rendent le même digest et la fusion attendue de **trois
parents à K10**. Ces contrôles ciblés n'ont pas encore de reçu versionné.
Les trois mutants T2 (`assignment`, `open`, `adjacency`) sont désormais
inscrits à CTest avec causes exactes. La sonde refuse K hors 1..10 avant
rétrécissement et limite `--grid=` à un alphabet sûr ; les deux défauts
reproduits dans le [contre-audit B](CONTRE_AUDIT_B_U18_ET_SONDE_20260922.md)
sont corrigés sur ce chemin. Le libellé `--grid=1mm` ne certifie toujours
pas la préparation : lier un manifeste d'entrée au pas réel.

Le juge T2 compare l'inventaire exact des boules et la tour Γ sur de petits
nuages (`n≤14`) ; depuis `e28296bb`, il compare aussi la tour publiée par
`run_tower_chain(..., run_tower=true)` en W1/W4. Il ne démontre pas
l'absence d'une BallKey complètement omise sur une grande trame.
`run_tower=false` rend aussi
`complete_relative` avec zéro ordre : tout lecteur de contrat doit exiger
`run_tower=true`, les ordres K=1..Kmax et un objet FULL cohérent.

Le [premier reçu FULL](../receipts/first_tower_20260922/README.md)
contient six exécutions locales, une par trame 08/000000, 000100, 000200
sans sol à 1 mm et par K5/K10, s8/W8. Mur K5 : **143/132/264 s** ;
K10 : **523/381/802 s**. q3/q4 prend 73–93 % et l'aval FULL 94–130 s
à K10. Les 30 hashes du manifeste et les trois entrées concordent ;
les comptes des cinq premiers ordres de K5 et K10 sont identiques pour
chaque trame. Cela certifie une base de coût **relative aux clés émises**,
pas la complétude exhaustive sur grande trame, ni l'égalité des tours par
ordre au seul vu des comptes. La [contrelecture B du
reçu](CONTRE_AUDIT_B_PREMIERE_CAMPAGNE_20260922.md) et l'[actualisation
A](CONTRE_AUDIT_A_MESURES_PLAN_20260922.md) détaillent les limites du
lecteur et les corrections du README. La chaîne ne publie que quatre
compteurs du registre q3/q4 pourtant disponible et matérialise deux
capacités complètes de présentations lors de la fusion.
Le [calcul de résidence B](CONTRE_AUDIT_B_RESIDENCE_CHAINE_20260922.md)
identifie aussi un cache temporel optionnel de 48·nextpow2(16n) octets
(24 Gio à 30 M sites), sa remise à zéro par ordre, et au moins 216n octets
de sortie K1. Ce sont des planchers ou capacités logiques, pas un RSS
mesuré ; ils imposent une résidence de travail maîtrisée et une
représentation de sortie adaptée pour les dizaines de millions de sites.

La [première session G4](../receipts/g4_tower_r1_20260922/README.md) est
réelle, clôturée et **CPU seulement** : huit cas complets du paquet
`e28296bb`, sur trois trames sans sol de la séquence 08, s8, une exécution
par cas. À W48, la tour K1..5 prend **18,81 / 15,05 / 29,25 s** et K1..10
**111,68 / 82,31 / 125,44 s** ; 000000/K10 descend à **70,00 s** en FULL
statique W48. Les six cas communs ont `generator`, `catalogue`,
`tower_work` et `orders` **statistiques** exactement égaux aux reçus locaux,
pas seulement le condensé ; les clés individuelles ne sont pas exposées.
Sous l'hypothèse du FULL non statique en un fil, le reste de
la chaîne consomme en moyenne **42,6–44,6 des 48 CPU logiques** : son
coût tient davantage au travail payé qu'à un manque évident de threads.
La voie FULL statique modifie aussi les compteurs MEB/cache : son gain
75,90→34,14 s n'est pas un facteur de parallélisme pur.
Ce reçu n'est ni GPU, ni brut, ni multi-séquence, ni s10/s12 ;
ces tailles voisines ne prouvent aucune pente sous-quadratique. Voir la
[lecture des mesures](CONTRE_AUDIT_B_PREMIER_G4_20260922.md) et la
[contrelecture du protocole](CONTRE_AUDIT_B_G4_R1_ET_SCHEMA_V2_20260922.md).

La [session G4 R2](CONTRE_AUDIT_B_G4_R2_PREFLIGHT_20260923.md) sur le
paquet `0b29b6c3` est close avec `worker_failed/probe_failed` : les
treize sondes invitées ont calculé et rendu code 0, mais le worker a
refusé les treize JSON sur `tower_work`. L'arrêt ciblé est certifié ;
**aucun reçu G4 accepté** n'en découle. Les médianes W48 des chronos
bruts de chaîne K5 sont 13,252/10,455/19,946 s et K10
50,071/37,317/64,289 s pour 08/000000, 000100, 000200.
Sur 000000/K10, le registre brut annonce 17,947 milliards de tests
ponctuels d'atlas q4 et 12,392 milliards d'insertions d'IDs de nœuds
frontaliers : le travail total, et pas seulement le nombre de workers, doit
changer. Ces diagnostics exploratoires ne qualifient ni 1 s ni GPU.
Sur les six cas communs avec R1, les sorties brutes R2 gardent mêmes
entrée, compteurs générateur publiés, catalogue, ordres et digest ; q3/q4 y est
**24–33 % plus rapide**. C'est un indice favorable à la saturation
profonde, avec sessions et options aval différentes, sans ablation
contrôlée ni promotion du reçu R2 refusé.

`e54f727c` ajoute le census q3 sur fragment exact de l'atlas q4 et le rend
actif par défaut dans la chaîne. Une feuille profonde sans fragment complet
reste un simple minorant et retombe sur le census global. Le petit juge
`wspd_q34` du build développeur a été rejoué directement : PASS, 60 appels
en mode feuille, 6 088 census feuille, 1 846 rejets et 44 300 tests
ponctuels ; `q4_local` passe aussi. Le rejeu de **tout CTest** sur ce
build donne **101/101 PASS**, une mutation héritée explicitement
`Disabled`, en 15,29 s avec quatre jobs. Les binaires du build développeur
ont été réutilisés : ce n'est ni une compilation indépendante sous
sanitizers ni une mesure LiDAR appariée. En complément, une **compilation
neuve Clang 18 ASan/UBSan** du seul juge `wspd_q34` et de ses dépendances
dans `/tmp` passe 19 903 vérifications, dont la branche feuille ; elle
ne rejoue pas les 101 autres portes sous sanitizers. La
[contrelecture B](CONTRE_AUDIT_B_Q3_FEUILLE_WIP_20260923.md)
donne la fixture K3 avec un intérieur et quatre contacts : l'invariant
géométrique est cohérent, mais son diagnostic LiDAR non versionné expose
610,29 M nouveaux tests ponctuels q3 sur feuille. Les gains globaux restent
à mesurer avec le coût de coquille et les octets de fragments retenus.

Le chiffre v8 de 104,63 s portait sur le seul flux q3/q4 en mode digest :
aucune régression ni accélération v9 ne se déduit de cette comparaison non
appariée.

## Priorités de preuve et d'optimisation

1. **Rendre le reçu rejouable et explicatif** : lier les SHA d'entrée,
   le masque/grille, le binaire et toutes les options à chaque ligne,
   vérifier les préfixes K5/K10 par digest d'ordre, et créer le marqueur
   de campagne seulement après six succès. Les trois entrées sans sol
   sont versionnées dans le [reçu v8 `lidar_ground_20260921`](../../morsehgp3D_v8/receipts/lidar_ground_20260921/README.md).
   Publier les registres déjà calculés : tests/copies de partition d'atlas, rejets q3,
   travail et attente par worker, `merge`, census, quotient et résolveur
   FULL. Ne pas confondre les sommes de temps worker avec le temps mur.
   Le reçu G4 R1 satisfait cette porte pour son paquet **ancien**
   `e28296bb` : huit cas achevés, sources et entrées recoupées, arrêt ciblé
   certifié. Il ne qualifie pas les nouveaux défauts. À `0b29b6c3`, le
   worker v3 refusait les valeurs `meb_accounting` (chaîne) et
   `meb_supports_by_size` (tableau) du vrai `tower_work` : c'est la cause
   précise du refus de R2. `e54f727c` introduit un worker v4 typé, épingle
   les modes de saturation/feuille et inscrit à CTest une vraie sonde native
   jugée par ce worker, avec onze mutations. Les 18 selftests du protocole
   et ses deux portes natives normal/`-O` passent en rejeu local ; aucun
   nouveau cas G4 n'en découle. Les défauts v4 reproduits dans la
   [contrelecture B](CONTRE_AUDIT_B_PROTOCOLE_V4_WIP_20260923.md)
   concernaient schéma FULL incomplet, préflight facultatif, tolérance de
   mur externe d'une seconde et campagne `partial` sans tour complète.
   La révision **v5 publiée à `099ca784`** impose maintenant les clés et types
   exacts, histogrammes de longueur fixe, un préflight natif avant LiDAR,
   une tolérance de 0,05 s et au moins un cas complet à la réception. Elle
   recertifie aussi les blobs depuis le commit annoncé, en réponse au
   [contre-test de provenance](CONTRE_AUDIT_B_G4_R1_ET_SCHEMA_V2_20260922.md).
   Sur `b4e480fc` figé, la suite Python de cycle de vie v5 passe
   **20/20 tests** en rejeu indépendant (59,9 s), et la porte native du
   probe passe ses **19 mutations** ciblées. Ces réussites ne contiennent
   pas les identités de masse ci-dessous.
   Une compilation indépendante des sources, recoupées par SHA avec le
   commit publié, permet de rejouer le vrai préflight de 1 500 sites/K5/W2 :
   il donne `complete_relative` avec
   **23 848 voies q3 et 26 428 voies q4 prouvées mortes** : les deux branches
   sont exercées. Mais `validate_probe` accepte encore, sur une copie de
   cette sortie réelle, chacune des six mutations isolées qui mettent à
   zéro `dead_loads`, `dead_form_sites`, `dead_q3_open`, `cover_builds`,
   `generator.q34_expanded_pairs` ou `catalogue.q3_presentations`.
   Sur une réponse `complete_relative`, les identités testables sont notamment
   `expanded_pairs=cover_builds+witness_rejected_pairs`,
   `dead_loads=cover_builds`,
   `dead_form_sites=cover_sites−2·dead_loads`,
   `dead_q3_open=q3_edges`, `dead_q4_open=q4_edges` quand l'option
   `dead_lanes` est active,
   `dead_qi_proved+dead_qi_open≤dead_loads` pour chaque voie,
   et les présentations par arité du catalogue égales aux émissions du
   générateur. La fausse sonde du selftest publie elle-même
   `dead_loads=1` avec `dead_q3_proved+dead_q3_open=2` : lui donner un
   ledger cohérent avant d'imposer ces relations, puis tuer leurs mutants
   en selftest avant un reçu G4 v5. Le préflight indépendant repose sur des
   objets `/tmp` non versionnés et ne qualifie pas encore une session G4 ; ablater
   séparément les trois options sur les trames entières. Le lecteur doit
   encore certifier la fermeture du groupe de toute commande tuée, même en
   campagne partielle. Le nouveau [contre-audit B de la
   réception](CONTRE_AUDIT_B_G4_RECEPTION_V5_20260923.md) reproduit en
   outre quatre acceptations de paquet incohérent : groupe censuré non
   fermé, identité cible/génération/provenance altérée, préflight à travail
   nul, stderr GNU time de préflight invalide. Les 20 selftests passés ne
   couvrent pas ces mutations. Ne pas convertir les sorties R2 refusées en
   reçu accepté.
2. **Portes causales et entrée** : rejouer les 28 portes de `e28296bb`
   indépendamment ; les portes MEB et FULL ciblées du nouveau noyau passent
   déjà en Release et sous ASan/UBSan, mais pas une campagne appariée LiDAR.
   Comparer la restriction sémantique des ordres K1..5 de K10 à la tour K5
   sur les mêmes octets, après égalité des catalogues actifs bas-rang ;
   rendre causal le
   mutant arithmétique u18, ajouter égalités de cellules, générateur porté
   et TSan. Les reçus v8
   R2 de la reprise u18 restaient `failed`
   à cause du lecteur JUnit : la provenance v9 épingle `3f0d188f`, mais
   aucune qualification n'est héritée automatiquement.
3. **Verrou q3/q4 mesuré** : préserver les miniballes k-Gabriel locales,
   le propriétaire et les certificats exacts. L'[audit q4
   A](Q4_STRUCTURE_ET_BORNES.md) propose un test d'absence de graine aiguë
   possédée **avant** le cover ; sans graine, q3 et q4 sont vides sur l'arête.
   Une borne locale de `m(K−2)` centres q4 peu profonds pour `m` droites
   distinctes motive un catalogue de centres. Sur les `s` droites de
   graines aiguës, une sélection top/bottom de racines exactes garde au
   plus `2(K−2−p_λ)` événements par droite en `O(sKm)` après regroupement ;
   un déterminant factorisé tient en i128 sous les bornes u18. Pour `s≈m`,
   deux familles de niveaux peu profonds proposent une sélection locale
   `O(mK polylog m)` sur modèle de comparaisons exactes. Le [contre-audit
   B](CONTRE_AUDIT_B_Q4_NIVEAUX_ORIENTES_20260922.md) propose une
   perturbation sortante qui préserve les strates dégénérées ; l'[oracle
   rationnel A](check_q4_outward_levels_20260922.py) retrouve les 5 926
   centres peu profonds de 2 004 arrangements dégénérés après rabattement
   (profondeurs 0..2). B a reproduit **1 250 cas supplémentaires** à
   profondeurs 3..7, couvrant localement le seuil K10 ; aucun constructeur
   de niveaux ni moteur q4 n'est jugé par ces scripts.
   Ni le port symbolique, ni son coût réel, ni le census ne sont acquis.
   Le reçu 1 mm compte **2,779 milliards d'incidences site–cover logiques**
   cumulées sur les arêtes pour `n=39 885`, déjà davantage que `n²`.
   Le brut R2 à K10 monte à **7,805 milliards** ; les seules arêtes q4
   représentent au moins **5,758 milliards**, soit plus de `3,6n²`
   ([calcul B](CONTRE_AUDIT_B_Q4_SHALLOW_20260922.md)). Construire les
   niveaux en lisant chaque cover entier déplacerait donc le verrou ; ces
   populations additionnées sur des nœuds certifiés ne sont ni `Σh` des
   droites q4 ni des visites physiques du moteur actuel. Sur la ligne 1 mm,
   la construction du cover et sa redécomposition dans l'atlas q4 comptent
   respectivement **440,194 M** et **315,737 M** visites. Census global et test
   `centre∈conv(coquille)` restent obligatoires ; le catalogue ne remplace
   pas automatiquement les présentations positives. La ligne v8 1 mm
   compte **171 444 arêtes q3 seules et 16,12 M census** que la réutilisation
   d'atlas q4 ne touche pas. Dans le raccord v9 actuel, `GlobalBoxes`
   recense ces q3 dans l'index global : la construction du cover sur les
   arêtes q3 seules n'est pas utilisée pour ce census et mérite une
   ablation exacte avant toute optimisation plus lourde du cover. L'[oracle
   q3 partagé](check_q3_shared_u18_20260922.py)
   vérifie la boîte rationnelle u18, un débordement i128 évité par
   annulation algébrique et une fixture où avancer le curseur Z à travers
   une feuille ambiguë perd un intérieur. La nouvelle fixture exacte à
   quatre graines atteint réellement EOF avec `lower≥0`, quatre supports
   valides et coquilles complètes après saut structurel des endpoints ;
   le futur port u18 doit exercer ce relais avec `relay_sites=2`. La
   [note q3](Q3_STRUCTURE_ET_BORNES.md) décrit le ticket possédé
   `(X,compte,curseur Z)` : une réponse GPU hors ordre ne valide pas un
   préfixe DFS continu. Elle donne aussi une économie immédiate pour le
   port q3 feuille : descendre l'atlas par comparaisons rationnelles aux
   coupures dyadiques plutôt que faire 40 étapes de conversion par centre.
   Le [petit oracle](check_q3_atlas_rational_location_20260923.py) passe
   192 352 centres, frontières et extrêmes inclus ; les 466,02 M
   consultations du reçu brut R2 rendent l'ablation LiDAR pertinente,
   sans gain de temps encore mesuré. Pour les fragments exacts, une
   palette privée de ≤`K−1−inside_count` sites actifs peut réordonner les
   tests q3 et rejeter tôt sans crédit entre graines : l'[oracle
   rationnel](check_q3_leaf_palette_20260923.py) sépare 47 tests dans
   l'ordre spatial d'un rejet en un test après apprentissage sur une
   seconde graine. C'est une fixture `Disk`, pas un gain LiDAR ; mesurer
   d'abord l'occupation `Leaf`/`Deep` et les rejets tardifs. Le relais
   produit reste à
   qualifier. Le [contre-audit
   B](CONTRE_AUDIT_B_PREATLAS_ET_Q3_20260922.md) rappelle que la suppression
   d'une cellule q4 peut aussi enlever un certificat de rejet q3. Sa
   fixture entière prouve même qu'une q4 admise peut survivre quand
   **toutes** ses faces q3 sont rejetées : les graines à parcourir ne
   peuvent pas être limitées aux q3 finalement émises. Un [certificat de
   cover par bloc d'arêtes survivantes](CONTRAT_COUTS_ET_PARALLELISATION.md)
   partage le prédicat exact sur `E×Z`. Réduire les **vrais** extrema des
   arêtes survivantes ne relâche jamais les bornes A×B, mais peut donner
   **exactement les mêmes** bornes ; l'[oracle
   entier](check_cover_batch_u18_20260922.py) passe 1 000 familles et
   deux cas discriminants. Le [contre-audit B](CONTRE_AUDIT_B_COVER_BATCH_20260923.md)
   avait relevé l'absence de test de subdivision `E×Z` : l'oracle couvre
   maintenant exactement 1 000 petites familles survivantes. Il ne teste
   toujours ni le raccord des ranges, ni le coût de matérialisation/réemploi
   des extrema E. C'est une piste secondaire pour les
   440 millions de visites de cover, à
   mesurer après le filtre de paire, sans lui attribuer le coût dominant
   de l'atlas. Un [certificat de domination par
   blocs](DOMINATION_Q4_PARESSEUSE_PAR_BLOCS_20260923.md) peut écarter des
   formes q4 avant l'atlas pour plusieurs arêtes via des gardes plus proches
   sur toute une cellule de centres ; l'[oracle
   entier](check_q4_block_dominance_20260923.py) passe 1 200 boîtes.
   À K10, huit gardes préservent le flux q4 courant, neuf préservent
   **ensemble q3 et q4** si la cellule contient leurs deux familles de
   centres, et dix sont nécessaires à une garantie autonome pour tout
   `q_min≥2`. La borne d'arête positive `|c−m|²≤D/8` contient aussi les
   centres q3 (`≤D/12`) : la note établit maintenant ce cas commun et
   demande une vue filtrée typée avant d'élaguer le cover partagé. **Le
   seuil q4 de huit gardes ne suffit pas pour q3** : une fixture K5
   perdrait l'intérieur qui rejette une boule q3. Une
   [ablation plus locale](SEUIL_SATURATION_ATLAS_PAR_VOIE_20260923.md)
   peut saturer à `K−2` l'atlas des arêtes **q4 seules**, sans changer le
   seuil `K−1` utile aux rejets q3 des arêtes mixtes. Une fixture u18 K5
   sépare exactement les seuils ; le reçu 1 mm contient 326 970 arêtes
   q4 seules, mais pas leur coût distinct. Mesurer les compteurs par masque
   et l'identité FULL avant de prioriser le port. Un certificat de **voie
   morte q3/q4** est publié à `099ca784`, sans mesure FULL appariée à cette
   date : la [note de coût et de preuve](DOMINATION_Q4_PARESSEUSE_PAR_BLOCS_20260923.md#certifier-une-voie-morte-sans-balayer-chaque-cover)
   montre qu'en charger les formes depuis chaque cover ferait de ses
   7,805 milliards d'incidences logiques R2/K10 presque autant de lectures
   physiques. Une petite palette de vrais gardes **adaptée par cellule**
   peut tenter le certificat avant le cover, avec repli exact et budget
   d'effort ; les gardes universels sont déjà traités par le filtre citron.
   La fixture K5 distingue les deux preuves. Le hook du filtre de paire
   doit aussi proposer les petits nœuds **écartés par Xi**, pas seulement
   ses feuilles : ils peuvent contenir tous les gardes non universels.
   `099ca784` met cette option **par défaut dans la chaîne** sans mesure
   FULL LiDAR appariée ; la garder expérimentale jusqu'à une ablation
   on/off qui mesure aussi les formes chargées, succès par arête et coûts
   réellement évités. Sa `docs/PROVENANCE.md` annonce
   **91 % du temps q3/q4** sur des arêtes sans émission et des covers moyens
   de 568 contre 43 sites. Le nouveau [reçu de profil par
   arête](../receipts/q34_dead_edges_20260923/README.md) apporte le harnais
   K5 de 08/000000 et les classes de cycles, avec hashes internes cohérents.
   Une compilation indépendante du juge `wspd_q34` sur le commit publié,
   sous Clang 18 ASan/UBSan/LSan, passe **23 756 contrôles** dont huit appels
   u18 extrêmes ; ses **cinq mutants compilés** sont rejetés pour la cause
   géométrique attendue, sans crash. Cette porte cible le générateur, pas
   FULL ni la performance G4.
   Le tableau du reçu donne **96,69 % des cycles d'arêtes après filtre**
   aux classes sans émission, ou **85,69 % avec le filtre de paire inclus** :
   91 % n'est reconstructible avec aucun de ces dénominateurs. Le cover moyen vaut
   **1 453 contre 43** pour les arêtes *mixtes* mortes/vivantes, et
   **1 704 contre 43** pour toutes les classes mortes/vivantes ; le 568
   de `PROVENANCE` reste sans définition dans cette capture. Indiquer le
   dénominateur et les classes exacts avant de citer une part du temps.
   L'essai linéaire du harnais conserve les mêmes émissions et digest et
   annonce q3/q4 **592→205 CPU·s à K5, 1 600→557 à K10** ; la variante à
   frontière baisse ses visites de 9,94→6,75 G à K5 et 30,1→17,7 G à K10
   sur 000100. À K10, le harnais réduit aussi les tests ponctuels d'atlas
   de 17,947 à 1,483 G et les graines q3 de 472,06 à 34,03 M : le
   **rejet aval est réel dans ce prototype**. Les deux JSON de frontière
   sont `--no-tower` avec digest zéro ; ils ne qualifient pas FULL. Ces
   signaux sont utiles, mais l'archive ne ferme pas encore
   les commandes, environnement et SHA des binaires du prototype : son
   patch imprime une ligne `refine:` absente de la sortie publiée. Ces
   chiffres restent distincts du nouveau reçu G4 apparié ci-dessous.
   Le brut R2 démontre une **proportion d'arêtes q4 muettes** supérieure à
   91 %, pas une fraction de temps. Le [reçu G4 R3
   publié](../receipts/g4_tower_r3_20260923/README.md), snapshot
   `b4e480fc`, apporte enfin six paires **FULL CPU** on/off sur les trois
   trames sans sol de la séquence 08, K5/K10, W48. La
   [contrelecture B des sorties brutes](CONTRE_AUDIT_B_G4_R3_20260923.md)
   recoupe les 14 cas achevés, les masses, hashes, ordres, catalogue et
   arrêt ciblé. Sans/avec certificat, la chaîne passe de
   **12,93→8,89 / 10,12→6,19 / 19,71→10,93 s** à K5 et de
   **48,33→35,56 / 36,29→25,96 / 62,99→37,75 s** à K10. Catalogues,
   ordres et digests publiés sont égaux dans chaque paire ; c'est un gain
   réel pour cet algorithme, pas une complétude exhaustive sur grande
   trame. À K10 la tour aval prend désormais 59–67 % du mur, tandis que
   le prouveur charge encore 4,15–9,28 milliards de formes selon la trame. Ces deux postes
   appellent un port architecturé ; sur 000000/K10 les graines q3 passent
   de 472,06 à 33,35 M et les tests ponctuels d'atlas de 17,947 à
   1,468 G : la réduction aval est mesurée. L'unique comparaison W24/W48
   montre peu de gain mural et plus de CPU/RSS à W48, sans isoler une
   partie intrinsèquement séquentielle : contention, taille des lots et
   déséquilibre restent des explications concurrentes. Aucune borne
   sous-quadratique, ni le contrat de 1 s, ni le GPU ne sont acquis. Le reçu
   reste `complete_relative` ; les lacunes v5 de réception demeurent pour un
   futur reçu arbitraire et `host/lifecycle.txt` reste à `targeted_running`
   malgré la preuve séparée d'arrêt `TERMINATED`.
4. **Aval FULL, grandes coquilles et échelle** : les 12,0 M appels MEB
   de 000000/K10 font 1,065 milliard de tests de puissance ; un test
   exact de la paire la plus éloignée peut éliminer toutes les autres
   paires q2 dans chaque appel, sous la preuve détaillée de l'[actualisation
   A](CONTRE_AUDIT_A_MESURES_PLAN_20260922.md). Le port `ad2d0ebb` a été
   [contrelu par B](CONTRE_AUDIT_B_ANCHOR_MEB_DIAMETRE_20260922.md) ; le
   noyau nominal et son mutant causal passent indépendamment sous Release
   et Clang ASan/UBSan, ainsi que trois portes FULL de petits nuages. Le
   contrat comptable et la cause exacte de mutation sont corrigés à
   `5ab4326c`. Le gain local annoncé 130→109 s n'a pas encore de reçu
   apparié versionné ; l'amélioration G4 du MEB n'est pas mesurée.
   Sur le brut G4 R2 08/000000/K10, **supprimer idéalement q3/q4 et FULL
   laisserait encore 3,145 s** dans la chaîne actuelle : q2 0,794 s,
   fusion 0,789 s, recensus 0,494 s et 1,046 s de queue hors sous-temps
   publiés, plus préparation/index ([analyse B](CONTRE_AUDIT_B_FULL_COUTS_ET_INTERFACES_20260922.md)).
   La queue inclut résumé et digest après la tour : mesurer ces postes et
   fixer explicitement s'ils appartiennent au produit chronométré avant
   de juger la cible. FULL fait aussi **11,309 M** recherches `BallKey`
   dans **5,513 M** boules sur ce cas ; un index immuable clé→ID avec
   égalité exacte aux collisions mérite une ablation appariée, coût et
   octets inclus. Ces chiffres R2 restent des diagnostics bruts d'une
   session refusée, pas une qualification G4 acceptée.
   La [découpe FULL par K](PREFETCH_GEOMETRIE_FULL_PAR_K_20260923.md)
   rend les BallIds géométriques pré-calculables sous fenêtre d'octets,
   tandis que la fermeture des lots reste chronologique. Les 34,14 s de
   tour statique G4 ne sont pas assez ventilées pour prédire un gain. La
   [contrelecture B](CONTRE_AUDIT_B_PREFETCH_FULL_20260923.md) exige un
   contexte possédé par K, l'annulation/jointure des jobs et distingue le
   scénario naïf de mémoire `60R` du plancher `4R` des cibles seules.
   Le port `e0ae05a7` construit les forêts K en parallèle et évite les
   vecteurs temporaires de facettes. La [contrelecture B du
   port](CONTRE_AUDIT_B_FULL_PARALLELE_WIP_20260923.md) trouve ses nouvelles
   portes ciblées vertes sur 1 500 sites avec plus de 100 000 requêtes,
   sans gain G4 mesuré ni borne RSS. Une **recompilation indépendante de
   l'archive Git exacte** `e0ae05a7` sous Clang 18 RelWithDebInfo passe
   les trois portes ciblées sous ASan/UBSan/LSan (tri, mutant, chaîne)
   et les deux portes applicables sous TSan (tri, chaîne), sans erreur
   détectée ; la chaîne garde son digest et 100 407 requêtes maximales.
   Les options étaient `MHGP9_SANITIZE=ON` ou `MHGP9_TSAN=ON`, les lecteurs
   CTest ciblés, jamais la suite entière ni une trame LiDAR ; ces sorties
   temporaires ne sont pas un reçu versionné. Un diagnostic
   local instrumenté, **non archivé comme reçu**, des
   mêmes sources (`full_ball_tower.hpp` SHA `fb8b2c63…`, `pool.hpp`
   `aa0b780b…`) donne à W4 **11 245 584 octets** pour le double buffer de
   requêtes, mais **7 584 268 octets** dans
   `static_peak_retained_bytes` : ce dernier est documenté comme capacité
   échantillonnée après tri, pas comme pic co-résident. Publier un vrai pic
   et le RSS des forêts K simultanées. Le gate de chaîne compte les workers
   du résolveur, pas ceux du tri ; une mutation qui sérialise seulement le
   tri peut encore passer : mesurer `sort_workers_created` dans la chaîne.
   Enfin, l'allocation du second buffer peut retourner
   `resource_exhausted` quand un `std::sort` en place réussirait ; une
   injection locale sur `vector<int>` le reproduit, sans prouver qu'un FULL
   30 M réussirait. Un repli ciblé sur cette allocation et son compteur
   sont une protection simple à qualifier sous le contrat massif.
   Une [piste exacte pour les intrus](INTRUS_FULL_PREFIXE_EXACT_20260923.md)
   réutilise, par BallKey, un préfixe complet d'intérieurs Morton ; son
   [oracle combinatoire](check_full_intruder_prefix_20260923.py) passe
   1 336 782 paires de requêtes. Les répétitions par clé et par worker
   manquent encore au reçu G4 : mesurer avant de réserver un cache. Les
   clés déjà cataloguées peuvent lire leur liste globale d'intérieurs
   aux ordres bas ; cela ne retire pas les intrus sur clés absentes à K10.
   Instrumenter les tailles de supports et le temps avant de promettre un
   gain. La [note B](PLATEAUX_GRANDES_COQUILLES_B_20260922.md)
   propose un quotient local compact, [contrelu par B sur sept petites
   coquilles](CONTRE_AUDIT_B_QUOTIENT_COQUILLE_20260922.md), tandis que l'[oracle entier
   A](check_qmin_planes_u18_20260922.py) donne des fixtures u13/u17 et les
   largeurs des prédicats. Le port doit traiter ensemble quotient,
   représentants, parents, contributions et masque de lecture ; d'ici là,
   maintenir le refus exact. Au-delà, mesurer le volume de BallKeys,
   présentations, tableaux et lectures FULL sur les régimes LiDAR, sans
   transférer les chiffres u16 historiques.

Le jalon temporel v9 commence par le **sans-sol u18/1 mm**. Le contrat
principal antérieur sur trames **brutes entières** et le profil float32
original ne sont pas effacés par ce jalon ; les trames 08/000000, 000100,
000200 viennent toutes d'une seule séquence. Une mesure FULL CPU G4
**relative au catalogue émis** est acquise ; aucun contrat GPU,
sous-quadratique global, K10 <1 s ou K5 <1 s n'est acquis.

La [synthèse A d'architecture](AUDIT_A_ARCHITECTURE_K_GABRIEL_20260922.md),
les notes [q3](Q3_STRUCTURE_ET_BORNES.md), [q4](Q4_STRUCTURE_ET_BORNES.md),
[coûts/parallélisme](CONTRAT_COUTS_ET_PARALLELISATION.md) et la
[contrelecture B des cellules/catalogue](CONTRELECTURE_B_CELLULES_ET_CATALOGUE_20260922.md)
restent les références de conception. Les objections historiques v8, les
reçus et leurs limites sont dans le [contre-audit A des
mesures](CONTRE_AUDIT_A_MESURES_PLAN_20260922.md). Verdict public :
`not_claimed`.
