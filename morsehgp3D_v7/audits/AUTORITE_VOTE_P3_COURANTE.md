# Autorité numérique du vote p3 : égalités exactes et intervalles bornés

5 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`. Mathématique et petite fixture Python seulement pendant la fenêtre constructeur F ; aucun moteur, build ou benchmark exécuté.

**Le vote p3 admet une décision exacte sans factorisation des radicandes.** Les égalités se reconnaissent par regroupement en classes de carrés rationnels ; les comparaisons non nulles utilisent des intervalles rationnels construits par racine carrée entière. Une exécution plafonnée peut rendre `indeterminate`, mais ne transforme jamais un intervalle contenant zéro en égalité. Cette note ferme la méthode mathématique et son juge borné d'audit ; elle ne livre pas encore un consommateur produit.

La portée est le choix d'une étiquette à partir des histogrammes certifiés du [contrat masses/vote](CONTRAT_MASSES_VOTE_COURANT.md). L'univers de cofaces, la multiplicité des incidences et le support restent ceux de ce contrat. La méthode ne reconstitue pas une incidence absente du payload.

## 1. Réduire la comparaison à une somme de racines

Pour un point x et une classe c, agréger les multiplicités des facettes incidentes portant cette classe. Le numérateur du vote est :

$$U_x(c)=\sum_\lambda h_{x,c,\lambda}\lambda^{-3/2},\qquad h_{x,c,\lambda}=\sum_{\substack{\tau\ni x\\\ell(\tau)=c}}H_{\tau,\lambda}.$$

Pour `T_x>0`, comparer V revient à comparer U, puisque le dénominateur T est commun. Une comparaison de deux classes donne des coefficients entiers signés `d_lambda=h_left−h_right`. Les niveaux λ sont positifs et rationnels ; écrire `λ=a/b` en fraction réduite avec `a,b>0` :

$$\lambda^{-3/2}=\frac{b}{a^2}\sqrt{ab},\qquad D=U_x(c)-U_x(c')=\sum_\lambda d_\lambda\frac{b_\lambda}{a_\lambda^2}\sqrt{a_\lambda b_\lambda}.$$

La transformation n'utilise pas le rayon approximé. Par exemple, `λ=4` contribue `1/8`, et non `1/64`. Tous les coefficients sont rationnels et tous les radicandes sont des entiers positifs. Agréger d'abord les niveaux rationnels égaux évite du travail ; cela ne remplace pas le regroupement des radicandes.

## 2. Classes de carrés sans factorisation

Pour deux radicandes N et M, poser `g=gcd(N,M)`, `n=N/g`, `m=M/g`. Comme n et m sont premiers entre eux :

$$\frac{N}{M}\in\mathbb{Q}^{\times2}\quad\Longleftrightarrow\quad n=s^2\ \text{et}\ m=t^2\ \text{pour des entiers}\ s,t>0.$$

Le sens direct vient de la parité des valuations dans une fraction réduite ; le réciproque est immédiat. Deux appels à `isqrt` et deux égalités de carrés vérifient la condition. Lorsqu'elle vaut :

$$\sqrt{N}=\frac{s}{t}\sqrt{M}.$$

Les coefficients s'agrègent donc exactement devant un représentant de chaque classe. Une factorisation en parties sans carré serait également correcte, mais n'est pas nécessaire. Le juge prend les radicandes dans l'ordre croissant et conserve un représentant de chaque classe ; ce choix est déterministe pour les permutations d'une même entrée. Il ne prétend pas fournir une forme universelle sans carré indépendante de tous les jeux de représentants possibles.

Une racine entière est absorbée dans le coefficient rationnel, devant `√1`. Les représentants restants désignent des classes distinctes dans le groupe multiplicatif des rationnels non nuls modulo leurs carrés.

Exemple ne se réduisant pas à la divisibilité : `λ=9/2` et `λ=25/2` donnent les radicandes 18 et 50. Leur rapport réduit est `25/9`, donc les racines diffèrent du facteur rationnel `5/3`. Les contributions `27·(9/2)^(-3/2)` et `125·(25/2)^(-3/2)` sont égales, sans calcul de racine réelle ni factorisation.

## 3. Pourquoi le test d'égalité est complet

**Lemme.** Des racines de rationnels positifs appartenant à des classes de carrés distinctes sont linéairement indépendantes sur les rationnels.

Voici une preuve complète adaptée au besoin, sans algorithme de factorisation. Prendre une base `d_1,…,d_r` du sous-groupe fini engendré par les classes des radicandes, vu comme espace vectoriel sur le corps à deux éléments. Soit `E_j=Q(√d_1,…,√d_j)`. Prouver ensemble, par récurrence :

1. Les `2^j` produits obtenus en prenant chaque racine zéro ou une fois forment une base de E_j sur Q.
2. Si `a∈Q`, `a≠0` et un élément de E_j a pour carré a, la classe de a est engendrée par celles de `d_1,…,d_j`.

À j=0, les deux affirmations sont immédiates. La deuxième affirmation au rang j implique que `d_{j+1}` n'est pas un carré dans E_j, par indépendance des classes. L'extension suivante est donc quadratique, de base `1,√d_{j+1}` sur E_j, ce qui prouve la première affirmation au rang suivant.

Pour la seconde, écrire un élément de carré rationnel sous la forme `z=u+v√d_{j+1}`, avec `u,v∈E_j`. Dans `z²=a`, le coefficient de `√d_{j+1}` est `2uv=0`. Un corps n'a pas de diviseur de zéro : soit v=0, et `u²=a` ; soit u=0, et `v²=a/d_{j+1}`. Dans les deux cas l'hypothèse de récurrence donne exactement la classe annoncée. La récurrence est fermée.

Chaque radicande original est maintenant un carré rationnel multiplié par un produit distinct des d_i. Sa racine est un multiple rationnel d'un monôme de la base ci-dessus. Des classes distinctes donnent des monômes distincts, donc sont linéairement indépendantes. Aucune étape de la procédure d'audit n'a besoin de calculer cette base abstraite ; elle sert à la preuve.

Après regroupement, le critère d'égalité est ainsi nécessaire et suffisant :

$$D=\sum_{i=1}^{g}c_i\sqrt{N_i}=0\quad\Longleftrightarrow\quad c_1=\cdots=c_g=0.$$

Les coefficients sont des fractions exactes. Une égalité irrationnelle peut donc être décidée **avant tout raffinement d'intervalle**, même si le budget de raffinement vaut zéro. Si le regroupement s'arrête sur une limite, aucune non-égalité n'est déduite d'un regroupement incomplet.

## 4. Signe certifié d'une somme non nulle

À une précision entière p≥0, calculer `q_i=isqrt(N_i·2^(2p))`. Alors :

$$L_i=\frac{q_i}{2^p}\leq\sqrt{N_i}\leq U_i=\frac{q_i+1}{2^p}.$$

Si le radicande mis à l'échelle est un carré exact, prendre `U_i=L_i`. Pour un coefficient positif, ajouter `[c_i L_i,c_i U_i]` ; pour un coefficient négatif, inverser les extrémités. Les sommes de bornes sont calculées en fractions exactes. Un intervalle entièrement positif certifie `greater` ; un intervalle entièrement négatif certifie `less`.

La largeur de l'intervalle total est majorée par :

$$2^{-p}\sum_i|c_i|.$$

Elle tend vers zéro. Comme les égalités ont déjà été décidées exactement, toute somme restante est non nulle ; en l'absence de plafond, le raffinement finit donc par séparer son signe. Cet argument ne fixe pas une latence pratique uniforme. La proximité à zéro peut demander bien davantage de précision qu'une comparaison usuelle.

La procédure bornée utilise les précisions `0,4,8,16,32,64,128`, tronquées à la limite déclarée. L'épuisement du budget rend `indeterminate` avec la raison, la dernière inclusion et les compteurs. Un changement d'exposant, une comparaison flottante ou un epsilon ne sont jamais des voies de repli autorisées.

Pour le vote complet, comparer les numérateurs des classes et appliquer la clé de départage déclarée uniquement lorsque le comparateur rend `equal`. Une comparaison nécessaire indécise laisse le vote indécis, sauf si un autre certificat établit déjà la domination de tous les candidats. La classe bruit et le cas `T_x=0` restent régis par les conventions explicites du contrat ; une égalité entre deux classes positives n'est pas une absence de support.

## 5. Travail borné et statuts

Le [juge d'audit](vote_p3_exact_probe.py) prend des triplets entiers `(num,den,multiplicité)` pour chaque côté. Les limites par défaut sont explicites :

| Grandeur | Plafond d'audit |
| --- | --- |
| Lignes d'entrée cumulées | 256 |
| Numérateur / dénominateur / multiplicité | 192 / 128 / 64 bits |
| Coefficient rationnel regroupé | 4 096 bits au numérateur et au dénominateur |
| Classes mémorisées | 64 |
| Tests de rapport entre classes | 4 096 |
| Précision des intervalles | 128 bits après la virgule de chaque racine |
| Tours de raffinement | 8 |
| Accumulateur rationnel d'intervalle | 16 384 bits au numérateur et au dénominateur |

Les bornes des niveaux u16 du contrat sont contrôlées : `1/4≤λ≤3·65535²/4`. Les tableaux sont bornés par les lignes et les classes. Les radicandes ont au plus 320 bits, donc les entiers passés à `isqrt` pour les intervalles ont au plus 576 bits avec ces paramètres. Le nombre de tests de classes est quadratique au pire en nombre de radicandes, mais borné par le plafond déclaré ; chaque test emploie un gcd et deux racines carrées entières.

Les opérations sur fractions peuvent produire un temporaire plus large que le résultat autorisé. Les opérandes déjà agrégés sont bornés avant l'opération suivante ; l'addition/multiplication crée des temporaires d'une taille bornée par la somme des tailles de ces opérandes, puis le résultat est vérifié. La limite de 4 096 bits n'est donc pas présentée comme un pic mémoire exact de l'arithmétique rationnelle. Les paramètres bornent le travail algébrique demandé, pas un temps mur garanti du runtime Python.

| Statut | Autorité |
| --- | --- |
| `equal` | Toutes les classes ont été traitées et tous leurs coefficients valent exactement zéro |
| `less` / `greater` | Intervalle rationnel complet strictement négatif / positif |
| `indeterminate` | Plafond de lignes, bits, classes, comparaisons, précision ou tours atteint ; aucun ordre ni égalité annoncé |
| `invalid_input` | Niveau non positif, dénominateur nul, multiplicité négative, domaine u16 ou schéma invalides |

Les valeurs trop grandes pour la limite de représentation déclarée rendent `indeterminate` avec `input_bits`, et non un overflow ou une approximation. Les plafonds sont des choix du consommateur : ils ne prouvent pas que tous les votes des futurs nuages industriels termineront dans cette enveloppe.

## 6. Falsifications rationnelles indépendantes

Les [reçus normal](receipts_resolver_20260905/weights/normal.json) et [Python optimisé](receipts_resolver_20260905/weights/optimized.json) portent des cas calculés exclusivement dans l'audit. Les signes attendus ne sont pas produits par une seconde exécution du même solveur : identités de changement d'échelle, monotonie stricte, carrés exacts et identités de Pell constituent les autorités séparées.

Les deux cas délicats sont :

$$367296043199-259717522849\sqrt{2}<0,\qquad 367296043199^2-2\cdot259717522849^2=-1.$$

$$886731088897-627013566048\sqrt{2}>0,\qquad 886731088897^2-2\cdot627013566048^2=1.$$

Ils sont bien des différences de sommes p3 : `√2=4·2^(-3/2)`. Les multiplicités nécessaires tiennent sur 42 bits ; il s'agit d'un stress du format d'histogramme, au-delà du plafond d'incidences du fold actuel, sans attribution à une sortie moteur. Le signe est donné indépendamment par le signe de `a²−2b²`, puisque `a+b√2>0`. La procédure certifie les deux signes à 128 bits ; avec un plafond de 16 bits, elle rend correctement `indeterminate`. Dans le second cas, le calcul binaire64 de la différence vaut 0 sur l'environnement reçu : cela ne devient pas une égalité mathématique.

Les exposants 22 et 23 de `(1+√2)^j` fournissent deux autres cas signés, dont les multiplicités cumulées `a+4b` restent strictement sous `2^31`. Ils exercent le raffinement à 64 bits et le même refus à 16 bits. Cette échelle respecte le majorant numérique actuel des incidences, sans prétendre qu'un nuage ou une sélection de clusters produit précisément ces histogrammes.

Les cas d'égalité incluent des radicandes distincts non divisibles, plusieurs classes irrationnelles se compensant, des fractions équivalentes et des lignes dupliquées. Le test `√2+√3−√6>0` comporte trois classes distinctes dans une extension de rang deux : le regroupement par classes ne doit pas confondre une relation multiplicative et une relation linéaire.

Quatre corruptions **d'audit** sont détectées : désactivation du test de rapport carré (égalité manquée), plancher utilisé comme majorant (signe erroné), exposant `λ^-3` (égalité transformée en inégalité), et égalité décidée par epsilon flottant (cas de Pell non nul déclaré nul). Ce ne sont pas des mutants produit ou des résultats CTest. Les portes normales utilisent des exceptions explicites et restent actives sous `python3 -O`.

```bash
python3 morsehgp3D_v7/audits/vote_p3_exact_probe.py --receipt normal
python3 -O morsehgp3D_v7/audits/vote_p3_exact_probe.py --receipt optimized
```

## 7. Ce qui est fermé et ce qui reste distinct

Le verrou « un vote p3 exact exigerait une factorisation générale ou un epsilon » est levé : une méthode exacte de comparaison existe, et ses échecs de budget sont explicites. Le coût supplémentaire utile à mesurer est le nombre de niveaux et de classes par comparaison, les tailles des coefficients, les tours de raffinement et le taux d'indécision au plafond choisi. Les regroupements peuvent être partagés entre les classes d'un même point lorsque l'univers est figé ; cette optimisation reste à raccorder et qualifier.

Cette fermeture porte sur les **numérateurs de vote**, qui sont des sommes linéaires de racines. Elle ne ferme pas automatiquement les masses `Σ S_tau/T_x`, les seuils de condensation, les scores d'excès de masse ou la sérialisation de probabilités : les divisions et produits d'expressions algébriques y changent le problème. Elle ne rend pas complète une mesure calculée sur le mauvais univers de cofaces.

Restent donc le raccord industriel aux histogrammes réellement exportés, la qualification du consommateur choisi et l'autorité des décisions de masses/condensation. Aucun fichier produit, registre, branche ou VM n'a été modifié. GCP non utilisé.
