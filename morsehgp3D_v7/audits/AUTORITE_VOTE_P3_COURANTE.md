# Vote p3 : autorité numérique conservée

La comparaison exacte des **numérateurs de vote** est fermée par un juge borné d’audit. Elle ne livre ni consommateur produit, ni autorité pour les quotients de masses ou la condensation. `public_status=not_claimed`.

## Égalité sans factorisation

Avec un niveau carré réduit λ=a/b>0, `λ^(-3/2)=(b/a²)√(ab)`. Le dénominateur T commun d’un point s’élimine lors de la comparaison des classes. Leur différence devient une somme finie de racines à coefficients rationnels.

Deux radicandes N,M ont un rapport carré rationnel exactement lorsque, pour `g=gcd(N,M)`, les entiers premiers entre eux N/g et M/g sont tous deux des carrés. Deux `isqrt` suffisent. Leurs coefficients se regroupent alors par le facteur rationnel correspondant ; aucune factorisation générale n’est requise.

**Complétude du test d’égalité.** Des classes de carrés distinctes donnent des racines linéairement indépendantes sur Q. Prendre une base d₁,…,dᵣ du groupe fini des classes engendrées, puis `E_j=Q(√d₁,…,√dⱼ)`. Par récurrence, les produits des j racines prises zéro ou une fois forment une base, et tout carré rationnel dans E_j a sa classe engendrée par les dᵢ. En effet, l’indépendance interdit que dⱼ₊₁ soit carré dans E_j ; dans l’extension quadratique, `(u+v√dⱼ₊₁)²` rationnel impose `2uv=0`. Les deux cas ramènent l’assertion sur les classes au rang précédent. La récurrence prouve l’indépendance des monômes, donc celle des racines de classes distinctes.

Après regroupement **complet**, la différence est nulle si et seulement si tous les coefficients le sont. Un regroupement interrompu ne décide rien.

## Signe sous plafond

À précision p, `q=isqrt(N·2^(2p))` fournit les bornes rationnelles `q/2^p` et `(q+1)/2^p`, égales si le carré est exact. Inverser les extrémités pour un coefficient négatif. Un intervalle complet strictement positif ou négatif certifie le signe ; sa largeur est au plus `2^(-p)Σ|cᵢ|` et tend vers zéro. Après exclusion exacte de zéro, le raffinement termine sans plafond théorique, sans fournir une latence uniforme.

Le [juge](vote_p3_exact_probe.py) borne lignes, bits, classes, comparaisons et raffinement. Il rend `equal`, `less`, `greater`, `indeterminate` sur plafond, ou `invalid_input` ; jamais un epsilon ni l’exposant λ^-3 comme repli. Les paramètres et compteurs exacts sont dans le programme et les reçus.

Les [preuves scellées](receipts_resolver_20260905/README.md) vérifient 27 cas normalement et sous `-O`, quatre permutations et quatre corruptions d’audit. Les signes de Pell ont une autorité séparée : `a²−2b²=±1`, donc le signe de `a−b√2` est connu, même lorsque le flottant donne zéro. Les essais à petit plafond rendent l’indécision attendue. Aucun de ces mutants n’est un test moteur.

Le [contrat d’incidence](CONTRAT_MASSES_VOTE_COURANT.md) reste préalable. Les histogrammes exportés, les décisions sur quotients et leur coût relèvent des [sujets différés](QUESTIONS_SECONDAIRES.md).
