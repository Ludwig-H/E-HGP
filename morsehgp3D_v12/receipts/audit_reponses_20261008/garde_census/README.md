# Census : resserrer NUM-GARDE, proposition du 8 octobre 2026

**Proposition, aucun moteur modifié ni exécuté, aucun gain mesuré.** Base publiée
`d8c6f7164f7b8117792e7f146487acd975d410e1` (session J), sources inchangées lors
de la lecture de `4831fd3ba9778d2c52ab5e92126b19e2a54be2d4` pour les quatre
fichiers du patch. `capture.json` épingle treize sources et le patch. Le patch
change deux lignes exécutables ; le reste explicite la preuve. Il ne change ni
les budgets `6s+11`, ni le certificat `s+2`, ni la sélection des voies.

G utilise déjà le census en **une passe** (`src/tower/resolve.cpp:293`,
`src/index/census_workspace.cpp:112`). La proposition ne supprime aucun census
et ne reprend pas G-L3, rejeté après mesure. Elle réduit les évaluations exactes
à l'intérieur d'un même parcours ; l'index peut déjà éliminer presque tout le
travail visé. Aucun gain de complexité ne découle du volume du nouveau pavé.

## Preuve et invariants

Soient `m` le coin minimal du support certifié, `M=2^s`, ses largeurs `w_j≤M−1`,
le centre `c` et le rayon `R`. Le certificat donne `c=Σλ_i p_i`, `λ_i≥0`,
`Σλ_i=1`, tous les `p_i` sur la sphère. Pour tout autre centre `y`,
`Σλ_i|p_i−y|²=R²+|c−y|²`. Toute boule contenant le support a donc rayon au
moins `R` : la boule certifiée est sa boule minimale. La boule du milieu de
la boîte contient le support, donc

`R² ≤ (w_x²+w_y²+w_z²)/4 ≤ 3(M−1)²/4 < M²`.

Cette borne de rayon utilise la propriété de minimum ; elle ne découle pas
du seul centre dans la boîte. Avec `m_j≤c_j≤m_j+M−1`, la boule fermée est
strictement dans **`m_j−M < x_j < m_j+2M`**, au lieu de `(m_j−2M,m_j+3M)`.
Cela inclut le singleton `s=0`, `M=1`, `R=0`. Pour `s≤32`, toutes les bornes
signées restent dans `i64` : `−2^32≤m_j−M` et `m_j+2M<3·2^32`.

Les gardes ponctuelles et de boîtes gardent exactement les mêmes réponses :

- Site hors du nouveau pavé : puissance strictement positive, y compris sur
  ses faces. Une tangence au pavé n'est jamais une tangence à la sphère.
- Boîte disjointe : minimum entier strictement positif. L'ancien calcul
  renvoyait déjà `{+1,+1}`.
- Boîte qui rencontre le pavé : l'entier le plus proche de `c`, ramené dans
  la boîte, reste strictement dans le pavé ; c'est le même minimum entier.
- Boîte non contenue : un de ses coins est hors du pavé, donc le maximum
  est positif. Elle n'est pas rejetée sur ce seul fait. Si elle est contenue,
  le coin lointain reste admissible et inchangé.

Les signes de `PowerBoundSigns`, visites d'index, tests de sites, ordre,
saturation, intérieurs/coquilles et décisions de G restent ainsi identiques.
Les évaluations de puissance sont un sous-ensemble des anciennes ; **les
compteurs physiques `GuardLedger` et `LaneCount` peuvent changer**. Cela devra
être vérifié sur le chemin intégré. `in_guard` expose, lui, le nouveau pavé.

La propriété est réservée à `CertifiedBall` (`guard.cpp:38`, constructeur
privé dans `guard.hpp`) : supports q1–q4 certifiés, y compris leurs coquilles
étendues. `index/bounds.hpp` sépare `GuardedBounds(CertifiedBall)` et
`GenericBounds(Sphere)` ; les deux API census suivent cette séparation.
`tower/supports.cpp:131,140` certifie aussi les boules issues du catalogue.
Une candidate/une sphère générique n'est pas admissible : l'oracle conserve
le contre-exemple obtus du contrat, avec un contact hors pavé. Le census des
feuilles catalogue CPU/CUDA est un autre corps ; aucune qualification ni
accélération du catalogue GPU n'est déduite de ce changement.

## Qualification à adapter avant intégration

Les anciens tests restent des preuves de l'ancienne garde. Les coordonnées
`(3M−1)^3` sont désormais hors pavé : conserver leur réponse géométrique,
mais ne pas continuer à en exiger un calcul large. Sont concernés
`tests/num/guard_test.cpp:68,114,183` (sites, boîtes, certificat, boucle de
paliers) et `tests/index/guarded_test.cpp:158` (ancien coin large en census).
Ne pas simplement supprimer les contrôles de voies.

Un nouveau témoin exact préserve le débordement **intermédiaire malgré une
valeur finale représentable** : `h=2^21−1`, support
`{(0,0,0),(h,h,0),(h,0,h)}`, requête `v=(h,h,h)`. Tous les points sont u21,
la requête dans le nouveau pavé. Centre `(2h/3,h/3,h/3)`, poids `1/3`,
`R²=2h²/3`. Dans la représentation non réduite de cette famille,
`D=6h^4`, `N=(4h^5,2h^5,2h^5)` : `D|v|²=18h^6` a **131 bits**, tandis que
la puissance finale `2h^6` a **127 bits**. `D,N` tiennent en i128, donc
`Lane::checked` puis repli large sont requis. Le `power_domain` de ce témoin
est **17** : il ne prouve pas une erreur du certificat au domaine `s=21`.
Dans un futur test natif, vérifier coefficients, calcul indépendant, résultat
positif et compteur wide ; le petit census doit conserver l'égalité avec la
voie générique. Aucune exécution native de ce témoin n'est revendiquée ici.

Deux mutations sont des **écarts de politique conservatrice**, pas des
fautes arithmétiques après resserrement : `certificat_du_support_seul` et
`garde_un_bit_trop_etroite` (`tests/mutants/num.json:594,602`). Reclasser leurs
notes et les commentaires de `guard_certificate`, sans annoncer un meurtre
causal de débordement. Les motifs des mutations `pave_haut_trop_court` et
`pave_bas_trop_court` doivent être réancrés ; leurs contre-exemples de sphère
débordant le support restent valables. Le patch proposé n'est donc pas une
livraison autonome avec sa qualification déjà acquise.

Pourquoi même le domaine `s` devient sûr ici (lemme secondaire, **non appliqué**) :
`D<2^(123−2s)` résulte du certificat. Posons `A=2^123`, `w=c−o=N/D`, `v=x−o`.
Le nouveau pavé donne `|v_j|,|v_j−w_j|<2M`, `|w_j|<M`. Chaque somme partielle
du code vaut `D[Σ_axes traités((v_j−w_j)²−w_j²)+Σ_autres v_j²]`, donc est
dans `(-3DM²,12DM²)⊂(-3A,12A)⊂i128`. Le produit initial est inférieur à
`12A` et chaque produit linéaire à `4A` en valeur absolue. Cette preuve dépend
de `CertifiedBall`, de la nouvelle garde et de l'ordre réel des opérations.
Elle ne qualifie pas un certificat générique plus faible, une candidate, ni
une future réduction des types. Une promotion de voies serait une seconde
tranche à déclarer et mesurer séparément. Contrelecture indépendante root/E
favorable ; aucune voie du patch n'utilise ce lemme secondaire.

Distinction historique : le domaine `s+1` était **déjà sûr avec l'ancien
pavé**. Les bornes deviennent `|x−c|,|x−o|<3M`, partielles dans
`(-3DM²,27DM²)` ; le certificat `s+1` donne `DM²<2^121`, donc
`27DM²<2^126`, chaque produit linéaire étant inférieur à `6DM²`. L'ancien
mutant `garde_un_bit_trop_etroite` tuait déjà une politique de voie, sans
prouver une faute arithmétique. En revanche, l'ancien mutant au domaine `s`
était réellement dangereux : le témoin span20 et son ancien coin le montrent.
Il devient sûr après resserrement. Les anciennes exécutions restent acquises,
mais cette distinction borne leur interprétation causale.
Le calcul préalable de la norme est également borné : `dot` en i64 n'est
choisi que si `s+2≤30`, donc `s≤28` et même l'ancien pavé donne
`|v|²<27·2^56<2^63`. Au-delà, `dot128` suffit pour `s≤32`. Aucun changement
de cette condition n'est proposé.

## Oracle borné et mesure proposée

`model.py` reconstruit les sphères par Gram rationnel et signes barycentriques,
puis compare ancienne garde, nouvelle garde et extrema entiers indépendants.
**159 supports**, dont 43 certifiés et 116 refusés ; arités 1–4, singletons et
extrêmes u32 ; **1 617 points, 2 470 boîtes**, faces incluses. Dans ce modèle
choisi pour exercer les frontières : 750 puissances de points évitées,
734 nouveaux rejets de boîtes et 352 majorants évités. Ces comptes ne sont
pas un profil LiDAR ni une estimation de gain. S'ajoutent 512 cas rationnels
normalisés pour les sommes partielles, le contre-exemple non certifié et le
témoin wide. Lecture des treize hashes avant/après et `git apply --check` dans
un répertoire temporaire ; aucun fichier produit modifié. Rejeu normal et
`-O` identique, environ une demi-seconde par lecture dans cet environnement.

```sh
python morsehgp3D_v12/receipts/audit_reponses_20261008/garde_census/model.py --repo /workspaces/E-HGP
python -O morsehgp3D_v12/receipts/audit_reponses_20261008/garde_census/model.py --repo /workspaces/E-HGP
```

Pour un futur microbanc, pré-déclarer deux bras ne différant que par les
bornes, mêmes trames complètes/cohortes J, K5 puis K10 informatif, W1/W48 et
capacités résidentes identiques. Les compteurs diagnostiques séparent
nouveaux rejets de points, nouveaux rejets de boîtes, majorants évités et
voies physiques ; exiger identité du travail logique et de l'objet.
**G total non instrumenté** décide : dix processus appariés et dix passes
par bras, première passe publiée à part, ordre équilibré, mêmes exclusions
pour les deux bras, intervalles calculés au niveau processus (pas les passes
traitées comme prises indépendantes). Si la promotion des voies est ensuite
explorée, isoler ancien pavé/domaine `s+2`, ancien/`s+1`, nouveau/`s+2` et
nouveau/`s`, ou procéder séquentiellement ; aucun de ces autres bras n'est
inclus dans le patch de bornes seules. Proposition de seuil d'adoption : borne
haute unilatérale à 95 % du ratio apparié G W48 inférieure à 1 sur chacune
des trois trames ; sinon gain non démontré. Publier aussi maximums et W1,
refuser toute régression inexpliquée ; un résultat positif resterait limité
au census/G mesuré, sans clôture FULL. Ce protocole n'a pas été joué.
