# Composition horizontale : source, localisation et payload v7

4 septembre 2026. `phase=exploration_v7_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.
Audit documentaire des sources gelées, sans mutation du produit.

**Conclusion.** Une composition horizontale exacte est démontrable pour
le sous-flot effectivement choisi par la v7, sous une obligation amont
précise : la fermeture complète des boules de support positif minimal
dans la fenêtre de rang. Elle ne demande ni toutes les premières cofaces
ex æquo, ni une régularité géométrique globale, ni un catalogue Gamma.
Le succès du census sur les seuls candidats reçus et le grand-livre de
masses de paires ne sont pas, séparément, ce certificat de fermeture.
La contre-lecture indépendante ferme désormais S1 comme
[théorème géométrique conditionnel](../audits/S1_COURANT.md#6-théorème-géométrique-conditionnel-et-rle),
y compris les marges des filtres et les cellules. Il reste à qualifier
ses primitives et son domaine numérique sur le produit exécuté.
Le présent texte ne déclare donc pas toutes ses prémisses certifiées par
une exécution v7, et ne change aucun statut public.

## 1. Objet exact de la conclusion

Soit un nuage fini de positions distinctes dans le profil u16, avec
identifiants distincts, et $2\leq K\leq K_{\mathrm{eff}}<n$. On écrit
$\beta(Q)=\rho(Q)^2$ et $r_{\max}=K_{\mathrm{eff}}+1\leq11$.
Le même raisonnement vaut lorsque la fenêtre effective est bornée par
$n$, comme le fait [run.hpp, lignes 443–444](../src/pipeline/run.hpp#L443).

L'objet de référence est la filtration des composantes **non triviales**
de Gamma élémentaire, avec leur couverture en points, aux coupes ouvertes
et fermées. Le manuscrit donne les objets et les changements de
représentation suivants :

- Définitions 21–22, pages imprimées 58–60, PDF 84–86 : composantes de
  facettes et K-polyèdres, puis correspondance avec les amas K-NN.
- Définition 25 et fait 12, pages 84–85, PDF 110–111 : miniball unique et
  support positif d'au plus quatre points en dimension trois.
- Proposition 5, page 86, PDF 112 : les cofaces de cardinal K+1 suffisent
  pour les composantes; elle n'identifie pas les graphes d'adjacence.
- Définition 27 et théorème 4, pages 86–89, PDF 112–115 : une coface
  non-Gabriel régulière ne sépare pas les facettes strictement antérieures.
  Cela n'autorise pas à oublier ses facettes simultanées. La proposition 6
  et le théorème 5, pages 90–91, PDF 116–117, sont réfutés dans cette
  utilisation par E5 et ne sont pas des prémisses de cette composition.

Référence : [manuscrit](../../docs/references/MANUSCRIT_THESE_HAUSEUX.pdf).
La suppression des isolés pour K supérieur à un vient du
[théorème 1 transverse](../../docs/math/INCIDENCES_SILENCIEUSES_GAMMA.md#1-référence-exacte-par-gamma),
pas d'une identification tacite avec tous les K-polyèdres du manuscrit.

La conclusion porte sur une bijection des composantes et leur évolution,
leurs points, les naissances réduites, les multifusions et leurs niveaux.
Les parents correspondent par cette bijection; leurs clés concrètes ne
sont pas nécessairement les clés d'une histoire Gamma exhaustive. Les
continuations sans changement de composante ou de points peuvent porter
des matérialisations de facettes propres au sous-flot. Aucune égalité de
`batch_id`, de `coverage_log` exhaustif ou d'identités v2 n'est conclue.

## 2. Obligation amont : fermeture de fenêtre, pas fermeture de Gamma

Pour une miniball positive $B$, choisissons un support positif
affinement indépendant de cardinal minimal $s(B)\in\lbrace2,3,4\rbrace$.
Notons $p(B)=\lvert X\cap B^{\circ}\rvert$ et $E(B)=X\cap\partial B$.
L'obligation **S** est la suivante :

1. Toute boule telle que $p(B)+s(B)\leq r_{\max}$ possède un candidat
   correct, avec sa vraie géométrie et une arité minimale, après RLE.
2. Aucun candidat ne prétend être une miniball positive s'il ne l'est pas;
   les identités de boules et les comparaisons de niveaux sont exactes.
3. Tout candidat survivant reçoit un census fermé complet. Tout shell
   plus grand que son support minimal provoque un refus dans cette route.
4. L'exécution est terminale : aucune lane, branche, tranche ou sortie
   nécessaire n'a été abandonnée sous un budget en conservant un succès.

Il suffit que chaque boule soit représentée une fois; tous ses supports
alternatifs ne doivent pas survivre à RLE. Trier par arité à clé de boule
égale puis dédupliquer conserve l'arité minimale **parmi les candidats
effectivement reçus** : [candidates.hpp, lignes 28–44](../src/pipeline/candidates.hpp#L28).
L'égalité avec l'arité minimale géométrique dépend donc de S1.

### 2.1 Ce que la WSPD permet de prouver, et ce qu'elle ne suffit pas à prouver

Les produits des deux enfants de chaque nœud interne partitionnent les
paires par leur plus bas ancêtre commun. Une scission d'un facteur
partitionne encore son produit; une lane est soit transmise aux enfants,
soit terminée, soit éliminée par un minorant de témoins. C'est l'induction
combinatoire de [generate.hpp, lignes 359–417](../src/pipeline/generate.hpp#L359).
La séparation sert à regrouper le travail; elle ne remplace pas la
preuve de couverture des supports. La borne linéaire du nombre de
rectangles est une question de coût, pas une prémisse d'exactitude.

Pour un support de taille $q$, la mort demande

$$h_q=r_{\max}-q+1,\qquad\text{minorant certifié de }p(B)\geq h_q.$$

Une boule pertinente vérifie $p(B)<h_q$ : aucun tel certificat ne peut
la supprimer. Les fuseaux ne sont que des sous-ensembles des intérieurs
possibles, et les crédits doivent compter des identités disjointes;
[spindle.hpp, lignes 4–27](../src/spindle/spindle.hpp#L4) et
[generate.hpp, lignes 1294–1335](../src/pipeline/generate.hpp#L1294).
Les faux négatifs des filtres de mort coûtent du temps mais ne perdent
pas une boule. Un faux positif, lui, briserait S1.

La chaîne nécessaire à S1 continue après la WSPD : ancre maximale
canonique, présence de tous les sommets et témoins dans le cover,
au moins un seed aigu pour chaque support q4 bien centré, conservation
de son représentant canonique, puis validité de chaque filtre de
profondeur. Le cover q4 doit utiliser quatre, et non trois :
[generate.hpp, lignes 1307–1314](../src/pipeline/generate.hpp#L1307),
[q4.hpp, lignes 13–26](../src/lanes/q4.hpp#L13).
Le balayage des racines retire les sorties avant de compter les
incidences de shell et ajoute les entrées après; l'ordre inverse
pourrait tuer un support pertinent :
[generate.hpp, lignes 1101–1166](../src/pipeline/generate.hpp#L1101).
Les filtres flottants ne peuvent décider qu'avec leur borne d'erreur,
sinon ils repassent en entier exact :
[float_filter.hpp, lignes 4–29](../src/pipeline/float_filter.hpp#L4).

Le contrôle [run.hpp, lignes 496–509](../src/pipeline/run.hpp#L496)
vérifie la somme des masses de paires. Il ne vérifie pas à lui seul
qu'aucun support d'une ancre vivante n'a été perdu par un filtre aval;
une omission et une duplication de même masse peuvent également être
invisibles à une simple somme. Le census ne peut pas retrouver une boule
que le générateur ne lui a pas fournie. La composition géométrique est
maintenant donnée par [S1](../audits/S1_COURANT.md), avec les
[fuseaux](../audits/FRONT_ET_TEMOINS_COURANT.md), les
[secteurs et cordes](../audits/PREUVE_CHORD_SECTOR_COURANTE.md), les
[cellules](../audits/CELLULES_COURANT.md) et les
[bornes flottantes](../audits/FILTRES_FLOTTANTS_COURANTS.md).
Elle doit encore être liée à la qualification des index, tris, clés,
opérations entières et conditions de compilation/exécution, sans rouvrir
les lemmes géométriques ainsi établis.

### 2.2 Census et catalogue direct sous S

Les bornes séparables de puissance sont des bornes exactes sur les
positions entières des boîtes. La passe de profondeur compte strictement
les intérieurs et ne compte pas le shell :
[census.hpp, lignes 61–82 et 111–149](../src/pipeline/census.hpp#L61).
Le préfiltre utilise $h_q=r_{\max}+1-q$; les survivantes subissent une
passe fermée, dont le compte intérieur est recoupé et dont un débordement
de shell est un refus :
[expand.hpp, lignes 138–147 et 198–233](../src/pipeline/expand.hpp#L138).

Sous S et après succès de la porte des plateaux
[run.hpp, lignes 627–637](../src/pipeline/run.hpp#L627), toute boule
de la fenêtre possède $E(B)=U(B)$, son support essentiel unique.
Une coface Gabriel régulière Q contient alors nécessairement **tous**
les points fermés de sa miniball : $Q=I(B_Q)\cup U(B_Q)$.
Réciproquement, cet ensemble est une coface Gabriel de miniball B.
Par conséquent, la règle d'expansion
$K=p(B)+s(B)-1$ est une bijection vers le catalogue direct complet
de chaque ordre, et non une heuristique d'échantillonnage :
[expand.hpp, lignes 335–350](../src/pipeline/expand.hpp#L335).

## 3. Régularité nécessaire : pourquoi les boules hors fenêtre ne bloquent pas tout

S ne certifie pas la régularité géométrique globale de X. Une boule
supprimée après trop d'intérieurs peut avoir un extra-shell jamais visité.
La fixture
[higher_rank_prune_does_not_certify_star_regularity.json](../../tests/fixtures/regressions/higher_rank_prune_does_not_certify_star_regularity.json)
réfute précisément cette implication.

Elle ne réfute pas l'inertie horizontale. Si $p(B)+s(B)>r_{\max}$, alors
pour chaque ordre demandé $K\leq p(B)+s(B)-2$. Le
[théorème 4.2 transverse](../../docs/math/INCIDENCES_SILENCIEUSES_GAMMA.md#531-inertie-h_0-exacte-des-blocs-saturés-au-dessus-de-la-fenêtre-de-rang)
prouve que les K-facettes **strictes** du saturé
$S_B=X\cap B$ forment déjà un graphe connexe couvrant tous les points de
$S_B$, même avec plusieurs supports positifs ou des extra-shells.
Pour K supérieur à un dans ce régime, ce graphe contient plusieurs
facettes et une coface stricte : son apex antérieur est non trivial.
L'activation du bloc fermé ne crée donc ni racine ni fusion ni point.

Il ne faut toutefois pas remplacer « sans effet en points » par
« toutes ses facettes peuvent être oubliées ». Le raccord utile à la v7
est le lemme suivant.

**Lemme de contact avec le cœur.** Soit $D_K$ l'ensemble global des
facettes des cofaces directes complètes. Après succès de
`build_silent_cofaces`, toute miniball d'une facette de $D_K$ est régulière.
Une boule hors fenêtre irrégulière B, de niveau a, ne peut donc apporter
à ce niveau une première incidence manquante d'une facette de $D_K$.

**Preuve.** Une suppression de sommet essentiel d'une coface directe
est visitée par le constructeur. Il vérifie son shell local puis termine
la requête de bord globale, même après deux intrus; toute égalité
extérieure est refusée :
[silent_incidence.hpp, lignes 194–246 et 278–321](../src/forest/silent_incidence.hpp#L194).
Une suppression d'un point intérieur, non visitée, conserve la miniball
directe régulière et n'a que ce point comme intrus étranger. Elle est donc
régulière aussi.

Considérons maintenant $F\in D_K$ contenu dans $S_B$. Si $\beta(F)=a$,
l'unicité de la miniball impose $B_F=B$; cela contredit la régularité
fraîchement vérifiée de $B_F$ lorsque B est irrégulière. Si
$\beta(F)<a$, F est un sommet du graphe strict connexe de $S_B$.
Ce graphe possède plus d'un sommet; F y est incident à une coface stricte.
Ainsi $\lambda(F)<a$. Dans les deux cas, le bloc irrégulier ne cache pas
une première incidence du cœur au lot a. Les contacts égaux de deux blocs
distincts sont impossibles pour une facette qui naît à a, car ils lui
donneraient deux miniballs différentes; les autres contacts sont stricts.
Le quotient simultané ne contourne donc pas l'apex antérieur. CQFD.

La route reste conservatrice : elle peut refuser une boule de cœur ou
de chaîne irrégulière, même si une stratégie plus fine aurait exploité
son inertie. Ce refus ne démontre pas qu'aucune réduction exacte n'existe.

## 4. Une seule première incidence et une chaîne par facette

Notons $J_F=(X\cap B_F^{\circ})\setminus F$. Sous la régularité de F :

- zéro intrus : tous les minimiseurs de première incidence sont directs;
- un intrus z : l'unique minimiseur est $F\cup\lbrace z\rbrace$, direct;
- au moins deux intrus : tous les minimiseurs sont
  $F\cup\lbrace z\rbrace$, $z\in J_F$, au niveau $\beta(F)$, non-Gabriel.

La preuve des deux premiers cas ne nécessite pas de supposer arbitrairement
la régularité de tous les minimiseurs : un minimiseur non-Gabriel régulier
permettrait de remplacer le point ajouté, essentiel, par un intrus et de
réduire le niveau; un minimiseur irrégulier ne peut être dans la fenêtre
sous S, et hors fenêtre le graphe strict du saturé donnerait déjà une
coface contenant F à plus petit niveau. Les deux possibilités contredisent
la minimalité. Les cas zéro et un sont donc déjà présents dans le
catalogue direct fourni.

Dans le troisième cas, deux minimiseurs $Q_z,Q_w$ ont le même apex
strict : pour un sommet essentiel u de F,

$$R=(F\setminus\lbrace u\rbrace)\cup\lbrace z,w\rbrace,\qquad\beta(R)<\beta(F).$$

Cette coface relie leurs facettes strictes avant le lot. Un seul
minimiseur choisi suffit donc à installer F **s'il est raccordé au bon
apex antérieur**. C'est la condition que ne satisfait pas une simple
émission isolée de ce minimiseur.

La v7 produit ce raccord. Pour chaque coface non-Gabriel régulière $Q_i$
de la chaîne, elle remplace un point essentiel u par un intrus strict w :

$$Q_{i+1}=(Q_i\setminus\lbrace u\rbrace)\cup\lbrace w\rbrace,\qquad\beta(Q_{i+1})<\beta(Q_i).$$

Les deux cofaces partagent $Q_i\setminus\lbrace u\rbrace$.
La diminution est contrôlée exactement et la chaîne ne réussit que sur
un terminal direct du catalogue, de même niveau, ou sur une chaîne déjà
certifiée :
[silent_incidence.hpp, lignes 323–359](../src/forest/silent_incidence.hpp#L323).
Le cache est alimenté après le terminal, jamais avant. Tous les maillons
sont publiés à leur vrai niveau. La descente finie explique la terminaison
mathématique; les plafonds peuvent interrompre le calcul bien avant et
ne sont pas une preuve de longueur pratique.

## 5. Théorème de composition pour le sous-flot effectivement produit

Supposons S, les décisions arithmétiques conformes à leurs bornes u16,
les requêtes et chaînes décrites ci-dessus réussies, puis un fold exact
par niveaux sémantiquement égaux. Soit $A_K$ le catalogue direct augmenté
des seules cofaces de chaînes choisies par la v7. Alors les composantes
incidentes de $A_K$ et les composantes non triviales de Gamma sont en
bijection à toute coupe ouverte ou fermée; la bijection préserve les
points et commute avec les inclusions horizontales. Elle préserve donc
les naissances réduites, $q_R$, les parents abstraits et les multifusions
après suppression des continuations sans changement de composante.

**Preuve par lots, avec invariant explicite.** Avant un niveau a,
maintenir : même activation et mêmes classes sur $D_K$; toute composante
retenue est attachée à un unique apex Gamma; les composantes correspondent
bijectivement et ont la même union de points. Les maillons de toute
chaîne retenue sont des cofaces Gamma : ils ne peuvent créer de connexion
étrangère à Gamma.

Toute composante non triviale de Gamma contient une coface directe.
En effet, suivre sa généalogie jusqu'à sa première naissance non triviale.
Une coface non-Gabriel régulière possède déjà un apex non trivial strict;
un bloc irrégulier hors fenêtre en possède un aussi par le théorème 4.2.
Ni l'un ni l'autre ne peut être la première naissance. Cette dernière
contient donc une coface directe, et ses facettes du cœur persistent.
Cet argument évite d'exiger une procédure top-K non implémentée pour
trouver un ancrage.

Au lot a, les facettes du cœur qui avaient une première incidence stricte
sont déjà installées par l'invariant. Celles dont la première incidence
vaut a sont installées soit par une coface directe, soit par le minimiseur
choisi et sa chaîne vers un terminal strict. Ce terminal appartient au bon
apex Gamma; l'hypothèse d'induction identifie déjà sa racine retenue.
Tous les co-minimiseurs de la même facette rejoignent cet apex par la
confluence du § 4, y compris si le lot contient plusieurs facettes nouvelles.

Les cofaces non-Gabriel régulières omises, contactant une facette égale,
partagent son apex par confluence; un contact avec une coface directe
est strict, sinon les deux cofaces auraient la même miniball mais la
coface directe omettrait un intrus. Une partie omise n'ajoute donc aucun
parent aux parties retenues. Les blocs irréguliers hors fenêtre ne
changent ni le cœur ni les points par le lemme du § 3. Toute partie
purement omise est une continuation sur un apex déjà présent.

Chaque maillon retenu d'une chaîne possède son propre raccord strict,
même si d'autres facettes de ce maillon sont hors cœur et ne sont pas
encore matérialisées dans le sous-flot. C'est pourquoi une facette hors
cœur ne doit pas être résolue artificiellement à l'aide de Gamma dans
le lecteur de payload : le raccord existe dans les cofaces retenues
antérieures. Les maillons n'ajoutent aucun point absent de leur apex.
Les cofaces directes sont présentes dans les deux constructions et
ajoutent exactement les mêmes points aux mêmes groupes. Après contraction
atomique du lot, l'invariant est rétabli. Les niveaux sont en nombre fini;
l'induction donne les coupes strictes et fermées ainsi que leur évolution.
CQFD.

Cette preuve est une spécialisation du
[corollaire 4.1 et du théorème de fenêtre](../../docs/math/INCIDENCES_SILENCIEUSES_GAMMA.md#53-réduction-conditionnelle-aux-premières-incidences-du-cœur),
avec deux raccords explicites propres à la v7 : choix d'un seul
co-minimiseur via sa chaîne matérialisée, et traitement des contacts des
blocs irréguliers hors fenêtre. Elle n'importe aucun reçu d'une autre lignée.

## 6. Du sous-flot au vrai payload : invariant du fold normalisé

Le tri compare les fractions exactes et les lots regroupent les niveaux
égaux sémantiquement, pas leurs octets :
[fold.hpp, lignes 398 et 507–515](../src/forest/fold.hpp#L398).
Pour K supérieur à un, `seen(root)` est vrai avant le lot exactement
quand sa composante a déjà rencontré une coface. Une racine encore
latente ne fournit pas de parent, même si sa facette est géométriquement
active. Les parents sont figés avant toute union :
[fold.hpp, lignes 1042–1055](../src/forest/fold.hpp#L1042).

Les unions joignent toutes les facettes de chaque événement, puis
regroupent les parents par composante post-lot. Chaque facette touchée
qui n'a jamais été matérialisée est ajoutée à `born`, et `seen` n'est
mis à jour qu'après le lot :
[fold.hpp, lignes 1059–1142](../src/forest/fold.hpp#L1059).
L'induction du DSU porte donc sur l'hypergraphe réellement retenu,
indépendamment de Gamma. Une naissance a zéro parent, une continuation
un parent, une multifusion au moins deux. Une continuation n'est omise
que si `born` est vide; dans ce cas son ensemble de facettes et sa clé
canonique ne changent pas. Ses références futures restent résolubles.

Le lecteur indépendant
[compare_delta_cuts](../tests/silent_incidence_gate.cpp#L253) reconstruit
les seuls deltas classic/CSR, consomme les jetons de parents vivants,
vérifie la première matérialisation et le canonique `output`, puis
compare les coupes à Gamma. Les corruptions ciblées de `born`, d'attache,
de niveau et d'`output` sont des portes permanentes. Elles constituent
une falsification causale bornée de cet invariant, pas une preuve
exhaustive pour tous les nuages u16.

À K=1, les points d'entrée sont des racines normatives initiales, et non
des racines à découvrir dans `born`. Le fold maintient explicitement
cette exception : [fold.hpp, ligne 846](../src/forest/fold.hpp#L846).
Sous S, toutes les arêtes Gabriel sont présentes; les faits 1–2 du
manuscrit, pages 16–17, PDF 42–43, donnent l'équivalence aux composantes
du graphe complet par inclusion d'un EMST. Cela ne transporte pas les
identités ou la liste complète des arêtes du graphe complet.

## 7. État des obligations et prochain verrou utile

| Obligation | État justifié | Ce qui ne la remplace pas |
|---|---|---|
| Gamma élémentaire et retrait des isolés | Théorèmes mathématiques, objet explicitement réduit | Suppression Gabriel brute |
| Partition des paires WSPD | Induction LCA/scissions; contrôle de masse dans le code | Fermeture des supports q3/q4 |
| Fermeture S1 des boules de fenêtre | Théorème géométrique conditionnel indépendant jusqu'au RLE; qualification des primitives et du domaine d'exécution à compléter | Accord v6/v7, masse totale, census des seuls survivants |
| Census et refus des shells pertinents | Requêtes fermées exactes sur les candidats reçus; refus explicite | Régularité géométrique globale |
| Inertie des boules haut rang | Théorème 4.2, même irrégulières | Autorisation d'omettre une attache de cœur sans resolver |
| Première incidence du cœur et chaîne | Preuve des §§ 3–5, contrôles locaux/terminaux et oracles bornés | Énumération de toutes les facettes Gamma |
| Fold normalisé et reconstructibilité | Invariant DSU ci-dessus, lecteurs/mutants classic et CSR | Seuls compteurs d'invariants ou seules cofaces |
| Verticale, identités publiques normalisées, reprise, 50k et massif | Hors conclusion, obligations séparées | Exactitude horizontale conditionnelle ou benchmark achevé |

Le prochain verrou utile est donc **la qualification des primitives et
du domaine numérique de S sur les octets exécutés**, pas l'énumération
de Gamma. Les sorties de prune ont maintenant leur composition : univers
partitionné, owner et seed canonique préservés, témoins et seuils stricts,
terminaison et représentant minimal après RLE. Il reste à relier leurs
hypothèses aux bornes de largeur, index, tris, PGCD, Cramer, produits
larges et conditions réelles binaire64/FMA, puis au census et à l'expansion.
Un reçu terminal peut rester sensible à la taille de sortie et vérifier
des prunes de blocs; il n'a pas à matérialiser les cofaces combinatoires
de tous les ordres. Les hashes seuls ne remplacent pas ce dossier.

Les contre-fixtures à conserver sont E5 (attache silencieuse), le triangle
aigu (latent ne signifie pas parent), les arcs liés (sortie quadratique),
la courbe des moments avec la facette `048` (un cœur suffisant pour H0
n'est pas un catalogue facetté exhaustif), et la boule haut rang avec
extra-shell (inertie ne signifie pas régularité). Une campagne de grande
taille qui refuse n'est pas une réfutation de ce théorème conditionnel;
elle montre que la route n'a pas satisfait ses prémisses de terminaison
ou de domaine pour cette entrée.

Enfin, le statut API et sa chaîne de diagnostic ne sont pas interchangeables.
[run.hpp, lignes 784–795](../src/pipeline/run.hpp#L784) pose correctement
le statut typé d'un refus de complétion, mais son message commence par
`silent incidence K=...`, sans préfixe de classe. Le banc antérieur pouvait
alors enregistrer un résultat `invalid` au niveau protocole au lieu
d'`engine_refused`; il ne le comptait pas comme un succès. Le
[correctif du banc](../receipts/incidence_runner_20260904/README.md)
reconnaît maintenant une liste fermée de motifs et leurs diagnostics,
avec rejets adverses et un refus réel par plafond MEB. Les anciens octets
et reçus restent conservés ; ils ne sont pas réinterprétés rétroactivement.
Cette classification ne modifie pas la preuve ni le statut API. Un retour
non complet invalide les payloads provisoires, et des callbacks antérieurs
restent provisoires jusqu'au succès global.

**GCP non utilisé pour cet audit documentaire. Aucun statut promu.**
