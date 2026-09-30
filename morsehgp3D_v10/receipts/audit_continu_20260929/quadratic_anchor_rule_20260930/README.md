# Qκ : ancrage quadratique par cohortes — proposition d'audit

Statut : proposition mathématique CONDITIONNELLE, aucun moteur natif/GPU/GCP, performance, statistique ou borne sous-quadratique de D qualifiés.

Les cinq fichiers PROOF.md,check.py,normal.stdout,optimized.stdout,execution.json sont des copies byte-identiques du petit paquet privé clos /tmp/mhgp10-quadratic-cohort-rule-20260930.jQmaep1M. PROOF.md reste inchangé : il prouve les conclusions sous L6 et l'interleaving scalaire indiqués. La revue indépendante mathématique n'a trouvé aucune erreur dans les calculs.

## Préconditions géométriques explicites

M est une hauteur NONNÉGATIVE de cohorte dans une filtration à plateaux fermés ; les maps de composantes, l'univers complet et la correspondance de points doivent effectivement satisfaire l'interleaving employé. Cela n'est pas acquis pour une liste de témoins partielle, une forêt incomplète, un vote de propriétaire ou une règle de naissance brute non normalisée.

Pour le raccord géométrique, utiliser le temps de résolution M(r)=max(r,b(J(r))), où J(r) est le LCA de la cohorte admissible fermée. Il ne faut pas présumer l'interleaving de la seule naissance brute b(J(r)). La preuve porte sur ce M normalisé, puis sur le propriétaire de la PREMIÈRE cohorte remonté à T. Elle ne choisit pas une cohorte maximisante tardive.

Ce changement de formulation ne coûte aucun calcul algébrique supplémentaire dans le MAX de Q :
max(r²,βJ)−κ(r²−A)=max(r²−κ(r²−A),βJ−κ(r²−A)).
Or r²−κ(r²−A)≤A pour r≥α,κ≥1. En présence du baseline A, calculer les candidats bruts βJ−κ(βc−A) donne exactement le même Q que M normalisé. L'interleaving est néanmoins une précondition à vérifier sur les cohortes/maps réelles ; l'équivalence du MAX ne prouve pas cet interleaving.

## Règle et conclusions conditionnelles

A=α²,Q=max(A,sup_r[M(r)²−κ(r²−A)]),κ entier≥2,T=√Q. L6 donne α≤T≤2α. Les seuls événements de cohorte pouvant améliorer Q ont r≤qα, q=(1+√(κ²−κ+1))/(κ−1). Un cutoff exact sans racines compare v=(κ−1)βc−κA : v≤0 ou v²≤4Aβc.

La stabilité de date est |TX−TY|≤Cε avec C=(κ+1)(q+1), et non seulement 2Cε. Les propriétaires sont compatibles sous les maps fermées au décalage≤(C+2κ)ε. On peut utiliser la constante rationnelle conservatrice Cbar=2κ(κ+1)/(κ−1) : pour κ2, date≤12ε et hauteur de raccord≤16ε. Ces constantes sont des bornes, pas un score statistique.

## Distinction de Pκ et coûts exacts

Qκ≥Pκ aux mêmes κ : Q est plus retardante, pas équivalente, et ce n'est pas une correction du mutant β de Pκ. Fixture abstraite α1,r3/2,m5/2,κ2 : P3/2 et Q²15/4. Les qualités statistiques, non-percolation, ARI/EOM et robustesse des propriétaires restent à revoir sur les vrais objets/benchmarks ; elles ne suivent pas de cette dominance.

Pour N<2^266,D<2^200,2≤κ≤8, les candidats non réduits peuvent avoir un numérateur signé de magnitude<2^671 et un dénominateur<2^600. Les produits Q/Q peuvent demander1271bits, donc20limbes64 ; Q/ancien niveau871bits, donc14. C'est une démonstration conservatrice de capacité, pas une primitive native ou un tri qualifié ; aucun repli implicite vers le comparateur8mots déjà audité n'est permis.

Un univers déjà fourni peut être balayé en fonction de D, mais aucune borne de D ni coût de production des cohortes, LCA, sorts ou sorties n'est démontré sous-quadratique. Aucun changement de moteur n'est publié.

## Vérification portable

Le script scalar check.py a effectué1728cas,12096inégalités de panel et deux fixtures abstraites, normalement et avec−O (mêmes stdout). Les nombres de panel décrivent les sept familles d'inégalités par cas ; les branches conditionnelles et contrôles additionnels ne constituent pas un comptage instrumenté de chaque expression exécutée.

verify.py ferme l'inventaire EXACT des sept fichiers et toutes leurs empreintes AVANT de lancer check.py en Python dans le mode courant, puis exige le stdout exact correspondant et des empreintes inchangées. Il vérifie également les deux captures historiques et leur reçu. Ce rejeu n'appelle ni native, ni moteur, ni GCP et ne dépend pas du /tmp historique.

    sha256sum -c SHA256SUMS
    python3 -B verify.py
    python3 -B -O verify.py

Les argv /tmp dans execution.json documentent les deux appels historiques clos ; le lecteur portable n'exécute pas ces chemins. Les limitations scalar/geometric/statistics sont maintenues dans le stdout du check et dans cette note.
