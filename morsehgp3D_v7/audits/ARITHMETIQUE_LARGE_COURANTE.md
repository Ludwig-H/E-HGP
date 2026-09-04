# Produits larges, niveaux et réductions : contrelecture courante

4 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

Les preuves des §§ 5–7 de [ARITHMETIQUE_PRIMITIVES.md](../docs/ARITHMETIQUE_PRIMITIVES.md#5-produits-larges--preuve-des-colonnes-écrites) concordent avec les expressions écrites : aucune erreur de borne des colonnes, de cast PGCD ou de correction de quotient n'a été trouvée dans leur domaine déclaré. Le [plan de portes](../docs/PLAN_PORTES_ARITHMETIQUES.md#4-u192u320-et-comparateurs--séparer-les-domaines) distingue correctement les limites de capacité génériques et les niveaux issus des lanes u16. Les exécutions C++ et mutants restent à qualifier séparément.

Cette contrelecture ajoute une comparaison minimale de deux niveaux numériques, avec le **premier bit de U320.w[4] non nul**, qui isole le second site de `level-trunc-hi`. Le [reçu](receipts_iteration3/wide_static_current.json) contient les 13 hashes stables et les calculs reproductibles en entiers Python. Aucun compilateur, programme C++ ni mutant produit n'a été exécuté pour cette note.

## 1. Colonnes : tous les intermédiaires sont bornés

Poser $R=2^{64}$. Un produit de deux chiffres est au plus $(R-1)^2=R^2-2R+1$, son chiffre haut au plus $R-2$ et son chiffre bas au plus $R-1$. Les casts, shifts et additions de [wide.hpp](../src/core/wide.hpp#L27) sont cohérents avec les bornes suivantes.

| Expression | Majorant | Type réellement utilisé |
| --- | --- | --- |
| `mid` du produit U192 | $3R-4$, report au plus 2 | u128 |
| Expression avant le cast de `U192.w[2]` | $2+(R-2)+(R-2)+(R^2-2R+1)=R^2-1$ | u128, y compris `p11` entier |
| Premier accumulateur U320 | $3R-4$, report au plus 2 | u128 |
| Deuxième accumulateur U320 | $4R-4$, report au plus 3 | u128 |
| Troisième accumulateur U320 | $3R-2$, report au plus 2 | u128 |
| Expression avant le cast de `U320.w[4]` | Au plus R par les bornes séparées ; strictement moins que R par la capacité totale | u128 avant cast u64 |

La dernière ligne demande les deux arguments, sans les confondre. Le majorant lâche R exclut déjà tout débordement de l'addition u128. L'identité des colonnes établit ensuite que cette expression est le quotient du produit total par $R^4$. Pour $n<R^3$ et $d<R^2$, le produit est inférieur à $R^5$ : ce quotient est donc inférieur à R et le cast u64 est exact. Les bornes séparées ne sont pas toutes simultanément atteignables ; il serait incorrect de conclure à une perte possible du dernier chiffre en utilisant seulement le majorant R.

Pour U192, le développement des colonnes donne toujours les trois mots bas, sans débordement unsigned intermédiaire. Leur égalité avec **tout** le produit exige en plus $xy<R^3$. Le helper n'émet aucun refus quand cette précondition manque. Pour U320, la capacité du résultat découle déjà des types d'entrée ; aucune restriction supplémentaire sur leurs valeurs n'est nécessaire.

## 2. Somme de carrés : la capacité collective protège aussi les additions u64

La [somme de trois carrés](../src/core/wide.hpp#L78) requiert $a^2+b^2+c^2<R^3$. Cette borne collective implique celle de chaque carré et de toute somme partielle, puisque les termes sont non négatifs. Les accumulateurs bas et milieu sont au plus $2R-2$ et $2R-1$ ; leurs reports sont au plus un.

Le point délicat est `r.w[2] += sq.w[2] + carry`, où l'addition de droite s'effectue en u64. Si cette addition atteignait R, ou si son ajout à `r.w[2]` atteignait R, la somme partielle complète atteindrait déjà $R^3$. La précondition collective exclut donc les deux wraps. Vérifier seulement chaque carré ne suffit pas : pour $a=b=3\times2^{94}$ et $c=0$, chacun vaut $9\times2^{188}<2^{192}$, mais leur somme vaut $18\times2^{188}>2^{192}$.

Le cas valide proposé, trois fois $(2^{95}-1)^2$, comporte effectivement un report non nul entre mots milieu et haut. Le calcul Python donne les mots de somme `[3, 18446744060824649728, 13835058055282163711]`. Ces calculs qualifient les valeurs attendues de la future fixture, pas l'exécution de la primitive C++.

## 3. Comparateurs : trois domaines distincts

| Appel | Domaine suffisant | Conséquence sur les mots hauts |
| --- | --- | --- |
| `mul_192x128_320` | Toutes entrées U192 et u128 | Produit $<2^{320}$ ; tous les bits du cinquième mot sont possibles |
| `compare_exact_level` | Numérateurs U192, dénominateurs i128 strictement positifs | Croisements $<2^{319}$ ; `w[4]` peut être non nul, mais son bit 63 est nul |
| Niveaux construits directement par les lanes u16 | Bornes Cramer du grand-livre : num q4 $<2^{144}$, den q4 $<2^{108}$ ; autres lanes plus petites | Croisements $<2^{252}$ ; `w[4]` est toujours nul |

La troisième ligne est conditionnée aux bornes des formes u16 relues séparément ; elle ne suppose pas que tout `ExactLevel` admissible à l'API ait une origine géométrique. Les comparateurs lisent les mots du haut vers le bas et comparent exactement les entiers représentés. Leur petite boucle descend jusqu'à -1 dans le type `int`, sans difficulté de largeur.

[compare_rational](../src/lanes/level.hpp#L32) a une autre précondition : outre `num>=0` et `den>0`, ses deux produits croisés doivent tenir en U192. Des i128 positifs arbitraires ne la garantissent pas. Exemple hors domaine : $x=2^{100}/1$ et $y=0/2^{100}$ ont un produit gauche $2^{200}$, perdu par la troncature U192. Le validateur de fixture doit refuser cet appel ; il ne s'agit ni d'un refus actuellement rendu par le helper, ni d'un défaut atteint sur le domaine q2/q3 annoncé.

[ExactLevel::operator==](../src/lanes/level.hpp#L40) compare une représentation ; `same_exact_level` compare la valeur rationnelle. Les cas `1/2` contre `2/4` et `0/1` contre `0/7` doivent donc garder deux verdicts distincts. `promote_level` ne transporte correctement que les numérateurs i128 non négatifs ; la réduction signée d'un rationnel n'autorise pas ensuite son passage à un comparateur unsigned.

## 4. Deux sites de mutant, deux fixtures causales

Le nom `level-trunc-hi` efface [U192.w[2]](../src/core/wide.hpp#L39) **et** [U320.w[4]](../src/core/wide.hpp#L61). Une future porte doit publier séparément la non-vacuité de chacun. Les numérateurs du test U320 ci-dessous sont des littéraux : aucune multiplication U192 ni somme de carrés ne les construit, donc le premier site n'intervient pas.

```cpp
const ExactLevel x{{0, 0, 4}, 1};
const ExactLevel y{{1, 0, 0}, (i128)1 << 126};
```

Ces deux niveaux représentent $x=2^{130}/1$ et $y=1/2^{126}$. Leurs dénominateurs sont positifs et représentables. Les produits croisés, de bas en haut, sont :

| Produit | Valeur exacte | Mots U320 |
| --- | --- | --- |
| Gauche | $2^{256}$ | `[0,0,0,0,1]` |
| Droit | 1 | `[1,0,0,0,0]` |

Le verdict exact est +1. Effacer seulement le cinquième mot transforme le produit gauche en zéro et impose −1. C'est une fixture de deux enregistrements atteignant le premier bit du mot recherché ; elle ne prétend pas minimiser toutes les valeurs d'entrée, ni représenter des boules u16. Le calcul Python confirme ces littéraux et cette inversion **modélisée**, sans prétendre avoir tué le mutant C++.

Pour le premier site, $2^{64}\times2^{64}=2^{128}$ donne directement U192 `[0,0,1]`. Pour les retenues complètes U320, le plan propose utilement $(R^3-1)(R^2-1)=R^5-R^3-R^2+1$, soit `[1,0,R-1,R-2,R-1]`, confirmé par le juge Python. Ces cas complètent le test de simple inversion ; ils ne s'y substituent pas.

La lecture du CMake courant ne trouve ni inscription de `level-trunc-hi`, ni cible `mhgp7_level_cmp`. Les produits du [selftest arithmétique](../tests/selftest.cpp#L54) sont jugés par commutativité et monotonie. Son produit U320 maximal dans cette séquence a 192 bits et `w[4]=0` ; il ne qualifie donc pas le second site. Une réussite d'un oracle géométrique qui construit d'abord un carré U192 ne comblerait pas non plus cette obligation.

## 5. PGCD et division : clôture sous les préconditions nommées

[uabs128](../src/core/intmath.hpp#L49) est sûr sur tout i128 : dans la branche négative, `v+1` puis sa négation sont représentables ; le dernier +1 est unsigned. Il rend ainsi $2^{127}$ pour `INT128_MIN`. Euclide n'évalue jamais `%` avec un diviseur nul. Chaque reste est strictement inférieur au diviseur précédent, ce qui assure la terminaison. Dans [ugcd128](../src/core/intmath.hpp#L63), le passage aux mots de 64 bits se fait avec $1\leq y<R$ ; le reste $x\bmod y$ est lui aussi inférieur à R. Les deux casts sont exacts, y compris après un x initial occupant tous les 128 bits.

Pour `ball_key_reduce`, A>0 impose $1\leq g\leq A\leq\mathrm{INT128MAX}$. Pour `rational_reduce`, den>0 impose la même borne avec den. Les casts de g en i128 et divisions signées sont donc définis, même si un coefficient ou un numérateur vaut `INT128_MIN`. Les divisions se font par un entier positif ; le cas interdit −1 ne se présente pas. Ce contrat de réduction est plus large que celui d'évaluation d'une puissance de boule.

Le commentaire de [floor_div128](../src/core/intmath.hpp#L76) doit nommer les deux exclusions : den=0 et `(INT128_MIN,-1)`. Hors ces cas, le quotient tronqué C++ existe. Une correction est nécessaire exactement pour un reste non nul et des signes opposés. Le décrément ne peut sous-déborder : obtenir déjà `INT128_MIN` dans le domaine admis impose une division exacte, donc aucun décrément. Les appels courants d'AxisBounds, avec numérateur de module inférieur à $2^{87}$ et dénominateur positif inférieur à $2^{69}$, satisfont ces exclusions. Le plan a raison de valider les entrées interdites sans les appeler en C++.

## 6. Reproduction et suite constructive bornée

```bash
PYTHONDONTWRITEBYTECODE=1 python3 morsehgp3D_v7/audits/receipts_iteration3/wide_integer_fixture.py
```

Cette [fixture indépendante](receipts_iteration3/wide_integer_fixture.py) utilise multiplication, découpage en mots, PGCD et division d'entiers Python de taille arbitraire ; elle ne recopie pas les produits par colonnes du C++. Le reçu conserve stdout, code 0 et hash du script. Les dix calculs ont aussi été rejoués avec succès sous [Python optimisé](receipts_iteration3/wide_fixture_optimized.json). Les contrôles utilisent des exceptions explicites, aucun `assert`.

La prochaine petite porte peut reprendre les littéraux, appeler les primitives réellement compilées, puis exercer chaque site de mutant avec son plancher propre. Les rejets de domaines doivent rester attribués au validateur de fixture tant que les helpers ne rendent pas eux-mêmes de statut. Une campagne exhaustive ou aléatoire ne remplace pas les preuves universelles d'intermédiaires ci-dessus ; une preuve sur le code source ne certifie pas à elle seule le binaire, le compilateur ou toutes les chaînes de publication. Aucun code produit modifié. GCP non utilisé.
