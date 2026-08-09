# Gate D — parent local exact pour la reverse search multiplicitaire

Date : 9 août 2026 UTC.

Cadre : `backend=reference_cpu_local`, `profile=quantized_u16_order_k_prototype`,
`mode=mathematical_parent_rule_only`, `public_status=not_claimed`.

> [!IMPORTANT]
> **Le parent peut être choisi sans `seen`, sans mosaïque globale et sans
> énumérer tous les voisins pour décider lequel est le parent.** Au sommet
> courant, un programme linéaire exact en dimension quatre sélectionne un
> unique rayon extrême du cône tangent de la chambre. Une seule requête de
> voisin le long de ce rayon atteint alors le parent. La règle vaut aussi aux
> sommets multiples; elle suppose en revanche une coquille complète, un
> ensemble intérieur exact et un oracle exact du prochain lot sur un flat de
> rang trois.
>
> Ce théorème ferme la **règle mathématique de parent**. Il ne ferme ni
> l'implémentation, ni l'énumération des enfants, ni le coût potentiellement
> combinatoire des flats incidents, ni le contrat 50 k.

Cette note renforce le parent par voisins de
[`AUDIT_REVERSE_SEARCH_ORDER_K_CF9374.md`](AUDIT_REVERSE_SEARCH_ORDER_K_CF9374.md)
et son extension abstraite aux multiplicités de
[`AUDIT_VOIE_MULTIPLICITES_ORDER_K.md`](AUDIT_VOIE_MULTIPLICITES_ORDER_K.md).
Le point nouveau est de choisir **directement la direction du parent** par un
problème local de dimension fixe, au lieu de construire tous les voisins
améliorants puis de les trier.

## 1. État local et préconditions

Dans l'espace relevé, pour $x=(c,t)\in\mathbb{R}^{4}$ et un site $p_i$, posons

$$L_i(x)=t-2c\mathbin{\cdot}p_i+\lVert p_i\rVert^2,\qquad a_i=\nabla L_i=(-2p_i,1).$$

À un sommet d'arrangement $v$, les seules données scientifiques nécessaires au
parent sont

$$B(v)=\left\lbrace i:L_i(v)<0\right\rbrace,\qquad S(v)=\left\lbrace i:L_i(v)=0\right\rbrace.$$

Le niveau de navigation est $\ell(v)=\lvert B(v)\rvert$. La coupe shallow porte
uniquement sur ce niveau, jamais sur $\ell(v)+\lvert S(v)\rvert$. Le sommet est
authentifié par les préconditions suivantes :

1. les formes $a_s$, $s\in S(v)$, engendrent $\mathbb{R}^{4}$;
2. $S(v)$ est la coquille complète et $B(v)$ l'ensemble intérieur complet;
3. un oracle `next(v,d)` rend le premier lot fini dans la direction exacte $d$,
   ou certifie l'absence de voisin;
4. un germe fixe $r$ de niveau zéro et une base indépendante canonique
   $T_r\subseteq S(r)$ de quatre formes sont connus.

La dimension affine trois du nuage implique le rang quatre des normales aux
vrais sommets. Les dimensions affines plus basses gardent leur voie directe.
Dans l'extension abstraite au profil multiplicitaire, des hyperplans répétés ne
cassent pas la preuve : ils restent tous dans la coquille et le census conserve
leurs multiplicités. Le prototype live refuse encore les coordonnées dupliquées;
il ne qualifie donc pas cette extension à lui seul.

## 2. La chambre et son cône tangent sont locaux

Pour $B=B(v)$, la fermeture de la chambre située juste au-dessus de $v$ est

$$P_B=\left\lbrace x:L_i(x)\leq0\ \text{si }i\in B,\quad L_j(x)\geq0\ \text{si }j\notin B\right\rbrace.$$

Toutes les contraintes actives en $v$ viennent de $S(v)$, puisque les indices
de $B(v)$ sont strictement intérieurs. Le cône tangent est donc exactement

$$K_v=\left\lbrace d:a_s\mathbin{\cdot}d\geq0\ \text{pour tout }s\in S(v)\right\rbrace.$$

Il est plein : la direction verticale $e_t=(0,0,0,1)$ satisfait strictement
toutes ses inégalités. Il est pointé : les $a_s$ engendrent $\mathbb{R}^{4}$.
Ses rayons extrêmes sont précisément les orientations admissibles des vrais
flats fermés de rang trois incidents à $v$, y compris lorsque plus de trois
hyperplans contiennent le même flat.

## 3. Sélection locale d'un rayon extrême

### 3.1 Niveau strictement positif

Si $B(v)\neq\varnothing$, choisissons simplement

$$h(v)=\min_{\prec}B(v)$$

pour une clef `PointId` totale canonique fixée par le profil, jamais pour la
position accidentelle dans le tableau d'entrée. Il n'est pas nécessaire de
chercher le point intérieur le plus proche de la coquille : si l'ensemble
intérieur ne change pas au parent, la même règle resélectionne le même $h$; s'il
change, son cardinal diminue déjà. Posons $b_v=a_{h(v)}$. La tranche

$$D_v=\left\lbrace d\in K_v:b_v\mathbin{\cdot}d=1\right\rbrace$$

est non vide, car $e_t\in D_v$.

### 3.2 Niveau zéro hors du germe

Définissons la fonction linéaire du germe

$$Q_r(x)=\sum_{s\in T_r}L_s(x).$$

Sur $P_{\varnothing}$, chaque terme est non négatif et $Q_r(x)=0$ impose les
quatre égalités indépendantes de $T_r$; le germe $r$ est donc son unique
minimum. Si $B(v)=\varnothing$ et $v\neq r$, le segment de $v$ à $r$ reste dans
le polyèdre convexe $P_{\varnothing}$. Sa direction appartient donc à $K_v$ et
fait strictement décroître $Q_r$. Posons $b_v=-\nabla Q_r$ et conservons la même
définition de $D_v$.

### 3.3 Le petit programme linéaire canonique

Sur $D_v$, minimisons d'abord

$$G_v(d)=\sum_{s\in S(v)}a_s\mathbin{\cdot}d,$$

puis, sur la face optimale, les quatre coordonnées de $d$ dans un ordre fixe.
Toutes ces décisions sont rationnelles exactes.

Cette prescription a une solution unique $d_v$ et celle-ci est un rayon
extrême de $K_v$. En effet, chaque terme de $G_v$ est non négatif sur $K_v$.
Si une direction de récession non nulle $q$ vérifiait $G_v(q)=0$, tous les
produits $a_s\mathbin{\cdot}q$ seraient nuls; le rang quatre de la coquille
imposerait $q=0$, contradiction. Les sous-niveaux de $G_v$ dans $D_v$ sont donc
bornés. La minimisation atteint une face compacte; les quatre tie-breaks
linéaires y sélectionnent un sommet unique de $D_v$. Ce sommet engendre un
rayon extrême de $K_v$ : sinon il appartiendrait à l'intérieur relatif d'une
face conique de dimension au moins deux, dont l'intersection locale avec
$b_v\mathbin{\cdot}d=1$ contiendrait un segment passant par $d_v$, contradiction.

Le calcul porte toujours sur quatre variables. Il peut être réalisé par un
solveur exact de programmation linéaire en dimension fixe avec
$\lvert S(v)\rvert$ inégalités. Le théorème ne donne aucune autorité à une
décision flottante et ne demande pas d'énumérer les $f_3(v)$ flats pour choisir
le parent.

Le solveur doit publier un certificat dual rationnel. Pour la première
minimisation, une identité de la forme

$$\nabla G_v-\mu b_v=\sum_{s\in S(v)}y_sa_s,\qquad y_s\geq0,$$

avec complémentarité sur les contraintes actives, certifie la borne
$G_v(d)\geq\mu$ sur la tranche. Les tie-breaks suivants possèdent le même type
de certificat sur leurs faces optimales. Un énumérateur exhaustif des rayons
extrêmes reste l'oracle borné indépendant du solveur.

### 3.4 Corollaire utile au prototype de Claude

La preuve n'exige pas spécifiquement le minimiseur de $G_v$. **Toute règle
déterministe qui choisit un rayon extrême de $K_v$ avec
$b_v\mathbin{\cdot}d>0$ définit un parent correct.** Le prototype peut donc
réutiliser les flats qu'il énumère déjà, filtrer exactement les deux
orientations par les contraintes de coquille et par le signe de $b_v$, puis
prendre la plus petite clef de flat admissible. Il énumère alors $f_3(v)$
directions mais ne construit aucun de leurs voisins avant d'avoir choisi le
parent; une seule requête `next` reste nécessaire.

Le LP local est la variante qui évite aussi cette énumération lors du calcul du
parent. Les deux sélecteurs doivent être confrontés sur petit $n$ après
normalisation de chaque rayon par $b_v\mathbin{\cdot}d=1$, avec comparaison de
la clef rationnelle $(G_v,d_0,d_1,d_2,d_3)$.

## 4. Le prochain événement est le parent

Définissons

$$\pi(v)=\mathrm{next}(v,d_v).$$

Ce voisin est nécessairement fini.

- Si $B(v)\neq\varnothing$, $L_{h(v)}(v+\tau d_v)=L_{h(v)}(v)+\tau$, alors que
  $L_{h(v)}\leq0$ partout dans $P_{B(v)}$. Le rayon ne peut donc pas rester
  indéfiniment dans la chambre.
- Si $B(v)=\varnothing$ et $v\neq r$,
  $Q_r(v+\tau d_v)=Q_r(v)-\tau$, alors que $Q_r\geq0$ sur
  $P_{\varnothing}$. Le même argument exclut un rayon non borné.

Le segment ouvert jusqu'au premier lot reste dans $P_{B(v)}$. Les contraintes
de coquille dont la pente est nulle sur $d_v$ ont un rang normal exactement
trois : un rang quatre forcerait $d_v=0$. Le premier lot possède une pente non
nulle et porte ce rang à quatre. `next` rend donc bien le sommet adjacent du
flat fermé, même si le lot contient plusieurs hyperplans. Par conséquent,

$$B(\pi(v))\subseteq B(v).$$

Si l'inclusion est stricte, le niveau diminue. Si les deux ensembles ont la
même cardinalité, ils sont égaux, la clef canonique resélectionne le même
$h(v)$ et

$$L_{h(v)}(\pi(v))>L_{h(v)}(v).$$

Le potentiel lexicographique $(\ell(v),-L_{h(v)}(v))$ décroît donc
strictement tant que le niveau est positif. Une fois le niveau nul atteint,
$Q_r$ décroît strictement à chaque parent et possède l'unique minimum $r$. Le
graphe étant fini, tout sommet shallow atteint $r$, sans cycle et sans jamais
augmenter son niveau.

> **Théorème du parent local.** Sous les préconditions du §1, chaque sommet
> d'arrangement $v\neq r$ possède un unique parent adjacent $\pi(v)$ calculable
> depuis $r$, $S(v)$, $B(v)$, un programme linéaire exact en dimension quatre et
> une seule requête `next`. Ce parent vérifie
> $\ell(\pi(v))\leq\ell(v)$. La règle couvre les sommets simples et multiples.

## 5. Reverse search sans table globale

Pour énumérer les enfants d'un sommet $v$ :

1. énumérer en flux chacun de ses vrais voisins shallow $w$ par les flats fermés
   de rang trois;
2. recalculer localement $\pi(w)$;
3. descendre dans $w$ si et seulement si $\pi(w)=v$.

Le parent de $w$ ne réénumère plus tous les voisins de $w$ : il résout le
programme local puis effectue une seule requête `next`. Cela retire le facteur
de degré supplémentaire de la formulation antérieure. Le schéma de reverse
search d'Avis--Fukuda permet de revenir par le parent et de réénumérer l'indice
local du fils; la mémoire de navigation ne dépend alors pas du nombre global
de sommets.

L'état résident propre au parcours est composé de la racine, du sommet courant,
de $B(v)$ — au plus $k_{\mathrm{nav}}$ identifiants —, de $S(v)$, de l'itérateur
local de flats et de l'index spatial éventuel. Il ne contient ni `seen`, ni
`frontier`, ni `visited`, ni incidence globale, ni cellule de mosaïque d'ordre
supérieur.

## 6. Arithmétique exacte directement disponible

Le choix de $h(v)$ ne compare aucune marge : l'ensemble $B(v)$ fourni par les
signes exacts suffit. Le parent ne demande donc pas d'élargir `sphere_side` pour
exposer son numérateur brut.

Le programme linéaire local emploie uniquement les coefficients entiers
$a_s=(-2p_s,1)$. Sa sortie rationnelle doit être convertie en direction primitive
du pinceau sans passage par `double`. Un contrôle hostile peut encore comparer
les puissances rationnelles aux deux extrémités pour vérifier le potentiel, mais
cette comparaison appartient au juge et non à la sélection du parent.

Pour une base planaire $(a,b,c)$ et $u=(b-a)\times(c-a)$, un rayon entier du
pinceau est directement

$$d=(u,2u\mathbin{\cdot}a),\qquad a_i\mathbin{\cdot}d=-2\,\mathrm{orient3d}(a,b,c,p_i).$$

Cette identité donne les signes du cône, la fermeture
$C(d)=\left\lbrace s\in S(v):a_s\mathbin{\cdot}d=0\right\rbrace$ et le signe
d'amélioration avec les prédicats entiers existants. Le code doit vérifier que
$C(d)$ a le rang normal trois, en extraire une base canonique, puis seulement
appeler `next`.

## 7. Ce qui reste global une fois le parcours local

La localité du parent supprime l'état global du **graphe**; elle ne transforme
pas les décisions scientifiques en décisions de voisinage euclidien. Cinq
classes subsistent et doivent rester séparées : lecture de l'entrée, flux exact
externalisable, état horizontal, jointure verticale et identité contractuelle.

### 7.1 L'oracle global en lecture

Le nuage entier $X$, ou un index immuable construit sur lui, reste l'autorité de
`next`, des lots, des coquilles et des rangs. Cette globalité coûte $O(n)$ en
mémoire d'entrée, mais aucune mémoire proportionnelle à $V_k$. Un index qui ne
sert qu'à proposer doit reprendre exactement; un index qui élague doit porter
son certificat entier.

### 7.2 Le propriétaire globalement défini, localement vérifiable

Pour un support indépendant $U$, soit $x_U$ sa sphère minimale et

$$B_U=\left\lbrace i:L_i(x_U)<0\right\rbrace.$$

Le polyèdre de propriété est

$$P_U=F_U\cap\left\lbrace x:L_i(x)\leq0\ \text{si }i\in B_U,\quad L_j(x)\geq0\ \text{si }j\notin B_U\cup U\right\rbrace,$$

où $F_U=\bigcap_{u\in U}H_u$. Pour un sommet $v$ qui contient $U$, l'appartenance
exacte à ce polyèdre équivaut aux **deux** inclusions

$$B(v)\subseteq B_U,\qquad B_U\subseteq B(v)\cup S(v).$$

La première inclusion seule est le préfiltre live; elle n'est pas un certificat
de propriété. La seconde interdit qu'un ancien intérieur de la sphère minimale
soit devenu extérieur au sommet candidat.

Sans l'hypothèse « $v$ contient $U$ », les deux inclusions ne suffisent pas : la
garde $U\subseteq S(v)$ est indépendante et doit être testée séparément. Ainsi,
l'appartenance complète équivaut exactement au triplet
$U\subseteq S(v)$, $B(v)\subseteq B_U$ et
$B_U\subseteq B(v)\cup S(v)$.

Un propriétaire unique existe sans table globale. Écrivons
$g_i=-L_i$ pour $i\in B_U$ et $g_j=L_j$ pour $j\notin B_U\cup U$, puis minimisons

$$G_U(x)=\sum g_i(x)$$

sur $P_U$, avec tie-break lexicographique exact des coordonnées. Chaque $g_i$
est non négatif sur $P_U$. Dans une direction de récession, leur somme ne peut
rester nulle que si toutes les formes du nuage s'annulent; la dimension affine
trois impose alors la direction nulle. La face optimale est donc compacte et le
tie-break choisit un unique sommet $o(U)$.

Cette définition est globale par ses contraintes, mais calculable à la demande
depuis $(X,U)$ en dimension fixe : dimension deux pour une paire, dimension un
pour un flat de triangle et zéro pour un support quatre. Au sommet courant, une
autre certification locale consiste à vérifier l'appartenance à $P_U$ puis
l'absence de rayon incident qui améliore $(G_U,x_0,x_1,x_2,x_3)$; pour un
programme linéaire convexe, l'optimalité locale est globale. Voilà la pièce qui
remplacera la table `emitted`.

À l'arité trois, la décision se quotiente réellement par flat fermé. Si $U$ et
$U'$ sont deux bases du même flat $C$ de rang trois, alors
$F_U=F_{U'}=F_C$, les sphères minimales et $B_U$ coïncident, et chaque forme de
$C$ s'annule identiquement sur $F_C$. Par conséquent, les restrictions de
$P_U,G_U$ et de $P_{U'},G_{U'}$ à cette droite sont identiques : le propriétaire
est $o(C)$, indépendant de la base choisie. Le support canonique public est
appliqué ensuite à la coquille complète.

Elle ne réclame même pas une somme en $O(n)$ par candidat. Précomputons une fois

$$A_X=\sum_{i\in X}a_i.$$

Le gradient du premier objectif se réduit exactement à

$$g_U=\nabla G_U=A_X-\sum_{u\in U}a_u-2\sum_{i\in B_U}a_i.$$

Pour une sortie de rang fermé au plus $s_{\max}$, une requête exacte sur la
sphère minimale rejette d'abord le cas surpeuplé; sinon
$\lvert B_U\rvert\leq s_{\max}-\lvert U\rvert$. À $s_{\max}=11$, les deux
inclusions et $g_U$ se calculent donc sur de petits ensembles triés, après une
seule requête `closed_ball` certifiée. L'index retire le balayage systématique de
$X$; une requête peut néanmoins toucher $O(n)$ points au pire et cette note ne
revendique aucune borne sous-linéaire universelle.

Au sommet candidat, considérons les rayons extrêmes du cône tangent **signé** de
$P_U$. Plus précisément, posons $\varepsilon_s=-1$ pour
$s\in B_U\cap S(v)$ et $\varepsilon_s=+1$ pour
$s\in S(v)\setminus(B_U\cup U)$. Ce cône est

$$K^U_v=\left\lbrace d:a_u\mathbin{\cdot}d=0\ \text{pour }u\in U,\quad \varepsilon_sa_s\mathbin{\cdot}d\geq0\ \text{pour }s\in S(v)\setminus U\right\rbrace.$$

Employer à sa place le cône non signé de la chambre peut rejeter le vrai
propriétaire : un membre de $B_U$ actif en coquille doit pouvoir devenir
strictement intérieur. Pour chaque rayon extrême de $K^U_v$, formons la dérivée
lexicographique

$$\Delta_U(d)=(g_U\mathbin{\cdot}d,d_0,d_1,d_2,d_3).$$

Alors

> **Critère local de propriété.** Sous $U\subseteq S(v)$ et les deux inclusions
> ci-dessus, $v=o(U)$ si et seulement si aucun rayon extrême admissible ne
> possède $\Delta_U(d)<_{\mathrm{lex}}0$.

Une direction négative donne immédiatement un point meilleur de $P_U$.
Réciproquement, si un point globalement meilleur existait, sa direction depuis
$v$ appartiendrait au cône tangent. En la décomposant en rayons extrêmes, le
premier composant non nul de la somme ne pourrait être négatif sans qu'au moins
un rayon soit lui-même lexicographiquement négatif. Le test local est donc un
certificat global d'optimalité.

La compacité assure l'existence de chaque minimum successif. La chaîne de faces
obtenue en minimisant $G_U$, puis les coordonnées, se termine par une face
singleton; ce singleton est donc un sommet de $P_U$. Les contraintes de $U$ et
les contraintes actives y ont ensemble rang quatre : c'est bien un sommet de
l'arrangement, pas un optimum flottant dans une face positive.

Pour une paire, ce cône vit dans un plan et ses rayons se déduisent des flats de
triangles contenant la paire. Pour un flat de rang trois, il vit sur une droite
et les deux orientations du pinceau suffisent. Un support quatre appartient au
sommet lui-même; les singletons restent directs. Aucun `seen_edge`, `seen_face`
ou `emitted` global n'est mathématiquement requis après cette porte, sous la
garde de support canonique suivante.

Plus précisément, pour une paire le cône relatif pointé de dimension deux a au
plus deux rayons; une intersection exacte de demi-plans sur les restrictions
signées de $S(v)$ les trouve sans énumérer les triplets. Pour une arité trois,
un scan de $S(v)$ teste les deux orientations de la droite et au plus une est
admissible. L'arité quatre n'a aucun rayon. Ces coûts sont locaux en
$\lvert S(v)\rvert$, mais une coquille peut avoir taille $n$ : aucun scan global
systématique n'est requis, sans promesse sous-linéaire au pire.

#### Le propriétaire de $U$ ne suffit pas lorsque la même boule a plusieurs supports

Deux supports inclusion-minimaux distincts peuvent porter la même miniboule. Appliquer
séparément le propriétaire à chacun émettrait encore plusieurs fois la même
sphère. Le cube u16 $\left\lbrace0,2\right\rbrace^3$ possède par exemple six
supports inclusion-minimaux pour une seule boule et un seul sommet propriétaire :
quatre paires antipodales, seules de cardinalité minimale, et deux tétraèdres de
parité. La règle complète d'émission est donc :

1. recenser exactement la boule fermée du candidat et refuser son rang s'il
   dépasse le contrat;
2. calculer sur sa coquille le support minimal canonique $U_{\mathrm{can}}$ selon
   la convention géométrique publique;
3. rejeter tout candidat indépendant $U\neq U_{\mathrm{can}}$;
4. tester ensuite que le sommet courant est $o(U_{\mathrm{can}})$;
5. émettre le record une fois, avec $U_{\mathrm{can}}$ et la coquille complète.

Pour les arités deux et trois, $U_{\mathrm{can}}$ est rencontré parmi les
sous-ensembles de la coquille. Pour l'arité quatre, la sphère est le sommet
lui-même : on canonise une fois sa coquille et on n'emprunte cette voie que si
le support canonique a bien arité quatre. Les singletons restent directs. Cette
composition « support canonique puis propriétaire » — et non le propriétaire
de chaque support brut — remplace exactement `emitted`.

### 7.3 Les globalités aval sont séparées

La reverse search ordonne la découverte, pas la filtration scientifique. Cinq
classes restent distinctes : lecture globale de $X$, permutation et lots exacts
externalisables, partition horizontale vivante, jointure verticale adjacente et
identités éventuellement imposées par le contrat public. Les événements doivent
encore être triés par rayon exact, les égalités de $\beta$ groupées atomiquement,
puis les incidences actives et silencieuses consommées par les forêts et
`coverage_log`.

Ces globalités n'imposent ni catalogue résident, ni graphe d'arrangement
résident, ni mosaïque : des runs bornés, un tri externe, un locator externalisé
et des journaux append-only suffisent. En revanche, les identités de facettes et
cofaces exigées par le contrat v2 doivent encore être produites en flux; seule
une migration contractuelle versionnée permettrait de les remplacer par un
quotient $H_0$ normalisé.

La factorisation complète, les obstructions au commit précoce et les compteurs
Gate D sont donnés dans
[`NOTE_GATE_D_GLOBALITES_RESIDUELLES.md`](NOTE_GATE_D_GLOBALITES_RESIDUELLES.md).

## 8. Ce qui reste ouvert pour Gate D

La règle ferme l'existence, l'unicité et la terminaison du parent; les quantités
suivantes restent décisives pour le contrat 50 k :

- $V_k$, nombre de sommets de niveau au plus $k_{\mathrm{nav}}$;
- $f_3(v)$ et sa queue lourde, car les **enfants** exigent encore tous les flats
  incidents réels;
- nombre de contraintes, pivots exacts et reprises du programme local;
- coût et charge de chaque requête `next` indexée;
- profondeur maximale de l'arbre et distribution du nombre d'enfants;
- octets de sortie, runs triés, high-water et lots exacts de rayon.

Une grande coquille peut posséder un nombre combinatoire de flats distincts.
Le parent local ne doit donc pas être présenté comme une borne de temps; il
retire précisément la mémoire globale et le second balayage des voisins dans le
calcul du parent.

## 9. Porte de falsification avant intégration

Sur une énumération indépendante des vrais sommets et événements consécutifs,
la porte permanente doit exiger, pour chaque sommet hors racine :

1. tranche locale non vide et solution LP unique;
2. rayon sélectionné réellement extrême et flat fermé de rang trois;
3. `next` fini et adjacent;
4. $B(\pi(v))\subseteq B(v)$;
5. baisse du niveau ou hausse stricte de $L_{h(v)}$; au niveau zéro, baisse
   stricte de $Q_r$;
6. absence de cycle, racine unique et couverture identique au BFS exact;
7. invariance du catalogue final sous permutations, même si l'arbre interne
   utilise une clef canonique différente;
8. égalité entre le rayon du LP et l'oracle exhaustif des flats normalisés;
9. mutations séparées du signe tangent, de la tranche normalisée, du lot
   entrant, de $B(v)$, de la base indépendante du germe et du tie-break LP.

Les fixtures minimales sont le tétraèdre, le cube cosphérique, le pont à
coquille cinq, plusieurs flats distincts dans une même coquille, un lot entrant
multiple, un flat à une extrémité non bornée et un sommet où le parent conserve
$B$ tout en augmentant $L_{h(v)}$. Trois témoins ciblent les tie-breaks :

- `lex_admissible_cycle`, points `(14,6,1) (7,10,8) (3,5,11) (3,8,5)
  (7,7,3) (14,3,14)` : choisir seulement le voisin admissible de coquille
  minimale crée un cycle de longueur deux; le signe strict de $L_h$ le coupe;
- `lp_optimum_tie`, points `(1,1,7) (7,9,4) (1,2,6) (9,2,10) (0,3,5)
  (0,2,6)` : deux rayons normalisés ont le même $G_v$; les quatre tie-breaks
  sont indispensables;
- `level_zero_lex_cycle`, points `(0,4,0) (1,4,15) (10,7,2) (11,15,0)
  (6,14,14) (9,6,5) (14,11,0)` : un choix lexicographique sans $Q_r$ crée un
  cycle de longueur deux au niveau zéro.

Deux fixtures doivent protéger le propriétaire :

- `owner_signed_cone`, support
  $U=\left\lbrace(0,0,2),(4,0,2),(1,3,2)\right\rbrace$ et points additionnels
  $(2,1,1),(2,1,3)$ : $B_U$ contient ces deux points, $P_U$ est le segment des
  centres $(2,1,z)$ pour $0\leq z\leq4$, et $G_U=8$ y est constant. Le lex choisit
  $z=0$; le cône de chambre non signé rejette à tort sa bonne orientation, tandis
  que le cône signé $K^U_v$ l'accepte;
- `owner_multiple_supports`, cube u16 $\left\lbrace0,2\right\rbrace^3$ : les
  quatre paires antipodales ont la même boule et le même owner. Sans le rejet
  $U\neq U_{\mathrm{can}}$, l'owner seul émet quatre fois.

La garde de rang quatre doit elle aussi avoir une fixture affine deux : sans
elle, le cône conserve une direction de linéalité, la face optimale peut être
non bornée et le tie-break ne possède plus nécessairement de minimum.

## 10. Décision

- parent local simple : **prouvé**;
- extension aux sommets multiples du vrai graphe quotienté : **prouvée sous
  oracle exact des flats et des lots**;
- choix direct du parent par LP local et une requête `next` : **prouvé ici**;
- reverse search streamée sans `seen/frontier` dans sa décision : **implémentée
  et différenciée dans les deltas postérieurs décrits par le ledger continu**;
- décision « ce sommet courant est-il le parent ? » sans seconde requête `next` :
  **corollaire prouvé au §12, non encore intégré**;
- harvest de la source, coût de l'énumération des enfants et contrat 50 k :
  **ouverts, Gate D**.

## 11. Review du premier delta live de Claude

Snapshot observé pendant l'écriture : parent Git `1a0a1f8`, header live
SHA-256 `ce9244647c3bbead6332a70ada325995e4004c1ccdc36d618e2164d26b20a27d`,
différentiel live SHA-256
`0168d33d514483c408834b5570e9c63c4d54e2843ad07482d8ba77722218ffcf`.
Ces empreintes décrivent un delta non committé et doivent être remplacées à
chaque réécriture.

### 11.1 Crédit mathématique

Le delta réutilise les flats déjà énumérés, calcule exactement le signe tangent,
rejette toute orientation qui quitte $K_v$, impose la croissance de $L_h$ ou la
décroissance de $Q_r$, puis choisit une clef de flat déterministe. C'est bien le
corollaire du §3.4 : aucun LP n'est requis pour ce premier juge, et aucun voisin
n'est construit pour **choisir** la direction. Le BFS avec `seen` reste en place;
le delta juge le parent mais ne revendique pas encore une reverse search.

### 11.2 P0 historique fermé dans le live — les quatre premiers membres ne sont pas une base

Le premier delta construisait `root_potential_base` en prenant les quatre
premiers indices de `seed.shell`, sans vérifier leur indépendance. Une coquille
de rang quatre peut commencer par quatre points coplanaires. $Q_r$ n'avait alors
plus le germe pour unique zéro, et un autre sommet de niveau zéro devenait une
seconde racine.

Contre-exemple u16 exact :

```text
0=(0,0,1)  1=(0,1,0)  2=(0,1,1)
3=(1,0,0)  4=(1,1,0)  5=(2,0,0)
```

Le germe a la coquille `{0,2,3,4,5}`. La base fautive `{0,2,3,4}` est dans le
plan `x+z=1`, donc de rang normal trois. Le sommet de niveau zéro de coquille
`{0,1,2,3,4}` contient les mêmes quatre hyperplans; il vérifie lui aussi
$Q_r=0$. Le rejeu ciblé du snapshot fautif rendait sept sommets et **deux
racines**.

Le live applique maintenant la correction sans combinaison de quatre ni LP : il
parcourt la coquille, prend un premier point, un deuxième distinct, un troisième
non collinéaire, puis un quatrième avec `orient3d_exact != 0`; un échec rend
`kInvariantViolated`. La fixture `germe_base_non_independante` est permanente.

Un rejeu exact externe sur ce snapshot couvre 5 623 nuages, 146 729 sommets et
15 258 sommets multiples; 568 germes avaient un préfixe de quatre points
coplanaires. Il vérifie le rang quatre de la base, une racine unique,
$B(\pi(v))\subseteq B(v)$, 123 240 hausses rationnelles strictes de $L_h$, 17 866
baisses rationnelles strictes de $Q_r$ et l'absence de cycle : zéro échec. Ce
rejeu crédite le correctif; il reste transitoire tant que ces contrôles ne sont
pas tous gravés dans la porte du dépôt. Le build du snapshot réussit et CTest
rend 39/39 tests verts en 676,03 s, dont les quatre portes flats.

### 11.3 Deux qualifications de claims

- Le test $B(v)\subseteq B_U$ de la récolte est un **préfiltre nécessaire**, pas
  une reconnaissance du propriétaire canonique : il ne vérifie pas les membres
  de $B_U$ devenus extérieurs en $v$. La table globale `emitted` reste donc
  nécessaire dans ce delta. Fixture u16 minimale :

  ```text
  0=(0,4,2)  1=(4,4,2)  2=(2,5,2)  3=(2,0,4)  4=(2,0,0)
  ```

  Pour $U=\lbrace0,1\rbrace$, la boule diamètre a $B_U=\lbrace2\rbrace$. Trois
  sommets passent $B(v)\subseteq B_U$; celui de coquille
  $\lbrace0,1,3,4\rbrace$ échoue la seconde inclusion, car le point 2 est devenu
  extérieur. Les deux autres passent les deux inclusions et ont le même premier
  objectif $G_U=8$ : le tie-break rationnel choisit seul le propriétaire. Sur le
  live, le catalogue indexé reste exact mais compte encore dix coquilles
  dupliquées sur cette fixture.
- La porte live vérifie racines, inclusion et cycles, mais son bloc est encore
  sauté silencieusement si le second parcours échoue ou si le vecteur de parents
  a une mauvaise taille. Elle ne vérifie pas directement le rang de la base du
  germe, l'extrémité finie, la stricte variation du potentiel, le rang trois de
  $C(d)$ ni l'identité $S(\mathrm{next})=C(d)\cup A$. Ces assertions doivent
  devenir fail-closed avant le remplacement effectif du BFS.

## 12. Corollaire post-`6a13b64` : décider le parent sans requête de retour

Le commit `6a13b64`, épinglé par
`order_k_flats.hpp=deb6858a4433806c801be2281a505ee08e06c1e50fd938aa4ed68d465b819270`
et
`flats_differential.cpp=c777ad9cd49498fb8de2ce1c812035da6232d50f71333068d87f85e619ced0e7`,
ajoute un préfiltre exact avant le calcul complet du parent. Soit un candidat
$w=\mathrm{next}(v,C,d)$ obtenu depuis $v$ sur le flat fermé $C$ dans la
direction $d$. Si $\pi(w)=v$, le couple retour $(C,-d)$ doit :

- rester dans le cône de chambre de $w$;
- faire croître strictement $L_{h(w)}$ lorsque $B(w)$ est non vide;
- faire décroître strictement $Q_r$ au niveau zéro.

`backward_pair_admissible` teste exactement ces trois conditions avec les mêmes
signes entiers que `canonical_parent`. La base ordonnée et l'orientation doivent
être transportées **ensemble** : une permutation impaire de la base sans
inversion de l'orientation changerait le signe et rendrait le filtre faux. La
fermeture reste la même aux deux extrémités; les nouveaux membres du lot ont un
`orient3d` non nul et n'agrandissent donc pas le plan. Un résultat `false`
certifie bien $\pi(w)\neq v$. Un résultat `true` n'est que nécessaire : un autre
couple admissible peut précéder le retour dans l'ordre canonique.

Un build Release isolé du commit passe les cinq CTests flats ciblés en 83,31 s.
Les campagnes fixtures, générique, dégénérée et cosphérique refusent
respectivement 5 265, 400 520, 451 359 et 372 968 candidats par le préfiltre,
avec zéro désaccord. Elles vérifient empiriquement les identités

$$N_{\mathrm{cand}}=N_{\mathrm{reject\_back}}+N_{\mathrm{parent\_queries}},\qquad N_{\mathrm{parent\_queries}}=N_{\mathrm{reject\_parent}}+N_{\mathrm{vertices}}-N_{\mathrm{roots}}.$$

La qualification reste partielle : aucun plancher ne porte sur les refus, et
chaque `false` n'est pas rejoué directement contre
`canonical_parent(...,full_scan=true)`. Le mutant « toujours `true` » laisserait
l'optimisation morte tout en gardant la porte verte. La comparaison finale
reverse--BFS détecte indirectement une censure sur les campagnes exécutées, sans
constituer la mutation ciblée.

Le même raisonnement donne un corollaire plus fort que le commit. L'adjacence du
pinceau est symétrique : puisque $w$ est le prochain événement depuis $v$ le long
de $(C,d)$, le prochain événement depuis $w$ le long de $(C,-d)$ est déjà connu,
c'est $v$. Après un préfiltre positif, il est donc inutile d'appeler une seconde
fois `neighbour_along`. Il suffit d'énumérer les couples de $w$ **jusqu'à** la
clef canonique $(C,-d)$ :

1. si un couple admissible strictement antérieur apparaît, rejeter $w$;
2. si la clef retour est atteinte et admissible, accepter $w$ comme fils de $v$;
3. si la fermeture manque, si l'ordre régresse ou si la clef retour n'est pas
   admissible, échouer fermé.

Cette décision supprime toutes les requêtes de voisin-parent restantes; elle ne
supprime pas les fermetures du préfixe canonique. Le sujet doit recevoir la
fermeture complète $C$ avec la base et l'orientation, puis conserver l'ancien
`parent_of` comme oracle différentiel de symétrie, jamais comme autorité partagée.

Fixture entière compacte pour les deux potentiels :

```text
A=(0,0,1) B=(1,0,1) C=(0,1,1)
E=(0,0,0) D=(0,0,2) F=(0,0,3)
```

Avec `base=[A,B,C]` et `root_base=[A,B,C,D]`, les orientations exactes valent
`orient(E)=-1`, `orient(D)=1`, `orient(F)=2`. Les sommets `ABCE` et `ABCD`
sont de niveau zéro, tandis que `ABCF` a `D` intérieur. La porte doit accepter
le retour niveau zéro `ABCE -> ABCD`, refuser son sens opposé, accepter le retour
niveau positif `ABCF -> ABCD` et refuser le retour qui quitte la chambre. Les
mutations signe inversé, base permutée seule, membre de coquille omis, couple
antérieur ignoré et target absente doivent toutes rougir.

GCP non utilisé.
