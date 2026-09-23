# R8 q3/q4 : publication des tâches et prochain essai d'ordonnancement

23 septembre 2026. Lecture du [reçu G4 R8](../receipts/g4_tower_r8_20260923/README.md)
au paquet `515b3666`, de son [contre-audit](CONTRE_AUDIT_B_G4_R8_20260923.md)
et de `src/gen/pipeline/wspd_q34.cpp:431–482,835–944`. Cadre : CPU G4,
u18/grille 1 mm, trois trames sans sol de la seule séquence 08,
`complete_relative`, `not_claimed`. La réception des 20 cas et des 336
empreintes est établie dans le contre-audit ; la présente note corrige un
diagnostic de l'ordonnanceur, sans nouveau chrono G4.

## Rectification vérifiable dans le code

La phrase « seul un rectangle de plus de 256 paires est publié comme tâche »
du reçu et du contre-audit est fausse. Après le filtre du rectangle, **tout
rectangle survivant** appelle `splitter_` si le partage est actif : lorsque
`|A||B|≤256`, une plage couvrant tout A est proposée ; au-delà, A est
découpé en plages de `max(1,⌊256/|B|⌋)` rangs. La **première** plage est
également proposée. Elle est développée en ligne uniquement si la file
bornée refuse l'offre. Si `|B|>256`, une plage singleton peut encore
représenter **plus de 256 paires** : ce nombre n'est pas un plafond par tâche.

Dans `probe_0.stdout` de R8 (08/000000, K5, s8, W48), le ledger contient
3 133 819 rectangles q3/q4, dont 2 005 653 rejetés par filtre ; il reste
1 128 166 rectangles et **1 195 607** plages publiées puis consommées.
Le front et ces plus de deux millions de filtres sont exécutés dans les
jobs déjà réclamés, avant publication des plages survivantes. L'attente
mesurée de 45,3 % du temps des fils est donc compatible avec un long
travail amont ou quelques longues plages/expansions de repli ; elle ne
s'explique pas par l'absence de publication des petits rectangles. Ces
comptes ne séparent pas encore ces causes.

## Essai qui peut trancher

Le premier diagnostic utile est la distribution des durées **des jobs de
front et des plages consommées**, l'heure de fin du dernier job et de la
dernière plage, le nombre d'offres refusées, la profondeur de file et les
fils actifs pendant la queue de calcul. Trier les jobs par masse `|A||B|`
et publier leur durée maximale, comme le WIP local du 23 septembre le
prépare, est une ablation raisonnable. Ce WIP propose aussi 64 jobs par
fil au lieu de 16 : comparer les **quatre** combinaisons 16/64 × ordre
du plan/ordre par masse sur les mêmes entrées et le même paquet, sinon
un maximum par job plus petit pourrait seulement refléter le grain.
Cette masse ignore toutefois le
rejet de 2,0 M rectangles et ne prédit pas à elle seule le coût réellement
payé. La durée d'un `plan->run_job` **exclut** le traitement ultérieur de
ses plages publiées : un petit maximum de job ne disculperait donc pas
l'ordonnanceur sans un maximum de plage et les deux heures de fin.

Si les jobs longs dominent, donner à la même équipe un **produit DFS du
front encore non visité**, avant de continuer la descente, permet de
redistribuer le travail sans refaire les ancêtres. Le dispatcher existant
`src/gen/wspd/front.cpp:678–699` montre le don d'un frère intact après
publication réussie. q3/q4 a besoin d'un seul prédicat de terminaison
pour produits du front et plages de rectangles, d'une file bornée avec
repli local, et de callbacks privés par ouvrier. Le front compte chaque
produit une fois ; le rectangle terminal paie son filtre une fois ; les
plages disjointes développent chaque paire une fois. Si les plages longues
dominent, découper aussi B quand A est singleton est l'ablation suivante.
Une grande file de terminaux individuels serait coûteuse : mesurer un lot
borné avant de généraliser.

Le cache de nœuds témoins appartient à l'`Engine` de chaque ouvrier et
dépend de l'ordre des arêtes. Changer l'ordre des jobs ou des plages peut
changer ses hits, visites et recherches alors que le **multiensemble des
candidats, les masses géométriques et le digest de tour** restent égaux.
Le gate q3/q4 retire déjà ces compteurs dépendant de l'ordonnancement
avant sa comparaison (`tests/gen/wspd_q34_gate.cpp:490–532`). Le commentaire
du WIP « same counters except timings » doit être restreint à ces comptes
logiques, ou le cache réinitialisé à une frontière canonique si l'identité
de tout le ledger est voulue. Une porte doit couvrir W1/W24/W48, file
pleine, don omis/doublé, exceptions de callback/lancement et TSan ; une
nouvelle mesure G4 ne devient probante qu'avec objet identique et coût
géométrique total publié.

Enfin, la méthode WIP `WspdFrontJobs::job_pair_mass` suppose `n<2^32`,
alors que `wspd_proposals_fit` n'impose cette borne qu'avec témoins hérités.
Le front calcule déjà `n(n−1)/2` en divisant avant multiplication ; faire
de même pour la masse d'un job diagonal, ou contrôler le produit, évite un
débordement théorique sans rapport avec les tailles LiDAR actuelles.

Même avec les **77,320 CPU·s** q3/q4 de 08/000100/K5 R8 réparties
idéalement sur 48 fils, cette phase prendrait au moins **1,611 s** à
travail inchangé. q2 et la tour prennent ensemble **0,952 s** dans ce
cas ; le chemin courant resterait donc au-dessus de 2,56 s, avant ses
autres phases. C'est une borne conditionnelle sur le travail CPU mesuré,
pas sur un autre algorithme, le GPU ou une réduction des formes. Le
signal de densité à boîte fixe du [quart LiDAR chaud](lidar_density_bbox_fixed_20260923/README.md)
montre pourquoi l'ordonnancement et la réduction du travail **avant**
expansion doivent avancer ensemble.
