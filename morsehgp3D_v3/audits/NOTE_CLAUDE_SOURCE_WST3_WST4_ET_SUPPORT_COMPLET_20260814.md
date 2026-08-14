# Note de Claude : la source d'ordre trois et quatre, et le support complet

Date : 14 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles`,
`profile=quantized_u16_input_only`,
`mode=mesure_exploratoire`,
`public_status=not_claimed`.

Cette note répond à
[`AUDIT_MINIBOULE_UNIQUE_RESIDUEL_SHALLOW_5809BD2_20260814.md`](AUDIT_MINIBOULE_UNIQUE_RESIDUEL_SHALLOW_5809BD2_20260814.md)
et rend compte de quatre implémentations nouvelles. Aucun de ces chiffres ne
qualifie un SLO. Les mesures sont locales, mono-thread, sur une machine
partagée à deux processeurs logiques.

## 1. Ce que je reçois de votre contre-audit

Votre réfutation de ma note précédente est acceptée sans réserve. `u<h`
n'exhibe aucun centre peu profond, la fixture à huit groupes couvrants a `u=0`
pour une profondeur huit, et l'inclusion Delaunay que j'en tirais ne suit pas.
Les sorties sont retypées `q2_midball_exact` et `q3/q4_universal_upper_window`,
la bande porte `modele=iid_NON_RECU`, et `--fenetre-exhaustive` remplace
l'intervalle contesté par un parcours complet des paires — à
`eight_clusters,n=1500`, `138 076` exactement là où l'échantillon annonçait
`139 126`.

Vos trois trous de porte sont fermés : `--exige-q4-ouvert` sans juge est refusé,
`collinear_seven` refuse toute cardinalité autre que neuf, et le terme
« cinquante-cinq témoins libres » est corrigé en candidats de base rejetés.

## 2. Le déplacement de quantificateur, implémenté

Votre section 6 et l'intuition de Louis convergent : une fois le support
complet, il n'y a plus de variable libre de centre. `Corner8BallDepth` code
cela.

Avec `O = det3(u,v,w)` et `J` le déterminant in-sphere translaté, `z` est
strictement intérieur si et seulement si `O*J < 0`. Aucun centre n'est formé,
aucune division n'est faite. Sur un bloc, les deux signes sont bornés
séparément — le produit n'est jamais formé, ce qui garde `i128` suffisant sous
u16.

La convexité fait le reste, exactement comme vous l'écriviez : à signe
d'orientation fixe, le coefficient de `||z||^2` dans `sigma*J` vaut `|O|>0`,
donc la forme est strictement convexe en `z` et son maximum sur une boîte
témoin est à l'un des huit coins.

Sur votre fixture u16 mise à l'échelle, le classifieur rend `ALL_INTERIOR` en
**huit tests de coin**, là où l'expansion demande `4096` supports et `32768`
couples support--témoin. Le contre-calcul ponctuel les confirme tous
intérieurs, et le pire `sigma*J` ponctuel vaut `-79114925100839940000`.

La contre-fixture de convexité est gravée : le tétraèdre
`(3,2,2),(1,2,2),(2,3,2),(2,2,3)` avec la boîte `[1,3]^3` a ses huit coins
extérieurs et son centre intérieur. `corners-outside-implies-none` meurt sur
`23 236` désaccords, `c8-accept-equality` sur trois cas de shell.

Deux mutants ne sont **pas** exercés par un tirage uniforme :
`c8-drop-corner` n'agit que si le huitième coin est le seul à échouer, et
`c8-norme-aux-coins` que si la boîte témoin chevauche un facteur support. Je
les laisse hors porte plutôt que de les compter morts.

## 3. `OwnedCK-WST3` : la source d'ordre trois existe et elle est exact-once

Chaque triangle appartient à son arête maximale. Si `ab` l'est, alors
`||x-a|| <= ||b-a||` et `||x-b|| <= ||b-a||` : le troisième sommet vit dans la
lentille. Sur un rectangle entier, majorer par `Dmax(A,B)` et remplacer les
endpoints par leurs boîtes donne une condition nécessaire — et sans aucune
racine carrée, la distance à une boîte se séparant par axe.

Le juge énumère les `C(n,3)` triangles, détermine l'owner de chacun et exige
que son troisième sommet appartienne à **exactement un** bloc du rectangle
owner. Sur `uniform` et `eight_clusters` : `447 580` triangles, zéro manquant,
zéro doublon. `wst3-rayon-min` en laisse `145 236` non couverts et meurt.

La mesure a corrigé ma première construction. Exiger l'inclusion complète d'un
nœud dans la lentille force à raffiner jusqu'à sa frontière : le coût suit une
surface et le nombre de blocs redevient quadratique. En arrêtant la descente
dès que la cellule est petite devant le rayon du domaine — votre échelle liée
au rectangle — la couverture reste exact-once et le domaine n'est que
sur-couvert.

| `uniform`, `s=2` | 1000→2000 | →4000 | →8000 | →16000 |
|---|---:|---:|---:|---:|
| inclusion complète | 1,95 | 1,56 | 1,70 | 1,76 |
| échelle=16 | 1,50 | 1,64 | 1,66 | 1,23 |
| échelle=1 | 1,28 | 1,47 | 1,48 | **1,09** |
| rectangles WSPD | 1,07 | 1,18 | 1,24 | **1,01** |

À échelle grossière, l'ordre trois suit donc l'ordre deux à une constante près.

## 4. `OwnedCK-WST4` : exact-once, mais le compte n'est pas linéaire

L'ordre quatre est un produit et non une nouvelle recherche : pour un tétraèdre
d'arête maximale `ab`, les **deux** autres sommets sont dans la lentille, donc
les blocs `WST4` d'un rectangle sont les couples non ordonnés de ses blocs
`WST3`, diagonale comprise. L'exact-once suit sans preuve supplémentaire, les
blocs `WST3` étant disjoints ; le juge le vérifie tout de même sur les `C(n,4)`
quadruplets : `487 635` à `n=60`, zéro manquant, zéro doublon.

C'est le compte qui refuse.

| `s=2`, `échelle=1` | 500 | 1000 | 2000 | 4000 | 8000 | pentes |
|---|---:|---:|---:|---:|---:|---|
| `uniform` blocs WST4 | 1 694 496 | 6 159 060 | 17 106 052 | 56 643 872 | 187 359 319 | 1,86 / 1,47 / **1,73 / 1,73** |
| `eight_clusters` | 238 498 | 1 427 318 | 2 830 947 | 12 774 516 | 62 881 305 | 2,58 / 0,99 / **2,17 / 2,30** |
| masse logique | | | | | | **4,02 / 4,01 / 4,01 / 4,02** |

Deux lectures, et je ne veux pas les confondre.

La masse logique suit exactement `n^4`. C'est la confirmation directe de votre
verrou M4 : elle ne doit jamais être remplie, et la factorisation la représente
ici par un nombre d'enregistrements plus petit d'un facteur `2*10^7` à `n=8000`.

Mais le nombre d'**enregistrements** n'est pas linéaire non plus. La raison est
mécanique : `blocs4 = somme_t k_t (k_t+1)/2`, et `k_t`, le nombre de blocs
`WST3` par rectangle, croît lui-même — de `15,4` à `n=500` jusqu'à `30,4` à
`n=8000`. Son carré amplifie cette croissance. Sur les amas, la pente atteint
`2,30`.

### 4.1 Correction : la pente `1,73` était transitoire

J'ai poussé la rampe à `16 000` et `32 000` avant de vous envoyer cette note, et
elle corrige ma propre lecture :

| `uniform` | 1000 | 2000 | 4000 | 8000 | 16000 | 32000 |
|---|---:|---:|---:|---:|---:|---:|
| rectangles | 26 532 | 55 690 | 126 148 | 298 646 | 602 116 | 1 267 213 |
| blocs WST3 | 483 373 | 1 170 433 | 3 246 230 | 9 079 065 | 19 282 814 | 44 509 862 |
| blocs WST4 | 6,16e6 | 1,71e7 | 5,66e7 | 1,87e8 | 4,18e8 | 1,05e9 |
| `k_t` | 18,2 | 21,0 | 25,7 | 30,4 | 32,0 | 35,1 |

Les pentes de `blocs4` deviennent `1,47 / 1,73 / 1,73 / 1,16 / 1,33`. La valeur
`1,73` n'est donc pas asymptotique : elle appartient au régime où `k_t` monte
encore vers sa constante géométrique. Une boule de rayon `R` contient de l'ordre
de `(2*sqrt(3))^3 = 41,6` cellules de diagonale `R`, et `k_t` semble converger
vers ce voisinage — `35,1` à `n=32000`.

La conclusion honnête n'est donc pas « super-linéaire » mais : **quasi-linéaire
avec une constante inacceptable**. À `n=32000` la source rend `32 736` blocs
d'ordre quatre par point. Ce n'est pas l'exposant qui interdit le chemin
produit, c'est le facteur constant.

Le filtre d'acuité aide, sans suffire. Comme `E+X-D = 2H`, le prédicat
d'acuité est exactement celui de Thalès, donc déjà séparable et exact : un bloc
dont `max H <= 0` ne peut porter aucun carrier. Il élimine `55 %` des blocs
WST3, mais le couple survit dès qu'un seul de ses deux membres est aigu — le
`OR` du lemme, jamais le `AND` — donc le gain plafonne à `1,46x` puis `1,62x`.

## 5. Ce que cela déplace

Votre ordre plaçait l'élimination des blocs sans carrier aigu **avant** la
formation des couples de cellules. Ma mesure dit que ce n'est pas une
optimisation mais la condition de viabilité : `k_t` doit être réduit avant le
produit, sinon son carré emporte tout.

Je note aussi que trois certificats de paire successifs échouent pour la même
raison structurelle : évalués à chaque nœud visité, ils ne peuvent pas
économiser plus de visites qu'ils n'en coûtent. Sur `eight_clusters,n=3000` :

```text
SOC64 branche : E4 -18 %, temps +15 %
BlockJungDual : E4 -0,9 %, lectures inchangees, temps +8 %
HCBlockDepth  : recertifications -12,6 %, q3 -17,6 %, temps +55 %
MidballBlockDepth : recertifications -6,3 %, q2 -28 %, temps dans le bruit
```

Seul le dernier survit, et c'est le seul dont le prédicat est **exact** plutôt
que relaxé — le domaine des centres q2 étant réduit à un point.

J'ai aussi réfuté une coupure par borne supérieure : `cred + reste` majore
exactement le crédit atteignable et l'abandon est sûr, mais le gain est nul
(`31 538 327 -> 31 535 026`), le seuil de huit points étant dérisoire devant
les populations empilées.

## 6. Questions

**Q6.** Le filtre d'acuité avant le produit : sur un bloc `(A,B,C)`, quel
prédicat uniforme certifie qu'aucun triangle du bloc n'a de face aiguë incidente
à l'owner ? `E+X-D>0` se sépare-t-il par axe comme `H`, ou faut-il des bornes
d'intervalle comme pour `||C||^2` ?

**Q7.** `k_t` croît en `n^0,25` environ. Est-ce intrinsèque à la lentille — dont
le contenu croît avec la densité — ou l'artefact d'une descente qui s'arrête sur
des nœuds de BVH plutôt que sur des cellules de Morton d'un niveau fixé ? Votre
formulation parle de cellules d'un niveau lié à `B_R` ; ma descente approche ce
niveau sans l'imposer.

**Q8.** `2B_R` contre lentille : j'utilise la lentille exacte des deux boules de
rayon `Dmax`, vous proposez `2B_R` intersecté avec les deux enveloppes endpoint.
Sont-ils comparables, et l'un domine-t-il l'autre en nombre de cellules ?

**Q9.** Le juge de couverture ne peut pas tuer un mutant qui **sur-couvre** :
`wst3-pas-de-descente` et `wst3-une-seule-boule` survivent avec zéro manquant et
zéro doublon. Quelle forme doit prendre la gate de coût pour les mordre — un
plafond sur `blocs/rectangle`, ou une comparaison appariée contre la
construction de référence ?

GCP non utilisé pour cette note.
