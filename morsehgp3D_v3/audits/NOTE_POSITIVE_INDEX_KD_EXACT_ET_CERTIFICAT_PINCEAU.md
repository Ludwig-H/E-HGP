# Note positive — index k-d exact et certificat de pinceau

Date : 9 août 2026 UTC.

> [!IMPORTANT]
> Cette note fournit deux briques constructives : un prédicat exact boîte--boule pour un k-d tree sur la grille u16, puis un lemme montrant que le désaccord ternaire des signes aux deux sphères terminales contient tous les événements non constants d'un pinceau. Elles remplacent la marge flottante réfutée sans revenir à une grille dont le coût dépend du volume d'une boule vide.

Cadre : `backend=reference_cpu_local`, `profile=quantized_u16_order_k_prototype`, `mode=exploration/diagnostic_only`, `public_status=not_claimed`.

## 1. Prédicat exact entre une boîte k-d et une boule

Une sphère du dépôt est représentée par `Sphere{base, nx, ny, nz, den}` avec `den>0` :

$$c_j=\mathrm{base}_j+\frac{n_j}{d},\qquad r^2=\frac{N}{d^2},\qquad N=n_x^2+n_y^2+n_z^2.$$

Considérons la boîte entière fermée d'un nœud k-d,

$$Q=[\ell_x,h_x]\times[\ell_y,h_y]\times[\ell_z,h_z].$$

Pour chaque axe, posons le numérateur du centre $C_j=\mathrm{base}_j d+n_j$, puis la distance rationnelle du centre à l'intervalle :

$$g_j=\max\left\lbrace \ell_jd-C_j,\ 0,\ C_j-h_jd\right\rbrace.$$

Alors :

$$d^2\,\mathrm{dist}(c,Q)^2=g_x^2+g_y^2+g_z^2.$$

### Théorème 1 — séparation exacte de la boîte

La boîte $Q$ est disjointe de la boule fermée si et seulement si

$$g_x^2+g_y^2+g_z^2>N.$$

Cette condition suffit donc à élaguer le nœud. La réciproque ne vaut pas pour
son ensemble **discret** de points : sa boîte peut couper la boule sans qu'aucun
point stocké n'y appartienne. En cas d'égalité, le nœud doit être conservé : la
boule est fermée et les points de coquille sont contractuels.

**Preuve.** Sur un axe, $g_j/d$ est exactement la distance de $c_j$ à l'intervalle $[\ell_j,h_j]$. La distance euclidienne minimale à un produit d'intervalles est la somme des trois carrés de ces distances. Comparer cette quantité à $r^2=N/d^2$, puis multiplier par $d^2>0$, donne l'équivalence. Aucun arrondi ni argument de séparation entre points entiers n'intervient. $\square$

Pour classifier aussi un nœud certainement **strictement intérieur**, posons

$$f_j=\max\left\lbrace \left\lvert \ell_jd-C_j\right\rvert,\ \left\lvert h_jd-C_j\right\rvert\right\rbrace.$$

La distance maximale du centre à la boîte vérifie alors

$$d^2\,\max_{x\in Q}\left\Vert x-c\right\Vert^2=f_x^2+f_y^2+f_z^2.$$

Le nœud est certainement strictement intérieur à la boule si et seulement si

$$f_x^2+f_y^2+f_z^2<N.$$

Les inégalités sont volontairement strictes. Une égalité peut contenir un point
de coquille : elle reste indécise et force la descente. Pour deux sphères, un
nœud peut donc être élagué de la requête de désaccord de signes seulement s'il
est strictement intérieur aux deux boules, ou strictement extérieur aux deux.
Tous les autres cas descendent jusqu'au test exact par point.

### 1.1 Largeurs suffisantes sur le profil u16

Les bornes déjà publiées par `sphere.hpp` donnent, dans le pire cas d'arité trois, `den` sous environ $2^{73}$ et une composante de numérateur sous environ $2^{90}$. Avec une coordonnée de boîte sous $2^{16}$, $C_j$, $g_j$ et $f_j$ restent sous $2^{91}$. Chaque carré reste sous $2^{182}$ et leur somme sous $2^{184}$.

Le chemin suivant est donc suffisant :

- calculer $C_j$, $g_j$ et $f_j$ en `i128`;
- calculer chaque carré par `mul128`;
- sommer dans `BigInt<4>`;
- comparer à `sphere_num2(sphere)` avec `big_cmp`.

Les opérandes sont non négatifs et très en dessous du bit de signe de
`BigInt<4>`. Ces largeurs exigent une sphère produite par `sphere1` à `sphere4`
depuis une entrée u16 gardée. Un `den<=0` ou une provenance non authentifiée est
une erreur d'invariant : descendre puis appeler `sphere_side` ne rendrait pas une
sphère invalide sûre. Le repli intégral n'est autorisé que si la sphère et le
prédicat terminal exact sont valides, mais que l'index seul choisit de ne pas
élaguer.

### 1.2 Contrat de propriété

Le k-d tree doit être construit derrière l'entrée gardée qui l'utilise, sur un
stockage immuable du même nuage. Passer un pointeur public vers un index construit
sur un autre vecteur, ou vers un vecteur modifié depuis sa construction, rend les
boîtes périmées. Un digest ordinaire n'est ni sans collision, ni une protection
contre une mutation après sa vérification.

La forme sûre du prototype est de rendre l'index strictement interne à
`flat_catalogue` et à son appel privé de navigation, propriétaire d'une vue
immuable dont aucun autre nuage n'est passé séparément. Une API publique future
devra posséder le stockage immuable et sa génération; elle ne peut pas se
contenter d'un pointeur emprunté et d'un contrôle initial.

### 1.3 Témoin minimal qui doit rester rouge sans ce prédicat

Le filtre flottant élargi de 0,5 est réfuté sur quatre points u16 distincts :

```text
a=(32767,32767,0)
b=(57863,57862,0)
c=(7672,7673,0)
d=(60104,30135,1)
```

Le déterminant affine vaut 1, `den=2` et le numérateur du centre relatif vaut
`(-63213951411449,63216470447551,1894465540706974539)`. La sphère exacte
vérifie `sphere_side==0` pour les quatre points. Sur le snapshot de header
`1117793f843f3cdca4da36beebfdeac11a51a2d8a7a813ac9a40ec642cac499b`,
`CertifiedIndex.build(cloud,16)` suivi de `closed_ball` élague la racine : zéro
point touché et aucun identifiant rendu. Le test exact donne
`g_x^2+g_y^2+g_z^2-N=-20268947903996245952`, conserve donc la boîte, puis le
filtre terminal exact rend les quatre supports.

Cette fixture est plus forte qu'un simple test de nœud interne : elle prouve que
l'enveloppe flottante peut censurer la racine elle-même. Elle doit être gravée
à la fois contre le prédicat local et contre le catalogue indexé complet.

## 2. Lemme de désaccord des signes terminaux

Fixons un cercle porté par un flat de rang trois. Les centres des sphères du pinceau s'écrivent $c(t)=c_0+tu$ le long de la normale au plan du cercle. Après choix d'une origine de paramètre, leur rayon vérifie $r(t)^2=r_0^2+t^2\lVert u\rVert^2$.

Pour tout point $x$, sa puissance par rapport à la sphère du pinceau est donc affine en $t$ :

$$\pi_x(t)=\lVert x-c(t)\rVert^2-r(t)^2=\pi_x(0)-2t\,u\mathbin{\cdot}(x-c_0).$$

### Théorème 2 — couverture par les extrémités

Pour $t=(1-\lambda)t_0+\lambda t_1$ avec $0\le\lambda\le1$,

$$B(t)\subseteq B(t_0)\cup B(t_1).$$

**Preuve.** L'affinité donne $\pi_x(t)=(1-\lambda)\pi_x(t_0)+\lambda\pi_x(t_1)$. Si $x\in B(t)$, alors $\pi_x(t)\le0$. Les deux valeurs terminales ne peuvent donc pas être strictement positives; au moins l'une est négative ou nulle, et $x$ appartient à l'une des deux boules terminales. $\square$

### Corollaire 2.1 — les événements non constants changent de signe terminal

Orientons la direction de sorte que $t_0<t_1$. Si $\pi_x$ n'est pas constante
et s'annule pour un paramètre strictement entre $t_0$ et $t_1$, ses valeurs aux
deux extrémités ont des signes strictement opposés. Si l'annulation a lieu à une
extrémité, une valeur est nulle et l'autre est non nulle. Dans les deux cas,
`sphere_side` diffère entre les deux sphères.

Le seul cas où un point est sur les deux sphères distinctes du pinceau est
$\pi_x\equiv0$ : il appartient au cercle fixe du flat et figure déjà dans sa
fermeture. Il ne doit pas être redécouvert comme événement. Ainsi, pour les
candidats hors fermeture, la requête exacte utile est

$$D(t_0,t_1)=\left\lbrace x:\mathrm{sgn}\,\pi_x(t_0)\ne\mathrm{sgn}\,\pi_x(t_1)\right\rbrace.$$

Ce n'est **pas** la différence symétrique des boules fermées. Si les signes
valent zéro et négatif, le point appartient aux deux boules, donc pas à leur
différence symétrique, mais il appartient bien à $D$ et doit être rendu. Le test
live `sphere_side(a,p) != sphere_side(b,p)` calcule $D$; son nom
`symmetric_difference` doit être corrigé, pas son verdict terminal.

Le théorème 2 implique aussi $D(t_0,t_1)\subseteq B(t_0)\cup B(t_1)$, mais $D$
est la requête plus précise.

### 2.2 Conséquence algorithmique

Pour une direction fixée du pinceau :

1. une amorce spatiale trouve **n'importe quel** candidat terminal $t_1$ strictement après $t_0$ dans la direction orientée; si elle couvre tout le nuage sans candidat, la direction est non bornée;
2. l'index exact énumère $D(t_0,t_1)$;
3. `compare_t` choisit dans cet ensemble le premier événement $t_\star$;
4. tous les membres non constants du lot à $t_\star$ ont eux aussi des signes terminaux différents; les membres constants sont déjà dans la fermeture du flat.

La requête brute $D$ contient aussi les membres de la coquille courante
$S(v)\setminus C(F)$, dont la racine est $t_0$ mais qui ne sont pas de nouveaux
événements. L'appelant doit donc exclure **toute** la coquille courante, pas
seulement la fermeture constante $C(F)$. Leur contribution au transport reste
calculée séparément.

Une seule requête de désaccord ternaire suffit. Une boucle arbitrairement bornée à quatre mises à jour n'est pas nécessaire. Les égalités de paramètre doivent être regroupées avant le transport de niveau.

Ce résultat tolère les lots cosphériques : il emploie seulement l'affinité de la puissance et les comparaisons exactes du pinceau. Il ne résout pas la propriété globale, la reverse search ou les forêts.

## 3. Porte de validation proposée

Le chemin indexé doit qualifier séparément la boîte racine et les nœuds internes. La porte minimale est :

1. **auto-suffisance** : un TU qui inclut seulement le header compile avec `-Wall -Wextra -Werror`;
2. **prédicat local** : comparer les classifications exactes « strictement dedans », « strictement dehors » et « indécis » à une énumération de toutes les coordonnées d'une petite boîte;
3. **grande sphère** : graver une sphère u16 à déterminant petit et centre très éloigné; chaque point de support doit atteindre sa feuille;
4. **nœuds internes** : au moins 17 points pour `leaf_size=16`, avec planchers strictement positifs de nœuds visités et élagués;
5. **catalogue** : égalité index/référence sur statut, records, supports, membres, centre rationnel, rayon, `beta` et ordre;
6. **lots** : fixtures cosphériques et coplanaires, avec lot terminal complet;
7. **propriété** : aucun appel public ne peut fournir un index étranger ou périmé;
8. **coût total** : publier séparément nœuds visités, nœuds élagués dedans/dehors, feuilles, points exacts, amorces, requêtes de désaccord, replis intégraux et census de contrôle.

Le test `verify_census=true` garde volontairement un rescan global par sommet. Il qualifie l'exactitude, pas la performance. Toute mesure de coût indexé doit soit le comptabiliser, soit exécuter séparément un run de mesure sans census après que la porte exacte est verte.

## 4. Ce que cette brique apporte au contrat 50 k

Le k-d tree exact remplace deux rescans locaux potentiellement en $O(n)$ par des requêtes dont le pire cas reste $O(n)$ mais dont le travail observé est explicitement mesurable. Il évite surtout le coût volumique d'une grille sur une grande boule vide.

Il ne supprime ni `seen`, ni `frontier`, ni `visited`, ni l'énumération des flats et triplets, ni le tri global, ni les forêts. C'est une brique positive d'indexation, pas encore l'architecture 50 k.

GCP non utilisé.
