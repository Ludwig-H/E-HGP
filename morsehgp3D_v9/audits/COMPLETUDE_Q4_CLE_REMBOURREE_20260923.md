# Audit — une clé q4 isolée survit à 32k sites, sans prouver le générateur entier

23 septembre 2026. Source du [test ciblé](check_q4_global_padding_20260923.cpp),
aucun changement du moteur, aucun GCP. Cadre : `reference_cpu`, u18/grille
1 mm, sonde du **flux brut q3/q4** à K3, pas de catalogue ni FULL. Le code
générateur de `origin/main` est identique à `ba9980fd` dans
`morsehgp3D_v9/src/gen/` (`git diff --quiet` rend 0). Bibliothèque locale
`libmhgp9_gen.a` SHA-256 `208aabb30a764bad25ab1c99d74885bd405e84a13dcb8375622d66aa6203f70a` ;
source de sonde SHA-256 `7798bd163548074311810b34748d21c8d51e47a763351a06c979878564b6785f`,
binaire local `740342dabdf14e0006f6d9d094a510e2acb3d86dc61721dacae35488cbb857f2`,
GCC 13.3.0, `-std=c++20 -O2 -DNDEBUG -Wall -Wextra -Wpedantic -Werror`.
Ces dépendances restent **LIVE** : ce n'est pas un reçu industriel autonome.

## Obligation mathématique visée

La [fixture rationnelle de douze sites](check_q4_without_q3_faces_20260922.py)
donne la clé primitive `(1,−40,−40,−40,900)`, centre `(20,20,20)` et
rayon carré 300. Ses quatre points de support forment un tétraèdre
strictement positif ; les huit témoins initiaux sont hors de sa boule.
À K3, sa profondeur q4 est 0, alors que chacune des quatre faces q3 a
déjà profondeur 2 et est rejetée. L'oracle `Fraction` a été rejoué ici :
`PASS`. Ce cas interdit d'utiliser « aucune face q3 acceptée » pour
éteindre la voie q4.

Le lemme de graine aiguë de la
[preuve antérieure](../../morsehgp3D_v8/audits/q34_global_contract_20260921/SUPPORT_ET_CITRON.md)
est valide. Pour l'arête maximale `ab`, son milieu `m`, le centre positif
`c` et `H(z)=(z−a)·(b−z)=|ab|²/4−|z−m|²`, les poids barycentriques
positifs `λ` donnent `ΣλH(v)=−2|c−m|²<0` et `H(a)=H(b)=0` ; au moins
une des deux autres faces de l'arête est donc aiguë. Mais la preuve
**exécutable** à fermer porte aussi sur le trajet complet : pour chaque
support q4 positif propriétaire et de profondeur admissible, son centre
doit atteindre une cellule d'atlas ni `Outside` ni `Deep`, l'événement
de complétion doit survivre aux classements de fragments et la graine
aiguë canonique doit émettre la clé. Le recensus de la chaîne ne peut
contrôler que les clés déjà émises.

## Sonde de rembourrage

Le test ajoute des sites distincts dans un cube entier `64³`, dans un
ordre de permutation déterministe, en écartant toute position à distance
carrée ≤300 du centre et les douze sites initiaux. Tous les nouveaux
points sont donc **strictement extérieurs** à la boule cible : sa clé,
sa profondeur 0 et sa coquille de quatre sites ne changent pas. Ils
peuvent en revanche changer index, rectangles WSPD, subdivisions de
l'atlas et choix d'IDs. Le mode 0 place le support avant le rembourrage ;
le mode 1 place les points ajoutés puis les témoins et inverse les IDs
du support. Les leviers géométriques q3/q4 de la chaîne R7b sont activés dans la
sonde, dont saturation profonde, census q3 sur feuille, cœur et cache.

Les **18/18** appels denses suivants ont trouvé la clé q4 avec
`depth=0`, support et coquille exactement égaux aux quatre points
attendus ; code de sortie 0. Pour chaque ligne, les trois séparations
`s=8,10,12` ont donné le même nombre d'émissions avant la clé :

| Sites | Mode IDs 0 | Mode IDs 1 |
| ---: | ---: | ---: |
| 8 000 | 6 734 | 6 586 |
| 16 000 | 10 753 | 3 284 |
| 32 000 | 19 653 | 7 506 |

Reproduction locale : compiler la source liée contre la bibliothèque
ci-dessus et lancer `<binaire> N permutation s 1` pour chaque
`N∈{8000,16000,32000}`, `permutation∈{0,1}` et `s∈{8,10,12}`.
Les compteurs sont des **callbacks avant la première clé cible** : la
sonde lève volontairement une exception locale après avoir vérifié la
clé, pour ne pas calculer les autres candidats. Ils ne sont ni le nombre
total de sorties, ni un temps de pipeline, ni une pente de complexité.
La petite fixture N12 passe aussi dans les deux ordres. Le rembourrage
en ligne éloignée du cube (`pattern=0`) a servi de smoke test, mais le
tableau ne retient que le cube spatialement entremêlé (`pattern=1`).

**Conclusion limitée :** cette clé difficile n'est pas omise dans les
18 grands index testés. Le test ne démontre ni la complétude de **toutes**
les clés d'une trame LiDAR, ni la correction de tous les événements
d'atlas, ni le sous-quadratique, ni le contrat GPU/FULL. La prochaine
porte de complétude doit cibler l'induction atlas/événement ci-dessus
et un inventaire indépendant de clés sur de petits nuages pathologiques ;
les comparaisons de projections des reçus G4 ne la remplacent pas.
