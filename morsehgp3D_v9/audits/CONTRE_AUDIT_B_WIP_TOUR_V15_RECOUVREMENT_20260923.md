# Contre-audit B — WIP v15, recouvrement phase 0 / phase A de FULL

23 septembre 2026, snapshot **non committé** du worktree du développeur
après `fe1142b5`. Lecture statique en lecture seule de
`src/tower/forest/full_ball_tower.hpp` et
`gcp-migration/tower_worker_v9.py`. Aucun chrono, test lourd, reçu G4
v15 ni correction moteur de B. Ce WIP est **postérieur** au paquet R9
`fe1142b5` ; les mesures R9 ne le qualifient pas.

## Le recouvrement semble conservateur pour les données, pas encore prouvé

`run_orders_overlapped` prépare les cibles statiques de K décroissant,
réveille le runner de cet ordre puis commence sa phase A. `order_lots`
travaille avec `OrderState::k`, des états privés et le catalogue, les
programmes et l'index immuables ; je n'ai pas trouvé de lecture du
`Builder::current_k` partagé dans cette phase. Les réveils sont protégés
par mutex/condition et tous les runners sont joints avant fusion des
statistiques, y compris lors d'une exception. C'est une base plausible,
**pas** une preuve d'égalité FULL : il manque au snapshot une porte
recouvrement ON/OFF sur mêmes entrées avec payload/digest identiques,
une injection de pannes sur cette voie et un stress TSan qui l'active.

## Porte de durée v15 trop stricte

Dans le moteur WIP, `ready[0]=1` **avant** le lancement des K runners :
le lot K1 peut démarrer avant la première horloge `static`.
`lots_by_k[1]` part de ce démarrage. En revanche, `static_ms` additionne
les durées démarrant **après** la création de tous les runners, et
`lots_ms` ne mesure que le reste **après** la phase statique. Le lecteur
v15 impose pourtant
`lots_by_k[i] <= static_ms + lots_ms + 0,01 ms` pour chaque K.
Une exécution parfaitement valide peut donc être refusée : si K1
commence à `t=0`, la création des autres runners finit à `t=2 ms`, la
phase statique dure `10 ms` et K1 finit à `t=13 ms`, le lecteur compare
`13 ms` à `10+1+0,01 ms`. Les frais entre sous-phases statiques peuvent
aussi manquer à cette somme. Cette chronologie est permise par le code ;
elle n'est pas une mesure LiDAR. Mesurer une fenêtre commune de **juste
avant le lancement des fils jusqu'à leur jointure** et borner les
`lots_by_k` par elle ; à défaut, les borner par le mur FULL total,
moins discriminant mais sûr. Tester explicitement le cas d'un runner K1
démarré tôt. Ne pas lancer un reçu v15 tant que sa porte peut refuser une
sortie correcte de manière intermittente.

## Deux libellés qui ne sont pas encore des invariants

Le commentaire promet une priorité séquentielle « plus petit K » entre
phases. Si la phase statique échoue à K élevé et la phase A à K plus
petit, le code rapporte toujours l'échec statique, avant d'examiner les
lots. Cela reproduit l'ancienne voie statique, qui préparait toutes les
phases 0 avant les lots : **ce n'est pas une nouvelle régression**, mais
la promesse de priorité séquentielle est trop large. La porte existante
injecte des échecs de lots/images, pas ce croisement phase 0/phase A.

`overlapped_orders` augmente systématiquement de `Kmax` à l'entrée du
mode, même si les phases ne se recouvrent pas réellement et même si
la tour refuse ensuite. Le compteur atteste donc **l'admission du mode**,
pas un nombre d'ordres effectivement recouverts. Renommer/expliciter
ce compteur ou mesurer le chevauchement réel avant de l'exposer comme
preuve de gain.

Prochaine porte minimale : même catalogue et même sortie FULL pour
OFF/ON, sur plateau, coquille étendue, pannes phase 0/A/C et LiDAR 8k ;
comparaison exhaustive des actions, parents, images, populations et
digest ; horloge commune et TSan activé sur cette voie. L'ablation G4
n'a de sens qu'après fermeture de ces points locaux.

### Suite après publication de la source v15

`76436d44` publie cette source et ajoute des injections de panne
phase 0/lots sur les deux modes, avec priorité « phase 0 avant lots »
documentée ; le constat « porte de panne croisée absente » du snapshot
ci-dessus est **clos** sur ces cas. Un digest ON identique est indiqué
sur 16k/K10 local, pas une comparaison exhaustive de tout le payload
ON/OFF : `full_ball_tower_gate` compare le payload complet en voie
`overlap_static=false`, tandis que `chain_static_paths_gate` vérifie
le digest FNV64 en voie ON. Les pannes `static+images` et le lancement
des nouveaux `runners.emplace_back` ne sont pas injectés ; le failpoint
de lancement existant vise une étape antérieure de la chaîne. Aucun
écart d'objet ni race n'a été démontré par cette lecture. À W48/K10,
jusqu'à 48 fils de géométrie et 10 runners d'ordres peuvent concourir :
mesurer aussi ce coût d'oversubscription, sans le présumer bénéfique.
Les deux défauts de porte de durée restent ouverts : le faux
refus décrit ici et le [faux accord par
ordre](CONTRELEC_V15_CHRONO_ORDRE_20260923.md) démontré séparément.
Le compteur `overlapped_orders` conserve le sens « mode admis », non
chevauchement chronométré. Aucun reçu G4 v15 ni gate TSan dédiée à ce
nouveau chemin n'est encore joint à cette publication.

### Correctif d'horloge `33d51efd` : deux fermetures, faux refus K1

Le correctif publié commence une fenêtre **avant** le lancement des
runners et la termine **après** leur jointure. Il calcule `lots_ms`
comme la fenêtre moins la somme des phases 0 séquentielles. Cela ferme
le faux refus initial par temps de lancement omis ; aucune anomalie de
précision n'a été trouvée dans cette soustraction. Le lecteur ajoute
une dépendance par ordre qui tue le mutant impossible K5 de la
[contrelecture A](CONTRELEC_V15_CHRONO_ORDRE_20260923.md).

La boucle du lecteur applique toutefois cette dépendance à **K1** :
elle additionne toutes les phases 0 à `lots_by_k[0]`, alors que K1 est
prêt avant la première phase 0. Exemple réalisable et refusé : validation
1 ms, phases 0 K5→K2 de 2 ms chacune, fenêtre launch→join de 9 ms,
lot K1 de 5 ms pendant ces phases, aval de 4 ms, tour 14 ms. Le juge
exige à tort `1+8+5+4≤14`. Sur une sortie locale complète, gonfler
seulement `lots_by_k[0]` à 2,0 ms tout en restant dans sa fenêtre de
19,377 ms produit aussi ce refus. Pour K1, une borne nécessaire sûre
est `validate + max(static, lots_by_k[0]) + après ≤ tower` ; pour
K≥2, conserver la somme des phases 0 de Kmax à K avant le lot K.
Ajouter un cas **positif** K1 recouvert et garder le mutant **négatif**
K5. Le lecteur `33d51efd` n'est donc pas encore une autorité de reçu
v15, même si la fenêtre du moteur est désormais cohérente.
