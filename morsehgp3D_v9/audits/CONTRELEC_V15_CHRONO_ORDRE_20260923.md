# WIP v15 : un chrono impossible par ordre reste accepté

23 septembre 2026, lecture du worktree **non committé** après
`fe1142b5` et du binaire local `build/v9-exp`. Complément au
[contre-audit B](CONTRE_AUDIT_B_WIP_TOUR_V15_RECOUVREMENT_20260923.md) :
celui-ci trouve un faux refus possible lorsque le lot K1 démarre avant
l'horloge statique. La porte actuelle a aussi un **faux accord**, sur
une mutation d'une sortie v15 réelle. Aucun écart de tour FULL ni reçu
G4 v15 n'est allégué.

Commande locale en lecture seule :
`build/v9-exp/mhgp9_tower_probe build/v9-exp/probe_worker_contract_normal/probe_worker_contract.u32le 5 2 --s=8 --static=2 --grid=1mm`.
La sortie complète est acceptée par `validate_tower_phases`. En
remplaçant **seulement** `tower_phases_ms.lots_by_k[4]` par
`tower_phases_ms.static + tower_phases_ms.lots`, le lecteur l'accepte
encore. Lors de notre rejeu, le mur FULL était **60,718 ms** ; la
somme des phases obligatoirement disjointes pour K5 était pourtant
**65,379 ms**, même **sans** validation :

`static_by_k[4] + lots_by_k[4] + populations + images + bank + encode`.

Dans le moteur, la phase A de K5 ne commence qu'après **sa** phase 0,
et populations/images/banque/encodage attendent la fin de tous les lots.
Cette somme doit donc être au plus le mur FULL, à la tolérance d'arrondi
près. La porte v15 ne vérifie que la borne individuelle
`lots_by_k[i]≤static+lots` et la somme des phases agrégées ; elle ne
relie pas ces phases à la dépendance d'un ordre. Ajouter l'inégalité
ci-dessus pour chaque K≥2 (et son analogue sans phase 0 pour K1),
avec un mutant qui gonfle un seul `lots_by_k`. Ce juge est compatible
avec un recouvrement réel entre **ordres distincts**. La correction du
faux refus de B (horloge commune du lancement au `join`) reste
nécessaire : relâcher uniquement la borne individuelle ne ferme pas
ce faux accord.

`static_by_k[0]` désigne K1 dans le JSON, alors que le moteur ne prépare
aucune phase 0 pour K1. Exiger zéro pour cette case empêche un transfert
artificiel du temps statique entre ordres qui conserve `sum(static_by_k)`.
Le test négatif v15 actuel ne modifie que cette première case ; il
n'exerce pas la dépendance temporelle K5 ci-dessus.

## Contrelecture du correctif publié `33d51efd`

Le correctif, publié après cette première note, ajoute bien une fenêtre
commune lancement→jointure et le juge par ordre K≥2. Le mutant K5
initial est maintenant refusé. Il applique toutefois à **K1** la somme de **toutes**
les phases 0 avant `lots_by_k[0]`. C'est faux : le runner K1 est prêt
avant même le lancement des autres runners et peut recouvrir ces phases.
Sur une sortie locale complète, `tower=65,993 ms`, `validate=10,066`,
`static=42,313`, `lots=1,677`, et les phases après lots totalisent
`11,893`. En ne portant que `lots_by_k[0]` à `42,313 ms`, la fenêtre
commune est respectée et le chemin nécessaire
`validate + max(static, lots_by_k[0]) + après = 64,272 ms` tient dans
le mur, mais le lecteur publié **refuse** la sortie. Pour K1, remplacer
la somme fautive par ce `max` (avec tolérance d'arrondi) ; pour K≥2,
conserver la somme des phases 0 de Kmax à K, qui précèdent réellement
le lot K. Ce cas doit accompagner le mutant K5 afin de fermer les deux
sens de la porte.

Une seconde contre-épreuve sur le **port publié**, avec la validation
complète d'une sortie réelle de 360 sites, confirme le même écart : en
ne remplaçant que `lots_by_k[0]` par `static=19,065 ms`, le chemin
physiquement nécessaire vaut `validate+max(static, lot K1)+aval=33,514 ms`,
inférieur au mur FULL de **35,219 ms**. Le lecteur la refuse pourtant
avec `tower phase A of order 1 before its phase 0`. La nouvelle porte
qui exige `static_by_k[0]=0` est utile, mais son test actuel modifie cette
case sans préserver `static=sum(static_by_k)` : il échoue aussi à une
ancienne garde. Transférer une durée de K2 à K1, somme inchangée,
isolerait ce nouveau contrôle. Aucun reçu G4 v15 ne découle de ces
contre-épreuves locales.
