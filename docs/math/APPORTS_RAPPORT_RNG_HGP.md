# Ce que le rapport RNG-HGP apporte à MorseHGP3D exact

Lecture critique d'un rapport externe proposant de généraliser le graphe de
voisinage relatif au graphe pondéré des facettes $\Gamma_K$. On sépare ici ce
qui est **utilisable tout de suite**, ce qui **corrige une conclusion du dépôt**,
et ce qui **ne répond pas au problème mesuré**.

---

## 1. Ce qui corrige une conclusion du dépôt

`PLAN_DE_ROUTE_CONTRATS_50K.md` §I2 concluait : « la seule route exacte connue
est la construction d'ordre $k$ ». **C'est trop fort, et le rapport le montre
par un lemme qui rend la construction globale inutile.**

**Lemme d'énumération par supports.** Soit $S \subset X$ affinement indépendant
avec $2 \le |S| \le p+1$, qui est exactement le support de sa propre miniball
$B_S = B(c_S, r_S)$. Alors
$$|X \cap B_S| = K+1 \iff \sigma_S = X \cap B_S \text{ est un } K\text{-simplexe de Gabriel de support } S.$$

*Sens direct* : si $\sigma$ est de Gabriel, sa miniball ne contient aucun point
extérieur en son intérieur, ni — en position générale — sur sa frontière ; elle
contient donc exactement les $K+1$ sommets. *Réciproque* : si $B_S$ contient
exactement $K+1$ points et $\sigma = X \cap B_S$, alors toute boule contenant
$\sigma$ contient $S$, donc a un rayon au moins $r_S$ ; d'où $B_\sigma = B_S$ et
aucun point extérieur à l'intérieur.

**Conséquence en dimension 3** : il suffit d'énumérer des supports de taille
**2, 3, 4 — indépendamment de $K$**. Pour $K=10$ on n'énumère jamais des
11-uplets : on forme une boule candidate à partir de 2, 3 ou 4 points, on compte
par requête spatiale, et on retient si le compte vaut exactement $K+1$.

**Or c'est exactement l'architecture du dépôt.** `higher_support_stream`
énumère les supports de taille 3 et 4 et les classe par rang fermé via
`ExactHigherSupportIndexedClosedBallQuery`. Le lemme **valide la conception
existante** et retire l'obligation de matérialiser une mosaïque d'ordre $k$.

**Ce qu'il ne fait pas** : fournir la liste des candidats. Le rapport le
reconnaît — « un critère plus élégant ne fabrique pas tout seul sa liste de
candidats ». L'univers des supports de taille $\le 4$ vaut toujours
$2{,}6\cdot10^{17}$ à $n=50\,000$, ce qui est précisément le mur mesuré. Le
verrou reste donc entier ; il change seulement de nom : ce n'est plus « quelle
structure construire » mais « comment restreindre les centres candidats ».

## 2. Ce qui explique une réfutation déjà mesurée

Le rapport met en garde (§6) contre le critère tentant
$$\bigcap_{x \in \sigma} B(x, 2\rho(\sigma)) \cap (X \setminus \sigma) = \varnothing.$$
Pour $K=1$ il redonne la lune du RNG ; pour $K \ge 2$ **il est faux pour Čech**.
La présence d'un point $z$ n'y garantit qu'un diamètre inférieur à $2r$ — donc
l'appartenance à Vietoris–Rips au niveau $r$, pas à Čech : le rayon de la
miniball des remplacements peut rester supérieur à $r$, l'encadrement
Čech–Rips ne donnant qu'un facteur de Jung $\alpha_p > 1$.

**C'est l'explication de deux réfutations que le dépôt avait mesurées sans les
expliquer.** La restriction certifiée $D \le 2R(p)$ de la germination locale et
le préfiltre par rang d'arête sont tous deux des critères de **niveau Rips**
appliqués à un problème **de niveau Čech**. Leur inefficacité — 0 % de paires
retirées sur nuage aggloméré, queue de rang à 38 — n'était pas un défaut de
réglage : c'est le facteur de Jung qui les sépare de ce qu'il faudrait tester.

Le rapport corrobore aussi qu'aucun $m$ fixe de $m$-NN ne garantit la
complétude : une arête peut avoir un rang de voisinage arbitrairement élevé.
Cela ferme définitivement la famille des préfiltres par voisinage.

## 3. Ce qui est directement utilisable sur l'aval

**Borne du support : $|S_\sigma| \le p+1 = 4$.** Quel que soit $K$, la miniball
d'un simplexe n'est déterminée que par au plus $p+1$ de ses points, donc les
**facettes actives** sont au plus 4, et les arêtes de fusion au plus 3. À
$K=10$, cela remplace $\binom{11}{2} = 55$ arêtes potentielles par au plus 3.

**Le dépôt exploite déjà cette borne, et la mesure le confirme** : la fermeture
de descente de facette visite un nombre **constant** de nœuds par événement —
9,92 / 7,89 / 8,17 / 8,81 / 9,29 de $n=12$ à $n=28$ — ce qui est de l'ordre de
$p+1$ et non de $K+1$. C'est une confirmation indépendante que l'aval est
structurellement correct, et que son coût mesuré vient bien de la **provenance**
et non de la combinatoire.

## 4. Ce qui ne répond pas au problème du contrat

La proposition centrale du rapport — remplacer Gabriel par un
$\operatorname{RNG}^{\mathrm{HGP}}_K$ défini par la connexité d'un graphe de
contournement $\mathcal{B}_\sigma$ sur les facettes actives — est
mathématiquement séduisante, avec une chaîne
$\operatorname{Sep}_K \subseteq \operatorname{RNG}^{\mathrm{HGP}}_K
\subsetneq \operatorname{Gab}_K$ dont l'inclusion stricte est établie par un
contre-exemple explicite en dimension 2.

Trois raisons de ne pas l'engager pour le contrat 50 000 points :

1. **Elle s'applique après l'énumération, pas avant.** C'est un filtre sur des
   candidats déjà connus ; le mur mesuré est la production des candidats.
2. **Elle coûte davantage.** Chaque simplexe retenu demande jusqu'à
   $\binom{4}{2}=6$ paires de facettes actives, chacune avec une recherche de
   témoins puis deux tests exacts de miniball. Le budget mesuré est de 2 667 ns
   par record sur 48 cœurs à $K=10$ ; six recherches de voisinage n'y tiennent
   pas.
3. **Elle casse les masses du chapitre 9.** Le rapport le signale lui-même :
   l'équivalence porte sur $\pi_0$, pas sur une fonctionnelle décorant le
   complexe. Supprimer un simplexe de Gabriel redondant avant d'accumuler sa
   contribution modifie $S_\tau = \sum_{\sigma \supset \tau} \psi(\rho(\sigma))$,
   donc la condensation et le vote pondéré.

Elle reste un objet à considérer **si l'on veut une représentation plus mince à
hiérarchie identique**, une fois le contrat tenu — pas pour le tenir.

## 5. Ce qu'on retient, en une ligne chacun

- Le lemme d'énumération par supports **valide l'architecture existante** et
  retire l'obligation de la mosaïque d'ordre $k$ ; il ne résout pas
  l'énumération.
- Le critère $2r$ est de niveau **Rips**, pas Čech : cela **explique** les deux
  réfutations mesurées et ferme la famille des préfiltres par voisinage.
- La borne $|S_\sigma| \le p+1$ est **déjà exploitée** par l'aval, et la mesure
  de 9 nœuds constants par événement le confirme.
- Le RNG-HGP est plus mince mais **plus cher et non conservatif** pour les
  masses : hors contrat.
- Le verrou reste **la restriction des centres candidats**, et il est désormais
  posé dans les bons termes : trouver les $c$ dont la boule des $K+1$ plus
  proches voisins a au moins trois points sur sa frontière, sans énumérer
  $\binom{n}{3}+\binom{n}{4}$.
