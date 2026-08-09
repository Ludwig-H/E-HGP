# Gate D — premières incidences du cœur sans étoile globale

Date : 9 août 2026 UTC.

Cadre : `backend=architecture_math`, `profile=hgp_reduced_quantized_u16`,
`mode=core_first_incidence_factorization`, `public_status=not_claimed`.

> [!IMPORTANT]
> Le théorème horizontal n'est plus le verrou : sous l'autorité régulière et la
> fenêtre de rang déjà définies, les cofaces directes et toutes les premières
> incidences des facettes du cœur suffisent à la forêt $H_0$ normalisée. Le
> résiduel v3 est de produire ces objets sans reconstruire l'étoile. Cette note
> extrait une dichotomie algorithmique plus générale : une requête de boule
> fermée par facette, plus un regroupement externe d'une source directe complète,
> suffit à reconstruire $M(F)$. La régularité n'est requise qu'ensuite pour la
> rétraction $H_0$; le branch-and-bound général reste le juge et le repli sans
> source directe terminale. Le contrat public v2 n'est pas modifié.

La base mathématique est le corollaire 4.1 de
[`INCIDENCES_SILENCIEUSES_GAMMA.md`](../../docs/math/INCIDENCES_SILENCIEUSES_GAMMA.md).
L'oracle général exact d'une facette est déjà démontré dans
[`PREMIERE_INCIDENCE_SPARSE_PHASE10.md`](../../docs/math/PREMIERE_INCIDENCE_SPARSE_PHASE10.md).
La présente note ne réimplémente aucun des deux : elle montre comment une source
directe terminale, avec une politique exacte des égalités extérieures, permet
d'éviter l'oracle général sur les facettes du cœur.

## 1. Objet exact

Fixons $2\leq k<n$. Notons $\mathcal{G}_k$ la famille complète des cofaces
directes de cardinal $k+1$, et $D_k$ l'ensemble de leurs facettes. Pour
$F\in D_k$, soit $B_F$ l'unique miniboule fermée de $F$ et soit
$b_F=\beta(F)$. Posons

$$E_F=\bigl(B_F\cap X\bigr)\setminus F.$$

Le premier niveau d'incidence et ses co-minimiseurs sont

$$\lambda(F)=\min_{x\in X\setminus F}\beta(F\cup\lbrace x\rbrace),\qquad M(F)=\left\lbrace F\cup\lbrace x\rbrace:\beta(F\cup\lbrace x\rbrace)=\lambda(F)\right\rbrace.$$

Le but n'est pas de matérialiser $\mathrm{Star}(D_k)$, ni toutes les facettes de
cardinal $k$, mais seulement de publier exactement $D_k$ et tous les $M(F)$.

## 2. Branche fermée : résultat sans hypothèse de régularité

**Lemme de la boule fermée.** Si $E_F\neq\varnothing$, alors

$$\lambda(F)=b_F,\qquad M(F)=\left\lbrace F\cup\lbrace x\rbrace:x\in E_F\right\rbrace.$$

**Preuve.** Pour $x\in E_F$, la boule $B_F$ contient déjà $F\cup\lbrace x\rbrace$;
la monotonie donne donc $\beta(F\cup\lbrace x\rbrace)=b_F$. Aucun ajout ne peut
avoir un niveau inférieur à $b_F$. Réciproquement, si une coface possède encore
le niveau $b_F$, sa miniboule est aussi une miniboule de $F$. L'unicité
euclidienne impose qu'elle soit $B_F$, donc $x\in E_F$. $\square$

Ce lemme accepte un intrus strict comme une égalité extérieure. Il exige un
`closed_ball` complet : trouver un seul témoin ne suffit pas, car tous les
co-minimiseurs du niveau exact appartiennent au même lot atomique.

Dans la porte régulière du corollaire 4.1, une égalité extérieure pertinente est
interdite. La branche non vide est alors exactement la famille des intrus
stricts $J_F$. Hors de cette porte, la source de $M(F)$ reste correcte, mais la
rétraction horizontale peut devoir refuser la dégénérescence.

## 3. Branche vide : le minimum direct suffit sans support unique

Pour chaque facette du cœur, définissons la famille incidente directe

$$\mathcal{G}_k(F)=\left\lbrace Q\in\mathcal{G}_k:F\subset Q\right\rbrace.$$

Elle n'est pas vide puisque $F\in D_k$. Posons

$$m_{\mathrm{dir}}(F)=\min_{Q\in\mathcal{G}_k(F)}\beta(Q).$$

**Théorème de première incidence depuis la source directe.** Supposons que
$\mathcal{G}_k$ contienne toutes les cofaces de Gabriel au sens de la vacuité
intérieure, avec les égalités extérieures soit développées complètement, soit
refusées explicitement. Si $E_F=\varnothing$, alors

$$\lambda(F)=m_{\mathrm{dir}}(F),\qquad M(F)=\left\lbrace Q\in\mathcal{G}_k(F):\beta(Q)=m_{\mathrm{dir}}(F)\right\rbrace.$$

**Preuve.** Le cas $E_F=\varnothing$ donne d'abord
$\lambda(F)>\beta(F)$. Soit $Q=F\cup\lbrace x\rbrace$ un minimiseur de niveau
$a$. Supposons qu'un point $z\in X\setminus Q$ soit strictement intérieur à sa
miniboule $B_Q$. Comme $a>\beta(F)$, $B_Q$ n'est pas une miniboule de $F$. Si
$B_Q$ était encore la miniboule de $F\cup\lbrace z\rbrace$, son centre
appartiendrait à l'enveloppe convexe d'un support positif pris parmi les points
de $(F\cup\lbrace z\rbrace)\cap\partial B_Q$. Le point $z$ étant strict, ce
support serait contenu dans $F$ et certifierait aussi $B_Q$ comme miniboule de
$F$, contradiction. On a donc
$\beta(F\cup\lbrace z\rbrace)<a$, contrairement à la minimalité de $Q$. Tout
minimiseur est ainsi Gabriel au sens ouvert, sans hypothèse de support unique.
La complétude de $\mathcal{G}_k$ donne exactement le minimum et tous ses ex
æquo. $\square$

Ce résultat est plus fort, algorithmiquement, que « appeler l'oracle général sur
toutes les facettes du cœur » : les candidats extérieurs ont déjà été certifiés
par la source directe terminale. Il ne faut plus reconstruire jusqu'à 176
supports pour chacun d'eux. Un support multiple ne casse pas le théorème de
$M(F)$; il peut en revanche invalider la rétraction $H_0$ et reste sous
l'autorité séparée de régularité ou de fenêtre.

## 4. Pipeline streamé exact

La composition minimale est la suivante.

1. La source directe terminale émet chaque $Q\in\mathcal{G}_k$ une fois, avec
   son niveau rationnel exact et son identité canonique.
2. Pour chaque $Q$, elle écrit ses $k+1$ suppressions sous la forme
   `(facet_key, beta, direct_coface_id)`; l'identité de masse est
   $R_k=(k+1)\lvert\mathcal{G}_k\rvert$.
3. Un tri externe par `(facet_key, beta, direct_coface_id)` déduplique les
   facettes, construit $D_k$ et conserve, pour chaque $F$, tous les records au
   plus petit niveau direct.
4. Une miniboule exacte et une requête `closed_ball` certifiée construisent
   $E_F$ pour chaque clef distincte.
5. Si $E_F\neq\varnothing$, la source publie exactement les cofaces
   $F\cup\lbrace x\rbrace$, $x\in E_F$. Si $E_F=\varnothing$, elle publie le
   groupe direct minimal déjà trié, sous l'autorité terminale de
   $\mathcal{G}_k$ et sa politique explicite des extra-shells.
6. Les cofaces identiques proposées par plusieurs facettes sont dédupliquées
   extérieurement par leur clef complète, sans table résidente `emitted`.
7. Le flux obtenu est trié par `(ordre, beta_exact, coface_id)` avant le quotient
   horizontal atomique.

Le tri des suppressions n'est pas une mosaïque de Delaunay d'ordre supérieur.
Il porte exactement sur la sortie directe et ses $k+1$ facettes, puis sur les
premières incidences effectivement retenues. Les clefs complètes peuvent être
jetées après leur réduction en identifiants canoniques et digests.

## 5. Certificat de complétude

Une capability v3 défendable doit lier les autorités suivantes au même nuage
canonique et au même index immuable.

- **terminalité directe** : aucune coface directe pertinente ne reste en attente;
- **masse des suppressions** : exactement $k+1$ records par coface directe;
- **groupement complet** : tous les records d'une même `facet_key` et tous les
  ex æquo du minimum direct sont réunis avant décision;
- **miniboule rejouée** : centre, rayon et support de chaque $F$ sont reconstruits
  depuis sa clef, jamais fournis comme autorité par le résultat observé;
- **requête fermée complète** : chaque feuille de $X$ est visitée ou élaguée par
  une classification boîte--boule entière, avec égalité descendue;
- **partition de branche** : `closed_nonempty` ou `closed_empty`, jamais une
  absence issue d'un budget;
- **sémantique des égalités extérieures** : la branche
  `closed_empty -> direct_minimum` exige une source de Gabriel ouverte complète;
  une source limitée à `shell == support` doit développer les extra-shells ou
  échouer fermée;
- **porte régulière ou fenêtre** : elle est indépendante de la justesse de
  $M(F)$ et autorise seulement sa réduction vers la forêt $H_0$ normalisée;
- **déduplication terminale** : chaque coface logique possède une seule identité
  dans le flux final, avec conservation de toutes ses provenances de facettes;
- **watermark** : aucun lot horizontal n'est committé avant la fermeture de la
  source et de tous les ex æquo de son niveau.

Un plafond atteint rend toute la facette `unresolved` et efface son niveau et ses
co-minimiseurs. Un résultat partiel n'est ni un minimum, ni une preuve
d'isolation.

## 6. Repli général sans source directe terminale

Quand la source directe n'est pas terminale, que sa convention de Gabriel ne
couvre pas les extra-shells ou qu'un juge indépendant est requis, le raccourci
direct est interdit. L'oracle général 10.6 reste exact sans aucune prémisse sur
$\mathcal{G}_k$. Pour un nœud d'AABB $N$, sa borne combine la borne radiale de la
miniboule source et les bornes diamétrales de tous les points de $F$ :

$$L_F(N)=\max\left(R_{b_F}(\delta_{c_F}(N)),\max_{f\in F}\frac{\delta_f(N)}{4}\right).$$

Il ne prune que si $L_F(N)$ est strictement supérieur à l'incumbent; l'égalité
descend afin de conserver tous les co-minimiseurs. Les feuilles évaluent la
miniboule exacte de $F\cup\lbrace x\rbrace$. Ce chemin peut visiter
$\Theta(n)$ éléments et produire $\Theta(n)$ co-minimiseurs; il certifie la
complétude, pas une borne sous-linéaire ni le SLO.

## 7. Ce qui reste global après cette factorisation

| objet | nature exacte | état résident global nécessaire ? |
| --- | --- | --- |
| nuage et index | lecture scientifique globale | $O(n)$ ou stockage adressable |
| fin de la source directe | watermark global | non, compteur et digest suffisent |
| regroupement par facette | tri et réduction globaux | non, runs externes |
| requête $E_F$ | décision locale avec lecture de $X$ | non au-delà de l'index |
| groupe de même $\beta$ | barrière sémantique globale | non, fusion externe possible |
| partition horizontale | état global logique | oui logiquement, pas nécessairement en RAM |
| couverture et verticales | historique et jointures globales | non, journaux et sweeps externes |

La **source silencieuse** cesse donc d'être une inconnue mathématique dès que la
source directe et sa convention de frontière sont terminales. Ce qui reste
ouvert pour v3 est cette production terminale, l'authentification séparée de
l'autorité régulière ou fenêtrée, l'intégration au reducer, puis les verticales
et l'identité de sortie. Le propriétaire local du parcours ne supprime aucune
de ces barrières, mais il permet de les alimenter sans catalogue de sommets
résident.

## 8. Portes de falsification

Avant de remplacer l'oracle général, le différentiel
doit graver au minimum :

- la fixture `AC` à deux intrus de niveau $33/2$, avec les deux co-minimiseurs;
- un point extérieur exactement sur le shell, qui reste dans $E_F$ mais fait
  échouer la capability régulière pertinente;
- $E_F=\varnothing$ avec deux cofaces directes minimales ex æquo;
- une coface candidate extérieure avec un intrus strict, dont le remplacement
  fournit une incidence strictement moins chère;
- une facette produite par plusieurs cofaces directes et une coface silencieuse
  proposée par plusieurs facettes, pour juger les deux déduplications;
- une permutation complète des runs et des records d'un même niveau;
- un budget moins un sur la source directe, `closed_ball`, le groupement et la
  déduplication, avec zéro payload scientifique partiel;
- un différentiel borné contre l'oracle 10.6 et Gamma exhaustif à toutes les
  coupes ouvertes et fermées.

## 9. Décision pour Claude

Le prochain prototype utile n'est pas une nouvelle recherche de voisinage pour
$\lambda(F)$. Il doit d'abord exposer la source directe terminale et son flux de
suppressions, puis mesurer la dichotomie ci-dessus : nombre de facettes du cœur,
requêtes fermées non vides, groupes directs minimaux, co-minimiseurs, doublons,
octets de runs et high-water. L'oracle général reste le juge hostile et le repli
hors porte.

Cette voie évite $\mathrm{Star}(D_k)$, Gamma global, l'univers
$\binom{n}{k}$, les cofaces successives et toute mosaïque de Delaunay d'ordre
supérieur. Elle ne qualifie ni le contrat public v2, ni les verticales, ni le
50 k, ni un statut exact.

GCP non utilisé.
