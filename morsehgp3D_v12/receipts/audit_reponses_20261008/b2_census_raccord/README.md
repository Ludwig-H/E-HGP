# B2-C : premier raccord CPU du census

Capture provisoire du 8 octobre 2026 à 11:03:36 UTC, cinq fichiers encore identiques à 11:06:02.
Prototype non publié, base produit `72f622a55`, dépôt de travail local `08ce8485` ; pins complets dans
`capture.json`. Aucun moteur, compilateur ni jeu de coordonnées exécuté ou lu. Aucun gain mesuré par cet audit.

Le raccord conserve le census produit : stockage de n sites, atomic_flag par workspace, contrôles d'index/témoins,
callback, exceptions, inversion de coquille et durée de vie empruntée. Le corps de `CensusWorkspace::run` est
inchangé. Il rend les définitions de la garde visibles au compilateur dans `guard.hpp`, spécialise les voies
native/certifiée et enlève `Point::make` répété sur les sites du Cloud. Les voies contrôlée/large restent dans
`slow_sign`, avec le même repli entier. La voie census générique garde `Point::make`. La déclaration `inline`
ne prouve pas que le compilateur supprime effectivement tous les appels : assembleur et mesure restent à vérifier.

**Coordonnées et repère local.** `prepare_cloud` contrôle chaque coordonnée dans `[0,2^b−1]`, b≤B≤32, puis possède
une copie immuable ; les points d'ancrage de la boule appartiennent au profil B. Dans `side_site`, les trois u32
sont convertis en i64 **avant** la soustraction avec `anchor_` i64. Ainsi
`−(2^B−1) ≤ v_j ≤ 2^B−1`, donc `|v_j| < 2^32 < 2^34` : le contrôle de domaine d'offset supprimé ne pouvait pas
refuser ces appels. Par exemple, pour x=0 et ancre=2^32−1, le calcul donne bien −4294967295, sans soustraction u32.
L'ancien `Point::make` ne pouvait pas refuser un site du Cloud vivant. Cette implication couvre algébriquement B≤32,
sans constituer une qualification native u24/u32 ou une extension du profil d'une campagne u21.

**Boîtes.** Le census passe les coins d'un véritable `Box`. `index/build.cpp` construit les feuilles avec min/max
des sites du Cloud, réunit les enfants par min/max, puis passe par `Point::make` et `Box::make`. Donc lo≤hi et chaque
coin appartient au profil. Dans la nouvelle surcharge, `i64{lo[j]} + hi[j] - 2*anchor_[j]` est signé avant toute
addition/soustraction, dans `]−2^33,2^33[`. Les différences proche/lointain sont aussi i64 ; le test `std::clamp` garde
sa précondition lo≤hi. Les décisions disjoint/partiel/proche/lointain sont les anciennes expressions déplacées.

**Puissance.** Le test du pavé précède la puissance : pour un point ou coin effectivement évalué, le contrat NUM-GARDE
donne `|v_j|<2M`, M=2^s. Quand `s+2≤30`, `Σv_j²<3·2^58<2^63`, donc la somme i64 ne déborde pas. Sinon chaque produit
commence par une conversion i128. Les produits par D et N gardent les anciens certificats : voie native
`6s+11≤107`, ou certificat de la boule au domaine `s+2` pour la voie certifiée. Le choix de voie, D, N, l'ancre et la
préparation proche/seuil ne sont pas changés. `inline_lane_` est vrai exactement pour une préparation réussie et
une voie native/certifiée ; les autres cas gardent préparation cassée, essai contrôlé puis large. `lanes.add(lane_)`
remplace le même incrément conditionné à un ledger présent. `side_offset` et `wide_sign` sont inchangés dans leur corps.

**Limite de l'API nouvelle.** `side_site(u32,u32,u32)` est public mais son commentaire exige des coordonnées du domaine
de `Point` ; u32 seul ne prouve pas cette condition pour B=21. La surcharge de coins bruts exige également des coins
dans le profil et lo≤hi : un couple arbitraire inversé n'est pas admis à `std::clamp`. Les appels actuels examinés
respectent ces conditions ; aucune erreur produit sur entrée valide n'est trouvée. Avant livraison, rendre ces
préconditions explicites avec leur caractère non vérifié, ou réserver ces entrées brutes à un accès interne au
propriétaire certifié. Garder les façades `side(Point)`/`bound_signs(Box)` pour les appelants publics ordinaires.
Il ne faut pas promettre le refus de toute coordonnée u32 hors profil sur cette voie brute.

Ce premier pas suit la recette du [reçu MES-G-APP](../g_appareil_natif/README.md) sans importer ses 76 places par requête,
ses compteurs u32 ni un parcours supplémentaire. Il n'ajoute ni Buffer ni capacité dynamique. Les portes à fermer
sur le corps livré restent l'égalité de tout CensusLedger et de I/U, les voies contrôlée/large, boîtes aux extrêmes,
saturation tardive/coquille large, refus publics, puis les empreintes FULL. Le gain doit être mesuré dans G et FULL ;
le ratio du microbanc ne se transfère pas automatiquement.

Rejeu de provenance : `python3 -B check.py DEPOT CAPTURE_EXTERNE`, puis avec `-O`.
La capture externe contient les cinq fichiers candidats aux chemins `src/...` et `change.patch` épinglé ; elle reste
nécessaire car le prototype n'est pas publié. Le lecteur vérifie les hashes et trois corps conservés, sans prétendre
transformer ces comparaisons textuelles en preuve complète C++ ou en test natif. Résultat : `capture.json.result`.
