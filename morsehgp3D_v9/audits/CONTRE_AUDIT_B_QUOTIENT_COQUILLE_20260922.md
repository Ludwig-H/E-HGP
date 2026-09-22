# Contre-audit B — suffisance du quotient de grande coquille

22 septembre 2026. Verdict : **oui pour le sous-problème local des
composantes**, sous les prémisses ci-dessous ; **non** comme remplacement
autonome du catalogue ou de FULL. Aucun contre-exemple à l'isomorphisme des
composantes n'a été trouvé. La construction exacte de l'arrangement n'est
pas implémentée ni qualifiée en u18, et `O(u²)` ne désigne pour l'instant que
sa taille combinatoire. Cette note contrôle la proposition de
[`PLATEAUX_GRANDES_COQUILLES_B_20260922.md`](PLATEAUX_GRANDES_COQUILLES_B_20260922.md).

## Prémisses et argument

La BallKey exacte `A>0` définit une sphère de rayon **strictement positif** ;
son intérieur global `I` et sa coquille globale `U` sont complets, et les
`u=|U|` positions sont distinctes. Une présentation positive certifiée
garantit `c∈conv(U)` ; sans elle, il faut vérifier cette condition et refuser
une coquille qui ne définit pas sa boule minimale. Pour le rang `K`, poser
`p=|I|` et `t=K−p` ; la branche `K≤p` reste analytique, et le quotient ne
concerne que `1≤t≤u`. Les cercles et directions sont **orientés** : ne pas
identifier `n` à `−n`.

Un `t`-sous-ensemble `S` est réduit strict si et seulement si
`0∉conv({x−c:x∈S})`, donc s'il existe une direction avec tous les produits
scalaires strictement positifs. Chaque région ouverte `C` de l'arrangement
donne son ensemble positif `P_C` ; tous les `t`-sous-ensembles de `P_C`
forment une famille connexe dans le graphe des cofaces strictes. Relier deux
régions voisines si `|P_C∩P_D|≥t` ne fusionne que des familles partageant
un sommet strict. Réciproquement, toute coface stricte tient dans une
région, et le cône ouvert des directions positives d'un même `S` est
convexe : un chemin légèrement perturbé traverse des **arêtes**, sans
quitter `S`, même aux intersections multiples. Ce graphe des régions a
donc exactement les composantes de `ShellTable::rank(t)`, y compris avec
antipodes, coplanarité et coïncidences. Une région de dimension nulle ou
une égalité `n·(x−c)=0` n'est jamais un certificat strict.

Pour FULL, il suffit de garder un `t`-ensemble représentant par composante
et l'union des sites couverts, afin de calculer `contribution_shell` ; les
`reduced_members` ne sont pas consommés par `visit_block`. Le choix
canonique de la v7 est le plus petit **masque numérique** par composante,
obtenu par le minimum des `t` plus petits indices positifs de chaque
région. `h=max_C|P_C|`. Si `u≥2t−1`, la contribution globale de coquille
est vide, **mais les composantes restent distinctes** : le carré avec deux
points d'arc a `u=6`, `t=2`, couverture totale et deux composantes, dont
une paire isolée. Aucun raccourci « couverture vide ⇒ un parent » n'est
valide.

`q_min` n'est **pas** la taille d'un ensemble positif d'une région : il
doit être calculé séparément sur la coquille entière. Sous la présentation
positive, tester d'abord les antipodes (`q_min=2`), puis les plans par le
centre dont l'enveloppe 2D contient celui-ci (`q_min=3`), sinon
`q_min=4`. Le code FULL actuel demande aussi un **support témoin minimal**
pour valider la BallKey, pas seulement le nombre `q_min` ; l'algorithme de
plans doit retourner un triangle concret, ou réutiliser la présentation
positive q4 lorsqu'aucun plus petit support n'existe. Les bornes de ce
calcul et les prédicats u18 de l'arrangement sont deux preuves distinctes.
Après exclusion des antipodes, chaque paire de directions définit un plan
unique par `c`. En groupant les paires par normale primitive non orientée,
`Σ_P binomial(|P|,2)=binomial(u,2)`, donc la somme des tailles de groupes
est `O(u²)`. Dans chaque plan, un tri angulaire exact décide si tous les
gaps circulaires sont `<π` ; c'est précisément `c∈conv(P)` et permet de
choisir un triangle témoin. Cible combinatoire `O(u² log u)`, **pas encore
borne de coût/largeur qualifiée pour un code v9**.

## Mini-oracle archivé

[`check_shell_region_quotient_small_20260922.py`](check_shell_region_quotient_small_20260922.py),
SHA-256 `98faf4758f98693d361b8d0200ed1c595876f460f8455148c4fe747f3dc58db9`,
est autonome, sans import du moteur. Le côté primal énumère par fractions
exactes les supports barycentriques indépendants de taille au plus quatre,
puis les masques stricts et cofaces ; le côté dual décide les motifs de
signes réalisables par élimination entière de Fourier–Motzkin, groupe les
cercles antipodaux et relie les seules régions voisines. Les partitions
**entières** des masques stricts, couvertures, représentants minimaux et
`h` sont comparés pour tous les rangs `t=1..u` de sept petites coquilles,
soit **39 rangs, dont 26 non vides** ; `q_min` est calculé côté primal et
contrôlé sur les fixtures `2/3/4`. La paire antipodale
exerce le cas d'un seul cercle ; octaèdre, cube et carré+arc exercent
coïncidences/intersections multiples ; le tétraèdre et deux fixtures
supplémentaires exercent `q_min=4` et `q_min=3`, y compris `h=7` avec
deux composantes au rang `t=5` et `t=7`.

```sh
python3 morsehgp3D_v9/audits/check_shell_region_quotient_small_20260922.py
python3 -O morsehgp3D_v9/audits/check_shell_region_quotient_small_20260922.py
```

Les deux lectures ont rendu `status=passed` avec la même sortie. Ceci est
un contrôle exact **borné à `u≤8`**, non un gate de construction DCEL, de
prédicats u18 ou du FULL public. Il ne teste pas les centres rationnels à
grands coefficients ni le catalogue de boules du LiDAR.

## Coût et raccord industriel encore ouverts

Pour `m≤u` grands cercles distincts, le nombre de régions est au plus
`m(m−1)+2`; sommets et arêtes sont aussi `O(u²)`. Cela ne donne pas par
simple énoncé un constructeur : il faut grouper les intersections
coïncidentes, trier exactement les rayons sur chaque cercle avec un vrai
ordre sur `2π` (demi-plan, déterminant orienté, axe médian de signe
inversé), gérer digones et `m=1`, puis produire une adjacence fiable.
Matérialiser un bitset de `u` sites **par région** coûterait `O(u³)` bits ;
pour K≤10, un ensemble positif mutable le long du parcours, un DSU des
régions et au plus `t` IDs de représentant par région visent `O(u²)` mots
et environ `O(u² log u + t u² log u)` opérations combinatoires. Il faut
encore prouver les largeurs et mesurer octets, tri, DSU, `q_min`, émissions
et appels de résolution. Un seul `u=Θ(n)` peut déjà imposer un coût local
quadratique ; aucune borne sous-quadratique globale n'en découle.

Le port v9 présent garde `BallData::shell_ids[12]` et refuse explicitement
une coquille plus grande par `chain_shell_above_12` ; `ShellTable` alloue
`2^u` octets de présence puis deux tableaux `2^u` par rang non
analytique, et `local_plateau::Mask` est `u16`. Les références de
couverture FULL ont aussi un masque `u16` et la banque de populations
refuse `|U|>16`. FULL ne lit que représentant et contribution, mais doit
encore résoudre les représentants contre les **parents globaux** et
construire les verticales. Ces types et certificats doivent être élargis
ensemble, tout en conservant I/U global et toutes les BallKey utiles ;
substituer seulement le DSU local ne débloque pas les grandes coquilles.
Leurs points de code sont `src/tower/forest/ball_data.hpp:21-33`,
`src/tower/forest/local_plateau.hpp:100-215`,
`src/tower/forest/full_ball_tower.hpp:923-953`,
`src/tower/forest/full_coverage_certificate.hpp:45-87` et
`src/chain/tower_chain.cpp:329-403` du moteur v9 publié à `d2700314`
et relu dans `build/v9-open-worktree/morsehgp3D_v9/`.
