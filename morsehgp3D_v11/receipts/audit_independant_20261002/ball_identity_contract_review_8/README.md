# Contrat proposé d'identité géométrique des boules

Conclusion mathématique pour le prochain raccord index/FULL, sur huit sources numériques figées au commit `ffc2ff95f0ae7296bdc522df81df34c58c3fdf47`. Aucun code produit, build, test natif ou GCP exécuté. La clé décrite ici n'est pas portée ; aucune qualification native n'en découle.

L'égalité de clés s'interprète dans le même repère de coordonnées, avec la même origine et les mêmes unités de grille. Si des catalogues utilisent des repères différents, ils doivent d'abord être transformés exactement dans un repère commun, ou porter un contexte explicite ; les seuls cinq coefficients ne décrivent pas ce contexte.

Une fabrique certifiée fournit l'ancre de coquille a, le centre a + N/D et D > 0. Le polynôme signé

`g(x) = D ||x−a||² − 2 N·(x−a) = A ||x||² + B·x + C`

a les coefficients entiers `A=D`, `B=−2(Da+N)`, `C=D||a||²+2N·a`. Le tuple `(A,Bx,By,Bz,C)` divisé par le PGCD de ses cinq magnitudes, avec A positif, est une clé primitive unique. Tous les zéros ont une magnitude nulle et aucun zéro signé distinct ne doit être exporté. Cette normalisation porte sur une nouvelle valeur locale ; elle ne réduit ni le Level public ni les octets de Sphere/Q4Candidate.

Preuve : `g(x)/D = ||x−c||²−r²` car l'ancre est sur la sphère. La clé détermine `c=−B/(2A)` et `r²=||c||²−C/A`. Deux boules de même centre et rayon ont donc des polynômes proportionnels par un facteur rationnel positif, puis le même tuple entier primitif ; réciproquement une même clé impose ces mêmes centre et rayon. Aucun ID, arité, qmin, poids ou Level n'intervient. Des centres égaux et des rayons différents doivent avoir des clés différentes.

C doit rester **signé** : le triangle `(2,0,0),(0,2,0),(0,0,2)` donne centre `(2/3,2/3,2/3)`, rayon carré `8/3` et clé `(3,−4,−4,−4,−4)`. L'origine est intérieure. `C=power(boule, Point(0,0,0))` peut utiliser le domaine exact déjà exposé par [predicates.cpp](source/morsehgp3D_v11/src/num/predicates.cpp#L126), sans construire un Level.

Les bornes de stockage doivent suivre la présentation certifiée : B est natif i128 aux profils 18/21/24, puisque `|N+D a|<48 M^5` et donc `|B|<96 M^5<2^(5Bbits+7)`, avec `5Bbits+7<=127`. C relève du type signé SideInt déjà prévu : q3 utilise le repli Wide aux profils 21/24. Pour q4, les bornes de l'ancrage commun `D<6M³`, `|Nj|<9M⁴` donnent `|C|<72M⁵<2^127`, même pour une sphère non critique. Ces bornes ne supposent pas le centre dans l'enveloppe convexe. [Budgets](source/morsehgp3D_v11/src/num/budgets.hpp), [preuve des intermédiaires](source/morsehgp3D_v11/src/num/predicates.cpp#L33).

Le [Wide capturé](source/morsehgp3D_v11/src/num/wide.hpp) fournit signe/magnitude, comparaisons, addition/soustraction, produit complet, redimensionnement et conversions natives. Il ne fournit **ni PGCD ni division exacte ni format de clé canonique**. Il faut construire et qualifier ces opérations, leur capacité, la factory fermée et l'encodage stable avant de revendiquer une clé native. La seule borne de SideInt ne qualifie pas cette nouvelle chaîne. Le tri ou le hash doit traiter une clé égale exactement, sans dépendre des représentations non réduites ou d'un hash non vérifié.

[check.py](check.py) utilise Gram/Gauss et Fraction, indépendamment des formules Cramer du produit. Il contrôle 32 présentations permutées q2/q3/q4 de la même boule, y compris un q4 dont le centre est sur une arête du tétraèdre ; l'identité géométrique ne certifie donc pas la criticité. Trois nuages ayant cette même boule ont qmin respectivement 2/3/4, ce qui interdit d'inclure qmin dans la clé. Les coefficients multipliés par sept donnent la même clé sans changer les données originales. Les autres contrôles distinguent deux rayons au même centre, imposent C signé et vérifient deux replis affines MEB exacts (rectangle coplanaire et trois points colinéaires), ainsi qu'une circonsphère obtuse différente de la MEB.

La [factory actuelle](source/morsehgp3D_v11/src/num/geometry.hpp#L25) peut donner une circonsphère non critique ; un support affine dépendant donne un succès sans sphère. Ce résultat vide ne signifie pas absence de MEB. Un futur calcul MEB doit retrouver un sous-support exact positif et vérifier toute la population. L'oracle borné l'illustre par énumération des sous-supports de taille au plus quatre ; ce n'est ni un algorithme massif proposé ni une qualification d'un moteur MEB v11.

Sources avant/après : [SOURCE_BEFORE.json](SOURCE_BEFORE.json), [SOURCE_AFTER.json](SOURCE_AFTER.json). Lecteurs : `python3 -B check.py`, `python3 -B -O check.py`, sorties identiques capturées dans [RUN.json](RUN.json). Le reçu fermé est autonome ; `SHA256SUMS` couvre tous ses payloads hors lui-même.
