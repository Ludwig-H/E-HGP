# F : pile locale du comptage fusionné de témoins

5 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

**Le delta F remplace seulement le stockage de la pile de
`count_universal_witnesses`.** Ses 64 premières entrées sont locales au
conteneur ; un `std::vector` conserve les entrées supplémentaires si besoin.
L'ordre de visite, les prédicats, les masques et les comptes ne changent pas.
Cette note justifie ce remplacement ; elle ne transforme pas une qualification
locale en qualification intégrée ou en gain de temps. Le statut des campagnes
réellement closes appartient à [PASSATION](../PASSATION.md).

La référence est le compteur utilisé par E/q2. Ni `collect_universal_ids`,
ni `true_spindle_count`, ni les bases des secteurs, ni les MEB ne sont
modifiés par ce delta. La [proposition MEB par pivots](PROPOSITION_MEB_ET_BUDGETS.md)
et son budget physique restent un autre chantier.

## 1. Invariant de représentation et objection sur `empty()`

Noter C la capacité inline, i=`inline_size_` et V=`overflow_`. La séquence
LIFO logique est la concaténation des i premières entrées du tableau et
de V, dans cet ordre. Les invariants sont :

- `0 <= i <= C`, avec C strictement positif ;
- `V` non vide implique `i == C` ;
- les seules cases du tableau lues sont celles du préfixe occupé ;
- `back()` et `pop_back()` exigent une pile non vide.

L'initialisation donne i=0 et V vide. Si i<C, `push_back` écrit la case i,
puis incrémente i ; l'invariant garantit que V était vide. Si i=C,
il ajoute seulement à V. `pop_back` retire d'abord dans V ; il décrémente
i uniquement lorsque V est déjà vide. `clear` vide V avant de remettre
i à zéro. Aucun transfert entre les deux stockages n'est nécessaire.

L'objection « `empty()` ne regarde que i et peut oublier V » serait fondée
pour un état artificiel i=0, V non vide. Cet état est **inatteignable** par
les opérations admises : si V est non vide, i=C>0. Réciproquement, i>0
signifie qu'au moins une entrée inline existe. Ainsi `i == 0` équivaut
exactement à la vacuité de la séquence entière. Les tailles sont privées,
et copie/déplacement du conteneur sont interdits ; aucun état déplacé
incomplet n'est ajouté au domaine de cette preuve.

Après retrait de la dernière entrée de V, i reste C. Une poussée suivante
retourne dans V ; après un `clear`, les poussées utilisent de nouveau le
tableau, même si V a conservé une capacité allouée. Ces deux frontières
font partie des tests du helper. Un appel de `pop_back` à vide, un index
forgé ou un accès concurrent sans synchronisation ne sont pas légitimés
par ce remplacement.

## 2. Échec d'allocation et limites du contrat générique

Le helper impose un T trivial et de disposition standard. L'Entry réellement
utilisée contient seulement `NodeRef z` et `u8 open` ; construction et copie
ordinaires de cette valeur n'exécutent aucun code susceptible de lever une
exception. Les opérations employées doivent être bien formées : ces deux
traits ne constituent pas une API acceptant toute classe C++ imaginable.

La poussée inline effectue l'affectation avant l'incrément du compteur,
sans allocation. La poussée en débordement ne touche aucun état inline
avant `vector::push_back`. Pour cette valeur trivialement copiable, une
panne d'allocation ou un refus de longueur du vector laisse sa séquence
inchangée ; l'ensemble du LIFO garde donc son état antérieur. Le cas
`stack.push_back(stack.back())` conserve aussi sa valeur, y compris à
l'entrée dans V et lors d'une réallocation de V.

La garantie porte sur la **poussée individuelle**. Elle ne promet pas un
rollback de tout le compteur géométrique si un appelant hors domaine fait
échouer une poussée après plusieurs nœuds déjà traités. Le débordement
n'est ni tronqué silencieusement ni transformé en absence de témoin.

Sur le domaine produit prouvé ci-dessous, la taille reste au plus 49 :
l'addition `i + V.size()` ne peut pas déborder. Sur le vector de secours
de cette ABI avec Entry de 8 octets, `max_size()` est au plus
`PTRDIFF_MAX / sizeof(Entry)` ; ajouter 64 reste représentable en `size_t`.
Ce constat ne promeut pas le template en conteneur universel de capacité
arbitraire sur toutes les bibliothèques ou ABI.

## 3. Pourquoi 64 suffit au parcours produit

L'[audit d'index](../audits/AUDIT_INDEX_20260905.md) prouve que les positions
u16 sont encodées par 48 bits Morton, que l'arbre radix est binaire, que
les plages enfants partitionnent leur parent, et que le préfixe augmente
strictement le long des arêtes internes. Sa hauteur est donc au plus 48.
La porte produit garde n<=2^30−1 avant construction ; m désigne le nombre
de positions uniques. Le compteur reçoit un index valide non vide et
deux plages d'ancrage A/B disjointes.

Dans une descente en profondeur binaire, la pile contient le prochain
sous-arbre à traiter et au plus un frère différé à chaque profondeur.
Une hauteur h impose donc une frontière de taille au plus h+1. Ici :

$$\text{taille de pile}\leq h+1\leq49<64.$$

L'élagage, l'épuisement d'un masque ou l'arrêt au seuil suppriment des
descentes ; ils ne créent pas de frère supplémentaire. La preuve vaut
aussi pour les parcours avec crédit de sous-arbre et pour tous les s du
domaine : s n'intervient pas dans les formules de ces compteurs. Un peigne
peut atteindre 49 dans le parcours topologique sans élagage ; cela ne
prétend pas que chaque requête géométrique atteigne ce pic.

Ainsi V reste vide et n'alloue pas dans ce consommateur sur le domaine
normal. C'est une suppression d'allocations **de cette pile**, pas de
l'index, du pipeline ou de tous les appels de la bibliothèque. La voie
vector reste disponible pour les tests du helper ou un futur consommateur
plus profond ; sa présence ne valide pas un arbre cyclique ou forgé.

## 4. Conservation du parcours, des masques et des statistiques

Les deux implémentations initialisent la même entrée `(racine, mask_eff)`.
À toute itération, des séquences LIFO égales donnent le même `(z, open)`
à `back`, puis la même séquence après `pop_back`. Le corps du compteur
est inchangé : il intersecte le masque avec les lanes encore demandées,
applique les mêmes prédicats et, s'il descend, pousse **gauche puis droite**.
Le fils droit est donc traité en premier dans les deux versions. Par
induction, les séquences, arrêts et résultats sont identiques à chaque
étape, sous le même régime où les opérations de référence réussissent.

La [preuve arithmétique des témoins](../audits/ARITHMETIQUE_SPINDLE_COURANTE.md)
justifie les conséquences déjà acquises :

- une lane créditée perd son bit avant la descente ; ses crédits forment
  une antichaîne, sans double comptage ;
- A et B disjoints rendent sûrs les retraits de poids non signés ;
- les seuils nuls n'ouvrent aucune lane et le `min(count,h)` terminal est
  conservé, même lorsqu'un crédit individuel dépasse h ;
- `c[0..2]`, `nodes_visited` et `corner_evals` sont identiques, pas seulement
  les trois résultats après écrêtage ; leurs bornes locales restent celles
  du rapport, sans transfert aux cumuls de toute une génération.

Le mutant produit `witness-no-lane-mask` garde son site d'injection et son
rôle causal. Sur `{0,4,5,6,10}`, ancre 0/10 et seuil 10, la porte indépendante
conserve 3 identités intérieures et observe 8 crédits avec ce mutant. Le
raccord de F doit préserver cette dent dans la vraie route compilée ;
un différentiel nominal sans `MHGP7_TESTING` ne remplace pas ce contrôle.

Ne pas confondre ce compteur avec `true_spindle_count` : ce dernier arrête
au seuil sans écrêter son retour, peut rendre 3 pour h=1 sur cette ligne,
et n'a pas d'appel produit identifié par l'auditeur. Son commentaire
historique n'est pas une autorité d'écrêtage pour F.

## 5. Empreinte locale et autorité des contrôles

Un contrôle C++20 **de syntaxe seulement**, GNU C++ 13.3.0 sur Linux x86-64,
a vérifié les `static_assert` suivants le 5 septembre 2026 : Entry=8 octets,
tableau de 64 Entry=512, vector vide=24, `InlineStack<Entry,64>`=544,
alignement du conteneur=8. Il a rendu 0 sans diagnostic, sans produire
d'objet ou de binaire, avec `-Wall -Wextra -Wpedantic -Werror` et une borne
de 30 secondes. Les 512 octets ne sont donc **pas** la taille entière
du conteneur. Ces `sizeof` ne mesurent pas le frame machine après
optimisation, la consommation totale de pile d'un thread, le RSS ou un pic
pipeline ; le compilateur peut réorganiser les variables locales.

La source du contrôle était transmise sur stdin : inclusion du helper
privé épinglé ci-dessous et du vrai `tree/cloud_index.hpp`, puis déclaration
`struct Entry { mhgp7::NodeRef z; mhgp7::u8 open; };`. Les sept assertions
portaient sur `sizeof(NodeRef)==4`, `sizeof(u8)==1` et les cinq tailles ou
alignements ci-dessus. La commande utilisait `-fsyntax-only -x c++ -` ;
ce diagnostic n'est pas une nouvelle qualification du moteur.

La qualification privée préparatoire `build/v7_witness_stack_review/qualification`
conserve séparément O3 et UBSan, les frontières inline/débordement, les
alias de `back`, les échecs d'allocation et le différentiel du consommateur.
Son sceau est `98b9c44bb02a792f6e2dadf06706f0ebf12f6ce6eefdf561063ecb2b1f73b294`.
Ces preuves locales ne sont ni ASan/LSAN ni une suite pipeline intégrée ;
leur future publication et leurs dépendances doivent rester explicites.

Octets relus pour cette note, sous `build/v7_witness_stack_review/` :

| Pièce privée | SHA-256 |
|---|---|
| `proposal/src/core/inline_stack.hpp` | `59bdc34eab997583e8221469fdc5e2b9109dfc516cc54356397eebb1bb8aeb42` |
| `proposal/src/spindle/witness_count.hpp` | `f20970aa2183f6c0904d640ae5fe072894b6e0b4f7f9440277fbed77e8245803` |
| `reference/witness_count.hpp` | `fce9510ee21cbe025283a9ce777b120e4173ce62eee736e1c48dc772afb67a42` |

Le contrôle de taille consomme l'index
`8c5acf166ce378b0271e15850c54ca1740a8f6cb899d34a60c832a533504ad95`.
La [preuve CPU](../audits/DOMAINE_CPU_COURANT.md) nomme les prémisses
de types, de compilation et de bibliothèque ; aucune autre ABI ou GPU
n'est qualifié par ce diagnostic.

## 6. Coût et limites de la conservation

Le calendrier de `bad_alloc` change volontairement : la référence peut
échouer en allouant sa pile là où F n'en a pas besoin et réussit. L'égalité
sémantique du parcours normal ne signifie donc pas identité des pannes
sous une injection d'allocation commune. La garantie d'état du vector de
secours reste distincte de cette différence assumée.

L'état ajouté est local à une requête, de taille fixe sur le domaine
produit. Aucun catalogue global, cellule, coface, incidence ou mosaïque
de Delaunay d'ordre supérieur n'est matérialisé. Aucun cache partagé ou
travail parallèle nouveau n'est introduit. Le nombre de nœuds visités et
de prédicats géométriques reste inchangé : seul leur stockage de parcours
est visé. Une baisse d'allocations ne démontre pas à elle seule une baisse
de temps ; le frame local et les effets de cache doivent aussi être mesurés.

Les [contrats 50k/1 seconde puis 100 ms](CONTRAT_PERFORMANCE.md), le repli
sur toute la tour K1..5 et les dizaines de millions de points sur G4 ne
sont ni satisfaits ni extrapolés par cette preuve. Une mesure E/F doit
lier ses binaires propres, le même payload, K, s, plafonds et compteurs,
sans réutiliser un ancien chrono E. Aucun gain mono, multi-CPU ou GPU,
ni exactitude industrielle globale, n'est revendiqué ici.

## 7. Qualification intégrée exécutée

Les [reçus propres à F](../receipts/witness_stack_integrated_20260905/README.md)
ferment 48/48 portes ciblées Release, 48/48 ASan/UBSan et 339/339 Release
complètes, sans échec ni saut. Les exécutions sont fraîches ; les résultats
de la préparation privée et ceux de E ne leur sont pas transférés.
Le CLI Release qualifié est `ee29d3d5cfb49a728fa9dfa44fdb85a5a6043c941b1f61d4a6d9531ea4671f85`.

Les quinze nouvelles portes utilisent deux cibles : l'une observe les
allocations ordinaires `new/new[]`, l'autre désactive cet observateur pour
vérifier la sémantique avec l'allocateur natif sous sanitizers. Les zéros
des compteurs de la seconde ne constituent pas une preuve d'allocation.
Dans la première, 48 960 requêtes de référence et leurs correspondantes F
retrouvent les résultats, 808 908 visites et 74 172 évaluations de coins.
L'observateur Release voit 118 404 allocations ordinaires de référence
contre zéro pour le compteur F. Il ne couvre pas toutes les familles
d'allocation du processus et ne mesure pas une durée.

Le corpus comprend aussi permutations, seuils et masques, ancrages internes,
frontière topologique 49, débordements du helper et échecs de poussée.
Le vrai mutant de double crédit produit bien 8 contre 3, dans les deux
cibles ; les refus d'arguments gardent leur code 2 et les deux portes de
mutant leur code 4 attendu. Cette qualification conserve les invariants
de la note ; les effets de temps restent l'autorité des paires mono
distinctes, y compris lorsqu'aucun gain n'est observé.

GCP non utilisé pour cette note. Aucun rapport de l'auditeur modifié.
