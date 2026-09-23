# Relecture du reçu des phases survivantes q3/q4

23 septembre 2026. Reçu examiné :
[`q34_survivor_phases_20260923`](../receipts/q34_survivor_phases_20260923/README.md),
publié par `b854088a7` avec deux patches hors produit sur `f9e6a5527`.
Les neuf fichiers du reçu vérifient leurs SHA-256 et les quatre blobs de
base des patches correspondent à ce commit. La trame est le **sans-sol**
08/000000, grille 1 mm/u18, W8, s8 ; K5 pour la consultation du préfixe,
K5/K10 pour la ventilation. Les sorties discrètes K5 des deux patches
coïncident. Aucun GCP n'a servi à ce reçu.

## Le « reste » des arêtes ouvertes inclut leur cœur

Dans `consultation.patch`, `Scope::t` reçoit `__rdtsc()` à l'entrée de
`filtered_edge`. Après la preuve du cœur, `scope.open=true` est posé sans
remettre `t` à l'instant courant. Son destructeur compte donc **toute** la
durée d'une arête ouverte, même si `build_cyc`, `load_cyc` et `prove_cyc`
ont déjà compté son cœur. Le bilan K5 est :

| Quantité du reçu | G ticks TSC |
| --- | ---: |
| `rest_cyc` publié pour les arêtes ouvertes | 354,704 |
| cœur ouvert déjà compté (build + load + prove) | 22,296 |
| reste après cœur, sans ce recouvrement | **332,408** |
| somme des intervalles distincts publiés (cœur fermé + arêtes ouvertes entières) | **415,957** |

Ces identités ne changent ni le nombre d'arêtes ni le préfixe consulté.
Elles corrigent l'attribution des ticks. Une correction locale de la sonde
remettrait `Scope::t` après `t3` avant d'activer `scope.open`.

## Les ticks ventilent du temps écoulé sous contention

`phases.patch` somme neuf intervalles `__rdtsc()` par arête. Une
calibration indépendante `__rdtsc()`/`steady_clock` de 0,6 s sur le même
codespace donne **2,44542 GHz**. Les champs `q34_batch.edges_ms` et
`q34_occupancy.cpu_sum_s` viennent des JSON du reçu ; ce dernier utilise
`CLOCK_THREAD_CPUTIME_ID` et comprend le front ainsi que les survivants,
mais pas le filtre par lots.

| K | somme des neuf postes | équivalent TSC en s-fils | 8 × mur `edges_ms` | CPU·s front + survivants |
| ---: | ---: | ---: | ---: | ---: |
| 5 | 466,988 G | 190,96 | 192,61 | 78,332 |
| 10 | 1 280,942 G | 523,81 | 526,61 | 281,209 |

La somme des postes suit presque `8 ×` la durée mur des workers, tandis
que leur temps CPU actif est bien plus faible. `rdtsc` avance aussi quand
un worker est désordonnancé ; avec l'hôte chargé déclaré par le reçu, les
parts **35/65 % à K5** et **28/72 % à K10** sont des parts de temps
écoulé cumulé, **pas** une ventilation certifiée des cycles CPU actifs.
Les neuf intervalles de `phases.patch` ne se recouvrent pas dans
l'instrumentation : cette
limite de mesure ne remet pas en cause ses comptes géométriques ni le fait
que l'atlas et les voies q3/q4 sont des postes importants.

## Portée de la décision sur les formes paresseuses

Les **156 698 009** formes de suffixe non consulté sur les arêtes fermées
représentent 45,89 % de leurs 341 485 633 formes chargées. Multiplier
45,89 % par les 12,766 G ticks de leur `core_load` donne une **projection
proportionnelle**, non un « gain maximal » démontré. Elle vaut 1,41 % du
total des intervalles distincts du patch consultation, ou 1,25 % du total du patch phases
issu d'un autre essai ; l'arrondi 1,3 % du README mêle ces deux essais.
Dans le premier découpage, même supprimer **tout** l'intervalle
`core_load` des arêtes fermées ne retrancherait directement que 3,07 %
des ticks distincts ainsi comptés, à travail aval inchangé. Ce plafond conditionnel
n'est pas une borne sur les effets de cache, d'allocation ou de scheduling.

La priorité pratique du constructeur — porter le cœur/cover réguliers,
puis traiter l'atlas et les voies q3/q4 — reste cohérente. Pour fermer une
optimisation comme non rentable, remplacer « gain maximal 1,3 % » par
« projection d'environ 1,3 % sur ce sans-sol K5 chargé » et juger une
ablation **eager/lazy** à source et entrée identiques avec CPU·s, mur,
RSS, sorties FULL et coût du cover. Notre
[shadow brut K5](lazy_prefix_dead_core_20260923/README.md) porte sur un
autre régime et mesure des formes, pas ce gain CPU. Ni le préfixe sans
sol K10 ni plusieurs séquences ni G4 ne sont qualifiés par ce reçu.
