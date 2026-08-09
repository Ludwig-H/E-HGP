# Audit du parcours `order_k` dégénéré — snapshot `c1548b3`

> **Verdict : NO-GO exactitude et NO-GO 50 k.** Le passage d'un support fixe de quatre points à une coquille vectorielle est une bonne direction, mais le snapshot ne traite pas encore les dégénérescences qu'il revendique. Trois réfutations indépendantes restent actives : le plafond en rang fermé coupe les seuls ponts nécessaires à la récolte, les témoins coplanaires constants faussent le niveau du germe, et une transition perd les membres constants de la coquille. Le census terminal empêche certaines émissions fausses ; il ne répare ni la navigation, ni la récolte des supports jamais atteints.

## 1. Snapshot, portée et reproductions

Audit lecture seule du dépôt. Les probes, builds et sorties ont été créés uniquement sous `/tmp` ; aucun code produit, commit, branche, état Git ou ressource externe n'a été modifié.

| objet | SHA-256 audité |
|---|---|
| [`prototype/order_k_bfs.hpp`](../prototype/order_k_bfs.hpp) | `c1548b3ce5336a423ceb7f069ba3311749efdca057025bbde1c63333be193457` |
| [`oracle/oracle_main.cpp`](../oracle/oracle_main.cpp) | `927809a35e0356a29e81dc6ed23ee9363655a4b3e4af2d12974edb8fe3ce6078` |
| [`CMakeLists.txt`](../CMakeLists.txt) | `384b940d52b883a98f06657389bc7da8ec5474dcdef4519d7c80e3aa733e0874` |
| probe `/tmp/audit_orderk_ddb.cpp` | `48ac66e06b5b31bb784d3a12be2a5cff9bd6d322f120e861b6c459eb9f7a5049` |

Le HEAD final observé était `5a6cdb1af030a264ce07adddd312be2c458459b4`, qui committe le header `c1548b3...` avec le message `handle cosphericity instead of rejecting it`. L'empreinte de l'oracle reste distinctement scellée parce que celui-ci conserve encore l'ancien contrat de domaine.

Le correctif d'intégration immédiatement antérieur est crédité : l'ajout de `mhgp/miniball.hpp` fait maintenant compiler `mhgp3v_oracle` dans un build CMake Release frais. Il n'existe toutefois encore aucun CTest dont la commande emploie `--subject order_k`.

## 2. P0 : une coquille surnuméraire peut être le seul pont vers un support de petit rang

La récolte parcourt `order_k_vertices(points, s_max + 2, ...)`, puis coupe un sommet dès que `shell_size + level > rank_ceiling`. La justification « deux événements suffisent pour prolonger une paire jusqu'à un sommet » ne borne que le nombre d'événements dans le cas simple. Un événement multiple peut ajouter un nombre arbitraire de points à la coquille. Il n'existe donc plus de majoration additive `+2` du **rang fermé** du sommet témoin.

### Fixture minimale du cube

À `s_max=4`, prendre les huit sommets de `\left\lbrace 0,2\right\rbrace^3` :

```text
(0,0,0) (0,0,2) (0,2,0) (0,2,2)
(2,0,0) (2,0,2) (2,2,0) (2,2,2)
```

Les huit points sont sur la sphère de centre `(1,1,1)`. Le germe a donc `shell_size=8`, `level=0`, alors que le plafond de récolte vaut `s_max+2=6`. Le snapshot le coupe avant toute récolte :

```text
cube n=8 smax=4 out=0 spheres=8 visited=0 beyond=1
emitted arities = 8,0,0,0
```

Or chacune des douze arêtes du cube porte une boule diamètre de rang fermé 2 : les deux extrémités sont sur la coquille et les six autres sommets sont strictement extérieurs, vérifié indépendamment par `mhgp::sphere_side`. Le résultat ne contient que les huit singletons et manque donc au moins ces douze sphères critiques. Ce n'est ni un rejet déclaré ni un problème de choix de support canonique : c'est une omission silencieuse sur une entrée u16 exacte.

### Fixture de déconnexion sous le plafond

La même erreur n'est pas limitée au cas où le germe lui-même est coupé. La preuve de connectivité sous simplicité, ses limites multipliciaires et la voie reverse search sont détaillées séparément dans l'[audit reverse search](AUDIT_REVERSE_SEARCH_ORDER_K_CF9374.md). Pour les neuf points suivants à `s_max=2` :

```text
0=(10,14,13) 1=(10,10,15) 2=(6,10,13)
3=(6,10,7)   4=(10,13,6)  5=(12,9,3)
6=(9,9,1)    7=(18,10,2)  8=(7,11,15)
```

l'énumération exacte du 1-squelette trouve huit sommets simples de niveau 0 répartis en deux composantes sous le plafond fermé 4. Leur seul pont est la coquille de cinq points `{0,1,2,3,4}`, elle aussi de niveau 0, mais coupée parce que son rang fermé vaut 5. L'entrée satisfait `RelevantGP`. La référence compte 21 sphères ; le sujet en rend 18 et manque exactement les paires `{0,8}`, `{1,8}` et `{2,8}` :

```text
bridge-shell5 n=9 smax=2 out=0 spheres=18 visited=7 beyond=0
```

Le compteur `vertices_beyond=0` masque ici la coupure : les voisins dépassant le plafond sont abandonnés avant d'être placés dans la frontière et ne passent jamais par le site qui incrémente ce compteur.

La conclusion structurelle est importante : la connectivité démontrable concerne un sous-niveau de **niveau strict** de l'arrangement, pas le filtre `shell_size + level`. Avec des lots égaux, filtrer sur le rang fermé peut déconnecter précisément le graphe nécessaire à la récolte de petits supports.

## 3. P0 : le germe oublie les témoins coplanaires strictement intérieurs

Le nouveau code ajoute bien au germe les points coplanaires qui sont **sur** le cercle constant du pinceau. Il ignore encore ceux qui sont strictement à l'intérieur de ce cercle, alors que leur état est lui aussi constant. Le niveau du germe reste forcé à zéro.

La fixture `RelevantGP` déjà publiée dans l'[audit du premier BFS](AUDIT_ORDER_K_BFS_A8111F0.md) reste donc réfutante :

```text
0=(4,1,0) 1=(14,19,0) 2=(4,17,0) 3=(17,9,0) 4=(15,8,19)
```

À plafond 6, le seul sommet rendu avant abandon porte bien la coquille `{0,1,3,4}`, mais son niveau transporté vaut 0 alors que le census indépendant avec `mhgp::sphere_side` trouve le point 2 strictement intérieur, donc le niveau 1 :

```text
stored_shell=0134 stored_level=0 actual_shell=0134 actual_level=1
order_k_catalogue(smax=4): out=1 spheres=0 visited=1
```

Le catalogue exact contient 22 sphères. Le sujet transforme donc désormais l'ancienne omission de quatre supports en rejet total erroné. Pour initialiser l'invariant, le germe doit faire un census exact unique de **tous** les points : intérieur strict, coquille complète et extérieur. Compter seulement les égalités ne suffit pas.

## 4. P0 : une transition perd les membres constants de la coquille

Pour un triangle de pivot donné, un ancien membre de coquille coplanaire au triangle et cocirculaire avec lui reste sur **toutes** les sphères du pinceau. Le snapshot le perd :

- les deux scans de `tied` sautent d'abord tous les indices de la coquille courante ;
- le transport de niveau constate correctement un signe nul, mais ne conserve pas l'indice ;
- la nouvelle coquille est reconstruite comme `tri + tied` seulement.

La fixture u16 suivante rend le défaut directement observable :

```text
0=(3,2,2) 1=(2,3,2) 2=(1,2,2)
3=(2,1,2) 4=(2,2,3) 5=(2,2,5)
```

Les points 0, 1, 2 et 3 sont cocirculaires dans le plan `z=2`. Les points 4 et 5 donnent deux événements du pinceau. Sur 14 sommets visités, le census indépendant trouve huit coquilles stockées incomplètes, notamment :

```text
stored 1234, actual 01234, level 0
stored 0234, actual 01234, level 0
stored 1235, actual 01235, level 1
stored 0235, actual 01235, level 1
```

Le même sommet géométrique est ainsi matérialisé plusieurs fois sous des sous-coquilles différentes. Cela casse à la fois la clé `seen`, le rang utilisé pour l'élagage et la liste des sous-supports récoltés. Le census global de `try_emit` peut filtrer une émission terminale mal classée ; il ne peut pas restaurer une branche qui n'a pas été parcourue ni un support jamais récolté.

La transition doit former la nouvelle coquille comme l'union du triangle, du lot entrant **et de tous les points constants sur le cercle**, y compris ceux déjà présents dans l'ancienne coquille. Pendant la phase prototype, un census `mhgp::sphere_side` de chaque sommet produit devrait vérifier systématiquement les invariants `stored_shell == actual_shell` et `stored_level == actual_level`.

## 5. L'oracle courant ne juge pas le nouveau domaine

Le sujet ne marque plus les cosphéries comme hors domaine, mais la référence `927809...` fait encore exactement cela : dès qu'un support bien centré possède un point supplémentaire sur sa sphère, `reference_catalogue` incrémente `degenerate_shells` et rejette le nuage. Le juge et le sujet n'ont donc plus le même contrat.

Une campagne Release indépendante :

```text
mhgp3v_oracle --subject order_k --clouds 40 --seed 4242 \
  --min-points 5 --max-points 8 --max-order 3 --coord-max 20 \
  --min-decided 1 --min-nodes 1
```

sort avec le code 1 : dix essais ont `sujet=dans reference=hors`, puis l'identité de campagne n'est pas fermée, soit onze échecs. Le claim du commit `5a6cdb1` selon lequel « le juge retourne des nombres identiques » est donc réfuté par le juge intégré lui-même. Cette sortie ne réfute pas à elle seule la géométrie du sujet ; elle prouve que l'oracle ne peut actuellement ni accepter ni falsifier la nouvelle sémantique dégénérée.

Ce delta constitue aussi un changement de contrat non ouvert. La spécification §12 conserve `RelevantGP` comme domaine de la première cible exacte, classe les coquilles de plus de quatre points parmi les extensions, et exige un échec explicite sur toute dégénérescence utile non couverte. `docs/implementation_status.toml` garde la phase 13 « Dégénérescences et multiplicités » bloquée. Le motif du commit — « un rejet de domaine n'est pas une option » — ne peut donc pas, à lui seul, élargir le statut public : il faut soit fermer cette phase et la sémantique des supports non uniques, soit continuer à publier `unsupported_degeneracy` exactement sur le périmètre normatif.

Enfin, « la coquille est la clé canonique » ne suffit pas encore à sceller l'événement HGP. Le catalogue stocke aussi un seul support minimal, et `build_forest` construit ses bras en retirant successivement chacun de ses éléments. Dès que plusieurs supports minimaux portent la même miniboule, leur quotient ou le choix canonique et son invariance topologique doivent être spécifiés et vérifiés ; la déduplication par coquille ne constitue pas cette preuve.

Pour devenir une autorité indépendante, la référence doit regrouper les supports par sphère/coquille exacte, calculer la miniboule exacte de la coquille, conserver une seule sphère critique canonique et compter tous ses membres. Elle doit alors exercer au minimum les trois fixtures ci-dessus. Les commentaires et reçus doivent aussi être réalignés : `degenerate_shells` et `cocircular_pencil` sont encore décrits comme « hors domaine », le reçu ne sérialise aucune statistique `order_k`, et son champ `subject` retombe encore sur `mhgp3v anchored_catalogue`.

## 6. P1 : les arités de base restent absentes

Le cas `n < 4` retourne avant toute navigation. Les singletons sont émis avant cet appel, mais aucune paire ni aucun triangle n'est récolté :

```text
n=2, smax=2: 2 sphères au lieu de 3
n=3, smax=3: 3 sphères seulement ; paires et triangle absents
```

Cette régression est déjà détaillée dans l'[audit du catalogue `cf9374`](AUDIT_ORDER_K_CATALOGUE_CF9374.md) ; elle demeure active dans `c1548b3`.

## 7. NO-GO 50 k : le coût dégénéré n'est pas output-sensitive

Pour une coquille de taille $m$, le code énumère tous les $\binom{m}{3}$ triplets. Pour chaque triplet accepté, il effectue deux directions et deux scans globaux, soit de l'ordre de $4n\binom{m}{3}$ candidats avant même le census terminal. Les triplets dépendants paient en plus la recherche linéaire d'un apex. Plusieurs triplets cocirculaires peuvent représenter le même pinceau géométrique ; `seen` déduplique les sommets après le travail, pas les requêtes.

Le reçu interne sous-compte précisément cette charge : `pencil_candidates` n'est incrémenté que dans le premier scan qui cherche `best`. Le second scan global qui reconstruit `tied` ne l'incrémente jamais. Les statistiques peuvent donc afficher environ la moitié des examens de points des pinceaux, sans compter les recherches d'apex, les `binary_search` dans les coquilles ni les census de publication. Le ratio annoncé de 674,9 sommets par point n'est accompagné d'aucun reçu rejouable donnant temps, mémoire, multiplicité maximale ou nombre réel de classifications ; il ne démontre ni le coût 50 k ni la seconde.

Sur une grille u16 où les grandes cosphéries sont précisément annoncées comme certaines, $m$ n'est pas borné par une constante produit. Le commentaire « le coût suit la taille du catalogue » est donc faux dans le nouveau domaine : une seule coquille non publiée peut déclencher un coût cubique en sa multiplicité, multiplié par un scan de $n$ points. Le cube montre même l'autre extrême : l'élagage évite ce coût en sacrifiant l'exactitude.

Une voie v3 cohérente doit choisir et prouver l'un des contrats suivants :

1. conserver les dégénérescences, naviguer selon un plafond de niveau strict justifié, canoniser les flats de rang trois plutôt que les triplets, puis appliquer le rang fermé seulement à la publication ;
2. appliquer une perturbation symbolique exacte avec une preuve de reconstruction des coquilles et des rangs originaux ;
3. restreindre explicitement le domaine et laisser l'oracle démontrer le rejet symétrique.

Dans tous les cas, la simple combinaison « coquille vectorielle + plafond fermé `s_max+2` » est réfutée.

## 8. Porte minimale avant nouveau claim

1. rendre permanentes les fixtures cube, pont coquille-5, témoin constant intérieur et coquille constante perdue ;
2. vérifier à chaque sommet prototype la coquille et le niveau par un census indépendant ;
3. séparer `navigation_level`, `closed_rank` et `published_rank` dans les invariants et statistiques ;
4. mettre à jour la référence exacte pour les coquilles surnuméraires, puis ajouter un CTest `--subject order_k` ;
5. publier dans le reçu les compteurs `order_k`, le hash du header et un sujet exact ;
6. ne reprendre une mesure 50 k qu'après fermeture des omissions et justification du coût en multiplicité de coquille.

GCP non utilisé.
