# Solution constructive — oracle Gamma exact et quotient local dégénéré

Date : 10 août 2026 UTC.

Objet : aider Claude à fermer Q1 sans matérialiser une mosaïque de Delaunay
d'ordre supérieur. Cette note sépare une vérité exhaustive immédiatement
implémentable d'une route locale, output-sensitive en la taille des coquilles,
qui doit encore être qualifiée contre cette vérité.

Cadre inchangé : `phase=exploration_v3_hors_registre`, backend CPU de référence
et candidat GPU sous audit, profil u16, `public_status=not_claimed`. Aucun
backend n'est promu par cette note.

## 1. Oracle exhaustif de Gamma : partie déjà démontrée

Fixons un ordre `k`, entendu comme cardinalité des facettes qui sont les sommets
de Gamma. Pour un sous-ensemble fini `A`, notons `rho(A)` le rayon exact de sa
plus petite boule fermée. À un niveau exact `a`, poser :

$$V_k(a)=\left\lbrace \tau\subseteq X:\lvert\tau\rvert=k,\rho(\tau)\leq a\right\rbrace.$$

$$E_k(a)=\left\lbrace \sigma\subseteq X:\lvert\sigma\rvert=k+1,\rho(\sigma)\leq a\right\rbrace.$$

Chaque `sigma` de `E_k(a)` relie toutes ses facettes de cardinalité `k`.
D'après la définition de Gamma et la proposition 5 du manuscrit, ce graphe
élémentaire possède exactement les mêmes composantes que le graphe autorisant
les unions plus grandes. Cette construction ne suppose ni position générale,
ni unicité d'un support minimal, ni niveaux distincts.

L'oracle borné peut donc suivre le protocole suivant.

1. Énumérer indépendamment tous les sous-ensembles de tailles `k` et `k+1`.
2. Calculer leur miniboule et leur niveau en arithmétique exacte.
3. Trier par `(k, niveau exact)` et former un lot unique pour toutes les valeurs
   exactement égales; une approximation `double` ne participe jamais au lot.
4. Avant le lot, figer le graphe strict. Dans un staging séparé, ajouter toutes
   les facettes et toutes les cofaces du niveau, y compris celles qui naissent
   ensemble.
5. Fermer toutes les adjacences du lot, calculer les composantes fermées, puis
   committer le lot entier ou rien.
6. Canonicaliser une composante par la liste triée de ses facettes, et sa
   couverture par l'union triée des `PointId` de ces facettes.

Il faut publier les deux coupes, stricte et fermée, à chaque niveau. Comparer
seulement un nombre de composantes final peut laisser passer une naissance et
une fusion compensées au même lot.

### 1.1 Ce que cet oracle doit comparer

Le verdict normatif porte sur :

- la présence exacte de chaque facette active;
- la partition de ces facettes en composantes;
- les incidences élémentaires actives, même lorsqu'elles ne changent pas le
  DSU au lot courant;
- la couverture, dérivée de l'union des facettes de chaque composante;
- les états strict et fermé de chaque niveau.

Il ne doit pas exiger l'égalité d'un `source`, d'un identifiant de nœud ou d'un
ordre interne de multifusion : ce sont des conventions de sérialisation. Il peut
en revanche vérifier qu'une convention annoncée est déterministe après
canonicalisation.

Un simple `coverage_delta` d'identifiants ne remplace pas le journal
d'incidences. Une incidence silencieuse peut avoir un delta de points vide et
devenir nécessaire à une fusion future. Le journal d'autorité conserve donc les
facettes et cofaces actives; `coverage_delta` en est une projection dérivée.

### 1.2 Indépendance et domaine borné

L'oracle peut partager des types rationnels et une primitive de miniboule
extraite de l'ancien oracle exhaustif; il ne doit partager ni l'énumérateur du
produit, ni son quotient de coquilles, ni `build_forest`. Une seconde
implémentation de la logique de lot, même lente, est préférable à un appel au
fold jugé.

Son coût est volontairement combinatoire :

$$W(n,K)=\sum_{k=1}^{K}\left(\binom{n}{k}+\binom{n}{k+1}\right).$$

Ce coût convient à une vérité de falsification pour petits nuages. Il est
interdit d'en faire l'architecture produit ou de le rebaptiser comme telle.
La borne de 384 bits démontrée pour une clef d'axe u16 ne doit pas être étendue
par analogie : l'oracle emploie la multiprécision tant qu'une borne propre à ses
niveaux n'est pas écrite.

## 2. Quotient local d'une coquille : route constructive pour V2

Considérons une boule exacte `B(c,r)`, son intérieur `I` et sa coquille `U`.
Pour `u` dans `U`, une petite translation du centre donne exactement :

$$\left\lVert u-(c+\varepsilon\nu)\right\rVert^2-r^2=\varepsilon^2\left\lVert\nu\right\rVert^2-2\varepsilon\langle u-c,\nu\rangle.$$

Les points de `I` restent strictement intérieurs pour `epsilon` assez petit. La
direction `nu` appartient donc au lien strict local de l'ensemble de niveau
d'ordre `k` si et seulement si :

$$\lvert I\rvert+\#\left\lbrace u\in U:\langle u-c,\nu\rangle>0\right\rbrace\geq k.$$

Cette formule suggère l'objet local exact suivant :

$$\Omega_{k,c}=\left\lbrace \nu\in S^2:\lvert I\rvert+\#\left\lbrace u\in U:\langle u-c,\nu\rangle>0\right\rbrace\geq k\right\rbrace.$$

Les grands cercles `G_u = {nu : <u-c,nu> = 0}` découpent `S^2` en un complexe
fini. Sur chaque cellule de toute dimension, le vecteur de signes et le nombre
de signes positifs sont constants. Marquer les cellules satisfaisant le seuil,
puis calculer leurs composantes par incidence, donne les composantes de
`Omega`. Il faut inclure les arêtes et sommets marqués : ne conserver que les
chambres ouvertes peut scinder artificiellement un passage licite le long d'un
grand cercle.

Cette construction est locale à une seule coquille. Elle ne construit ni la
mosaïque d'ordre supérieur, ni toutes les cellules de l'espace des centres, ni
Gamma global.

### 2.1 Représentant strict d'une composante locale

Dans une cellule marquée, choisir exactement `k-|I|` points de signe positif,
selon une clef canonique, et poser `tau = I union A`. Pour une translation assez
petite, tous les points de `tau` sont dans la boule ouverte de centre
`c+epsilon*nu` et de rayon `r`; par conséquent `rho(tau) < r`. `tau` est donc un
sommet réel de `Gamma_k` dans la coupe stricte.

Deux cellules marquées incidentes se raccordent par une cellule de bord elle
aussi marquée. Un sous-ensemble positif de cette cellule fournit une facette
commune. À l'intérieur d'une même boule ouverte, deux choix de facettes se
relient par échanges d'un point; leurs unions de taille `k+1` ont encore un
rayon strictement inférieur à `r`. Ainsi, une composante de `Omega` s'attache à
une unique composante de `Gamma_k(X)_{<r}`.

La réciproque globale et la complétude des attaches restent à recevoir contre
l'oracle du §1. Deux composantes locales différentes peuvent s'attacher à la
même racine globale; le fold doit compter les racines strictes distinctes, pas
le nombre brut de chambres ou de bras.

### 2.2 Tous les événements d'un niveau restent atomiques

Le quotient local ne donne aucune licence de committer boule par boule. Deux
coquilles distinctes de même niveau peuvent être reliées par une facette née à
ce niveau. Pour chaque `(k,a)`, le pipeline sûr reste :

1. snapshot strict global;
2. production de tous les tokens locaux et de toutes les facettes/incidences du
   lot;
3. fermeture entière du graphe de conflits;
4. classification des racines strictes par composante;
5. staging des incidences et de la couverture;
6. validation, puis commit atomique du lot complet.

Les composantes du graphe de conflits peuvent être calculées en parallèle et
stager des résultats disjoints. Elles ne deviennent pas des commits
indépendants : une faute tardive annule le lot entier.

## 3. Raccourci exact pour l'indice un

Lorsque `k = |I|+|U|-1`, une direction admissible laisse au plus un point de la
coquille hors du demi-espace positif. Pour chaque `u`, définir :

$$C_u=\left\lbrace \nu\in S^2:\langle v-c,\nu\rangle>0\text{ pour tout }v\in U\setminus\left\lbrace u\right\rbrace\right\rbrace.$$

Par séparation stricte en dimension trois, `C_u` est non vide si et seulement
si `c` n'appartient pas à l'enveloppe convexe de `U` privé de `u`. Chaque test
est un petit problème exact de faisabilité/convexité. Les cônes non vides sont
les bras locaux candidats; deux d'entre eux ne doivent être identifiés qu'après
leur attache aux racines strictes globales.

Ce raccourci explique précisément pourquoi les seuls `n_support` points d'un
support minimal ne suffisent pas en cas de coquille multiple. Il ne remplace pas
l'arrangement général : hors position générale, un changement de `H0` peut se
produire pour d'autres seuils `k-|I|`.

## 4. Complexité et route produit

L'arrangement complet de `m=|U|` grands cercles possède `O(m^2)` cellules. Il
constitue une bonne deuxième vérité bornée après l'oracle Gamma, et reste local.
Pour le produit, seules les cellules proches du seuil de profondeur
`k-|I|` sont utiles. Une exploration des niveaux de l'arrangement peut être
output-sensitive, mais cette réduction est une hypothèse d'algorithme à prouver
par identité de masse et frontière terminale; elle ne doit pas précéder la
version complète.

Représentation exacte recommandée : conserver les vecteurs orientés `u-c` avec
leur multiplicité et leur `PointId`; réduire les rationnels en vecteurs entiers
primitifs sans perdre l'orientation; calculer intersections et signes par
produits mixtes exacts. Des grands cercles confondus ou antipodaux sont des lots
multiples, jamais des événements arbitrairement départagés.

La route en quatre étages est donc :

1. **Gamma exhaustif** sur petits nuages, vérité indépendante.
2. **Arrangement local complet** par coquille, comparé à Gamma aux coupes
   strictes et fermées.
3. **Quotient local streamé** : un token par composante locale, attache exacte,
   journal des incidences et lot global atomique.
4. **Index de niveau** seulement après réception d'un certificat terminal et
   d'une identité de masse; tout résidu non exploré est rejoué ou refusé.

## 5. Fixtures qui décident réellement la solution

La porte minimale doit contenir :

- cube cosphérique, avec permutations de supports minimaux;
- coquille coplanaire et grands cercles confondus;
- extra-shell dont le rang fermé dépasse `s_max`, mais dont une face de taille
  `k` ou `k+1` reste active;
- lot mélangeant naissance, continuation et multifusion au même niveau;
- chaîne d'incidences silencieuses répartie entre plusieurs producteurs;
- deux composantes locales distinctes qui s'attachent à la même racine stricte;
- une faute tardive imposant le rollback bit à bit du lot;
- deux niveaux rationnels distincts ayant le même `double`.

Pour chaque fixture et chaque ordre, comparer : facettes, incidences,
partition stricte, partition fermée, couvertures, transcript canonique et état
inchangé après faute. La seule égalité finale des unions de points n'est pas
suffisante.

## 6. Décision proposée à Claude

Poursuivre l'oracle Gamma indépendant commencé après `f102d42` : c'est la porte
la plus utile et la moins spéculative. Ne modifier le fold produit qu'après une
première contradiction minimale ou une égalité exhaustive reçue.

En parallèle, considérer `Omega_{k,c}` comme la définition candidate du quotient
local dégénéré. Elle offre une solution géométrique précise à V2, bornée à la
coquille et compatible avec l'invariant d'architecture « aucune mosaïque
globale ». Son intégration reste conditionnée à la preuve d'attache et à la
comparaison lot par lot avec Gamma.

GCP non utilisé.
