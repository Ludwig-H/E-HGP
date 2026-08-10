# Solution mathématique — transcript Gamma par support minimal

Date : 10 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`, `backend=CPU de référence +
oracles bornés`, `profile=quantized_u16_input_only`, `mode=audit_indépendant`.
Cette note ne modifie aucun prototype. Elle résout le verrou de classification
apparu dans le pipeline saturé et donne à Claude une réduction directement
falsifiable, sans matérialiser la mosaïque de Delaunay d'ordre supérieur.

## Résultat utilisable

À l'ordre `k`, un générateur saturé porte un événement élémentaire de Gamma si
et seulement si sa capacité vaut au moins `k` et si la cardinalité minimale
d'un support de sa boule vérifie `q_min<=k+1`. Sous une source certifiée
complète pour cet ordre, les générateurs `q_min>k+1` sont déjà entièrement
représentés à la coupe stricte : ils peuvent être retirés du `DSU_k` et de ses
postings sans perdre de connexion future.

La conséquence opérationnelle est importante : une continuation Gamma peut ne
changer ni la partition des observations ni leur couverture. `coverage_delta`
est donc un payload utile, jamais le prédicat qui décide si l'événement existe.

## 1. Définitions et hypothèses

Soit `X` un ensemble fini de points de l'espace euclidien tridimensionnel. Pour
une partie non vide `A`, notons `B_A` son unique miniboule et `beta(A)` son
niveau exact. Un générateur est un couple formé d'une miniboule `B` et de son
saturé fermé `M=X inter B`. Posons :

$$q(B)=\min\left\lbrace \lvert U\rvert:U\subseteq M,\ B_U=B\right\rbrace.$$

Il s'agit de la plus petite **cardinalité** parmi tous les supports de `B`, pas
de la taille d'un support arbitraire minimal pour l'inclusion. Les preuves
ci-dessous exigent également :

- niveaux de miniboules comparés exactement;
- membres `M` complets, triés et sans doublon;
- générateurs dédupliqués par boule et saturé;
- fermeture atomique de tous les générateurs de même niveau;
- certificat de complétude de la famille pertinente lorsque le résultat est
  annoncé comme exact.

## 2. Caractérisation locale des faces et cofaces

**Théorème 1.** Pour tout entier `k>=1` :

$$\exists F\subseteq M,\ \lvert F\rvert=k,\ B_F=B\quad\Longleftrightarrow\quad \lvert M\rvert\geq k\ \text{et}\ q(B)\leq k.$$

$$\exists C\subseteq M,\ \lvert C\rvert=k+1,\ B_C=B\quad\Longleftrightarrow\quad \lvert M\rvert\geq k+1\ \text{et}\ q(B)\leq k+1.$$

Ainsi, `B` porte une `k`-face ou une `(k+1)`-coface exacte si et seulement si :

$$\lvert M\rvert\geq k\quad\text{et}\quad q(B)\leq k+1.$$

**Preuve.** Si `U` supporte `B` et `U` est inclus dans `A`, lui-même inclus
dans `M`, alors `B` couvre `A`; réciproquement, toute boule couvrant `A` couvre
`U`. Les rayons minimaux coïncident et l'unicité de la miniboule donne
`B_A=B`. Il suffit donc de compléter un support de taille au plus `k`, ou au
plus `k+1`, jusqu'à la cardinalité demandée. Dans l'autre sens, tout ensemble
`A` de miniboule `B` contient un support de sa propre miniboule, donc
`q(B)<=|A|`.

Trois cas doivent rester distincts :

- `q(B)<=k` : au moins une `k`-face naît exactement à ce niveau;
- `q(B)=k+1` : une coface naît, mais aucune `k`-face;
- `q(B)>k+1` : le générateur ne porte aucun événement élémentaire de
  `Gamma_k`.

Si `q(B)=k+1`, chaque `k`-facette d'un support `U` de taille `k+1` possède un
niveau strictement inférieur. Sinon cette facette aurait elle aussi `B` pour
miniboule et fournirait un support de taille au plus `k`. Sous source complète,
un générateur de ce cas rencontre donc au moins une composante stricte et ne
peut jamais créer une naissance.

## 3. Réduction exacte de la tour à l'ordre `k`

Définissons la sous-famille pertinente :

$$\Sigma_k=\left\lbrace (B,M)\in\Sigma_X:\lvert M\rvert\geq k,\ q(B)\leq k+1\right\rbrace.$$

**Théorème 2.** À toute coupe exacte, stricte ou fermée :

$$\Gamma_k(a)=\bigcup_{\substack{(B,M)\in\Sigma_k\\\beta(M)\leq a}}J_k(M).$$

Cette égalité porte sur les graphes, pas seulement sur leurs composantes.
Chaque sommet `F` de `Gamma_k` est porté par `Sat(F)`, dont le support a taille
au plus `k`. Chaque arête élémentaire provient d'une coface `C` de taille
`k+1` et est portée par `Sat(C)`, dont le support a taille au plus `k+1`.
L'inclusion inverse vient du fait que toute partie d'un saturé actif possède un
niveau au plus égal à celui du générateur.

Pour `q(B)>k+1`, tout sous-ensemble de `M` de taille `k` ou `k+1` a même un
niveau **strictement** inférieur à celui de `B`. Le graphe `J_k(M)` existe donc
déjà avant l'activation de ce générateur. Une future intersection
`|M inter N|>=k` n'est pas perdue : choisir une `k`-face dans l'intersection;
son propre saturé, membre de `Sigma_k`, porte déjà l'attache, éventuellement
dans le même lot exact que `N`.

Cette réduction requiert `source_complete_for_order[k]=true`. Avec une source
partielle, supprimer un générateur `q>k+1` peut changer la sous-filtration
partielle; son résultat reste alors seulement `partial_refinement`.

Pour les ordres demandés de `1` à `K`, la fenêtre d'un générateur est :

$$\max\left(1,q(B)-1\right)\leq k\leq\min\left(K,\lvert M\rvert\right).$$

Pour une paire de générateurs de poids `w=|M inter N|`, sa fenêtre utile est :

$$\max\left(1,q(B_M)-1,q(B_N)-1\right)\leq k\leq\min\left(K,\lvert M\rvert,\lvert N\rvert,w\right).$$

En dimension trois, `q<=4`. La réduction retire donc les arités trois et quatre
à l'ordre un, puis l'arité quatre à l'ordre deux; tous les supports sont
admissibles à partir de l'ordre trois. Son premier bénéfice est la justesse du
transcript; le gain de masse dépend de la distribution réelle des supports.

## 4. Transcript exact après fermeture du lot

Au niveau exact `a`, marquer les nouveaux générateurs de `Sigma_k`. Après
activation de tout le lot, remplacer chaque générateur marqué par sa racine
finale et dédupliquer ces racines. Par le théorème 2 et la bijection S.4, les
racines finales marquées correspondent exactement aux composantes fermées de
`Gamma_k(a)` touchées par une nouvelle face ou coface.

Pour chaque racine marquée, compter les handles de composantes strictes
distinctes qu'elle absorbe :

- zéro : `birth`;
- un : `continuation`, même si couverture et partition d'observations ne
  changent pas;
- au moins deux : `multifusion`.

Plusieurs générateurs marqués dans la même racine produisent un seul événement
de composante. Une racine finale non marquée n'en produit aucun. Les compteurs
suffixés par `_batches` comptent des niveaux; un compteur par racine doit porter
un nom tel que `*_component_events`.

Garde fail-closed : une racine marquée sans handle strict doit contenir un
générateur `q<=k`. Si elle ne contient que des générateurs `q=k+1`, refuser le
transcript. La cause est nécessairement une source incomplète, un `q_min`
incorrect, un join incomplet ou un lot de niveaux égaux non atomique.

`coverage_delta`, `structural_growth` et les changements d'incidences restent
des payloads orthogonaux. Ils peuvent enrichir une continuation, mais ne la
créent ni ne la suppriment.

## 5. Correction de la fixture dite « silencieuse »

À `k=2`, la fixture `A={1,2,3}`, `B={2,3,4}`, `S={1,3,4}` puis
`N={1,4,5}` montre bien que jeter la posting de `S` perd l'attache future de
`N`. Elle ne prouve toutefois pas que `S` est silencieux pour Gamma :

- si `q(S)<=3`, son niveau porte une face ou coface exacte; `S` est une
  **continuation sans croissance de couverture** et sa posting doit rester;
- si `q(S)>3` et la source est `Sigma_2`-complète, des générateurs de niveaux
  strictement inférieurs portent déjà toutes ses faces et remplacent l'attache;
- si la source est partielle, la collecte peut effectivement changer la
  sous-filtration, mais le transcript n'est alors pas autoritatif.

La fixture actuelle de `postings_join_gate.cpp` force `n_support=1` sur des
ensembles synthétiques sans géométrie. Elle reçoit le join et l'atomicité, pas
le prédicat `q_min`. Son mutant `collect_silent` devrait être interprété comme
« collecte d'une continuation sans croissance », puis séparé d'un mutant qui
inclut à tort `q>k+1` dans le transcript exact.

## 6. Sources partielles et contre-exemples

Une source globalement partielle peut encore être exacte à l'ordre `k` si elle
certifie spécifiquement la complétude de `Sigma_k`. Sans ce certificat,
plusieurs racines partielles peuvent appartenir à une seule composante Gamma,
des composantes touchées peuvent manquer et les types naissance, continuation
et multifusion ne sont pas autoritatifs.

À `k=2`, une source partielle contenant seulement le générateur du cercle
circonscrit d'un triangle régulier a `q=3` et aucune racine stricte : elle le
classerait naissance. Dans la source complète, les trois paires existent
strictement avant et la coface les multifusionne.

Toujours à `k=2`, une source partielle réduite au générateur d'un tétraèdre
régulier, de `q=4`, contient `J_2(M)` à son niveau. Le supprimer change cette
sous-filtration. Dans une source `Sigma_2`-complète, il est redondant, car les
faces et cofaces élémentaires sont portées strictement avant par les
générateurs d'arités deux et trois.

## 7. Contrat proposé à Claude

1. Certifier par record `rank=|members|`, saturé complet et
   `minimum_support_cardinality=q_min`.
2. Publier `source_complete_for_order[k]`; ne pratiquer la réduction exacte que
   lorsque ce bit possède une preuve.
3. Figer les handles stricts avant le lot, puis appliquer ancien--nouveau et
   nouveau--nouveau dans un staging unique.
4. Activer seulement la fenêtre utile dans chaque `DSU_k`; en mode partiel,
   conserver au besoin la sous-filtration complète mais suffixer tout transcript
   `relative_to_certified_subfamily`.
5. Après fermeture, marquer, dédupliquer et classifier les seules racines
   d'événement par `0/1/>=2` handles stricts.
6. Appliquer la garde `q=k+1` sans racine stricte et committer atomiquement.
7. Comparer faces, cofaces, racines marquées et transcript à l'oracle Gamma
   exhaustif sur des catalogues géométriques complets, supports multiples
   compris.

Le `flat_catalogue` courant calcule déjà un support canonique de cardinalité
minimale sur la coquille complète dans `prototype/order_k_flats.hpp`, lignes
2471--2506. `CriticalSphere.n_support` peut donc servir de `q_min` sur ce chemin
borné, à condition que cette provenance soit certifiée. Le futur type
`SaturatedGenerator` doit nommer explicitement la propriété au lieu de la
déduire d'un support arbitraire.

Cette solution ne construit ni sous-simplexes, ni graphes de Johnson, ni
mosaïque globale. Elle ajoute un entier `q_min`, des bits de complétude par
ordre et un marquage local des racines déjà touchées par le join.

GCP non utilisé pour cette note.
