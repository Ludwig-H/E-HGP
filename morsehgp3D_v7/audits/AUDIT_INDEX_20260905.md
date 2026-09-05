# Audit de l'index : partition radix, identités et domaine signé

5 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

**Verdict constructif : le verrou topologique de l'index nommé dans la qualification S1 se ferme sous le domaine d'appel produit déjà gardé.** L'argument ci-dessous établit la partition parent/enfants, l'atteignabilité unique, la racine et la licéité de `clzll`. Un juge autonome exécuté sur le code courant contrôle ces propriétés directement, ainsi que l'identité des buckets et les boîtes. Aucune omission compensée par un doublon ne peut passer ce juge au seul motif que la masse totale est bonne. Aucun défaut de l'index n'a été observé dans la campagne bornée.

Ce résultat décharge la prémisse « index correct » de [S1, § 6](S1_COURANT.md#6-théorème-géométrique-conditionnel-et-rle) pour les sources et préconditions explicites ici. Il ne qualifie pas à lui seul les parcours WSPD/covers, le reste des primitives S1, le produit industriel ni ses coûts. L'index reste un arbre linéaire de positions : cette fermeture n'introduit aucune cellule ou incidence de Delaunay d'ordre supérieur.

## 1. Objet et autorité

Sources examinées : [cloud_index.hpp](../src/tree/cloud_index.hpp), [morton.hpp](../src/core/morton.hpp), [types.hpp](../src/core/types.hpp), [caps.hpp](../src/core/caps.hpp) et la garde de [run.hpp](../src/pipeline/run.hpp). La réserve initiale figure dans [QUALIFICATION_S1_PRIMITIVES, § 3](../docs/QUALIFICATION_S1_PRIMITIVES.md#3-index-parcours-tris-et-clé-primitive).

Le [reçu](receipts_20260905/index_summary.json) épingle les hashes avant/après, le HEAD `e6d33698e62ebecf74dff01c16d7de17149d7a4e`, l'état du worktree, les commandes, le compilateur et les binaires. Les sources examinées sont restées identiques pendant la campagne. Les modifications v6 déjà présentes sont consignées, sans être modifiées ni utilisées par cet audit. Le rapport et les outils vivent exclusivement dans `morsehgp3D_v7/audits/`.

Les trois autorités restent séparées : la preuve statique porte sur l'algorithme et ses domaines ; les contrôles exhaustifs d'un domaine fini portent sur ce domaine explicitement dénombré ; les autres fixtures, permutations et mutants qualifient des exécutions bornées. Ni le nombre de fixtures ni UBSan ne prouvent universellement le compilateur.

## 2. Morton et buckets : aucune identité perdue

Pour des coordonnées u16, la clé voulue est $M(x,y,z)=\sum_{b=0}^{15}(x_b2^{3b}+y_b2^{3b+1}+z_b2^{3b+2})$. Les 48 positions binaires sont disjointes ; la lecture des bits d'indices $3b+a$ restitue l'axe a. Il s'agit donc d'une bijection entre les positions du profil et les clés de 48 bits.

Le raccord aux masques écrits dans `morton_spread3` est fini et vérifiable : chaque étage de décalage, OU et masque distribue sur le OU des bits d'entrée. Le bit d'entrée b aboutit au seul bit 3b, pour chacun des 16 bits ; zéro aboutit à zéro. Le juge calcule la somme ci-dessus sans reprendre ces masques et contrôle les **65 536 valeurs de chacun des trois axes**, puis l'inverse sur tous les axes. La composition des trois sorties disjointes dans `morton48` ferme le raccord pour les positions u16, sous la sémantique entière non signée du code exécuté. Ce n'est pas une extrapolation de nuages aléatoires à tout le profil.

Le tri `(clé, PointId)` est strict total sur les enregistrements acceptés : les identités ont d'abord été rendues distinctes par rejet des doublons. Les clés égales correspondent exactement aux positions égales. La bucketisation démarre un bucket au premier enregistrement de chaque clé et ajoute le terminal n : les intervalles CSR sont donc non vides, contigus, disjoints et couvrent exactement les n identités. Leur ordre interne est croissant par PointId. `point_id(u)` restitue leur minimum, sans assimiler ce PointId au rang Morton. La somme télescopique des tailles de buckets vaut n ; chaque poids de plage est exactement la somme des multiplicités de cette plage.

L'index accepte volontairement des positions répétées avec des IDs distincts ; le pipeline les refuse ensuite avec `unsupported_degeneracy`. Ce sont deux contrats compatibles. La preuve topologique porte sur les m clés uniques ; dans le domaine S1 à positions distinctes, m=n. Une permutation physique conservant les associations `(PointId, position)` conserve les buckets et tout le reste de l'index.

## 3. Arbre radix : preuve des intervalles et références

Écrivons les clés strictement croissantes $k_0<\cdots<k_{m-1}$. Pour deux clés distinctes, $\delta(i,j)$ désigne leur longueur de préfixe commun sur 64 bits ; elle appartient à 16..63. Posons $D_i=\delta(i,i+1)$ pour les voisins valides, et $D_{-1}=D_{m-1}=-1$ aux frontières.

### Coupure unique et choix du côté

Dans tout intervalle non singleton [a,b], le premier bit où ses extrêmes diffèrent partage ses clés en une plage de bits 0 suivie d'une plage de bits 1. Il existe donc une **unique** coupure s entre ces deux plages. Le minimum des $D_a,\ldots,D_{b-1}$ est atteint uniquement en s ; c'est le préfixe des extrêmes. Les autres paires adjacentes conservent ce bit et ont un préfixe strictement plus long. En particulier, les deux préfixes adjacents à une clé ne peuvent être égaux lorsqu'ils sont tous deux valides.

À l'indice i, le code choisit donc sans ambiguïté le voisin de plus long préfixe et pose le seuil au préfixe du voisin opposé, ou à −1 au bord. Le prédicat $\delta(i,j)>d_{\min}$ décrit exactement un bloc contigu de clés partageant un préfixe avec $k_i$. Le voisin opposé est exclu par l'égalité au seuil ; le voisin choisi est inclus. La recherche dans cette direction retrouve ainsi un intervalle non singleton dont i est une extrémité.

La recherche exponentielle termine sur un voisin hors intervalle. Ensuite le prédicat est monotone le long du rayon de recherche ; la recherche binaire par puissances de deux reconstruit exactement le plus grand déplacement admissible. Elle ne saute ni trou ni seconde composante, puisque l'ensemble défini par un préfixe est contigu dans l'ordre numérique.

### Bijection entre indices internes et sous-arbres

Considérons inversement un nœud interne du trie binaire comprimé, de plage [a,b] et de préfixe q. Les préfixes aux deux frontières extérieures $D_{a-1}$ et $D_b$ sont strictement inférieurs à q. Hors racine, ils sont différents : deux traversées du même premier bit discriminant aux deux côtés contrediraient l'ordre croissant. À la racine seulement, ils valent tous deux −1.

Si $D_{a-1}>D_b$, l'algorithme situé en a prend la direction positive et le seuil $D_{a-1}$ ; son bloc maximal est exactement [a,b]. Si $D_b>D_{a-1}$, l'indice b prend la direction négative et retrouve le même intervalle. À la racine, i=0 retrouve toutes les clés. Ces règles donnent un unique indice à chaque nœud interne. Les m−1 indices exécutés construisent donc exactement les m−1 nœuds internes du trie, avec **racine 0** lorsque m≥2.

Pour la coupure s d'un nœud [a,b], son fils gauche non singleton [a,s] a sa frontière extérieure droite au préfixe du parent, plus long que sa frontière extérieure gauche : son indice est s. Le fils droit non singleton [s+1,b] a symétriquement l'indice s+1. C'est exactement l'adressage écrit dans `node.left` et `node.right`. Les fils singletons portent les références `-1-u`. Les deux plages enfants sont ainsi disjointes, strictement plus petites et leur union est celle du parent. Les références ne créent ni cycle, ni feuille dupliquée, ni nœud inaccessible.

### Recherche de la coupure et remontée des boîtes

Dans la boucle de coupure, soit S la vraie dernière clé du côté gauche. L'invariant est `split <= S` et `S - split < step`, avant réduction de `step`, avec l'initialisation `split=first`, `step=last-first`. Après le passage à `ceil(step/2)`, si le candidat reste à gauche, le déplacement retire cette moitié ; sinon la distance restante est strictement inférieure à cette moitié. L'invariant est donc préservé ; à `step=1`, il impose `split=S`. Le test `cand < last` ne rejette que des candidats déjà au-delà de S.

La pile visite chaque interne une fois et renseigne le parent de chacun de ses fils internes. L'ordre inversé de cette visite place chaque descendant avant son parent : l'union axe par axe des deux boîtes enfants est exactement la boîte serrée de la plage entière. Les feuilles fournissent les boîtes singletons ; l'induction ferme le raccord pour tous les nœuds. Le préfixe augmente strictement le long de chaque arête interne : la hauteur est au plus 48, nombre de bits utiles. Le cas m=1 a pour racine sa feuille −1 ; le cas m=0 est un index valide vide, dont `root()` ne doit pas être déréférencé.

Un détail de représentation doit être nommé : `cell_of_prefix` ne rend pas le rectangle du préfixe binaire partiel évoqué en tête du fichier. `used/3` garde les triplets complets et rend **le cube ancêtre** de ce préfixe. Le cube contient toutes les positions de la plage ; au plus deux bits de contrainte sont oubliés. La propriété de surcouverture nécessaire aux usages géométriques reste vraie. Cet audit ne déduit pas une borne de performance du front à partir du commentaire de packing.

## 4. Précondition non nulle de clz et bornes signées

Tous les appels internes de `key_delta` respectent son contrat. Dans les tests adjacents, le déplacement vaut ±1. Dans la recherche exponentielle il est non nul ; dans la recherche binaire, `l+t>=1`. L'intervalle final contient au moins le voisin choisi, donc `first != last`. Dans la coupure, `cand=split+step>first` puisque `step>=1`. Un j hors domaine retourne −1 **avant** lecture du vecteur. Sinon les indices sont distincts, les clés aussi, et leur XOR ne vaut pas zéro : aucun appel du constructeur à `__builtin_clzll` ne lui fournit zéro.

Le domaine produit est gardé avant construction par `run_pipeline_into` : $n\leq2^{30}-1$, donc $m\leq n$. Sur l'ABI CPU testée, `int` a 31 bits de valeur et `NodeRef` est i32.

| Expression | Borne / justification |
|---|---|
| Casts de n en u32 et de m en int | $n,m\leq2^{30}-1$ ; aucun rétrécissement |
| `leaf_ref(u)=-1-u` | $0\leq u\leq m-1$ ; référence comprise entre −m et −1 |
| `lmax` | Puissance de deux au plus $2^{30}$ ; à cette valeur le voisin est nécessairement hors domaine et il n'y a plus de décalage |
| `i+lmax*d` | Compris entre $-2^{30}$ et $2^{31}-3$, pour $0\leq i\leq m-2$ |
| `l+t` pendant la recherche | Au plus `lmax-1` par reconstruction binaire ; `i+(l+t)*d` tient aussi en int |
| `last-first`, `step+1`, `split+step` | Le dernier candidat est au plus $2m-2<2^{31}$, y compris avant le test de borne |
| Multiplicités et `wsum` | Au plus n ; les u32 du CSR et les u64 de cumul suffisent |
| Extraction Morton et cellule | Décalages de bits 0..47 pour l'extraction ; `shift` dans 0..16 pour la cellule ; aucun décalage de largeur excessive |

**Limite d'API précise.** `build_cloud_index` appelé directement ne contrôle pas le plafond de cardinal ; il doit recevoir explicitement la même précondition. `detail::key_delta` appelé hors constructeur exige aussi `i` valide et `j != i` si j est valide. Le résultat présent qualifie le chemin produit gardé et les appels directs satisfaisant ces contrats. Il ne transforme pas ces fonctions internes en API acceptant des tailles ou indices arbitraires. La garde produit est relue statiquement ; aucun vecteur géant n'a été fabriqué pour prétendre l'exercer dynamiquement.

## 5. Porte indépendante exécutée et mutants

La sonde [C++](index_probe_20260905.cpp) reconstruit les buckets avec `std::map` et l'interlacement bit par bit, puis reconstruit les **coupures du trie par balayage du bit discriminant**, sans `clzll`, sans recherche Karras ni masques produit. À chaque nœud, elle impose l'intervalle attendu aux fils, vérifie parents et atteignabilité unique, et recalcule les boîtes serrées par balayage de tous les points de la plage. Les identités et leur association aux positions sont vérifiées individuellement. La bibliothèque standard reste une prémisse commune, déclarée, de tri/conteneurs.

Commande reproductible depuis la racine :

```bash
python3 -O morsehgp3D_v7/audits/index_probe_20260905.py
```

Le [pilote](index_probe_20260905.py) compile avec GNU C++ 13.3.0, C++20 et `-Wall -Wextra -Wpedantic -Werror`, d'abord à `-O2`, puis à `-O1 -g -fsanitize=undefined -fno-sanitize-recover=all -D_GLIBCXX_ASSERTIONS`. Il n'utilise pas `assert` pour ses portes. Les deux exécutions normales rendent **0**, avec exactement les mêmes compteurs :

| Contrôle | Par binaire |
|---|---:|
| Valeurs Morton axe exhaustives | 196 608 |
| Nuages jugés | 237 212 |
| Nœuds internes jugés | 1 730 634 |
| Feuilles jugées | 1 967 845 |
| Permutations physiques jugées | 171 570 |
| Nuages avec buckets multiples | 6 |
| Rejets d'entrée | 8 |
| Hauteur maximale effectivement atteinte | 48 |

Les familles comprennent les **65 536 sous-ensembles d'un univers de 16 clés**, leurs inversions et mélanges déterministes, les **40 320 permutations** de huit enregistrements, les 48 bits discriminants près de zéro et du maximum de clé, deux peignes de hauteur 48, trois lignes axiales, trois nuages de 4 096 points et des positions répétées à IDs distincts. Les IDs proches du maximum u32 évitent une validation accidentelle par assimilation au rang. Les rejets portent sur chaque axe hors profil aux deux frontières et sur deux formes d'identités répétées. Des planchers exécutables imposent les cardinalités et la profondeur annoncées.

Sept **mutants de résultat**, appliqués après une construction normale préalablement validée, rendent chacun **3** sous les deux binaires : alias des fils, mauvais parent de racine, plage amputée, boîte serrée fausse, clé Morton modifiée, IDs intervertis et frontière CSR décalée. Ce sont des tests de sensibilité du juge aux fautes structurelles annoncées, pas sept modifications du code produit. Un mutant survivant rendrait 1 ; une CLI inconnue rendrait 2. La campagne comporte 19 commandes réussies au sens de leur code attendu : une identification du compilateur, deux compilations, deux normales et quatorze injections. Les stdout/stderr et codes exacts figurent dans le reçu ; aucune alerte UBSan n'a été émise.

## 6. Conséquence pour le constructeur

L'invariant d'arbre requis par S1 peut désormais être cité avec cette preuve et ce reçu, en conservant la garde de cardinal et les préconditions de l'ABI. Le [raccord des consommateurs](AUDIT_RACCORD_INDEX_FRONT_20260905.md) démontre ensuite que les piles et antichaînes préservent les ensembles de feuilles fournis par cet index. L'obligation restante porte sur le grand-livre arithmétique des témoins du front et le domaine de compilation ; elle ne rouvre pas la partition topologique démontrée. L'appel direct de l'index doit annoncer ou garder sa borne avant d'être promu comme API publique.

Aucun code produit, registre, statut public ou résultat GPU n'est modifié. **GCP non utilisé.**
