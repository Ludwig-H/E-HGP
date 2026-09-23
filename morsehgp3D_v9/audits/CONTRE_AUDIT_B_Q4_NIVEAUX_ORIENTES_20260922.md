# Contre-audit B — niveaux orientés peu profonds q4

22 septembre 2026. Proposition mathématique indépendante, **non portée et non
qualifiée**. Aucun résultat FULL, GPU, G4 ou sous-quadratique sur une trame
entière n'en découle. Cette note complète
[`Q4_STRUCTURE_ET_BORNES.md`](Q4_STRUCTURE_ET_BORNES.md), § « Une borne
linéaire » et § « Si `s≈m` », sans modifier le moteur.

## Objet et borne de sortie

Fixer l'arête propriétaire `ab` et son cover **complet**. Soit `h` le nombre de
formes de sites non constantes, avec leurs IDs et multiplicités, `m≤h` le
nombre de droites géométriques distinctes et `g≤2m` le nombre de groupes
par droite **orientée** primitive, chacun portant son poids et ses IDs. Les
formes constantes négatives contribuent `p₀` ; les constantes nulles sont des
contacts. À l'ordre `K`, poser
`d=K−3−p₀`. Si `d<0`, aucun centre q4 n'est retenu sur cette arête. Sinon un
centre candidat est l'intersection de deux droites indépendantes de profondeur
stricte au plus `d`. La positivité du tétraèdre et la propriété de `ab` restent
des vérifications supplémentaires.

Sur chaque droite géométrique, prendre les deux sommets peu profonds extrêmes.
Chaque croisement distinct strictement entre eux apporte au moins un site
strictement négatif à l'une des extrémités. Il y a donc au plus `2d+2` sommets
peu profonds par droite, même avec concurrences, parallèles, sites de signes
opposés sur une droite et multiplicités. Ainsi `V≤m(d+1)`, et la somme des
incidences *site–sommet* de coquille est au plus `h(2d+2)` hors formes
identiquement nulles. Sur un nuage de sites géométriquement uniques, seules
les deux extrémités `a,b` ont une forme identiquement nulle pour cette arête ;
leurs contacts ajoutent `2V`. C'est une **borne de sortie**, non un algorithme de
construction. Elle prolonge par ce comptage direct le lemme 2.5 de
[Har-Peled–Sharir, *Depth contours in arrangements of halfplanes*](https://www.math.tau.ac.il/~michas/k_depth.pdf),
énoncé en position générale pour des demi-plans orientés arbitrairement.

## Construction proposée sur modèle de comparaisons exactes

1. Choisir un cisaillement entier inversible qui rend chaque droite graphe
   `v=r_i(u)` : parmi `g+1` directions entières, au plus `g` sont interdites.
   Séparer les groupes selon le signe de leur coefficient de `v`. Pour la famille
   `y_i>0`, les intérieurs sont sous la droite ; pour `y_i<0`, ils sont au-dessus.
2. Traiter les dégénérescences par une perturbation **vers le côté non négatif**
   de chaque groupe signé primitif, `f_i^ε=f_i+ε+ε^{i+2}`, avec `ε>0` formel
   et `i` l'indice de groupe distinct. Les poids/IDs ne sont pas perturbés
   séparément ; les constantes restent hors de l'arrangement. Les
   puissances propres brisent coïncidences et triples concurrences : le
   déterminant d'un triple comporte au coefficient de `ε^{i+2}` le produit
   croisé des normales des deux autres droites ; si le triple contient deux
   normales indépendantes, un de ces coefficients est non nul. Les droites
   originellement parallèles restent parallèles et ne créent pas de sommet.
   Les comparaisons se font par le premier coefficient non nul de polynômes
   clairsemés, sans substituer un `ε` flottant. C'est une *proposition
   spécialisée* de simulation symbolique, pas un résultat hérité de la
   [technique générale d'Edelsbrunner–Mücke](https://arxiv.org/abs/math/9410209).
3. Pour `q=d+1`, construire les `q` niveaux les plus hauts de la première
   famille et les `q` plus bas de la seconde. Chacune a `O(gq)` morceaux, et
   [Everett–Robert–van Kreveld](https://doi.org/10.1142/S0218195996000186)
   donne `O(g log g+gq)` pour ses premiers niveaux ordinaires en position
   générale. Celle-ci **autorise les parallèles** dans l'énoncé précis de
   [Halperin et al., annexe A](https://sarielhp.org/p/20/max_level/max_level.pdf),
   mais interdit les triples concurrences, déjà cassées ici ;
   [Chan, §1](https://tmc.web.engr.illinois.edu/vio.pdf) décrit explicitement
   leur combinaison pour les demi-plans d'orientations mixtes. Énumérer les
   sommets internes à ces niveaux, puis balayer les intersections de leurs
   morceaux. Une intersection entre familles sélectionnées a profondeur au
   plus `2d` dans leur union ; leur nombre est donc `O(h(d+1))` par le lemme
   précédent. Un balayage de segments coûte `O((N+I)log N)` pour `N`
   morceaux et `I` intersections
   ([Bentley–Ottmann, 1979](https://www.itseng.org/research/papers/topics/VLSI_Physical_Design_Automation/Physical_Verification/DRC/Geometric_Intersection_Problems/1979-Bentley.pdf)).
   Interroger les `q` chaînes de niveaux de chaque famille à chaque sommet
   permet de tester sa profondeur perturbée **non pondérée** en `O(q log g)`
   par sommet, sans scan des `h` formes. Elle fournit un surensemble du seuil
   pondéré : tout poids de groupe est au moins un.
4. Rabattre chaque sommet conservé sur l'intersection **originale** de ses
   deux droites, dédoublonner les centres rationnels, puis reporter les formes
   originales `f_i(c)≤0` ; elles donnent à la fois tous les intérieurs stricts
   et tous les contacts, avec leurs IDs. Deux index plans de reporting de
   demi-plans fermés après dualisation ont le coût classique `O(h log h)` de
   préparation, `O(h)` d'espace et `O(log h+r)` par requête de sortie `r`
   ([Chazelle–Guibas–Lee, théorème 3](https://www.cs.princeton.edu/~chazelle/pubs/PowerDuality.pdf)).
   Les égalités, points duaux confondus et multiplicités doivent être conservés
   dans le port exact. Un repli autonome plus simple partage les points duaux
   en blocs de `⌈√h⌉` et stocke l'enveloppe convexe de chaque bloc : un bloc
   dont l'enveloppe rencontre le demi-plan livre au moins un ID et peut être
   scanné ; la requête coûte `O(√h log h+√h r)` avec extrema exacts. Il reste
   sous-quadratique ici, même si l'index optimal n'est pas porté.

La perturbation conserve **exactement** les sommets originaux peu profonds
après rabattement. Pour un sommet original `c`, toutes ses formes de contact
sont positives en `c` après perturbation. Leur intersection non négative a un
intérieur près de `c` et, puisque deux normales sont indépendantes, au moins
un coin perturbé tendant vers `c`. Les formes originellement non nulles gardent
leur signe près de `c` : ce coin a la même profondeur stricte. Réciproquement,
tout site strictement négatif au rabattement d'un sommet perturbé reste
négatif au sommet perturbé pour `ε` assez petit ; la profondeur originale ne
peut donc excéder la profondeur perturbée. L'exemple `u,−u,v,−v` a un sommet
original isolé de profondeur zéro à l'origine : `f−ε` le perd, tandis que
`f+ε` crée un petit carré de profondeur zéro dont les coins se rabattent tous
à l'origine. Une perturbation générique non orientée n'est donc pas suffisante.

**Contrôle indépendant complémentaire du commit A `fa27bf31`.** Son oracle
`check_q4_outward_levels_20260922.py` couvre aléatoirement seulement
`d=0…2`. J'ai chargé ce blob en mémoire et appelé sa fonction `check_case`
sur 250 cas supplémentaires pour chacun des `d=3…7` : graine 230923,
4 à 12 formes entières tirées dans les bornes du code ci-dessous, quadruplet
antipodal tous les quatre cas et copie à coefficients triplés tous les six.
La fonction
compare les **ensembles** de centres originaux peu profonds et de centres
perturbés rabattus, sans `assert` désactivable. Reproduction, sans écrire
de fichier :

```bash
python3 - <<'PY'
import random
import subprocess
source = subprocess.check_output(['git', 'show', 'fa27bf31:morsehgp3D_v9/audits/check_q4_outward_levels_20260922.py'], text=True)
ns = {'__name__': 'audit_module'}
exec(source, ns)
rng = random.Random(230923)
counts = {}
for depth in range(3, 8):
    exact = perturbed = 0
    for case in range(250):
        forms = []
        for _ in range(rng.randrange(4, 13)):
            x, y = rng.randrange(-3, 4), rng.randrange(-3, 4)
            if x or y:
                forms.append((rng.randrange(-4, 5), x, y))
        if case % 4 == 0:
            forms += [(0, 1, 0), (0, -1, 0), (0, 0, 1), (0, 0, -1)]
        if forms and case % 6 == 0:
            forms.append(tuple(3 * value for value in forms[0]))
        a, b, _ = ns['check_case'](forms, depth, f'depth={depth}/case={case}')
        exact += a
        perturbed += b
    counts[depth] = (exact, perturbed)
print(counts)
PY
```

Résultat PASS, couples `(centres originaux, sommets perturbés)` :
`d=3: (2453,3098)` ; `d=4: (3730,4786)` ; `d=5: (4940,6452)` ;
`d=6: (5970,7746)` ; `d=7: (6112,8190)`. C'est un contrôle
**combinatoire local** de 1 250 cas, non un test du constructeur de niveaux,
du moteur q4, du cover ni d'une trame LiDAR.

Avec `d≤7` (`K≤10`), le schéma vise `O(h log h)` comparaisons géométriques
et `O(h)` mémoire **par arête**, hors construction du cover, test de positivité
et d'arête propriétaire sur la coquille, `q_min`, catalogue, intérieurs de FULL
et largeur en bits. Le repli par blocs donne au moins une cible
`O(h^{3/2} log h)` par arête pour l'exactification. Ces bornes supposent le
port correct de l'algorithme de niveaux, du balayage et des prédicats
symboliques ; **aucun de ces ports ni leur coût réel n'est qualifié**. Les
formes `f_i^ε` ne sont utilisées que pour trouver des sommets : seuls les
formes et centres originaux décident du census et de l'émission. Le cas
`s≪h` peut préférer le balayage par les seules `s` droites de graines déjà
proposé dans l'audit A. Une profondeur faible ne prouve jamais l'existence
d'un support q4 strictement positif propriétaire ; toutes les incidences de
coquille doivent être examinées par un oracle séparé.

## Coût de trame et contrelecture locale

Pour une voie qui **matérialise toutes les formes de chaque cover par arête**,
le travail total inclut `Σ_e cover_sites(e)` pour les lire/classer, puis
`Σ_e h_e`, ses tris/niveaux, l'expansion des arêtes WSPD, les sorties, la
déduplication BallKey et FULL. Le reçu 1 mm cité
dans [`Q4_STRUCTURE_ET_BORNES.md`, § coût global](Q4_STRUCTURE_ET_BORNES.md) donne
`Σ_e cover_sites(e)=2 778 563 938` sur `1 872 168` arêtes, pour `n=39 885`
sites. C'est une **population logique certifiée du moteur v8**, ni le nombre
de sites effectivement lus par son exécution, ni une mesure de `Σ_e h_e`.
On a `n²=1 590 813 225` : **la seule voie hypothétique qui lirait chaque
cover une fois par arête dépasserait déjà `n²` sur cette trame.** Ce n'est
pas un minorant universel du générateur ; une voie sous-quadratique doit
partager/élider ces covers ou réduire les arêtes, et mesurer sa somme entière.
L'algorithme local seul n'apporte donc aucune borne LiDAR ou G4 globale.

Deux nouvelles bornes de l'audit A résistent à la contrelecture :

- [`Q3_STRUCTURE_ET_BORNES.md:35`](Q3_STRUCTURE_ET_BORNES.md) : en posant
  `t=E/D∈(0,1)` et `r=|x−a|²/D`, l'acuité donne `r>t`, et `ab` maximale donne
  `r≤min(1,2t)≤3t−2t²`. Donc `ξ=(r−t)/(2(r−t²))` vérifie bien
  `0<ξ≤1/3`, égalités de longueur incluses. Pour `M=2¹⁸−1`, les bornes
  annoncées `<30M³`, `<234M⁴` et `<54M⁵` sont conservatrices :
  `D,|x−a|²≤3M²`, `|H_i|≤4M³`, `G≤9M⁴` donnent respectivement
  `|N_{boîte}|≤26M³`, `|N_{puissance}|≤210M⁴` et
  `|N_{centre réduit}|<30M⁵`. Elles ne justifient pas l'expression
  non réduite `D(|x−a|²−E)H_i` en i128. Cela prouve la sûreté locale de
  l'enveloppe, non une baisse du nombre de graines ni du census.
- [`Q4_STRUCTURE_ET_BORNES.md`, § guard `U_C`](Q4_STRUCTURE_ET_BORNES.md) : pour
  `T=K−2` gardes correspondant à des **sites distincts du même nuage**,
  `U_C=max_{g,c∈C}|g−c|²` sur une cellule fermée, `R²>U_C` imposerait
  `T` intérieurs stricts et rejetterait q4. Ainsi un nœud Z avec
  `gap²(C,box(Z))>U_C` ne peut porter ni support, ni intérieur, ni contact
  d'une boule q4 acceptée centrée dans `C`. L'égalité ne permet aucun
  retrait ; des gardes dupliquées ou prises dans un autre masque non plus.
  C'est un certificat de voie q4 et de cellule, non un compte de profondeur
  transférable à q3/q2. Sélection des gardes, production des cellules et
  frontière résiduelle restent à payer.

Gate discriminant avant tout claim : comparer l'inventaire exact de **tous**
les sommets peu profonds, profondeurs et coquilles au brute-force sur petits
nuages entiers (concurrences, parallèles, droites coïncidentes de signes
opposés, coquilles nombreuses, sommets isolés), sous permutation des IDs et
avec `K=3…10` ; puis mesurer sur les mêmes arêtes LiDAR `h,m,s`, sommets
perturbés/originaux, contacts, rejets positifs, temps/nœuds/covers,
`Σ_e h_e` et `Σ_e cover_sites(e)`. Le flux de toutes les présentations
cosphériques v8 et le catalogue FULL réduit sont deux sorties distinctes.
GCP non utilisé.
