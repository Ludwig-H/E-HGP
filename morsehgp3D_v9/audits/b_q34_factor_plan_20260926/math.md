# Audit mathématique du plan par facteurs q3/q4

26 septembre 2026 ; code lu `92c709bc8f48aedd09d5c5f1ce3a283598fe23bc`.
Audit indépendant du prototype C++ voisin. Profil entier 1 mm/u18,
`not_claimed`. Aucun moteur, protocole ni GCP modifié.

## Verdict

Le prédicat universel à huit coins est correct **sur la boîte continue**.
Les crédits de facteurs s'additionnent sans double compte si leurs témoins
restent respectivement dans A privé de a et B privé de b, pour deux facteurs
disjoints. Avec cœur h=0, le plan peut rejeter q3 à K−1 témoins et q4 à K−2.
Ces certificats sont des rejets sûrs, pas des comptes exacts de profondeur.

Le petit Pool projeté peut néanmoins manquer **tous** les témoins utiles
d'une ancre, même à s12. Son coût linéaire à K fixé ne garantit donc pas un
résidu sous-quadratique. Il faut mesurer préparation, couples de classes,
paires réellement développées et tout l'aval. C'est un port du mécanisme P0
déjà étudié, pas une nouvelle preuve globale.

## 1. Pourquoi le citron est intérieur à toute boule propriétaire pertinente

Fixons une boule de support positif q=3 ou q=4, de centre c et rayon R,
et une arête ab de longueur maximale L dans ce support. L'ordre lexical
qui départage les égalités sert à l'unicité, pas à cette inégalité.
Écrivons c comme combinaison convexe des q sommets, de poids λ_i. Alors

`R² = ½ Σ_ij λ_i λ_j |p_i−p_j|² ≤ (L²/2)(1−Σ_i λ_i²)`.

Donc `R²≤L²/3` à q3 et `R²≤3L²/8` à q4. Au milieu m de ab,
c=m+w avec w perpendiculaire à ab et `R²=L²/4+|w|²`. Pour un témoin z :

`H=(z−a)·(b−z)=L²/4−|z−m|²`,

`Xi=|(z−a)×(b−z)|²=L²·|z_perp|²`,

`power(z)=−H−2w·z_perp ≤ −H+sqrt(Xi/alpha_q)`,

où alpha_3=3 et alpha_4=2. **H>0 et alpha_q H²>Xi** impliquent donc une
puissance strictement négative. La séparation WSPD n'est pas nécessaire
à cette implication ; elle influence seulement l'efficacité du certificat.
Les autres sommets du support sont sur la coquille et ne peuvent donc pas
être comptés comme ces intérieurs stricts.

L'hypothèse d'arête de longueur maximale est essentielle. Contre-exemples
exacts avec a=(-1,0,0), b=(1,0,0), tous poids du centre positifs :

- q3 : support supplémentaire (0,10,0), témoin (0,−1/2,0) ; témoin du
  citron ab mais extérieur à la boule réelle ;
- q4 : sommets supplémentaires (0,10,±1), témoin (0,−1/4,0), même défaut.

Ces cas sont testés par Fraction. On peut multiplier et translater toutes
les coordonnées pour rester dans u18. Le plan ne doit pas supprimer une
présentation qui emploierait ab comme arête quelconque ; le raccord doit
conserver la propriété existante du générateur.

## 2. Pourquoi huit coins suffisent — et pourquoi le centre ne suffit pas

À a,z fixés, H est affine en b et `(z−a)×(b−z)` est linéaire en b.
La fonction `sqrt(alpha_q) H − norme(cross)` est concave. Son domaine
strictement positif est convexe ; c'est exactement le prédicat ci-dessus.
Tout point d'une boîte est combinaison convexe de ses huit coins. Si tous
les coins sont strictement dans ce domaine, leur combinaison l'est aussi.
Réciproquement, la boîte entière contient ses coins. Les huit tests sont
donc nécessaires et suffisants pour **cette boîte**, boîtes dégénérées
incluses. Pour un témoin de tout A×B, appliquer la convexité séparément
en a puis en b ; aucune convexité conjointe n'est supposée.

Cela ne signifie pas que les coins soient de vrais sites. Exemple a=0,
z=(1,1,0), avec A={a,z}, à séparation s12 :

| voie | deux sites de B | résultat |
| --- | --- | --- |
| q3 | (1000,−266,0), (1020,−271,0) | vrais sites et centre passent ; un coin croisé échoue |
| q4 | (1000,−170,0), (1020,−173,0) | même phénomène |

Traduire tout par (1000,1000,1000) donne des coordonnées u18. Ces exemples
montrent à la fois le défaut d'un mutant « centre seulement » et la perte
possible d'efficacité de la boîte continue face au facteur discret.
Augmenter s ne rend pas ces deux notions identiques.

Les égalités ne sont jamais créditées. Fixtures entières avec a=0 :
q3 b=(-3,−3,0), z=(-2,−1,−1) donne H=3, Xi=27 ; q4 b=(-3,−3,−2),
z=(-2,−2,0) donne H=4, Xi=32. Le mutant `>=` les accepterait. Omettre
H>0 accepte aussi z=(11,0,0) pour b=(10,0,0), clairement extérieur.
L'arithmétique u18 doit rester celle certifiée du produit, élargissement
avant multiplication inclus ; le raisonnement ne qualifie pas un port f32.

## 3. Addition, seuils et conservation des voies

Chaque crédit h_a est le nombre **minoré et saturé** d'IDs distincts
dans A privé de a, universels sur la boîte B ; h_b est défini symétriquement.
Pour une paire (a,b), ces deux ensembles sont disjoints et chacun est
intérieur à toute boule propriétaire pertinente. Leur somme minore la
profondeur d. Pour les objets de la tour, d+q≤K+1 ; les rejets sûrs sont
donc d≥K−1 à q3 et d≥K−2 à q4.

- h=0 est parfaitement sûr et évite toute obligation de fusion avec un
  cœur extérieur. Un cœur non nul ne s'ajoute que si sa provenance assure
  des témoins distincts, hors A∪B, certifiés pour le même produit. Ne pas
  additionner aveuglément un compte partiel hérité d'une autre recherche.
- Une même source de témoins ne s'ajoute pas deux fois : Pool+Tubes ou
  proches+octants demandent déduplication ou une preuve de disjonction ;
  à défaut, prendre le maximum de deux minorants, pas leur somme.
- La saturation séparée à T=K+2−q préserve le test `h_a+h_b≥T`. Elle
  n'autorise pas à précharger le census global, qui pourrait recompter
  les mêmes témoins. Il repart de zéro.
- Traiter les voies inactives avant l'arithmétique non signée : q3 est
  inactive à K1, q4 à K1/K2. Un seuil nul sur une voie inactive n'est pas
  un rejet géométrique à transporter comme certificat.
- Un témoin q4 est aussi un témoin q3. Si les propositions sont communes,
  ce fait évite un second prédicat q3 réussi, mais les **rejets de voies
  ne sont pas emboîtés**, car leurs seuils diffèrent. Ne jamais terminer
  les deux collectes dès qu'une seule sature.
- Fermer q3 n'autorise pas à supprimer les graines nécessaires à q4 :
  le masque résiduel garde séparément les deux voies et le producteur
  conserve ses règles de positivité/propriété/canonicité.

Contre-exemple au double compte : tétraèdre régulier
`(1,1,1),(1,−1,−1),(−1,1,−1),(−1,−1,1)`, unique intérieur z=(1,0,0).
Pour l'arête des deux premiers sommets, z est un témoin strict q4. Le
créditer dans A et de nouveau dans le cœur donne 2 au lieu de 1 et rejette
à tort la boule admissible à K4. Le caractère « certifié » de chaque
crédit pris isolément ne suffit pas à justifier leur addition.

## 4. Regrouper deux voies sans développer deux fois une paire

Grouper chaque facteur par le couple `(c3,c4)`, avec permutation stable
des **IDs/rangs explicitement distingués**. Chaque produit de deux classes
non vides apparaît une seule fois et porte le masque

`masque_parent & (2·[a3+b3<K−1] | 4·[a4+b4<K−2])`.

Un masque nul supprime ce produit ; les autres masques valent 2, 4 ou 6.
Les classes sont des partitions disjointes et couvrantes, donc cette règle
ne perd ni ne duplique une paire. Concaténer une liste de bandes q3 et une
liste q4 développerait deux fois leur intersection. Une autre option est
de partitionner explicitement « q3 seul / q4 seul / les deux », pas deux
ensembles superposés traités indépendamment.

À K≥3, il y a au plus `K(K−1)` classes par facteur, davantage limitées par
sa population. Si les propositions communes assurent c4≤c3, certaines
classes sont impossibles, mais la sûreté ne doit pas dépendre de cette
optimisation. Tester seulement les couples de classes non vides coûte
O(C_A C_B), pas O(|A|+|B|) en général. Même à K10, matérialiser jusqu'à
8 100 couples par rectangle serait un mauvais défaut sur des millions de
rectangles. Compter `cells_tested`, descripteurs et résidu ; envisager une
énumération différée des régions utiles si ce poste apparaît réellement.

Le Pool paie O(KF), F=Σ_rectangles(|A|+|B|), plus O(ΣC_A C_B) et le vrai
travail aval sur les paires résiduelles. Ni F ni ce résidu ne sont bornés
globalement par cette preuve. Ne pas refaire un plan pour chaque ancre,
copier le nuage, ou réordonner globalement l'index emprunté par d'autres
tâches. Le plan et ses permutations doivent vivre jusqu'à la consommation
des descripteurs ; une future file GPU requiert une propriété explicite.

## 5. Ce que le Pool peut manquer et les alternatives déjà disponibles

Contre-fixture Pool, K5/s12 :

`A = {(0,0,0)} ∪ {(x,0,0):x=1..4} ∪ {(x,100,0):x=5..9}`,
`B = {(10000,0,0)}`.

Pour a=0, les quatre premiers sites axiaux sont des témoins universels.
Les cinq plus grandes projections vers B sont pourtant les points de la
rangée y=100, tous faux témoins q3/q4 pour cette ancre. Le Pool donne zéro,
l'exhaustif saturé donne quatre à q3 et trois à q4. La séparation s12 est
vérifiée exactement par l'oracle. Ce n'est pas un défaut d'exactitude :
les paires doivent continuer dans la voie ordinaire.

Structurellement, plusieurs rails, nappes ou groupes transverses peuvent
réserver les meilleures projections à une seule zone. Les autres ancres
ont des successeurs utiles dans leur voisinage mais pas dans le Pool.
La grande distance entre facteurs ne répare pas nécessairement ce défaut
angulaire local. Pour deux nappes exactement perpendiculaires à la
direction de séparation, certains témoins de même nappe ont H≤0 : aucune
meilleure sélection ne créera alors des crédits inexistants. Sur LiDAR,
occlusions, feuilles minces et trous entre directions de scan peuvent
combiner sélection pauvre et boîtes trop conservatrices ; c'est une
motivation de mesures spatiales, pas une preuve d'échec général du LiDAR.

Ne pas redécouvrir les variantes historiques :

- `src/gen/pipeline/tube_credits.hpp` prépare des cellules transverses,
  projections triées et suffixes comptés en bloc : préparation O(m log m),
  balayage O(m), sous D≥10R. Le test entier est d²≥100·diam², pas 25.
  Les marges certifiées sont q3 Q≤Δ², q4 16Q≤9Δ² et Δ>0. La préparation
  commune aux voies existe déjà ; elle doit être portée sur les nœuds du
  même index, pas appelée avec une copie du nuage par rectangle. Cellules
  trop fines ou trop larges peuvent laisser un mauvais résidu.
- Le [shadow octant](../SHADOW_HA_OCTANT_Q34_LIDAR_20260923.md) propose des
  proches directionnels par ancre, puis utilise **le même test exact**.
  À 08/000000 K5/s8 il évitait 5,73 M des 23,69 M paires ; ces temps CPU
  hors chaîne et son sidecar dense ne sont pas une qualification GPU.
  Il apporte une diversité spatiale que le Pool global ne garantit pas.
- `DualBlocks` évite une matrice initiale, mais son parcours exhaustif des
  couples ancre/témoin peut rester quadratique. Ne pas l'introduire comme
  repli systématique censé résoudre le coût P0.

## 6. Vérification indépendante et gates conseillées

`math_oracle.py` n'importe aucun code produit. Les boules et barycentres
sont résolus en Fraction ; Xi est un déterminant de Gram, pas le produit
vectoriel du moteur. Normal et `−O` passent avec résultats identiques :
17 576 requêtes sur huit arêtes maximales, 296 témoins stricts effectivement
testés contre la puissance réelle ; 136 boîtes certifiées et 3 672 points
rationnels intermédiaires ; 19 664 produits de classes, 13 430 masques
résiduels K3..10. Les deux échecs Pool, les deux coins trompeurs, les deux
arêtes non propriétaires, H≤0, auto-témoin, égalités strictes, cœur
recouvrant et les masques non emboîtés sont exercés. Les tests finis
accompagnent les preuves ci-dessus, ils ne les remplacent pas.

Premier lancement normal/−O échoué, conservé ici dans l'historique : le
nouvel oracle avait écrit `5−q` au lieu de `6−q` pour alpha_q ; sa propre
fixture de centre l'a rejeté. Correction de l'oracle seulement, aucun
défaut produit déduit de cet échec. SHA-256 final du script :
`a5b943a64c202d4df30a9ebcc04ed353e05f57cb1bbec7979a48a5ff8d945f44`.

```sh
python3 -B morsehgp3D_v9/audits/b_q34_factor_plan_20260926/math_oracle.py
python3 -B -O morsehgp3D_v9/audits/b_q34_factor_plan_20260926/math_oracle.py
```

Pour la gate C++ indépendante : ajouter K1/K2 inactifs, h=0, facteurs
singleton/petits ignorés par optimisation, scores égaux départagés par
ID, permutations où ID≠rang, facteur entier saturé et absence de toute
réduction. Comparer **chaque masque résiduel** au témoin, chaque produit
énuméré une fois ; juger ensuite supports/catalogue/FULL. Mutants prioritaires
communiqués à l'auteur : centre à la place des huit coins, `>=` aux
frontières, H>0 omis, crédit h_a ajouté deux fois, seuil q3/q4 inversé,
arrêt conjoint trop tôt, union q3/q4 concaténée et confusion rang/ID.
Aucun nouveau contrat G4 ou sous-quadratique global n'est acquis.
