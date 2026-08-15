# Réponses auditées — route sparse Gabriel, gateways et fold normalisé

Date : 12 août 2026 UTC.

Cadre : `phase=exploration_v3_hors_registre`,
`backend=cpu_reference_bounded_oracles_and_g4_diagnostic`,
`profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`,
`public_status=not_claimed`.

Question auditée :
[`QUESTIONS_CLAUDE_ROUTE_SPARSE_20260812.md`](QUESTIONS_CLAUDE_ROUTE_SPARSE_20260812.md).
Le `HEAD` à la consolidation est
`8c00ab07695ef353e673ab73a778a6f260c87509`. Le prototype suivi
`certified_locality_probe.cpp` a alors le SHA-256
`87f7e77d914f59fbf846a0d7a8476a420f328a9604b32cc648606d2c072ca43a`.
Les conclusions mathématiques ci-dessous ne reçoivent pas ce probe de
dimensionnement comme une implémentation conforme.

## Verdict court

La route doit viser le **foncteur horizontal normalisé des composantes non
triviales de Gamma**, décoré par la couverture en `PointId`, et non Gamma
facetté ni le K-graphe de Gabriel brut. Sous une autorité terminale de fenêtre
et la porte régulière pertinente, `cofaces directes + toutes les premières
incidences M(F)` est une source suffisante. Ce n'est pas une condition
nécessaire et ce n'est pas encore une source produite par le probe courant.

Les réponses qui changent l'architecture sont :

1. les compteurs q2/q3/q4 actuels ne sont pas encore des cofaces directes ;
2. la porte régulière ne peut pas être supposée sur les familles quantifiées ;
3. les supports multiples se traitent par un quotient de plateau ou par un
   générateur de boule saturée, jamais en choisissant un pivot dans leur union ;
4. une nouvelle clé de Gram permet de comparer les niveaux u16 avec 256 bits,
   tandis que la représentation `Sphere` actuellement réutilisée exige encore
   son comparateur fixe de 384 bits ;
5. une facette se déduplique par ses labels, mais une descente de resolver n'a
   aucune petite borne de profondeur ;
6. le fold se parallélise dans un lot gelé, pas en permutant les niveaux ;
7. la sortie normalisée est un journal de transitions et de couverture, pas
   une suite de snapshots facettés.

## Q0 — quand une activation est-elle une coface directe ?

Soit une boule minimale de support propre positif `U`, de taille `q`, et soit
`I` l'ensemble **global et complet** des points strictement intérieurs, de
taille `p`. Si son shell global est exactement `U`, alors

$$Q=U\cup I,\qquad |Q|=p+q,\qquad k=p+q-1$$

est la coface directe associée à l'ordre `k`. Pour `K_eff=10`, la fenêtre
`p+q<=11` couvre donc les cofaces directes demandées aux ordres au plus dix.
Cette identité mathématique est admise.

Elle ne s'applique au producteur que si cinq obligations sont reçues :

- `U` est un support minimal affinement indépendant et propre positif ;
- la requête d'intérieur est terminale et publie les identités de **tous** les
  éléments de `I`, pas seulement un compteur arrêté au seuil ;
- le shell global est fermé et ne contient aucun label hors `U` dans la
  fenêtre régulière pertinente ;
- les représentations multiples d'une même boule sont réunies par une
  `BallKey` exacte avant l'attribution de l'owner ;
- chaque record possède un owner exact-once et une provenance rejouable.

Le mode `arity` audité ne satisfait aucune de ces cinq obligations ensemble.
Il compte des tuples de support, arrête les intérieurs à un seuil, ne publie ni
`I`, ni shell, ni `BallKey`, ni record, et sa fenêtre q3/q4 peut être tronquée.
Son juge compare trois cardinalités calculées avec les mêmes primitives de
sphère ; une omission compensée par un doublon peut donc rester verte. La
réponse à « mes activations sont-elles la source directe complète ? » est
ainsi : **oui pour l'objet mathématique régulier après les cinq fermetures, non
pour la sortie logicielle actuelle et non pour un plateau extra-shell**. Les
`68,07` objets par point mesurés sur `terrain` sont des supports proposés, pas
des `BallActivation` ou des cofaces directes reçues. La multiplication
`68 * 4` n'est donc qu'un dimensionnement de la branche régulière avant
fermeture et déduplication, pas une borne produit reçue.

Plus précisément, pour une boule dégénérée posons `E=X cap boundary(B)`. Les
cofaces directes portées par cette boule sont les ensembles de la forme

$$Q=I\cup A,\qquad A\subseteq E,\qquad c_B\in\mathrm{conv}(A),\qquad |Q|\leq11.$$

Le record `(U,B)` ne représente que le choix `A=U`. Il donne bien une coface
directe minimale si son census `I` est fermé, et propose au plus quatre bras
pour **cette** coface, mais il n'énumère pas les autres choix admissibles dans
`E`. Dès que `E` diffère de `U`, `4 * nombre_de_supports` ne majore donc plus la
famille directe complète.

## Q1 — régularité, grilles LiDAR et supports multiples

### Aucune fraction ne peut être annoncée aujourd'hui

Coplanarité, cosphéricité, extra-shell et multiplicité du support sont quatre
faits distincts. Une nappe coplanaire n'est pas automatiquement cosphérique.
Réciproquement, un point supplémentaire sur le shell ne rend pas forcément le
support minimal non unique : une paire antipodale peut rester l'unique support
minimal en présence d'un troisième point sur son cercle.

Le compteur `coquilles DEGENEREES` du probe cherche un point de shell et
l'étiquette « support minimal non unique ». Avec `max_neighbours=n-1`, son
prédicat d'extra-shell est global pour chaque support effectivement émis; en
revanche, `support_window=48` peut encore tronquer l'univers des supports q3/q4
proposés. Il mesure donc une fraction de records émis portant une extra-shell,
ni une fraction de boules ou cofaces, ni la multiplicité des supports. Aucune
fraction à 50 k ne se déduit honnêtement des mesures publiées.

Le préflight pertinent groupe les sorties par `BallKey` et publie séparément,
sur un shell fermé :

- le nombre de boules avec `E\U` non vide ;
- le nombre de boules possédant plusieurs sous-ensembles minimaux positifs de
  `E`, chacun de taille au plus quatre ;
- les mêmes fractions pondérées par activations, bras et travail ;
- la part au-dessus de la fenêtre, rendue H0-inerte par le théorème saturé, et
  la part **dans** la fenêtre qui exige régularité ou quotient de plateau.

Un échantillonnage donne un diagnostic de conception. Seule une façade
terminale exhaustive ou un certificat d'exclusion peut ouvrir la source.

### Le pivot dans l'union des supports est faux

Prendre `u0` dans l'union de supports minimaux ne garantit pas que
`beta(Q\{u0})<beta(Q)`. Pour quatre sommets d'un carré cosphérique, les deux
diagonales sont des supports minimaux. Après suppression d'un sommet d'une
diagonale, l'autre diagonale peut rester entière ; le niveau ne décroît pas.
Le prétendu pivot n'appartient alors à aucune composante strictement
antérieure certifiée. L'étoile silencieuse régulière ne se généralise donc pas
ainsi.

Deux traitements exacts restent possibles :

1. fermer le shell et contracter atomiquement l'hypergraphe complet du plateau
   pertinent, avec une preuve de ses composantes strictes ;
2. représenter la boule par son générateur saturé `S=(X cap B)` et son bloc de
   Johnson implicite. L'intersection de générateurs restitue exactement les
   composantes de Gamma, sans choisir de support privilégié.

Le second traitement est la bonne référence sans position générale, mais son
énumération brute des supports et des memberships n'est pas une route 50 k.
Dans les blocs vérifiant `p+s>K_eff+1`, le théorème d'inertie saturée permet
une tombstone H0 même avec extra-shells et supports multiples. Dans la fenêtre
utile, faute de quotient dédié, la décision correcte reste
`unsupported_rank_relevant_extra_shell_degeneracy`.

Les extra-shells rapportées dans la note de mesure portent précisément sur des
records avec `p+q<=11`, où `q` est la taille du support exhibé. Elles restent
donc dans la fenêtre de rang pertinente et ne sont pas éliminées par le
théorème 4.2.

La connexité fermée du bloc de Johnson ne suffit pas à fabriquer ce quotient
pertinent avec quelques facettes arbitraires. Le fold doit aussi connaître
chaque composante **strictement antérieure** qui rencontre une `k`-facette du
saturé. Un sous-ensemble canonique de facettes peut manquer précisément un
carrier silencieux réutilisé plus tard. Une compression en `O(|S|)` n'est donc
admise que si un certificat d'endpoints prouve qu'elle rencontre tous ces
carriers, ou si une autorité de rétraction déjà établie lie exhaustivement les
facettes du cœur au bloc. Le théorème 4.2 évite cette dette seulement dans le
cas haut rang où une unique composante stricte couvre déjà tout `S`.

Même retenir toutes les facettes du cœur actuellement connues dans `S` ne ferme
pas l'avenir. E5 demande que le saturé `ACDE` activé à `33/2` conserve la
possibilité de résoudre plus tard `AC`, lorsque `ABC` la réutilise. Le record
compact exact doit donc garder `S` et fournir un join historique complet
`F subseteq S -> handle_closed(S)`, ou matérialiser `F`; quelques facettes qui
couvrent ponctuellement `S` ne suffisent pas. La voie saturée sans position
générale exige ainsi les memberships fermés, tous les générateurs pertinents,
le join antérieur `|S intersection T|>=k`, l'atomicité du niveau et le lookup
futur de containment. La tour globale prouve cette architecture, mais ses
supports et joins bruts sont combinatoires; aucune variante 50 k de ces quatre
autorités n'est aujourd'hui démontrée.

### Changer l'hypothèse ou changer l'objet

Trois contrats cohérents sont possibles, et ils ne doivent pas être mélangés :

- **Gamma exact sur l'entrée u16 originale** : traiter les plateaux pertinents
  ou échouer fermé ;
- **Gamma exact sous porte régulière certifiée** : accepter seulement les
  nuages satisfaisant réellement cette porte, ce qui ne couvre pas par décret
  toutes les familles LiDAR ;
- **filtration symboliquement perturbée** : potentiellement plus simple, mais
  c'est un autre objet, avec niveaux et multifusions modifiés, jamais Gamma de
  l'entrée originale.

Le K-graphe de Gabriel brut fournit encore un quatrième objet exact mais plus
faible : sa propre filtration de composantes. Une MSF la compresse exactement,
mais E5 interdit de l'appeler Gamma ou MorseHGP.

## Q2 — clé exacte et borne de bits u16

Il n'existe pas de dénominateur commun global raisonnable. Il existe en
revanche une paire entière canonique bornée. Pour un support affinement
indépendant de taille `q`, poser `r=q-1`, prendre les vecteurs de différences
comme colonnes de `V`, puis `G=V^T V` et `h=diag(G)`. Pour `q>=2`, le rayon
carré vaut

$$\beta=\frac{N}{4D},\qquad D=\det(G)>0,\qquad N=h^{T}\mathrm{adj}(G)h.$$

Pour `q=1`, la clé vaut `0/1`. Le support propre d'un ensemble dépendant est
réduit à une arité inférieure avant cette formule. La clé canonique est
`(N/g,4D/g)` avec `g=gcd(N,4D)` et dénominateur positif.

Si les coordonnées appartiennent à `[0,65535]`, chaque différence est de
module strictement inférieur à `2^16` et chaque entrée de `G` est de module
strictement inférieur à `2^34`. Des bornes conservatrices donnent :

| support | numérateur avant réduction | dénominateur avant réduction |
| --- | ---: | ---: |
| q2 | `<2^68` dans la formule uniforme; après réduction le numérateur est `<2^34` et le dénominateur au plus `4` | `<2^36` dans la formule uniforme |
| q3 | `<2^104` | `<2^70` |
| q4 | `<2^141` | `<2^104` |

Pour q3, `D` est le carré de la norme d'un produit vectoriel et reste sous
`2^68`. Pour q4, `D=det(V)^2<2^102`; chaque cofacteur de `G` est sous `2^69`,
et les neuf termes de `N` donnent la borne `2^141`.

Comparer deux clés de Gram réduites demande des produits croisés de moins de
245 bits. Un entier signé de 256 bits, implémenté par quatre limbs de 64 bits
avec retenues contrôlées, suffit donc pour **cette nouvelle représentation**
des niveaux q1--q4 sur u16.

Ce résultat n'autorise pas à raccourcir le comparateur existant. La structure
`mhgp::Sphere` représente le niveau par `|num|^2/den^2` avant l'annulation
algébrique de Gram. Ses bornes auditées sont `|num|^2<2^180,8` et produit
croisé `<2^326`; `sphere_cmp_beta` emploie donc correctement six limbs, soit
384 bits. Deux options exactes sont recevables : conserver ce comparateur fixe
déjà éprouvé, ou introduire une `GramLevelKey` canonique avec calcul de `N`,
`4D`, pgcd, sérialisation et mutants propres. Réutiliser les bornes 256 bits
avec l'ancien layout serait un overflow.

Le tri primaire porte sur `beta`; le lot horizontal est identifié par
`(k,beta)`. Tous ses records sont quotientés atomiquement. La clé secondaire
doit décrire l'événement **après** déduplication : `(k,sorted PointIds(Q))` dans
la branche régulière ou `(k,BallKey)` pour un plateau. L'arité du support et le
`source_kind` restent de la provenance; les inclure dans l'identité laisserait
plusieurs événements pour une même boule à supports multiples. Cette clé rend
le stockage reproductible mais ne séquentialise aucune décision dans le lot.
Un tri lexicographique de `(num,den)` ne respecte pas l'ordre rationnel : il
faut un comparateur croisé, un filtre par intervalles suivi du comparateur fixe,
ou un rang exact construit après regroupement. Pour les verticales, tous les
ordres partageant `beta` doivent avoir engagé leur post-état avant de construire
les applications du macro-lot.

## Q3 — clé de bras, co-minimiseurs et profondeur

### Déduplication

La clé scientifique d'une facette est
`(cloud_digest, epoch, order, sorted PointIds(F))`. Sa miniboule et `beta(F)`
sont des fonctions de cette clé ; deux cofaces proposant le même `F` ne
peuvent pas lui donner deux niveaux exacts. Le niveau, le support et la
`BallKey` doivent néanmoins être répétés dans le reçu comme contrôles
d'intégrité. Une divergence est une faute, pas une seconde facette.

### Descente du resolver

La stricte décroissance de `beta` interdit les cycles et garantit la
terminaison sur un univers fini. Elle ne donne aucune petite borne. Une
descente de facettes de cardinalité `k` peut, en général, visiter une chaîne
dont la seule borne universelle immédiate est le nombre de facettes possibles,
au plus `C(n,k)-1` transitions. La documentation de référence confirme
qu'aucune borne pratique n'est démontrée.

Le produit doit donc utiliser mémoïsation, partage structurel et compression
de chemins, mesurer la profondeur et le high-water, et rendre `unresolved`
sur budget plutôt que d'inventer un terminal. « Au plus quatre bras » borne
le fan-out proposé par une coface directe en dimension trois ; cela ne borne
ni le nombre de facettes uniques, ni la profondeur de leur résolution.

Le dépôt contient déjà, dans le probe borné de premières incidences, le motif
utile de descente à témoin transporté : chaque remplacement prouve un niveau
strictement plus petit et les branches terminales retombent sur une coface
directe antérieure. Pour casser toute circularité dans le produit, un hit n'est
admis que dans le dictionnaire du cœur construit inductivement avec un stamp
strictement pré-lot. Une gateway `|J_F|>=2` exige un terminal déjà enraciné;
un simple bras direct peut rester latent jusqu'à sa première promotion.

### `M(F)`

Oui :

$$\lambda(F)=\min_{x\notin F}\beta(F\cup\lbrace x\rbrace),\qquad M(F)=\left\lbrace F\cup\lbrace x\rbrace:\beta(F\cup\lbrace x\rbrace)=\lambda(F)\right\rbrace.$$

Tous les co-minimiseurs exacts sont requis dans la branche `|J_F|=0`. Si
`|J_F|=1`, l'unique extension égale installe `F`. Si `|J_F|>=2`, la famille
entière des extensions égales conflue vers le même apex sous la porte
régulière ; une gateway canonique peut alors représenter son **effet H0**,
mais seulement avec un reçu de confluence et un carrier terminal. L'expression
« une attache par facette » ne s'applique pas à la branche zéro sans cette
distinction.

Enfin, `beta(F)<beta(Q)` ne borne pas à lui seul la requête spatiale `J_F`.
La localité doit être recertifiée pour la boule `B_F` depuis un point de son
support, ou la requête de boule fermée LBVH doit produire une exclusion
terminale. Le voisinage de la coface parente n'est pas un certificat de
fermeture de `J_F`.

## Q4 — parallélisme exact du fold

Les niveaux distincts sont causalement ordonnés : `q_R`, les parents et les
deltas d'un lot sont définis sur les racines de la coupe **strictement
antérieure**. Muter un DSU dans un autre ordre change la généalogie, même si la
partition finale reste la même.

Le parallélisme exact se place à trois endroits :

1. génération, certification, déduplication et tri des records ;
2. à niveau `a` fixé, gel des roots `<a`, construction en parallèle du quotient
   complet sur tokens enracinés, latents et facettes égales, calcul de ses
   composantes connexes, puis une seule action par composante du quotient ;
3. traitement indépendant des composantes finales qui n'ont aucune arête
   future commune.

Un Boruvka ou un Kruskal parallèle peut construire une MSF de l'autorité
pondérée. Le lemme MSF garantit alors les mêmes composantes à chaque coupe.
Il faut ensuite reconstruire la hiérarchie en regroupant les arêtes retenues
par poids exact, en gelant la partition pré-lot et en contractant chaque lot
atomiquement. Émettre un nœud par union binaire reste faux. Le simple test
« endpoints actuellement disjoints » ne prouve pas que deux niveaux peuvent
être permutés.

Une architecture plausible est donc `source -> exact level rank/sort -> MSF
sparse parallèle -> reconstruction par buckets égaux`. Elle évite Gamma, ses
cliques, ses cofaces et ses incidences globales. La dépendance entre seuils est
sémantique; elle n'impose pas nécessairement un DSU physique séquentiel pour
chaque valeur, car un arbre de reconstruction offline peut aussi être construit
en parallèle depuis la MSF. Employer l'ordre des rondes Boruvka comme filtration
ou émettre les unions binaires d'un tie resterait faux. Il faut mesurer
séparément niveaux distincts, taille maximale d'un lot, comparaisons exactes,
arêtes MSF retenues, opérations de quotient et bande passante. Aucune preuve
actuelle ne dit que ce fold est négligeable devant la source à 50 k.

## Q5 — cible exacte de `normalized_horizontal_h0`

La cible n'est ni une partition des points, ni les facettes à chaque niveau.
Deux composantes distinctes de facettes peuvent couvrir des ensembles de
`PointId` qui se chevauchent. L'objet minimal est, pour chaque ordre, un
système persistant **indexé de classes de carriers non triviales**, muni de sa
couverture.

Une sérialisation suffisante est un journal ordonné par niveau contenant :

- l'ordre et le niveau exact ;
- la classe résultante canonique ;
- les racines antérieures distinctes et `q_R` ;
- le verdict naissance, continuation ou multifusion ;
- les parents lorsque `q_R>=2` ;
- les `added_point_ids` et la couverture initiale d'une naissance ;
- les racines finales, le digest de source et l'autorité de complétude.

Une continuation `q_R=1` à delta vide peut être normalisée hors du journal.
Une continuation qui agrandit la couverture doit rester rejouable, même si
elle ne crée aucun nœud. Les singletons d'ordre un sont des naissances
publiques. Aux ordres supérieurs, les facettes isolées et le terminal `k=n>1`
restent des carriers latents/certificats de totalité ; ils ne créent aucun
nœud public dans `hgp_reduced`, dont la forêt terminale est vide.

Les identités complètes des facettes sont nécessaires aux clés de source, aux
gateways, au resolver et au certificat différentiel. Elles ne font pas partie
du payload horizontal normalisé. Les naissances de toutes les facettes sont
obligatoires pour `full_pi0`, pas comme événements publics de cette cible
réduite.

La spécification sémantique nécessaire et suffisante est : l'inclusion du
flot candidat induit un isomorphisme du foncteur filtré des composantes non
triviales de Gamma, décoré par la couverture ponctuelle, aux coupes ouvertes
et fermées. De façon opérationnelle, à chaque niveau exact, chaque composante
du quotient exhaustif gelé doit avoir les mêmes racines antérieures distinctes,
la même classe résultante et le même delta de couverture dans le candidat.
`Directes + tous M(F)` sous la porte globale et l'autorité de fenêtre est une
condition suffisante de cet énoncé, pas une caractérisation nécessaire.

Cette cible permet un jalon exact horizontal sans prétendre produire le
transcript Gamma, les identités v2, `full_pi0`, les verticales ou le produit
MorseHGP3D complet. Le SLO officiel exige encore dix forêts, les applications
verticales, les lots et le certificat minimal contractuel.

## Q6 — audit de la liste des dettes

Les quatre dettes reconnues par Claude sont réelles, mais la liste doit être
complétée :

1. la fermeture directionnelle q2 par cône possède des bornes exactes
   conservatrices et une campagne manuelle n'a trouvé aucune différence contre
   le juge, mais aucun des 28 CTests `locality` ne l'exerce; elle peut visiter
   tout l'arbre par cellule et par ancre, puis lancer des requêtes de boule, et
   ne donne aucune borne sous-linéaire ;
2. sans `--judge-census`, le mode `arity` audité peut rendre le code zéro avec
   une fenêtre non saturée. Le message imprimé n'est pas un refus ;
3. le juge d'arité compare seulement les trois comptes, partage les prédicats
   de sphère du générateur et ne compare ni ensembles, ni `I`, ni shell, ni
   `BallKey` ;
4. le commentaire d'en-tête attribuant un facteur causal `384` et la
   superlinéarité à Yao48 n'est pas démontré ;
5. la fermeture de `J_F`, la porte de régularité pertinente, la confluence des
   gateways et la terminaison du resolver manquent encore ;
6. aucun compteur ne borne aujourd'hui la mémoire et le travail complet
   `source + gateways + level order + MSF/fold` ;
7. aucun producteur ne matérialise `BenchmarkOutputContract-v1`.

Le nouveau mode `sparse` ne ferme pas ces points : il utilise un kNN complet
par ancre, puis conserve une fenêtre fixe de 48 partenaires pour q2/q3/q4,
balaie le
nuage pour classer `J_F` et ne construit ni gateway, ni resolver terminal, ni
MSF, ni fold. Ses volumes à `n<=4 000` sont un dimensionnement CPU borné, pas
la réalisation de la route décrite ici.

Avant toute cible GPU, les portes minimales sont donc : records authentifiés,
différentiel par identités de boules et cofaces, shell terminal, matrice de
plateaux/supports multiples, source `M(F)` complète, schedule-invariance du
quotient, comparaison exacte des niveaux, puis pentes de travail et high-water
sur les quatre familles. Une fenêtre ou un budget épuisé rend le record
`incomplete`, jamais absent.

## Décision d'architecture

- Conserver `G_k^+` exhaustif et la tour de boules saturées comme références
  exactes complémentaires.
- Nommer la route produit `normalized_horizontal_h0_direct_gateway`, sans
  réutiliser « K-MST de Gabriel ».
- Conserver d'abord le comparateur `Sphere` fixe de 384 bits; une migration
  vers la clé de Gram 256 bits est une optimisation distincte avec sa propre
  preuve et ses portes. Ne pas imposer de dénominateur commun global.
- Construire une MSF de carriers si elle réduit le stream, puis rejouer ses
  poids par lots atomiques ; un fold direct équivalent reste autorisé.
- Traiter les plateaux pertinents par une autorité dédiée ou échouer fermé.
- Garder G4 fermé tant que la source, les reçus et les pentes locales ne sont
  pas reçus ; aucune des réponses présentes ne prouve le contrat une seconde.

GCP non utilisé.
