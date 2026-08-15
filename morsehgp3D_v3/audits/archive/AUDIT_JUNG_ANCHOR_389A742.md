# Audit ciblé — clipping de Jung et ancre non diamétrale

Date : 9 août 2026 UTC.

Cadre du dossier : `phase=exploration_v3_hors_registre`, `backend=cpu_reference_bounded_oracles_and_g4_diagnostic`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`. Sous-portée historique du snapshot : dictionnaire de profondeur ancré par arête, probe Release CPU et comparaison à un oracle rationnel exact; aucun code du dépôt n'avait été modifié, les probes correctifs étaient restés sous `/tmp`.

## 0. Snapshot et verdict

Le snapshot audité est le commit complet `389a7428c88d9dede7a9c767634774b9ea842ca0`.

| Fichier | SHA-256 du blob commité |
| --- | --- |
| `prototype/edge_shallow.hpp` | `43992a786bbed0c6ff1877f39b828ae8442cf77cf7bb9a1df5306c0f861f91b1` |
| `oracle/oracle_main.cpp` | `ed0fe1c1b86a5d0b4dd1a96a6ab00ccd094f0dbd1f3e5abcff83b27029989dbc` |
| `CMakeLists.txt` | `384b940d52b883a98f06657389bc7da8ec5474dcdef4519d7c80e3aa733e0874` |

> [!CAUTION]
> **P0 de qualification : l'hypothèse diamétrale utilisée par le clipping de Jung n'est pas imposée aux deux carriers.** Le masque vérifie séparément que chacun est dans la lentille de `pq`, mais le code ne vérifie pas leur distance mutuelle. Une fixture entière, bien centrée et dans `RelevantGP` fait alors classer un vrai témoin intérieur comme « extérieur constant », calcule le rang 4 au lieu de 5 sur cette ancre et incrémente fatalement `dictionary_refuted`.

La fixture ne prouve pas une erreur du catalogue exhaustif du snapshot : l'énumération de toutes les paires retrouve le même support depuis sa véritable arête maximale. Elle prouve en revanche que le sujet utilisé pour « vérifier le dictionnaire » applique ce dictionnaire hors de son hypothèse, rougit à tort sur une entrée valide et publie des compteurs d'ancres diamétrales faux.

## 1. Rupture exacte de l'implication « deux points dans la lentille, donc ancre diamétrale »

Le contrat mathématique de `PROPOSITION.md` §6.3 commence par une **ancre diamétrale** `e=pq`. L'ellipse $J_e^{(4)}$ est justifiée par Jung seulement lorsque $D=\lVert p-q\rVert$ est le diamètre du support considéré.

Le prototype dit la même chose dans son commentaire aux lignes 237--240, puis construit pourtant `in_lens` aux lignes 268--270 par les deux seuls tests individuels $\lVert x-p\rVert^2\leq D^2$ et $\lVert x-q\rVert^2\leq D^2$. Au sommet de deux droites, les lignes 464--470 demandent que les deux bits `active_lens` soient vrais, mais ne testent jamais $\lVert x-y\rVert^2\leq D^2$.

Cette implication est fausse : la lentille de deux boules de rayon $D$ peut avoir un diamètre strictement supérieur à $D$. Pour deux carriers `x` et `y`, les cinq comparaisons contre `p` ou `q` ne contrôlent pas la sixième arête `xy`.

La conséquence est plus grave qu'un compteur imprécis. `classify(4)` retire auparavant toute droite ne coupant pas $J_e^{(4)}$ et la traite comme constante sur cette ellipse, aux lignes 275--294. Si le croisement des carriers est hors de l'ellipse parce que `pq` n'est pas diamétrale, le signe d'une droite retirée peut être différent au croisement. Le calcul $4+c_e+\delta_e$ n'a alors plus autorité.

## 2. Fixture u16 exacte

Dans l'ordre des identifiants, prendre :

```text
p = 0 = ( 5, 10, 10)
q = 1 = (15, 10, 10)
x = 2 = (10, 16, 16)
y = 3 = (10,  4, 16)
w = 4 = (10, 10, 20)
```

Pour l'ancre `pq`, $D^2=\lVert p-q\rVert^2=100$. Les deux carriers passent séparément le masque :

$$\lVert p-x\rVert^2=\lVert q-x\rVert^2=\lVert p-y\rVert^2=\lVert q-y\rVert^2=97\leq100.$$

Mais leur distance mutuelle vaut :

$$\lVert x-y\rVert^2=144>100.$$

`pq` n'est donc pas diamétrale pour le support `pqxy` ; sa véritable arête maximale est `xy`.

### 2.1 Sphère critique et bon centrage

La sphère portée par `pqxy` a le centre et le rayon carré exacts :

$$C=\left(10,10,\frac{167}{12}\right),\qquad \rho^2=\frac{5809}{144}.$$

Ses coordonnées barycentriques dans l'ordre `(p,q,x,y)` sont :

$$\lambda=\left(\frac{25}{144},\frac{25}{144},\frac{47}{144},\frac{47}{144}\right).$$

Elles sont toutes strictement positives : le tétraèdre est bien centré. Le cinquième point est strictement intérieur :

$$\lVert w-C\rVert^2=\frac{5329}{144}<\frac{5809}{144},\qquad \rho^2-\lVert w-C\rVert^2=\frac{10}{3}.$$

Le shell exact est donc `{p,q,x,y}` et le rang fermé réel vaut 5.

### 2.2 Le centre sort exactement de l'ellipse de Jung de `pq`

Le milieu de `pq` est $M=(10,10,10)$. L'ellipse de l'arité quatre impose $\lVert C-M\rVert^2\leq D^2/8$, ou de façon équivalente $\rho^2\leq3D^2/8$. Ici :

$$\lVert C-M\rVert^2=\frac{2209}{144}>\frac{1800}{144}=\frac{D^2}{8}.$$

$$\rho^2=\frac{5809}{144}>\frac{5400}{144}=\frac{3D^2}{8}.$$

Ce n'est pas une erreur du théorème de Jung : son hypothèse est simplement fausse pour l'ancre courte `pq`.

### 2.3 Le clipping retire précisément le témoin qui fixe le rang

Avec la base entière choisie par le code, $b_1=(0,0,10)$ et $b_2=(0,-10,0)$. Les formes `(a,b,c)` sont :

```text
x : (120, -120, 188)
y : (120,  120, 188)
w : (200,    0, 300)
```

Les formes de `x` et `y` sont actives et se croisent en $s=(\frac{47}{30},0)$, qui représente exactement le centre $C$. Pour `w`, la matrice de Gram donne $\det(G)=10000$ et $Q=4000000$. Le test de croisement de l'ellipse compare :

$$c_w^2\det(G)=900000000>800000000=2D^2Q.$$

La droite de `w` ne coupe donc pas l'ellipse. Comme $c_w=300>0$, les lignes 286--290 la classent « extérieure constante » et la retirent de la profondeur. Pourtant, au croisement des carriers :

$$a_ws_1+b_ws_2-c_w=200\frac{47}{30}-300=\frac{40}{3}>0.$$

`w` est bien strictement intérieur à la sphère correspondante. Le prototype obtient `constant_inside=0`, `depth=0`, donc `rank=4` ; le census terminal trouve cinq membres et incrémente `dictionary_refuted` aux lignes 491--493.

## 3. La fixture satisfait `RelevantGP`

Un probe indépendant en rationnels Python `Fraction` a énuméré les 26 sous-ensembles de tailles 2 à 5. Pour chacun, il a reconstruit toutes les sphères de supports de tailles 1 à 4 par élimination de Gauss, sélectionné la miniball couvrante de rayon minimal, contrôlé l'unicité du support propre et classé exactement tous les points extérieurs.

Résultat pour $K_{\mathrm{eff}}=4$ et $s_{\max}=5$ :

```text
subsets_checked=26
proper_miniballs=26
RelevantGP_violations=0
target_support=(0,1,2,3)
target_barycentric=(25/144,25/144,47/144,47/144)
target_sides=(shell,shell,shell,shell,inside)
```

En particulier, pour chaque sous-ensemble propre dont la miniball ne contient aucun point extérieur strict, aucun point extérieur n'est sur le shell. Les cinq points sont distincts et le support cible est affinement indépendant. La symétrie de la fixture ne masque donc pas une dégénérescence pertinente.

Le même résultat est confirmé par le juge du dépôt : `attempted=1`, `decided=1`, `rejected_domain=0`, comparaison du catalogue et des quatre forêts sans mismatch. Le seul verdict rouge vient ensuite du compteur `dictionary_refuted=1`.

## 4. Reproduction dynamique contre l'oracle exact

Une copie de `oracle_main.cpp` sous `/tmp` a reçu uniquement la fixture littérale ci-dessus ; elle a été compilée contre le snapshot hashé. Commande :

```sh
/tmp/mhgp3v-jung-anchor.UUlAlw/oracle_jung \
  --fixture jung_nonmax_anchor --subject edge_shallow \
  --clouds 1 --min-points 5 --max-points 5 \
  --min-order 4 --max-order 4 --coord-max 20 \
  --min-decided 1 --min-nodes 1
```

Sortie pertinente :

```text
aretes=10 dont retenues=3 | droites actives=19
sommets examines=10 dont peu profonds=10
arite2 emise=10 arite3 emise=10 arite4 emise=5
DICTIONNAIRE REFUTE=1
attempted=1 decided=1 rejected_domain=0
spheres=27 forets=4 noeuds=33
ECHEC : dictionnaire rang = 4 + c_e + delta_e REFUTE 1 fois
exit_code=1
```

Le probe direct isole les deux ancres :

```text
anchor=01 D2=100 target=0 rank=-1 refuted=1 retained=0
anchor=23 D2=144 target=1 rank=5 refuted=0 retained=1
CATALOGUE target=1 rank=5 refuted=1
```

Avec les quatre premiers points seulement, l'absence du témoin `w` fait coïncider accidentellement le rang calculé et le rang réel. Le même support est alors émis depuis l'ancre non maximale et le compteur la dit retenue :

```text
FOUR_POINT_NONMAX anchor=01 target=1 retained=1 refuted=0
```

Cela réfute aussi la documentation de `edges_retained` (« arête diamétrale d'au moins un support ») : une simple absence de `dictionary_refuted` ne prouve pas que le contrat d'ancrage était satisfait.

## 5. Portée exacte du finding

Ce que la fixture établit :

- le masque individuel de lentille n'est pas un certificat d'arête diamétrale pour un support d'arité quatre ;
- le clipping de témoins par $J_e^{(4)}$ est appliqué hors de son domaine ;
- une entrée u16 bien centrée et `RelevantGP` peut faire échouer la porte du dictionnaire du snapshot alors que son oracle et son catalogue exhaustif concordent ;
- `dictionary_refuted` mélange une réfutation de l'identité sous ses hypothèses avec une violation préalable de ces hypothèses ;
- `edges_retained` peut compter une ancre non diamétrale.

Ce que la fixture n'établit pas :

- elle ne réfute pas l'identité $4+c_e+\delta_e$ lorsque `pq` est réellement diamétrale ;
- elle ne montre pas de support absent du catalogue exhaustif du snapshot, car l'énumération de toutes les paires fournit l'ancre maximale `xy` ;
- elle ne justifie pas de supprimer du flux témoin les points hors lentille : `w` est hors de la lentille de `pq` mais reste précisément le témoin de rang nécessaire.

Cette distinction est importante : la correction doit agir sur l'**éligibilité de la paire de carriers**, jamais sur le flux témoin complet.

## 6. Correction minimale et validation sous `/tmp`

Pour un croisement porté par `x` et `y`, la condition manquante est :

$$\lVert x-y\rVert^2\leq D^2.$$

Les quatre comparaisons déjà garanties par les deux bits de lentille, cette sixième comparaison assure que toutes les arêtes de `pqxy` sont au plus $D$ ; `pq` est alors bien une arête de diamètre maximal et Jung s'applique. Le filtre doit arriver après le maintien du balayage, car chaque ligne reste un témoin pour les autres sommets, mais avant `vertices_examined`, le budget shallow, le calcul de rang, le census et `dictionary_refuted`.

Une variante minimale, appliquée uniquement à une copie sous `/tmp`, a ajouté ce test exact avant la ligne 465. Sur la fixture :

```text
anchor=01 target=0 refuted=0 retained=0 vertices=0
anchor=23 target=1 rank=5 refuted=0 retained=1
CATALOGUE target=1 rank=5 refuted=0
```

Le même oracle exact rend alors :

```text
attempted=1 decided=1 rejected_domain=0
spheres=27 forets=4 noeuds=33
DICTIONNAIRE REFUTE=0
OK : campagne fermee, structure complete comparee sur la grille declaree
exit_code=0
```

Cette expérience valide la réparation sur le contre-exemple ; elle ne remplace pas la suite complète ni une preuve du constructeur.

Une autre politique possible serait d'imposer un propriétaire canonique parmi les arêtes maximales du support. Un contrôle effectué seulement après le census est toutefois trop tardif : le rang aurait déjà été calculé avec un clip sans autorité et pourrait déjà avoir incrémenté `dictionary_refuted`. La précondition doit être contrôlée avant toute interprétation du croisement comme candidat.

## 7. Porte permanente demandée

Ajouter deux fixtures littérales distinctes :

1. les quatre points `p,q,x,y`, avec l'attendu que l'ancre `pq` n'émette pas le support et ne soit pas comptée comme retenue, tandis que l'ancre maximale `xy` l'émette au rang 4 ;
2. les cinq points `p,q,x,y,w`, avec l'attendu `RelevantGP=true`, `dictionary_refuted=0`, support `(0,1,2,3)` présent une fois au rang 5, membres `(0,1,2,3,4)`, oracle et forêts verts.

La fixture doit contrôler explicitement les six distances du support, la sortie de l'ellipse pour l'ancre courte, le signe de `w` au centre, les barycentriques, le shell complet et l'ancre propriétaire. Une campagne aléatoire verte n'exerce pas de manière fiable cet écart rationnel étroit.

## 8. Conclusion

Le clipping elliptique exact est algébriquement correct **sur une ancre diamétrale**. Le défaut est l'absence de certification de cette précondition au niveau du couple de carriers. Tant que la distance mutuelle n'est pas testée, `dictionary_refuted=0` n'est pas une preuve stable du dictionnaire et une entrée valide peut faire échouer la qualification pour une contradiction fabriquée par une ancre courte.

GCP non utilisé.
