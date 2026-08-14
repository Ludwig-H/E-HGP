# Audit courant de MorseHGP3D v3

Date : 15 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

> **Verdict live au `HEAD=0bf46822416d126e0852391a978c3998e4b4c04f`.**
> Le pin noyau `a369452` porte `Q4SeedAxisTopR4`, son probe, 39 CTests déclarés,
> les vrais IDs de census, la capacité 163, les comptes requis et les fates de
> plateau. Il refuse aussi tout replay après `MORT_GAP` et tout apex dont le
> compte retenu atteint `r4`. `acd792d` ajoute la première sonde J0 et
> `0195480` une option de coupure et une recette de session ; aucun de ces deux
> commits n'est une mesure reçue. `0fe016f` conserve désormais le brut J0 et
> son statut tronqué ; `91486d4` grave les trois contre-fixtures qui réfutent
> l'ancien théorème des calottes. `2d8aa5f` raccorde ensuite `--axe` et ramène
> la sélection top-r de `O(m^2)` à `O(m*r4)` ; `2c14313` ajoute sa baseline
> host/device plate ; `11130cb` ajoute sa première recette G4. Son attempt
> incomplet est qualifié plus bas.
> `95b41b7` implémente ensuite l'enveloppe de calottes corrigée et `6be6bd8`
> accélère par grille la construction du lot device. `0bf4682` committe le
> tuilage diagnostique, le transcript CUDA incomplet et une recette de
> récupération ; malgré son sujet de commit, le brut n'est pas encore local.
> Le worktree courant ne contient que les deux présents audits. Aucune mesure
> CUDA ou GPU nouvelle n'entre dans le verdict.
> Q14 est fermée contractuellement : aucune structure de Delaunay n'est
> autorisée, y compris d'ordre un, comme source, squelette, filtre ou census.
>
> La route active comporte trois producteurs autonomes par miniboule : q2
> boule diamétrale avec `I<=9`, q3 triangle aigu/miniboule ambiante avec
> `I<=8`, q4 tétraèdre bien centré/circumsphère avec `I<=7`. Ils peuvent lire
> le même index Morton et une `NeutralPairPartition` immuable, mais aucun
> record, verdict, cap, continuation ou reçu de complétude ne passe d'une lane
> à une autre.
>
> Le théorème `Q4SeedAxisExtremalCompletion-r4`, interne à `Lane4`, retire le
> carré q4. Pour un `Q4Seed3` aigu exact avec `p` intérieurs permanents, tout quatrième point régulier
> shallow est parmi les `8-p` premières racines axiales entrantes ou les
> `8-p` dernières sortantes : au plus seize groupes, avec reconstruction
> exacte de `I_B/U_B`. Pour une arête owner portant `m_e` lignes carrier, le
> nombre de centres q4 shallow est au plus `8m_e` lorsque `m_e` compte toutes
> les lignes admissibles ; sur les seules faces aiguës parcourues, la borne de
> propositions est `16m_e^acute`, et non
> `binom(m_e,2)`. `WST4/CellPair/Sym2` sont donc rétrogradés en diagnostics de
> masse, pas route P0. `Q4Seed3` appartient exclusivement à `Lane4` et ne
> désigne ni un support ni une sortie q3. Contrat et réponse à Q14 :
> [`NOTE_SOLUTION_CONTRAT_SOURCE_AIGUE_20260814.md`](NOTE_SOLUTION_CONTRAT_SOURCE_AIGUE_20260814.md).
>
> Le pin noyau paramètre `r4=smax-3`, transporte les IDs permanents et le shell,
> produit `MORT_GAP`, compare `I_B/U_B` à un juge InSphere à trois classes et
> reçoit le primary/exact-once sur trois petites familles. `33766f6` répare la
> troncature 100-vers-99, distingue groupes et masse d'IDs et refuse les
> plateaux sous `RelevantGP`. `a369452` ferme les faux `EXACT` après
> `MORT_GAP`/deep. Il reste un P0 reproductible : ni la sélection ni le replay
> n'imposent l'injectivité et la disjonction des `PointId`. Contre-audit,
> fixtures et hashes :
> [`AUDIT_WORKTREE_Q4SEED_AXIS_TOPR4_20260814.md`](AUDIT_WORKTREE_Q4SEED_AXIS_TOPR4_20260814.md).
>
> Q15--Q17 sont tranchées pour cette tranche : la v3 reste u16 et le contrat
> binary64 conserve un statut séparé ; une coupure `--dmax` validée seulement
> par le maximum observé ne reçoit pas la complétude de J0 ; le best-first axial
> est exact sur un `Q4Seed3` singleton, pas sur les coins d'un `FaceBlock`
> variable. Réponse et preuves :
> [`REPONSE_AUDIT_Q15_Q17_PROFIL_J0_AXIS_20260814.md`](REPONSE_AUDIT_Q15_Q17_PROFIL_J0_AXIS_20260814.md).
>
> Le prototype `lane_source_scale_probe` confirme certaines identités de
> fuseau et conserve des bits de verdict séparés, mais ne reçoit pas J0. Sa
> famille intégrée `two_lines,n=10` retourne code zéro avec `q2=20` et
> `diam_max/dmax=0,007`, tandis que son propre brute force trouve `45` q2.
> La borne `--dmax-espacements=64` garde le même faux vert. L'owner q3 compare
> seulement le plus petit endpoint d'arêtes maximales égales et duplique un
> triangle aigu isocèle ; aucun juge q3 ne le voit. Le vecteur `acu` est en
> outre un record matériel partagé entre q3 et q4, contraire au fork autonome
> s'il devenait architecture. Les `11/11` CTests ciblés passent en `8,82 s`,
> mais n'exercent aucune de ces réfutations, ni `I_B/U_B` ou le shell.
> Contre-audit :
> [`AUDIT_WORKTREE_LANE_SOURCE_SCALE_J0_20260814.md`](AUDIT_WORKTREE_LANE_SOURCE_SCALE_J0_20260814.md).
>
> Q18--Q20 sont également tranchées. Le lemme antipodal de la note des calottes
> admissibles est juste, mais son théorème est faux : une direction admissible
> à `D` ne l'est pas nécessairement à `r<D`, et le test aux sommets décide la
> contenance d'une calotte au lieu de son intersection avec la cellule. La q2
> `x=0,v=(10,0,0),r=5` réfute la proposition par vacuité. La réparation sûre
> marque fail-open toute cellule intersectant
> `2(s.u)>max(r,||s||)`, puis applique séparément `10/9/8`. `91486d4` grave
> désormais la vacuité q2, l'échange de quantificateurs et la différence entre
> arête q4 et diamètre `2R`. `95b41b7` met en œuvre l'enveloppe réparée par
> intersection, emploie les partenaires globaux et corrige le falsificateur q4
> pour comparer le diamètre `2R`. Ses `34` CTests sont déclarés ; sur les
> mesures bornées rapportées, la part dite certifiée passe de `23,00 %` à
> `88,00 %` sur `uniform` et de `0,00 %` à `14,33 %` sur `terrain`, sans
> violation q2/q4 sur quatre familles. Ces gains ne sont pas encore reçus : le
> classifieur remplace `||s||` par `floor(sqrt(||s||^2))+1`, rétrécit
> l'enveloppe et peut omettre une cellule potentielle. La fixture
> `s=(10,0,0)`, cellule `(2,0,-6),(3,0,-5),(2,-1,-5)`, donne `3600>3400`
> exactement mais le code teste `3600>4114`. Réparation directe : propager
> `T2=max(r*r,||s||^2)` et comparer les carrés, sans racine. q3 n'est toujours
> pas falsifié et le résiduel terrain reste majoritaire.
> [`REPONSE_AUDIT_Q18_Q20_CALOTTES_ADMISSIBLES_20260815.md`](REPONSE_AUDIT_Q18_Q20_CALOTTES_ADMISSIBLES_20260815.md).
>
> La recette `session_axis_top8_g4.sh` de `840a2e2` ne devait pas être lancée :
> quand son horodatage de génération est vide, son trap appelle un arrêt non
> versionné susceptible de toucher une session préexistante ; ses 76 runs
> séquentiels ne tiennent pas dans le budget. Une tentative concurrente a
> échoué avant build ; le garde interne a arrêté sa génération exacte et
> certifié `TERMINATED`. Contre-audit et transcript :
> [`AUDIT_CONTRE_SESSION_AXIS_TOP8_G4_840A2E2_20260814.md`](AUDIT_CONTRE_SESSION_AXIS_TOP8_G4_840A2E2_20260814.md).
> Son successeur répare l'arrêt générationnel et la zone, mais son parser ne
> peut jamais accorder simultanément les sweeps et neuf exact-once, cherche les
> anciens noms de métriques et rapatrie encore les preuves après le verdict. Il
> reste interdit tant que ces défauts et le P0 d'identités ne sont pas fermés.
>
> Il n'existe toujours aucune mesure GPU G4 bout-en-bout. Le reçu
> `chaine_complete_g4_20260813` est `GPU_RUN=NO`, `PRODUCT_OUTPUT=NO` et mesure
> `78,8 s` CPU chargé sur `uniform,50000`, sans BallKey, census, fold ou
> payload. La cible d'une seconde reste ouverte ; aucune nouvelle session G4
> ne précède les gates CPU des trois producteurs.
>
> La rampe J0 concurrente n'est pas une mesure GPU et rend elle-même
> `INCOMPLET_OU_TRONQUE` : dix runs, quatre codes non nuls et pire cutoff
> `0,940`. Seul `uniform` atteint 50 000 : `21 432 482` candidats à `smax=11`
> en 38 s CPU et `4 004 994` à `smax=6` en 4 s CPU. Les amas meurent à 12 500,
> après jusqu'à `24 135 659 695` paires de lentille. Les générations de calcul
> ont été arrêtées et certifiées `TERMINATED`. Le brut récupéré est un ledger
> sans ordre garanti, pas un compte de supports. Contre-audit et hashes :
> [`AUDIT_CONTRE_SESSION_J0_LANE_SOURCE_G4_20260815.md`](AUDIT_CONTRE_SESSION_J0_LANE_SOURCE_G4_20260815.md).
>
> La sortie constructive de ces réfutations est maintenant explicite : WSPD
> avec conservation de masse, morts de fuseau séparées `10/9/8`, `Third3`
> direct pour Lane3 et niveau shallow des lignes du plan médiateur pour Lane4.
> Le top-r axial remplace le produit de lentille ; `Q_theta=q*A-p*B` fournit le
> futur best-first et le census capé sur AABB. Aucun résultat de lane n'en
> alimente une autre.
> [`NOTE_SOLUTION_WSPD_NIVEAUX_SHALLOW_AUTONOMES_20260815.md`](NOTE_SOLUTION_WSPD_NIVEAUX_SHALLOW_AUTONOMES_20260815.md).
>
> Le pivot ponctuel est vert et matériel : à `n=6000,smax=6`, même sortie q4
> `89796`, avec `830044` roots au lieu de `48 791 131` couples, soit environ
> `59x` moins de propositions. Les petits `--axe --verifie` retrouvent le brute
> q4. Le temps reste `25--28 s`, car chaque seed rescane encore son voisinage.
> Le lot device de `2c14313` transporte précisément ce flat scan en CSR ; ses
> `3/3` CTests host-only passent en `1,48 s`, mais le test nominal est tronqué
> (`cap=1`) et aucune descente BVH n'existe. Le jalon immédiat est de brancher
> `census_replay` après owner/positivité afin de supprimer le second scan et de
> produire les vrais `I_B/U_B`. Le jalon suivant est le moteur `Q_theta` AABB
> partagé par First/Last, census et shell, avec un batch complet explicite et
> les identités comparées. Deux portes de sûreté restent nettes :
> `DEBORDEMENT` est aujourd'hui agrégé aux morts dans le raccord et l'API fixe
> déborde pour `r4=65`; il faut continuation/refus pour le premier et
> `1<=r4<=64` pour la seconde.
> `6be6bd8` retire déjà un coût incident du harness : la construction du lot
> n'effectue plus un scan de tous les points par paire, mais réutilise une grille
> exacte de voisinage. Le commit rapporte `200 000` seeds et `19,3 M` sites
> construits en `40 s` à `n=3000`. Ce chiffre qualifie le builder, pas le
> kernel ; une gate CSR multiensemble grille = scan naïf sur petit `n` doit
> précéder son emploi comme autorité de parité.
>
> Le tuilage committé à `0bf4682` a une preuve simple et utile : attribuer
> le support à son sommet lexicographique minimal et charger un halo entier
> `ceil(3*dmax/2)` contient tout `I_B/U_B`, puisque
> `2R<=sqrt(3/2)*D` en q4. Il ne certifie que le domaine déjà borné par `dmax`.
> Son raccourci courant `Q.size()<5 => tuile vide` est incomplet : une q2 vide
> tient dans deux points, une q3 dans trois et une q4 dans quatre. Appliquer les
> planchers `2/3/4` par lane, conserver les IDs globaux, puis comparer les
> multiensembles globaux et tuilés.
> Deux rejeux manuels du worktree recollent les **cardinaux du ledger borné** :
> `uniform,n=100,k=2` donne `2139/8439/7069` des deux côtés, et
> `two_lines,n=10,k=2` donne `20/0/0`. Le second rappelle exactement la portée :
> le brute q2 vaut `45`, donc le tuilage préserve la troncature sans la réparer.
> Le commit annonce aussi des recollements `k=2/3` sur uniform/terrain, mais ne
> versionne ni brut ni CTest permanent ; ces mesures restent à graver.
> [`AUDIT_CONSTRUCTIF_AXIS_DEVICE_2C14313_20260815.md`](AUDIT_CONSTRUCTIF_AXIS_DEVICE_2C14313_20260815.md).
>
> Contrôles frais : `39/39` CTests `^mhgp3v_q4axis` passent en `38,25 s`, avec
> des seuils `smax=7/14`, trois exact-once, le plateau de 100 IDs et le refus de
> replay sur la fixture `MORT_GAP`. Le mutant du shell persistant est tué par la
> campagne scanline `Plateau`, et non par la fixture ties qui ne contient aucun
> `B=0,A=0`. Le noyau ponctuel `Lane4` reste un oracle borné ; les trois
> producteurs WSPD autonomes ne sont pas implémentés et aucune complexité 50k
> n'est reçue. Une comparaison indépendante de `O(m*r4)` à la référence
> `O(m^2)` sur `10 000` configurations rationnelles, ties et caps compris, ne
> trouve aucun écart pour `r4>=2`.
> La configuration et les builds ciblés courants réussissent ; `37/37` CTests
> `^mhgp3v_(caps_|axis_device)` passent en `199,89 s`. La fixture entière qui
> réfute `sqrt(ds)+1` n'est pas encore dans ces portes : leur vert ne reçoit pas
> les nouveaux pourcentages de calottes.
>
> La recette G4 au pin `11130cb` prévoyait douze cas pouvant durer chacun
> `900 s`, soit `10 800 s`, contre un arrêt invité à `4 500 s`; son
> `RUN_TIMEOUT=3300` n'encadrait rien. Le worktree réduit ce budget à huit cas
> de `300 s`, mais doit encore imposer un timeout global, le cardinal exact de
> la matrice et un fate `PREFIX_PARITY` pour tout `cap=1`.
>
> Un attempt concurrent du pin propre `11130cb` a compilé sous CUDA 12.9 sur
> RTX PRO 6000 Blackwell, puis a fini `rc=127` avant scp et avant verdict. Le
> reçu local ne contient que le transcript SHA-256 `9354b206...`; le fichier
> distant de 72 lignes n'a pas été rapatrié. Il n'existe donc aucune mesure de
> parité ou de débit recevable. La génération ciblée est certifiée
> `TERMINATED`. Avant toute relance, préserver cet attempt et récupérer le brut
> distant sans exécuter le `rm -rf ~/ax` du runner. La compilation a en outre
> ciblé `52`, pas explicitement `120-real`; la prochaine recette doit fixer
> l'architecture et hacher sa propre copie immuable. La recette n'a pas été
> exécutée par l'audit ; GCP n'a pas été utilisé.
>
> `0bf4682` ajoute `session_ax_recuperation_g4.sh`, mais ne récupère encore
> aucun fichier : le dossier reçu contient toujours le seul transcript. La
> recette ne doit pas être lancée avant deux corrections de preuve. Elle écrit
> son propre log dans le même `transcript.txt` et écraserait le reçu original ;
> elle réemploie en outre exactement la voie `gcloud compute scp` qui vient de
> rendre `127`. Employer un répertoire d'attempt distinct et streamer
> `sha256sum` puis `cat` via la commande SSH déjà fonctionnelle, avec compte de
> lignes et hash local, préserve l'original et évite de répéter le même échec.

> **Alerte au pin logiciel `6e815d28b3e229a0161eb00d6fa0c9a272efac5d`.** Les
> commits `89774d0`, `e3f1925` et `88a9ba8` ajoutent respectivement
> `Corner8BallDepth`, un broad phase WST3 et son produit WST4 ; `22d1cb0` en
> publie la note de coût. `a73161c` corrige ensuite le signe d'acuité par
> `E+X-D=-2H` et garde `H<0`. La fixture associée vérifie le scalaire et les
> six longueurs égales, mais pas indépendamment les quatre poids `1/4` ni
> l'owner sur de vrais `PointId`.
>
> `069d903` raccorde Corner8 après le broad phase ; `f7ab7bb` ajoute un
> certificat bisigne conditionnellement sûr ; `52585c5` cesse de supprimer les
> couples `C,D` non séparés, compte une décomposition `Sym2` et ajoute une
> fixture plate. Ces trois ajouts restent diagnostiques : Corner8 intervient
> après matérialisation, `cd_pending` n'est pas un état de continuation et le
> compteur `Sym2` ne remplace pas les blocs diagonaux dans le consommateur.
> Sur `u=v`, `masse_soumise` et `masse_fermee` emploient encore `|C|^2` au
> lieu de `binom(|C|,2)`. La fixture plate, reçue par regex, ne teste ni poids
> `1/4`, ni ratio d'aplatissement.
>
> Le théorème de la
> miniboule unique est reçu uniquement pour un **support minimal positif
> complet**. Ces supports forment une supersource finie exhaustive par
> Carathéodory et témoignent `c in conv(U_B)` ; ils ne suffisent pas à publier
> un événement dégénéré. Le contrat régulier exige encore
> `c in relint(conv(U_B))`. Hors `RelevantGP`, le shell complet et sa disposition
> donnent `unsupported_degeneracy/plateau_pending` tant qu'une extension
> indépendante n'est pas reçue. Corner8 est un certificat `ALL_INTERIOR` conditionnel sur un vrai
> support q4 déjà authentifié ; ses six portes historiques ne jugent ni positivité, ni
> owner, ni IDs, et son oracle ponctuel recopie le prédicat du sujet. Les
> `5/5` portes WST3 et les portes WST4 reçoivent une couverture dans le rectangle
> owner choisi par leur juge, pas une source physique exact-once : le tape brut
> conserve les ancres non-owner et les diagonales, tie-breake par rang Morton et
> rejette à tort les positions dupliquées. Le verdict détaillé et les réponses
> Q6--Q13 sont dans le contre-audit support-complet.
>
> Le paramètre `--echelle=num/den` n'a toujours pas une sémantique cohérente.
> Le code compare `diag2*num<=rayon2*den`, tandis que le commentaire de formule,
> CMake et le nom de la porte fine emploient encore le rapport réciproque. Le
> nouveau compteur `Sym2` et la conservation des couples non séparés réparent
> des pertes conceptuelles, mais aucun des deux ne retire encore du travail au
> chemin Corner8. Le certificat bisigne n'a ni juge causal de jonction, ni
> mesure appariée avant produit. Son caller abandonne en outre un nœud dès que
> **l'un** des deux bits conclut, même si la lane active demande l'autre : c'est
> sûr mais incomplet. Il faut des tâches `(node,active_sign_mask)`.
>
> `1fd9cf1` ajoute ensuite un sampler `--masse` et une CTest à regex. Ils ne
> sont pas reçus : le sampler choisit les
> blocs témoins uniformément au lieu de les pondérer par `|C|`; son ordre fixe
> de slots tranche le tie owner au lieu de `EdgeKey(PointId)` et il force
> provisoirement la positivité par `const bool centre=true`. Il accepte aussi `--masse` sans
> `--ordre=4` et appelle `retenus` le seul test `I<=7`, sans extra-shell.
> Aucune de ses sorties ne décrit donc encore la masse q4 positive.
>
> `f1b78c0` avait committé `--supports-retenus`, SHA-256 WST
> `23a2be338ac36b364d89b52669657f31602662ee9bb6b9e1716f97c4177ef0a3`, et
> sa CTest à regex, CMake
> `258b9eb979ca2119d21eade9a8ac6db9dcb8c4893a8e57d7042fdcbc7477b11a`.
> Dans cette révision, il énumérait tous les tétraèdres non
> coplanaires avec `I<=7`, sans positivité, shell ni `BallKey/RLE`. Cette boucle
> globale est exact-once sans owner ; elle ne calcule toutefois pas l'owner
> requis pour une provenance ou comparaison `OwnedCK`. Son
> commentaire assimile abusivement la plus grande arête au circumrayon, et le
> maximum des douze rangs dirigés n'est pas une condition nécessaire d'une
> route owner. Le cap `n=260` autorise un pire cas prohibitif. La nouvelle CTest
> passe en 30,49 s puis 33,91 s à seulement `n=120` sur la machine de l'auditeur et omet
> précisément `--ordre=4` : elle
> reçoit donc une sortie q4 sous le mode q3 par défaut au lieu de mordre le
> défaut d'option. Le titre « aucun préfixe kNN » n'est pas reçu : la fixture
> 50k exclut une source ancrée `k<=12499`, pas toute architecture locale ni une
> croissance asymptotique.
>
> `34cf05d` committe ensuite `c8::bien_centre` par Cramer et signes de faces,
> puis `08dec609` répare l'échappement de sa regex CMake : Corner8 header
> `34ae231c3b1518a67f0b573269b9ba68a2fe1038fda8b84419b725aa5b2ae68b`,
> probe `a126622e9c4ebbd5c81f60d54f4a3e896d0a9a86f26ce665ba940c7f85d90bc1`
> et WST `03dc123a2c7eadb48b79cb6c54bc6a8ff44cd2911aa73f89106c2e3e5c02e9b1`.
> La formule est cohérente sous le profil u16 et le raccord transforme la
> population en `Positive4 intersect {I<=7}`. Les `18/18` portes Corner8/WST
> ciblées passent sur deux rejeux en `13,39--19,31 s`. Les deux portes nouvellement pertinentes
> `mhgp3v_corner8_centre` et `mhgp3v_supports_retenus_localite` passent au
> présent rejeu en `0,00--0,02 s` et `5,10--9,61 s`; le catalogue reconfiguré contient `839`
> tests. L'ancien `CTest rc=8` est donc un replay historique antérieur à
> `08dec609`, pas l'état courant. La réception reste pourtant ouverte : les
> portes sont à regex, la seconde omet `--ordre=4`, les fixtures ne couvrent ni
> permutations, orientation opposée, extrêmes u16 ou entrées invalides, et
> aucune autorité C++ indépendante ne juge les poids. Le shell, `BallKey/RLE`,
> les positions dupliquées, l'ABI typée et la voie device restent absents. Deux
> commentaires CMake sont en outre faux : l'un affirme que le compteur omet la
> positivité ; l'autre reçoit `4096` supports « tous bien centrés » dans une
> fixture qui n'appelle jamais `bien_centre`.
>
> Le commit `6e815d2` ajoute aussi sur `prototype/wst3_probe.cpp`,
> SHA-256 `a697ae896240652ec372d7dce3b26f119c7a82971d478312abb9b16ddc6a5c51`,
> ajoute un histogramme de rang. Il ne change pas `bien_centre`, mais son
> commentaire sur-affirme la portée : la CDF ne décrit que la clique kNN
> mutuelle sous la convention `rho<k`, et reste optimiste sur les égalités car
> le rang ignore le tie par vrai `PointId`. Ce delta mobile n'est ni testé ni
> reçu comme source locale générale.
>
> Les défauts antérieurs restent actifs : les portes HC saines sont à regex et
> ne jugent pas chaque promotion ; `--fenetre-exhaustive` contourne les gates ;
> `cred+reste` ne majore que le ledger singleton baseline et perd encore une
> fermeture SOC combinée à `n=16` et deux fermetures BJD q4 à `n=64`. La porte
> borne ne compare ni fates ni masses à une exécution OFF.
> Le verdict exact sur l'idée de miniboule unique est
> [`AUDIT_MINIBOULE_UNIQUE_RESIDUEL_SHALLOW_5809BD2_20260814.md`](AUDIT_MINIBOULE_UNIQUE_RESIDUEL_SHALLOW_5809BD2_20260814.md) : l'événement q2 canonique est bien
> la boule diamétrale ; q3 et q4 ont chacun une boule canonique une fois
> leur support complet. Sur une ancre partielle, le sandwich `U<=D<=C` choisit
> entre fermeture, `tau(F)` et passage immédiat aux complétions.
> La dernière session G4 a échoué avant sa rampe et la cible a été certifiée
> `TERMINATED` : elle ne fournit aucune mesure 50k.

## Snapshot

Le dernier commit logiciel stable relu est
`6e815d28b3e229a0161eb00d6fa0c9a272efac5d`, commit
`le rang moyen est sous-lineaire, le pire ne l'est pas encore`. Il
absorbe le raccord Midball de `a58d020`, HC et la borne de `c1e2e3b`, Corner8
de `89774d0`, WST3 de `e3f1925`, WST4 de `88a9ba8`, la correction du signe
aigu, l'échelle rationnelle, le diagnostic Corner8 postérieur au broad phase,
le certificat bisigne de `f7ab7bb`, la conservation des couples non séparés,
les compteurs `Sym2`, le sampler `--masse`, puis l'oracle brut
`--supports-retenus`, la positivité Cramer de `34cf05d`, sa porte corrigée à
`08dec609` et l'histogramme de rang de `6e815d2`. Le worktree était propre avant
les présentes écritures documentaires. Aucun logiciel n'a été modifié par
l'auditeur.

Le code applique `diag2*num<=rayon2*den`, donc `num/den` se comporte comme une
finesse croissante. Un commentaire de formule, CMake et la porte dite
`echelle_fine` conservent encore `diag2*den<=rayon2*num`. Il faut versionner le
sens : soit renommer le comportement actuel `--finesse=num/den`, soit garder
un rapport diamètre/rayon et inverser la comparaison. Le cas historique `1/1`
ne distingue pas ces contrats ; une paire de portes réciproques `4/1` et
`1/4` est nécessaire.

Le rejeu causal `uniform,n=1000` donne `6 159 060` blocs WST4 à `1/1`,
`35 494` à `1/64` et `65 167 274` à `64/1`. À `1/4096,n=60`, l'arrêt racine
rend encore le juge owner vert par pure surcouverture et publie
`3 132 900=binom(60,2)^2` tuples candidats. Cela ne reçoit aucune sélectivité
aval.

Empreintes SHA-256 au commit `6e815d2` :

- `CMakeLists.txt` :
  `051f0103db6d8bf2ddd3ec600fd70039c3bacacfbcb77308274c597bf12f5642` ;
- `prototype/wst3_probe.cpp` :
  `a697ae896240652ec372d7dce3b26f119c7a82971d478312abb9b16ddc6a5c51` ;
- `prototype/corner8_ball.hpp` :
  `34ae231c3b1518a67f0b573269b9ba68a2fe1038fda8b84419b725aa5b2ae68b` ;
- `prototype/corner8_probe.cpp` :
  `a126622e9c4ebbd5c81f60d54f4a3e896d0a9a86f26ce665ba940c7f85d90bc1` ;
- `prototype/wspd_wavefront_probe.cpp` :
  `6330b79586066c575c59e4480a17fdf864fdef77b261a3a7f33c66bd68ed9c5b` ;
- `prototype/cloud_families.hpp` :
  `f825334096c80407c57e2ca05f6f59f6ae3dd6313746beb8e73d689e9082dded` ;
- `prototype/midball_block.hpp` :
  `921e649f0ebbbfb7a8034bedaeeb0a14a2eaaadf9eadb092f4f8c3cdbfd9403b` ;
- `prototype/midball_probe.cpp` :
  `587ecc58ebdd592245449fef901be9d2fac3f4b9287752648554c48f2e7dcc49` ;
- `prototype/jung_dual_probe.cpp` :
  `d05a997c1c10e1e02918f3a585e7cdee45fda6814edf4930ccbae9b4030fd4e6` ;
- `prototype/block_jung_dual.hpp` :
  `99d960eb42e452a7cec126428945c1a56bf8984c7b138d18d3ea837f9a56ae5d` ;
- `receipts/soc64_actif_g4_20260814/transcript.txt` :
  `18e6dd0f1aeafca51639805761a15855acbd08f4446ef9b4cbe7132db64977eb`.

Le commit stable reçoit la primitive BJD, un packing disjoint sûr sur ses
campagnes causales, huit portes BJD ciblées, treize portes Midball, cinq portes
HC, une porte de réfutation de la borne, sept portes Corner8 et onze portes
WST3/WST4 enregistrées. Les `18/18` portes Corner8/WST ciblées passent sur deux
rejeux en `13,39--19,31 s`. Les deux portes nouvellement pertinentes,
`corner8_centre` et `supports_retenus_localite`, passent respectivement en
`0,00--0,02 s` et `5,10--9,61 s`.
Aucun de ces temps diagnostiques n'est un benchmark.
Aucune suite complète du catalogue de `839` tests n'a été rejouée dans ce
snapshot. Ces verts ne
ferment pas les défauts de composition et d'identité ci-dessus. Il ne reçoit ni
projection CK--WST distinct-ID/owner,
ni profondeur `tau(F)`, ni ledger persistant de vrais `PointId`, ni primitive
device, ni payload. Son `--fenetre-exacte` n'est exact que pour la miniboule q2
échantillonnée sous domaine régulier ; ses lanes q3/q4 publient un majorant par
témoins universels. Le préfixe SplitMix fixe ne reçoit pas les hypothèses
probabilistes des crochets Hoeffding.
Les titres de commits restent des claims, jamais des verdicts reçus. GCP non
utilisé par le présent auditeur.

## Verdict

Le contrat G4 reste ouvert. Il n'existe encore ni source u16 reçue, ni stage
`0B` produit, ni `BenchmarkOutputContract-v1` complet et éligible, ni campagne
p95 à 50 000 points. Le squelette du contrat de benchmark existe, mais ses
étages critiques restent incomplets ou absents.

La source mathématique prioritaire est maintenant un fork de trois producteurs
et non le produit `OwnedCK-WST4` : `{Lane2(MidballDepth10),
Lane3(Q3MiniballDepth9), Lane4(Q4SeedAxisTopR4 -> Positive4)}`. Les accolades
ne sont pas une composition. Chaque lane possède son univers, son ledger et sa
preuve de complétude ; les sorties se rejoignent seulement dans `BallKey/RLE`.
Aucun enregistrement carrier×apex ni `Sym2` n'est admis dans la tranche à
recevoir.

`SOC64 --actif` et `BlockJungDual64` sont des diagnostics amont. Aucun ne
traverse `BallEvent -> 0B -> payload`; ils ne qualifient donc ni exactitude
industrielle ni SLO.

Le snapshot contient au moins sept probes/campagnes utiles, mais aucune chaîne produit
reçue :

1. `BallFormToBallEvent-v0` forme des sphères et des census sur petit domaine ;
2. `PointHypergraphBottleneckClosureProbe-v0` compare Kruskal et Floyd sur un
   même hypergraphe de points ;
3. le raffinement local réduit le résiduel `E4` du certificateur central, au
   prix d'un front et d'un nombre de recertifications supérieurs ;
4. la rampe à 50 000 points rejette la configuration centrale mesurée sur
   `eight_clusters`, sans réfuter les certificateurs rectangles corrélés ;
5. `q4_brute_oracle` énumère les 4-sous-ensembles à petit `n`, avec un census
   et des prédicats encore corrélés/incomplets sur le shell ;
6. Corner8 certifie conditionnellement des blocs de témoins q4 `ALL` ;
7. WST3/WST4 compte une couverture candidate avant projection owner/positivité.

Les noms « 0A fermé », « stage 0B » et « le raffinement paie » dépassent les
preuves disponibles.

## Ancienne source candidate : `CKPairTape -> WST3 -> WST4`

Cette section juge le logiciel au pin et conserve ses obligations de
couverture. Elle est supersédée comme ordonnance P0 par la source shallow et le
théorème axial ci-dessus. `WST4`, `CellPair` et `Sym2` ne doivent plus être
raccordés au chemin produit.

Le bloc ci-dessous décrit uniquement le probe historique et ses masses ; il ne
prescrit aucun flux entre les trois producteurs. Le probe courant n'implémente
que le `CandidateCover` :

- une WSPD Callahan--Kosaraju canonique partitionne toutes les paires en
  `O(s^3 n)` rectangles physiques sans les développer ; après filtrage des
  seules paires endpoint `D=0`, elle source tous les q2 propres. Un bucket de
  positions dupliquées conserve obligatoirement la multiplicité et les vrais
  `PointId` dans les pools témoins et les produits ; les paires vers toute
  troisième position gardent cette multiplicité ;
- pour chaque rectangle `R=(A,B)`, les cellules Morton d'une échelle liée à sa
  boule `B_R` et rencontrant `2B_R` couvrent tout carrier d'un support dont
  `ab` est l'arête maximale ; l'intersection avec les deux enveloppes endpoint
  resserre encore ce domaine. Après projection distinct-ID, owner
  longueur/`EdgeKey` et positivité, on obtient un unique
  `OwnedCK-WST3(A,B,C)` ;
- après la même projection, les couples non ordonnés de ces cellules donnent
  un unique `OwnedCK-WST4(A,B,C,D)` ; q4 recertifie directement les quatre
  barycentriques du circumcentre. Le compteur brut antérieur à cette projection
  n'est pas `Owned`.

Les nombres de blocs **initiaux** conditionnels sont `O(s^3 n)`,
`O(s^3*eta^-3*n)` et `O(s^3*eta^-6*n)` sous une vraie propriété
fair/compressed-split et `0<eta<=1`. Ils ne bornent ni les raffinements `MIXED`,
ni les masses logiques `|A||B|`, `|A||B||C|` et `|A||B||C||D|`. Chaque bloc
reste paresseux jusqu'à un consommateur factorisé reçu ou au preflight atomique
de sa vraie sortie.

Dans l'architecture active, `Lane4` régénère ses `Q4Seed3` depuis la partition
neutre ; elle ne lit ni `q3_open`, ni tape, ni événement, ni verdict de
`Lane3`. Le rapport historique et le recalcul exact sont dans
[`AUDIT_SOURCE_CK_WST_Q2_Q3_Q4_35FCEA8_20260814.md`](AUDIT_SOURCE_CK_WST_Q2_Q3_Q4_35FCEA8_20260814.md).

Pour une paire ponctuelle, `JungDiskDepth9/8` restreint les centres au disque
imposé par Jung dans le plan médiateur. Des groupes disjoints de trois IDs au
plus peuvent y fermer q3/q4 même lorsque le LP sur tout le plan échoue. Cette
preuve ne ferme pas un rectangle CK : le plan et les demi-plans varient avec
`a,b`. Une contre-fixture `2×2` est désormais documentée ; il faut scinder vers
des microtiles rejoués ou prouver un `BlockJungDiskDepth` uniforme.

## Miniboule canonique : oui au support complet, non à la cascade

Pour un support minimal positif affinement indépendant fixé, la miniboule et
donc la boule canonique de son événement complet sont uniques : diamètre q2,
centre/rayon donnés par le circumcercle intrinsèque du triangle aigu q3 mais
boule et census ambiants en dimension trois,
circumsphère du tétraèdre bien centré q4. Pour les arités inférieures à quatre,
une famille de sphères ambiantes reste incidente au support sans être portée
minimalement par lui. Le circumdisque
planaire q3 est le cœur des sphères incidentes, pas l'événement ambiant. Une
arité supérieure conserve toutefois des centres différents tant que son support
n'est pas complet. Deux fixtures séparées montrent qu'une paire q2 de rang
douze peut porter respectivement un q3 de rang trois ou un q4 de rang quatre ;
une face q3 de rang douze peut porter un q4 de rang quatre. Un troisième point
de shell ne change pas nécessairement l'arité minimale : le triangle droit
reste q2.

Pour un domaine continu `K` contenant le centre canonique, les quantités
`U=singletons universels`, `D=profondeur collective minimale` et
`C=profondeur au centre canonique` vérifient `U<=D<=C`. L'ordre exact est donc :

```text
C<h      -> sauter ce certificateur, sans décider les cofaces
U>=h     -> fermer
U<h<=C   -> tau(F), sweep ou split
support complet -> census de son unique boule canonique
```

Le sampler du commit mesure `U`, pas `D`. Ses pentes proches de `1,09` sont des
pentes d'un majorant échantillonné ; `U<h` n'exhibe aucune sphère peu profonde.
La fixture Jung à huit groupes ferme q4 avec `U=0`, réfutant le non-converse et
l'inclusion Delaunay affirmée dans la note de Claude. Le résultat reste utile
pour éviter des appels Jung lorsque `C<h` et pour fermer immédiatement lorsque
`U>=h`, à condition de rester blockwise. Un scan de 50 000 témoins pour chacun
des `1 392 028` blocs historiques `s=2` dépasserait 69 milliards de tests.

## `MidballBlockDepth` stable : théorème q2 reçu, ABI et raccord ouverts

Le minimum de `H(a,b,z)=(z-a) dot (b-z)` implémenté au pin est exact sur
l'AABB continue et `hmin>0` constitue donc un `ALL` q2 sûr. Son maximum teste
les entiers voisins du point stationnaire : il est exact sur le réseau u16, pas
sur l'enveloppe continue. Avec `A={0}`, `B={1}`, `C=[0,1]`, le code trouve
`hmax=0` et `NONE`, alors que le maximum continu vaut `1/4`. Ce verdict reste
sûr pour les vrais sites quantifiés ; il doit être typé `NONE_LATTICE_U16` ou
remplacé par le numérateur exact d'échelle quatre avant toute réutilisation
continue.

Le header ne préflighte ni domaine u16, ni `lo<=hi`, ni paire propre, ni
identités et n'expose aucun `INVALID/UNKNOWN`. Il duplique
`rect_h_interval`, dont le calcul et les fixtures portent eux aussi un domaine
réseau insuffisamment typé dans l'API. Les 13/13 CTests affichés verts ne ferment
pas ces points : trois portes saines emploient `PASS_REGULAR_EXPRESSION` ;
`--selftest=1` imprime `accord=OUI` puis rend le code `3`, démontrant que le
texte précède le plancher. La fixture de non-hérédité ne recertifie pas encore
l'acuité q3 ni les barycentriques positives q4.

Le raccord stable ALL-only est l'ordonnance saine : ajouter uniquement un gain
`ALL`, sans remplacer le `NONE` du certificateur central. Au snapshot courant,
il compile et affiche 13/13 CTests ciblés verts, mais la porte saine WSPD à regex
reste fail-open. Le source appelle la primitive complète, mais GCC Release
élimine déjà le maximum inutilisé : le bloc machine de promotion contient 24
multiplications. Une ABI min-only reste utile pour l'autorité et le device, pas
comme gain CPU présumé. Sur
`eight_clusters,n=1500,s=8`, une ablation alternée réduit les lectures de
`7,41 %` et le résiduel q2 de `29,95 %`, avec `pending=0`, mais augmente la
médiane de vague de `14,1 %` sur la machine partagée. Cette hausse ne peut pas
être attribuée à `axis_max`, absent du binaire. La porte suivante est une
autorité partagée min-only, un juge de chaque promotion et un différentiel
causal profilé de lectures/temps.

Le juge live reste ouvert : ses compteurs sont globaux à une liste de tailles,
son produit de cap `na*nb*m` peut déborder i64, il juge les fermetures finales
plutôt que chaque promotion et le retour de `--fenetre-exhaustive` contourne ses
planchers. Le plancher de gain doit être une option de campagne. La seule
dominance reçue est `central ALL => Midball ALL`; Midball peut au contraire
promouvoir un `central NONE`, par exemple sur `A=[0,8],B=[10,100],C={9}`.

## `HCBlockDepth` au pin historique : formule sûre, intégration non reçue

Le delta HC emploie `H=(z-a) dot (b-z)` et
`C=(b-z) cross (z-a)`. Les conditions q3 `3H^2>||C||^2` et q4
`2H^2>||C||^2` sont exactes ; `Hmin` exact et une majoration par composantes de
`C` donnent un `ALL` sûr mais conservateur. Ce n'est ni la source des supports
q3/q4 ni une nouvelle autorité exacte : `corner512_all_lane` reçoit déjà
l'enveloppe continue complète. Le commentaire du commit ne perd pas seulement
la corrélation entre les deux produits d'une composante : il perd aussi la
simultanéité entre les trois maxima de composantes, puis entre `hmin` et le
pire `C`. Une fixture publiée isole la perte entre composantes ; les deux
autres pertes sont identifiées algébriquement et doivent encore devenir des
fixtures permanentes.

Les cinq CTests HC du commit passent, avec les deux mutants tués. Les portes
saines `mhgp3v_hc_selftest` et `mhgp3v_hc_vwave` restent toutefois à
`PASS_REGULAR_EXPRESSION` ; `--selftest-hc=1` imprime `accord=OUI` avant son
plancher puis rend code `3`. Le raccord recalcule HC jusque trois fois par
nœud, n'a aucun juge de chaque promotion, hérite ses compteurs entre tailles et
est contourné par `--fenetre-exhaustive`. `--hc --midball` finit
structurellement code `3`, HC ayant absorbé tous les gains q2 avant le plancher
Midball. À `eight_clusters,n=200`, les lectures baissent
`247966 -> 226535`, mais la vague one-shot monte `119,8 -> 214,2 ms` ; à
`n=3000`, le commit rapporte `31 538 327 -> 27 552 424` lectures mais une
médiane CPU `8,3 -> 13,7 s`. Ces diagnostics reçoivent la réduction logique,
pas un chemin chaud ni le SLO.

## `--borne-sup` au pin historique : invariant utile, raccord multivue réfuté

Pour une seule vue de crédits singleton, l'idée est exacte. Si les tâches
empilées forment une antichaîne, poser pour chaque lane
`P=cred+sum(pop(task))`. Un `ALL` transfère la population de la pile au crédit,
un `NONE` la retire, et un `MIXED` remplace le parent par deux enfants disjoints
de même population totale. Ainsi `P` ne croît pas ; `P<h` prouve que cette
source singleton ne fermera plus la lane.

La première révision, hash `90640885`, soustrayait le parent `MIXED` sans
réinsérer ses enfants. Elle transformait donc cette borne supérieure en
sous-estimateur. Fixture causale `uniform,n=16,coord=64,seed=1,window=1024` :
la baseline ferme une q2 après `3171` lectures ; la révision annonce zéro
fermeture après `117` lectures, `351` lanes mortes et aucune troncature. La
révision désormais commise réinsère bien les deux populations ; cette fixture
retrouve la fermeture et passe de `3171` à `2177` lectures. Le premier défaut
doit rester un mutant permanent `drop-mixed-children`.

Le raccord reste néanmoins faux sur les modes acceptés :

- `reste/mort` ne suit que `cred/mask`. Il efface ensuite `m`, donc aussi
  `cmask`, alors que la vue combinée peut avoir `ccred>cred`. Sur
  `terrain,n=16,coord=64,seed=3,--soc64-shadow`, la vue combinée passe d'une
  fermeture à zéro avec la borne ;
- BJD propose des crédits collectifs **après** la boucle à partir des feuilles
  vues. La borne des singletons ne majore pas ces groupes et l'arrêt précoce
  réduit leur banque. `terrain,n=64,coord=64,seed=3,BJD8` passe ainsi de
  `84` à `82` q4 fermées et de `338` à `319` groupes avec code zéro ;
- `visites_evitees` ne compte ni les tâches encore empilées au break, ni leurs
  descendants, et les compteurs sont globaux aux tailles. Ce n'est pas encore
  une mesure du travail supprimé.
- `--climb` initialise le potentiel sur les seuls frères remontés et omet la
  feuille `pos0` ; la mortalité porte alors sur un parcours incomplet, pas sur
  tous les témoins géométriquement disponibles.

La CTest `mhgp3v_borne_sup_refutation` passe, mais ne vérifie qu'une ligne de
compteurs par regex : aucune exécution OFF n'est comparée et les combinaisons
fausses restent acceptées. La réparation minimale sépare `reste/mort` par vue et garde le parcours vivant
sur l'union de leurs bits. Jusqu'à une borne composée authentifiée, le mode doit
refuser BJD et tout proposant post-boucle. Les portes permanentes comparent
fates, masses et fermetures exactes à la baseline sans troncature, mordent
`drop-mixed-children`, le cas `cred=0,ccred=7,reste=1,h=8`, la fixture BJD
ci-dessus, les fenêtres capées et plusieurs tailles. Aucun gain de la révision
commise n'est recevable avant cette parité. Le snapshot falsifié, la portée
exacte du lemme et les obligations sont détaillés dans
[`AUDIT_LIVE_BORNE_SUP_CREDITS_A58D020_20260814.md`](AUDIT_LIVE_BORNE_SUP_CREDITS_A58D020_20260814.md).

## `Corner8BallDepth` au pin historique : lemme `ALL` reçu, événement non raccordé

Pour un support q4 fixé et positif, `sigma*J(S,z)` est strictement convexe en
`z`. Ses huit coins donnent donc un certificat `ALL_INTERIOR` complet sur une
AABB témoin. Les largeurs `|O|<2^51` et `|J|<2^87` tiennent sous u16 lorsque
les signes sont jugés séparément. Les `6/6` portes ciblées passent.

La primitive ne préflighte toutefois ni boîtes u16, ni quatre IDs distincts,
ni positivité barycentrique, ni owner, ni shell ou `BallKey`. Son oracle de
bloc réemploie les prédicats ponctuels du sujet. Trois nominales sont à regex ;
`--selftest=1` imprime `accord=OUI` puis rend code `3`. Deux mutants déterminants
restent hors porte, `kProduitOJ` n'est pas implémenté et le mutant nommé
`corners-outside-implies-none` produit en réalité un faux `ALL`.

La fixture de 4096 supports prouve un bloc q4 utile, pas le raccord : avant
owner/positivité/census, sa télémétrie est `domain_mass_closed`. Un bloc `ALL`
paie environ 1508 multiplications d'extrémités `i128`, et non huit opérations.
Le détail, les fixtures `drop-corner`, support non positif et q3 ambiant sont
dans
[`AUDIT_CONTRE_RECEPTION_SUPPORT_COMPLET_CORNER8_WST34_22D1CB0_20260814.md`](AUDIT_CONTRE_RECEPTION_SUPPORT_COMPLET_CORNER8_WST34_22D1CB0_20260814.md).

## `WST3/WST4` au pin historique : candidate cover, pas source `Owned` reçue

Le filtre d'acuité rouge de `3703097` est réparé au pin `a73161c`. Pour
`H=(x-a) dot (b-x)`, une face est aiguë en `x` lorsque `H<0`. La classification
exacte est `Hmax<0 => ALL_ACUTE`, `Hmin>=0 => NONE_ACUTE`, sinon `MIXED`.
Le juge WST4 contrôle cependant toujours les couples bruts et ignore le booléen
`aigu`; la nouvelle fixture tue le signe inversé, mais ne reçoit ni owner, ni
positivité q4, ni la relation filtrée.

Le ratio stable `--echelle=num/den` reste contradictoire entre code,
commentaire et CMake. Le code teste `diag2*num<=rayon2*den`, donc un rapport
plus grand raffine, tandis que le commentaire de formule et la porte dite
`echelle_fine` emploient le rapport réciproque. Les mesures étiquetées `1/256`
décrivent le comportement du code, pas un contrat API reçu.

Le lemme arête maximale--lentille est correct et les campagnes bornées couvrent
chaque triplet/quadruplet dans le rectangle de l'owner choisi : WST3 `5/5` et
WST4 `1/1`. Le juge ne cherche cependant aucune copie sous les ancres non-owner,
ne compte pas les diagonales du tape, tie-breake avec les rangs Morton plutôt
qu'avec les vrais `PointId` et refuse les positions dupliquées. Sujet et juge
partagent ces deux dernières conventions.

Ainsi les blocs courants sont une `candidate-cover exact-once après projection
distinct-ID et owner`, pas encore `OwnedCK-WST3/WST4`. La projection distinct-ID, owner et
positivité doit rester factorisée avant le produit. L'arrêt LBVH à échelle
grossière donne une surcouverture sûre mais aucune borne de cellules par
rectangle ; les pentes et la constante proche de 32 sont des diagnostics finis.
La masse WST4 proche de `n^4` et `187` millions de blocs à `n=8000` rendent le
produit brut rouge, sans mesurer encore `M4_apex`.

La généralisation exacte à l'ordre `q` est combinatoire. Pour une antichaîne
témoin disjointe `C_1,...,C_k`, les atomes sont
`A x B x product_i binom(C_i,alpha_i)` avec `alpha_i>=0` et
`sum alpha_i=q-2`, puis intersection avec distinct-ID, owner total,
indépendance affine et positivité. À q4, cela donne tous les produits croisés
et toutes les diagonales `binom(C_i,2)`. Le nombre de types vaut
`[z^(q-2)] product_i (1+z+...+z^min(|C_i|,q-2))`, au plus
`binom(k+q-3,q-2)`. Aucune séparation entre toutes les paires de facteurs n'est
requise ; son échec donne split ou `PENDING`.

La réparation de performance est une vraie grille Morton au niveau lié à
`r_R`, puis `ALL/NONE_ACUTE`, profondeur et owner uniforme **avant** tout
`sum k_t^2`. Le count de masse des paires de cellules emploie
`binom(sum |C_i|,2)` en temps linéaire par rectangle et en u128, jamais une
boucle `u<v`. La sortie reconvertit encore l'u128 en `double` à six chiffres
pour l'affichage : le reçu exact doit publier l'entier. Les questions Q6--Q13 et
la gate coût appariée sont résolues dans le contre-audit ci-dessus.

Le raccord Corner8 commis n'économise encore aucun produit : il est
exécuté après le count WST4 et après le juge, sur `par_terminal` déjà
matérialisé. Il ne supprime aucune tâche aval et ne possède ni CTest de jonction,
ni juge causal, ni refus sur support non owner/non positif. Son crédit de huit
spans témoins disjoints est une voie `ALL` potentiellement sûre sous les
préconditions de Corner8, mais ses compteurs `fermes/masse_fermee` portent la
candidate-cover brute, pas des supports q4 authentifiés. Un cap `--corner8`
tronque en outre le diagnostic; il ne doit jamais être interprété comme une
fermeture complète.

La diagonale `C=D` contient le choix `x=y`, donc son intervalle d'orientation
contient nécessairement zéro. Elle exige un raffinement paresseux exact de
`binom(C,2)` en `LL/LR/RR`; la supprimer perdrait les q4 dont les deux sommets
restants partagent la cellule. `52585c5` ne supprime plus un couple `C--D` non
séparé : il incrémente `cd_pending`, puis continue le diagnostic. Ce compteur
n'est toutefois ni un tape, ni un statut `PENDING`. Malgré son commentaire
« tous les couples », il ne teste en outre ni `A--C/A--D` ni `B--C/B--D`.
Le prétest d'orientation centré est sûr, mais `corner8_block` recalcule ensuite
l'ancien intervalle au lieu de consommer son signe. L'enclosure centrée contient
un terme `e1` linéaire dans les rayons ; son ancien commentaire « erreur
quadratique » est faux. Une arête-owner longue peut avoir deux apex proches :
la séparation reste une priorité de coût, jamais une condition de source.

Le nouveau `wst4_sym2` ne fait encore que compter les nœuds internes de la
décomposition. La jonction Corner8 boucle toujours sur les couples `u<=v`
originaux ; elle ne remplace donc aucun parent diagonal par les trois tâches
disjointes `Sym2(L)`, `L cross R`, `Sym2(R)`. De plus,
`masse_soumise/masse_fermee` emploient `|A||B||C||D|` : pour `u=v`, la masse
candidate non ordonnée est au contraire
`|A||B| binom(|C|,2)`, avant même distinct-ID et owner.

Le coût de ce compteur est déjà rouge. Sur `uniform,n=1000/2000/4000`, il
parcourt `25434157/103783757/459477476` nœuds internes, de pentes
`2,029/2,146`; à `n=4000`, c'est `3248x` les `141468` couples plats. La valeur
diagnostique `|C|-1` se calcule en `O(1)` par cellule. La relation réelle doit
rester un descripteur paresseux et ne se raffiner qu'au prédicat `MIXED`.

Le certificat bisigne commis à `f7ab7bb` emploie deux ledgers de témoins,
`J<0` et `J>0`. Son principe est sûr : les supports `O>0` consomment huit vrais
`PointId` du premier ledger, les supports `O<0` huit du second, et `O=0` n'est
pas q4. Son caller reste conservateur et incomplet : lorsqu'un masque non nul
crédite seulement une strate, il abandonne le nœud au lieu de poursuivre les
lanes encore actives. La réception exige un masque de lanes par tâche, un oracle
de jonction, des reçus d'identités séparés et une exécution avant produit. Sa
télémétrie agrège encore les strates et reconvertit u128 en `double` ; aucun de
ses taux ne qualifie le SLO.

La contre-fixture existe déjà côté Corner8 : pour `A=(3,2,2)`, `B=(1,2,2)`,
`C=(2,3,2)`, `D=(2,2,3)` et `Z=[1,3]^3`, on a `O=-2`, les huit coins du
parent ont `J=-4` et le centre a `J=+2`. Le parent ne certifie que le bit inutile
`O>0`, puis le `continue` cache les descendants utiles à `O<0`. La fixture
ledger directement exécutable prend
`A=(14,10,10)`, `B=(6,10,10)`, `C=(10,14,10)`, `D=(10,10,14)`, donc
`O=-128`. Le parent témoin `[5,15]^3` donne `J=-7552` aux huit coins, soit le
mauvais bit 1 ; ses huit descendants `{9,11}^3` donnent `J=1664`, soit le bit 2
qui ferme réellement la lane négative. Le mutant `mask-any-continue` doit
manquer cette fermeture et le swap de deux sommets exerce la lane opposée.

La fixture plate enregistrée dans ce snapshot historique vérifie seulement, par regex,
`R2=1000001`, `cospherique=1` et `orientation_nulle=0`. Elle réfute utilement
l'implication « petit déterminant donc grand rayon relatif », mais ne juge ni
les poids `1/4`, ni l'owner, ni `R/diam`, ni la mesure d'aplatissement.

La rampe CPU locale `12500/25000/50000`, `--echelle=1/256` au sens du code et
Corner8 désactivé, rend la gate physique rouge. Sur uniform, les deux dernières
pentes des visites sont `1,907/1,036`; sur huit amas, `F3`, visites et `F4`
finissent respectivement à `1,756`, `2,050` et `2,193`. À 50 000, la masse
candidate vaut environ `3*binom(n,3)` en q3 et `6*binom(n,4)` en q4 : l'échelle
coarse compresse presque l'univers brut sous plusieurs ancres, elle ne le
sélectionne pas. Le prochain gain doit donc fermer ou projeter owner/positivité
sur les descripteurs avant toute expansion ; une nouvelle rampe de préfixes
Corner8 ne répondrait pas à ce verrou.

### Commit `1fd9cf1`, `--masse` : sampler non uniforme et positivité factice

Le commentaire promet un quadruplet uniforme dans la masse, mais le code
pondère seulement le terminal par `|A||B|(sum_i |C_i|)^2`, puis choisit les
indices de blocs `u,v` uniformément. Un point d'une petite cellule reçoit donc
une probabilité différente d'un point d'une grosse cellule. La diagonale est
ordonnée, contient `x=y`, et le cumul en `double` perd les unités au-delà de
`2^53`. L'owner ignore les égalités `EdgeKey` : la première paire dans l'ordre
fixe des six slots gagne tout tie, indépendamment des vrais
`PointId=pid[rank]`. L'égalité de deux rangs teste néanmoins correctement
l'identité du record tant que le tri reste bijectif. Enfin
`const bool centre=true` classe tout support non dégénéré comme q4 positif :
`non_centres` reste zéro par construction et `retenus` mélange les
circonsphères non minimales. `retenus` ne teste en outre que `I<=7`, sans
extra-shell ni `BallKey`.

La fixture de loi `|C_1|=1,|C_2|=3` rend le biais explicite : le point singleton
est tiré avec probabilité `1/2`, chacun des trois autres avec `1/6`, au lieu de
`1/4`. Le tie est faux sur le tétraèdre régulier relabellé : la première paire
de slots gagne toute égalité, indépendamment de la vraie `EdgeKey(pid)`. Enfin
`(0,0,0),(4,0,0),(2,3,0),(2,0,1)` a un owner unique et `I=0`, mais un poids q4
négatif ; le stub le compte parmi les retenus.

La CTest à regex fige le résultat biaisé `0/3000`. Même sous une loi correcte,
zéro succès sur 3000 ne donnerait à 95 % que l'ordre `p<3/3000`, pas `10^-8`.
Seulement `507` tirages du nominal passent actuellement les rejets
IDs/owner/dégénérescence. Le rejeu amas donne `82,40 %` owner-hors et `939,5`
intérieurs moyens, contre `82,70 %/915,0` en uniform : le commentaire CMake
« sur les deux familles » est faux littéralement. Aucune variance, quantile,
seed Monte Carlo séparée ou barre d'incertitude n'est publiée.

Le scan `interieur_strict` recopie le prédicat du sujet et compte `I`, pas le
rang fermé : il fusionne shell et extérieur, ne RLE pas les supports partageant
une `BallKey` et surpondère les sphères ayant plusieurs `SupportKey`. Il ne
mesure aucun rayon. Une grande circumsphère peut être vide dans un nuage
arbitraire ; conclure « rayon discriminant » d'un simple census est donc sans
preuve. Owner élimine déjà environ 82 % sous cette proposition biaisée, tandis
que positivité et acuité ne sont pas mesurées.

Le routage d'option est vacuaire : `--masse` ne requiert pas `--ordre=4`. Le
rejeu `uniform,n=40,--masse=10` sur l'ordre trois par défaut rend code zéro et
publie une ligne q4 sans avoir construit le WST4. Le cap maximal autorise
`2^24` échantillons et le census coûte `samples*n`, sans plafond produit ; le
mode doit exiger l'ordre quatre, une taille/cohorte explicite et un plafond u128
avant tout scan.

Le sampler exact borné part de la masse non ordonnée u128
`|A||B|*binom(N,2)`, `N=sum_i |C_i|`. Après sélection du terminal par un cumul
u128, tirer uniformément un indice combinadique dans `binom(N,2)`, mapper ses
deux offsets par les préfixes de populations vers les cellules, puis tirer les
endpoints. Un rejet distinct-ID/owner/positivité préserve l'uniformité
conditionnelle si l'univers brut a été tiré uniformément. Employer les vrais
`PointId`, le tie total `EdgeKey` et le microkernel Gram/T--Q de positivité,
puis seulement le census exact. Publier masse de base, rejets par strate,
seed/digest, intervalle statistique et coût `samples*n`. Ce sampler reste un
audit statistique, jamais une preuve de complétude ni un hot path. Même après
réparation, zéro succès sur 3000 tirages ne reçoit ni une fréquence `10^-8`, ni
la causalité « rayon dominant » : il faut plusieurs seeds, un intervalle sur la
loi réellement tirée et des strates positives/owner appariées. La seule CTest
enregistrée fixe `uniform,n=2000` et accepte une regex ; elle ne reçoit ni la
seconde famille citée par le commit, ni le code de sortie après cette ligne.

### État historique `f1b78c0` de `--supports-retenus` : population q4 brute

À ce pin, le mode exhaustif filtre seulement `O!=0` et `I<=7` : son objet réel est
`Raw4={S: |S|=4, affdim(S)=3, I(circ(S))<=7}`. Il manque les
quatre poids strictement positifs : la fixture
`(10,10,10),(14,10,10),(10,14,10),(10,10,11)` a `I=0`, mais des poids
`(-1/2,1/2,1/2,1/2)` et serait retenue. La fixture frontière
`(0,0,0),(4,0,0),(2,2,0),(2,0,2)` a deux poids nuls et doit également être
rejetée à q4. Il
manque aussi le shell, `BallKey/RLE` et la provenance ; plusieurs tétraèdres
cosphériques sont donc comptés comme autant d'objets. `I<=7` décrit un support
régulier potentiel, ni un événement ni son rang fermé.

L'énumération globale `i<j<k<l` visite déjà chaque 4-ensemble une fois : un
owner n'est pas nécessaire à son exact-once propre. Il devient obligatoire pour
comparer ce diagnostic à `OwnedCK`, attribuer l'arête génératrice ou mesurer une
route owner ; ce cas exige le tie par vrais `PointId`.

Le code ne calcule pas le circumrayon. Il publie
`sqrt(dmax/nearest_neighbor_distance^2)`, rapport entre la plus grande arête et
le plus petit espacement local. Pour un support positif complet, le vrai rayon
est rationnellement `R^2=||Q||^2/(4*T^2)` ; sa comparaison à l'espacement se
fait sans flottant par produits élargis. Après positivité, la plus grande arête
`D` vérifie seulement `D<=2R` et `R<=D`, donc ce rapport est un proxy à facteur
deux. Avant positivité, aucun contrôle inverse utile ne suit. Une grande sphère
peut rester vide sur un nuage arbitraire, donc rayon ou ratio local ne ferment
jamais huit IDs.

Le « rang voisin » compte les distances strictement plus petites entre chaque
paire de slots, sans tie par vrai `PointId` ni contrat kNN dirigé/non dirigé. Il
prend le maximum des douze directions : c'est le seuil d'une clique kNN
mutuelle, pas celui d'une source ancrée ni d'une clique symétrisée-OR. Cette
métrique n'est donc ni nécessaire pour une route owner ni suffisante pour
exclure toute autre source locale. Si une
position dupliquée donne un plus proche voisin à distance nulle, `max(1,esp)`
invente en outre une échelle positive sans contrat. Au cap autorisé `n=260`, il existe
`186043585` quadruplets, jusqu'à `47627157760` tests in-sphere et, sur une
cohorte cosphérique shallow, `575990939160` distances de rang : la borne
d'entrée n'est pas un budget. Le mode s'exécute en outre après le WST et, en
ordre quatre, le compteur `Sym2` qu'il n'utilise pas. La directive à Claude est : brancher ce
mode avant le broad phase, préflighter un budget u128, publier `PARTIEL` au cap,
et nommer la population (`SupportKey` ou `BallKey`) ainsi que la relation kNN.
Le probe refuse en amont toute position Morton dupliquée, contrairement au
profil : les `PointId` et leur multiplicité doivent rester dans le census, seule
une paire endpoint `D=0` étant impropre. Par conséquent le cas `esp=0` masqué
par `max(1,esp)` est actuellement inatteignable dans ce mode, pas traité.
La voie reçue est :
positivité T--Q/L, BallKey/RLE, census `I/U`, métrique locale explicitement
versionnée, cap d'opérations et statut `PARTIEL`, le tout oracle-only.
Le mode doit aussi exiger `--ordre=4` et être exclusif de `--masse` ; il accepte
actuellement l'ordre trois par défaut tout en imprimant des q4.

La seule porte CTest de ce pin fixe `uniform,n=120`, attend par regex
`retenus=60378`, `ratio moyen=7,07` et `rang pire=109`, puis passe ici en
30,49 s. Elle n'exerce ni positivité, ni shell, ni `BallKey`, ni la fixture 50k,
et `PASS_REGULAR_EXPRESSION` n'impose pas le code final après la ligne. Son
temps à 120 points confirme un oracle borné de falsification, pas une route vers
50k.

Les réparations encore demandées à Claude sont de rendre le mode standalone ou
d'exiger `--ordre=4`, fixer explicitement les deux seeds, poser un `TIMEOUT` et
un cap u128 de travail, puis faire contrôler `rc=0` par un wrapper plutôt que par
une regex seule. Les fixtures de poids positif, négatif et nul, shell, tie
`PointId` et la construction analytique 50k doivent être séparées du snapshot
statistique.

### Commits `34cf05d` et `08dec609` : positivité q4 ponctuelle reçue sous u16, intégration ouverte

`c8::bien_centre` résout implicitement
`2 M c=(||p_i||^2-||p_0||^2)_i`, puis compare le centre et chaque sommet au
même côté de la face opposée. Sous indépendance affine, ce test est équivalent
aux quatre barycentriques strictement positives. Les bornes publiées sont
conservatrices sur u16 ; une redérivation donne `|N|<2^69` et `|dc|<2^104`,
donc i128 suffit. Le build ciblé est vert et `--centre` classe correctement les
quatre fixtures régulière, positive aplatie, poids nuls et obtuse. Un diagnostic
inline non versionné, seed `20260814`, a comparé une réimplémentation
Cramer/faces à la positivité Gram sur 20 000 tirages dans `[0,20]^3`, dont
19 929 non dégénérés, et trois permutations choisies par cas, sans désaccord.
Il n'a ni digest, ni commande conservée et ne constitue donc pas un reçu.

Les `18/18` portes Corner8/WST ciblées passent sur deux rejeux en
`13,39--19,31 s`. La porte
`mhgp3v_corner8_centre` est désormais enregistrée et, avec
`mhgp3v_supports_retenus_localite`, passe sur deux rejeux diagnostiques en
`0,00--0,02 s` et `5,10--9,61 s`. L'échec `rc=8` observé pendant le raccord
est historique : `08dec609` a corrigé l'échappement de la regex. Cette réception logicielle reste
incomplète. Les deux portes sont à `PASS_REGULAR_EXPRESSION`, les fixtures du
dépôt partagent le sujet et ne gravent ni permutation impaire, ni orientation
opposée, ni extrêmes u16, ni comparaison aux numérateurs `L_0,...,L_3`/Gram.
L'ABI booléenne fusionne dégénérescence, précondition invalide et poids non
positif, et le header `i64` ne préflighte pas le domaine avant les soustractions
signées : hors u16, l'overflow peut précéder tout fail-open. Il faut soit des
points u16 authentifiés par type, soit un verdict
`{positive,nonpositive,degenerate,invalid,numeric_failure}`. Le helper local
lambda sous `MHGP_HD` doit aussi être compilé sur la voie CUDA avant tout claim
device.

Le raccord transforme la population mesurée en
`Positive4 intersect {I<=7}`, mais ne répare ni shell, ni `BallKey/RLE`, ni
positions dupliquées, ni rang kNN typé. À `uniform,n=120`, il publie
`non_degeneres=8212806`, `bien_centres=600311 (7,31 %)` et `retenus=7344`.
Cette chute est un diagnostic utile de sélectivité, pas encore un `BallEvent`,
une pente ou un résultat H4 : elle mesure exactement
`Positive4 intersect {I<=7}`. Le commentaire CMake de la porte dit
encore à tort que la positivité n'est pas appliquée. Un autre commentaire reçoit
`4096` supports « tous bien centrés », alors que la fixture `--bloc` n'appelle
pas `bien_centre` : cette jonction support-complet vers Corner8 reste sans
oracle. Le titre de commit « H4 trente n » n'est donc pas reçu ; les deux
tailles uniformes donnent déjà `61--73 n`, contre `30--35 n` seulement sur les
amas, et aucun shell/RLE/rang fermé n'est calculé.

## P0 : `0A` reste ouvert sur u16

- les constructeurs q3/q4 rabattent vers `int64` des numérateurs qui atteignent
  respectivement 81 et 67 bits sur des fixtures u16 valides ;
- les chemins de clé et de positivité forment ensuite des intermédiaires au-delà
  de 128 bits ;
- `be_level_num` et `be_level_cmp` multiplient en `i128` signé avant de tester
  l'overflow. Sous UBSan, le mutant de clé existant déclenche un comportement
  indéfini à `ball_event.hpp:290` ;
- le vert Release de ce mutant n'est pas causal : `desaccords=0`,
  `fold_desaccords=0`, puis `fautes=263`, dont 262 proviennent de
  `runs.size()` ajouté sans comparaison à une vérité et une de l'erreur
  numérique ;
- le juge partage encore des identités, calculs et filtres avec le sujet ;
- `PointId`, epoch, profil, schéma, lanes, complétude et publication
  transactionnelle ne forment pas encore une ABI reçue ;
- le census reste payé par support avant le RLE par `BallKey`.

Le microkernel Gram unifie maintenant la réparation q2/q3/q4 sur un support
complet. Avec `G=M^T M`, `Delta=det(G)`, `r=adj(G) diag(G)` et
`t=M r`, les numérateurs des poids sont `r_i` et
`2 Delta-sum r_i`, tandis que
`Phi(z)=Delta||z-a||^2-(z-a)^T t` décide exactement la puissance. Il redonne la
boule diamétrale en q2, la boule ambiante en q3 et `Delta=O^2`, `Phi=O*J` en
q4. Un contrôle algébrique Python corrélé au profil u16 a vérifié ces deux identités sur
10 000 q4 non dégénérés après 10 029 tirages, seed `20260814`, avec digest des
fixtures
`e0a7f07cc819a8c402e44394baf2c41e38b837dba901153eddaa87d61905f4b1` :
[`AUDIT_RECU_GRAM_UNIFIE_1FD9CF1_20260814.md`](AUDIT_RECU_GRAM_UNIFIE_1FD9CF1_20260814.md).
Il ne reçoit aucun microkernel C++ et partage `det3` entre ses deux côtés ; les
fixtures déterministes de signe, shell, permutation et dégénérescence restent
à graver.

Au terminal q4, calculer directement `T=det3(v1,v2,v3)` et
`Q=sum_i ||v_i||^2*(v_j cross v_k)` : les quatre numérateurs de Cramer tiennent
sous 106 bits et la BallForm `T/Qbar` sous 87 bits sur u16. Sur un bloc au signe
indécis, la valeur finale de `Phi` reste sous 137 bits de magnitude, mais une
enclosure Gram naïve peut demander 140 bits signés d'intermédiaire ; i192 est
sûr. Les huit coins éliminent seulement la variable témoin : il faut encore
prouver `sup_support Phi(q)<0` à chaque coin, après indépendance et positivité
uniformes sur les facteurs support. Huit coins extérieurs ne prouvent jamais
`NONE`.

Une route G4 candidate garde le hot path `O/J` séparé en i128 sur les blocs de
signe uniforme, réserve un entier élargi au résiduel et ne paie le pgcd que
pour les survivants émis. La sûreté arithmétique de trois limbs i192 est
démontrée, mais son coût ne l'est pas : comparer i192 et i256 sur la fraction
résiduelle, les registres, spills et l'occupation avant de choisir. Ces formes
ne sont pas encore implémentées ni jugées par une autorité BigInt ; elles
rendent le P0 actionnable, pas reçu.

Une conséquence supplémentaire est reçue algébriquement : après RLE, toute
`BallForm=(A,B,C)`, `A>0`, emploie le même range-count exact sur AABB u16,
car `A||z||^2+B dot z+C` est une somme de trois quadratiques convexes séparées.
Minimum entier aux deux voisins du sommet axial clipé, maximum aux endpoints.
Les lanes-support ferment à `10/9/8` intérieurs pour q2/q3/q4 ; si une
`BallKey` est partagée entre arités, le run garde les bits encore vivants et ne
sature pas globalement à huit. Après census complet, `U_B` décide le rang fermé
et la disposition ; `c in conv(U_B)` est déjà garanti par le support positif.
Cette factorisation évite
un backend et un census par arité ; elle ne borne ni le nombre de `SupportKey`,
ni celui des `BallKey`, et n'est donc pas encore une preuve du SLO.

Autorité, valeurs exactes et fixtures :
[`AUDIT_BALL_EVENT_V0_2B89EA1_20260813.md`](AUDIT_BALL_EVENT_V0_2B89EA1_20260813.md).

## P0 : le fold live n'est pas `0B`

Le sujet écarte chaque `SphereRun` dont la disposition n'est pas `kRegular`,
puis le sujet et son juge consomment les mêmes runs, membres, niveaux et rangs.
Le nominal grid peut ainsi imprimer `refus_domaine=99` puis
`ball_event accord=OUI fold=OUI`.

Le calcul unionne `I_B union U_B` dans une seule DSU de `PointId`. Il juge une
primitive de goulot pour `K=1`, pas Morse/HGP aux ordres `1..10`. Dès `k=2`,
deux générateurs qui partagent un seul point doivent rester séparés, alors que
la DSU de points les fusionne. Manquent notamment :

- générateurs/facettes par ordre et lanes ;
- lots de niveaux égaux avec racines pré-lot gelées ;
- naissances, multifusions, coverage et continuations ;
- neuf applications verticales et payload officiel ;
- preflight complet et zéro sortie sur événement unsupported.

Le juge Floyd est en outre un oracle borné : ses deux matrices d'entiers
coûteraient environ 20 Go à 50 000 points et sa fermeture ferait
`1,25e14` triplets. Il ne peut devenir l'architecture produit.

Contre-audit et fixture `k=2` :
[`AUDIT_CONTRE_RECEPTION_STAGE_0B_3C11BC8_20260813.md`](AUDIT_CONTRE_RECEPTION_STAGE_0B_3C11BC8_20260813.md).

## Raffinement local : levier reçu comme diagnostic seulement

À `n=3000`, `s=8`, `window=512`, une exécution locale a mesuré :

| famille | profondeur | masse q4 ouverte | terminaux | recertifications |
|---|---:|---:|---:|---:|
| `uniform` | 0 | 1 027 538 | 885 188 | 108 858 186 |
| `uniform` | 4 | 464 599 | 1 221 936 | 193 020 841 |
| `eight_clusters` | 0 | 4 045 644 | 363 018 | 31 538 327 |
| `eight_clusters` | 4 | 2 597 699 | 1 182 988 | 199 169 436 |

Le ledger terminal est exclusif et final dans ces six rejeux. En revanche les
anciens compteurs `bank.closed`, `pending_lane` et `mass_closed_q2` comptent des
tentatives parentes avant leur scission, puis leurs enfants : ils peuvent
dépasser 100 %. Séparer impérativement `AttemptStats` de `TerminalLedger`.

Le prochain jalon utile est `ProofCarryingLocalRefinement-v0` : un enfant
hérite les spans `ALL/NONE` du parent et ne rejoue que sa frontière `MIXED`.
Mesurer ensuite le coût transitif `E4 -> F3/C4_carrier -> F4/M4_apex ->
BallKeys -> census -> fold`, pas la seule pente de `E4`.

Audit complet et défauts de la recette G4 :
[`AUDIT_CONTRE_RAFFINEMENT_LOCAL_ET_SESSION_G4_3C11BC8_20260813.md`](AUDIT_CONTRE_RAFFINEMENT_LOCAL_ET_SESSION_G4_3C11BC8_20260813.md).

## Rampe 50 000 : no-go borné, pas pivot automatique vers les cages

La session G4 CPU du commit publie quarante `code=0` et une provenance
substantielle. Les pentes q4 post-calculées sont arithmétiquement correctes.
Pour `eight_clusters/r4`, elles valent `1.898146 / 1.909154 / 1.911063`, avec
`pending=0` aux quatre tailles. Le seuil numérique préannoncé `1,7` justifie
donc l'arrêt d'ingénierie de
`CentralBall209 + DVT scalaire + s=8 + window=512 + raffinement r=4` sur cette
famille et cette graine. Le protocole plus large demandait plusieurs graines ;
ce n'est pas une réception asymptotique.

La portée s'arrête là :

- le script lance une taille par processus et ne calcule aucune pente ;
- `--max-slope=9` est inerte et aucune gate `sum_E4` n'est armée ;
- la complétude compte les lignes `code=` sans tester explicitement leur
  valeur. Un code non nul fait actuellement disparaître la ligne sous
  `errexit`, donc le fichier est rejeté, mais le diagnostic promis est perdu ;
- `terrain` contient du pending q4 à 25 000 et 50 000 ;
- le filtre retire recertifications, temps et HWM, donc le « prix exact » du
  front n'est pas mesuré.

`uniform` seule ne peut qualifier le SLO : `TEST_PLAN` §14.5 et G6 exigent
Poisson uniforme et le mélange équilibré de huit amas. `sum E4` reste la gate
du certificateur universel mesuré, mais la contre-famille `two_lines` interdit
d'en faire une gate de la source : elle conserve une masse quadratique avec
zéro carrier aigu et zéro q4.

Réponses complètes aux trois questions de Claude :
[`AUDIT_REPONSE_CRITERE_MORT_SOC64_LP_35FCEA8_20260813.md`](AUDIT_REPONSE_CRITERE_MORT_SOC64_LP_35FCEA8_20260813.md).
Le détail contractuel du reçu et des continuations est dans
[`AUDIT_CONTRE_RECU_RAMPE_RAFFINEMENT_G4_35FCEA8_20260813.md`](AUDIT_CONTRE_RECU_RAMPE_RAFFINEMENT_G4_35FCEA8_20260813.md).

## `C4/M4/H4` : le mur avant rang est réel, les claims du pin audité ne le sont pas

Le schéma courant sépare `C4_carrier` pour les faces aiguës,
`M4_apex` pour les quadruplets canoniques avant barycentriques,
`W4_positive` après positivité et `H4_rank` après census. L'owner maximal doit
rester dans la route proposée : changer seulement l'owner ne réduit ni les
vrais supports ni les 4-ensembles à attribuer. L'arête maximale fournit un
diamètre qui borne immédiatement les cinq autres distances. Une autre broad
phase exigerait une nouvelle preuve de couverture ; une `BallKey` est aval et
serait un owner de génération circulaire.

Le sampler v2 améliore v1 : il exclut `PENDING` et ne censure plus les grosses
lentilles. Le pin audité a remplacé la fausse barre `2 sigma` par une demi-largeur
Hoeffding correcte **conditionnellement** à des tirages indépendants et
uniformes. Cette loi n'est pas reçue : multiply-high reste sans rejet exact,
les deux streams SplitMix à seed fixe n'ont pas de contrat d'indépendance et le
delta n'est pas réparti sur les décisions simultanées. `W4` n'a pas d'intervalle
propre. Le champ est honnêtement renommé `repetitions_consecutives`, mais le
contrôle direct ne compare toujours pas le décodeur rang--`PairId` à un mode
exhaustif déterministe. La vue combinée SOC reste absente.

`--rang` peut sortir code zéro sans `--porteurs` et sans aucune ligne rang. Sur
`two_lines,n=40`, il peut aussi juger zéro bien-centré puis réussir. Son taux
est conditionné par arête et par positif, sans poids `binom(|L_e|,2)` : ce
n'est pas `H4/W4`. Surtout, il compte seulement `I_B`. Le rang fermé vaut
`|I_B|+|U_B|`; sous le régime régulier q4 il faut `I<=7` **et** `U=4`, ou un
refus des extra-shells pertinents. Le libellé `rang<=7` est donc faux.

Le nouvel énumérateur brut confirme seulement des comptes exhaustifs à petit
`n`. Il recopie Gram--Cramer et in-sphere du sujet : l'énumération est
indépendante, les prédicats ne le sont pas. Son claim
`M4=Theta(n^4)` « pour n'importe quel nuage » est réfuté par sa propre famille
`two_lines`, où `M4=0`. Les tailles `60/90/120` ne prouvent ni `H4` linéaire ni
une marge de deux ordres ; à `eight_clusters,n=120`, le rapport moyen publié au
seuil sept est seulement `6,6`. Les cas `n<4` produisent encore `NaN`, et aucun
cap n'empêche le coût `O(n^5)`.

Le mur mathématique existe néanmoins. Le support strict
`a=(20,20,20)`, `b=(30,30,30)`, `x=(19,31,31)`, `y=(31,19,31)` possède owner
`ab` unique, deux faces aiguës et barycentriques `(47,3,55,55)/160`. Un produit
de voisinages réels conserve ces inégalités et montre qu'une masse logique
quartique peut exister avant rang. L'ancienne instanciation par cubes unitaires
u16 n'était cependant pas uniforme : seuls `2093/4096` supports passaient.

La fixture de bloc exacte mise à l'échelle utilise
`A=(20000,20000,20000)+{0,1}^3`,
`B=(30000,30000,30000)+{0,1}^3`,
`C=(19000,31000,31000)+{0,1}^3`,
`D=(31000,19000,31000)+{0,1}^3` et
`Z=(20000,20000,30000)+{0,1}^3`. Ses `4096` supports ont owner `AB`, une
barycentrique minimale `13217143/721310286>0` et les huit IDs de `Z`
strictement intérieurs. Après fixation du signe d'orientation, le déterminant
in-sphere normalisé est convexe en `z`; ses huit coins suffisent donc pour
certifier `ALL_INTERIOR`. La bonne cible est une preuve de profondeur **par
bloc avant fill**, pas une énumération plus rapide des quadruplets.

Le count M4 n'exige pas davantage de fill. Initialiser `M4_pending` à la masse
distinct-ID non décidée, calculée par Möbius sur les quinze partitions de
`A/B/C/D`. Pour une masse `m`, `ALL_Q` fait
`M4_pending-=m; M4_L+=m`, `NONE_Q` fait `M4_pending-=m` et `MIXED_Q` remplace
atomiquement le parent par ses enfants ; `M4_U=M4_L+M4_pending`.
`M4_L>B_fill` rejette seulement le fill ponctuel. `M4_U<=B_fill` reçoit sa
capacité, mais count final, offsets et publication exigent encore
`M4_pending=0`. L'identité exacte par arête/`PlaneKey` reste un microkernel
endpoint borné : à 50 000 points, un RLE global matérialiserait `1249975000`
arêtes et `62496250050000` incidences `(e,z)`. Les compteurs soustraits restent
complets ; les saturer séparément peut transformer `B+2-(B+1)=1` en zéro.
`M4_raw` pré-profondeur et `residual_output` après profondeur sont deux ledgers :
une fermeture de profondeur crédite `domain_mass_closed`, pas `M4_closed`.

La route reçue comme proposition est : proposer des bases sur l'arête, les
vérifier par `BlockJungDual64`, puis fermer seulement si `tau(E_Q)>=8` ;
`FaceAxisJungDepth8` après la porte aiguë, puis
`Corner8BallDepth/BlockBallDepth8` après la cellule apex. Un reçu de profondeur
`ALL` ferme un bloc, `MIXED` scinde et la sweep ne reçoit que le résiduel
preflighté. `2B_R` est une enveloppe extérieure sharp lorsque seule la
boule contenant les endpoints est connue ; elle réduit les cellules, pas la
masse réelle. Une grande masse logique peut tenir dans peu de blocs, mais elle
n'est utile que si le consommateur de profondeur reste lui-même factorisé.
Sur une face fixe, ce consommateur est désormais explicite : poser
`p=min(8,n_permanents)`, puis conserver les `8-p` seuils gauches les plus grands
et les `8-p` seuils droits les plus petits donne un noyau d'au plus 16 IDs qui
préserve exactement le seuil `Depth>=8`. Un scan
top-k `O(n)` et un replay des égalités remplacent la sweep et le DAG général ;
une égalité uniforme est groupée comme shell, seul un ordre indécis scinde.
Les bouts irrationnels montent vers 207 bits sous u16 et exigent i256/quatre
limbs ; le lift bloc garde sa propre preuve de largeur.

La dissection `n=1500` trouve huit témoins singleton exacts pour `89,5 %` de
200 PairId q4 ouverts tirés par masse, contre `26,5 %` de 200 rectangles hachés.
Elle révèle du potentiel pairwise, sans mesurer la perte causale : les deux
échantillons ne sont ni appariés ni pondérés dans la même unité, et le second
n'exerce aucun groupe Jung. L'option live `--ordre-proche` réduit fortement le
pending aux fenêtres 32--128, mais à finalité 256/512 le résiduel reste exactement
`E4=1071162` dans les deux ordres. C'est une compression de budget du
certificateur central, pas une réduction M4.

Réponses aux six questions, preuves, contre-fixtures et portes :
[`AUDIT_REPONSE_M4_PORTEURS_AIGUS_4515A8B_20260814.md`](AUDIT_REPONSE_M4_PORTEURS_AIGUS_4515A8B_20260814.md).
Le contre-audit du sampler v2, du brute-force, la réponse entière à la question
7 et les microgates `JungDual/BlockBallDepth8` sont dans
[`AUDIT_CONTRE_RECEPTION_M4_V2_DEPTHBLOCK_5BFC5C8_20260814.md`](AUDIT_CONTRE_RECEPTION_M4_V2_DEPTHBLOCK_5BFC5C8_20260814.md).

## `JungDepth` et `BlockJungDual64` au pin historique : théorème fixe reçu, intégration ouverte

La forme entière duale `A/P/R` est correcte sous u16, paire propre et
`sum(weights)<=65535`, mais elle vérifie seulement les poids fournis. Son échec
reste `UNKNOWN`. `BlockJungDual64::make_base` préflighte la somme en `i128`, mais
`dual_lane` l'additionne encore en `i64` sans ce cap ; aucun des deux headers ne
porte les `PointId`, et le mutant étroit utilise un overflow signé. Le wrapper
futur authentifie donc profil, cap, IDs et disjonction. Il peut exposer une
primitive nommée `verify_dual_weights_lane`, mais ce symbole n'existe pas au
ce pin et ne serait jamais un décideur de couverture.

Le pin audité ajoute le bon juge indépendant pour une base fixe. Dans le plan
`s dot (b-a)=0`, il minimise exactement `||s||^2` sur les demi-plans mauvais,
en BigInt, puis compare strictement `3*r^2>D` ou `2*r^2>D`. Les formes
`K_ij`, `Delta` et `N` sont algébriquement cohérentes et les fixtures q4 de
taille un/deux passent. Les `13/13` CTests Jung ciblés sont verts. Il manque
toutefois le cas Helly réellement ternaire, les normales projetées nulles et le
preflight u16. Une fixture exacte nécessaire prend
`a=(0,100,100)`, `b=(100,100,100)`, puis
`z1=(5,90,100)`, `z2=(5,100,90)`, `z3=(0,110,110)` : les régions mauvaises
`Y>=75/2`, `Z>=75/2`, `Y+Z<=20` ont chaque intersection par paire non vide dans
le disque q4, mais leur triple est vide.

Le titre du commit « le primal retire encore un espoir » n'est pas reçu.
`--primal` ne remplace que les groupes de taille deux d'un greedy disjoint sur
`--voisins`; la taille trois emploie encore une banque finie. Il mesure donc
`p`, jamais la profondeur `d`. Aucun CTest ne lance ce mode et le claim « quatre
mesures identiques » est faux sur le binaire du même pin : à
`eight_clusters,n=600`, les fermetures passent `169 -> 170`, et à `n=1500`,
`189 -> 190`. Même sur sa cible réduite, le primal récupère déjà une base.

La fixture `u<p<d` du probe ne calcule pas `d` : elle imprime le littéral huit
après avoir cherché seulement singletons/paires et extrait greedily `p`.
Dupliquer géométriquement `g1` comme troisième gadget laisse cette porte publier
`u=6,p=7,d=8`, alors que le vrai minimum tombe à sept. Le prochain juge doit
calculer `d` ou son équivalent combinatoire, jamais le graver.

Cet équivalent est maintenant exact. Soit `E_q(P)` l'hypergraphe de rang trois
des bases Helly couvrantes. Les intérieurs de tout centre frappent chaque base,
et tout transversal des bases laisse, par Helly, un contre-centre sur les IDs
restants. Donc :

```text
Depth_q(P) = tau(E_q(P))
```

Le packing courant n'est que `nu(E)<=tau(E)`. Un branch-and-cut alterne un
solveur bitset de transversal et le juge primal : un transversal `R`,
`|R|<h`, donne soit un contre-centre sur `P minus R`, soit une nouvelle base
disjointe de `R`. Chaque base géométrique n'est vérifiée qu'une fois ; le replay
GPU devient combinatoire.

Le lift uniforme de chaque hyperarête est lui aussi résolu. Pour une base et
des poids fixes, poser
`A0=-W*(a dot b)+(a+b) dot Z-Q` et
`C0=W*(a cross b)-a cross Z-Z cross b`. q4 teste
`A0>0 && 2*A0^2>||C0||^2`, q3 remplace `2` par `3`. Le couple `(A0,C0)` est
affine séparément en `a` et `b`, et chaque lane est un cône de Lorentz convexe.
Les 64 couples de coins caractérisent donc exactement `ALL` sur l'enveloppe
`A×B`. `BlockJungDual64` ajoute au branch-and-cut seulement les bases qui
passent uniformément ; les autres provoquent un split. Sous u16 et
`1<=W<=65535`, les identités `A4=4*A0`, `R=4*||C0||^2` et les bornes
`|A0|<2^50`, `|C0_i|<2^49` placent ses comparaisons dans i128. Cette combinaison
`primal proposer -> dual64 verifier -> tau(E)>=8` est le premier certificat
constructif fail-open de `BlockJungDepth8` sans tableau de `PairId`. Son pire
cas peut encore visiter toutes les feuilles du produit.
Ces bornes supposent un widening avant les sommes, normes et produits, ainsi
qu'un preflight de `W` en accumulation large ou saturante.

Aucun artefact durable ne permet d'identifier un second auteur ou modèle : Git
attribue les deux flux documentaires au même auteur. Le présent contre-audit
redérive néanmoins le résultat. À base et poids communs fixés, `(A0,C0)` est
séparément affine et la lane est un cône de Lorentz strict convexe ; la double
interpolation des 64 coins est donc correcte sur l'enveloppe AABB continue. La
réserve `for all pair exists lambda(pair)` contre
`exists lambda for all pair` reste impérative. Les constantes 80/99 sont
cohérentes avec Montejano--Oliveros, théorème 3.1, puis la borne stricte de Tuza
`eta(3,h)<(h+1)^2`. Cette redérivation valide les énoncés, pas une indépendance
organisationnelle ni l'intégration live.

La réception logicielle de bloc reste ouverte. Les tests du header comparent
les deux formes sur des couples ponctuels puis des AABB dégénérées en points.
Le commit ajoute trois CTests d'intégration WSPD, dont deux mutants de ledger,
mais toujours aucun produit AABB non dégénéré, changement de poids par coin ou
preflight coordonnées/boîtes/IDs du wrapper. `bjd_lane_box` paie en outre un
prétest intérieur puis jusqu'à 64 coins : le pire cas annoncé est 65 prédicats,
pas 64.

Son ABI confond encore invalidité et réfutation : `!base.valide` retourne
`kLaneNone`. Le callsite q4 live reste fail-open parce qu'il teste seulement
`retour>=q4`, mais un consommateur générique de lanes pourrait publier un faux
`NONE`. La réception exige `ALL_GROUP/MIXED/INVALID_OR_UNKNOWN`. Le commentaire
de `jung_dual.hpp` inverse par ailleurs le minimax ; la bonne identité est
`min_w max_z Phi_z(w)=max_lambda min_w sum_z lambda_z Phi_z(w)`. Les formules
codées suivent la bonne identité, donc cette faute documentaire ne réfute pas la
primitive.

Un wrapper de profondeur reçu devra conserver des ensembles authentifiés de
`PointId`, pas seulement `cred/ccred`. Le packing minimal exige chaque groupe disjoint des
autres groupes et de tous les singletons/spans déjà crédités dans la même vue.
La route plus complète garde les groupes comme hyperarêtes et teste
`tau(F)>=h`, ce qui traite leurs recouvrements sans double crédit. Un cap de
juge donne `PARTIEL/UNKNOWN`; il ne transforme jamais un raccord non jugé en
accord.

Le packing live est désormais sûr sous ses invariants de probe, mais il est
NO-GO comme hot path dans son ordonnance actuelle. À `n=1500`, les lectures
restent exactement identiques avec et sans BJD (`32387961` sur `uniform`,
`9366805` sur les amas), tandis que les médianes CPU utilisateur augmentent de
`5,47 %` et `8,15 %`. Le gain de masse q4 vaut respectivement `12,55 %` et
seulement `0,87 %`. Le certificat doit donc fermer avant la descente. Le
préfiltre `BJD-BilinearBounds` redérivé dans le contre-audit est sûr : 36 extrema
bilinéaires donnent le minimum exact de `A0` et une surborne de `||C0||^2` ; un
succès implique le verdict des 64 coins, un échec retombe sur ceux-ci ou
`MIXED`.

Helly avec tolérance borne en plus le payload ponctuel. Pour tout
`R`, `|R|<=h-1`, la profondeur exige l'intersection vide des mauvais convexes
hors `R`. Il existe donc un `ToleranceKernel` de taille
`eta(3,h)<(h+1)^2` : par intégralité, au plus **80 IDs pour q4** et **99 pour
q3**, sans affirmer l'optimalité de ces constantes. Ce théorème est existentiel
et pointwise : il ne borne ni la découverte, ni le nombre de proof-tiles, et
`for all pair exists kernel` ne donne aucun noyau commun à un rectangle CK.
Sur une tuile, les mêmes bases doivent être vérifiées uniformément ou provoquer
un split. Après une face aiguë fixe, la dimension un est plus forte : les top-k
seuils donnent constructivement un noyau d'au plus 16 IDs pour q4.

## SOC64 actif au pin historique : baisse locale du résiduel, aucune campagne G4 reçue

Le faible gain du classifieur scalaire `D/V/T` ne réfute que des extrema
décorrélés. Il ne réfute ni le spindle ponctuel, ni les rectangles corrélés, ni
le disque de Jung.

Pour `e=z-a`, `t=b-z`, `H=e dot t`, `E=||e||^2`, `X=||t||^2`, q4
exige `H>0` et `3H^2>EX`. Si les 64 couples de coins de
`(C-A) times (B-C)` passent, tout le rectangle est `ALL`; le premier échec est
seulement `UNKNOWN`. La fixture axiale où toutes les différences sont
colinéaires positives ferme q4 par `SOC64` alors que la borne scalaire échoue.

Le commit `110fe76` ajoute `--soc64-actif` comme disjonctif q4 : un succès
change le fate sans double-compter ses descendants. Son message publie, à
`n=3000`, `uniform 22,842 % -> 13,602 %`,
`terrain 12,603 % -> 6,781 %` et
`eight_clusters 89,933 % -> 73,767 %`. Aucun transcript de commande ni reçu de
sortie associé dans le dépôt ne permet de transformer ces chiffres locaux en
pente ou en résultat G4.

Le branchement demeure incomplet pour la vue combinée : sous `central-NONE`,
un échec SOC au nœud courant ne descend pas vers d'éventuels descendants
SOC-`ALL`. Cela perd des fermetures mais ne crée pas de faux `ALL`. Les ledgers
baseline, SOC et futurs groupes Jung gardent des preuves d'identités et des
statuts `PENDING` séparés.

La session `soc64_actif_g4_20260814` n'a produit aucune rampe. Elle a construit
seulement `mhgp3v_wspd_wavefront_probe`, puis un regex CTest plus large a
demandé des exécutables absents ; CTest a rendu `rc=8`. Le transcript conserve
l'échec et `stop_and_verify.sh` certifie exactement
`devpod-gpu-exploration/europe-west4-ai1a/ehgp-blackwell-spot-ai1a` en
`TERMINATED`. Ce reçu qualifie l'arrêt, pas SOC64, une pente ou le SLO. Le
script inchangé jusqu'au commit `a58d020` est seulement corrigé partiellement et n'a pas de
reçu d'exécution : il omet encore `mhgp3v_jung_dual_judge`, sélectionné par son
regex CTest, et ce regex ignore les portes `mhgp3v_bjd_*`. Il avale le code de
l'analyseur, omet `--exige-fenetre-finale`, gate le seul `sum_E` à `1,70` au
lieu des compteurs physiques à `1,35`, et autorise `4*3000 s` par job sous des
coupe-circuits de `4800/5400 s`. Sa rampe `s=8,r0` reste CPU-only, sans aval
officiel et sans vue SOC complète sous `central-NONE`.

Prochaine porte : stabiliser le worktree, construire toutes les cibles réellement
demandées, rejouer localement les mutants et l'oracle d'union par `PointId`,
puis mesurer les mêmes tâches à `1500/3000/6000` avec `pending=0`, coût, HWM et
sorties. Aucune nouvelle rampe 50 000 avant cette porte.

`JungDiskDepth8` restreint le plan des centres au disque
`||y-d||^2<=D/2`. Une fixture à huit groupes disjoints ferme ce disque alors
que le LP global échoue même à profondeur un. Sous `smax=11`, le LP à 3280
appels correspond à `h=8`; il reste un oracle pairwise, jamais le hot path ni
le diagnostic d'une « vraie pénurie » sur le domaine Morse.

Preuves, largeurs, fixtures et cascade :
[`AUDIT_REPONSE_PLAN_VERTICAL_SOC64_LP_1AA487D_20260813.md`](AUDIT_REPONSE_PLAN_VERTICAL_SOC64_LP_1AA487D_20260813.md).

## Contre-audit croisé des flux documentaires

Dans ce corpus, « l'autre auditeur » est une relation locale au document et ne
désigne aucune identité globale vérifiable. Par exemple,
`AUDIT_REPONSE_PLAN_VERTICAL_SOC64_LP_1AA487D_20260813.md` section 0.1 relit
explicitement `AUDIT_REPONSE_ROUTE_VERTICAL_SLICE_1AA487D_20260813.md`, tandis
que d'autres rapports relisent PLAN ou une proposition non nommée. Lorsqu'un
lien explicite existe, il est cité ; sinon la provenance est indéterminée. Les
conclusions sont reçues ou rejetées par preuve, fixture et pin, jamais par
autorité personnelle.

`AUDIT_REPONSE_ROUTE_VERTICAL_SLICE_1AA487D_20260813.md` a correctement
découvert l'owner décidé sur
`GenerationRank` plutôt que `PointId`; Claude l'a réparé localement au commit
`f516198`. Il a aussi correctement relevé l'incomplétude de `0A` et la nécessité
des lots/verticales.

Les propositions LP/cages de
`AUDIT_REPONSE_PLAN_VERTICAL_SOC64_LP_1AA487D_20260813.md` sont recevables après les restrictions intégrées
dans la proposition consolidée : complétude LP seulement sur le pool mondial,
largeur du constructeur distincte du vérificateur `F`, traitement des bases de
rang inférieur, et recalcul de la fleur après réduction d'une cage. Les comptes
`32/36` ne valent que pour des tétra-cages : les maxima six-sites deviennent
`64/72`; `P=48` est une capacité q4, pas une existence. Pour une base positive
minimale 3D, le seuil angulaire suffisant est `delta>=4h-3`, soit `29` pour
huit cages, et n'est jamais une condition nécessaire. Son oracle de profondeur
ne prouve pas l'absence d'un support source et son arbre à 3280 LP pour `h=8`
ne doit pas être présenté comme GPU/hot path.

Sa restriction « WSSD compacte ne borne pas la sortie » est correcte. Sa
conclusion « WSSD seulement broad phase » est trop étroite : avec partition
CK, owner total, certificats `[L,U]` et blocs paresseux, `OwnedCK-WST` devient
une source factorisée exacte. Elle ne devient toujours pas une liste sparse.
De même, le théorème des faces aiguës q4 est utile uniquement avant le rang ;
la fixture 64 points interdit toute cascade depuis les événements q3 retenus.

Le présent contre-audit du probe live resserre à son tour les formulations
de `AUDIT_SOURCE_CK_WST_Q2_Q3_Q4_35FCEA8_20260814.md` : les
conditions précédentes sont un contrat, pas l'état du tape `22d1cb0`. Le juge
choisit l'owner puis ignore les autres ancres, tandis que le producteur brut ne
projette ni owner, ni distinct-ID et partage un tie-break Morton.
`AUDIT_SOURCE_CK_WST_Q2_Q3_Q4_35FCEA8_20260814.md` avait raison de demander une
broad phase et de refuser une borne de sortie ; son angle mort était la couture
entre l'owner abstrait et les records réellement comptés. Le statut live reste
`CandidateCover`.

`AUDIT_CONTRE_RECEPTION_M4_V2_DEPTHBLOCK_5BFC5C8_20260814.md` corrige également
les formulations antérieures : `2B_R` est sharp seulement à information de boule
contenante fixée ; un poids `lambda` commun rend `BlockJungDual` sûr mais
incomplet à cause du swap `for all pair exists lambda`; et un glouton arbitraire
de groupes sur l'axe n'est pas complet. La formulation est maintenant plus
simple : les extrema top-k donnent directement le noyau tolérant exact, sans
matching global. Ces corrections sont intégrées à la proposition et aux fixtures. Pour
`BlockBallDepth`, une profondeur moyenne élevée ne prouve pas encore que huit
mêmes groupes couvrent uniformément un bloc : cette efficacité reste une gate
falsifiable.

Le même contre-audit valide les signes et seuils stricts du primal Jung,
mais corrige deux nouveaux raccourcis : i256 n'est sûr qu'après la réduction
`g_i/K_ij`, et les comptes `3280/9841` appartiennent aux profondeurs `h=8/9`
sous `smax=11`. Il réfute aussi l'interprétation du diagnostic feuilles : seul
le témoin est singleton, `A/B` restent des boîtes, et `pending=0` n'implique
pas que les sous-arbres `central-NONE` ont été visités. Ces corrections sont
maintenant dans la proposition et dans l'audit M4 actif.

Le prototype historique `JungDual` code correctement la forme entière ponctuelle
`A/P/R` et ses seuils q3/q4 sous la précondition u16 et `sum w<=65535`. Son
`dual_lane` ne préflighte pas cette somme et les paramètres zéro rendent
l'ablation vacuaire. Le nouveau `BlockJungDual64::make_base` corrige le cap pour
sa propre base, sans durcir l'ancienne ABI. Le juge primal séparé reçoit désormais le collectif `k=2`, mais aucune
fixture `k=3`, aucune profondeur `tau(E)` et aucun rectangle CK uniforme. Le
raccord `--primal` reste un packing greedy et son claim de gain nul est réfuté
par les replays `169 -> 170` et `189 -> 190`. Son succès représente une paire
ponctuelle, jamais encore une proof-tile uniforme ni un chemin device.

Le contre-audit détaillé corrige aussi les anciennes confusions entre q2 et q4,
cutoff kNN, taux empirique et borne structurelle.

## Rejeux ponctuels

```text
ball_event Release ciblé : 10/10 verts
suite ciblée ball_event/WSPD/Gamma/postings/saturated : 87/87 verts
faces q4 exactes : 5/5 ; q3 côtés hors rang : 6/6
fixtures centre_cell arité 3/4 + mutant cascade : 3/3
SOC64 isolé : 16/16 ; WSPD--SOC intégré : 5/5, oracle d'union incomplet
WSPD--SOC/porteurs/two_lines/cap au snapshot historique : 17/17 en 33,18 s
q4_brute au snapshot historique : 5/5 en 2,40 s, prédicats et shell non indépendants
sous-suite SOC/WSPD/porteurs/two_lines/q4/Jung/diag/ordre : 56/56 en 25,79 s
contre-calcul BigInt ad hoc Corner8 (pas une CTest) : 4096/4096,
  marge owner=11892000,
  min bary=13217143/721310286, pire J_U=-79011820908103787995
dissection de perte live : 1/1 en 0,80 s, populations non appariées
flip direct cap 1000 : accord=OUI, juges=47, sautes=3, faux=0 (statut faux)
mutant somme cap complet : code 4, juges=168, sautes=0, faux=25
JungDual UBSan étroit : overflow signé à jung_dual.hpp:157
Jung ciblé au snapshot historique : 13/13 verts en 0,31 s, collectif k=2 reçu, k=3/tau(E)/OPEN_FINAL absents
BJD header au snapshot historique : seulement point + boîtes dégénérées ; produits non dégénérés absents
BJD intégré au pin 5809bd2 : 3/3 CTests verts, nominal et deux mutants de ledger
rejeu consolidé précommit SOC/Jung/BJD : 10/10 verts en 1,21 s sur cette machine
pin 5809bd2, BJD cap 1 : code 0/OK avec groupes sautes=98 et fermetures sautees=10 (statut faux)
pin 5809bd2, BJD sans vwave : code 0/OK, essais=0, couvrants=0 (mode vacuaire)
snapshot historique, SOC actif + juge shadow : zéro verdict actif jugé ; raccord actif sans autorité intégrée
snapshot historique, judge-vwave + SOC/BJD : code 1, 149 fermetures dites « sans 10 » ; juge central incompatible avec les preuves collectives q4
rejeu local historique Jung/BJD : 16/16 verts en 1,45 s, dont 13 Jung et 3 BJD intégrés
pin 694920a BJD ciblé : 8/8 verts ; 0,26 s présent rejeu, 0,36 s précédent
  PARTIEL code 3, sans-vwave code 2, mutants code 4
pin 694920a exige-q4-ouvert seul : code 0/OK ; collinear_seven points=200 exécute n=9
pin 8fd6f59 BJD ciblé : 8/8 verts ; exige-q4-ouvert sans juge et collinear n=200 refus code 2
pin 8fd6f59 collinear_seven sain : zéro groupe, zéro fermeture, 55 visites de feuilles déjà créditées rejetées
snapshot historique, midball standalone : 9/9 affichés verts en 0,95 s présent rejeu ;
  deux portes saines à regex, doublon de rect_h_interval, réception ouverte
snapshot historique, midball --selftest=1 : accord=OUI imprimé puis plancher code 3
snapshot historique, fenetre-exacte n=200, S=1000 : 198000 scans ; q2 exact échantillonné, q3/q4 majorants ; aucune CTest dédiée
snapshot historique, exhaustif n=200 : 19900 paires, U<h=3790/10059/10937, 3184359 tests
snapshot historique, exhaustif --points=8,9 : n=9 sauté par retour après les 28 paires de n=8
snapshot historique, exhaustif + plancher BJD impossible ou mutant : code 0, gates court-circuitées
pin a58d020 WSPD 63c79bd6 / CMake 4c6cb24e : build vert ; 13/13 affichés verts
  n=1500 amas : lectures -7,41 %, résiduel q2 -29,95 %, vague médiane +14,1 %
  multi-n : compteurs hérités ; cap i64 et bypass exhaustif du juge ouverts
révision borne-sup 90640885 : zéro fermeture q2/q3/q4, pending=0, fenêtre finale OUI ; réfuté
révision commise ec5ec3d4 : fates/masses appariés sans SOC/BJD/climb ; lectures -0,0274 %
  ledgers combinés, BJD, climb, CTest de parité et mutant de conservation ouverts
pin 1fd9cf1 Corner8 : 6/6 ciblés ; bloc=4096 supports/32768 témoins, oracle ponctuel corrélé
  selftest=1 imprime accord=OUI puis code 3 ; drop-corner/norme hors porte ; produit-OJ absent
pin 22d1cb0 WST3 : 5/5 ciblés ; uniform n=120, 280840 triangles, zéro manquant/doublon owner
pin 22d1cb0 WST4 : 1/1 ciblé ; uniform n=60, 487635 quadruplets, zéro manquant/doublon owner
  copies non-owner, diagonales, vrai PointId, doublons de position et coût non jugés
pin 1fd9cf1 : dix portes WST3/WST4 ; 10/10 vertes en 31,49 s, durée non benchmark
pin 1fd9cf1 : 16/16 portes Corner8/WST3/WST4 ciblées vertes ; aucune ne passe --corner8 ni n'exerce le bisigne
pin 1fd9cf1 --masse n=60/S=1000 : 9,00 % IDs répétés, 76,10 % owner hors, moyenne 25,3, retenus 1,00 % ; sortie biaisée/non positive
pin f1b78c0 --supports-retenus n=120 : 1/1 regex verte en 30,49 s puis 33,91 s ; population Raw4 non positive, mode q3 par défaut
révision transitoire pré-08dec bien_centre + supports n=120 : CTest rc=8 en 8,63 s ; bien_centres=600311, retenus=7344
pin 08dec609, Corner8/WST ciblés : 18/18 verts en 13,39--19,31 s ; centre 0,00--0,02 s, supports 5,10--9,61 s ; regex fail-open, shell/RLE absents
catalogue historique : 839 CTests après reconfiguration ; aucune suite complète 839/839 rejouée ici
ablation BJD n=1500 uniform : masse q4 -12,55 %, CPU user médian +5,47 %, lectures identiques
ablation BJD n=1500 amas : masse q4 -0,87 %, CPU user médian +8,15 %, lectures identiques
session G4 SOC actif : CTest rc=8, aucune rampe, cible TERMINATED
ablation primal amas n=600 : 169 -> 170 fermetures ; n=1500 : 189 -> 190
--rang=10 sans --porteurs : code 0, aucune ligne rang
two_lines n=40 avec rang : code 0, bien_centres_juges=0
suite complète : interrompue après 28/734 terminés, tous verts
grid n=16 : refus_domaine=99 puis fold=OUI
clusters n=5, coord=4 : timeout après 2 s, capacité non preflightée
UBSan mutant clé : overflow signé à ball_event.hpp:290
```

Ces rejeux falsifient des claims précis ; ils ne qualifient ni exactitude
publique, ni performance.

## Ordre bloquant

```text
réparer 0A u16 et isoler les juges de mutants
  -> raccorder BallEvent aux autorités Gamma par ordres/lots/verticales
  -> recevoir 0B et le payload borné
  -> recevoir manifeste NoDelaunay + NeutralPairPartition exacte et immuable
  -> FORK Lane2 : Pair2 -> MidballDepth10 -> BallKey/RLE
  -> FORK Lane3 : PairAnchor3 x Third3 -> Q3MiniballDepth9 -> BallKey/RLE
  -> FORK Lane4 : PairAnchor4 x Q4Seed3 x Fourth4
       -> Q4SeedAxisTopR4 -> owner6/primary/Positive4 -> BallKey/RLE
  -> prouver séparément couverture, caps, pending=0 et payload de chaque fork
  -> mesurer leurs tâches, racines, sorties, octets, census, H et HWM
  -> seulement alors portage device et campagne G4 50k
```

Cet ordre bloque toute réception produit, pas la falsification architecturale.
En parallèle, une piste `counter-only` peut recevoir sur petit `n`, contre
vérité exhaustive, `NeutralPairPartition -> Lane4(Q4Seed3Block ->
Q4SeedAxisTopR4)`. Elle ne promeut ni
0A, ni 0B, ni le statut public ; elle permet de mesurer les morts
`T2/permanents/gaps`, les sélections extrémales, les ties, les splits, les
octets et la HWM avant d'investir dans un portage G4.

La proposition consolidée est
[`../PROPOSITION.md`](../PROPOSITION.md). L'index court des audits est
[`README.md`](README.md).

GCP non utilisé par le présent auditeur ; la session échouée de Claude est
certifiée `TERMINATED`.
