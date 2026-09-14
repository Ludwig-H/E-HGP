# Census q2 conjoint : partager les ancres avant leur éclatement

14 septembre 2026, onzième tranche ouverte après e3af11a7.
`exploration_v8_hors_registre`, `cpu_reference`, `quantized_u16_input_only`,
`implementation_v8_p0`, `not_claimed`. GCP non utilisé.

## Objet et décision

`Q2AnchorMode::SharedProduct` conserve les deux facteurs A et B d'un
rectangle WSPD au lieu de commencer immédiatement une recherche par
ancre. Il est réservé au census SharedBlocks. Individual reste le défaut.
`SharedAnchors` est le bras qui ne divise qu'A avant le relais singleton.
Les deux ordres GlobalDfs et ComplementFirst restent comparables, avec
ou sans certificat frère. Le front, les candidates et les supports
complets demandés ne changent pas.

Le compte est commun à toutes les paires A×B sur le préfixe de témoins
déjà résolu. Pour une boîte Z, les
[extrema exacts H de l'auditeur A](../audits/P0_SOUS_RECTANGLES_ET_GROUPES.md#9-census-q2--des-extrema-exacts-pour-partager-les-recherches)
permettent trois actions : minimum strictement positif, créditer sa
population ; maximum non positif, résoudre cette population à zéro ;
sinon raffiner. La saturation à K rejette le sous-produit entier.
Une fin de parcours sous K permettrait son admission, mais la politique
actuelle passe en pratique par le relais singleton décrit plus bas.
La collecte des intérieurs et de toute la coquille reste payée pour
chacun des supports.

Les 96 octets de constantes préparées décrivent les quatre couples de
bornes A/B par coordonnée. Ils ne transportent aucun crédit ni témoin.
Les carrés et différences sont calculés en i64 après promotion ; les
centres doublés et carrés de différences des extrémités tiennent en u32.
Le minimum sur Z est atteint à une borne, tandis que le maximum peut
être au sommet intérieur de la parabole : ne pas tester seulement les
coins pour ce maximum. Les boîtes continues donnent des certificats
sûrs sur leurs sites ; une indécision n'est jamais un rejet.

## Continuation et reprise à une ancre

Une tâche garde A, B, compte, curseur, phase, B original et son échappement.
Global suit le DFS habituel. Complement reporte B original, pas le B
courant : ses ancêtres propres sont divisés avant toute consommation et
sa population est visitée dans la seconde phase, terminée à son escape.

Tant qu'A est un groupe, **aucun point de A n'est exclu**. Un autre point
de A peut être intérieur à la paire d'une ancre donnée. Quand A devient
singleton, la tâche existante reprend compte/curseur/phase sans nouvelle
racine. L'exclusion de cette seule ancre dans Complement ne retire qu'une
contribution connue nulle, qu'elle soit déjà résolue ou encore à visiter.
Un bloc crédité uniformément ne pouvait pas contenir cette ancre car
H(a,b,a)=0. La collecte finale ne change pas.

Pour une indécision, la première politique divise Z si sa diagonale est
strictement plus grande que celles de A et B ; sinon elle divise le plus
large des deux facteurs de requête, égalité vers A. Les deux enfants
héritent du même préfixe et du même compte. Cette politique est à mesurer,
pas un théorème d'optimalité. Le bras `SharedAnchors` garde le même test
de diagonale Z, mais divise toujours A quand il faut raffiner la requête.
Il préserve ainsi B original jusqu'au relais ; chaque ancre peut être
transmise au plus une fois par rectangle. Le bras équilibré peut au
contraire transmettre plusieurs sous-groupes B pour la même ancre,
ce qui risque de fragmenter le travail et de retarder les tests frère.
La comparaison compte toutes ces reprises, pas seulement les racines.
Le certificat frère reste limité aux
subdivisions B du chemin à ancre fixe : aucun crédit partiel du frère
ou certificat conjoint supplémentaire n'est ajouté dans cette tranche.

Le compteur d'admission conjointe et le changement de phase conjoint
restent nuls dans les fixtures qualifiées. Ce n'est pas un exercice
de branche manquant à masquer : tant qu'A contient deux sites distincts,
le bloc Z contenant A ne peut être résolu uniformément. Son minimum
est au plus zéro (z=a), son maximum continu est strictement positif
sur un segment de la boîte A. Arrivé au nœud Z=A, le test strict de
diagonale impose une subdivision de requête plutôt qu'une descente Z.
Le relais singleton survient donc avant la consommation de tout A,
et avant la seconde phase. Ces compteurs restent exposés sans plancher
artificiel ; ni l'admission ni la seconde phase conjointe ne sont
revendiquées comme branches positivement exercées.
L'[audit A publié à 7e315009](../audits/q2_product_20260914/README.md)
prouve plus précisément que le préfixe reste disjoint d'A courant.
Il propose de descendre Z au nœud A et de supprimer les tests dont
l'indécision est connue. Ce sont deux expériences distinctes ; ni
l'une ni l'autre n'est intégrée aux sources gelées de cette tranche.
Son modèle de file FIFO/LIFO ne qualifie pas une file produit C++.

## Coûts et distribution future

`joint_work` sépare racines, tâches, divisions A/B/Z, bornes, mouvements
structurels, consommations, événements de crédit, reprises singleton
après crédit et masses rejetées/admises/transmises. Ces trois masses
partitionnent les candidates. `credited_pair_mass` est cumulatif par
événement et peut dépasser les candidates. La profondeur maximale
concerne la récursion conjointe, pas toute la pile ni son empreinte.

Les compteurs géométriques génériques conservent le travail de la phase
à ancre fixe, plus la collecte. Les masses globales et `uniform_*`
peuvent aussi inclure des décisions conjointes : ne pas les ajouter
une seconde fois. En mode conjoint, `count_root_starts` vaut le nombre
de rectangles et `query_tasks` vaut les reprises singleton plus deux
fois les splits B génériques. `anchor_queries` reste la somme descriptive
des petits facteurs ; ce n'est plus le nombre de recherches démarrées.
Le schéma v4 expose ces différences sans requalifier les anciens reçus.

Un test conjoint paie douze couples de constantes contre six à ancre
fixe : ne pas comparer leurs nombres comme des instructions équivalentes.
Mesurer aussi tâches, propositions frère, structure, front, collecte,
capacités retenues et temps englobant. Aucun tableau de paires, de
témoins ou de frontières n'est ajouté ; les handles désignent le même
index immuable. La récursion actuelle est synchrone ; une future file
doit posséder sa continuation complète et ses tampons de sortie.

Le nombre de sous-produits peut encore croître presque au carré. Le
partage de préfixes ne démontre donc pas une borne globale. L'épreuve
reste n8k/16k/32k, particulièrement les amas, puis le coût de toute
la chaîne et non celui du seul composant. q3/q4, FULL et GPU restent ouverts.

## Qualification et campagne

Deux gates nouvelles : bornes conjointes face à un oracle indépendant,
puis supports/clé/intérieurs/coquilles face au census scalaire exhaustif.
Fixtures de frontière, retrait erroné de tout A, crédit hérité,
continuation des deux ordres, transformations et exceptions. La fixture A={(100,0,0),(100,4,0)},
B={(0,1,0),(0,2,0),(0,3,0)}, z=(50,2,0) vérifie le témoin uniforme
Hmin=2498 ; à K2 les profondeurs cross [1,2,3] et [3,2,1] laissent
deux admissions, perdues si z était compté deux fois après la reprise.
La [capture initiale](../receipts/q2_joint_20260914/README.md) conserve
36 mesures8k et la qualification Release47 PASS. La tentative Clang
échoue sur un test historique de collecte à l'interruption, sans défaut
géométrique signalé. Ce défaut du runner est corrigé séparément :
les signaux levant une exception sont différés pendant Popen et chaque
lecture, puis rejoués hors collecte ; une attente de 50 ms permet leur
traitement sans plafonner la durée du benchmark. L'arrêt TERM/KILL
conserve les flux et la première interruption même si une seconde
survient pendant l'annulation. Six nouveaux contrôles déterministes
complètent les six contrôles de lancement, en Python normal et −O. Les
sources C++ restent inchangées. Les builds initiaux sont épinglés,
nouvelle qualification dans v8_joint_r2_20260914 et
v8_joint_sanitize_r2_20260914. Aucun résultat FULL/G4 n'en découle.
La [clôture r2](../receipts/q2_joint_r2_20260914/README.md) conserve
47 CTests Release/Clang ASan/UBSan réussis, 27 commandes par qualification,
lecteurs normal/−O identiques et 44 mesures. Les deux builds r2 sont
désormais épinglés. Le bras équilibré régresse ; A seul ne ferme pas
la croissance des amas : visites après relais ×4,107/×4,231 à
8k/16k/32k. P0 reste ouvert. Les coûts des bornes conjointes, tâches,
structure, certificats frère et collecte sont publiés séparément.

## Priorité suivante

Les crédits locaux Pool sur les gros rectangles terminaux évitent de
laisser systématiquement leurs produits entiers au census. La
[proposition de raccord](P0_POOL_TERMINAL_RACCORD.md) reprend la piste
de B avec propriétaire/index globaux, rangs et IDs explicites, compte
census nul et sortie complète. Ce raccord n'est pas encore implémenté ;
il doit payer préparation, résidu et census ensemble. Les titres et
temps de composant de l'audit ne ferment ni P0 ni la chaîne q2.
