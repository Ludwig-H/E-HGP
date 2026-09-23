# Contre-audit B — noyau diamétral q3/q4 (chantier du 23 septembre)

Statut : **code publié dans `a78664d4`, correctifs `028067a3` en cours
d'intégration, performance non qualifiée, hors registre**. Lecture
d'abord du diff au-dessus de `84c74a5e`, puis des commits produit.
Cette note ne transforme ni R5 ni le harnais local en reçu du nouveau
levier. La réception v8 est auditée séparément.

## Ce qui est mathématiquement sûr

Pour une arête `ab`, `Q34EdgeCover::make_diametral` garde les sites du disque
fermé `|2z-a-b|² ≤ |b-a|²`, contre `≤ 4|b-a|²` pour le cover existant.
C'est un **sous-ensemble** du cover complet et il conserve `a,b` sur sa
frontière. Le prouveur de voie morte ne crédite que des sites *strictement*
intérieurs à **toutes** les boules de la cellule considérée. Omettre des
sites ne peut donc créer un faux crédit : si le cœur atteint `K−1` pour q3
ou `K−2` pour q4, cette voie ne peut rien émettre. Si le cœur laisse une
voie ouverte, le code reconstruit le cover complet et réexécute le prouveur
sur cette voie. Son test de profondeur ponctuelle n'est alors qu'un
**minorant** de la vraie profondeur ; il ne sert qu'à abandonner une preuve,
avec repli exact. Les comptes de masses ajoutent correctement les voies
prouvées par le cœur et les arêtes entièrement fermées.

Cette preuve suppose le contrat actuel u18/1 mm et les bornes numériques
du prouveur. Elle ne qualifie pas le float32 ni une nouvelle largeur de
coordonnées. Le type public `Q34EdgeCoverPtr` est commun au sous-cover et au
cover complet : un futur branchement du sous-cover vers le census/atlas
serait incorrect. `028067a3` ajoute un marqueur `complete()` et un refus
à l'entrée des principaux consommateurs ; voir toutefois le cas K1/2
ci-dessous. **Aucune divergence de sortie n'a été observée**
sur les petits oracles à ce stade.

## Coût : le verrou principal n'est pas touché

Le cœur s'exécute **après** l'expansion `A×B` et le filtre de paire. Il ne
réduit donc ni `expanded_pairs`, ni la borne asymptotique de cette étape.
Chaque arête survivante paie désormais un parcours supplémentaire de
l'index, le chargement des formes du cœur et une preuve ; celles dont une
voie survit paient aussi le cover et la preuve complets. Sur R5,
08/000000/K5, le chemin ancien a 23 686 751 paires développées et
2 043 612 covers, avec 2 963 451 407 incidences site–cover. Ces chiffres
fixent l'échelle du surcoût potentiel, **pas** un chrono du cœur.

Le travail interne `WspdQ34Work.core_cover` additionne visites de nœuds,
tests de boîtes et tests ponctuels, mais le `GeneratorLedger` et la sonde
v7 ne publient que `core_builds`, `core_sites`, `core_closed_edges` pour le
cover du cœur. De même, les cellules `outside/deep/failed` de
`dead_core` restent hors du ledger alors que `cells/uniform/point` y sont.
Ainsi une baisse apparente de `cover_node_visits` pourrait simplement
déplacer des visites dans `core_cover` non visible. Publier au minimum
`core_cover_node_visits`, `core_cover_bound_tests`,
`core_cover_point_tests` et les trois classes de cellules, puis le coût
CPU/mur/RSS de la chaîne complète. La durée q34 englobe bien les deux
chemins ; un ledger incomplet n'est pas une erreur de temps, mais empêche
d'expliquer et d'extrapoler ce temps.

`028067a3` raccorde ensuite ces **six** compteurs à la sonde v8
(`core_cover_node_visits/bound_tests/point_tests` et
`dead_core_outside/deep/failed_cells`) avec des identités de réception.
La lacune décrite au paragraphe précédent concerne donc la **sonde v7**,
pas le nouveau schéma. Les nouveaux comptes doivent encore être reçus
sur LiDAR entier et comparés au coût du cover évité.

Le `PROVENANCE.md` produit en cours mentionne un harnais local sur
08/000000 : 1,14 M/2,04 M arêtes fermées, CPU q34 −18 % (K5) et −16,5 %
(K10), formes « 2,96 G→0,61 G ». Le fichier du harnais est hors dépôt,
sans sortie, binaire, source et entrée hachés dans un reçu ; son condensé
omet les clés `ExactBall` et les IDs/répartition des coquilles. La formule
de formes doit publier **`dead.form_sites + dead_core.form_sites`** ON,
contre `dead.form_sites` OFF : 0,61 G semble désigner seulement les
formes du cover complet restantes. Ces chiffres sont des indications de
développement, ni un gain FULL/G4 rejugeable ni une égalité du flux entier.

Un autre auditeur a depuis publié dans [l'état courant](ETAT_COURANT.md)
deux paires FULL **locales** sur un quart spatial brut 1 mm de
08/000000, 30 263 sites/K5/s8/W8 : sortie et digest égaux, CPU de chaîne
environ 72,0→68,6–68,7 s, mais mur OFF 14,81/17,04 s contre ON
16,08/24,06 s sur hôte partagé. Le diagnostic montre une économie CPU
modeste sur ce morceau et **aucun** gain mural stable ; il ne mesure ni
une trame entière sans sol, ni le G4, ni la croissance 8k/16k/32k.

Contrairement à une alerte intermédiaire rétractée, les **deux** objets
cover ne vivent pas simultanément : le `core` est détruit à la fin du
bloc `if` avant la construction du cover complet. La mesure de pic ne
sous-compte pas leur somme pour cette raison.

## Portes et mesures à fermer avant activation par défaut

Le commit met `ChainOptions::q34_dead_core=true` par défaut et fait évoluer
la sonde v6→v7, le plan worker v4→v5. Une porte indépendante Release sur
un build séparé a passé `mhgp9_gen_wspd_q34` (18,01 s), puis les portes
`mhgp9_probe_worker_contract_{normal,optimized}` (0,76/0,89 s). Un échec
observé sur un binaire intermédiaire du développeur provenait d'un test
modifié après compilation ; **il ne doit pas être publié comme défaut du
code courant**. Les petits oracles du gate comparent supports, profondeur,
clé exacte et coquilles ; ses compteurs exigent des fermetures et des
replis du cœur. Une lecture directe d'un binaire plus récent annonce
27 674 assertions, 990 appels indexés dont 330 cœur ON, 1 060 arêtes
closes, 900/1 180 voies q3/q4 prouvées par le cœur et 40 voies ensuite
prouvées au cover complet. `core_q4_proved > core_closed_edges` garantit
qu'une preuve partielle q4 existe ; cela ne prouve pas qu'une sortie q3
aurait été perdue si la paire était close abusivement. La mutation
« clore sur n'importe quelle voie » est donc honnêtement une porte de
**ledger** tant qu'un fixture géométrique discriminant n'est pas ajouté.
Les deux nouveaux mutants compilés ont ensuite passé leur contrôle
indépendant : chacun sort code 1 sur l'identité de masse, avec le
`judgment=ledger` et le `stderr` attendu corrigés ; ce ne sont pas des
mutants tués par une mauvaise sortie géométrique. Le gel de campagne et
les mesures appariées restent ouverts.

La porte de raccord avec la **vraie** sonde compare ON/OFF, mais sa
non-vacuité vérifie encore les voies mortes du cover complet, pas
`core_closed_edges>0` ni la preuve d'une voie du cœur ; une variante qui
ne fait rien pourrait la passer. Le selftest de protocole utilise un faux
producteur synthétique : il vérifie schéma/identités mais pas ce passage
réel. Ajouter au raccord réel une non-vacuité du cœur et un mutant causal
de son effet, tout en permettant qu'un cœur très efficace laisse zéro
preuve au cover complet sur un autre fixture.

Le selftest **complet** `tower_selftest_v9.py` lancé pendant le diff
non commité a 15 erreurs sur 21 tests : le worker WIP attend le schéma
de sonde v7 tandis que `snapshot.build('HEAD', ...)` empaquette encore la
sonde v6 de `84c74a5e`, puis refuse l'incohérence. Ce n'est ni un reçu G4
ni un défaut géométrique attribuable au nouveau moteur. Après le commit
atomique `a78664d4`, le même selftest normal passe **21/21** en 44,09 s.
Le rejeu `-O` a de nouveau croisé une modification *ultérieure* des
fichiers du contrôleur/worker (nouveau schéma v8) pendant son exécution ;
ses erreurs de hash ne sont pas un verdict sur `a78664d4`. Le rejeu `-O`
sur un prochain commit figé reste requis. Les deux CTests de raccord réel
avaient, eux, passé sur le build local du diff.

L'oracle indexé exercé à cette lecture couvre le cœur à K3/5/10 avec
les filtres témoins `Pair` et `RectanglePair`. Une porte directe
d'appartenance et d'inclusion rationnelle du sous-cover a été ajoutée
ensuite à `q34_cover_gate.cpp` ; sa compilation/exécution indépendante
passe avec 18 011 assertions sur 248 arêtes, 735 membres du cœur et
573 sites du cover qui n'y sont pas, dont des nuages u18 extrêmes.
Cela ferme le contrôle local du **sous-ensemble géométrique**, pas un
résultat de performance. Le cœur n'est pas exercé
spécifiquement à K1/2 ou avec filtre témoin désactivé dans la grande
porte indexée ; ces cas restent utiles pour tuer des mutations de
contrôle de voie et de contact.

Le garde de consommateur ajouté dans `028067a3` a encore un trou de
**contrat d'API** : les deux surcharges directes
`run_q4_local_edge_candidates(Q34EdgeCoverPtr, K, ...)` de
`q4_local.cpp` rendent un travail vide à `K<3` **avant** d'appeler
`Q4LocalAtlas::make`, seul endroit qui refuse le cœur. Un cœur diamétral
passé directement à K1 ou K2 est donc accepté sans exception, alors que
la documentation annonce que *tout* consommateur hors prouveur le refuse.
Cela n'émet aucune q4 incorrecte, puisque la voie est inactive ; déplacer
`require_complete_q34_cover` avant le retour précoce dans les deux
surcharges et tester K1/2 dans la porte de cover. Les huit refus de la
porte actuelle ne portent que sur K5.

Avant de qualifier le défaut ON ou de lancer une campagne G4 coûteuse,
demander des paires
ON/OFF **même entrée, même snapshot** sur les trois trames sans sol,
K5/K10, W48, avec sorties FULL/catalogue/tour/digest identiques, travail
q34 séparé, CPU/mur/RSS et `core_builds/core_closed_edges/core_sites`.
Faire aussi les sept coupes capteur d'une même scène et 8k/16k/32k pour
juger la croissance des masses de paires, visites des deux covers et
formes ; ni une seule scène ni un digest réduit du harnais ne démontrent
le sous-quadratique. Comparer ensuite s8/s10/s12. Le harnais hors dépôt
ne hache ni la clé `ExactBall` ni les IDs complets des coquilles et ses
sources B/D/E ont changé après leurs binaires : ses anciens chronos ne
qualifient pas ce levier. Si le gain net n'est pas établi, garder le
levier **OFF par défaut** et expérimental.

Priorité architecturale inchangée : certificat par produit/ligne avant
expansion, ou autre partage qui diminue la masse `A×B` et son coût aval ;
le noyau par arête est une optimisation complémentaire, pas une preuve
de chaîne sous-quadratique ni du contrat G4.
