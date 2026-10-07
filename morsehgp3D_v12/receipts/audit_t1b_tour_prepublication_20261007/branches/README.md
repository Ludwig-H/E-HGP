# Collecter les branches ouvertes sans barrière de plateau

**Proposition constructive, hors produit ; 7 octobre 2026.** Complément au reçu `tour/` publié en `f558ae6b9`, jamais réécrit. À la fermeture, `main` = `59d604b873886d757aacea17297042a524b9963e` reçoit G non commis (`tower.hpp` SHA-256 `3c22c52f…`) mais ne contient pas encore les fichiers T/M/V `forest*`. Les types et accesseurs de G sont compatibles avec `ForestInput` du prototype T/M/V épinglé dans le reçu précédent : mêmes cibles naissance/cellule, mêmes rangs et offsets, mêmes identifiants. Cette proposition vise ces corps précisément, sans qualification transférée à une fusion future.

**Recommandation : compter pendant T après chaque cellule retenue, remplir les seules cellules retenues entre T et M, puis traduire les jetons à la phase B de M.** Cela réutilise la barrière T→M déjà présente ; aucune barrière globale par plateau, aucune nouvelle descente G ni lecture géométrique. Le prix explicite est deux lectures supplémentaires des représentants des cellules retenues : comptage puis remplissage.

## 1. La requête ouverte et sa preuve

Soit une cellule c de rang r et l un **témoin de naissance immuable** de l'une de ses cibles. Pour une cible naissance, l est `birth_node[index]`. Pour une cible cellule de rang r' < r, c'est l'élément de naissance mémorisé lors de son traitement (`Entry.node`) : toutes ses composantes ont déjà fusionné à r'. Ne pas remplacer l par sa racine courante avant la requête ouverte.

```text
open_root(l, r):
  x = l
  tant que attach_parent[x] existe ET attach_rank[x] < r:
    x = attach_parent[x]
  rendre x
```

L'historique d'attache est distinct du DSU compressé du noyau. Chaque attache de rang s n'unit les composantes qu'à partir de s. Garder exactement les attaches s < r restitue donc le DSU après tous les rangs strictement antérieurs à r. À l'intérieur du rang r, les nouvelles attaches sont ignorées : le résultat est identique avant la cellule, après ses unions, après tout le plateau, et même après tous les rangs futurs. Les racines ouvertes distinctes sont exactement les branches ouvertes distinctes. L'union par taille borne chaque remontée par `floor(log2(b))`, où b est le nombre de naissances. Ne pas comprimer **l'historique** avec la racine courante ; toute mémo doit garder sa date de coupe.

Le résultat x est un identifiant de naissance qui représente la composante, **pas son nœud de forêt**. Pour obtenir ce nœud après M, la requête existante `component_at(l,r-1)` convient, car les rangs sont entiers et toute cible de T a un rang strictement inférieur à r. Il faut r > 0, déjà impliqué par une cible valide de rang non négatif. **Aucune hypothèse β(F) ≤ ℓ(r−1) n'est ajoutée** : la requête porte sur le témoin résolu de rang < r, pas sur le niveau de naissance de la partie F (précaution CST-0104).

Avant M, utiliser la même requête sans sa dernière traduction : après `open_root`, chercher dans `survivor_events[x]` le dernier événement e de rang **< r**. Rendre `event(e)` s'il existe, `birth(x)` sinon. La dichotomie est celle de `component_at`, avec la coupe stricte. Ce jeton est le sommet binaire antérieur au plateau ; après la phase B de M, `event_node[e]` donne son nœud canonique. Aucun événement de rang r ne doit entrer dans ce jeton.

## 2. Placement dans les structures actuelles

1. **Dans T**, l'union reste inchangée. Quand une cellule a produit au moins une union, relire ses représentants une fois, prendre leurs racines ouvertes, dédupliquer avec un tableau de marques indexé par naissance et compter a_c. Les cellules non retenues n'ont aucun surcoût de relecture. Garder les comptes dans l'ordre canonique des cellules retenues ; `event_cell` contient déjà leurs indices par blocs consécutifs. Ne pas incrémenter une deuxième fois `birth_targets`/`cell_targets` : ces compteurs existants décrivent le noyau original.
2. **À la fin de T**, l'historique CSR existe déjà. Le pilote calcule les préfixes u64 des a_c, contrôle les capacités et admet le stockage exact. La liste des cellules retenues se parcourt en dédupliquant les blocs de `event_cell` (au plus b−1 événements), sans balayer toutes les cellules de G.
3. **Remplissage avant M**, par ordre : relire les représentants de ces seules cellules. Reconstituer un témoin immuable : `birth_node` pour une naissance ; pour une cellule cible, son `work.cell_top`, soit une naissance, soit `events[e].surv`. Ce dernier est un témoin de la composante de la cellule cible, dont le rang est < r ; son éventuelle absorption ultérieure ne l'invalide pas. Refaire `open_root`, dédupliquer, puis chercher un jeton ouvert seulement une fois par branche distincte. Exiger que le nombre rempli égale a_c, sinon refus transactionnel. Aucun suivi de chaîne G.
4. **Après la phase B de M**, transformer les jetons en place par `birth(x) → x`, `event(e) → event_node[e]`, avant libération des événements. Trier chaque ligne par identifiant de nœud. Les racines ouvertes distinctes donnent des nœuds distincts : pas de changement du compte après traduction. Le codage à bit 31 n'est utilisé qu'avant traduction ; les identifiants finaux de nœuds peuvent utiliser ce bit et ne doivent jamais être redécodés comme des jetons.

Cela produit R au fil de T/M. Une variante de raccord plus simple peut remplir après M avec `component_at(l,r−1)` ; pour une cible cellule, `minleaf[cell_node[index]]` est alors un témoin valide. Elle reste limitée aux cellules retenues mais reporte le coût de requêtes au-delà de M : le distinguer dans le temps R, sans l'assimiler à une vue gratuite.

## 3. Capacité, mémoire et coût

Notations : R cellules retenues, P_R leurs représentants (avec répétitions), A = somme a_c des branches publiées. Les bornes sûres sont

```text
R <= événements <= b-1
2R <= A <= P_R <= représentants d'entrée
```

**A n'est pas borné par b−1.** Avec 7 naissances au même plateau et les cellules `{0,1}`, `{0,1,2}`, …, `{0,…,6}`, chacune ajoute une seule union : 6 événements, mais 27 branches à publier. Cette famille est un modèle d'hypergraphes ; sans borne d'arité elle donne un A quadratique. Elle n'affirme pas une famille géométrique réalisable. Le G courant impose en outre ses plafonds de combinaisons par cellule et de représentants par ordre ; ils doivent rester visibles, jamais remplacés par la borne incorrecte b−1 sur A.

Stockage de sortie brut proposé : `retained_ball[R]` (4R octets, ou identifiant de cellule avec propriétaire conservé explicitement), offsets u64 (8(R+1)), nœuds (4A). Chaque a_c tient en u32 sous le domaine b ≤ 2^31−1 ; la somme A, ses préfixes et les octets restent u64 avec contrôle d'addition et de multiplication **avant** allocation. Une borne générique sûre des nouveaux auxiliaires T est 4b octets de marques et 4(b−1) de comptes. Le nouveau compte doit entrer dans la formule d'admission T de tous les ordres ; le stockage R exact doit entrer dans l'admission T→M avec toutes les coexistences. Utiliser la marge/cache du `MemoryBudget` corrigé, sans recopier l'ancien corps du prototype. Une erreur ne publie ni CSR partielle ni compteurs partiels.

Les marques sont remises à zéro une fois par passe et datées par indice de cellule, pas remises à zéro pour chaque cellule. Un propriétaire par ordre suffit à ces passes ; une parallélisation ultérieure par cellules devra budgéter des marques privées ou choisir une autre déduplication.

Travail ajouté : deux parcours de P_R représentants, au plus `2 P_R floor(log2 b)` attaches examinées ; A recherches dichotomiques dans l'historique des survivants ; A traductions ; tris des lignes en `O(sum a_c log a_c)`. S'y ajoutent `O(b + événements + R)` pour marques et préfixes. C'est sensible à la sortie et peut rester coûteux si toutes les cellules sont retenues ; **aucun gain en millisecondes n'est acquis**. Évaluer P_R/P, A, les attaches réellement lues et le temps R avant adoption dans le chemin des 100 ms.

## 4. Variante locale et mémo optionnelle

Un jeton peut aussi être mémorisé immédiatement pendant T, sans dichotomie : ajouter `old_top[b]` (4b octets). Lors de la **première victoire du survivant s au rang r**, avant d'écraser `cells[s].last`, conserver son ancien `top(s)` si son dernier événement n'a pas déjà le rang r. Après la cellule, pour chaque racine ouverte x : si son dernier événement a le rang r, utiliser `old_top[x]`, sinon son `top(x)` inchangé. Même un x devenu perdant au rang r garde cette information. Le modèle vérifie cette règle aussi. Mais conserver les jetons jusqu'à M demande un stockage provisoire O(A) : arène ou blocs budgétés, puis CSR exacte (coexistence éventuellement 8A), ou préallocation sur la borne P de tous les représentants. Cette variante ne contourne pas le comptage/allocation et n'est pas le choix minimal recommandé.

Une mémo `(témoin de naissance, rang) → racine ouverte` reste valide pendant tout le rang, car les unions de ce rang n'en changent pas la réponse. Elle peut réduire les attaches lues pour des témoins répétés. La dater explicitement, budgéter ses tableaux, ne jamais vider b cases à chaque rang, et ne pas modifier `attach_parent`. Mesurer d'abord le taux de répétition ; pas de cache universel déclaré d'avance. Les nouveaux compteurs R logiques (cellules, représentants, branches) et de travail doivent être séparés des compteurs T existants ; les lectures physiques évitées par la mémo ne doivent pas maquiller une baisse des représentants logiques.

## 5. Témoin reproductible et limites

`check.py` traite cinq fixtures explicites et 64 petits hypergraphes déterministes : plateaux répétés, doublons, cellules inertes/non retenues, cibles cellule strictement antérieures, plusieurs rangs. Il compare indépendamment les partitions ouvertes gelées avant chaque plateau, les requêtes avant/après union, l'historique final, les jetons instantanés, les jetons de l'historique traduits après M, et le comptage/remplissage de la CSR. Normal et `-O` donnent le même `result.json`. Les deux hypergraphes du reçu précédent reçoivent bien des CSR différentes.

```sh
python3 -B -S morsehgp3D_v12/receipts/audit_t1b_tour_prepublication_20261007/branches/check.py
python3 -O -B -S morsehgp3D_v12/receipts/audit_t1b_tour_prepublication_20261007/branches/check.py
```

Modèle mathématique d'entrées T, pas oracle géométrique complet. Aucun code produit changé, aucune compilation native, aucun chrono ni GCP. L'intégration, les portes natives, les allocations fautives, les compteurs W1/WN et le coût réel restent à qualifier sur la livraison finale. `phase=exploration_v12_hors_registre`, `backend=cpu_reference`, `public_status=not_claimed`.
