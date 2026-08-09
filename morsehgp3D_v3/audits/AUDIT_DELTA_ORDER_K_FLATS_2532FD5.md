# Audit continu de `order_k_flats` — de `2532fd5` à `1a0a1f8` et Gate D

Date : 9 août 2026 UTC.

> [!IMPORTANT]
> **Verdict courant : GO ciblé pour le germe, les gardes u16, l'index k-d exact, la règle mathématique de parent, un endpoint reverse différencié et une API sink diagnostique; NO-GO inchangé pour une promotion exacte, le domaine multiplicitaire produit et le contrat 50 k.** Le commit `1a0a1f8` ferme le P0 d'élagage par classifications boîte--boule entières et retire les scans systématiques des singletons, du census et du pinceau. Les commits `6fa7e9d` et `969db5c` jugent le parent puis la reverse search. Restent ouverts l'implémentation live du propriétaire complet, l'intégration transactionnelle du sink au catalogue, le producteur terminal `directes + premières incidences`, les lots, l'état horizontal, les verticales et le contrat de sortie.

Cadre : `backend=reference_cpu_local`, `profile=quantized_u16_order_k_prototype`, `mode=exploration/diagnostic_only`, `public_status=not_claimed`. Aucun de ces libellés n'ouvre une phase officielle.

## 1. Snapshots et portée

| objet | SHA-256 ou identité |
| --- | --- |
| commit qui ferme le germe | `7cb176fd89568ea58b8bc945b64ab71f52fa6c2f` |
| `prototype/order_k_flats.hpp` dans ce commit | `2532fd5513b143beb55a75a34b05d286df64642bc5d47dd33876a0025ca7831c` |
| `prototype/flats_differential.cpp` dans ce commit | `a685092045bdf2288636f503e870a4a98fd528b63810ab41af63580e546c04ff` |
| `CMakeLists.txt` dans ce commit | `a879744bb73d17c25fb751fea30f99134656da0bbdf337faff294cf2087d0ca9` |
| commit qui ferme les gardes CLI/u16 | `7397cc214ebb4a03f1cc3ba4f90172cec4beb3de` |
| `prototype/order_k_flats.hpp` réaudité | `fefa573f9e7e369461ebf243afa60d23e1aae852c91c870119108ebf987aad49` |
| `prototype/flats_differential.cpp` réaudité | `dfa1f0a5686468af373967c0b0fc59a683a41844893b2841c62fd14c600fd963` |
| `CMakeLists.txt` réaudité | `66bbea1bf48fe09b736d456a6b5dbf88a46332da34631f85b5437a9e7c17f7dd` |

Ce rapport prolonge [`AUDIT_ORDER_K_FLATS_9C587E6.md`](AUDIT_ORDER_K_FLATS_9C587E6.md). Le commit `7cb176f` conserve la provenance du premier réaudit et `7397cc2` scelle son durcissement. Le ledger du §7 porte l'état courant au-delà de ces deux snapshots. Toute nouvelle empreinte demande un rejeu ciblé, pas la conservation mécanique d'un ancien finding.

> [!NOTE]
> Les §§2 à 6 conservent l'historique des snapshots `2532fd5` et `fefa573`.
> Plusieurs formulations devenues fausses après `1a0a1f8` — notamment le P0 de
> l'index et les scans globaux systématiques — sont supersédées par le ledger
> courant du §7; elles ne doivent plus être lues comme état live.

## 2. Le P0 de la descente de rayon est fermé

### 2.1 Justification de la nouvelle construction

Supposons la face bidimensionnelle, les coordonnées u16 distinctes et un repère euclidien orthonormé obtenu par isométrie avec $A=(0,0)$, $B=(L,0)$, $L>0$. Le point `ha` lexicographiquement minimal est exposé par un fonctionnel linéaire suffisamment perturbé. Les directions depuis `ha` appartiennent à un demi-plan ouvert; `plane_side` les ordonne modulo les directions collinéaires positives, puis le tie-break de distance complète l'ordre et choisit le premier point sur chaque rayon.

Pour un apex $C$ du côté $y>0$, écrivons son cercle sous la forme $x^2+y^2-Lx+D_Cy=0$, avec $D_C=\frac{Lx_C-x_C^2-y_C^2}{y_C}$. La factorisation du déterminant employé par le code est $L^2y_C^2y_d(D_C-D_d)$. Le prédicat est donc négatif exactement lorsque $D_d>D_C$, c'est-à-dire lorsque $d$ est strictement intérieur au cercle de $ABC$. Une passe conserve le maximum de cet ordre; une égalité désigne le même cercle cocirculaire.

Le segment obtenu n'est pas toujours une 1-face combinatoire maximale. Pour `A=(0,0,0)`, `B=(1,0,0)`, `C=(2,0,0)`, `D=(0,1,0)`, le code choisit `AB` alors que la 1-face maximale est `AC`. Le terme exact est **sous-segment support primitif**; cela ne casse pas la construction, car `C` est extérieur au cercle `ABD`.

Les contrôles qui positionnent `seed_failure_stage=5` et `seed_failure_stage=7` recontrôlent le segment et le cercle avec les mêmes prédicats. Ils ferment une mauvaise sélection, mais ne constituent pas un certificat indépendant de leur signe ou de leur largeur; la factorisation ci-dessus porte cette justification.

### 2.2 Rejeux

La fixture réfutant la descente de rayon est permanente :

```text
A=(0,0,0) B=(0,3,0) C=(2,1,0) P=(1,1,0) Q=(1,1,2)
```

- La porte versionnée rejoue ses 120 permutations à `s_max=5` : zéro refus, signature unique.
- Un harnais externe a rejoué les 120 permutations à `s_max=2..8`, soit 840 comparaisons : zéro statut ou census faux, germe unique de niveau 0.
- Les catalogues ont respectivement 9, 15, 19, 20, 20, 20 et 20 records pour `s_max=2..8`; les nombres de sommets sont 3, 4, 4, 4, 4, 4 et 4.
- Un contrôle transitoire de propriétés sur grille planaire $4\times4$ a exercé 294 040 ordres sans contre-exemple. Ce script Python n'est ni versionné, ni le C++ compilé, ni un oracle indépendant; il ne ferme aucune porte reproductible à lui seul.
- Les fuzz structurés Release et UBSan n'ont trouvé aucun désaccord. Ils restent des diagnostics tant que leurs harnais et reçus ne sont pas conservés.

Le faux potentiel de rayon et le débordement de `q*q+8` sont donc fermés. Le second disparaît structurellement avec la boucle, au lieu d'être seulement élargi.

## 3. Corrections créditées après le premier réaudit

Le commit `7397cc2` ferme réellement les points suivants :

- `--clouds 4294967296` et `--coord 4295032832` sont bornés avant le cast et rendent 2;
- neuf points distincts demandés dans huit positions rendent 2, et l'accord demandé/généré est vérifié par famille;
- `flat_catalogue` et `navigate_shallow` refusent avant tout prédicat une coordonnée négative, 65536 ou d'ordre $10^9$ avec `kOutsideDeclaredGrid`;
- la frontière 65535 demeure acceptée;
- une contradiction effective entre transport et census positionne `kInvariantViolated`; `flat_catalogue` rend alors un catalogue transactionnellement vide;
- CMake décrit enfin les trois primitives partagées par le juge;
- les trois nouvelles reproductions ont des CTests négatifs permanents.

Build Release propre sur les empreintes de `7397cc2` : 13/13 CTests `mhgp3v_flats_*` verts en 11,08 s. Les quatre portes positives rendent 170, 1 143, 1 250 et 2 030 cas sans désaccord. Un probe UBSan externe appelle les deux entrées avec des coordonnées d'ordre $10^9$ : statut `hors_grille_u16_declaree`, catalogue, membres et sommets vides, aucune erreur sanitizer.

## 4. Portes encore ouvertes

### 4.1 Le contrat C++ n'est pas encore totalement fermé

La garde u16 est correcte pour les deux entrées réauditées; l'ancien finding « u16 gardé seulement au CLI » est **fermé**. Trois résiduels distincts restent :

1. `flat_catalogue` ne valide pas `s_max`; `s_max=INT_MIN` atteint encore le calcul signé `s_max-2`. L'API doit refuser l'ordre avant les singletons et avant toute soustraction.
2. Sous `verify_census=true`, un `census(...) == false` est ignoré. L'impossibilité de reconstruire la sphère d'un sommet doit elle-même devenir `kInvariantViolated`.
3. `navigate_shallow` ajoute le sommet courant à `visited` avant le census et rend donc un préfixe partiel avec le statut d'erreur. `flat_catalogue` jette ce préfixe, mais l'autre entrée annoncée publique n'est pas transactionnelle.

Le test permanent des quatre frontières appelle seulement `flat_catalogue`; le probe externe couvre `navigate_shallow`. Il faut graver les deux entrées, exiger `kOk` sur la frontière valide et injecter les deux branches de census.

Reproduction UBSan sur le header `fefa573`, avec un nuage de cinq points de dimension affine trois dans la grille : `flat_catalogue(..., INT_MIN, ...)` échoue à `order_k_flats.hpp:1007` sur `-2147483648 - 2`, code 1. Un mutant transitoire qui remplace seulement le niveau du germe par 1 produit `kInvariantViolated` avec un sommet rendu par `navigate_shallow`, contre un catalogue et des membres vides par `flat_catalogue`.

Les helpers de germe et de prédicats restent appelables sans garde. Cela est acceptable seulement s'ils sont explicitement internes avec précondition u16; ils ne doivent pas devenir une API parallèle non protégée.

### 4.2 Le juge et le payload ne sont pas indépendants

Le payload est nettement mieux jugé qu'au snapshot `9c587e6` : doublons, tranches de membres, contiguïté, sentinelles, ordre et appartenance à la boule publiée sont contrôlés. Le résiduel est plus étroit mais décisif avant un statut exact : centre rationnel, rayon et `beta` ne sont pas confrontés à une vérité distincte, et les forêts ne sont pas construites.

La vérité partage encore `sphere4`, `sphere_side`, `miniball_of`, le bon centrage et la convention canonique. Le libellé défendable reste « portée de navigation et signature de catalogue concordantes relativement aux primitives v2 ». Une référence rationnelle multiplicitaire et des injections de centre, rayon, `beta`, membres, ordre et statut restent requises.

### 4.3 Couverture et compteurs

L'accord demandé/généré ferme la censure silencieuse d'une famille. Les minima demeurent agrégés par invocation : la part forcée cosphérique, les statuts accepté/refusé/décidé et toutes les branches suivies n'ont pas chacun un plancher propre.

Les bornes individuelles du CLI autorisent aussi jusqu'à un million de nuages et `smax=4096`, alors que `cases` et `failures` sont des `int`. Le nombre de cas autorisé par cette combinaison dépasse `INT_MAX`; il faut soit borner la combinaison avant la boucle, soit employer des compteurs larges.

Les tableaux live du README annoncent encore 19 544 et 13 669 cas. Le nouveau bloc de quatre frontières incrémente `cases` une fois sans consommer le générateur; les mêmes campagnes doivent donc être rejouées et reçues avant de republier leurs totaux, qui sont mécaniquement décalés d'une unité.

### 4.4 Multiplicités et domaine produit

Deux coordonnées identiques sont maintenant refusées explicitement et transactionnellement. C'est la bonne politique fail-closed pour ce prototype distinct, mais pas la sémantique du profil produit quantifié : transformation, collisions et multiplicités doivent y être conservées ou quotientées selon le contrat. Le profil dyadique exact demeure également hors de ce chemin.

### 4.5 P0 documentaire fermé dans le worktree — le parcours et la mosaïque n'ont pas les mêmes sommets

Le README et la proposition du commit `7397cc2` affirmaient que le parcours visite « le même objet » que la mosaïque de Delaunay d'ordre supérieur et que leurs nombres de sommets sont identiques. C'est faux, même en position simple. Les deux documents sont corrigés dans le worktree audité; ce finding sera fermé durablement avec leur prochain commit.

Prenons les quatre points u16 `A=(0,0,0)`, `B=(1,0,0)`, `C=(0,1,0)`, `D=(0,0,1)`. Les quatre hyperplans relevés ont un unique sommet d'arrangement de niveau strict zéro. La mosaïque de Delaunay d'ordre 1 possède pourtant quatre sommets, un par observation, et un tétraèdre. Le sommet d'arrangement est dual du sommet de Voronoï et de la **cellule tridimensionnelle** de Delaunay; ce n'est pas un sommet de cette mosaïque.

La formulation correcte est plus étroite : sous position générale, le pavage de rhomboïdes est dual de l'arrangement et sa tranche de profondeur $k$ est la mosaïque d'ordre $k$. Un sommet d'arrangement est dual d'un rhomboïde de dimension quatre; ses tranches non triviales donnent des cellules Delaunay tridimensionnelles à plusieurs ordres. La dualité et la tranche ne préservent donc ni la dimension, ni le nombre de sommets. Avec des coquilles multiples, un même record `(niveau, coquille)` se décline en plusieurs ordres et cellules, ce qui éloigne encore toute bijection naïve.

La borne de Clarkson--Shor donne bien $O(n^2k^2)$ en dimension quatre sous ses hypothèses asymptotiques et de position générale; pour $K=10$ fixé, elle est quadratique en $n$. Il faut écrire « aucune borne worst-case utile au contrat », et non « aucun théorème utilisable ». Cette borne ne démontre ni l'identité de $V$, ni le facteur mémoire cinq à dix revendiqué.

Références primaires : [Edelsbrunner--Osang, définition et dualité des mosaïques d'ordre supérieur](https://pub.ista.ac.at/~edels/Papers/2020-J-07-SimpleAlgorithm.pdf), pp. 2--4; [Clarkson--Shor, corollaire 3.3](https://link.springer.com/content/pdf/10.1007/BF02187740.pdf), p. 397.

### 4.6 P0 live — l'index k-d n'est pas encore certifié

Un delta non committé postérieur à `7397cc2` ajoute `CertifiedIndex`, un k-d tree, et confronte son catalogue au chemin de référence. Le témoin ci-dessous a été rejoué sur le header `1117793f843f3cdca4da36beebfdeac11a51a2d8a7a813ac9a40ec642cac499b` et le différentiel `f71097e4c1675e2b0a5b9805d5e16c92eb6a2d2cac76bf3fae98c163bf29721b`. Ce delta évolue rapidement; tout correctif doit être réépinglé et rejoué sur son commit éventuel.

Crédit : l'appartenance finale reste décidée par `sphere_side`, le chemin indexé est comparé record par record au chemin lent, les singletons distincts passent en temps constant et le census de récolte devient local. Une itération live ultérieure remplace en outre le balayage de l'union des deux boules terminales par les points dont les signes ternaires diffèrent : c'est une amélioration mathématiquement valide, car la puissance est affine le long du pinceau et tout événement non constant change de signe entre les extrémités. Le nom live `symmetric_difference` est toutefois faux : le cas signe zéro contre négatif appartient aux deux boules fermées, mais doit être rendu et l'est par le code. Le même delta ferme aussi, au niveau statique, les trois résiduels du §4.1 : ordre gardé avant soustraction, échec de construction du census rendu invariant et préfixe de navigation vidé. Ces fermetures restent à graver par des tests directs et à sceller dans un commit. Le certificat d'élagage, en revanche, est faux dans sa forme rejouée.

Le commentaire suppose que centres et rayons restent sous $2^{17}$ parce que les **points** sont u16, puis ajoute une marge flottante fixe de 0,5. Or `sphere3` et `sphere4` autorisent un numérateur de centre énorme avec un petit déterminant; la fixture permanente `giant_centre_det1` existe précisément pour ce régime. L'erreur absolue de `double(base + num/den)` et de la racine peut alors dépasser 0,5. Le test exact en feuille ne répare pas un nœud élagué à tort : un point de la boule fermée peut ne jamais atteindre la feuille.

Le contre-exemple u16 minimal est :

```text
a=(32767,32767,0) b=(57863,57862,0)
c=(7672,7673,0)   d=(60104,30135,1)
```

La sphère exacte portée par ces quatre points satisfait `sphere_side==0` sur les quatre supports. Avec exactement le `leaf_size=16` du chemin produit, `closed_ball` rend `touched=0` et aucun identifiant : la boîte racine elle-même est élaguée. Le harnais transitoire final a pour SHA-256 `5a1cd210...`; la fixture et sa sortie doivent être rendues permanentes avant fermeture.

Les campagnes courantes ont au plus 13 points. Elles exercent bien la décision `node_may_touch` sur la boîte racine; elles n'exercent en revanche ni découpe en nœuds internes, ni cas arithmétique adverse. Il faut conserver le témoin quatre-points, ajouter une fixture à au moins 17 points, puis publier des compteurs de nœuds gardés/élagués. `union_sweeps` existe dans les statistiques mais n'est ni agrégé, ni imprimé, ni planché.

La correction la plus directe reste entière : pour chaque axe d'une boîte, calculer exactement les distances rationnelles minimale et maximale du centre à l'intervalle, sommer les carrés avec `mul128`, puis comparer à `sphere_num2`. Un nœud n'est élagué de la requête de désaccord que s'il est strictement intérieur aux deux boules ou strictement extérieur aux deux; les égalités descendent. Le k-d tree garde ainsi son avantage sur les régions vides sans aucune conversion flottante. La preuve, les largeurs, le lemme de désaccord ternaire et le contrat de propriété sont donnés dans [`NOTE_POSITIVE_INDEX_KD_EXACT_ET_CERTIFICAT_PINCEAU.md`](NOTE_POSITIVE_INDEX_KD_EXACT_ET_CERTIFICAT_PINCEAU.md). À défaut, il faut une enveloppe dirigée prouvée sur les vraies largeurs de `num/den`, avec repli intégral dès que cette preuve ne conclut pas.

Le header emploie aussi `std::isfinite` et `std::sqrt` sans inclure `<cmath>`. Un TU qui inclut seulement ce header échoue; le différentiel masque la dette en incluant `<cmath>` avant lui. Cette faute d'auto-suffisance est P1, distincte du P0 d'élagage.

Enfin, `navigate_shallow` accepte un pointeur d'index construit hors de l'appel. Un index bâti sur un autre nuage, ou sur un vecteur ensuite modifié, peut avoir un CSR et des boîtes périmés tout en conservant des identifiants valides. Cette entrée doit être internalisée derrière `flat_catalogue`, ou authentifier une vue immuable; l'exactitude du chemin interne ne doit pas créer une seconde API non gardée.

## 5. Le NO-GO 50 k ne bouge pas

Les durcissements ci-dessus ne changent aucun coût décisif :

- les `n` singletons déclenchent encore `n` census globaux, soit 2,5 milliards de classifications avant le germe à 50 k;
- `seen`, `frontier` et `visited` matérialisent tous les sommets et leurs coquilles;
- chaque direction de flat et chaque tentative d'émission rescannent le nuage, ce qui conserve au moins un terme de travail en $\Omega(nV)$;
- les triplets d'une coquille sont énumérés avant quotient;
- propriétaire local, index fail-open, reverse search, streaming, forêts horizontales et verticales, incidences silencieuses, lots contractuels et `coverage_log` sont absents.

Cette correction de dualité ne rend pas le parcours actuel plus petit : son propre nombre de sommets d'arrangement reste mesuré, entièrement matérialisé et multiplié par les rescans. Elle retire seulement un faux argument d'identité et le facteur mémoire qui en était déduit.

La reverse search sous arrangement simple est un sous-problème d'architecture légitime si le domaine reste gardé fail-closed et si aucune mesure réelle n'est revendiquée. Elle ne qualifie pas le contrat u16 dégénéré, où les multiplicités sont normales.

Ce chemin est CPU-only : aucune G4 ne doit être utilisée pour ses tests ou mesures. Une G4 ne sera justifiée que lorsqu'un véritable kernel CUDA du pipeline qualifié existera.

## 6. Porte de reprise

1. Fermer `s_max`, le retour faux du census et l'atomicité de `navigate_shallow`; graver les injections.
2. Donner des compteurs larges et des planchers propres à chaque famille et branche.
3. Construire la référence rationnelle multiplicitaire avant toute fermeture de qualification exacte.
4. Conserver la correction de dualité arrangement--Voronoï--Delaunay du worktree et la sceller avec l'audit.
5. Remplacer la marge flottante du k-d tree par le test boîte--boule exact de la note positive, puis exercer la racine adverse, ses nœuds internes et fermer son contrat de propriété.
6. Conserver ce BFS comme oracle structurel borné; développer séparément la source streamée/reverse-search qui évite la matérialisation globale.
7. Ne tenter 50 k qu'après suppression des census globaux, preuve de la règle de propriétaire et sortie complète avec forêts et incidences.

Décision historique : créditer pleinement le nouveau germe, la garde u16, le CLI fermé et le progrès du census. Maintenir `exploration/diagnostic_only` et le NO-GO exact/50 k.

## 7. Ledger courant après `1a0a1f8` et premier juge Gate D

### 7.1 Snapshots réaudités

| objet | SHA-256 ou identité |
| --- | --- |
| commit index exact et census local | `1a0a1f8446d980f9b4068ee331eeee8228ff1089` |
| header committé | `b71fda051a190e90b3541827d167b8c8de5d350dac0f7be5696f3d363b6903cf` |
| différentiel committé | `c6a75ba9bbdd038e6f26f1df3cc334a39af607735f395062162bc1c5481987c7` |
| header live Gate D | `ce9244647c3bbead6332a70ada325995e4004c1ccdc36d618e2164d26b20a27d` |
| différentiel live Gate D | `0168d33d514483c408834b5570e9c63c4d54e2843ad07482d8ba77722218ffcf` |
| CMake live | `4baf8410ac2b26d74f99430471e671cea96ac64f4e935db2da5ccded9031b826` |
| commit du premier juge parent | `6fa7e9dd8e6b202c024f2826ac0640b0bab9a574` |
| header worktree avec `ParentEdge` | `218aa8a76a2d5251aee0addd2bd6b217103035ddc751eda3d106da28b1fdfb27` |
| différentiel worktree renforcé | `aec6bbf04a7ca4ca987329f0434cd9eb3daa7eddf354e4350c084abcd4ca6391` |
| CMake worktree | `b5063a3476de98483386711ed2488a664f7d6f2700b69a9df43d036d19a5e211` |
| prototype de première incidence | `9eee050171267eb7213e51214b5864d52aa533048a405294094f4a47a7ac6fee` |

Les trois premières empreintes live de la table décrivent le delta qui a précédé
`6fa7e9d`; elles sont conservées comme provenance. Le worktree alors courant ajoutait le
payload `ParentEdge`, un juge de potentiel et un prototype de première incidence.
Les résultats plus forts du parent proviennent d'un harnais exact externe et
sont explicitement qualifiés comme transitoires ci-dessous.

### 7.2 Matrice de clôture

| finding antérieur | état courant | preuve ou dette résiduelle |
| --- | --- | --- |
| ordre `s_max`, census impossible, préfixe partiel | **fermé en code** | garde avant soustraction, `kInvariantViolated`, `visited.clear()`; injections directes des deux API encore à graver |
| P0 d'élagage k-d à centre lointain | **fermé** | boîte--boule entière, fixture quatre points et nœuds internes permanents |
| header sans `<cmath>` | **fermé** | include committé |
| singletons en $O(n^2)$ | **fermé dans le chemin indexé** | publication directe sous points distincts |
| census et pinceau rescannant toujours $X$ | **fermé comme scan systématique** | `closed_ball` et désaccord ternaire indexés; visite $O(n)$ encore possible au pire |
| propriété de l'index | **partiel** | appel interne propriétaire correct; pointeur public étranger ou périmé encore non authentifié |
| propriétaire et déduplication | **théorème fermé, live ouvert** | les deux inclusions, le cône signé et le support canonique donnent un owner local unique; le live garde seulement le premier préfiltre puis `emitted` |
| parent local multiplicitaire | **prouvé, substitué dans un endpoint différentiel** | commit `969db5c` : décision sans `seen/frontier`, parité BFS; sortie encore matérialisée, catalogue non substitué |
| source HGP complète | **théorèmes conditionnels fermés, produit ouvert** | les cofaces d'une sphère certifiée sont caractérisées, leur owner est shallow et `directes + attaches` suffit horizontalement; harvest output-sensitive, stream terminal, lots, couverture et verticales absents |
| profils produit | **ouverts** | doublons refusés, dyadique absent, oracle encore relatif aux primitives v2 |

Le P0 de l'ancien §4.6 est donc fermé au commit `1a0a1f8`. Les phrases du §5
affirmant encore un census global pour chaque singleton, chaque direction et
chaque émission sont historiques et désormais fausses. Le NO-GO 50 k demeure,
mais pour les raisons actuelles : volume $V_k$ sans borne utile au SLO,
énumération des flats et queue lourde des coquilles, index sans borne
sous-linéaire universelle, sink non composé au catalogue et high-water partiel,
propriétaire non intégré et pipeline HGP aval absent.

### 7.3 Parent local : crédit et dettes exactes

Le delta live implémente le corollaire constructif de
[`NOTE_PARENT_LOCAL_REVERSE_SEARCH_GATE_D.md`](NOTE_PARENT_LOCAL_REVERSE_SEARCH_GATE_D.md) :
signes tangents par `orient3d_exact`, contraintes de coquille, croissance de
$L_h$ ou décroissance de $Q_r$, puis clef déterministe de flat. Le premier delta
prenait quatre membres coplanaires pour la base de $Q_r$ et produisait deux
racines sur six points. L'extraction gloutonne indépendante ferme ce P0 et la
fixture `germe_base_non_independante` est permanente.

Un rejeu externe exact couvre 5 623 nuages, 146 729 sommets et 15 258 sommets
multiples. Il vérifie 123 240 hausses rationnelles strictes de $L_h$, 17 866
baisses strictes de $Q_r$, le rang quatre de la base, l'inclusion des intérieurs,
une racine et zéro cycle : zéro échec.

Les deux grandes campagnes du README annoncent 4 761 et 5 611 cas, puis 510 154
et 530 197 sommets avec parent. Elles sont cohérentes avec les compteurs du juge,
mais aucune commande complète, graine, sortie brute ni sidecar versionné n'est
conservé dans le dépôt. Elles restent des mesures diagnostiques non reçues, pas
une porte reproductible.

Le commit `6fa7e9d` rend le second parcours fail-closed sur son statut et la
taille du vecteur de parents. Le worktree `218aa8a` / `aec6bbf` ajoute une
`ParentEdge`, compare exactement les potentiels $L_h$ et $Q_r$ par produits
croisés, confronte la fermeture à l'intersection des coquilles et vérifie les
cycles. Le build incrémental et la fixture permanente passent; une campagne
saturée locale de 341 cas porte 11 248 sommets parentés et 265 racines, sans
désaccord.

Ces progrès ferment les anciens constats « skip silencieux » et « potentiel non
jugé ». Cinq P1 restent précis.

1. L'out-paramètre `parents` n'est ni vidé à l'entrée, ni restauré sur erreur;
   un vecteur prérempli ou un census rouge laisse un résultat désaligné ou
   partiel.
2. Le producteur rend encore `kOk` lorsqu'un sommet non racine ne trouve aucun
   parent; seul le juge global requalifie ensuite cette absence.
3. Aucun plancher `parent_vertices` ni test négatif n'exerce
   `GATE D INEXPLOITABLE`; retirer tout le bloc parent peut laisser les CTests
   verts.
4. Le second parcours emploie `verify_census=false` et n'est comparé ni au
   parcours certifié, ni à la vérité de sommets; un sous-parcours auto-cohérent
   peut former un arbre valide et passer.
5. Le juge n'utilise pas `ParentEdge.orientation`. Une fermeture contenant un
   triplet non aligné n'est pas vérifiée entièrement coplanaire, et
   $S(v)\cap S(w)=C$ ne prouve pas que $w$ est le **premier** événement du rayon.
   L'adjacence orientée reste donc ouverte.

Le commentaire CMake est maintenant périmé dans l'autre sens : il dit que rang,
transition et potentiel ne sont pas vérifiés, alors que le worktree les juge
partiellement. Il faut décrire exactement les résiduels ci-dessus.

### 7.4 Le propriétaire complet est local, mais pas encore live

Le préfiltre $B(v)\subseteq B_U$ accélère réellement la récolte; il ne certifie
ni $v\in P_U$, ni l'unicité. La fixture minimale à cinq points de la note laisse
trois sommets passer : un échoue $B_U\subseteq B(v)\cup S(v)$, puis deux ont le
même $G_U$ et exigent le tie-break. Sur le live, elle conserve un catalogue exact
mais encore dix déduplications par `emitted`.

La règle complète, prouvée dans la note, est : census fermé, rang, support
canonique de la coquille, rejet des supports non canoniques, puis optimalité
locale dans le cône signé de $P_U$. Le cube u16 possède six supports minimaux de
la même boule — quatre paires antipodales et deux tétraèdres de parité : appliquer
seulement un owner par support émettrait
encore quatre fois. La composition « support canonique puis owner » est donc la
condition exacte pour supprimer `emitted`.

### 7.5 Ce qui reste global

La suppression de `seen` ne rend pas le contrat HGP local. La
[`NOTE_GATE_D_GLOBALITES_RESIDUELLES.md`](NOTE_GATE_D_GLOBALITES_RESIDUELLES.md)
sépare cinq classes : lecture immuable de $X$, tri et lots exacts
externalisables, partition horizontale vivante, jointure verticale adjacente et
identités exhaustives éventuellement imposées par le contrat v2. Le théorème
horizontal conditionnel et la dichotomie de
[`NOTE_GATE_D_PREMIERES_INCIDENCES_DU_COEUR.md`](NOTE_GATE_D_PREMIERES_INCIDENCES_DU_COEUR.md)
ferment désormais la forme mathématique de la source sparse : cofaces directes,
facettes du cœur, puis toutes leurs premières incidences. Le résiduel est de
produire et authentifier ce flux terminal. Les autres globalités peuvent être
streamées; elles ne peuvent pas être supprimées sémantiquement. Même le locator
résident est remplaçable par un journal externe de versions et du
pointer-jumping, mais l'information de partition reste irréductible.

### 7.6 Review du prototype de première incidence `9eee050`

Crédit : le prototype matérialise exactement la bonne factorisation, développe
$k+1$ suppressions par coface directe, reconstruit $E_F$ par `closed_ball`,
conserve tous les ex æquo et se compare à un balayage exhaustif pour chaque
facette qu'il reçoit. Ses cinq CTests passent. Les trois campagnes positives
exercent empiriquement les deux branches et rendent respectivement 280/421,
829/628 et 1 012/499 facettes fermées/vides, sans désaccord relatif.

Le claim de **source directe complète** est toutefois faux sur ce snapshot. Le
code filtre `sphere.rank == k+1`, c'est-à-dire la vacuité **fermée**, alors que
le théorème demande toutes les cofaces de Gabriel à vacuité intérieure, avec une
politique explicite des extra-shells. Sur les cinq points

```text
(0,0,0) (0,2,2) (2,0,2) (2,2,0) (0,0,2)
```

les quatre premiers forment un tétraèdre régulier et le cinquième est sur sa
sphère. L'oracle ouvert compte cinq cofaces de Gabriel de taille quatre; le
prototype n'en conserve qu'une et en omet quatre. Il affiche pourtant zéro
désaccord, car les facettes disparues ne sont jamais soumises à
`brute_first_incidence`. Le probe transitoire a pour SHA-256 source
`f8c273fb9882bf1a3f97f7c0574a08bff73efe8d5cf29a8d493b3535cc58e111` et
binaire `c61b62f858059b4e26e9a2499804378e93ee635fee046730cb23758d3bd079a4`.

La reproduction sans harnais
`--clouds 1 --points 7 --coord 2 --k 2 --seed 1` est plus sévère : six facettes
ont une coface décidée contre trois co-minimiseurs exhaustifs et le binaire rend
1. La même commande avec `--no-judge` rend 0 et imprime encore « exactement ».
Le suffixe `--clouds 1junk` est accepté; `4294967297` est tronqué en un nuage.

Autres dettes de qualification : un statut non `kOk` censure silencieusement un
nuage; aucun plancher par branche ou source n'est imposé; la vérité partage
`miniball_of` et `sphere_cmp_beta`, puis calcule `truth_level` sans jamais le
comparer. L'identité de masse des suppressions est tautologique après leur
construction et ne certifie aucune terminalité. `duplicate_cofaces` n'est pas
une condition d'échec; aucune seconde déduplication des $M(F)$ proposés par
plusieurs facettes n'est mesurée.

Les CTests utilisent $n\leq11$ avec des feuilles d'index de taille 16 : chaque
requête touche les $n$ points et n'exerce aucun nœud interne. Enfin,
`deletion_bytes` n'est ni la taille de `Record`, ni celle d'un wire exact défini.

Verdict : bon falsificateur borné de la dichotomie sur le domaine générique;
NO-GO comme capability de source jusqu'à la fixture extra-shell, une vérité
directe exhaustive indépendante, les planchers, le CLI fermé, les budgets
atomiques et les permutations exigés par la note.

### 7.7 Correction de la source ouverte au commit `b9b5b4c` et delta d'attache

Le commit `b9b5b4c` répond correctement au P0 principal du §7.6. Le prototype
de première incidence y possède l'empreinte
`dcd19178bd84391b5aa5a4135575d235ee8669ae477ff05c55d9720d1ddd02d3`
et CMake l'empreinte
`1425f0bf75a88bfdd7a3d312c092f415fc21fddc2d31a2b9395fe8b5e3704737`.

Crédit : la source développe maintenant chaque sphère critique en
$I(B)\cup T$, où $I(B)$ contient tous les intérieurs et $T$ parcourt les
sous-ensembles de coquille du cardinal requis. Relativement à un catalogue
critique complet, cette construction énumère toutes les cofaces de Gabriel à
vacuité ouverte. La vérité énumère séparément tous les $(k+1)$-sous-ensembles,
construit son propre univers de facettes, puis compare source, facettes,
$\lambda(F)$ et $M(F)$. Le CLI est strict, les statuts non `kOk` rougissent et
les niveaux sont comparés. Les sept CTests passent; les trois portes positives
rendent respectivement 1 400/1 120, 995/2 045 et 311/816 cofaces/facettes, zéro
manquante, zéro surnuméraire et zéro désaccord. L'ancien témoin
`--clouds 1 --points 7 --coord 2 --k 2 --seed 1` est désormais vert avec 31
cofaces et 21 facettes; `--no-judge` et le suffixe `1junk` sont refusés.

Cette fermeture qualifie un **oracle borné relatif aux primitives partagées**,
pas une source produit. Le sujet appelle `flat_catalogue(pts,n)`, matérialise le
catalogue complet puis développe combinatoirement les coquilles; la vérité
énumère $\binom{n}{k+1}$. Cette architecture est précisément celle que le
contrat 50 k interdit. `miniball_of`, `sphere_side` et `sphere_cmp_beta` restent
communs. De plus, un échec de `miniball_of_set` est encore converti en simple
« non-Gabriel » dans `gabriel_open`, et en `continue` dans la vérité de première
incidence : une erreur géométrique peut donc censurer scientifiquement une
coface au lieu de faire échouer la porte.

Le plancher `--min-internal-nodes` ne compte pas les nœuds visités par les
requêtes : il additionne tous les nœuds internes **construits** par l'index. Le
run à vingt points touche 9,2 points par facette, ce qui établit un élagage
empirique, mais pas le nombre de nœuds parcourus. Les deux multiplicités de
provenance sont comptées sans déduplication terminale ni plancher. Budgets,
rollback, permutations de runs, wire exact, high-water et oracle 10.6 appelé
restent absents.

Le commit suivant `bf53620`, épinglé par les empreintes
`81d0d18fc19468e13fa8d4fe975f14b643922445104c6ecd83ef36e9383866c7`
et `e1e6764321243df997bc9d00e82d7bccdffb0a0504fa43827d1f84a61eda75d1`,
instrumente la règle d'une attache par facette. Il crédite utilement les classes
d'intrus stricts, la descente $\beta(T_F)<\beta(F)$ et les cibles brutes hors du
cœur. Il ne vérifie toutefois ni la porte régulière, ni le support unique
essentiel, ni l'absence d'extra-shell; il choisit `support[0]`, compte le
payload fermé `decided` comme masse remplacée et ne résout aucun carrier. Une
violation de descente hors domaine régulier ne doit pas réfuter la dichotomie,
et une campagne verte ne juge pas encore l'équivalence des quotients.

La provenance publiée a dû être rectifiée : les nombres 5 103 facettes,
1 997/3 011/95 classes d'intrus et 210 co-minimiseurs viennent de 30 nuages,
pas des 12 du CTest. Cette campagne compte 3 184 branches fermées mais seulement
3 106 facettes avec au moins un intrus strict; elle contient donc au moins 78
égalités extérieures sans intrus et **échoue effectivement** la porte régulière.
Les 95 objets sont des candidats locaux, pas des attaches autorisées. Le CTest
à 12 nuages exerce 39 candidats, 88 co-minimiseurs fermés et 5 cibles brutes
hors du cœur; son plancher certifie uniquement que cette branche de diagnostic
n'est pas vacante.

Le résultat mathématique exact est maintenant plus fort :
[`NOTE_GATE_D_UNE_ATTACHE_PAR_FACETTE_COEUR.md`](NOTE_GATE_D_UNE_ATTACHE_PAR_FACETTE_COEUR.md)
prouve qu'un census régulier saturé à deux et une unique attache suffisent. La
descente locale transforme son bras en une clef cœur $R_F\in D_k$. Sa fixture u16
à dix points rend les trois bras immédiats hors de $D_3$; aucun choix alternatif
du support supprimé ne remplace cette descente. Seul le `find_<a_F(R_F)>` dans la
partition du cœur reste global dans la résolution du carrier.

### 7.8 Reverse search live au commit `969db5c`

Le commit `969db5c` fige les empreintes suivantes :

| objet | SHA-256 |
| --- | --- |
| `prototype/order_k_flats.hpp` | `2ca55c946c666c2ff91bcc49514dc48806e2586e93cf5673a31d6f7cb4e4f537` |
| `prototype/flats_differential.cpp` | `1a4bfb96c4e16aad433e5fba1ea12ff268e3b3d65b84cea5cb990fd7d89d73d4` |
| `CMakeLists.txt` | `4893b4c82e101b47d687b32675d58dd85eb37b933ac83a1c0ad5b9bc06a81185` |

Crédit scientifique : `reverse_search_shallow` n'emploie ni `seen` ni
`frontier` pour décider si un sommet est un fils. Il énumère les flats et les
deux orientations dans un ordre déterministe, recalcule le parent du candidat,
et descend exactement lorsque ce parent est le sommet courant. Le différentiel
compare au BFS les statuts, les coquilles et les ensembles intérieurs, puis
refuse tout doublon de coquille.

Un build Release frais et les treize CTests `mhgp3v_flats_*` passent. Les quatre
portes positives cumulent 4 757 cas, 325 498 sommets de reverse search et
2 012 590 candidats fils testés, avec zéro désaccord et une profondeur maximale
observée de 21. Leur détail reproductible est : fixtures 211 cas / 1 578
sommets / profondeur 7; générique 1 184 / 110 873 / 21; grille saturée
1 291 / 111 170 / 17; cosphérique 2 071 / 101 877 / 16.

Deux fuzz supplémentaires sur le même binaire Release, SHA-256
`0d5cbba3...`, rendent encore zéro désaccord :

```text
./build/v3/mhgp3v_flats_differential --clouds 60 --points 12 --coord 4 --smax 10 --seed 99173 --min-cases 1 --min-navigated 1 --min-vertices 1
./build/v3/mhgp3v_flats_differential --clouds 30 --points 14 --coord 64 --smax 10 --seed 77123 --min-cases 1 --min-navigated 1 --min-vertices 1
```

Ils cumulent 262 801 sorties reverse, avec des profondeurs 17 puis **23**. La
profondeur 21 des CTests et l'ancienne valeur publiée 18 ne sont donc que des
observations de campagnes, jamais des bornes.

Le claim exact est donc **absence de table globale de visitation dans la
décision du parcours**, pas encore mémoire de sortie bornée. Sept dettes restent.

1. L'endpoint retourne un `std::vector<Vertex> visited` et y pousse chaque
   sommet. Ce vecteur n'influence pas le parcours, mais matérialise encore
   $\Omega(V)$ objets et leurs payloads. Aucun high-water ne prouve un gain
   mémoire tant qu'un callback ou sink borné ne le remplace pas.
2. Chaque requête `neighbour_along` alloue un bitmap `seen_candidate` de taille
   $n$ et une liste `touched` de taille $O(n)$ au pire. Chaque frame de pile
   recopie aussi la coquille et l'intérieur du sommet. Même après streaming, la
   borne live est un scratch $O(n)$ plus la somme des tailles des sommets sur le
   chemin, pas seulement le nombre de niveaux; une coquille cosphérique peut
   être de taille $\Theta(n)$.
3. Le différentiel appelle `reverse_search_shallow` avec `index=nullptr`. Il
   qualifie donc les scans exhaustifs du pinceau, pas le chemin indexé qui sera
   nécessaire au produit.
4. `parent_of` confond « racine » et échec de `neighbour_along`; une absence de
   parent fait simplement ignorer le candidat sans changer `CloudStatus`. La
   comparaison au BFS détecte une troncature sur les nuages jugés, mais le futur
   endpoint autonome doit distinguer le cas racine certifié d'une erreur de
   primitive et échouer fermé.
5. Aucun plancher CTest ne porte directement sur `reverse_vertices`, profondeur
   ou enfants. La comparaison est forte lorsqu'elle s'exécute, mais une mutation
   qui retire tout le bloc de qualification n'est pas elle-même gravée par un
   test négatif. Si BFS et reverse rendent le même statut non `kOk`, le bloc
   saute aussi toute comparaison sans rougir. La variante device, les écritures
   de sink et la répartition de sous-arbres ne sont ni écrites ni mesurées.
6. L'équivariance permute et rejoue seulement `flat_catalogue`, donc le BFS;
   elle n'appelle jamais la reverse search. `interior.front()`, la base du germe
   et les fermetures sont ordonnés par les indices d'entrée. Le prototype suppose
   la canonicalisation `PointId` amont, mais sa porte ne grave pas encore
   l'équivariance de l'arbre ni de l'ordre des enfants.
7. `reverse_children_tested` compte les candidats qui atteignent le calcul de
   parent, pas le travail complet. Après avoir trouvé un enfant,
   `for_each_flat` continue d'énumérer les triplets restants et de reconstruire
   leurs fermetures : le `return` ne sort que du callback. Les voisins non
   bornés ou hors coupe ne sont pas inclus non plus dans le ratio publié. « Six
   parents par sommet » n'est donc ni une borne, ni une mesure totale des flats
   et prédicats exécutés.

Ces réserves ne remettent pas en cause le théorème de parent ni la parité bornée.
Elles bornent honnêtement la promotion : le **parcours décisionnel** est local;
son endpoint produit, sa mémoire et son intégration au catalogue restent
ouverts.

#### Delta live après `2c395d3`

Le worktree suivant, épinglé par
`order_k_flats.hpp=35f3d5108cf88f0c858b4f24d6ade3cc6777f991336da6323ce899228a9491f9`,
`flats_differential.cpp=3937261a076a389953038123265bfe5e5652fa99f3d8457c6dcc51e5c995fd01`
et `CMakeLists.txt=705e526c6266b35155bdc28229c340d8a23be121785758f5f049f4ef24123fec`, répond
positivement à plusieurs findings : arrêt réel de `for_each_flat` après le fils,
échec atomique si la requête du parent prouvé casse, compteurs de flats et de
requêtes, planchers reverse sur les portes générique/saturée et second rejeu par
l'API indexée. Les treize CTests restent verts. Avec les deux parcours par cas,
ils durent 154,38 s; la porte générique rend 110 873 sorties, profondeur 21,
737 895 flats canoniques, 665 099 requêtes de parent et zéro skip.

Le crédit ne ferme pas encore six résiduels.

1. `!canonical_parent` est toujours classé `kIsRoot` sans vérifier que le
   candidat égale le germe. Un non-germe dont la direction manque reste ignoré
   avec `kOk`; seule la panne du `next` est maintenant `kBroken`.
2. L'index est construit avec des feuilles de 16, tandis que tous les nuages
   permanents ont au plus 13 points et les fixtures au plus 9. Le rejeu indexé
   traverse donc une feuille unique. L'auto-test à 48 points ne juge que
   `closed_ball`, pas `box` ni `sign_disagreement` utilisés par le pinceau. Une
   campagne transitoire à vingt points est verte, mais n'est pas une porte.
3. La sortie indexée est projetée dans une `map` sans vérifier que sa taille
   égale le nombre de records. Des doublons indexés peuvent être masqués.
4. `reverse_skipped` est imprimé mais jamais exigé nul. Les planchers ne portent
   ni sur les sorties indexées, ni sur les nœuds internes visités, ni sur les
   requêtes de parent; fixtures et cosphérique n'ont pas de plancher reverse.
5. L'équivariance et le sink restent inchangés. L'endpoint matérialise toujours
   tous les `Vertex`, sa pile copie les coquilles et son scratch reste $O(n)$.
6. `reverse_flats_enumerated` ne compte que les flats canoniques parvenus au
   callback. Les triplets non canoniques dont la fermeture a déjà été
   reconstruite restent hors compteur; la queue multiplicitaire est encore
   sous-instrumentée.

Commande transitoire qui exerce réellement un arbre interne, zéro désaccord :

```text
./build/v3/mhgp3v_flats_differential --clouds 2 --points 20 --coord 40 --smax 4 --seed 20260809 --min-cases 1 --min-navigated 1 --min-vertices 1 --min-reverse 1
```

Elle rend 218 cas, 3 062 sorties reverse, profondeur 19, 20 043 flats et 16 070
requêtes de parent. Il faut la rendre permanente avec une feuille de taille
quatre et un compteur de nœuds **visités**, puis injecter doublon, faux germe et
skip non `kOk`.

### 7.9 Filtre de régularité du worktree après `969db5c`

Le delta worktree épinglé par
`first_incidence_dichotomy.cpp=45ad067227ac573e02b9193533dc07541d36223f53bbb0d28c5846a88e0d225a`
et `CMakeLists.txt=d9c07ce78b844c965c1aa43fc93cc98ff96418742bed7de01244005c439ee79d`
améliore réellement le diagnostic : provenance complète, séparation
`attachment_candidates`, compteurs d'égalités et supports, fixture u16 des trois
bras hors cœur, mode `--require-regular` et deux CTests dédiés. Les neuf CTests
de première incidence passent en 11,9 s; le positif régulier exerce 26 candidats.

Le mode n'est pourtant pas encore une autorité régulière. Le compteur
`outside_equalities` n'est incrémenté que si la boule fermée contient un outsider
**et** que `strict_intruders` est vide. Une égalité extérieure mixte à un intrus
strict est donc censurée. Fixture exacte à cinq points, $k=3$ :

```text
(3,9,13) (4,9,2) (10,2,14) (4,6,3) (1,9,11)
```

Pour $F=012$, la miniboule a le support unique essentiel `12`. Les signes exacts
des points 0 à 4 sont `-1,0,0,-1,0` : 3 est un intrus strict et 4 un outsider
sur la coquille. La coface `0123` est de Gabriel ouverte, avec 4 en extra-shell.
Le live rend néanmoins `outside_equalities=0`, `ambiguous_support=0`, zéro
désaccord et classerait cette campagne régulière. Toute égalité
`z not in F && sphere_side(B_F,z)==0` doit être comptée indépendamment des
intrus.

Le test des supports ne compare en outre que les sous-ensembles de la cardinalité
`facet_mb.n_support`. Il ne contrôle ni `shell(F)\setminus U_F=empty`, ni un
support essentiel alternatif d'une autre cardinalité. Exemple local : sur la
sphère de centre `(5,5,5)` et rayon 5, la paire antipodale
`(0,5,5),(10,5,5)` est un support positif, et le triangle
`(5,10,5),(5,2,9),(5,2,1)` en est un second, positif, de cardinalité trois. Un
énumérateur limité aux paires annonce une réalisation unique.

Enfin, `attachment_candidates` est incrémenté même si la miniboule de facette
échoue, et son plancher compte des candidats, pas des cibles validées. Toute
erreur de miniboule dans l'autorité doit rendre un statut distinct. La fixture
u16 permanente vérifie son cœur et ses trois bras; elle ne recertifie pas dans
le C++ toutes les obligations de régularité décrites par son commentaire.

Verdict : bon filtre et bonne fixture de réfutation du lookup brut; **NO-GO pour
nommer les candidats attaches autorisées** jusqu'à la fixture mixte, au contrôle
de toute la coquille hors support et au fail-closed des primitives.

### 7.10 Première exécution historique de la descente locale de carrier

Le worktree `first_incidence_dichotomy.cpp=da3d12d37fff2e88305bc52b3de0a98d1c735810db8fb5b2cf3807390a20ad4d`,
supersédé par le §7.11,
implémente le squelette du nouveau théorème : témoin initial
$W_0=T_F\cup\lbrace w_F\rbrace$, remplacement canonique de `support[0]` par le
plus petit intrus strict, comparaison stricte des niveaux et arrêt sur une clef
du cœur exhaustif. C'est une réponse constructive importante, pas seulement un
commentaire.

Les neuf CTests de première incidence passent. La porte appelée régulière
observe 26 descentes, quatre pas au total et une longueur maximale de deux; la
campagne irrégulière observe 39 descentes, huit pas et le même maximum. Aucune
non-baisse, aucun témoin trop haut et aucun terminal hors cœur ne sont observés.

Le différentiel reste toutefois diagnostic pour cinq raisons.

1. La descente s'exécute même hors porte régulière et ne réauthentifie sur aucun
   descendant le support unique positif essentiel, l'absence de membre de
   coquille hors support et les extra-shells. Une non-baisse hors domaine devient
   un désaccord scientifique au lieu d'un refus `unsupported`.
2. Le terminal ne sérialise que `truth_facets.find(current)`. Le témoin transporté
   et le théorème prouvent bien l'existence d'une coface directe sous le cutoff;
   il ne s'agit pas d'une erreur mathématique. Le reçu logiciel doit néanmoins
   engager cette coface — ou le groupe direct minimal de la branche vide — et
   son niveau $<a_F$, au lieu d'une appartenance au cœur tous niveaux.
3. Le cap `n*n+16` n'est pas démontré. La seule borne immédiate est
   combinatoire. Au domaine CLI $n\leq64$, il échoue fermé mais peut refuser une
   chaîne légitime; transplanté tel quel à 50 k, le produit `int*int` déborde.
   L'épuisement est confondu avec `terminal hors cœur` au lieu d'un statut budget.
4. Aucun plancher n'impose une étape, une branche terminale zéro/un, une longueur
   ou un budget. Supprimer le bloc peut laisser les CTests verts.
5. La fixture permanente à dix points vérifie le bras initial mais ne traverse
   pas ce bloc de descente. Elle doit forcer une chaîne non vide, puis les
   fixtures `E5` et sept points doivent graver la branche vide et les relais
   non directs.

Le résultat mathématique, lui, est positif et désormais isolé dans
[`NOTE_GATE_D_DESCENTE_LOCALE_CARRIER_ET_FRONTIERE_GLOBALE.md`](NOTE_GATE_D_DESCENTE_LOCALE_CARRIER_ET_FRONTIERE_GLOBALE.md) :
le locator non-cœur disparaît; seul `find_<a_F(R_F)>` dans la partition du cœur
reste global. Le prototype ne juge encore ni ce `find`, ni l'équivalence des
quotients, ni le fold.

### 7.11 Delta live `b0741d4e` : porte locale renforcée

Le worktree suivant répond directement à l'audit précédent :

| objet | SHA-256 |
| --- | --- |
| `prototype/first_incidence_dichotomy.cpp` | `b0741d4edcc9839ad4ab12bb58867b8c125fc83f9ab127708dcd15a91e640c17` |
| `CMakeLists.txt` | `c148e5c59ebd36c5bbcaaa0298752eb6357b943a35ed15924364f287a407d48b` |

Crédit complet : égalité extérieure mixte, supports essentiels tous cardinaux,
coquille hors support, réauthentification des descendants, statuts de descente,
fixtures `E5` et sept points, reçu direct sous cutoff et planchers des deux
terminaux sont écrits. Le delta suivant rend en outre les échecs de primitive
fail-closed dans le contrôle de support et la descente, ne soustrait plus les
refus au plancher et interdit refus ou panne sous `--require-regular`. Les dix
CTests de première incidence passent en 3,41 s sur un build frais.
La campagne à quatorze points rend 26 descentes, quatre pas, maximum deux, neuf
terminaux sans intrus, dix-sept avec un intrus, 26 reçus et zéro désaccord.

Cette réponse constructive ne ferme pas encore l'autorité pour cinq raisons.

1. La porte globale du théorème d'attache concerne aussi les objets silencieux
   omis. Le binaire contrôle les facettes cœur et les chaînes choisies, pas tout
   le plateau. La fixture dix points ne rejoue pas elle-même les 120 triplets et
   210 quadruplets annoncés.
2. Le type `Receipt` matérialise désormais le terminal, la chaîne courte, la
   branche, la coface directe et son niveau dans les fixtures `E5` et sept
   points. Les campagnes ordinaires appellent toutefois `descend_to_core` avec
   `receipt=nullptr` : elles ne gardent que des compteurs. Surtout, la coface est
   cherchée dans `truth_direct`, carte exhaustive globale; ce reçu juge le
   théorème mais n'est pas le chemin produit. L'objet n'est pas remis à zéro à
   l'entrée et la fonction le remplit même après un terminal hors cœur, une
   coface absente ou un niveau trop tardif : un appelant qui réutilise un reçu
   peut donc observer un payload périmé ou invalide. Il faut un statut typé, un
   objet temporaire vierge et une affectation unique après toutes les portes.
3. `truth_direct`, `gabriel_open` et la force brute partagent encore
   `miniball_of` et `sphere_side`; une panne de miniboule peut devenir
   silencieusement « non-Gabriel » dans la construction de la source ou être
   sautée dans la force brute.
4. Il n'existe toujours ni `find_<a_F(R_F)>`, ni fold, ni comparaison aux
   coupes, couvertures et $q_R$. La source et les groupements sont matérialisés
   en maps après `flat_catalogue(s_max=n)`.
5. Le cap saturé à $2^{40}$ est un budget d'échec fermé, pas la borne
   combinatoire complète lorsque $\binom{n}{k}>2^{40}$, et encore moins une
   borne SLO.

Verdict : **GO pour le falsificateur local renforcé; NO-GO pour nommer l'attache
autorisée, le resolver, Gate D complète ou le chemin 50 k.**

### 7.12 Delta reverse live `67e562a7` : les objections retirées et celles qui restent

Le snapshot suivant répond directement aux six résiduels du §7.8 :

| objet | SHA-256 |
| --- | --- |
| `prototype/order_k_flats.hpp` | `67e562a75856f08463779b02a8bbe298c56c436afa553167b4079c4dc5877398` |
| `prototype/flats_differential.cpp` | `74b731152cee18047d52cb6f6013ee30dd6ca6de13e3bdc9f64afb00fc84e5e1` |
| `CMakeLists.txt` | `c148e5c59ebd36c5bbcaaa0298752eb6357b943a35ed15924364f287a407d48b` |

Les corrections sont réelles : un sommet sans direction n'est racine que si sa
coquille est celle du germe; sinon la sortie devient atomiquement
`kInvariantViolated`. Le replay indexé emploie des feuilles de quatre et une
porte permanente à vingt points; les doublons indexés, les skips, les sorties
indexées, les requêtes de parent et les visites internes sont gardés par des
postconditions. Les triplets balayés et les fermetures reconstruites sont
comptés intégralement. `box` et le désaccord ternaire ont un auto-test interne,
et l'équivariance appelle enfin la reverse search.

Le build frais et les 24 CTests `mhgp3v_flats_*` ou
`mhgp3v_first_incidence_*` passent en 135,56 s. Deux portes reverse utiles :

- fixtures : 211 cas, 1 578 sorties indexées et non indexées, profondeur 7,
  7 981 requêtes de parent, zéro skip;
- arbre indexé : 218 cas, 3 062 sorties de chaque chemin, profondeur 19,
  16 070 requêtes de parent, 89 045 triplets et autant de fermetures, zéro
  désaccord.

Les anciens findings « fausse racine », « feuille unique », « doublon masqué »,
« skip sans plancher », « queue de triplets non mesurée » et « équivariance BFS
seulement » sont donc **fermés sur ce snapshot**. Quatre réserves demeurent.

1. `reverse_search_shallow` retourne encore un `std::vector<Vertex>` contenant
   tous les sommets. La pile copie coquille et intérieur, et chaque voisin garde
   un bitmap/listes de scratch $O(n)$. Le catalogue appelle toujours le BFS : il
   n'existe ni sink, ni high-water, ni gain mémoire produit.
2. Les compteurs prouvent la traversée de nœuds internes. La preuve d'élagage
   vient d'une requête synthétique `box`, pas d'une postcondition propre aux
   requêtes `box`/désaccord effectivement lancées par la reverse search.
   `CertifiedIndex::build` ne remet en outre pas ses compteurs à zéro lorsqu'un
   objet est rebâti.
3. L'équivariance projette la sortie reverse dans un `set`; elle peut donc
   masquer un doublon propre à une permutation et ne transporte pas l'ensemble
   intérieur. La porte identité voit les doublons, pas chaque permutation.
4. Les branches fail-closed nouvelles n'ont pas encore de mutations ciblées
   « non-germe sans parent » et « doublon indexé ». Aucun kernel, sink ou
   partage device n'est écrit.

### 7.13 Delta sink live `e641518d` : sortie streamée, mémoire complète non mesurée

Le worktree poursuit immédiatement le snapshot précédent :

| objet | SHA-256 |
| --- | --- |
| `prototype/order_k_flats.hpp` | `e641518d0e7dd40296d4126fa7c6f23c3bb9811157e1b787f0ceda352df0a541` |
| `prototype/flats_differential.cpp` | `ef19fa735b09120c456b43599258b08510066d61bb9d8cbef60b6ab50a619a78` |
| `CMakeLists.txt` | `c148e5c59ebd36c5bbcaaa0298752eb6357b943a35ed15924364f287a407d48b` |

Crédit : `reverse_search_stream` appelle un consommateur sommet par sommet et
ne conserve plus la sortie complète. L'ancienne API devient une simple enveloppe
qui accumule ces callbacks pour l'oracle. Le sink peut interrompre le parcours,
et le différentiel vérifie un statut non `kOk`. Un rejeu frais de la porte
indexée et des fixtures est vert. La commande diagnostique à cinq nuages publie
5 400 sommets, 85 identifiants portés au maximum par les sommets du chemin,
109 interruptions, zéro désaccord détecté et aucun skip.

Cette fermeture retire bien le vecteur $\Omega(V)$ du **producteur streamé**,
mais sept limites empêchent de convertir le chiffre 85 en claim mémoire produit.

1. Le high-water additionne seulement `shell.size()+interior.size()` sur les
   frames actives. Il exclut capacité de pile, racine, candidat, parent, flat et
   fermeture temporaires, bitmap `seen_candidate`, listes `touched`, index,
   allocateur et sink. Il mesure des identifiants, pas des octets. Le ratio
   `5400/85` compare des records émis à des identifiants actifs; ce n'est pas un
   facteur mémoire.
2. La parité du consommateur est réduite à un compte et à une somme de hachages
   64 bits. Une collision est possible : cette porte est un bon falsificateur,
   pas une égalité exacte. Puisque la sortie oracle matérialisée existe déjà, un
   curseur qui compare chaque callback au record attendu donnerait une porte
   exacte avec $O(1)$ mémoire additionnelle.
3. Le sink de parité est appelé sans lui passer le `CertifiedIndex`. La
   composition `stream + index`, qui est le chemin produit visé, n'a donc aucune
   porte permanente.
4. Les 109 interruptions refusent toutes le **germe** : la lambda de test rend
   `false` au premier callback. L'arrêt après un préfixe et la branche
   `interrupted` prise depuis un enfant ne sont pas exercés.
5. Un callback peut avoir publié un préfixe avant qu'un parent cassé ou un arrêt
   volontaire ne rende le statut rouge. L'intégration doit écrire dans un segment
   non committé, puis engager seulement après statut final `kOk`; sinon le sink
   transforme une erreur fail-closed interne en sortie externe partielle.
6. `kInvariantViolated` confond une contradiction scientifique et l'arrêt demandé
   par le consommateur. Un résultat typé `complete / cancelled / budget /
   invariant` est nécessaire avant un reçu produit.
7. Le catalogue appelle toujours le BFS. L'équivariance par `set`, le high-water
   mémoire complet, l'élagage propre aux requêtes reverse et la forme device
   restent ceux du §7.12.

### 7.14 Parent au premier admissible, commit `35a8d01`

Le commit `35a8d0116893d949450289758847c8a0559ec084` est épinglé par :

| objet | SHA-256 |
| --- | --- |
| `prototype/order_k_flats.hpp` | `b9718460232eaca054102a5d906bafcf9c946f576e919b1928ef1c81da52df24` |
| `prototype/flats_differential.cpp` | `bf9b0fc34ebe9cb7c5caf6a4cb5fbc6bb29b45fc9c2d856bff3644320445df5` |
| `CMakeLists.txt` | `c148e5c59ebd36c5bbcaaa0298752eb6357b943a35ed15924364f287a407d48b` |

La simplification est mathématiquement valide. La preuve du parent n'exige pas
le minimum du tuple : toute direction admissible améliore strictement le
potentiel, et tout sommet non-germe en possède une. Le premier admissible dans un
ordre déterministe donne donc encore une fonction, une racine unique et aucun
cycle. Sous points distincts et coquilles triées, trois points d'une sphère non
nulle ne sont pas collinéaires; la base canonique d'un flat est alors formée de
ses trois plus petits identifiants. L'ordre des triplets canoniques est bien
l'ordre lexicographique des fermetures, et la direction négative précède la
positive : le premier admissible coïncide aussi avec l'ancien minimum.

Le différentiel compare le balayage précoce et complet sur chaque sommet accepté.
Le commit publie 50/50 CTests verts et, sur sa campagne générique, une baisse de
335 314 à 171 856 fermetures reconstruites. Une relance locale postérieure des
cinq portes flats, sur le wrapper encore compatible, passe 5/5 en 60,48 s.

Ce gain ne ferme ni le pire cas ni la source directe. Une racine ou un parent
admissible tardif peut encore parcourir tous les flats; une coquille de taille
$m$ garde jusqu'à $\binom{m}{3}$ triplets candidats. Surtout, la source de
sphères doit récolter **tous** les flats incidents, y compris ceux qu'aucune
direction de parent n'admet. La fixture `ABCpy` de
[`NOTE_GATE_D_SOURCE_DIRECTE_DEPUIS_SPHERES_CERTIFIEES.md`](NOTE_GATE_D_SOURCE_DIRECTE_DEPUIS_SPHERES_CERTIFIEES.md)
réfute explicitement toute source branchée seulement après ce premier filtre.

### 7.15 Snapshots de curseur `234ae9ef` puis `26c66aa3` : toujours non intégrés

Un premier header worktree transitoire
`order_k_flats.hpp=234ae9ef937040ddc8b1e185264eab8c25820354cadd6d32a518180affceae2f`
ajoute `for_each_flat_from(i0,j0,k0)`, mais la reverse search ne l'appelle pas
encore. Les cinq portes flats restent vertes parce que le wrapper historique
repart de `(0,1,2)`; elles ne mesurent aucun gain de reprise.

Ce delta a d'abord disparu avant commit. Au rehash suivant, le worktree courant
`order_k_flats.hpp=26c66aa301cb3803bfff8d0104b36e7ba34de3e265461a155695a2c3a19a032e`
réintroduit le helper et des commentaires de mesure, mais conserve exactement la
même frontière : `reverse_search_stream` appelle `for_each_flat`, et ce wrapper
appelle toujours `for_each_flat_from(...,0,1,2,...)`. Aucun frame ne stocke ni ne
réinjecte un curseur. Le chemin live ne peut donc pas bénéficier de la reprise;
les nombres 308 832/308 832 restent le journal d'un essai transitoire, pas une
capability qualifiée par le code et les CTests présents.

Le contrat du curseur est en outre ambigu au snapshot épinglé. Le callback reçoit
le triplet **courant**, puis un retour `false` quitte la fonction avant
l'incrément. Réappeler `for_each_flat_from` avec ce triplet le rejoue. Il faut
publier le successeur canonique, ou graver explicitement une convention
`last_consumed` avec avancement contrôlé et tests aux frontières. Les mutations
doivent couvrir premier/dernier triplet, triplet collinéaire, fermeture non
canonique, arrêt puis reprise après chaque position, et concaténation exacte des
segments contre un balayage intégral.

Verdict : **GO pour la simplification du parent; diagnostic seulement pour le
curseur tant qu'il n'est ni consommé ni différencié.** Le HEAD `a5adde7` ne
contient aucun curseur; le helper du worktree `26c66aa3` n'est pas encore relié à
la reverse search.

### 7.16 Porte de reprise courante

1. Intégrer le sink au catalogue derrière un segment transactionnel et un statut
   typé; mesurer son high-water complet en octets. Remplacer le digest 64 bits par
   une comparaison exacte et ajouter les mutations fail-closed.
2. Implémenter et différencier « support canonique puis owner » avant de retirer
   `emitted`. Récolter tous les flats incidents, pas seulement les directions de
   parent; qualifier le curseur de reprise sans omission ni répétition.
3. Étendre la porte d'équivariance au payload intérieur et aux multiplicités de
   sortie, en conservant le BFS comme oracle borné.
4. Transformer l'oracle ouvert maintenant corrigé en source terminale sans
   `flat_catalogue(s_max=n)`, puis composer l'autorité de fenêtre avec le census
   saturé et la descente locale de carrier. La porte régulière globale doit
   couvrir les objets silencieux, et chaque échec de primitive dans la source ou
   la vérité doit être distingué d'une décision « non-Gabriel ».
5. Construire et différencier le fold pré-lot typé
   $R^{-}\sqcup L^{-}\sqcup N_a$, puis les runs, la couverture et la jointure
   verticale avant toute mesure du contrat 50 k.

Ce chemin reste CPU-only tant qu'aucun kernel CUDA de reverse search et aucun
pipeline aval device n'existent. Aucune G4 ne doit être utilisée pour exécuter
les CTests ou les harnais CPU ci-dessus.

GCP non utilisé.
