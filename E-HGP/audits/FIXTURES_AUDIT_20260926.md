# Fixtures permanentes issues de l'audit du 26 septembre 2026

> Cible épinglée : commit `f44a8db0372189dba2ede3a55bbce669aa74baab`.
> Doctrine du dépôt : toute contradiction devient une fixture minimale permanente
> **avant** que le travail continue. Ces fixtures ne s'effacent pas quand le
> défaut est corrigé : elles deviennent des portes de non-régression.
> Chaque fixture ci-dessous a été **reproduite par l'auditeur lui-même**, pas
> seulement rapportée.

## F-AUD-1 — Le point fixe de la descente peut avoir un rang fermé $m+1$

Le carré gravé du chantier, à masse $m=3$.

```text
X = [(0,0), (10,0), (0,10), (10,10)]     masse m = 3
```

Sortie de `critical_catalogue(X, 3)` : un seul candidat, centre
$(5,5)$, niveau $50$, `fixed = True`, coquille $U=\lbrace0,1,2,3\rbrace$,
intérieur $I=\varnothing$, donc **rang fermé $s=4$**.

Or le catalogue exact à rang fermé $3$ est **vide** : aucune partie de cardinal 3
n'a de boule fermée vide sur ce nuage. La descente à masse $m$ ne rend donc pas
un point de rang fermé $m$ : elle rend ici un point de rang $m+1$, soit — par la
caractérisation de `docs/SPECIFICATION_MORSEHGP3D.md` § 5 — un point critique
d'**indice 1** pour l'ordre 3, et non une naissance.

**Ce que la fixture réfute** : « en un point fixe la sphère a un rang fermé égal
à $m$ hors dégénérescence cosphérique » et « points fixes de la descente
MEB-Lloyd = sphères critiques, zéro faux positif ». La clause d'exclusion du code
ne parle que des observations **hors** de $N(y)$ ; ici les quatre sommets sont
cosphériques et $N(y)$ n'en contient que trois, donc le code se croit dans le
régime sûr.

**Porte attendue** : la descente doit publier son rang fermé et refuser, ou
étiqueter, tout candidat dont le rang diffère de la masse demandée.

## F-AUD-2 — `conv` n'est pas `relint conv`

```text
X = [(-1,0), (1,0), (0,1), (10,0)]       masse m = 3   départ y = (0,0)
```

`meb_lloyd` rend le point fixe $(0,0)$, niveau $1$, $s=3$,
$U=\lbrace0,1,2\rbrace$, $I=\varnothing$. Le support de la coquille est
$\lbrace0,1\rbrace$ : $(0,0)$ est le milieu de $(-1,0)$ et $(1,0)$, et $(0,1)$
est **sur** la sphère sans appartenir au support. Les coordonnées barycentriques
de $(0,0)$ dans $U$ valent donc $(1/2,1/2,0)$ :

$$(0,0)\in\mathrm{conv}(U),\qquad (0,0)\notin\mathrm{relint}\,\mathrm{conv}(U).$$

**Ce que la fixture réfute** : l'affirmation que la condition de point fixe est
« exactement » la condition $c\in\mathrm{relint}\,\mathrm{conv}(U)$ de la
spécification. Le code écrit `conv`, le document écrit `relint conv`, et les deux
diffèrent sur les configurations dégénérées. Fréquence mesurée en rationnels :
$2/240$ en $d=2$, $m=3$.

**Conséquence à trancher, et elle est mathématique** : le critère de la
spécification **rejette** ce point alors que des sondes voisines montrent qu'il
s'agit d'un minimum local de $a_3$. Soit le critère normatif doit être relu, soit
la descente doit exclure ce cas ; les deux lectures ne peuvent pas cohabiter en
silence.

## F-AUD-3 — Le décalage d'hyperplan n'est pas optimal

```text
X = [(0,0), (1,0), (100,0), (101,0)]     ordre 2   paire (0,1)   normale (1,0)
```

Sur l'intervalle ouvert $(0,1)$, la deuxième plus petite des $(t_l-c)^2$ vaut
$\max(c^2,(1-c)^2)$, dont le **supremum est 1**, approché aux deux bouts et non
atteint. `best_offset` rend $1/4$ en $c=1/2$ ; un balayage donne $0{,}99998$ en
$c=10^{-5}$. Rapport supremum sur code : $3{,}99992$.

**Ce que la fixture réfute** : « le décalage optimal se calcule exactement ; il
suffit d'évaluer ces candidats ». La **sûreté** n'est pas en cause — une borne
inférieure plus faible reste valide — mais ni le code ni la preuve ne traitent le
cas où le maximum du morceau tombe sur une extrémité **exclue**.

## F-AUD-4 — Un certificat non sûr que la porte ne voit pas

```text
X = [(11,161), (84,25), (73,60), (21,170), (10,139), (96,75)]
ordre 1   paire (0,3)   sphère de centre (11,161), rayon au carré 181/2
```

Un mutant d'un cran sur le comptage de la condition de tranche accepte le niveau
$185/4$ alors que le niveau de fusion exact vaut $181/4$. Sur l'ensemble du
corpus, ce mutant accepte **41 668** certificats à des niveaux strictement
au-dessus du niveau exact, contre **0** pour l'original — et pourtant
`tests/test_separation.py` passe, code 0.

**Ce que la fixture réfute** : « la porte confronte chaque certificat à
l'ultramétrique exacte de l'oracle ; une seule violation est un échec ». La porte
compare le niveau **certifié**, qui est le majorant à témoins, déjà égal à
l'exact sur la quasi-totalité des paires : un certificat faux rend donc quand
même le bon nombre. La sûreté du certificat reste vraie ; c'est la porte qui ne
la garde pas.

## F-AUD-5 — « toutes les paires certifiées » est faux

```text
n = 7, d = 20, coordonnées entières dans [0,200]
graine : random.Random(31 * 1000 + 20 * 10 + 7)
```

Couverture du certificat à l'ordre 1, $d=20$, selon la graine :
$21/21$, $17/21$, $\mathbf{9/21}$, $21/21$, $15/21$, $17/21$ — moyenne
$15{,}7/21$. Agrégé : $385/441=0{,}873$ à $d=20$, $325/441=0{,}737$ à $d=3$.

**Ce que la fixture réfute** : le quantificateur « toutes » et le statut
« démontré ». Un compte est **mesuré** ; ce qui est démontré est le certificat de
tranche, pas le fait que toutes les paires le reçoivent.

## F-AUD-6 et F-AUD-7 — Le port flottant n'est pas un minorant, et son auto-certification ment

```text
F-AUD-6 : n = 12, d = 50, coordonnées 1e10 + [0, 1002]  (presque cosphériques)
F-AUD-7 : n = 8,  d = 20, coordonnées 1e8  + [0, 40]
```

À l'échelle $10^7$ tout est propre (aucune violation sur trois graines). À
$10^8$ la borne basse est violée de $4{,}9\cdot10^{-2}$ en relatif et seuls 1 à 6
couples sur 84 encadrent encore la vérité ; à $10^{10}$ la violation relative
atteint $9{,}437\cdot10^{4}$. Le **majorant** s'effondre aussi
(`high_violation = 1,0`, c'est-à-dire un majorant nul alors que la vérité est
positive).

Le plus grave : **l'auto-certification ment**. À l'échelle $10^8$ et $d=20$, 84
couples sont déclarés exacts et **un seul** l'est ; à $10^9$, 84 déclarés et
**zéro** vrai. Les garde-fous publiés ne voient rien.

**Ce que la fixture réfute** : « le port flottant est un minorant certifié du
poids de segment rationnel exact : il ne le dépasse jamais » et « l'égalité des
deux bornes est une preuve d'exactitude ». Le mot « certifié » ne peut pas
s'appliquer à ce module ; son seuil d'échelle doit être publié.

## F-AUD-8 — Le digest n'est pas invariant par permutation

```text
X = [(0,), (1,), (3,)]    dimension 1    k_max = 1
```

Les six permutations donnent **six digests distincts**
(`40474e84`, `57d97e3b`, `604dcdf2`, `67dc4622`, `d597df3e`, `fe73940f`), alors
que le multiensemble des niveaux est **invariant** (une seule valeur sur les six).

**Ce que la fixture réfute** : l'en-tête de `tests/test_tour_exacte.py`, qui
annonce l'invariance du digest par permutation. Ce qui est invariant, et ce que la
porte vérifie effectivement, est `levels` et `merge_levels`. L'enregistrement
canonique contient des identifiants d'observations : il **ne peut pas** être
permutation-invariant, et il n'a pas à l'être.

## F-AUD-9 — Le juge de grille peut être décisivement faux

```text
X = [(0,0), (3,0)]    ordre 1    niveau 2    steps = 3
```

L'encadrement rend bas $=$ haut $=1$ — donc « grille assez fine » selon le
critère du module — alors que la vérité est **2** composantes. À `steps = 6`
l'encadrement rend $2$ et $2$, correct.

**Ce que la fixture réfute** : « si les deux coïncident, la grille est jugée assez
fine », et l'affirmation du docstring que la connexité d'axe surestime toujours et
la connexité pleine sous-estime toujours. Les deux peuvent se tromper **du même
côté**.

## F-AUD-10 — Le niveau de Fermi n'est pas unique

```text
énergies e = (0, 0, 10)   epsilon = 1   masse k = 2
```

Le comptage doux vaut exactement $2$ pour $\mu=1$, $2$, $5$ et $10$ : tout
l'intervalle $[1,10]$ convient, soit neuf fois $\varepsilon$ de largeur. Le
**poids** $w=(1,1,0)$ est bien unique ; le **multiplicateur** ne l'est pas.

**Ce que la fixture réfute** : « $\mu$ est l'unique réel tel que
$\sum_ig((\mu-e_i)/\varepsilon)=k$ ». `soft/families.py` le dit correctement
(« $w$ est toujours unique, $\mu$ ne l'est pas ») : le document est plus faible
que son propre code, et la convention d'extrémité gauche dont dépend tout le
paragraphe suivant n'est pas énoncée.

## F-AUD-11 — La forme close spectrale exige une hypothèse non énoncée

```text
p = N(0,1)    q = N(0,4)    phi = (1, x, x^2)    moments exacts
```

La forme close rend $\Theta=(0{,}468613;\,0;\,-0{,}247321)$ contre les vrais
coefficients $(\log2;\,0;\,-3/8)$, **alors que le log-rapport est exactement dans
le span des descripteurs**. L'hypothèse réellement requise est que
$(t-1)/(\rho t+1-\rho)$ appartienne au span pour **tout** $\rho$ de $[0,1]$, et
non la bonne spécification usuelle. L'identité est exacte dans le cas discret à
atomes (écart $1{,}7\cdot10^{-16}$).

**Ce que la fixture réfute** : la présentation de l'identité comme « résolue » et
sans hypothèse. L'**algèbre** ($\Theta$ égale l'intégrale en $\rho$ des potentiels
optimaux du modèle linéaire) est confirmée à $2{,}2\cdot10^{-13}$ ; c'est
l'identification au vrai log-rapport qui demande l'hypothèse.
