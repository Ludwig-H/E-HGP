# D'où viennent les continuations FULL ?

27 septembre 2026, base `fd1a2c7ee`. Audit CPU isolé, profil entier
18 bits/grille 1 mm, hors registre, `not_claimed`. Aucun moteur modifié,
aucun GCP. Les captures sont dans
[receipts/full_continuation_origin_20260927](../../receipts/full_continuation_origin_20260927/README.md).

## Résultat mathématique utile

**Un catalogue entièrement régulier ne produit aucune continuation dans
le draft FULL actuel, même si plusieurs boules ont le même niveau.**
Ici « régulier » signifie précisément `n_shell == arity` pour toutes les
boules du catalogue, avec leurs supports positifs certifiés. Ce n'est
ni « coordonnées différentes », ni « pas d'égalité de rayons », ni une
propriété supposée de tous les nuages LiDAR quantifiés.

Le critère suffisant est plus faible : **tout bloc qui apporte une
contribution doit avoir zéro représentant**. Preuve :

1. Un bloc sans représentant a une liste de racines vide.
2. Le regroupement d'un plateau ne joint deux blocs que s'ils partagent
   une racine. Un bloc à liste vide reste donc seul dans son groupe.
3. Chaque bloc contribuant produit ainsi une naissance, sans parent.
4. Tous les groupes qui ont des parents ne contiennent que des blocs sans
   contribution. Avec au moins deux parents ils produisent une fusion ;
   avec un seul parent ils restent silencieux et ne sont pas publiés.

Les sites initiaux de K1 sont eux aussi des naissances. La preuve vaut
pour `order_lot` et `close_lot`, y compris leurs branches singleton.
Sources : [full_ball_tower.hpp](../../src/tower/forest/full_ball_tower.hpp),
`count_block_at`, `visit_block_at`, `order_lot`, `close_lot`.

Pour une boule régulière, p points intérieurs et q points de support :
à K=p+q, le bloc contribue et n'a aucun représentant ; à K=p+q−1, il
a q représentants et ne contribue pas. Les autres ordres ne programment
pas ce bloc. Le critère suffisant est donc satisfait.

Le critère **exact au niveau d'un plateau produit** est l'existence d'un
groupe dont l'union des racines a cardinal un et dont la contribution est
non vide. Une coquille étendue est nécessaire pour cela, mais nullement
suffisante : le carré fournit un contre-exemple positif. Un bloc étendu
contribuant peut aussi rejoindre un groupe à plusieurs parents et produire
une fusion, pas une continuation.

## Pourquoi le repli reste indispensable

La fixture native `growth_ABCZ`, attribuée à
[full_ball_tower_gate.cpp](../../tests/tower/full_ball_tower_gate.cpp),
contient A=(1,8,0), B=(5,10,0), C=(9,8,0), Z=(5,0,0).
À K3, la composante ABC existe au niveau de rayon carré 16. Au niveau 25,
la coquille commune ajoute Z à sa couverture, **sans nouveau nœud de
topologie**. La contribution datée doit rester dans la tour explicite.

Le mutant de ce gate supprime les actions de continuation puis reconstruit
un draft structurellement admissible. Il conserve les mêmes nœuds, niveaux,
parents et successeurs, mais perd des contributions. Il est donc réfuté
par une vraie perte d'objet, pas par un crash ou un refus de forme.
Un test qui ne comparerait que la topologie laisserait passer ce défaut.

Conséquence pour l'encodeur first-parent : conserver la détection globale
des continuations et le repli exact. Une API publique reçoit des drafts,
pas une preuve de régularité du catalogue qui les a produits. Ce lemme
explique un domaine fréquent de la voie rapide ; il n'autorise pas à
supprimer la détection sur la foi d'une étiquette « LiDAR ».

## Test natif et portée

Le harnais porte explicitement la partie avant `main` du précédent
`b_full_real_drafts_20260927/probe.cpp`. Une seule unité de traduction
conserve l'accroche aux deux constructeurs natifs : générateur géométrique,
catalogue, décisions FULL, banque et verticales restent ceux du produit.
La bibliothèque géométrique est épinglée ; aucune bibliothèque de chaîne
avec une autre définition du Builder n'est liée.

Huit fixtures, deux permutations et chemins static0/static4 : paire,
triangle, tétraèdre, tétraèdre u18, carré, ABCZ, ABCZ doublé et boule inerte.
Tous les drafts sont comparés champ à champ à la forêt réellement publiée,
avec first-parent en sens direct et inverse ; le choix rapide/repli est
exigé égal à la présence effective d'une action à un parent.
Le compteur `grouped_lots` positif sur ABCZ doublé prouve l'exécution d'un
lot groupé quelque part dans cette tour, pas à lui seul l'appartenance
de la continuation observée à ce lot. La preuve des plateaux est celle
donnée ci-dessus, indépendante de ce compteur.

La capture r1 conserve un échec de compilation : renommer le `main` du
harnais lui faisait perdre son retour zéro implicite, refusé par
`-Werror=return-type`. R2 utilise un port mécanique sans ce main, avertissements
inchangés, source de r1 et builds préservés. Aucun défaut géométrique n'est
déduit de cet incident de harnais.

R2 close : huit commandes, Release et Clang ASan/UBSan/LSan ; mêmes
32 chaînes et 116 ordres, 16 chaînes régulières sans continuation,
16 à coquilles étendues dont quatre sans continuation, 16 actions de
continuation. Les deux calendriers donnent 208 comparaisons rapides et
24 par le repli. Le mutant est tué dans chaque build avec code 1 et le
même motif de perte de contribution, sans stderr. Builds désormais épinglés :
`/workspaces/E-HGP/build/v9-continuation-origin-20260927-r2`.

Ce lot ne mesure aucune performance ni croissance LiDAR, et ne qualifie
pas un nouveau constructeur FULL. Il explique le domaine du raccourci,
et éprouve son repli sur des drafts réellement produits, au lieu de se
limiter aux historiques structurels synthétiques.
