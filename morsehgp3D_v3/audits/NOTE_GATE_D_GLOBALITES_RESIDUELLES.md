# Gate D — ce qui reste global après le parent local

Date : 9 août 2026 UTC.

Cadre : `backend=architecture_math`, `profile=hgp_reduced_quantized_u16`,
`mode=residual_global_state_factorization`, `public_status=not_claimed`.

> [!IMPORTANT]
> Le parent local et le propriétaire canonique peuvent retirer l'état global du
> **parcours de l'arrangement**. Ils ne retirent pas les dépendances globales de
> la **hiérarchie HGP** : complétude des incidences silencieuses, ordre exact en
> $\beta$, fermeture atomique des ex æquo, partition horizontale vivante,
> provenance de couverture et jointure verticale. Ces dépendances peuvent être
> externalisées et segmentées; elles ne peuvent pas être remplacées par des
> décisions indépendantes par sommet.

Cette note répond à la question architecturale laissée ouverte par
[`NOTE_PARENT_LOCAL_REVERSE_SEARCH_GATE_D.md`](NOTE_PARENT_LOCAL_REVERSE_SEARCH_GATE_D.md).
Elle suppose le parent local exact et, pour chaque record scientifique, une
clef canonique et un propriétaire unique. Elle ne suppose ni catalogue résident,
ni mosaïque de Delaunay d'ordre supérieur, ni $\Gamma$ global matérialisé.

## 1. La frontière exacte

| objet | globalité | doit résider en RAM ? | raison |
| --- | --- | --- | --- |
| nuage $X$ et index certifié | lecture globale | oui, ou stockage adressable en $O(n)$ | `next`, coquilles, rangs et lots doivent rester décidés contre tout $X$ |
| racine et somme $A_X$ | réduction globale unique | non au-delà de $O(1)$ | canonisent parent et propriétaire |
| `seen/frontier/visited` | accidentelle | non | le parent de reverse search les remplace |
| table `emitted` | accidentelle sous propriétaire complet | non | support canonique puis propriétaire unique donnent l'émission unique |
| source d'incidences silencieuses | autorité globale | non, si elle est complète et streamée | les événements critiques seuls ne déterminent pas tout $\Gamma_k$ |
| carrier d'un bras strict non-cœur | local jusqu'à une clef $R\in D_k$, puis global | non pour la descente; partition requise pour le dernier `find` | une baisse canonique de $\beta$ atteint le cœur, mais sa racine dépend de l'histoire stricte |
| ordre exact et lots égaux en $\beta$ | barrière globale | non, tri externe autorisé | aucune mutation n'est sûre avant le dernier record de même niveau |
| partition horizontale active | état global logique | pas nécessairement; cache et stockage externe permis | une incidence future doit retrouver la composante de ses carriers |
| `coverage_delta` et provenance | historique global append-only | non | une incidence silencieuse peut modifier un futur lot sans créer de nœud |
| applications verticales | jointure globale adjacente | non, sweep externe permis | la cible dépend de l'état fermé de l'ordre inférieur à la même coupe |
| forêt et journaux terminés | sortie globale | non | segments immuables, runs et checkpoints suffisent |

Le mot **local** qualifie donc le choix scientifique au sommet. Il ne signifie ni
« sans lecture du nuage », ni « sans tri », ni « sans état de composantes », ni
« sans sortie globale ».

## 2. Factorisation constructive sans mosaïque globale

Sous une source finie et complète, le pipeline exact minimal est le suivant.

1. La reverse search émet chaque événement critique auprès de son propriétaire,
   sans table de sommets globale.
2. Une source directe terminale développe ses facettes du cœur, puis la
   dichotomie de
   [`NOTE_GATE_D_PREMIERES_INCIDENCES_DU_COEUR.md`](NOTE_GATE_D_PREMIERES_INCIDENCES_DU_COEUR.md)
   décide leurs premières incidences. Pour un payload qui exige $M(F)$, elle les
   émet toutes. Pour le seul quotient $H_0$ régulier, la
   [`note d'attache unique`](NOTE_GATE_D_UNE_ATTACHE_PAR_FACETTE_COEUR.md)
   remplace les familles silencieuses par au plus une attache par facette cœur.
   La
   [`descente locale de carrier`](NOTE_GATE_D_DESCENTE_LOCALE_CARRIER_ET_FRONTIERE_GLOBALE.md)
   transforme son bras strict en clef cœur; seul le `find` de cette clef dans la
   partition pré-lot reste global. Chaque record possède une identité canonique
   et une preuve de complétude relative à cette source.
3. Les producteurs écrivent des runs bornés triés par la clef
   `(ordre, beta_exact, type, identite)`.
4. Une fusion externe déduplique les identités et forme les groupes maximaux de
   même $\beta$ exact.
5. Le réducteur horizontal fige l'état pré-lot, résout toutes les composantes du
   groupe égal, puis commet le lot atomiquement.
6. La forêt horizontale et `coverage_log` sont écrits en segments append-only.
   La partition reste logiquement vivante; un journal externe de versions peut
   remplacer le locator résident.
7. Les requêtes verticales sont écrites, triées et jointes par sweep avec
   l'histoire fermée de l'ordre inférieur.
8. Une seconde jointure ordonnée vérifie les carrés de naturalité et ferme les
   compteurs de masse, de couverture et de provenance.

Cette factorisation évite simultanément le catalogue critique résident, le
graphe d'arrangement résident, les snapshots complets du DSU, une matrice
verticale tous ordres contre tous ordres et toute mosaïque de Delaunay d'ordre
supérieur. Un batch d'I/O n'est toutefois pas un lot sémantique : un même niveau
exact peut traverser plusieurs runs et plusieurs fenêtres.

## 3. Pourquoi les quatre barrières principales sont intrinsèques

### 3.1 Complétude de la source silencieuse

Un événement critique donne une information positive locale. Il ne prouve pas
que toutes les cofaces nécessaires à la connectivité ont été vues. Une coface
non critique peut n'ajouter aucun point, ne créer aucun nœud public et pourtant
attacher une facette qui sera décisive lors d'un lot ultérieur. C'est exactement
la fonction des incidences silencieuses dans le contrat `hgp_reduced`.

Le parent et le propriétaire ne résolvent donc pas, à eux seuls, la question
suivante :

> produire, avec certificat de complétude, toutes les premières incidences utiles
> de chaque facette ou un quotient prouvé équivalent à $H_0$.

C'était le verrou mathématique aval principal après le parent. Il est désormais
**fermé conditionnellement** pour la sémantique horizontale normalisée : le
corollaire 4.1 prouve que les cofaces directes et toutes les premières incidences
des facettes du cœur suffisent sous l'autorité régulière; le théorème de fenêtre
rend les blocs saturés de haut rang inertes; enfin, la dichotomie boule fermée /
minimum direct reconstruit chaque $M(F)$ sans étoile globale lorsque la source
directe est terminale et sa convention d'extra-shell explicite.

Le résiduel v3 est donc un verrou **de production et de capability**, plus une
preuve nouvelle à découvrir : fermer la source directe, développer exactement
$D_k$, terminer toutes les requêtes fermées, authentifier l'autorité de fenêtre,
lier leurs watermarks et alimenter le fold. Pour le quotient régulier, il n'est
même plus nécessaire de matérialiser tous les $M(F)$ : une attache propriétaire
par facette suffit. `ResolveStrictCarrier` se factorise désormais en une
descente locale strictement décroissante jusqu'à $R_F\in D_k$, puis un unique
`find` de $R_F$ dans l'état strict. La boîte noire géométrique et le locator des
facettes non-cœur disparaissent; la partition du cœur reste une information
globale. Une source seulement plausible ou un préfixe Gabriel direct ne suffit
toujours pas. Les contre-exemples et
obligations sont recensés dans
[`INCIDENCES_SILENCIEUSES_GAMMA.md`](../../docs/math/INCIDENCES_SILENCIEUSES_GAMMA.md)
et dans la [spécification](../../docs/SPECIFICATION_MORSEHGP3D.md). Le contrat
v2 exhaustif reste hors de cette réduction.

### 3.2 Aucun commit en $\beta$ sans watermark

Deux exécutions peuvent partager exactement le même préfixe de records observés,
puis différer par un record futur de niveau $\beta'<\beta$ ou
$\beta'=\beta$. Le premier cas change l'état strict antérieur; le second change
le lot atomique courant. Sans producteur monotone accompagné d'un watermark
certifié, seule la terminalité de la source ou une fusion globale permet de
committer $\beta$.

Le tri externe retire cette globalité de la RAM, pas de la sémantique. Le merger
doit conserver la dernière clef ouverte jusqu'à la preuve qu'aucun autre run ne
contient la même valeur rationnelle.

### 3.3 Un lot égal ne se réduit pas record par record

Considérons deux racines pré-lot $R_1,R_2$, un carrier nouveau $F$, et au même
niveau les incidences $\left\lbrace R_1,F\right\rbrace$ et
$\left\lbrace F,R_2\right\rbrace$. Prise seule, chacune ressemble à une
continuation avec une racine antérieure. Leur quotient commun est pourtant une
multifusion à deux racines. Une chaîne de ce type peut avoir une longueur
arbitraire.

Le lot peut être calculé avec des composantes connexes externes et un staging
borné, mais la mutation publique ne devient visible qu'après fermeture de toute
sa composante égale.

### 3.4 La partition horizontale contient une information irréductible

Une incidence future entre deux carriers doit distinguer « déjà dans la même
composante » de « encore séparés ». Dans le modèle abstrait sans théorème
géométrique supplémentaire, un historique peut induire toute partition de $m$
handles actifs. Encoder cette réponse demande l'équivalent de la partition —
jusqu'à $\log_2 B_m=\Omega(m\log m)$ bits, où $B_m$ est le nombre de Bell — ou
la relecture d'un journal permettant de la reconstruire.

Un locator externalisé et un cache borné sont acceptables. Supprimer
l'information de composante ne l'est pas. La forêt publique seule est
insuffisante : une continuation silencieuse peut modifier locator et couverture
sans créer de nouveau nœud.

### 3.5 Le locator résident est supprimable

L'information précédente n'impose pas une table proportionnelle aux facettes en
RAM. Sous une source terminale exactement triée, un fold externe déterministe
reproduit le DSU résident.

1. Trier les occurrences de facettes, leur attribuer un handle stable et créer
   une version initiale.
2. Conserver un journal append-only `version -> successor`, chaque lien portant
   le lot où il devient effectif; les composantes ne se scindent jamais.
3. Avant un lot $a$, résoudre chaque handle touché vers sa version strictement
   antérieure par jointures triées et pointer-jumping.
4. Calculer extérieurement les composantes connexes de tout le lot égal, puis
   compter les racines publiques distinctes et les carriers latents de chaque
   groupe.
5. Ajouter atomiquement les nouveaux liens, versions, parents, provenances et
   introductions de couverture.

L'induction sur les lots exige plus que le pointer-jumping : le snapshot doit
séparer racines publiques et carriers latents, conserver les continuations sans
nœud, cacher les activations du niveau courant et committer une unique version
par composante égale. Le contrat complet, son oracle borné et ses mutations sont
décrits dans la
[`note de fold pré-lot`](NOTE_GATE_D_FOLD_PRELOT_EXACT.md).

Le cas $q_R=1$ est décisif. Même sans nœud public, toute nouvelle facette doit
laisser une attache ou version interne vers la racine continuée. Sinon un lot
futur ne peut plus distinguer une facette encore latente d'une facette déjà
rattachée. Le locator résident est ainsi une optimisation de débit, pas une
nécessité mathématique. Le journal externe retire sa résidence, jamais son
information, et peut exiger plusieurs tris globaux; aucune borne de SLO n'en
découle.

## 4. Couverture : la vue seuil n'est pas l'autorité source

Un résumé plafonné à vingt identifiants suffit pour décider la vue
`relation=at_least, min_cluster_size=20`; il ne suffit pas pour produire le
`coverage_delta` exact. Les couvertures
$\left\lbrace1,\ldots,20,100\right\rbrace$ et
$\left\lbrace1,\ldots,20,101\right\rbrace$ ont la même visibilité au seuil.
Ajouter ensuite le point $100$ produit un delta vide dans le premier cas et non
vide dans le second.

Le résumé plafonné est donc un consommateur aval. L'autorité doit conserver soit
les identités exactes de couverture, soit un DAG append-only dont l'union et la
différence sont rejouables exactement. Les couvertures n'ont pas besoin d'être
matérialisées à chaque coupe.

## 5. La jointure verticale est globale, pas quadratique

Les dix ordres ne doivent pas devenir dix arbres indépendants. Une composante de
l'ordre $k+1$ doit rejoindre la composante de l'ordre $k$ qui contient ses
carriers à la même coupe fermée. L'union des `PointId` ne suffit pas : deux
composantes de $\Gamma_k$ peuvent se recouvrir en observations tout en restant
distinctes, et les births latentes doivent rester adressables.

Une matrice verticale résidente n'est pas nécessaire. Pour chaque source, on
peut émettre des requêtes `(ordre_cible, beta, carrier, composante_source)`, les
trier extérieurement, puis rejouer une fois la forêt cible en ordre de $\beta$.
Chaque lot cible est appliqué en entier avant les requêtes à coupe fermée. Les
réponses sont ensuite jointes aux sources, et une seconde passe vérifie les
carrés de naturalité.

La globalité verticale est ainsi une jointure adjacente ordonnée. Pour
$K_{\max}=10$, elle n'autorise ni dix MST sur les points, ni une matrice de toutes
les paires d'ordres, ni une mosaïque globale.

## 6. Le contrat v2 rend une partie de la masse inévitable

Le profil exact v2 définit `hgp_reduced` à partir de toutes les facettes et
cofaces de $\Gamma_k$, de leurs incidences, des lots exacts et de leurs deltas.
Tant que les identités de facettes, cofaces, groupes et provenances font partie
du payload public ou de son reçu rejouable, leurs octets sont une borne de sortie
et non un défaut de l'algorithme. Ils peuvent être streamés; ils doivent être
encodés quelque part.

Un quotient $H_0$ normalisé pourrait être beaucoup plus petit. Il ne serait pas
byte-compatible avec un contrat qui expose `batch_id`, identités de cofaces
Gamma, snapshots ou journaux exhaustifs. L'adopter demanderait une révision
contractuelle versionnée, un théorème de complétude de la source quotientée, la
validation des verticales et de nouveaux oracles. Une optimisation ne peut pas
effectuer cette migration implicitement.

## 7. État résident minimal et compteurs Gate D

L'état simultané visé se factorise en

$$M_{\mathrm{resident}}=O(n)+M_{\mathrm{locator\_buffer}}+M_{\mathrm{batch}}+M_{\mathrm{run}}+M_{\mathrm{scheduler}}.$$

- $O(n)$ couvre l'entrée immuable, l'index, la racine et $A_X$;
- $M_{\mathrm{locator\_buffer}}$ est un cache ou buffer de jointure borné; la
  partition complète peut vivre dans le journal externe de versions;
- $M_{\mathrm{batch}}$ est le staging du lot égal courant, segmentable par
  composantes externes mais committé atomiquement;
- $M_{\mathrm{run}}$ est une fenêtre bornée de tri et de fusion;
- $M_{\mathrm{scheduler}}$ est une pile de reverse search ou une file GPU
  explicitement bornée, jamais une table de tous les sommets.

Gate D doit mesurer au minimum, par ordre et au total :

- sommets shallow, flats incidents, enfants et profondeur de l'arbre;
- événements critiques proposés, propriétaires, doublons évités et octets;
- incidences directes **et silencieuses**, carriers distincts et première
  incidence;
- nombre de niveaux, taille maximale d'un lot égal et composantes temporaires du
  lot;
- handles actifs, taille et I/O du locator, unions utiles et silencieuses;
- facettes et points de `coverage_delta`, nœuds, enfants et segments de forêt;
- requêtes verticales, réponses, résidus, carrés de naturalité et octets de
  jointure;
- runs, high-water hôte/device, octets écrits et lus, reprises et temps par
  étage.

Mesurer seulement les sommets d'arrangement ou le catalogue critique ne qualifie
donc pas le contrat 50 k.

## 8. Décision

- état global du parcours d'arrangement : **supprimable par parent et
  propriétaire locaux**;
- lecture globale du nuage : **intrinsèque, $O(n)$ et immuable**;
- complétude sparse des incidences silencieuses : **théorème conditionnel fermé;
  tous les $M(F)$ compressibles à une attache par facette cœur, bras ramené
  localement au cœur puis résolu par le fold; producteur v3 terminal et
  capability commune encore ouverts**;
- tri exact et fermeture des lots : **intrinsèques mais externalisables**;
- partition horizontale et provenance de couverture : **information intrinsèque
  mais locator résident supprimable par fold externe multipasse**;
- jointure verticale adjacente : **intrinsèque mais streamable**;
- identités exhaustives du contrat v2 : **masse de sortie obligatoire jusqu'à
  migration contractuelle explicite**;
- mosaïque de Delaunay d'ordre supérieur, $\Gamma$ résident complet et catalogue
  global en RAM : **toujours interdits et non nécessaires**.

GCP non utilisé.
