# Contre-audit B — préflight R21/v26, budget 100 ms explicite

24 septembre 2026. Bases examinées : v26 publiée `09885f163` ; plan brut
local WIP `61cfba666`. Aucun reçu G4 R21 n'était publié lors de cette
lecture. Le reçu R20 demeure le dernier résultat G4 : 18/18 tours relatifs,
mais 1,010–1,260 s à K5 sans sol sur trois trames de la seule séquence 08.
Le contrat de 100 ms porte sur la **tour FULL explicite**, nœuds, parents
et liens déjà matérialisés avant l'arrêt du chronomètre ; ni digest seul ni
sortie compacte à développer ensuite ne suffisent.

## Défaut causal du contrôle de mur externe

`bench/tower_probe.cpp` lit la trame, ouvre facultativement la session GPU,
exécute la chaîne, puis calcule les condensés. La sonde publie
`device_session.context_ms` et `reserve_ms` hors de `chain_total`, ce qui
est correct pour la latence chaude conditionnelle. Mais
`gcp-migration/tower_worker_v9.py:1087–1096`,
`validate_external_wall`, ne confronte au mur GNU que
`read + chain_total + digest + catalogue_digest`. Elle omet les deux
intervalles de session, pourtant séquentiels dans ce même processus.

Reproduction indépendante sur la version publiée : une valeur avec
`read=0,25 ms`, `chain_total=0,5 ms`, `digest=0,125 ms`,
`catalogue_digest=0`, `context_ms=1 000 000 ms` et
`reserve_ms=1 000 000 ms` est **acceptée** avec un mur externe de
`0,001875 s`. C'est un défaut d'attribution temporelle, pas une preuve
d'erreur géométrique ou de fraude des reçus R20, où ce levier n'existait
pas. Avant R21, additionner `context_ms + reserve_ms` à la borne du mur
externe, mais **pas** à `chain_total`, et ajouter une mutation causale qui
gonfle seulement ces champs tout en conservant les autres mesures et le
statut complet. La tolérance d'arrondi publiée peut rester inchangée.

## Portée réelle du plan R21 WIP

Le plan `61cfba666` passe de 18 à 30 cas en ajoutant, après les cas sans
sol, trois trames brutes avec sol (08/000000, 000100, 000200), chacune à
K5/K10, bras GPU et jumeau moteur. Les tailles, empreintes et FNV des
trois charges locales ont été recalculés. Ces cas sont utiles, mais restent
à `s=8`, séquence 08 et grille entière 1 mm ; ils ne couvrent pas `s=10/12`,
plusieurs séquences, ni le profil float32 secondaire. Les cas bruts n'ont
pas encore de condensés CPU indépendants épinglés : la comparaison interne
des jumeaux établit une identité relative, pas une référence absolue.
Avec un budget utile de 1 500 s et 600 s au plus par cas, un reçu partiel
est possible ; publier précisément les cas achevés et les reports/refus.

Le même commit WIP a encore des scénarios d'autotest codés pour les 18
anciens cas (`tower_selftest_v9.py`, p. ex. `test_budget_exhaustion_skips_following_cases`,
`test_gpu_preflight_alone_is_not_gpu_executed`,
`test_protocol_defect_stops_the_campaign`) alors que le plan en a 30.
Une exécution sur le worktree actif a donné 28 tests dont six erreurs de
scénarios ; comme ce worktree bougeait pendant l'exécution, **ce n'est pas
une qualification figée ni un échec du moteur**. La discordance des
assertions est néanmoins visible dans le source `61cfba666`. Corriger les
attendus et rejouer normal puis `-O` sur un SHA gelé avant le prochain G4.

Le bras dit « chaud » de R21 ouvre encore un **nouveau processus pour
chaque cas** puis préchauffe avant `chain_total`. Il mesure ainsi une
latence de chaîne conditionnée par la session préouverte, non le débit
d'un unique processus multi-trames persistant. C'est un diagnostic
valable s'il est nommé comme tel ; il faut un test distinct en boucle
multi-trames dans un même processus pour qualifier le flux 10 Hz.
De plus, `warm_up_lanes` peut retourner un succès avec zéro warp prévu
pour une ardoise, sans l'avoir réservée ; le champ `opened=true` prouve
donc une ouverture CUDA, pas à lui seul la résidence de toutes les
ardoises. Contrôler les tailles réellement réservées et le temps
`gpu_prepare_wait_ms` avant d'attribuer un gain à leur préallocation.

## Chemin critique FULL vers 100 ms

Sur R20/08/000000/K5, FULL vaut 421,5 ms : validation 70,5, phase
statique 198,5, lots 56,5, populations 19,6, images 26,9, banque 17,2
et encodage 32,2 ms. Supprimer gratuitement la seule phase statique
laisserait encore environ 223 ms de FULL ; supprimer le seul tri ne peut
donc pas clore 100 ms. La chaîne hors FULL vaut encore 687 ms sur cette
trame. Le code construit bien `FullBallTowerResult.orders` (forêts,
parents, contributions, nœuds inférieurs) dans `chain_total`, mais la
résolution de certaines facettes et les copies/encodages imposent une
refonte conjointe, pas seulement un meilleur digest.

Pistes à éprouver dans cet ordre, **sans crédit de gain acquis** :

1. Diminuer exactement les paires/charges du cœur q3/q4 avant émission,
   avec certificats de blocs mesurés en shadow sur rectangles lourds et
   coût total `sélection + preuve + repli + aval` ; distinguer les voies
   q3 et q4, ne créditer le cœur que si les deux sont closes.
   R20/08/000000/K5 compte environ 23,7 M paires pour 1,13 M rectangles
   encore ouverts, soit seulement ~21 paires par rectangle en moyenne :
   **64 tests de coins sur chacun seraient probablement plus coûteux que
   l'expansion**. Un premier minorant exact O(1) par boîte et un seuil de
   masse doivent précéder les coins ; qualifier leur sélectivité réelle.
2. Résoudre les cibles de facettes en lots sur tout K en conservant rangs,
   identités et plateaux canoniques ; mesurer les visites MEB et les
   sorties, pas seulement le temps de tri.
3. Représenter la tour temporelle pour partager la phase statique entre K,
   puis attribuer les premières rencontres par minimum d'ordinal et
   préfixes stables ; tester d'abord contre les vrais catalogues R20.
4. Écrire directement la sortie explicite dans des plages préfixées,
   tout en gardant cette écriture et toute conversion dans `chain_total`.

Attention à `Builder::run()` : le chemin parallèle entre ordres K est
désactivé lorsqu'un `batch_resolver` externe est branché. Un simple
callback GPU peut donc détériorer FULL ; sa parallélisation doit être
vérifiée de bout en bout, avec comparaison exacte des sorties.

GCP non utilisé dans ce contre-audit ; aucun nouveau contrat acquis.
