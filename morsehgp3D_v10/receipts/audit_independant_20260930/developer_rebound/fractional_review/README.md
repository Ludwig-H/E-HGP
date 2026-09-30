# Proposition 11 : masse au split et durée sous le seuil

Lecture ciblée du mémo privé `build/v10-verrou-points/masses_selection/MEMO_MASSES_SELECTION_20260930.md:388` et de `lib/selection.py:27,68,91`. Aucun verdict adverse « masses » n’était encore déposé au relevé ; ce reçu ne reprend pas la vérification des autres théorèmes. Aucun changement produit, GCP ou test statistique. Sources dans [sources.json](sources.json), passage du mémo dans [memo_excerpt.txt](memo_excerpt.txt).

**La masse de fin de l’enfant n’anticipe pas sa présence au split.** FULL fusionne lorsque β croît, mais la condensation descend lorsque λ=β^(−z/2) croît. À un split β=b_parent=d_enfant, l’enfant s’ouvre avec sa masse `m_enfant(d_enfant−)`. En progressif, l’invariant exact est `m_parent(b_parent+)=Σ_enfants m_enfant(d_enfant−)` : la masse propre nouvellement couverte dans le parent est encore nulle à cet instant. L’activation par marches peut ajouter à la naissance du parent une masse propre qui doit partir avant le split inverse ; les masses des enfants restent celles de fin. Les fusions simultanées sont traitées ensemble.

**Les masses aux splits ne fixent pourtant pas les dates de fin chronologiques.** Le prototype classe les enfants avec `end_mass` et ne coupe la stabilité qu’aux événements géométriques. Une masse progressive peut franchir `mcs` pendant la vie d’une branche. Si `mcs` signifie une masse minimale à chaque densité, cet événement manque. Si `mcs` signifie uniquement un seuil d’ouverture aux splits géométriques, le prototype définit une autre règle cohérente, mais cette convention doit être publiée ; elle n’est pas l’extension automatique de la condensation chronologique par linéarité.

Le petit cas exact est `X={(0,0,0),(1,0,0),(2,0,0),(10,0,0),(11,0,0),(12,0,0)}`, K=3, z=2, mcs=2. L’oracle Γ3 indépendant par parties collinéaires donne deux feuilles à β=1, deux ponts à β=81/4, puis une fusion simultanée des quatre composantes à β=25. Pour la feuille gauche :

- les poids totaux des trois sites sont `1`, `2044/2025`, `2063/2025` ;
- `a=1+2025/2044+2025/2063=12533447/4216772` ;
- `m(λ)=a(1−λ)` pour `1/25≤λ≤1` ;
- au split, `m(1/25)=75200682/26354825>2` ; à λ=1/2, `m=12533447/8433544<2` ;
- le seuil est atteint à `λ*=4099903/12533447`, strictement entre le split et la naissance géométrique de la feuille.

Le prototype sélectionne les deux feuilles et intègre chacune jusqu’à λ=1 : score `902408184/658870625`. Une règle qui termine la branche lorsque sa masse passe sous 2 soustrairait exactement `8433544/12533447`, l’aire triangulaire sous le seuil. Le mode par marches conserve sa masse de fin jusqu’à λ=1 et n’a pas ce même événement intérieur. Les masses de fin sont identiques entre les deux activations ; leurs dates de fin chronologiques ne le sont pas. Aucun changement d’étiquettes n’est revendiqué sur ce cas.

**Ce contrôle confirme** l’additivité au split et l’intégrale fractionnaire écrite dans `fractional_stats`. Il ne réfute ni la propriété d’antichaîne ni le vote de la proposition 12. Il précise l’obligation de la proposition 11 : définir le sens temporel de `mcs` et, pour la variante chronologique, ajouter les franchissements de masse au calendrier avant d’intégrer les stabilités. Une réparation par cohortes de la condensation dure traite le problème analogue des départs datés, mais le progressif demande aussi des franchissements continus.

[probe.py](probe.py) utilise un oracle Γ3 collinéaire exact et des formules rationnelles indépendantes, puis consomme sans réécriture les corps `condense`/`eom` et leurs helpers de [selection.source.txt](selection.source.txt). Deux exécutions minuscules, normal et `-O`, ont le même résultat octet pour octet : [normal.json](normal.json), [optimized.json](optimized.json). Reproduction autonome depuis ce dossier : `python3 -B probe.py` et `python3 -B -O probe.py`. Aucun binaire du moteur n’est requis.
