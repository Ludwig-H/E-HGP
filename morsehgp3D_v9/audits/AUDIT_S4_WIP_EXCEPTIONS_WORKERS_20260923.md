# S4a actuel : portes hôte et réception v20

23 septembre 2026. Source du port S4a sur `main` : `5ceb4d219`, correctif
`3765080cf`, addendum `8b47a75a9`. Le [reçu G4 R15](../receipts/g4_tower_r15_20260923/README.md)
a été publié par `7ceadffad` sur ce code ; son contrôle indépendant est
résumé dans la [contrelecture R15](CONTRELECTURE_G4_R15_S4A_20260923.md).
Cette note porte sur les gardes du port et du lecteur, non sur l'exactitude
des boules réussies. Les premières fenêtres d'exception et l'ancien
sous-comptage de `both_edges` sont **corrigés** dans `3765080cf` ; ils ne
sont plus des défauts actifs.

## Ce qui est maintenant exercé

`run_lanes_batch_host` englobe allocations, initialisations de bloc et
commit ordonné dans la capture d'exception, réveille les autres workers et
les joint avant de relancer. Un lot sans arête revient avant les slabs
(`src/gpu/lanes_host.hpp:45,66–141`). La correction est structurelle ; la
porte d'injection aux trois points reste à faire.

Le ledger `both_edges` est ajouté avant la séparation entre voie q3 décidée
et voie reportée (`src/gen/pipeline/wspd_q34.cpp:1694–1705`). Le gate de
chaîne compare sept champs du ledger entre S3, S4a normal/jugé et S4a à
ardoise réduite. Exécution locale du binaire
`build/v9-exp/mhgp9_chain_batch_q3_gate` SHA-256 `a576edd6…`,
`--n=1000` : 32 cas, code 0, `asked=368886`, `tails=174420`,
`both=281530`. S3 est CPU dans cette porte et ne reporte rien ; les deux
derniers ensembles sont donc contenus dans `asked`. Par
inclusion–exclusion, **au moins 87 064 occurrences d'arêtes-cas** ont à
la fois q3 reportée et q4 ouverte. Le chemin corrigé est effectivement
exercé et son ledger comparé. Encoder un plancher direct sur cette
intersection préserverait la porte si la fixture change ; comparer aussi
`q4_emitted` du bras réduit, actuellement omis, fermerait sa masse de
sorties.

La réception Python v20 a aussi la borne juste
`lanes_asked ≤ q3_edges ≤ lanes_asked + deferred`
(`gcp-migration/tower_worker_v9.py:623–627`) et des mutants de subset.
L'objection faite sur un snapshot mutable avant `3765080cf` est close.

## Panne mémoire : remplacer la réservation géante

`tests/gpu/lanes_port_gate.cpp:570–585` tente une panne en fixant
`record_capacity=0xffffffff`, soit environ **512 Gio de records par
worker**. Sur un hôte avec overcommit, la réservation virtuelle peut
réussir puis l'initialisation tuer le processus sans `bad_alloc` contrôlé.
Je n'ai pas lancé cette porte. Elle ne cible ni l'`assign` après un premier
bloc ni la croissance de `out.records.insert` au commit ordonné. Une
injection d'allocation déterministe et bornée à ces trois points, avec
plusieurs blocs et workers, devrait vérifier jointure, réveil, erreur
typée et absence de résultat partiel. Le succès du chemin normal et du
reçu R15 ne teste pas ces exceptions.

## Faux refus possible dans le lecteur S4a

`validate_lanes` refuse toute arête q3 reportée sous la capacité par
défaut si `case.n < 65536` (`tower_worker_v9.py:636–641`). Ce seuil borne
le **cover en sites par arête**, pas l'ardoise de **4096 records par
arête** ni l'arène commune. Un appel exact de l'API S4a peut dépasser
cette dernière avec moins de 65 536 sites : choisir `E=4097` grappes
espacées d'au moins 500 unités dans la grille u18 ; par grappe,
`a=(-10,0,0)`, `b=(10,0,0)` et cinq points `(0,u,v)` où
`(u,v)=(13,0),(0,13),(-13,0),(0,-13),(5,12)`, puis translater chaque
grappe. Ces sept points ont `|ab|²=400` et `|ax|²=|bx|²=269` ; chaque
triangle `abx` est strictement aigu. Le centre transverse est `69x/338` ;
la puissance d'un autre point `y` du cercle de rayon 13 vaut
`69(1−x·y/169)>0`. Les autres grappes sont hors de ces petites boules.
Les cinq supports q3 par arête sont donc admissibles à K5 si ces arêtes
sont soumises à la voie ; chacune reste sous son plafond de records.
Pour `7E=28 679` sites, elles produisent `5E=20 485` records, un de plus
que l'arène par défaut `4E+4096=20 484` (`filter_runner.hpp:225–229`).
Le débordement reporte légitimement une arête vers le CPU. Cette
construction ne prétend pas que la WSPD sélectionne précisément ces
arêtes : elle réfute la règle **générale de validation du batch**.

Contrelecture dynamique du lecteur avec `tower_selftest_v9.probe_value`
au code `8b47a75a9`, Python `-B` : le rapport normal à 39 885 sites
passe ; le même rapport avec une voie reportée et une ardoise explicite
réduite passe ; avec cette répartition identique et la capacité par
défaut, le lecteur répond `q3 lanes deferred edges of a frame below the
slab`. Cela démontre le faux invariant de réception, sans mesurer sa
fréquence sur G4. Le mutant distinct où
`lanes_asked+deferred<q3_edges` est, lui, bien refusé. La correction est
d'autoriser les reports par records/arène même sous 65 536 sites, puis
de juger la traîne et les digests. Une porte directe de la construction
ci-dessus, ou une fixture plus petite à arène explicitement bornée,
protégerait le changement.

**Même porte, fixture beaucoup plus compacte à essayer.** Poser
`r=1105=5·13·17`, `L=1000`, `a=(−L,0,0)`, `b=(L,0,0)` et prendre **tous
les 108 points entiers** `(0,u,v)` tels que `u²+v²=r²` ; le compte exact
est `4·3³=108` (confirmé par énumération). Espacer 40 copies de cette
grappe de 5 000 unités le long de `x`, avec centres
`(1000+5000c,1105,1105)` pour `c=0,…,39`, entièrement dans le domaine
u18. Chaque triangle `abx` est aigu et `ab` est sa plus longue
arête : `|ax|²=|bx|²=2 221 025<|ab|²=4 000 000` et
`2|ax|²>|ab|²`. Son centre transverse vaut
`t x`, `t=(r²−L²)/(2r²)>0` ; tout autre point `y` du même cercle a
une puissance `(r²−L²)(1−x·y/r²)>0`. Les autres grappes sont hors de la
boule. Chaque arête fournie peut donc émettre 108 records q3 distincts à
K5, bien sous le plafond de 4 096 records par arête. Avec **4 400 sites
et 40 arêtes**, l'arène par défaut vaut `4·40+4096=4256` records,
contre `40·108=4320` requis ; les 39 premières arêtes prennent 4 212
places et la dernière est reportée. Cette proposition mathématique
demande encore une exécution de porte hôte et la comparaison au moteur ;
elle ne prétend pas que le front WSPD choisit ces arêtes. Elle réduirait
fortement le coût du gate direct de 4 097 grappes actuellement ajouté
au WIP développeur.

## Préflight jumeau et mesures suivantes

`validate_preflight_work` dispense à juste titre le census q3 de feuille
et le cache de témoins du bras batch/GPU quand S4a les remplace. Le
jumeau `preflight_engine` est exécuté et comparé en objet et travail de
certificats (`tower_worker_v9.py:1156–1170`), mais ces deux planchers de
travail ne lui sont pas appliqués. Dans R15, il a effectivement
**82 056 census q3 de feuille** et **321 903 rejets par cache** ; le
reçu est donc positif sur ce point. Une garde explicite et deux mutants
qui annulent ces compteurs maintiendraient la promesse de préflight
« chaque levier actif a travaillé » si la fixture change.

Le script local S4a `receipts/s4a_q3_lanes_local_20260923/run.sh`
a été lancé partiellement à cette lecture, mais son dossier n'a pas de
reçu clos. Il note `HEAD` et les SHA des sorties sans lier la propreté
source, les octets d'entrée et du binaire, ni comparer automatiquement
les statuts, digests et ledgers de ses JSON. Cette capture CPU distincte
ne remplace pas R15. Pour la question de croissance, rejouer S4a sur les
[demi-scènes, quarts physiques et densités emboîtées](CROISSANCE_LIDAR_PLANS_ET_DENSITE_20260923.md),
en gardant la trame entière comme contrat ; les statistiques `--file`
seules ne prouvent pas l'exactitude LiDAR.
