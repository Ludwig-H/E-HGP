# Doctrine numérique corrigée — relecture bornée

2 octobre 2026, capture `suivi_verrous/snapshot`, base `986f75799`. Cadre `exploration_v11_hors_registre`, `cpu_reference`, `quantized_u18_input_only`, `not_claimed`. Trois documents sont [copiés et hachés](sources.json). Deux petits calculs Fraction seulement : aucune compilation, campagne locale, exécution native FENV ou GCP.

**L'ancien défaut F3 est fermé par le nouvel énoncé.** F2/F3/F4 et la borne d'erreur F6 sont cohérents sous leurs hypothèses. Il reste deux précisions de Q2 à rétablir dans F6, dont une omission de domaine démontrée ci-dessous ; aucune erreur d'un filtre exécuté n'est constatée.

## Preuve relue

Dans [ARCHITECTURE.md](sources/ARCHITECTURE.md), lignes 77–94, poser L=1−u, u=2⁻⁵². Une conversion normalisée a son rapport dans [L,L⁻¹]. Multiplier additionne les exposants, chaque produit arrondi ajoutant un ; inverser conserve cet intervalle symétrique. Le quotient par inverse puis produit demande donc Eₐ+Eᵦ+2. Pour une somme de même signe, le rapport avant arrondi est une moyenne pondérée positive des rapports des termes ; son exposant est au plus leur maximum, puis au plus n−1 arrondis supplémentaires. Aucune indépendance des erreurs n'est utilisée. Une réutilisation multiplie bien ses occurrences dans l'expression.

Le témoin initial N=2⁵⁴+1, conversion ascendante puis carré, expose trois facteurs : conversion deux fois, multiplication une fois. Sa sortie viole l'ancienne borne à deux instructions, mais appartient à [L³,L⁻³] relativement à N². F2 ne l'admet pas dans son domaine d'exactitude ; ses nouvelles conditions sur coefficients, produits et sommes partielles sont appropriées.

F4, lignes 95–99 : si x̃<fl(cỹ), alors x≤x̃L⁻ᴱˣ<cỹL⁻¹⁻ᴱˣ≤y dès que c≤L^(Eₓ+Eᵧ+1). L'arrondi du produit est bien payé. Bernoulli donne Lᴺ≥1−Nu≥1−2⁻⁴⁰ pour N≤4096. La constante et le test strict sont donc sûrs, dans le domaine des opérations annoncé. Les zéros sont explicitement sortis avant ce filtre.

F6, lignes 102–115 : développer l'arbre effectivement évalué donne des monômes exacts, chacun multiplié par des facteurs dans [Lᴱ,L⁻ᴱ]. Avec M majorant la somme de leurs valeurs absolues, |ṽ−v|≤(L⁻ᴱ−1)M. Les récurrences de M et E préservent cette propriété avec réutilisations ; la forme non contractée majore la FMA. Il faut majorer E sur tous les parenthésages autorisés, comme le texte le demande. Pour E entier et Eu≤1/2, Bernoulli donne Lᴱ≥1−Eu, donc L⁻ᴱ−1≤Eu/(1−Eu)≤2Eu. Le seuil proposé majore bien cette erreur **lorsqu'il est représentable**.

## Deux précisions normatives à restaurer

**P2 documentaire : protéger aussi l'exposant du seuil.** La réponse Q2 annonce une reprise telle quelle, mais [Q2, lignes 54–57](sources/AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md) disait « exactement représentable si son domaine exponentiel est protégé ». F6 ligne 111 a perdu cette condition. L'absence de débordement de l'expression ne suffit pas :

- partir de x=2¹⁷, entier du profil 18 bits ; cinq carrés exacts donnent a=x³²=2⁵⁴⁴, avec Eₐ=31 ;
- évaluer v=((a−a)+1)² : tous les calculs restent finis et exacts dans les quatre arrondis, v=1 ; les zéros exacts ne sont pas des sous-flux ;
- la récurrence avant annulation donne M=(2⁵⁴⁵+1)², E=67, donc q=1091 et e=7 conviennent ; pourtant τ=2¹⁰⁴⁷ dépasse le domaine fini binary64.

Ce polynôme artificiel n'est pas un prédicat géométrique livré ; il réfute uniquement l'implication générale « expression sans débordement ⇒ seuil représentable ». Ajouter une garde de l'exposant q+e−51, avec repli exact si elle échoue. Une puissance de deux binary64 finie et non nulle exige −1074≤q+e−51≤1023 ; exiger au moins −1022 si le seuil doit rester normal. Pour les budgets entiers non triviaux actuels q,e≥0, la borne basse est automatique.

**Précision sur les feuilles :** Q2 ligne 51 exigeait de compter les coefficients/conversions non exacts dans E. « Feuille exacte » dans F6 doit signifier exacte **dans binary64**, pas seulement un entier exact conservé ailleurs. Réintroduire cette phrase prévient une mauvaise initialisation E=0 ; aucun tel défaut de code n'est allégué ici.

Le domaine doit aussi couvrir chaque forme autorisée : 2¹⁰²³/2¹⁰²³ vaut 1, mais la transformation par inverse calcule d'abord 2⁻¹⁰²³, sous-normal. F3 l'exclut déjà par son contrat ; la preuve de domaine doit donc porter sur cet intermédiaire, pas seulement sur le résultat du quotient. Ni FTZ/DAZ ni une transformation algébrique non énumérée ne sont qualifiés par ces preuves.

## Qualification

[check_rules.py](check_rules.py) vérifie les témoins précédents, 240 comparaisons exactes aux extrémités des intervalles F3, quatre couples F4 jusqu'à Eₓ+Eᵧ+1=4096, six valeurs de la borne gamma et le témoin Q2 d'annulation dans quatre arrondis émulés. Le filtre F6 y demande correctement le repli exact. [normal.json](normal.json) et [optimized.json](optimized.json) sont identiques octet pour octet ; les deux [commandes](execution.json) rendent zéro, sans `assert`. Les documents sont revérifiés avant et après. Ce reçu confirme une doctrine et précise ses hypothèses ; il ne qualifie aucun futur filtre géométrique réel.
