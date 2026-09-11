# Des résolutions indépendantes aux lots GPU résidents

11 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

Le nouveau raccord par lots est qualifié localement en émulation hôte. La couture optionnelle `83f1c78e…` est intégrée au Builder avec seize nouveaux CTests ; la reconstruction active commune passe les **40 CTests ciblés**. La variante CUDA corrigée compile et lie sans avertissement en SM120, mais reste un prototype séparé. **Aucun kernel de ce contexte n'a encore été exécuté sur carte ; aucun nouveau temps de tour 50k n'est mesuré.**

## Ce qui change

La phase géométrique n'a plus besoin de consulter l'état temporel des composantes. Pour chaque K, le constructeur dédoublonne les représentants, traite les semis initiaux, puis transmet les clés restantes en un lot. Chaque requête rend une identité de boule terminale, jamais un identifiant union-find. Le calendrier CPU retrouve ensuite les parents pré-lot et produit les mêmes forêts et verticales.

| Partie | Travail du nouveau raccord | Partie encore sur CPU |
| --- | --- | --- |
| Préparation | Index, catalogue et semis capturés une fois dans un propriétaire commun | Génération WSPD, census, tri et dédoublonnage des requêtes |
| Lot géométrique | MEB, clé, recherche du premier intrus et descente dans le même kernel ; raccourci exact après échange conservé | Validation des résultats du lot et restitution des occurrences |
| Histoire FULL | Identités terminales indépendantes du calendrier | Contributions datées, multifusions, normalisation des ancres, verticales et sortie retenue |

Une grille CUDA finie parcourt les requêtes avec une boucle de pas égal à la taille de grille. Sa taille ne plafonne donc ni le nombre de demandes ni la longueur d'une descente. Il n'y a aucun aller-retour CPU/GPU par support ou par intrus. L'index, le catalogue et les semis restent résidents entre les lots ; K1 conserve son chemin vers les feuilles points.

## Réductions de transferts vérifiées

Le premier prototype envoyait une sentinelle complète pour chaque résultat à chaque lot. Le nouveau ne met les sorties à zéro que lors d'une allocation de capacité nouvelle. Un numéro de lot strictement croissant distingue ensuite un résultat neuf d'une ligne omise ou périmée. Les omissions sont effectivement refusées après réutilisation, rétrécissement puis agrandissement des buffers.

Dans l'ABI hôte qualifiée, une demande occupe 104 octets et une ligne de résultat 232 octets. Le H2D récurrent de **232 octets par demande** est supprimé ; les résultats reviennent toujours en D2H. La mise à zéro de capacité nouvelle est comptée séparément, pas maquillée en transfert gratuit. Sur la séquence de réutilisation du test : 24 demandes, 2 496 octets H2D, 2 552 octets d'initialisation locale, deux réserves et quatre allocations.

La copie intermédiaire de publication passe de 208 à 16 octets par résultat, soit **192 octets de copie hôte évités par demande**. Le diagnostic brut et la dernière copie compacte existent encore. Ces volumes ne sont ni un facteur de vitesse GPU ni un gain de pic mémoire mesuré ; le compteur de capacité du callback ne comprend pas toute la résidence index/catalogue/semis et tout le staging hôte.

## Exactitude et erreurs

Toutes les demandes sont prévalidées et toutes les sorties vérifiées avant publication. Une erreur n'en publie jamais un préfixe. Les ordinaux conservent la première occurrence d'origine, même au-delà de 2³². Le propriétaire lie les identités géométriques ; une simple égalité de tailles ne suffit pas. L'appelant conserve les données sources immuables pendant le calcul.

La relecture a détecté un défaut de comptabilité : un débordement pendant la somme laissait un total partiel déclaré connu. L'adaptateur publie désormais la somme complète atomiquement. Le Builder fusionne le travail avant les compteurs de capacités ; si cette fusion elle-même déborde, le travail est déclaré inconnu. La [contre-fixture indépendante](../audits/receipts_batch_work_20260911/README.md) réfute l'ancien adaptateur et valide le corrigé en O2/SAN. Elle ne prétend pas simuler deux résolutions géométriques réalisables avec des compteurs proches de la limite entière.

La validation du lot contrôle forme, identité, fenêtre d'admission, antériorité et cohérence des compteurs. Elle n'est pas un juge géométrique capable de réfuter toute mauvaise boule pourtant admissible. Les gates comparent donc aussi les **BallId terminales directement**, en plus des forêts physiques, des coupes et des verticales.

## Preuves séparées

La [couture CPU](../receipts/static_batch_seam_20260911/README.md) porte le candidat `83f1c78e…`, optionnel et synchrone. O2/SAN confrontent les bras un/quatre threads, 34 nuages, 150 ordres et 87 230 contrôles verticaux par bras ; 118 terminales directes dans 56 lots, et 65 cas de frontière dans le harnais final. Le cache temporel par défaut reste inchangé.

Le [contexte GPU en émulation hôte](../receipts/gpu_terminal_batch_host_20260911/README.md) consomme ce même Builder, le terminal semé `d73de05f…`, la route `a62bd1d5…` et l'adaptateur `993786f3…`. Chaque build O2/SAN passe 28 044 contrôles, 64 paires de forêts, 176 terminales directes, huit hits semés, 41 rejets transactionnels et un débordement synthétique. Les premiers échecs de compilation et du harnais sont conservés, pas transformés en succès rétroactifs.

Le vrai raccord census→tour et son raccord au nouveau callback ont leurs qualifications distinctes : les petits catalogues du test de transport ne deviennent pas une preuve de génération WSPD complète. De même, la [gate K9/K10 du terminal c03](../receipts/gpu_static_terminal_k10_20260911/README.md) conserve son ancien helper sans semis et n'est pas réattribuée au nouveau.

Les [onze CTests census→tour](../receipts/census_tower_permanent_20260911/README.md) et les [cinq CTests du callback CPU](../receipts/full_ball_batch_permanent_20260911/README.md) passent chacun en O2/SAN ROOT sur leurs candidats. Les cinq fichiers de test intégrés n'en diffèrent que par une ligne vide finale ; leur nouvelle qualification active épingle ces octets séparément. Le callback permanent observe 103 terminales directes dans 49 lots par voie CPU1/4, sans instrumentation du produit. **Erratum du libellé de son paquet scellé : ses 65 cas comprennent 64 refus et un cas positif sans requête, pas 65 refus.** Les injections `wrong_admissible` sont réfutées par le propriétaire scalaire de test, pas par le validateur structurel du Builder.

La [reconstruction active Release](../receipts/full_ball_batch_active_cmake_20260911/README.md) crée douze exécutables et passe les 24 portes antérieures plus ces seize nouvelles, puis la sonde n200/statique1. Les sorties et le travail coïncident avec la référence O2 du header6763 : 48 618 MEB payées, 2 945 hits. Seuls les huit temps et deux compteurs de capacités sont exclus de cette comparaison : les nouveaux champs de `FullBallStats` augmentent `sizeof(Worker)`, donc ses capacités en octets, sans modifier le travail géométrique. Les 40 CTests ne sont ni toute la suite du dépôt ni une nouvelle qualification SAN de toutes les cibles.

Le [raccord census→batch semé→tour](../receipts/gpu_terminal_batch_t2_20260911/README.md) ferme O2/SAN sur 18 vrais census, s8/10/12 : 54 tours retenues dont 36 batch, plus 36 constructions scalaires payées pour comparer le travail. Il confronte directement 1 872 terminales chronologiques, Q=228 et H=T=120. Un diagnostic séparé prend toutes les 3 575 facettes K9/K10 des trois nuages, sans filtrage d'échecs : 36 hits à K9 et cinq à K10. Ces facettes diagnostiques ne sont pas les seules demandes chronologiques FULL, et leurs coûts ne sont pas ajoutés à celles-ci. Ces captures précèdent la correction HD et ne lui transfèrent pas leurs résultats.

## Ce qui reste avant le contrat

La [variante HD corrigée](../receipts/gpu_terminal_batch_hd_20260911/README.md) ferme sa propre qualification. Les 63 constantes ABI passent côté hôte. Le premier lien SM120 comportait des avertissements d'appels hôte/device : la copie des sources CPU actives avait perdu les annotations GPU de `q4_center_strictly_inside`, `ball_key_reduce`, `ugcd64` et `ugcd128`. Le contrôle strict `--Werror=cross-execution-space-call` réfute cet ancien état avec le diagnostic d'appel hôte ; les quatre préfixes HD rétablis, sans changement arithmétique, compilent et lient sans avertissement avec la même option. L'expansion native hôte est identique, hors blancs de bord de ligne. Les nouveaux builds O2/SAN ROOT rejouent FULL et T2, avec stdout/stderr identiques aux références : 176 terminales FULL, 1 872 terminales chronologiques T2 et 3 575 facettes haut-K séparées. Aucun résultat device n'est encore acquis.

Il faut ensuite qualifier le contexte sur carte, mesurer les phases avec les transferts froids et la fermeture, puis chronométrer toute la tour retenue. Le vrai kernel corrigé annonce **238 registres**, 1 392 octets de pile par thread et aucun spill déclaré par ptxas. Les 166 registres de l'ancien lien avec avertissements ne décrivent pas cette variante. Ces ressources de compilation ne sont pas une mesure d'occupation ou de débit GPU ; leur coût mérite une mesure sur carte.

Accélérer les seules descentes ne supprime pas le coût de génération ni celui des fusions CPU. La [réduction du graphe filtré aux naissances](GRAPHE_FILTRE_BOULES_PROPOSITION_20260911.md), validée conditionnellement par l'auditeur, vise cette seconde dépendance. Son raccord C++/GPU reste à faire, avec dates, contributions et verticales conservées. La mémoire de sortie et les régimes multi-millions doivent toujours être qualifiés séparément.

L'[autre piste MEB de l'auditeur](../audits/NOTE_CLAUDE_COEUR_MEB_20260911.md) est complémentaire au parallélisme : paire diamétrale pour q2, confinement commençant par ses extrémités, puis proposition de MEB et canonisation sur coquille. Le prototype récursif non gardé est toutefois [réfuté à K7](../audits/receipts_meb_boundary_20260911/README.md). La réparation par validation et repli exact est testée sur cette fixture, pas encore intégrée ni chronométrée sur le flux réel. Le facteur micro 2,9 annoncé pour l'ancien prototype ne qualifie donc ni cette réparation ni toute la tour.

Les contrats restent : toute la tour K1..10 à 50k sous une seconde, repli K1..5 si nécessaire, puis 100 ms. Aucun de ces contrats n'est acquis par les présents tests. GCP non utilisé pour cette étape locale.
