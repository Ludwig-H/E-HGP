# Classification et parcours des verticales

Tranche du 2 octobre 2026, après la première campagne FULL àc6ca345e0.
Implémentation en préparation, qualification native distincte requise.
Le contrat200ms reste ouvert. Les petites comparaisons v10 ne remplacent
pas le différentiel sur les trames LiDAR entières.

## Coûts observés qui motivent cette tranche

Sur08/0 sans sol, K1..5/u21, le premier FULL prend21,294s :4,408s domaine,
16,885s forêts et verticales. Le ledger donne235254420 marches de parents
pour2319956 requêtes verticales, dont145615556 marches àK5. Il donne aussi
63994685 présentations MEB des parties, mais ne sépare pas les temps de
classification, de rejeu et des verticales. Ces nombres établissent un
travail à éviter ; ils n'attribuent pas les16,885s à une seule cause.

## Classifier sans construire les traces

Le classificateur privé vérifie la même fenêtre critique et le même domaine
combinatoire que `build_cell`. Il ne réserve aucun Buffer et ne fabrique
aucune partie I∪A. Avec t=k−p et m=|U| :

- t=m donne une naissance puisque U contient le support global S* ;
- t<qmin donne des traces strictes puisque aucun A de taille t ne contient
  le centre dans son enveloppe convexe ;
- sinon, les t-parties A sont parcourues dans l'ordre lexicographique
  jusqu'au premier témoin β(A)<λ. Sans témoin, le parcours doit être exhaustif.

La décision emploie qmin global, indépendamment du premier site de U et du
support initialement fourni. La branche t=m ne demande pas de MEB sur toute
une coquille ; une coquille de plus de12 avec t=m est cependant hors du
domaine natif K≤12. La faisabilité affine exacte du modèle est indépendante
des formules MEB du produit.

La classification ne compte ni composantes locales ni incidences. Le rejeu
conserve `build_cell` exhaustif, toutes les traces et leur niveau initial
strictement inférieur au plateau. Pour la cellule centrale `octa_center`,
la classification passe symboliquement de30 tests à1 ; le rejeu conserve30.
Les deux buffers précédents ne coexistaient pas : aucun faux gain de pic
de1248 à624octets n'en est déduit.

`ClassificationLedger` publie séparément univers combinatoire, parties
examinées et travail MEB réel. `CellLedger` couvre désormais le rejeu.
Les anciens reçus conservent leur schéma et leurs compteurs agrégés.

## Remontées fermées par activation des fusions

La forêt inférieure possède déjà deux séquences ordonnées par niveau :
ses naissances, puis ses nœuds de fusion. Le balayage active chaque fusion
inférieure dès que son niveau est inférieur ou égal à la coupe demandée.
Il réunit le nœud et chacun de ses enfants dans un DSU avec union par taille
et compression des chemins. Le sommet géométrique courant est stocké
séparément du représentant DSU : changer la racine technique ne change
jamais la réponse géométrique.

Les requêtes supérieures sont traitées dans l'ordre obtenu en fusionnant
leurs séquences de naissances et de fusions, sans tableau de tri supplémentaire.
Chaque enfant a un niveau strictement inférieur à celui de son parent ;
son image verticale existe donc avant le traitement du parent. Toutes les
fusions inférieures d'un même niveau sont actives avant une requête à ce
niveau. La coupe demeure fermée. Chaque enfant supérieur est contrôlé,
y compris les images déjà égales ; aucune sélection d'un seul enfant.

La structure réserve trois Buffer u32, soit12N octets pour N nœuds de la
forêt inférieure. Ils coexistent avec les sorties déjà retenues et le
tableau des verticales en construction, et sont rendus après cet ordre.
Les requêtes et activations coûtent amorti O((N+Q)α(N)), en plus du travail
inchangé des descentes et des autres étapes FULL. Cette borne locale ne
borne pas le nombre de cellules, traces ou événements géométriques.
`ancestor_closed` conserve sa marche de référence pour les portes.

Les quatre compteurs nouveaux distinguent requêtes, fusions activées,
unions et pas de recherche DSU. Une activation peut faire plusieurs unions,
puisque le DSU contient aussi le nœud de fusion : leur borne est le nombre
d'arêtes de la forêt inférieure, pas son seul nombre de naissances.

## Diagnostic et qualification

`FullTimings*`, facultatif, expose quatre temps par ordre : classification,
préparation des naissances, plateaux et verticales. Sans pointeur, aucune
horloge interne n'est consultée. Le brouillon entier n'est publié qu'après
succès de FULL ; ces intervalles disjoints ne couvrent pas tout le temps mur.
Le banc utilise `full_campaign.v3` et `full_work.v2` ; les captures c6/v1
et cache-tri/v2 restent attachées à leurs lecteurs figés.

Les portes confrontent classification au modèle affine, balayage à toutes
les marches de référence et FULL à Definition. Elles couvrent égalités de
plateaux, multifusions, requêtes sur nœuds internes, refus de mémoire,
absence d'allocation du classificateur, publication des diagnostics et
mutants causaux. Les mesures précédentes ne qualifient pas ces changements.
