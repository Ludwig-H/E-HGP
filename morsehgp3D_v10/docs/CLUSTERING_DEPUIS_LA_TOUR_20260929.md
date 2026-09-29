# Clustering hiérarchique depuis la tour : modèle, lecture critique, tête v10-b (29 septembre 2026)

`public_status=not_claimed`. Cette note est un raisonnement, pas une preuve. Elle s'appuie sur les parties I et II de la
thèse, sur HGP-old et sur nos mesures dev, et ne prend aucune de ces sources comme oracle (directive de l'utilisateur
du 28 septembre).

## 1. Ce que la tour contient

Pour chaque ordre K et chaque rayon r, la tour contient les composantes connexes de L_K(r), l'ensemble des y tels que
la boule fermée B(y, r) contient au moins K points. Ce sont les amas de forte densité de l'estimateur K-NN, sur tout
l'espace et pas seulement aux points de données. S'y ajoutent les fusions N-aires à niveaux exacts, et les cartes
verticales de l'ordre K vers l'ordre K − 1.

HDBSCAN à `min_samples` = K ne voit de cette bifiltration qu'une tranche, et sous une autre connexité : la liaison
simple robuste, faite de cœurs puis d'arêtes.

## 2. Lecture critique des parties I et II

**Ce qui tient.**
- Le théorème 2 est une identité de définitions, et sa preuve est correcte : les K-polyèdres sont les amas discrets
  de forte densité, c'est-à-dire les points à distance au plus r d'une composante de L_K(r).
- Le chapitre 7 sépare deux défauts de la liaison simple robuste et de HDBSCAN :
  - la dilatation appliquée avant l'identification des composantes, qui fait percoler trop tôt ;
  - la condition de cœur, qui exige que le point appartienne lui-même au niveau et coûte du rappel.
- Les amas discrets évitent le second défaut.
- Le chapitre 5 (l'arbre comme espace de résolution) justifie de choisir la sélection, EOM ou autre, séparément de la
  hiérarchie.

**Ce qui ne tient pas ou reste ouvert.**
- La proposition 6 et le théorème 5 (le graphe de Gabriel contient toutes les fusions utiles) sont faux en général :
  contre-exemple E5 de l'audit de la v9. La tour v10 calcule l'objet exact sans s'appuyer sur eux.
- L'argument de percolation (§ 7.5.3) est asymptotique et en partie conjectural. Il n'est mesuré que sur un processus
  de Poisson homogène. Sa prédiction, les amas discrets meilleurs que les cœurs, est en revanche confirmée par nos
  mesures dev (§ 4).
- Les masses et le vote du § 9.1 sont des heuristiques : aucun théorème ne relie m_τ à une masse de probabilité, et la
  proposition 7 ne garantit qu'une partition. Mesure : le vote de couverture ne fait pas mieux que l'étiquette de
  l'arbre suivie d'un remplissage borné.
- ~~Le poids ψ(t) = t^(−p), avec p la dimension ambiante, est moins bon que l'échelle à la dimension intrinsèque ẑ
  (mesuré).~~ Contredit par la grille z du 29 septembre (reçu `bench_dev_z_samehead_20260929`) : z = 3 égale ẑ jusqu'à
  K = 5 et le dépasse à K = 8 et 10, où ẑ ≈ 2,1 fait s'effondrer `shells`. L'optimum de z dépend de n (≈ 3–4 à 2 000
  points, ≈ 6 à 8 000). z est un cadran de granularité de l'EOM, pas une dimension : voir
  [l'audit du 29 septembre](../audits/audit_hierarchie_knn_20260929/AUDIT_HIERARCHIE_KNN_20260929.md).
- Le tableau 9.3 (SIPU) n'établit pas la supériorité sur HDBSCAN : une exécution par jeu, `min_samples` non déclaré,
  traitement du bruit dans l'ARI non déclaré. D'où notre protocole apparié à K = `min_samples`.
- Le paradoxe du choix de K (§ 4.4.5) est attribué à la connexité de HDBSCAN. Or notre tête v10 du 28 septembre, sur
  la connexité exacte, se dégradait aussi quand K augmentait. La cause était la condition de cœur (§ 3). Avec les
  amas discrets, l'optimum passe de K = 1 à K = 3 ; le biais d'un grand K demeure (K = 5 est moins bon que K = 3).

HGP-old, première implémentation de la thèse, fait déjà entrer les points par leurs facettes et étiquette par vote de
couverture : c'est une sémantique d'amas discrets, que notre tête v10 avait perdue. Il condense séparément les
composantes qui ne fusionnent jamais et laisse la racine sélectionnable. Nous ne reprenons ni l'un ni l'autre.

## 3. Diagnostic de la tête v10 du 28 septembre

C∩X fait entrer x à d_K(x), dans la composante de L_K qui le contient. C'est la sémantique des cœurs, sur la connexité
exacte. Mesure dev à K égal : parité avec sklearn dès K ≥ 3, et optimum bloqué à K = 1, où cœurs et amas discrets
coïncident. Le remplissage borné, qui apportait +0,03 à +0,04 aux deux méthodes, en était un palliatif a posteriori.

## 4. Tête v10-b (prototype mesuré sur dev)

1. **Entrée par première couverture.** x entre au niveau α_K(x)², le carré du plus petit rayon d'une boule fermée
   contenant x et au moins K − 1 autres points, avec d_K(x)/2 ≤ α_K(x) ≤ d_K(x). Il entre dans la composante de cette
   boule. Précision de l'audit du 29 septembre : en position générale, cette boule est une naissance d'ordre K, sous
   mcs ; la condensation ne lit donc pas α_K(x), et le point sort au λ de la fusion qui absorbe sa naissance. Le gain
   de cette entrée vient de la profondeur à laquelle la masse entre dans l'arbre. Calcul exact :
   - la première boule couvrante est la première boule du catalogue, par niveau, qui contient x et au moins K sites ;
   - toutes les K-parties d'une boule fermée contiennent son centre dans leur région témoin, donc une seule
     résolution par boule suffit.
2. **Condensation HDBSCAN** en masse (mcs = √n), puis **EOM** avec λ = r^(−ẑ), où ẑ est la dimension intrinsèque de
   Levina–Bickel. La racine est exclue.
3. **Étiquettes** : la lignée dans l'arbre, puis un remplissage borné b(1,5). Le vote de couverture, prévu comme
   variante, donne le même score à 0,002 près.

Mesure dev (reçu `receipts/bench_dev_cover_20260929`), meilleure configuration de chaque méthode. À K = 1, les
hiérarchies de la tour et de sklearn coïncident (liaison simple) : l'écart y est entièrement un effet de tête.

| K | Tour, première couverture | sklearn à `min_samples` = K |
| ---: | ---: | ---: |
| 1 | 0,776 | 0,715 |
| 2 | 0,776 | 0,726 |
| 3 | 0,783 | 0,748 |
| 5 | 0,778 | 0,765 |

## 5. Questions ouvertes, à trancher par la mesure

- **Choix de K.** Dev donne K = 3. Deux voies restent à mesurer : un choix par scène fondé sur la tour elle-même
  (verticales, stabilité à travers K), ou une tranche de la bifiltration au sens de Rolle et Scoccola.
- **Familles allongées** (`filaments`, `anisotropic`), où sklearn avec feuilles à grand `min_samples` reste meilleur.
  Deux pistes : une échelle ẑ locale plutôt que globale, et une sélection autre qu'EOM.
- **Recouvrements** pour K ≥ 2 : affectation dure à la première couverture, comme maintenant, ou masses fractionnaires
  partagées entre les composantes couvrantes.
- **Coût** : une résolution par boule couvrante au lieu d'une par point. Il reste à le mesurer dans le régime LiDAR.
- **Validation** : toute revendication passera par un nouveau préenregistrement sur un espace de graines neuf,
  puisque la tête a changé (EVAL_v2 D11).
