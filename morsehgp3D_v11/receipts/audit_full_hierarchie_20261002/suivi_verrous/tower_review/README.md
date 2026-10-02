# Q1 — Contrelecture indépendante des théorèmes de la tour

**Avis favorable aux lemmes 1–4 et théorèmes B–F, avec les précisions Q1 déjà acceptées par le développeur.** Aucun contre-exemple supplémentaire à ces énoncés corrigés n'a été trouvé dans cette revue. Deux précisions de rédaction et de futur contrat de mémo sont données ci-dessous ; elles ne constituent pas des défauts d'un moteur v11 exécuté.

Instantané : `986f75799e8b8112080ce61bc06200721e69391c`, capturé à `2026-10-02T08:30:04.919611+00:00`. Les **72 fichiers** du [manifeste](../snapshot.json), y compris les fichiers non suivis, ont été vérifiés par empreinte avant et après les petits calculs. Périmètre : [L02 §4](../snapshot/private/L02_MATH_TOUR.md), spécialement lignes 99–202, et les [réponses indépendantes](../snapshot/morsehgp3D_v11/audits/AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md) / [acceptation du développeur](../snapshot/morsehgp3D_v11/audits/REPONSE_CLAUDE_VERROUS_MOTEUR_20261002.md). `docs/MATHEMATIQUES.md` n'existe pas dans cette capture : l'acceptation du correctif de formulation n'est pas encore un théorème porté dans ce document.

Cadre : `phase=exploration_v11_hors_registre`, `backend=cpu_reference`, `profile=quantized_u18_input_only`, `mode=contrelecture_mathematique_Q1`, `public_status=not_claimed`. Aucun build, aucun test produit, aucune campagne d'échelle, GCP non utilisé.

## 1. Quotient local : le chaînon à écrire dans le port

Dans les notations L02, pour `p<k`, définir `H_loc` comme le graphe strict **induit sur les k-parties de P_b**, et `J` comme le graphe des parties séparables `A⊂U` de taille `t=k−p`, reliées lorsque `A∪A′` est séparable.

La correspondance `A ↦ I∪A` induit une **bijection** entre composantes de `J` et composantes de `H_loc` :

1. Tout sommet strict `F` se compresse vers `I∪A` en remplaçant ses points de coquille excédentaires par les intérieurs manquants. Chaque union de deux sommets successifs garde une trace séparable, donc est strictement sous `λ_b` (lemme 1). Cela couvre tous les sommets de `H_loc`.
2. Si `F–G` est une arête de `H_loc`, la trace `(F∪G)∩U` est séparable. Pour **n'importe quels** `A⊂F∩U` et `A′⊂G∩U` extraits par compression, `A∪A′` est une sous-partie de cette trace ; les deux images sont reliées dans `J`. Un chemin local ne peut donc relier deux morceaux distincts.
3. Inversement, si `A∪A′` est séparable, les échanges de Johnson dans `I∪A∪A′` donnent un chemin strict local. Cela prouve la réciproque.

L'inclusion de `H_loc` dans le graphe global donne ensuite seulement une **surjection** de ces morceaux vers les composantes globales rencontrées. Des chemins extérieurs à `P_b` peuvent identifier des morceaux. L'exemple Q1 déjà accepté, `X={(0,0),(2,0),(4,0),(2,3)}`, `k=2`, `λ_b=4`, a deux composantes locales et une seule globale ; les triangles extérieurs sont de niveau `13/4`. Recalcul exact minuscule conservé, sans lecture du moteur.

**Conséquence d'implémentation :** un représentant par morceau suffit ; dédupliquer les racines globales **pré-plateau** avant de compter les enfants. La preuve complète ci-dessus remplit le passage trop bref « le reste par le lemme 4 » de L02:153, sans changer la solution retenue dans les réponses Q1.

## 2. Raffinement et exactitude relative

Le corollaire L02:160 tient pour une **partition exhaustive** de chaque morceau, avec un représentant dans chaque sous-bloc non vide. Toutes ces images restent dans `V_<`, et chaque morceau est représenté ; leur surjection globale vérifie donc H4 (L02:188). Le raffinement peut produire des répétitions d'une même racine, jamais une composante ancienne supplémentaire. L'appartenance individuelle à `V_<` ne remplace pas cette couverture.

H4 est en réalité plus faible : couvrir une fois chaque composante **globale** rencontrée suffit. La partition exhaustive des morceaux est une façon locale de le garantir sans connaître à l'avance les chemins extérieurs. Ce n'est pas une invitation à sous-échantillonner sans preuve.

Avec H1–H4, B classe les événements et C traite simultanément le plateau ; l'induction E fonctionne. Une naissance ne peut appartenir à une autre sphère au même niveau : toute boule de rayon au plus `λ_b` qui contient un sommet nouveau doit être son unique boule minimale `b`. Cela exclut les fusions instantanées naissance→parent au même niveau. La règle des verticales F est cohérente avec les coupes **fermées** et l'inclusion des faces ; ni poids ni multiplicité ne sont couverts ici.

**Précision de rédaction supplémentaire, L02:157.** Remplacer « l'admission est nécessaire et suffisante » par :

> La fenêtre candidate de la sphère rencontre les ordres 1…K si et seulement si p+q≤K+1 ; conserver toutes ces sphères suffit, sous les autres hypothèses, à reconstruire les forêts. Il ne s'agit pas d'un catalogue minimal de leurs changements de composantes.

Témoin : les quatre sites `(2,1),(1,2),(0,1),(1,0)`. La sphère de centre `(1,1)`, niveau `1`, a `p=0,q=2,m=4` et est admise à `K=1`. Elle est pourtant inerte : le cycle des côtés connecte déjà les quatre sommets au niveau `1/2`. Omettre cette sphère dans une reconstruction de `π₀` d'ordre 1 ne change aucune composante. Cela ne contredit ni la fenêtre candidate ni la suffisance de H2 ; cela évite de transformer H2 en nécessité de chaque enregistrement.

## 3. Descente : choix libre, terminal non unique, validité datée

Les trois branches D (L02:172–180) sont cohérentes. Si `p≥k`, **toute** k-partie des intérieurs tient strictement dans une boule concentrique plus petite ; la règle des k plus proches n'est pas nécessaire à la preuve. Sinon une partie séparable `I∪A` a également un niveau strictement plus bas. Dans les deux cas, `F` et `F′` sont k-parties de `P_b` et se rejoignent au niveau `β(F)=λ_b` par Johnson. Une suite termine : elle visite au plus autant de niveaux distincts que ceux des k-parties de X, donc au plus `binom(n,k)−1` pas. **Il ne faut pas remplacer ces niveaux par les seuls niveaux du catalogue admis** : les boules rencontrées pendant la descente peuvent être hors de sa fenêtre. Cette borne finie n'est pas une borne utile de performance.

Les terminaux peuvent différer ; leur classe est commune seulement aux coupes garanties par le théorème. Le témoin Q1 `{0,2,4}`, `k=2`, le montre : les deux naissances de niveau `1` restent distinctes à la coupe **ouverte** de `4`, puis fusionnent à la coupe fermée de `4`.

**Contrat précis proposé pour le mémo (complément à la réponse déjà acceptée) :** un mémo de cellule `(b,k)` porte un terminal obtenu par descente valide d'une k-partie de `P_b`, et une date de validité `λ_b`. Il représente toute k-partie de `P_b` à coupe fermée `a≥λ_b`, mais à coupe ouverte seulement si **`a>λ_b`**. Il ne remplace jamais l'ensemble des représentants de morceaux avant la jonction. Lors de la résolution d'un représentant `R∈V_<(b_parent,k)`, un raccourci de descente reste admissible si `λ_b≤β(R)<λ_parent` ; cette inégalité place bien son terminal dans la bonne classe pré-plateau. Un éventuel ancêtre publié doit ensuite être remonté à la coupe demandée.

La date n'implique pas l'unicité du terminal. Une politique déterministe fixée peut définir une fonction de résolution pure ; l'indépendance des classes ne prouve pas celle des numéros de naissance ni des traces de calcul.

## 4. Preuves déposées et portée

[check_fixtures.py](check_fixtures.py) énumère seulement les cercles englobants de trois nuages de **3 ou 4 sites**, avec `Fraction`, puis leurs petits graphes de Johnson : surjection Q1 existante, mémo Q1 existant avec coupe ouverte explicitée, carré inerte supplémentaire. Deux exécutions normal/`-O`, mêmes sorties, environ quelques centièmes de seconde au total ; commandes et codes dans [commands.json](commands.json), résultats dans [normal.stdout](normal.stdout), empreintes dans [pins.json](pins.json). Aucun appel au code produit, aucune qualification générique de cet oracle miniature.

Cette contrelecture examine des **propositions mathématiques** et leurs obligations de port. Elle ne rejoue pas les campagnes privées de L02, n'établit pas H2 pour un générateur, ne qualifie pas le traitement natif des plateaux/mémos, ne traite pas les multiplicités et ne ferme aucune borne de temps ou mémoire. Les corrections Q1 connues sont reconnues comme déjà acceptées, pas signalées comme défauts nouveaux non traités.
