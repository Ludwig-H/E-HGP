# Audit continu de `order_k_flats` — de `2532fd5` à `1a0a1f8` et Gate D

Date : 9 août 2026 UTC.

> [!IMPORTANT]
> **Verdict courant : GO ciblé pour le germe, les gardes u16, l'index k-d exact et la règle mathématique de parent; NO-GO inchangé pour une promotion exacte, le domaine multiplicitaire produit et le contrat 50 k.** Le commit `1a0a1f8` ferme le P0 d'élagage par classifications boîte--boule entières et retire les scans systématiques des singletons, du census et du pinceau. Le commit `6fa7e9d` juge un parent local multiplicitaire, renforcé dans le worktree courant. Restent ouverts l'implémentation live du propriétaire complet, la reverse search effective, le producteur terminal `directes + premières incidences`, les lots, l'état horizontal, les verticales et le contrat de sortie.

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
`6fa7e9d`; elles sont conservées comme provenance. Le worktree courant ajoute le
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
| propriétaire et déduplication | **ouvert** | live : premier préfiltre seulement, puis `emitted`; seconde inclusion, cône signé et support canonique avant owner non implémentés |
| parent local multiplicitaire | **prouvé et jugé, non substitué** | live calcule le parent depuis le BFS; reverse search garde `seen/frontier/visited` |
| source HGP complète | **théorème conditionnel fermé, produit ouvert** | `directes + M(F)` suffit horizontalement et $M(F)$ se factorise sans étoile; source directe terminale, capability commune, lots, couverture et verticales absents |
| profils produit | **ouverts** | doublons refusés, dyadique absent, oracle encore relatif aux primitives v2 |

Le P0 de l'ancien §4.6 est donc fermé au commit `1a0a1f8`. Les phrases du §5
affirmant encore un census global pour chaque singleton, chaque direction et
chaque émission sont historiques et désormais fausses. Le NO-GO 50 k demeure,
mais pour les raisons actuelles : volume $V_k$ sans borne utile au SLO,
énumération des flats et queue lourde des coquilles, index sans borne
sous-linéaire universelle, parcours encore résident, propriétaire incomplet et
pipeline HGP aval absent.

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
locale dans le cône signé de $P_U$. Le cube u16 possède quatre supports
antipodaux de la même boule : appliquer seulement un owner par support émettrait
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

### 7.7 Porte de reprise courante

1. Fermer le contrat de l'out-paramètre parent, comparer la couverture au BFS
   certifié et graver l'adjacence orientée au premier événement.
2. Implémenter et différencier « support canonique puis owner » avant de retirer
   `emitted`.
3. Remplacer le BFS par la reverse search en conservant le BFS comme oracle
   borné; publier profondeur, enfants, flats et high-water.
4. Corriger la source directe du prototype de première incidence : extra-shell
   développée ou refusée, univers de facettes jugé indépendamment, puis
   capability terminale commune avec $M(F)$ et l'autorité de fenêtre.
5. Construire runs, fermeture des ex æquo, locator horizontal, couverture et
   jointure verticale avant toute mesure du contrat 50 k.

Ce chemin reste CPU-only tant qu'aucun kernel CUDA de reverse search et aucun
pipeline aval device n'existent. Aucune G4 ne doit être utilisée pour exécuter
les CTests ou les harnais CPU ci-dessus.

GCP non utilisé.
