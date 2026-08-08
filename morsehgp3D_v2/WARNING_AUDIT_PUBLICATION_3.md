# WARNING 3 — les portes et le reçu public ne sont pas fail-closed

> [!CAUTION]
> Troisième audit indépendant du 8 août 2026, réalisé pendant que `morsehgp3D_v2` était modifié en parallèle. L'instantané relu à `20:34:40 UTC` avait pour empreinte de liste SHA-256 `359196a909cd932c9547622a0a84cc24c4b25926640b8d2268448f9cb36e3457`. Ce fichier ne modifie ni la conception ni le code de Claude. Une correction ultérieure doit rejouer les cas ci-dessous avant de rendre cet avertissement caduc.

Les deux avertissements précédents restent l'autorité pour les obstructions mathématiques, le recouvrement sphérique heuristique et le no-go de complexité à 50 k. Le présent audit porte sur un autre verrou : la révision courante contient de vraies réparations, mais ses tests et sa sortie peuvent encore annoncer un succès alors qu'une partie requise n'a pas été évaluée ou publiée.

## 1. Réparations effectivement observées

La révision courante apporte plusieurs corrections substantielles :

- un événement dont un bras de descente échoue est censuré atomiquement ;
- une forêt non autoritaire est retirée par `run` ;
- chaque nœud publié possède une sphère source permettant de relire son niveau exact ;
- les coquilles cosphériques observées sont comptées et retirent l'autorité au résultat ;
- les régressions R7 et R8 complètent désormais R1 à R6 ;
- `mhgp_oracle2` compare le catalogue avec des constructions rationnelles distinctes des primitives de production et compare des partitions de minima à des niveaux critiques exacts.

Les quatre CTests passent en Release sur l'instantané audité. Ces progrès ne ferment toutefois pas la porte, pour les raisons suivantes.

## 2. L'oracle indépendant peut passer après avoir sauté du travail

### 2.1 Son arithmétique « exacte » exécute un débordement signé

`tests/oracle2.cpp` représente ses rationnels par deux `i128`. Les fonctions `radd`, `rsub`, `rmul`, `rdiv` et `rcmp` effectuent les produits et sommes signés avant de consulter `kGuard`. Le garde intervient donc après le débordement qu'il prétend détecter.

Sous ASan/UBSan, le CTest `mhgp_oracle2` échoue dans `radd` sur un débordement signé `__int128`, par la pile `radd -> vdot -> sphere_through -> oracle_catalogue -> main`. Le même binaire passe en Release, où ce calcul a un comportement indéfini.

Le problème existe aussi à la largeur contractuelle : `node_level` et le contrôle O1 forment directement `nx * nx + ny * ny + nz * nz` dans un `i128`, alors que `sphere.hpp` emploie précisément `BigInt<4>` et documente un numérateur carré pouvant atteindre environ 181 bits. La campagne courante ne tire que des coordonnées entre 0 et 120 ; elle ne qualifie donc pas la grille 16 bits déclarée.

Ce n'est pas une borne purement théorique. Le triangle aigu entier `(17611,8271,33432)`, `(15455,64937,58915)`, `(61898,49756,27519)` produit déjà une composante de numérateur sur 80 bits ; son carré ne tient pas dans `i128`.

### 2.2 La campagne nominale n'est pas fermée

La commande par défaut demande 40 nuages, mais la sortie observée est :

```text
nuages compares : 39 ; cas O2 : 2360 ; nuages hors domaine (coquille cospherique, rejet certifie) : 0 ; forets censurees : 0
OK : oracle rationnel independant (catalogue + structure du merge tree)
```

La branche `if (overflowed) continue` abandonne silencieusement le nuage. Aucun compteur `attempted`, `skipped_overflow` ou invariant `attempted = compared + rejected` n'est vérifié. Des campagnes supplémentaires de 80 nuages ont également terminé `OK` avec des totaux incomplets. Enfin, `mhgp_oracle2 -1 1` retourne le code 0 et annonce `OK` avec zéro nuage et zéro cas O2.

Une porte exacte doit au contraire échouer dès qu'un cas planifié n'est pas décidé, sauf si ce rejet appartient à un domaine borné explicitement vérifié et comptabilisé.

### 2.3 Une forêt censurée neutralise O2 au lieu de le faire échouer

Lorsque `build_forest` renvoie `authoritative=false`, O2 incrémente `censored` puis passe à l'ordre suivant. Il n'exige ni zéro censure, ni un nombre minimal de forêts et de niveaux effectivement comparés. Une régression qui censurerait toutes les forêts pourrait ainsi supprimer toute la partie structurelle de la campagne sans rendre le test rouge.

La censure est le bon comportement du produit face à une ambiguïté. Elle n'est pas une preuve positive et ne peut compter comme succès d'une porte destinée à certifier la structure.

Le garde de domaine est lui aussi asymétrique. Si l'oracle voit une coquille et que v2 ne la déclare pas, le test échoue ; si v2 invente au contraire `degenerate_shells` ou `shell_anomalies` sur un nuage que l'oracle juge en position générale, le test entre dans la branche « hors domaine », vérifie seulement que `run` supprime les forêts, puis saute O1 et O2. Un faux positif de dégénérescence peut donc masquer tout le nuage.

### 2.4 O2 n'observe pas toute la structure qu'il annonce

L'en-tête de `oracle2.cpp` annonce la comparaison du multiensemble des niveaux et arités de fusion. Le code compare en réalité :

- la partition des minima à chaque niveau critique ;
- la monotonie des niveaux le long des parents ;
- l'absence de deux nœuds de fusion consécutifs au même niveau.

Il ne compare jamais `n_children`, les listes `first_child` / `next_sibling`, le multiensemble des arités, le nombre canonique de nœuds, les racines ou la source sémantique de chaque multifusion. Par exemple, remplacer tous les `n_children` par zéro sans changer les liens parentaux laisse les comparaisons O2 inchangées. R6 couvre un carré particulier, pas le contrat structurel général.

O1 masque aussi les doublons de support en insérant les sphères produites dans une `std::map` sans comparer `cat.spheres.size()` au nombre d'insertions réussies.

## 3. Le JSON masque précisément les états qui retirent l'autorité

`run` supprime toutes les forêts lorsque le catalogue est non certifié, lorsqu'une coquille sort du domaine déclaré ou lorsqu'un ordre est censuré. Le CLI ne sérialise pourtant aucun des champs suivants :

- `forests_suppressed` et `out_of_declared_domain` ;
- `degenerate_shells` et `shell_anomalies` ;
- `censored_orders` ;
- `Forest::authoritative`, `unresolved_arms` et `censored_events`.

Il retourne toujours le code 0. Une sortie invalide avec `"forests": []` est donc indistinguable d'une sortie vide légitime.

Le CLI ne sérialise pas non plus la forêt contractuelle : il n'émet que des compteurs par ordre, sans nœud, parent, source ni niveau exact. C'est un outil de diagnostic, pas encore un producteur de la sortie fixée par `DESIGN.md`, mais ni son nom ni le README ne rendent cette limitation explicite.

Enfin, `DESIGN.md` promet dans le reçu l'échelle, l'origine et le nombre de collisions de quantification. `quantize` calcule les deux premières valeurs, le CLI les abandonne, et aucune couche ne compte les collisions. Il est impossible de relier le JSON au nuage entier exact sur lequel portent les résultats.

## 4. Le contrat de catalogue n'est ni canonique ni déterministe

`Catalogue::members` promet que chaque tranche `I union U` est triée. `classify` insère d'abord l'ancre `pid`, puis des voisins triés par distance à cette ancre, jamais par `PointId`. O1 trie la tranche avant de la comparer et masque donc la violation publique.

Un probe exhaustif de 12 points, graine 1, a observé 61 tranches non triées sur 90. Il a aussi obtenu des payloads différents entre `threads=1` et `threads=4`. Pour le support `{0,1,3}`, une exécution conservait la base `(528,462,930)` et les membres `0,6,3,1`, l'autre la base `(424,776,563)` et les membres `3,6,1,0`.

La cause est visible dans `catalogue.cpp` : le bloc annoncé comme filtre du propriétaire canonique est vide, toutes les copies ancrées sont concaténées, puis `std::sort` compare seulement les identifiants du support et la déduplication garde un représentant équivalent non spécifié. Tous les oracles officiels forcent `threads=1`, donc aucun ne teste la voie par défaut `threads=0`.

Cette variation ne prouve pas à elle seule une différence géométrique : les représentations rationnelles peuvent désigner la même sphère. Elle réfute néanmoins le contrat de payload trié et canonique, peut changer les projections `double`, et interdit un digest reproductible tant que le propriétaire et l'ordre des membres ne sont pas imposés avant publication.

## 5. L'autorité peut être contournée par l'API publique

`build_forest` est une fonction publique qui accepte directement un `Catalogue`. Elle ne vérifie ni les drapeaux `certified`, ni les compteurs de dégénérescence, ni la provenance, l'identité du nuage, le rang maximal ou l'intégrité des offsets. Elle peut donc renvoyer `Forest::authoritative=true` à partir d'un catalogue incomplet, étranger ou fabriqué. Les gardes de `run` ne protègent pas cet appel public.

Le domaine arithmétique n'est pas davantage validé à la frontière :

- `run` accepte des `P3` arbitraires en `i64`, alors que toutes les bornes de largeur supposent des coordonnées dans la grille 16 bits ;
- `quantize` tronque silencieusement un tableau dont la taille n'est pas multiple de trois et ne refuse pas les non-finis ;
- `kMaxRank=32` est annoncé comme borne dure, mais `k_max` et `s_max` ne sont pas validés ;
- `cone_directions<0` devient une taille de vecteur gigantesque ;
- une option CLI sans valeur lit au-delà de `argv`.

Les exécutions observées confirment l'absence de fermeture : `--cones -1`, `--n -1` et une option sans valeur terminent par exception non interceptée, tandis que `--k -1 --n 8` retourne un JSON de succès incohérent. Le test `mhgp_oracle2 -1 1` constitue le cas zéro déjà mentionné.

Autre divergence d'API : `Result::forests` est documenté comme indexé par `k-1`, mais le mode `all_orders=false` ne pousse que l'ordre `K` à l'index zéro.

La borne normative `K_eff = min(K,n)` n'est pas appliquée au résultat. `run` borne seulement `s_max`, conserve `r.k_max = opt.k_max`, puis construit les ordres jusqu'au `k_max` demandé ; pour `K>n`, il peut donc publier des forêts vides supplémentaires qui ne correspondent pas à la tour effective.

## 6. Les portes P0 à P4 restent ouvertes

- **P0** : O1 est une avancée réelle, mais seulement pour de petits nuages, `K` égal à 2 ou 3, coordonnées 0 à 120, avec des cas silencieusement sautés et sans qualification des largeurs 16 bits.
- **P1** : aucune campagne dédiée ne compare la forêt d'ordre un à l'EMST exact ; R6 ne couvre qu'un carré.
- **P2** : les partitions aléatoires passent, mais les censures sont acceptées et l'arité, la généalogie complète et le payload public ne sont pas comparés.
- **P3** : le recouvrement sphérique reste échantillonné et flottant, comme l'établit `WARNING_AUDIT_IMPLEMENTATION_2.md`. L'implémentation emploie en outre `3.399186938124345` comme « golden angle » pour construire les directions, alors que la valeur indiquée par sa propre formule est voisine de `2.399963229728653`. Un probe d'un million de directions a vu le rayon de couverture estimé se dégrader d'environ `0.42145` à `0.48747` avec la constante courante ; cette mesure est diagnostique, pas une preuve de couverture.
- **P4** : aucun backend CUDA ni test 50 k n'existe, et la complexité combinatoire documentée par le second audit reste inchangée.

La CI racine ne configure, ne construit et ne teste jamais `morsehgp3D_v2`. Les résultats Release locaux ne disposent par ailleurs d'aucun reçu liant commit, compilateur, options, entrée, binaire et sortie brute.

La documentation n'est pas synchronisée avec la révision courante : `ETAT_EN_COURS.md` annonce encore O2 rouge et R1 à R6, le README omet le second warning, O2, R7 et R8, et `DESIGN.md` affirme à la fois que la position générale est assumée puis qu'elle ne l'est pas. Les mesures de `RESULTATS.md` antérieures aux corrections restent utiles comme diagnostics, pas comme artefacts qualifiants.

## 7. Conditions minimales avant promotion

1. Remplacer l'arithmétique de l'oracle par une représentation sans débordement, faire échouer tout cas non décidé et fermer les identités de campagne.
2. Exiger zéro censure inattendue dans une campagne positive et tester séparément, comme résultat négatif, que `run` supprime une sortie censurée.
3. Comparer une sérialisation canonique complète de la forêt : nœuds, niveaux rationnels, parents, enfants, arités, racines, sources et couverture attendue.
4. Rendre le reçu fail-closed : publier le domaine, la quantification, les collisions, tous les motifs de suppression et un statut non ambigu ; retourner un échec lorsqu'aucune hiérarchie autoritaire n'est produite.
5. Imposer un propriétaire canonique et des membres triés avant déduplication, puis tester l'identité byte à byte pour plusieurs nombres de threads et permutations d'entrée.
6. Lier l'autorité de `build_forest` à une capability de catalogue vérifiée, ou rendre cette primitive interne ; valider toutes les préconditions arithmétiques et options à la frontière publique.
7. Ajouter v2 à une CI stricte Release et ASan/UBSan, puis synchroniser README, état, résultats et statut réel des portes.

Jusqu'à fermeture de ces obligations, les succès Release courants sont des diagnostics de recherche utiles. Ils ne certifient ni un catalogue complet, ni une forêt publique exacte, ni un résultat reproductible, ni le contrat 50 k.
