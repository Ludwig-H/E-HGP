# Contrelecture mathématique S8 — 5 octobre 2026

Verdict : aucun défaut mathématique important trouvé au pin publié `53c027fe848b0d890f164eb87ebf347338c58d55`. Cadre `phase=exploration_v11_hors_registre`, `backend=cpu_reference`, `profile=quantized_u21_input_only`, `public_status=not_claimed`. Sources figées avant lecture/rejeu, empreintes dans `source_manifest.json`. Les rapports développeur copiés stables citent le commit local antérieur `adfcdc692` ; les sources examinées sont celles du pin publié, avec le refus du nombre de rangs hors u32 déjà corrigé.

La lecture couvre `src/num/big.{hpp,cpp}`, `rational.cpp`, `radical.{hpp,cpp}`, `roots.{hpp,cpp}` et les nouveaux juges. Aucune compilation, aucun binaire natif, GCP ou campagne complète de 5 900 décisions exécuté par cet audit. Les preuves natives et leur qualification restent celles des reçus dédiés.

## Raisons du verdict

La transformation `c sqrt(n/d) = (c/d) sqrt(nd)` est exacte pour n≥0 et d>0, y compris avec des niveaux non réduits. Deux radicandes entiers positifs sont dans la même classe si et seulement si leur produit est carré : le coefficient de passage est `isqrt(N rep)/rep`. `RadicalSum::group` utilise la signature pour éliminer des essais, puis exige effectivement ce carré parfait. Les coefficients de classe nuls sont retirés après regroupement ; l'égalité certifiée correspond à l'annulation de toutes les classes. Pour justifier l'indépendance, prendre le corps multiquadratique contenant les racines des premiers qui divisent les radicandes : les différentes parties sans carré donnent des caractères distincts des changements de signe. L'application de ces caractères à une relation rationnelle et leur moyenne isolent chaque coefficient. Aucune factorisation n'est nécessaire dans l'algorithme.

`RadicalSum::refine` donne les bornes orientées correctes pour les coefficients positifs et négatifs, décide seulement si zéro est strictement hors de l'intervalle et refuse à l'épuisement des précisions effectivement parcourues (96, …, 6 144). Il ne remplace pas une quasi-égalité par zéro. Les deux classes sont comparées par carrés avec leurs signes préservés. Les branchements de `sqrt_diff_cmp` et `sqrt_cmp2` correspondent aux élévations au carré valides sur leur domaine déclaré de radicandes non négatifs.

Pour `RootTable`, `R = isqrt(floor(N 2^128/D))` vérifie `R²D ≤ N 2^128 < (R+1)²D`. Les sommes signées sont donc encadrées par les bornes publiées ; leur largeur est exactement le nombre de termes dans l'unité 2^-64. Avec 16 termes au plus et R<2^89, les accumulateurs restent largement dans i128. Le repli conserve le niveau exact par `(sign/D) sqrt(ND)`. Le code refuse une racine hors du domaine et un nombre de rangs non représentable, et garde une sentinelle hors des racines possibles.

## Nouvelle contre-épreuve portable

`check_radicals.py` traduit seulement les nouveaux regroupements et encadrements dans Python entier/Fraction. Il les compare à un juge de signe indépendant dans `Q(sqrt(2),sqrt(3),sqrt(5))`, par multiplication dans la base des huit monômes puis élévations au carré récursives ; ce juge ne regroupe pas des classes et n'utilise aucun encadrement isqrt. Résultats : 80 signes conformes, dont 20 égalités, et 60 décisions par la première borne RootTable. La copie Python historique est contrôlée par AST contre le banc épinglé, puis fournit un troisième accord sur ces petits cas.

80 paires de radicandes de classe commune, avec facteurs carrés de plus de cent bits, ont des signatures égales. Une collision volontaire de 229 bits est construite : `N = 1 + 8 × produit(des 39 premiers impairs de la signature)`. N a la signature de 1 mais n'est pas carré. Le regroupement traduit conserve correctement deux classes, et `sqrt(N)−1` reste positif. Cela vérifie la nécessité du contrôle de carré après une collision ; la signature seule ne prouve jamais l'équivalence.

Les certificats des racines et les bornes de largeur sont vérifiés sur les 80 sommes avec des niveaux non réduits, ainsi qu'sur les bornes géométriques conservatrices u18/u21/u24 : 83/86/89 bits de racine, compatibles avec les plafonds. Cela ne qualifie pas ces profils natifs. La famille strictement concave `sqrt(n²)−2sqrt(n²+1)+sqrt(n²+2)` tranche −1 à 192 bits pour n=2^40 ; à n=2^2050+1, elle rend explicitement le refus après 6 144 bits. Aucun grand banc inchangé n'est repris.

Les quatre invocations (normal, `-O`, replay Git normal, replay Git `-O`) finissent au code 0, sans essai de harnais en échec. Sorties identiques octet pour octet, SHA256 `c3fe96cfa1aeea6f905e5a7e4a97daec0d83d9d56ee1664e7501d47dcd9dd4d1`. Commandes, codes, stderr exacts et empreintes des scripts sont conservés dans `executions.json`. `attempts/initial_check_radicals.py` conserve la source initialement exécutée, identique à la finale.

## Rejeu sans snapshot

```sh
python3 replay.py /workspaces/E-HGP
python3 -O replay.py /workspaces/E-HGP
```

`replay.py` reconstruit les sources par `git show` du pin, vérifie leurs SHA256 et exécute le harnais dans un répertoire temporaire ; aucun worktree développeur ni binaire n'est requis. Reprendre tout le contenu de la capsule sauf `snapshot/` : rapport, manifeste des sources, deux scripts, quatre sorties et stderr, executions, SHA256, deux rapports développeur et première source du harnais.

Limites : il s'agit d'une lecture du C++ et d'une contre-épreuve du raisonnement/mathématiques dans un modèle Python, pas d'une nouvelle preuve d'exécution de Big/Knuth en C++. Les dépassements de capacité des opérations rationnelles restent des refus autorisés ; aucun résultat en sortie native n'est jugé ici. Aucun nouveau verrou introduit.
