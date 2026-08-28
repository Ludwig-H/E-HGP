# ⚠ RÉTRACTATION CONSOLIDÉE EN TÊTE — lire ceci avant le reste

Deux projections successives sont retirées. La première annonçait 31 jours sur
`terrain` et un déficit « exact » de 0,86 à partir de `jung_cert_skip`, qui
n'est ni le coût de q4 ni son temps. Son remplacement par `completions_q4`
améliorait l'unité de boucle, mais ne transformait toujours pas ce compteur en
horloge : les temps `2 s / 42 s / 1,3 min` à 10 M sont donc retirés eux aussi.

À 50 k, les débits réellement observés du corps q4 sont seulement 121,5 M
complétions/s sur `uniform`, 66,5 M/s sur `terrain` et 67,5 M/s sur `scanline`,
soit 395 à 722 fois sous les 48 G/s supposés. Sur `terrain`, la pente de
`completions_q4` décroît alors que celles du corps q4, de la génération et du
mur augmentent. Ce compteur ne permet donc de conclure ni que le régime
s'améliore, ni que seul `scanline` se dégrade, ni que la génération n'est pas
un mur.

La campagne **historique** au pin source `82f613d3`, 48 fils CPU, donne le
diagnostic le plus utile sur la série homogène du binaire produit 50/100/200 k.
Sur `scanline_single_pass`, le mur suit les pentes locales 1,69 puis 2,716 et
la somme des trois corps `rects` 2,14 puis 3,135. À 200 k, q4 seul prend
214,544 s, soit 80,14 % d'un mur de 267,701 s ; les trois corps prennent
239,579 s, soit 89,50 %, et le fold 14,269 s, soit 5,33 %. Les parts des corps
valent 19,04 % à 8 k, 66,93 % à 100 k et 89,50 % à 200 k. Cette mesure doit
être rejouée au pin courant avant toute conclusion de performance actuelle.

Entre 8 k et 200 k, les ancres q4 croissent de ×238,21, mais les candidats q4
émis de ×14,95 : les ancres par candidat passent de 16,94 à 269,94, soit une
dégradation de rendement ×15,94, pas ×238. Seuls ces candidats q4 provisoires
sont sous-linéaires ; candidats totaux, boules, événements et facettes restent
approximativement linéaires. Le reçu localise donc le coût dans q4 sans encore
le partager entre visites de points durant la construction des covers, sites
retenus, tests de cœur, entrées de profondeur et appels de puissance.

Les projections 10 M restent conditionnelles à la conservation de la seule
pente 100→200 k : 4,97 h pour `uniform`, 17,18 h pour `eight_clusters` et
127,7 jours pour `scanline`. Ce ne sont pas des tests. Même l'extrapolation
temporelle favorable de `uniform` échoue largement en résidence. Pour
`scanline`, prolonger la pente locale du pic GNU donne environ 308 Gio et le
palier interne environ 318 Gio ; le seul pic mesuré est 10,60 Gio à 200 k.
Garder deux rails : mur/RSS mesurés d'une part, masses de boucles et
architecture d'autre part.

**Leçon de méthode :** un compteur de boucle n'est pas une horloge, une pente
locale n'est pas une loi asymptotique et une projection n'est pas une mesure.

---

# Question Claude — la sous-quadraticité **par régime**, et ce qu'elle exige (28 août 2026)

Ancrage : reçus de génération épinglés depuis `839cf1ec`, compteurs
`seeds`/`completions_q4` ajoutés et reçus à `c95cfa95`, campagne directe
`scanline` 200 k et instruments exploratoires jusqu'à `819cac3c` ; la fraîcheur
du pin jugé est tenue dans [`ETAT_COURANT.md`](ETAT_COURANT.md).
Cadre : `phase=exploration_v5_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

## Réponse à Louis — généraliser la WSPD, mais par les centres

### Verdict

**Oui, il faut généraliser, mais pas en une WSPD symétrique de triplets ou de
quadruplets.** La bonne cible est une décomposition à deux étages : la WSPD de
paires reste le squelette exact et possède chaque ancre diamètre une fois ;
derrière chaque ancre survivante, un arrangement local unique traite les
complétions q3 et q4 sans développer leur produit cartésien.

La généralisation nommée existe dans la littérature : la
[WSSD de Kerber--Sharathkumar](https://arxiv.org/abs/1307.3272) couvre chaque
simplexe par un tuple de cellules bien séparées et donne une approximation de
Čech de taille linéaire à dimension fixée. Elle est utile comme broad phase ou
comme vocabulaire, mais elle n'est ni une partition exacte des supports, ni une
autorité de rang, de shell ou d'exact-once. La rendre exacte en développant
`A x B x X` ou `A x B x X x Y` recrée précisément le coût recherché. La
frontière directe de produits déjà testée confirme ce no-go pratique : ses
boîtes mélangent presque toujours supports acceptés et refusés, et un certificat
ne couvrait qu'environ 1,1 tuple dans
[`RAPPORT_SESSION_20260808.md`](../../docs/research/RAPPORT_SESSION_20260808.md).

Le nom de travail utile est donc **décomposition simpliciale bien séparée
fibrée par l'ancre**, pas WSSD standard :

1. `PairBlock(A,B)` partitionne les paires par la WSPD actuelle ; un certificat
   universel peut tuer tout le bloc, sinon il est raffiné ou émet des ancres ;
2. `AnchorCenterArrangement(a,b)` traite ensemble tous les tiers d'une ancre,
   en scratch borné puis en flux vers le RLE existant.

### Ce que le code courant propose réellement pour q3 et q4

La WSPD courante possède chaque paire non ordonnée une fois, mais le chemin
produit développe ensuite chaque rectangle vivant `A x B` par la double boucle
sur toutes ses positions. En notant $A_q=\sum_r\lvert A_r\rvert\lvert B_r\rvert$,
on a $A_q\leq\binom{n}{2}$, sans garantie que cette masse vivante soit linéaire.
Sur `scanline` 100 k → 200 k, elle suit localement $n^{1{,}981}$ en q4 tandis
que le nombre de rectangles reste presque linéaire : la compacité WSPD est
perdue au moment de proposer les ancres.

Pour une ancre `(a,b)` de longueur carrée `D2`, le cover coefficient 3 retient
chaque site vérifiant $\lVert 2x-a-b\rVert^{2}\leq 3D^{2}$. Les handles sont
partagés au niveau du rectangle, mais leurs points sont balayés de nouveau pour
chaque ancre survivante; ce n'est ni un voisinage de taille bornée ni un k-NN.

- **q3 :** chaque point du cover est essayé comme `x`. Il survit seulement si
  $\lVert x-a\rVert^{2}\leq D^{2}$,
  $\lVert x-b\rVert^{2}\leq D^{2}$ et
  $\lVert 2x-a-b\rVert^{2}>D^{2}$, puis si `(a,b)` est l'arête maximale
  canonique du triangle. Chaque seed survivant rescane ensuite le cover pour sa
  profondeur stricte, jusqu'à `h3`.
- **q4 :** le code forme d'abord `lens`, sous-ensemble du cover satisfaisant les
  deux premières inégalités. Chaque point de `lens` est essayé comme `x` aigu;
  le cœur et la corde de ce seed rescannent le cover. Si le seed vit, une seconde
  boucle essaie **chaque** `y` de `lens`. Le compteur `q4_completions` est
  incrémenté avant les rejets `x-y`, owner, exact-once, bien-centrage et forme
  q4. Toute complétion parvenue à la profondeur rescane encore le cover.

La réponse à « l'algorithme est-il quadratique ? » est donc : **le catalogue
d'ancres peut l'être presque, et le chemin aval n'est pas borné au carré par sa
structure de boucles**. Les majorants syntaxiques, volontairement lâches, sont
$O(n^{4})$ pour q3 — jusqu'à $O(n^{2})$ ancres, $O(n)$ tiers et un rescan
$O(n)$ — et $O(n^{5})$ pour q4 quand une complétion `x,y` paie encore un rescan.
Ils ne sont ni des bornes géométriques serrées ni des exposants mesurés. Les
supports distincts eux-mêmes sont bornés par $\binom{n}{3}$ et
$\binom{n}{4}$; le gaspillage vient des propositions et rescans avant la petite
sortie canonique. C'est pourquoi optimiser seulement un filtre placé après
`q4_completions` ne change pas l'exposant de proposition.

### L'objet commun q3/q4

Fixons l'ancre possédée `e=(a,b)`, son milieu $M$, sa longueur $D$ et son plan
médiateur. Chaque site $x$ non collinéaire à l'ancre induit dans ce plan la
droite orientée $h_x(v)=0$, avec une identité exacte déjà démontrée :

$$h_x(v)=2v\mathbin{\cdot}(x-M)-\left(\left\Vert x-M\right\Vert^{2}-\frac{D^{2}}{4}\right)=r^{2}-\left\Vert x-(M+v)\right\Vert^{2}.$$

Ainsi `h_x(v)>0`, `=0`, `<0` signifie respectivement intérieur strict, shell,
extérieur pour la sphère de centre `M+v`. Un site collinéaire induit une
fonction constante : intérieur universel, extérieur universel ou shell
universel selon sa position sur l'axe ; il faut le router vers le compte fixe
ou le census, pas inventer une droite. Ce même arrangement donne les deux lanes :

- **q3 :** `x` ne demande pas une nouvelle recherche géométrique. Après les
  filtres de lentille, d'acuité stricte et d'owner canonique, le centre du
  triangle `(a,b,x)` est le point marqué $v_x$ de norme minimale sur sa propre
  droite `h_x=0` — son intersection avec le plan affine du triangle. On ne
  conserve ce point marqué que si sa profondeur stricte est au plus
  $\kappa_3=h_3-1=s_{\max}-3$, soit **8** pour `smax=11`. Le scan actuel
  `x x cover` devient une localisation dans le préfixe peu profond commun ;
- **q4 :** le centre de `(a,b,x,y)` est le sommet commun à `h_x=0` et `h_y=0`.
  On énumère directement les sommets de profondeur au plus
  $\kappa_4=h_4-1=s_{\max}-4$, soit **7**, puis seulement les filtres exacts de
  diamètre, owner, bon centrage, shell et `BallKey`. On ne forme jamais les
  $\binom{m_e}{2}$ paires de tiers.

Les points intérieurs sur tout le disque sont comptés une fois dans
`c_{e,q}` ; le budget résiduel devient `kappa_q-c_{e,q}`. Le disque q3 est
contenu dans celui de q4 : rayons carrés respectifs `D2/12` et `D2/8`. Une
préparation des droites du disque extérieur peut donc servir les deux lanes
pour toute ancre commune, q3 n'interrogeant que son disque intérieur. Les
certificats W/secteurs, grille et morceaux de corde restent des pré-prunes
facultatifs : ils ne deviennent pas la source des sorties. Pour préserver
d'abord le contrat v5 et `digest_balls`, l'intégration doit garder le cover
coefficient 3 du chemin courant. Élargir le range-report q4 au confinement
complet de Jung peut tuer davantage de candidats profonds et constitue un
changement de contrat séparé, déjà averti par
[`PISTES_FERMEES.md`](../docs/PISTES_FERMEES.md).

Deux profondeurs doivent donc rester nommées séparément. La profondeur
$\delta_e^{\mathrm{full}}$ porte sur tous les intérieurs stricts de la sphère :
elle donne la contribution intérieure à la porte de profondeur du carrier. Le
rang fermé complet ajoute encore le shell, traité par le census et la politique
de plateau. La profondeur $\delta_e^{(3)}$ ne parcourt que le cover coefficient
3 et reproduit le filtre historique de génération. Pour q3, ce cover contient
tous les intérieurs ; pour q4, il contient les carriers utiles mais peut omettre
des intérieurs, comme le grave `mhgp5_q4_cover_fixture`. Préserver d'abord
`digest_balls` impose $\delta_e^{(3)}$ ; revendiquer la borne de rang exige
$\delta_e^{\mathrm{full}}$ avec le census du shell, ou une recertification
terminale équivalente. Ne jamais échanger silencieusement ces objets.

Le futur `center-cover` possède toutefois un garde de compatibilité exact,
simple et conservateur. Pour des boîtes d'extrémités $A,B$ et un nœud témoin
$W$, poser $I_i=[2w_i^- -a_i^+-b_i^+,\,2w_i^+ -a_i^- -b_i^-]$,
$U=\sum_i\max((\inf I_i)^2,(\sup I_i)^2)$ et
$L=\sum_i\mathrm{dist}([a_i^-,a_i^+],[b_i^-,b_i^+])^2$. Alors toute ancre
$p\in A,q\in B$ et tout $x\in W$ vérifient
$\lVert2x-p-q\rVert^2\leq U$ et $\lVert p-q\rVert^2\geq L$. Le test
$U\leq3L$, ajouté au certificat d'intériorité stricte, prouve donc que chaque
témoin crédité appartient aussi au cover coefficient 3 de **toute** ancre du
produit. Pour que le compte soit additif, chaque $W$ doit être disjoint en
plages de $A\cup B$ et les nœuds crédités doivent former une antichaîne sans
descendants comptés deux fois. Avec au moins $h_4$ témoins distincts ainsi
certifiés par patch, on peut supprimer ce produit de la **lane q4** sans changer
le multiensemble historique ni `digest_balls`; si une précondition ou le garde
échoue, la route compatible v4 reste fail-open. L'égalité $U=3L$ est admissible
pour l'appartenance au cover, tandis que l'intériorité demeure stricte. Sous le
profil u16, ces carrés tiennent en `i64`.

L'abstraction se généralise en dimension $d$, sans généraliser naïvement les
produits : une paire diamètre laisse un espace de centres de dimension $d-1$ ;
chacun des $q-2$ porteurs supplémentaires y ajoute un hyperplan orienté ; le
centre du support est le point de norme minimale de leur intersection, et son
budget de profondeur stricte vaut $s_{\max}-q$. En 3D, cela donne `q2 = v=0`,
`q3 = point marqué sur une droite`, `q4 = intersection de deux droites`. Cette
formulation unifie les preuves et les données, mais ne promet pas la même borne
combinatoire en dimension supérieure.

### Enveloppe entière immédiate : ne pas attendre l'arrangement

Le dernier diagnostic de cover a posé comme ouverte l'existence d'un
sur-ensemble entier plus serré que la boule coefficient 3. Cette existence se
ferme directement. Posons

$$d=b-a,\qquad D^{2}=\lVert d\rVert^{2},\qquad w=2z-a-b,\qquad S=\lVert w\rVert^{2}-D^{2},\qquad \Xi=\lVert d\times w\rVert^{2}.$$

Pour q3, un centre admissible s'écrit $m+v$, avec $v\perp d$ et
$\lVert v\rVert\leq D/(2\sqrt{3})$, et sa boule a
$R^{2}=D^{2}/4+\lVert v\rVert^{2}$. Maximiser le produit scalaire transverse
sur ce disque donne l'enveloppe continue exacte de toutes les circumboules q3
géométriquement admissibles :

$$z\in U_3(a,b)\quad\Longleftrightarrow\quad S\leq0\quad\text{ou}\quad 3S^{2}\leq4\Xi.$$

En coordonnées axiale $t$ et radiale $r$, la même région est
$t^{2}+(r-D/(2\sqrt{3}))^{2}\leq D^{2}/3$. Elle est un solide de révolution
pour la famille **continue** des centres ; l'union finie des boules réellement
proposées par un nuage ne l'est généralement pas. L'équilatéral atteint la
frontière extérieure $\sqrt{3}D/2$ : la boule coefficient 3 est bien la plus
petite boule centrée en $m$ qui englobe $U_3$, mais elle est strictement plus
large que $U_3$ ; sur l'axe, par exemple, $U_3$ s'arrête à
$\lvert t\rvert=D/2$ alors que la boule englobante va jusqu'à
$\sqrt{3}D/2$.

Pour q4, Jung donne seulement $\lVert v\rVert\leq D/(2\sqrt{2})$. Il en découle
le sur-ensemble sûr, sans prétendre qu'il décrit exactement tous les centres de
tétraèdres réalisables :

$$z\in U_4^{J}(a,b)\quad\Longleftarrow\quad z\text{ appartient à une boule q4 admissible},\qquad U_4^{J}(a,b)=\left\lbrace z:S\leq0\text{ ou }S^{2}\leq2\Xi\right\rbrace.$$

Le cover q4 historique coefficient 3 n'est pas un sur-ensemble de toutes les
boules q4. La réduction compatible est donc son **intersection** avec
$U_4^{J}$, jamais son remplacement par l'enveloppe Jung entière. Tout porteur,
point de coquille ou intérieur réellement utile appartient à la boule
candidate correspondante ; supprimer de ce cover un point hors $U_4^{J}$ ne
peut donc pas changer une décision exacte. Cette dernière phrase doit néanmoins
être reçue par oracle et digest avant intégration.

Une descente d'arbre fail-open vient sans racine. Pour une boîte de nœud, soit
$Q_{\min}$ la borne inférieure exacte de $\lVert w\rVert^{2}$ par distances aux
trois intervalles et $\Xi_{\max}$ le maximum de $\lVert d\times w\rVert^{2}$
aux huit coins — le maximum est aux coins car cette forme est convexe. Si
$Q_{\min}>D^{2}$, le nœud est extérieur à q3 sous
$3(Q_{\min}-D^{2})^{2}>4\Xi_{\max}$, et extérieur à l'enveloppe Jung q4 sous
$(Q_{\min}-D^{2})^{2}>2\Xi_{\max}$. L'égalité reste conservée ; les feuilles
emploient le prédicat ponctuel fermé. Le profil u16 tient en `i128`.

Plan court et falsifiable pour Claude :

1. filtrer d'abord les sites **après** la visite actuelle des handles, sans
   changer l'arbre ; publier `sites_avant/après` séparément en q3/q4 et mesurer
   le coût du produit vectoriel exact ;
2. exiger le même multiensemble brut, `digest_balls`, événements, niveaux et
   forêts sur les six familles, avec frontières équilatérale q3 et régulière q4
   et mutants de stricte inégalité/facteur ;
3. seulement si le filtre réduit le travail réel, pousser le certificat de
   boîte dans la descente et comparer nœuds/visites/mur sur un protocole calme ;
4. garder l'arrangement shallow : cette enveloppe réduit les scans et $m_e$,
   mais ne change pas à elle seule le pire exposant du catalogue d'ancres.

### Borne locale visée, et limite honnête

Pour une ancre `e`, notons `m_e` le nombre de droites actives. En position
générale, le nombre de sommets q4 admissibles par profondeur vérifie
$Z_e\leq m_e(\kappa_e+1)$ ; à `smax=11`, cela donne au plus `8*m_e`, contre
`m_e*(m_e-1)/2` intersections dans le développement naïf. Ce dernier nombre
n'est pas le compteur actuel `q4_completions`, placé après plusieurs filtres et
gouverné aussi par le nombre de seeds. Les constructions de niveaux peu
profonds donnent comme cible théorique locale
$O(m_e\log m_e+m_e(\kappa_e+1))$. Le document
[`RNG_JUNG_CLIQUES_ET_NIVEAUX_PEUPROFONDS.md`](../../docs/math/RNG_JUNG_CLIQUES_ET_NIVEAUX_PEUPROFONDS.md)
porte la preuve géométrique, les bornes et leurs limites.

La complexité totale revendicable reste conditionnelle :

```text
T = T_pair_blocks + T_range
  + sum_e O(m_e log(m_e) + m_e*(kappa_e+1))
  + T_exact + T_sink
```

Cette architecture supprime le carré **local** de q4 et le rescan q3 par `x`.
Elle ne prouve pas que le nombre d'ancres `a`, les blocs visités ou
`M=sum_e m_e` sont sous-quadratiques. C'est la WSPD extérieure et ses
certificats center-cover qui doivent contenir ces quantités. Le pire cas reste
dense ; le contrat réaliste est de mesurer `a`, `M` et `sum Z_e` par régime.
En dimension trois, il n'existe pas de support minimal de miniboule au-delà de
quatre points : il faut généraliser l'abstraction, pas ouvrir q5.

### Plan qui aide Claude sans engager une refonte aveugle

1. **R0, sonde sans décision :** à l'échelle, publier `m_e`, `c_e`, `kappa_e`,
   `sum binom(m_e,2)` et `sum m_e*(kappa_e+1)` comme majorants analytiques,
   quantiles/maxima, temps de range, exact et sink, plus le pic scratch. Ne
   calculer `Z_e` que sur les petites ancres ou un échantillon explicitement
   borné : le former exhaustivement reconstruit précisément les
   $\binom{m_e}{2}$ intersections que la sonde doit éviter.
2. **R1, oracle borné :** pour petits nuages seulement, développer toutes les
   intersections, grouper les concurrences exactement et comparer le
   multiensemble de `BallCandidate` pré-RLE, les `BallKey` post-RLE,
   `digest_balls`, le statut, les événements, niveaux et forêts au chemin
   courant. Ne pas exiger l'égalité des compteurs de travail historiques
   (`seeds`, `q4_completions`, rejets ou certificats), puisque les boucles qui
   les définissent disparaissent ; versionner de nouveaux compteurs propres au
   constructeur shallow.
3. **R2, vrai constructeur shallow CPU :** scan plat pour les ancres légères,
   arrangement q4 pour les moyennes/lourdes, puis q3 lourd si la préparation
   commune paie. Un prototype qui calcule d'abord les
   `binom(m_e,2)` intersections échoue la porte même s'il filtre ensuite.
4. **R3 seulement si l'amont domine encore :** ajouter le center-cover exact
   par blocs d'extrémités avant l'émission des ancres. Ne pas revenir au produit
   symétrique de quatre nœuds.

### R0a livré — les masses internes ferment enfin la comptabilité q4

Le worktree ajoute, sous `MHGP5_PROFILE_Q4` seulement, six masses : covers
construits, visites de points durant leur construction, sites retenus, tests de
cœur, entrées du filtre de profondeur et appels à `q4_power`. La sonde refuse
son résultat si l'une des trois partitions cœur, profondeur ou complétions ne
ferme pas. Les quatre runs locaux `n=400`, un fil, avec prétest par requête
forcé, ferment les trois identités :

| famille | tests cœur | complétions | entrées profondeur | appels `q4_power` |
|---|---:|---:|---:|---:|
| `uniform` | 5 783 195 | 1 551 035 | 159 932 | 2 762 711 |
| `eight_clusters` | 8 150 461 | 1 594 384 | 126 942 | 2 529 198 |
| `terrain` | 756 870 | 170 629 | 3 410 | 51 505 |
| `scanline_single_pass` | 1 233 035 | 273 392 | 10 112 | 145 096 |

À cette petite taille, seulement 2,0 % des complétions `terrain` et 3,7 % des
complétions `scanline` atteignent la profondeur. Une descente radix exacte sur
le cover coefficient 3 reste utile pour supprimer les appels `q4_power`, mais
elle intervient trop tard pour enlever les scans de cœur ou la formation des
complétions. L'ordre d'aide à Claude est donc : réutiliser les certificats
partiels cœur/corde par `x`, faire rejeter exactement `|x-y|^2>D^2` avant la
forme q4, puis intégrer la descente radix ; l'arrangement shallow est la voie
qui peut retirer la formation `x × y` elle-même. Rejouer ces masses à
50/100/200 k sur le même pin est indispensable avant de classer leurs gains.
Ces runs R0a sont des ratios exploratoires, pas un reçu de scalabilité; leurs
chronométrages à un passage sont volontairement exclus du tableau.

La nouvelle fraction $f_4(n)=\mathrm{anchors}[2]/\binom{n}{2}$ fournit un
second diagnostic propre. Sur `scanline`, elle passe seulement de 0,971 % à
0,959 % entre 100 k et 200 k, soit une pente locale $n^{-0{,}02}$ et donc une
masse d'ancres explicites proche de $n^{1{,}98}$ sur ce doublement. Pour rendre
**ce catalogue explicite** linéaire, demander $f_4=O(n^{-1})$ est exactement
équivalent. Ce n'est toutefois pas l'unique contrat algorithmique possible :
le center-cover peut fermer un bloc sans émettre ses ancres, et l'arrangement
peut partager le traitement de nombreuses ancres. Le vrai objectif est donc
« masse d'ancres explicites linéaire **ou** traitement implicite certifié à
travail sous-quadratique », pas obligatoirement « tout résoudre par l'élagage
de rectangle ». Cette fraction ne décrit en outre que q4 et une pente sur deux
points ; publier séparément q2/q3/q4, le ledger des paires et le coût du
raffinement.

Le mode concurrent `--descente-seule` de `rect_probe` est un plafond
exploratoire, pas encore un algorithme reçu : il reprend une profondeur 40 déjà
refusée comme route produit, ne rejoue ni covers, ni candidats, ni digests, et
aucune commande ou sortie brute n'est encore au tip. Sa comptabilité doit
fermer `paires_avant = paires_tuées + paires_après`, prouver que chaque split
partitionne le produit et rapporter le travail par paire retirée. Une campagne
de **compteurs** déterministes peut bien tourner localement ; seuls les temps
comparables demandent la machine gardée.

Les cas dégénérés sont contractuels : droites parallèles, identiques ou
concourantes, fonctions constantes, centre sur la frontière du disque,
profondeurs 8/9 en q3 et 7/8 en q4, shell cosphérique jusqu'au cap, ties d'owner
et `det=0`. Attention : le shell complet ne participe pas à la profondeur
stricte et ne borne pas la pertinence d'un sous-support. Douze points
cosphériques peuvent avoir un rang fermé complet 12 tout en contenant un
tétraèdre bien centré de cardinalité quatre pertinent à `smax=11` ; la fixture
`mhgp5_plateau_shell_relevance` grave ce contre-exemple. Les concurrences sont
groupées par centre, sans jitter, puis passées au census complet.

Un groupement par seul `BallKey` ne suffit toutefois pas encore à préserver le
contrat observable : à clé égale, le RLE choisit l'arité puis la représentation
brute minimale de `ExactLevel`, et `q4_level_raw` dépend du carrier. Sous succès
du cap de shell 12, une ancre n'a au plus que dix autres sites sur ce shell ;
examiner au plus 45 couples **après** avoir trouvé le centre permet donc de
retrouver le représentant historique sans réintroduire le carré en `m_e`. La
porte R1 doit comparer le `BallCandidate` post-RLE complet, les digests, statuts
et forêt, pas seulement l'ensemble des `BallKey`. Au-delà du cap, le statut
reste `resource_exhausted`, jamais une omission. Les mutants minimaux retirent
ou dupliquent une droite, changent `<` en `<=`, décalent `kappa` de un et
éclatent une concurrence.

### Correction du dernier contre-proxy

La rétractation de « 31 jours » et de « il manque exactement 0,86 » est juste.
En revanche, les nouveaux temps `2 s / 42 s / 1,3 min` ne sont pas reçus : ils
appliquent encore le débit non mesuré `4,8e10/s`, cette fois à
`completions_q4`, une unité différente de celle qui avait inspiré ce débit.
`completions_q4` est un meilleur compteur de boucle que `jung_cert_skip`, mais
pas une horloge.

Le reçu direct `scanline` 200 k avec grille mesure déjà **214,544 s** dans le
corps q4 sur **267,701 s** de mur. Il interdit donc la conclusion générale
« la génération n'est pas le mur » et rend a fortiori le temps 10 M de 1,3 min
non interprétable. La mémoire reste un chantier prioritaire, mais elle ne
disqualifie pas la suppression architecturale de `x x y` : conserver deux rails
séparés, mur/RSS mesurés d'une part, compteurs de travail d'autre part. La note
autonome de rétractation est consolidée ici puis retirée du tip.

La contradiction est visible sans extrapoler jusqu'à 200 k. À 50 k, le débit
`completions_q4 / t_rects_q4` vaut seulement 121,5 M/s sur `uniform`, 66,5 M/s
sur `terrain` et 67,5 M/s sur `scanline`, soit respectivement 395, 722 et 711
fois moins que 48 G/s. En conservant malgré tout les masses 10 M publiées et en
figeant ces débits, les temps conditionnels de la seule lane seraient environ
13 min, 8 h 21 et 15 h 38, pas 2 s, 42 s et 1,3 min. Ils ne constituent pas
plus une projection produit ; ils montrent seulement l'erreur d'unité.

Surtout, la baisse du compteur masque une hausse du coût payé. Sur `terrain`,
les exposants locaux 8→16 k, 16→32 k et 32→50 k valent :

| grandeur | 8→16 k | 16→32 k | 32→50 k |
|---|---:|---:|---:|
| `completions_q4` | 2,065 | 1,805 | 1,561 |
| corps q4 | 2,221 | 2,302 | 2,450 |
| génération entière | 1,459 | 1,757 | 2,053 |
| mur de bout en bout | 1,177 | 1,394 | 1,654 |

Dire que `terrain` « s'améliore » ou que seul `scanline` se dégrade inverse
donc la mesure temporelle. Enfin, 0,354 MiB/point est le pic du run **uniform**
complet résident à 50 k ; les mêmes reçus donnent 0,075 sur `terrain` et 0,072
sur `scanline`. Son extrapolation linéaire à environ 3,5 TiB reste utile pour
refuser le chemin uniforme complet résident, pas pour déclarer la mémoire
« mur réel » de toutes les familles ou du futur profil streamé.

## Réponse auditée — V36 à V41

**Verdict court : V39 oui ; V40 oui seulement comme tuilage de scheduling ;
V41 est une bonne ablation q3/q4, mais pas encore un algorithme reçu ; V37 ne
ferme qu'une coupure aveugle ; V36 est refusé dans sa forme actuelle ; V38 doit
être reformulé avec un modèle de prétraitement.** Les calculs d'extrapolation
sont arithmétiquement reproductibles, mais leur interprétation « trois régimes
tiennent 10 M » n'est pas démontrée et est déjà contredite par la mesure
historique `scanline`.

Quatre faits doivent d'abord être corrigés :

1. Les tailles ne viennent pas du « même binaire » : 8/16/32 k ont été
   extraites de `mhgp5_conformity_v4`, 50 k de `mhgp5`. Aucun hash des deux
   binaires n'est épinglé. Les nuages sont
   régénérés avec un domaine `coord` différent ; ce ne sont pas des préfixes
   point à point. Le fichier `MESURE_CLAUDE_OU_EST_LA_QUADRATICITE_20260828.md`
   cité en ancrage a en outre été consolidé puis retiré du tip ; l'autorité
   active est la requalification de
   [`QUESTION_CLAUDE_LANE_RESIDENTE_20260828.md`](QUESTION_CLAUDE_LANE_RESIDENTE_20260828.md).
2. La colonne « évaluations Jung » est `jung_cert_skip`, pas le total des
   évaluations ni le temps q4. En sommant les trois champs imprimés
   `jung_cert_kill + jung_cert_skip + jung_fallback`, les pentes 32→50 k sont
   `1,055/1,042/2,109/3,040` pour uniform/clusters/scanline/terrain, au lieu de
   `1,056/1,064/2,212/3,144` pour le seul `skip`. Les scans de profondeur et les
   autres étages ne sont toujours pas contenus dans ce total.
3. La provenance du débit de `4,8e10` évaluations/s n'est pas établie. Il est
   compatible, sur le seul cas terrain 50 k, avec une double agrégation des 48
   fils :
   `7 677 090 545 / 7,8782 s = 9,74e8 skip/s` au mur ; multiplier encore par
   48 donne `4,68e10`, presque la constante annoncée. Cette coïncidence n'est
   pas une preuve causale, d'autant que le mur q4 contient d'autres coûts. Selon
   la famille, le débit
   mural observé du total Jung ne vaut qu'environ `1,03e8` à `1,02e9` par
   seconde : l'hypothèse est 47 à 465 fois trop haute. Elle prédit même uniform
   1 M en 0,2 s alors que q4 seul prend déjà 3,12 s à 50 k. En supposant malgré
   tout ce débit, une pente figée et aucun autre coût, les temps du tableau sont
   bien ceux du calcul ; ils ne décrivent pas le mur CPU. L'écart
   `3,14 - 2,28 = 0,86` est donc l'écart entre deux hypothèses incompatibles avec
   le reçu, pas l'exposant « exactement manquant » à l'algorithme.
4. La session 11, au pin historique `82f613d3`, mesure déjà `scanline` à 100 k
   et 200 k. Sur 50/100/200 k,
   `jung_cert_skip` a des pentes 2,586 puis 3,220 et le mur q4 2,216 puis
   3,305. Même le débit fictif de 48 G/s, appliqué au dernier segment depuis
   200 k, projette environ 206 h à 10 M, pas 1,5 h. Les débits effectivement
   observés à 50 k ne valent d'ailleurs qu'environ 0,05 à 0,98 G de
   `jung_cert_skip` par seconde selon la famille.

À titre de diagnostic 32→50 k seulement, les pentes des temps effectivement
mesurés sont très différentes de celle du proxy :

| famille | rectangles q4 | génération | mur complet |
|---|---:|---:|---:|
| `uniform` | 1,285 | 1,195 | 1,105 |
| `eight_clusters` | 1,365 | 1,271 | 1,198 |
| `scanline_single_pass` | 2,160 | 1,815 | 1,440 |
| `terrain` | 2,398 | 2,023 | 1,632 |

Les deux extrémités utilisent encore des exécutables distincts : ces nombres
localisent des postes, ils ne constituent ni des lois asymptotiques ni une
preuve du budget 10 M.

### V36 — séparer complexité et budget produit

Ne pas recevoir la porte proposée. D'une part, un seuil 2,30 ou 2,20 ne définit
pas une sous-quadraticité. D'autre part, un seuil unique par famille ne peut pas
être appliqué à des compteurs de bases et d'unités différentes. La porte
annoncée « au-dessus des mesures actuelles sauf `terrain` » échouerait déjà :

- `eight_clusters`, seuil 1,20 : ancres q3 `1,54/1,53/1,34`, ancres q4
  `1,49/1,67/1,40`, et proxy Jung `1,27` sur le premier intervalle ;
- `scanline`, seuil 2,30 : proxy Jung `2,74` entre 16 k et 32 k ;
- `terrain`, seuil 2,20 : proxy Jung `2,82/2,83/3,14`.

Dans le même modèle fictif, les exposants admissibles pour 30 M ne seraient
plus que `2,395/2,320/2,091/1,892` : les seuils 2,30 et 2,20 ne garantissent
donc même pas le budget 30 M annoncé.

Conserver deux rails distincts : un **diagnostic de pente** par compteur,
source et intervalle explicitement épinglés, puis un **budget produit** sur le
mur de bout en bout et le pic mémoire d'une taille cible. Le premier reste un
rapport de benchmark tant que les familles, binaires et tailles ne sont pas
stables ; ne pas ajouter maintenant un CTest volontairement rouge ni un code 3.
Le second ne peut être extrapolé depuis `jung_cert_skip`.

Concrètement, la garde déterministe de non-régression doit être indexée par
`(famille, compteur payé, intervalle)` et comparer un même binaire produit
hashé sur des tailles et graines fixes, sans claim statistique. La scorecard de
recherche peut viser une pente sous 2 sans faire échouer CTest, mais doit aussi
publier les pentes adjacentes et les résidus : un unique ajustement log-log peut
masquer la courbure déjà observée. Toute inférence de famille exige un protocole
préenregistré qui fixe tailles, graines, répétitions, niveau de confiance et
unité de rééchantillonnage ; cinq pentes ne justifient pas à elles seules une
borne bootstrap. Les temps demandent des répétitions appariées et un
environnement épinglé. Les SLO produit restent absolus : mur, RAM, SSD et octets
de sortie à 50 k, pont réel résident-streamé à 1 M, puis contrats séparés 10 M
`prefixe_k5` et complet. Un compteur de sortie ne doit jamais être soumis au
plafond du travail payé.

### V37 — fermer seulement la coupure aveugle

La mesure rend injustifiable une règle du type « rejeter tout rectangle dont
`Dmax >= 64` » : elle contient encore des candidats pré-RLE dans cette classe.
Pour prouver un changement de l'objet canonique, il manque toutefois un mutant
ON/OFF et l'inégalité de `digest_balls` ; un candidat brut peut être dupliqué.
Elle ne réfute ni un certificat exact dépendant du rayon, ni un raffinement qui
conserve toutes les paires. Inversement, zéro candidat observé sur un run
`uniform` ne rend pas la coupure exacte sur cette famille.

Les pourcentages cités sont `seeds` contrefactuels et candidats pré-RLE du
probe, pas le travail résiduel du produit ni son objet final ; leurs sorties
brutes ne sont pas versionnées. `Dmax=64` n'est en outre normalisé ni entre
familles ni entre tailles. Une entrée de `PISTES_FERMEES.md` peut donc viser
précisément la **coupure fixe aveugle par `Dmax`**, après fixture, sortie brute
et mutant épinglés ; pas « toute coupe par rayon ».

Le nouveau `plafond_test_rectangle` ne mesure pas un plafond. Les tests W,
secteurs et grille sont suffisants mais non nécessaires ; un futur certificat
peut tuer ce qu'ils laissent vivre, et le raffinement peut tuer les enfants
d'un parent mixte. De plus, `alive` signifie post-histogramme en q3 mais
post-W4 en q4, `killed` omet W4 et la grille, et `seeds`/`covers` suivent le
rejeu contrefactuel. Renommer ce bloc en diagnostic après alignement du vrai
flux, ou le retirer ; sa phrase « gain MAXIMAL » est fausse.

### V38 — poser le modèle avant de demander du polylogarithmique

Aucune construction v5 ne satisfait aujourd'hui la demande. Sans borne de
prétraitement et d'espace, une table exhaustive rendrait artificiellement la
requête constante : la question doit imposer au moins un prétraitement et un
espace quasi linéaires. Le comptage
universel courant est un minorant certifié, mais son parcours n'est pas
polylogarithmique au pire cas ; les histogrammes sont ancre-spécifiques et leur
précalcul n'est pas polylogarithmique non plus. Surtout, des ancres différentes
peuvent être tuées par des ensembles de témoins différents : un certificat de
témoins communs peut rester lâche alors que chaque test ponctuel tue.

Ne pas qualifier cela de problème ouvert universel sans preuve bibliographique.
La voie falsifiable immédiate est le raffinement post-séparation q3/q4 déjà
décrit dans la question active : il resserre les boîtes, réutilise le certificat
existant et s'abandonne si le temps et les visites payées ne baissent pas.

### V39 — oui, dans le vrai flux produit

Instrumenter avant de concevoir le chemin `terrain` est la bonne priorité.
`3,14 - 1,41 = 1,73` est seulement la pente du quotient
`jung_cert_skip/anchors_q4` sur le dernier intervalle, pas celle du coût complet
par ancre. Réutiliser les compteurs existants et ajouter seulement les masses
de boucle non reconstructibles : handles/requêtes, W/secteurs, constructions et
visites de cover, grille, vrais tests de `x`, lentille, remplissage affine et
puissance q4. Joindre `Dmax`, `D2`, population des handles et taille de cover ;
les identités exactes à graver sont listées dans la question active.

### V40 — oui au tuilage, non à la confusion géométrique

Le prototype GPU courant n'affecte pas un rectangle à un bloc : q3 affecte un warp
par seed et q4 un bloc par seed vivant. L'asymétrie observée concerne donc
d'abord la formation hôte et tout futur ordonnanceur groupé par rectangle.

Un tuilage de scheduling est sûr s'il partage seulement l'itération de
`A x B`, tout en conservant le rectangle parent, son `core`, ses histogrammes,
la sémantique de chaque ancre et un merge déterministe. Il répartit le travail
mais n'en retire aucun. Un **sous-rectangle géométrique** qui recalcule boîtes,
certificats ou histogrammes est une autre optimisation : la route q2 reste
fermée, désormais sur une contre-fixture radix valide où un candidat et une
boule RLE se réveillent sans masse q2 tuée. Les routes q3/q4 sont conditionnées
par les portes de conservation de la question active.

### V41 — signal utile, descente non recevable telle quelle

Le probe `57deaaa6` montre qu'un comptage universel sur des sous-produits peut
certifier une masse non vacante de paires q3. Il ne prouve pas encore que la
transformation proposée conserve le chemin produit, et trois phrases de son
commentaire sont fausses :

- `separated` n'est pas héréditaire : en 1D avec `s=8`, le parent
  `A=[0,10], B=[50,60]` passe à égalité, tandis que l'enfant
  `A'=[5,10], B=[50,60]` échoue. Construire les deux enfants, vérifier d'abord
  leur séparation, puis seulement compter ; sinon réémettre le parent sans
  effet. En revanche, le **nombre sémantique** de témoins universels est
  monotone sur un sous-produit : tout témoin du parent reste valable et des
  points du frère deviennent éligibles. Ne pas lui attribuer une régression au
  seul motif que le centre bouge. Conserver néanmoins le minorant déjà prouvé
  par `child.core = max(parent.core, fresh_core)`, jamais leur somme. Avec
  `with_corners=true`, le comptage frais complet est lui-même attendu monotone ;
  graver `fresh_child >= parent.core`, y compris aux frontières, avec
  multiplicité et près de `h-1`. Le mutant `with_corners=false` distingue cette
  garantie du raccourci par seule boule-cœur, dont les boules ne sont pas
  imbriquées ;
- `kMaxDepth=40` et une profondeur observée 11 ne sont pas la profondeur bornée
  `L=0..3` proposée. Chaque `count_universal_witnesses` peut lui-même parcourir
  l'arbre du nuage : le coût n'est donc pas borné par deux fois le nombre de
  feuilles ou de paires ;
- « objet inchangé » exige l'exclusion de q2, le ledger borné des masses, le
  multiensemble trié des candidats et les signatures complètes. Scinder B change
  l'ordre brut ; ce n'est pas un défaut si la canonisation et toutes les sorties
  restent identiques.

Les `92,1 M` sites de cover et `20,6 M` seeds « évités » sont des proratas de
compteurs contrefactuels. Les comparer aux `27,3 M` nœuds d'arbre ne donne pas
un rapport de coût 3 pour 1 : les unités et prix par opération diffèrent, et le
vrai routage requête/cover n'est pas rejoué. De même, le signal « 6,2 % des
rectangles portent 73,1 % du travail » ne peut pas encore choisir une politique
adaptative du produit.

La ligne « 257 810 sous-rectangles engendrés, 1,49 par rectangle » compte aussi
les 173 190 racines et `core_evals` les recompte toutes. Le run contient donc
84 620 visites enfant, soit 42 310 scissions et 0,489 nouvel enfant visité par
rectangle racine, non 1,49 sous-rectangle engendré. Enfin, le rejeu local reproduit les
nombres mais le binaire imprime `pin_configure=0b3f3fd6`, faute de
reconfiguration, et aucune commande/sortie brute n'est versionnée : la mesure
n'est pas un reçu attribuable à `57deaaa6`.

Cette action minimale est désormais exécutée sur le chemin CPU de référence :
bras transactionnels `L=0/1/2/3`, rollback `refine-separated-not-hereditary`,
ledger, multiensemble littéral borné et mesure du mur intégré. Le résultat est
négatif pour le rescannage global inconditionnel : la masse retirée ne paie pas
les visites LBVH ajoutées. La piste encore ouverte est un certificat local par
frère ou un center-cover sélectif, pas une profondeur globale supérieure à 3.

Le nouveau compteur `k=1` est seulement une borne sur les ancres de la
population post-histogramme/post-W4 qui atteignent effectivement le test W. Il
ne borne ni toute la masse de paires que le raffinement peut certifier avant ces
filtres, ni un gain de temps ; son commentaire doit conserver cette restriction.


## Réponse auditée à V42 et deux corrections de réception

**Oui : en faire une porte de correction bornée dès le prototype.** Deux
niveaux complémentaires évitent de transformer l'oracle en coût produit :

1. Sur de petits arbres, développer littéralement les couples de positions
   uniques. La porte compare après tri canonique la réunion disjointe
   `couples_emis union couples_branches_certifiees_mortes` au multiensemble du
   front vivant de base. Elle exige au moins une scission de A, une de B, une
   branche morte, une survivante et un rollback pour enfant non séparé. Les
   mutants retirent un enfant, le dupliquent ou abaissent à $h-1$ le seuil de
   mort; le rollback est une fixture nominale distincte. La route q2 est fermée
   par construction et gardée par un mutant dédié : la contre-fixture radix
   à six points réveille exactement un candidat et une boule RLE quand cette
   route est ouverte, bien que son ledger de masse reste vert.
2. Sur le chemin de taille, ne matérialiser aucun couple : conserver seulement
   le ledger d'ancres uniques
   `emitted_pair_mass + postsep_killed_pair_mass = base_alive_pair_mass`. Si la
   masse pondérée par multiplicité de PointId est revendiquée, lui donner un
   second ledger explicitement nommé. Les digests de candidats, sorties,
   forêts, événements et niveaux restent des portes aval distinctes.

La porte intégrée du worktree réalise maintenant ces deux niveaux, refuse le
ledger avant publication, tue les mutants de perte/duplication/sur-mort,
d'ouverture q2 et de recomptage sans coins, puis compare le digest diagnostique
`digest_raw_candidates` pré-RLE, `digest_balls`, les cardinalités brutes par
lane, événements, `batch_levels` et forêts entre `L=0..3` et entre un et quatre
fils. Le digest brut est opt-in dans cette porte : le `--digest` historique ne
paie pas un second hachage du catalogue. La fermeture q2 exige aussi zéro
activité structurelle de raffinement et `parents == produits == rect_alive` ;
le mutant conserve sa preuve de réveil via le helper test-only puis est refusé
par le vrai pipeline. `u64` suffit ici : les positions sont indexées en `i32`,
donc la masse reste strictement sous $\binom{2^{31}}{2}$.

Cette porte est un CTest dédié du prototype, avec planchers de non-vacuité et
codes de sortie exacts. Le benchmark garde seulement les compteurs ; il ne
devient jamais l'oracle quadratique.

Deux formulations de la réception `4ecb57d4` restent à corriger avant la série
suivante :

- le binaire 50 k est `mhgp5`, pas `mhgp5_probe`. Surtout, imposer une valeur
  numérique de `coord` identique à toutes les tailles changerait la densité et
  donc le régime. Épingler le générateur et sa règle `coord(n)`, ou déclarer une
  construction emboîtée différente ; enregistrer chaque valeur effectivement
  utilisée ;
- la masse `k=1` ne borne que les ancres post-histogramme/post-W4 effectivement
  soumises au test W. Elle ne borne ni toute la masse de paires supprimable avant
  ces filtres, ni le temps. Les 69,8 M visites mesurées et 4,0 M covers estimés
  au prorata ne suffisaient donc pas à conclure. La mesure intégrée tranche
  maintenant contre le rescannage global : tous les bras `L>0` mesurés sont
  plus lents, malgré une masse q4 nettement réduite.

En q3, `33,7 % -> 43,8 %` décrit donc une **opportunité de pruning sur deux
tailles**, pas encore un gain croissant ni une baisse d'exposant. Cet ordre
historique est remplacé par R0--R3 ci-dessus : partager le diagnostic puis
traiter le carré local q4 avec le constructeur shallow.

## Contre-audit de `3e785622` / `e5ca5023` — garder le mécanisme, retirer le ratio

Les masses 8/16/32 k et leurs exposants sont arithmétiquement justes. Sur
`scanline` q4, la fraction survivante passe localement de 1,4389 % à 0,7913 %
puis 0,4698 %, et l'exposant de la masse survivante vaut 1,14 puis 1,25. C'est
un signal suffisamment bon pour continuer la sonde. Il prouve seulement que le
raffinement réduit la masse d'ancres explicites sur ces deux intervalles, pas
qu'il réduit le travail total ni qu'il « restaure » une complexité.

Le rapport annoncé `3,3:1` est faux par mélange de populations. À 32 k, la
base contient 6 951 708 paires et le raffinement en tue 4 546 210, alors que le
stage aval ne construit que 1 673 861 covers. L'intersection entre « paire
tuée » et « paire qui aurait construit un cover » n'est pas mesurée; elle peut
contenir entre zéro et 1 673 861 paires. Au plus 36,8 % des paires tuées
auraient donc atteint ce stage. Les 708 593 623 visites de points cumulées sur
tous les covers ne sont pas l'économie du raffinement. Même en lui attribuant
irréalistement toute cette masse, le quotient avec les 580 417 374 visites de
nœuds du raffinement ne dépasserait que 1,22, et ces deux unités n'ont pas le
même prix.

Le coût de la sonde croît lui-même vite : les visites de nœuds valent environ
43,98 M, 148,91 M et 580,42 M à 8/16/32 k, soit des pentes 1,76 puis 1,96. Une
sortie temporaire 100 k, non versionnée et donc seulement diagnostique, donne
48 557 755 paires avant, 11 044 864 après et 6 965 456 210 visites : la masse
survivante suit localement une pente 1,34 depuis 32 k, mais les visites 2,18 et
le coût par paire tuée monte d'environ 128 à 186 visites. Chaque comptage de
cœur peut traverser le LBVH; le pire cas ajouté reste donc jusqu'à cubique en
la taille du nuage. Une meilleure fraction survivante ne suffit pas si elle est
achetée par davantage de travail.

La sonde intégrée réutilise désormais `parent.core`, ferme le ledger
`avant = tuées + survivantes` et chronomètre le surcoût dans le flux réel. Elle
montre que les morts déplacées en amont ne compensent pas le rescannage global.
Une politique sélective devra encore apparier les branches avec leur dernier
étage réellement évité — `killed_pre_hist`,
`killed_prequery_W4_sector_cell`, `killed_cover_built` et somme exacte des
`point_visits` — puis conserver comme portes terminales le multiensemble pré-RLE,
les digests post-RLE, événements, niveaux et forêt.

L'intégration sûre conserve chaque rectangle WSPD parent et lui associe une
antichaîne de sous-produits survivants. Les enfants ne sont pas réinjectés
comme rectangles WSPD, car la séparation n'est pas héréditaire; ils servent
seulement à itérer les produits `A' x B'` encore vivants. Un masque consulté
depuis la double boucle originale conserverait l'énumération de toutes les
paires et rendrait le gain d'exposant fictif. La porte bornée doit vérifier que
chaque paire vivante apparaît exactement une fois, que les masses des enfants
partitionnent le parent, que q2 refuse cette route et que les caps échouent
ouverts. Ces portes sont maintenant présentes. Verdict constructif mis à jour :
**conserver le mécanisme comme instrument exact, garder `L=0` par défaut et ne
plus approfondir globalement**. La suite utile est le crédit local par frère,
puis le center-cover et l'arrangement shallow qui attaquent les propositions
d'ancres et le produit q4 `x` par `y`.

## Réponse auditée — V43 à V45

**V43 est recevable après réécriture, pas dans sa version `fd318929`.** La
preuve mélangeait la convention de séparation écrite avec celle de la WSPD de
Callahan--Kosaraju, omettait de déclarer les facteurs non vides et disjoints,
et la contre-famille initiale ne garantissait pas l'acuité q3. Le théorème
autonome corrigé est maintenant dans
[`RNG_JUNG_CLIQUES_ET_NIVEAUX_PEUPROFONDS.md`](../../docs/math/RNG_JUNG_CLIQUES_ET_NIVEAUX_PEUPROFONDS.md),
Théorème 4 : deux populations de taille $m$, l'une sur un cercle et l'autre
espacée sur son axe, forcent au moins $m(m-1)=\Omega(n^{2})$ blocs ternaires
symétriques, fortement séparés et exact-once, bien que tous les supports
croisés soient strictement aigus. Le seuil utile est $s>1$, sans argument de
spanner externe.

**V44 ferme seulement cette réalisation symétrique explicite de tous les
supports aigus comme remplacement linéaire de q3.** Elle ne ferme ni une WSSD
approximative, ni une source asymétrique ancre--tiers, ni une source restreinte
aux centres de profondeur au plus huit, ni l'arrangement shallow. C'est cette
portée étroite qui entre dans `PISTES_FERMEES.md`; le titre général « la WSPD ne
se généralise pas » est retiré.

**Le corollaire sur le circumrayon était faux pour la lane q3.** Pour un
triangle aigu ancré par sa plus longue arête $D$, le centre vérifie
$R_c^{2}\leq D^{2}/3$ et $\lVert c-m_D\rVert^{2}\leq D^{2}/12$. Le cas proche
de 89°--89°--2° tend vers un centre au milieu de l'arête maximale; il montre
une disparité de côtés, pas une délocalisation. Le troisième sommet `x` est le
carrier proposé dans le cover; ce sont les autres sites qui servent de témoins
de profondeur.

**V45 devient une fixture locale, jamais une preuve asymptotique.** La porte
`mhgp5_q3_skinny_center` vérifie la localisation précédente et une instance
u16 cercle--axe de 12 points par population : ses 792 supports croisés sont
tous aigus et satisfont les inégalités locales à $s=8$. Si un producteur
ternaire symétrique apparaît, une porte séparée devra vérifier l'exact-once sur
ces 792 supports et le plancher de 132 blocs. Le domaine u16 fini, à lui seul,
ne peut pas prouver une borne asymptotique.

## Requalification de la mesure prédicteur `905c5361`

La ventilation `k=1/k=2` est utile, mais le coefficient `70,6 %` n'est pas
identifié par les totaux publiés. Notons, dans les rectangles vivants de base :

- `E` les paires éliminées avant le test W ponctuel — histogramme en q3,
  histogramme puis W4 explicite en q4 ;
- `P` la population restante qui atteint ce test, et `W1` les paires de `P`
  tuées par `k=1` ;
- `K` les paires certifiées mortes par la descente post-séparation.

La relation sûre est `K subset E union W1`. Elle ne donne ni `E subset K`, ni
une partition de `K` sans mesurer l'intersection. Pour `scanline q3` 16 k, les
valeurs annoncées `|K|=696 537`, `|E|=310 615` et `|W1|=546 779` impliquent
seulement :

```text
385922 <= |K inter P| <= 546779
70,581 % <= |K inter P| / |W1| <= 100 %
149758 <= |K inter E| <= 310615
```

Le `70,6 %` publié est donc la **borne inférieure** obtenue en supposant à tort
que toutes les morts histogramme appartiennent à `K`. Pour l'identifier, la
sonde doit imprimer la partition exacte
`K = K_pretests + K_k1`, avec intersections calculées paire par paire et les
conservations correspondantes. Sur uniform q3, `141 246 / 564 834 = 25,007 %`
n'est également qu'une borne supérieure avant retrait de `K inter E`, pas un
taux récupéré « inférieur à 25 % » déjà mesuré.

Le zéro `k=1` q4 signifie `K inter P = emptyset`, pas `K = emptyset`. Il est
donc compatible avec les 5,9 à 40,4 % de paires q4 supprimées par la sonde :
ces paires doivent appartenir aux prétests `E` et peuvent encore éviter des
coûts amont. Elles ne prouvent en revanche aucune économie du corps q4 aval.

Enfin, les rapports 0,06:1 à 6,3:1 divisent des covers estimés au prorata par
des visites d'arbre mesurées. Ils ne classent ni gain ni perte de temps. Les
phrases « paie sur un seul cas », « perd partout ailleurs » et « gain croissant »
restent donc rejetées jusqu'au prototype intégré. La note autonome de
`905c5361`, sans commande, sortie brute, hash de binaire ni pin de configuration
opposable, est consolidée ici puis retirée pour garder `audits/` propre.
