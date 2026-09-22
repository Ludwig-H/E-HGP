# Mini-reçu B — q4 positive à deux intérieurs, seuil K5

22 septembre 2026. Ce reçu est un **test borné**, pas une preuve de complétude globale. Le code du mini-gate est [`check_q4_k5_interior_chain_20260922.cpp`](check_q4_k5_interior_chain_20260922.cpp). Aucun fichier du moteur n'a été modifié.

## Oracle géométrique indépendant

Nuage, dans cet ordre puis dans l'ordre inverse : `(3,3,3), (3,1,1), (1,3,1), (1,1,3), (2,2,2), (2,2,3)`. Les quatre premiers sommets portent la sphère de centre `(2,2,2)` et rayon carré `3` ; leurs poids barycentriques sont tous `1/4`. Les deux derniers sites sont strictement intérieurs, aucun autre site n'est sur la coquille. La forme primitive de puissance est `|z|²−4(z_x+z_y+z_z)+9`, donc `BallKey=(1,−4,−4,−4,9)`. Son support minimal a arité 4 et son rang d'entrée dans le catalogue est `|I|+q_min=6` : la boule est admissible à K5 (`K+1=6`), mais non à K3/K4.

L'**absence du catalogue** à K3/K4 suit de ce rang ; elle ne démontre pas en général qu'une présentation n'a pas été fabriquée en amont. Dans ces six exécutions particulières, le compteur `q4_emitted` vaut aussi zéro à K3/K4 et un à K5. Le gate exige à K5 exactement une ligne de catalogue de cette clé avec `q_min=4`, `|I|=2`, `|U|=4`.

## Reproduction et résultat

Depuis la racine du dépôt, en réutilisant les bibliothèques Release déjà construites :

```sh
c++ -std=c++20 -O2 -pthread \
  -I build/v9-open-worktree/morsehgp3D_v9/src \
  -I build/v9-open-worktree/morsehgp3D_v9/src/gen \
  morsehgp3D_v9/audits/check_q4_k5_interior_chain_20260922.cpp \
  build/v9-open-worktree/build/v9/libmhgp9_chain.a \
  build/v9-open-worktree/build/v9/libmhgp9_gen.a \
  -o /tmp/check_q4_k5_interior_chain_20260922
/tmp/check_q4_k5_interior_chain_20260922
```

Même gate sous Clang 18 ASan/UBSan, avec les bibliothèques instrumentées préexistantes :

```sh
clang++ -std=c++20 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -pthread \
  -I build/v9-open-worktree/morsehgp3D_v9/src \
  -I build/v9-open-worktree/morsehgp3D_v9/src/gen \
  morsehgp3D_v9/audits/check_q4_k5_interior_chain_20260922.cpp \
  /tmp/mhgp9-public-t2-VJ2IsN/asan/libmhgp9_chain.a \
  /tmp/mhgp9-public-t2-VJ2IsN/asan/libmhgp9_gen.a \
  -o /tmp/check_q4_k5_interior_chain_20260922_asan
/tmp/check_q4_k5_interior_chain_20260922_asan
```

Lecture du 22 septembre : six appels à `run_tower_chain`, `run_tower=true`, `keep_catalogue=true`, `s=8`, un worker ; statut `complete`, `orders.size()==tower.orders.size()==K`, indices d'ordres corrects et digest non nul à chaque appel. Les inventaires ont 16/20/21 boules pour K3/K4/K5 dans les deux permutations. La ligne cible est absente/absente/présente une fois ; `q4_emitted=0/0/1`. Les six cas passent en Release et ASan/UBSan avec la même sortie et aucune alerte. Les digests diffèrent naturellement lorsque les IDs sont renversés : seul l'inventaire géométrique de la clé et les cardinalités sont comparés ici.

Empreintes SHA-256 des bibliothèques Release : `7107859fd23f7cf575ae7da4b814c983b0905007bb529a8de90dac799236ab02` (chain), `04817e9f8ad5086b19bef69abde1460890d16575f7250af6c8a4daa57fe74ef7` (gen). Bibliothèques instrumentées : `bfa04bb7f4f407c524dfbeb7ea69319d991499bdd6416507e4290ee9640dd32c` (chain), `c2b8426c89f3143cd92925e078588ab5101fea4e80fb62b3858488835476a3ce` (gen). La source du worktree de build est à `ba762036` ; `src/gen` n'a pas de diff par rapport à `d2700314`, mais quatre fichiers `src/tower` diffèrent. Il s'agit donc d'une sonde comportementale de la voie générateur d270 raccordée à cet aval, non d'un reçu binaire figé du commit d270 exact.

Le gate ne compare pas l'inventaire complet à un oracle externe : il cible une unique boule q4, le franchissement du seuil et la construction matérielle des ordres FULL. Il ne qualifie ni toutes les dégénérescences, ni les coquilles `>12`, ni la complétude globale WSPD/q3/q4, ni LiDAR/G4.

Pour lever une ambiguïté du gate T2 existant : [`chain_census_tower_gate.cpp`](../tests/chain/chain_census_tower_gate.cpp) nomme `--shell14` un nuage de **14 sites** dont la grande coquille n'en a que **12** (`:238-249`) ; il n'exerce donc pas le refus du cap `>12`. Ce gate appelle d'abord la chaîne avec `run_tower=false` pour obtenir le catalogue (`:89-111`), puis construit explicitement et vérifie la tour (`:167-189`).
