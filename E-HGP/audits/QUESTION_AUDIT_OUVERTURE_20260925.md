# Questions d'audit à l'ouverture de E-HGP — 25 septembre 2026

> `phase=exploration_ehgp_hors_registre`, `backend=python_reference`,
> `profile=any_dimension_rational_exact`, `mode=audit_independant_math_and_architecture`,
> `public_status=not_claimed`. GCP non utilisé. Commit d'ancrage : voir
> `receipts/ouverture_20260925/MANIFESTE.json`.

Ce fichier liste, par ordre décroissant de conséquence, les verrous que je
demande à un auditeur indépendant de **contredire**. Chaque question est
formulée pour être falsifiable, avec la commande qui produit le chiffre en
cause. Les réponses attendues vont dans ce dossier, en `AUDIT_*` ou
`CONTRE_AUDIT_*`, datées et ancrées au hash court.

## Q1 — L'obstruction de taille est-elle correctement énoncée ?

Ce qui est écrit : le nombre de **naissances topologiques** de la tour d'ordre
$k$ passe de $O(n)$ en dimension 2 à exactement $\binom{n}{k}$ en dimension
100 ; le compte de parties à boule **fermée** vide minore ce nombre ; et
l'énoncé « tend vers $\binom{n}{k}$ » ne vaut qu'à $n$ et $k$ **fixés** quand
$d\to\infty$, la fraction décroissant avec $n$ à $d$ fixé.

À contredire : trouver une loi de nuage, un régime de $n$ ou un ordre $k$ où
la sortie reste de taille polynomiale en grande dimension, ou montrer que
l'inégalité `boules vides` $\leq$ `naissances` s'inverse.

```bash
python3 bench/births_vs_dimension.py --n 8 --k-max 4 --dims 2,3,5,10,20,50,100 --seeds 5,17,31
python3 bench/empty_ball_fraction.py --n 11 --ks 2,3,4,5 --dims 2,3,5,10,20,50,100 --seed 5
```

## Q2 — La descente MEB-Lloyd produit-elle bien des points critiques, et seulement eux ?

Ce qui est écrit : les points fixes de l'itération « aller au centre de la
boule englobante minimale de ses $m$ plus proches » sont exactement les
points critiques de rang fermé $m$ de la spécification ($m=k$ pour les
naissances, $m=k+1$ pour les fusions), avec **zéro faux positif** mesuré, et
une couverture complète en $d=2,3$ mais partielle au-delà.

À contredire : exhiber un point fixe qui n'est pas un point critique (un faux
positif tue la brique), ou un point critique inatteignable par la descente
depuis **tout** point de départ (ce serait une limite structurelle et non un
manque de départs), ou une dégénérescence cosphérique qui casse
l'identification du rang fermé.

## Q3 — Le majorant à témoins est-il vraiment un majorant, et son égalité est-elle un hasard ?

Ce qui est écrit : la tour projetée avec témoins **majore** l'ultramétrique
exacte, et l'égalité est mesurée sur 2843 paires sur 2856 sans témoins de
triplets, puis sur la totalité des paires mesurées avec eux — sans jamais une
valeur strictement inférieure.

À contredire : une paire où le majorant est **strictement inférieur** à la
valeur exacte (ce serait un défaut de la certification de segment, donc
grave), ou une famille de nuages où l'écart devient systématique.

```bash
python3 bench/witness_campaign.py --ns 8 --dims 2,3,5,10,20,50 --k-max 3 \
    --seeds 17,23,31 --families uniform,clusters --triples \
    --min-cases 30 --min-pairs 2000 --out receipts/relecture/campagne.json
```

## Q4 — Le certificat de séparation est-il correct ?

Ce qui est écrit : si une hypersurface sépare deux observations et si au plus
$k-1$ boules $B(x_l,\sqrt{a})$ la rencontrent, alors le niveau de fusion est
au moins $a$ ; la version sphérique se décide **sans racine carrée** par
$(a-s-R)^2\leq4sR$ ou $a>s+R$ ; aucun faux certificat n'a été observé, et à
l'ordre 1 en $d=20$ les 21 paires sur 21 sont certifiées exactes.

À contredire : un certificat qui affirme une séparation fausse (fatal), une
erreur dans l'élimination des racines, ou une paire certifiée dont le niveau
diffère de celui de l'oracle.

## Q5 — Les trois énoncés réfutés en chemin sont-ils bien réfutés ?

1. la limite du niveau de Fermi logistique à masse **entière** est le **milieu
   du trou** et non $a_k$ ;
2. le critère de naissance par l'**intérieur strict** surcompte (fixture F1 :
   $(0,0)$, $(2,0)$, $(1,1)$, ordre 2) ;
3. lisser le **niveau** ne réduit pas le nombre de points critiques en grande
   dimension (53 minima contre 50 quand $\varepsilon$ croît de $10^{-4}$ à
   $1$ fois la variance en $d=20$).

À contredire : l'un de ces trois points, ou la constante optimale
$\varepsilon\log(2\max(k,n-k+1)-1)$ de la variante logistique.

## Q6 — Le lien avec le cadre de Bach est-il honnête ?

Ce qui est écrit : $\rho=0$ de la famille $f_{\rho}$ correspond au noyau
rampe ; $\rho$ n'agit sur l'encadrement que par la largeur de fenêtre
$\varepsilon\,c(\rho)$ avec $c(\rho)=(2-\rho)/(2(1-\rho)^2)$ ; le gain
**spectral** du billet, lui, ne se transporte **pas** au programme de masse,
parce qu'il vient de la forme quadratique d'un modèle linéaire en
descripteurs ; et l'identité intégrale entre noyau logistique et noyaux rampe
pondérés est **conjecturale**.

À contredire : démontrer ou réfuter cette identité intégrale, ou montrer que
le gain spectral se transporte effectivement.

## Q7 — Ce qui reste ouvert, et que je n'ai pas su faire

1. renforcer le certificat de séparation aux ordres $k\geq2$ (le rayon doit
   éviter la $k$-ième coquille) ;
2. une garantie de couverture de la descente sous hypothèse de séparation ;
3. la portée sur données réelles : l'obstruction est gouvernée par la
   dimension **effective à l'échelle des événements**, donc par le rapport
   entre l'espacement local et l'amplitude du bruit ambiant. Tant que ce
   rapport n'est pas mesuré sur des nuages réalistes, aucune conclusion ne
   doit être tirée pour un usage applicatif ;
4. l'apprentissage de descripteurs pour la couche spectrale, avec
   préservation de la monotonie verticale $L_{k+1}(a)\subseteq L_k(a)$.
