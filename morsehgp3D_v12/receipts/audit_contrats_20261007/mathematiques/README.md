# Contre-lecture mathématique indépendante avant T2

7 octobre 2026. Cadre : `phase=exploration_v12_hors_registre`, `backend=cpu_reference`,
`profile=quantized_u21_input_only`, `public_status=not_claimed`. GCP non utilisé.
Pin initial lu : `13c52bc602a4e7ea90964ee23e155a8ad2cfd301`. Lecture complémentaire, par
`git show` : contre-lecture Claude et registre `fc1f913ce`, contrat numérique
`e264de6f2`, réponse et correctifs du développeur `a0e31abfe`. Aucun moteur v12
exécuté, aucune modification produit, aucun commit.

Conclusion : les preuves originales de **LEM-T1, T3, T4, T5, T6 et T7** sont
acceptables avec les hypothèses explicites ci-dessous. Le résumé initial v12 de
T1 était faux sans `S ⊆ F` ; `a0e31abfe` le corrige. Les autres réserves portent
sur le domaine des énoncés, les
représentations à conserver et la portée des coûts, sans contre-exemple nouveau
aux théorèmes correctement formulés. Cela autorise leur traduction en contrats
et portes ; cela ne qualifie pas une future implantation ni ses performances.

La note Claude `audits/AUDIT_CONTRE_LECTURE_LEMMES_T_20261007.md` et ses
`CST-0101` à `CST-0107` ont été confrontés à cette lecture : accord. Ce reçu ne
crée pas de deuxième registre. Il apporte des modèles exécutables indépendants,
des précisions de port et le constat sur les empreintes ci-dessous.

**Réponse `a0e31abfe`, relue après les expériences.** Les corrections de T1
dans le contrat et l'architecture sont suffisantes sur le plan documentaire.
La fixture permanente `reference/fixtures/wit_t1_carre.json` et sa porte
`reference/test_witness_t1.py` ont été relues : deux diagonales supports,
huit couples support–partie, contre-exemple du côté, échec de recherche de la
diagonale non canonique. L'oracle énumère les sphères englobantes et minimise
leur rayon exact ; la positivité barycentrique n'est pas requise pour chaque
candidat car le vrai MEB est inclus et aucun candidat englobant ne peut avoir
un rayon plus petit. Le parent rejoue cette porte et son mutant ; ce reçu ne
revendique pas son exécution. La correction rédactionnelle et la gravure du
témoin peuvent être reconnues sans prétendre contrôler le futur chemin natif.
Les reformulations de T3–T7 répondent à `CST-0102`–`CST-0107` ; les précisions
de coût et de branche étendue ci-dessous restent utiles au port. Les lignes
citées plus bas décrivent le pin initial, sauf indication contraire.

## Sources et méthode

Dans la suite, **CT** désigne
`morsehgp3D_v11/receipts/conception_v11_20261002/conception/CONCEPTION_TOUR.md`.
Ses lignes 718–772 contiennent les preuves originales, et ses lignes 139–169,
201–250, 311–359 et 399–411 les algorithmes auxquels elles s'appliquent.
Le contrat v12 jugé est `docs/OBJET_ET_CONTRAT_MATHEMATIQUE.md:85` et
`docs/ARCHITECTURE.md:74`. Les empreintes des sources effectivement utilisées
sont dans `sources.json`. Les numéros de ligne du contrat numérique sont ceux
du blob `e264de6f2`, les autres ceux du pin initial.

`witnesses.py` n'importe aucun oracle géométrique du dépôt. Sa seule importation
de production est le lecteur strict v11 `bench/full_semantic.py`, pour tester
son empreinte réelle sur un flux synthétique valide. Les tests géométriques de
T7 utilisent des entiers Python et des fractions exactes ; la séparabilité de
référence est obtenue par résolution barycentrique exhaustive indépendante.
Les tests T4–T6 sont **hypergraphiques abstraits**, sans prétendre réaliser
géométriquement toutes les permutations testées.

Rejeu, depuis la racine du worktree :

```sh
python3 morsehgp3D_v12/receipts/audit_contrats_20261007/mathematiques/witnesses.py
python3 -O morsehgp3D_v12/receipts/audit_contrats_20261007/mathematiques/witnesses.py
```

Les deux modes produisent des JSON identiques : 720 ordres de traitement d'un
plateau, 23 040 requêtes d'historique, 240 cas où une jonction doit remonter vers
une fusion ultérieure au même rang ; 7 coquilles et 36 ordres locaux comparés
à la définition brute ; 5 translations du triangle équilatéral. Aucun `assert`
n'assure ces contrôles. Le journal `attempts.txt` conserve une erreur initiale
de construction d'une fixture Python, corrigée avant les captures finales.

## T1 : certificat combinatoire, réserve majeure déjà CST-0101

CT:722 prouve exactement : si `S` est un vrai support de `b` et
`S ⊆ F ⊆ P_b`, alors `B(F)=b`. On peut même omettre la canonicité de `S` du
théorème : `B(S)=b`, et l'encadrement par inclusion force le rayon minimal de
`F` à celui de `b` ; l'unicité de la plus petite boule conclut. Le moteur peut
choisir de ne reconnaître que `S*` pour sa table de recherche.

Le témoin indépendant est collinéaire : `X={0,1,2}`, `K=2`, support
`S={0,2}`, `b=(centre 1, niveau 1)`, `p=1`, `q=2`, donc boule admise au
catalogue `p+q=K+1`. La partie `F={0,1}` est entièrement dans `P_b`, mais son
niveau minimal est `1/4`. Le test manquant peut donc produire un faux positif
dans le domaine déclaré, pas seulement sur une entrée artificiellement hors
contrat. La proposition flottante ou une marche depuis une boule précédente
doit être suivie d'un contrôle exact `S ⊆ F` (au plus quatre identifiants), ou
provenir d'une interface qui garantit et fait vérifier ce prérequis.

La preuve n'exige pas un catalogue complet pour **ce** succès : support exact
et absence de faux sites dans l'enregistrement suffisent. La complétude et le
census complet restent requis ailleurs pour reconstruire FULL. Un support
non canonique sur une coquille étendue doit provoquer le repli prévu, pas être
rejeté comme géométriquement invalide. Références v12 : contrat:90,
architecture:78–82 ; CT:201–250, 720–726.

## T3 : mémo de cellule et dates d'usage

Accepté dans le domaine de CT:734–738. Le représentant initial d'une jonction
de niveau `λ` doit être **strict** : `β(F₀)<λ`. Toute descente exacte diminue
`β`; sa cellule d'arrêt `b'` vérifie donc `λ(b')<λ`. Le transfert vers un
représentant de cette cellule s'effectue dans sa composante fermée au niveau
`λ(b')`. Cette composante est déjà présente strictement avant la jonction
source. Les pointeurs entre cellules descendent en rang, ce qui donne la
terminaison sur le catalogue fini.

Deux contrats distincts doivent rester visibles : le pointeur de cellule est
utilisable à partir du niveau de cette cellule ; la cible terminale obtenue
pour une partie arbitraire `F₀` est valable à partir de **β(F₀)**. Elle n'est
pas valable depuis sa propre naissance. `WIT-MEMO` et `WIT-D2` sont donc les
portes appropriées (`CST-0104`). Le rang catalogue immédiatement précédent la
jonction ne borne pas la date d'une trace : CT:738 ne l'affirme pas.

Un pointeur mathématiquement acyclique ne prouve pas la sûreté d'une lecture
concurrente. Le recouvrement G/T prévu devra publier une cible achevée avant sa
consommation et distinguer absence, refus et cible valide. C'est une obligation
de port, pas une objection au lemme. Aucun nouveau témoin de descente n'a été
exécuté ici ; les preuves et témoins originaux ont été relus.

## T4 : événements binaires et multifusions

Accepté par la récurrence autonome de CT:742–746, sous les ponts explicités par
`CST-0102` : catalogue complet par niveau, tous les morceaux représentés,
cibles correctes de T3 ; jonctions faibles, étendues et cellules inertes de
fenêtre conservées. Les représentants d'une cellule touchent toutes les
composantes strictes pertinentes. L'union-find fusionne exactement celles que
le plateau relie.

Chaque union réussie doit écrire ses **sommets courants**, et non les racines
numériques de l'union-find. Les événements de même rang se contractent par
**composantes connexes** de la relation producteur–consommateur, non par
composantes fortement connexes. Une jonction redondante ne crée aucun événement,
mais garde son `jtop`. Les plateaux géants sont couverts par la preuve, même si
les plateaux mesurés sur LiDAR étaient petits. `REG:243` est remplacée dans la
portée indiquée par `CST-0103`, pas prouvée littéralement.

Le modèle exécuté a huit naissances et trois rangs, avec six jonctions au rang
central. Les 720 permutations de ces six jonctions produisent exactement les
mêmes multifusions **et les mêmes ensembles d'enfants**, comparés à un graphe
par lots indépendant. Des jonctions inertes et redondantes sont incluses.
Cela contrôle le raisonnement combinatoire ; cela ne teste ni un catalogue
natif ni le raccord géométrique des graines.

## T5 : historique d'attache

Accepté avec `rang(naissance)≤coupe` (`CST-0105`). L'union par taille fait au
moins doubler la taille de la composante à chaque **attache historique** d'une
racine perdante. Le chemin d'attache a donc au plus `⌊log₂(n_b)⌋` arêtes et des
rangs croissants au sens large. Ce résultat ne borne pas la profondeur de
l'arbre de fusion, qui peut être linéaire.

Trois objets doivent rester distincts : pointeurs compressibles du DSU,
attaches historiques immuables avec leur date, minimum canonique de naissance.
Remplacer l'union par taille par « le plus petit identifiant gagne » détruit la
preuve de profondeur ; conserver le minimum dans un champ séparé la préserve
(CT:323–331). Les événements par survivant restent dans l'ordre chronologique,
y compris aux rangs égaux. `component_at` cherche le dernier événement de rang
au plus la coupe et le traduit par le quotient de T4.

Les 23 040 requêtes exactes du modèle concordent avec les composantes fermées
du graphe par lots ; profondeur maximale observée 2 pour huit naissances.
Ce chiffre est un contrôle borné ; la borne générale vient du doublement.

## T6 : verticales après contraction

Accepté, CT:401–411 et 754–758. Il faut d'abord traduire `jtop` en son **nœud
final contracté**. Si ce nœud a déjà le rang de la jonction, toute fusion
ultérieure du même plateau liée à lui appartient au même nœud. Sinon, son
parent au même rang est précisément l'image fermée recherchée. Une seule
remontée suffit parce que les niveaux des nœuds emboîtés sont strictement
croissants après contraction. Une remontée unique dans l'arbre **binaire brut**
n'aurait pas cette propriété.

Les 240 cas de reprise tardive de `jtop` du modèle contrôlent explicitement la
branche « jonction sans nouvelle union, puis absorption au même rang ».
Pour une naissance étendue, la cellule du même `b` à l'ordre inférieur peut
être une **naissance**, et non une jonction (CT:407). Le carré en fournit un
exemple exact : ses cellules d'ordres 3 et 4 naissent toutes deux au niveau du
cercle circonscrit. Les résultats T7 le retrouvent. Il faut conserver cette
branche et, pour l'image d'une fusion, choisir une naissance sous cette image
inférieure ; la formule explicative « premier représentant de la jonction »
de CT:409 ne suffit pas littéralement lorsqu'il n'existe aucune telle jonction.
Il s'agit d'une précision de port du choix `L(v)`, pas d'une réfutation de T6.

La naturalité reste à contrôler sur **tous** les enfants d'une fusion ; les
modèles abstraits ne qualifient pas les verticales géométriques multi-ordres.

## T7 : quotient exact, maximalité et coût

Accepté, CT:139–169 et 760–772. Hypothèses : coquille finie, sites distincts,
rayon strictement positif commun, centre fixé ; le cas collinéaire critique
est une paire antipodale. La séparation stricte équivaut à l'exclusion du
centre de l'enveloppe convexe. Les fenêtres semi-ouvertes se réalisent par une
petite perturbation dans l'ensemble **fini** : leur origine est incluse, son
antipode exclu. Les plans par le centre ne doivent être traités qu'une fois.

La famille produite contient tous les séparables maximaux mais n'est pas
réduite à eux (`CST-0106`). Notre tétraèdre produit déjà dix masques, dont six
non maximaux ; le cercle exact proposé par Claude en produit cinq, dont deux
non maximaux. Aucun filtrage de maximalité n'est nécessaire à la preuve.
Deux masques actifs partagent une `t`-partie exactement quand leur intersection
a au moins `t` sites. Les `t`-parties d'un même masque sont reliées par échanges
successifs ; réciproquement toute adjacency brute est couverte par un masque.
C'est la bijection des composantes, et non l'identité des graphes.

Le juge indépendant énumère les supports convexes contenant le centre par
systèmes rationnels de taille au plus quatre (Carathéodory), puis toutes les
parties de chaque petite coquille. Il contrôle la séparabilité et la couverture
de chaque masque, les composantes pour **chaque ordre** et le complément local
des sites présents dans une partie stricte. Antipodes, carré, tétraèdre,
octaèdre, cube, cercle et plans partageant un axe : 36 ordres, zéro écart.
La contribution complémentaire est exacte pour l'union de couverture locale ;
elle ne compte pas des sites mondialement nouveaux (`CST-0106`).

**Précision de coût, complément au registre existant.** L'énoncé v12:95 écrit
correctement `O(m³) prédicats` si « prédicats » signifie les prédicats
géométriques construisant les masques. Il ne faut pas en déduire un quotient
complet cubique. CT:167 donne ensuite `O(|S_t|²)` intersections par ordre,
donc jusqu'à `O(Km⁴)` opérations de masques ; CT:168 budgète explicitement
`Σ(m³+K m⁴/2)`. Le modèle un masque = un mot et le plafond `m≤64` font partie
de ce coût ; une extension au-delà doit requalifier stockage et coût des
intersections. À rappeler au contrat de ressources, sans présenter une borne
de travail globale de FULL.

## WIT-TRANSL : ordre canonique et empreintes

L'ordre lexicographique des coordonnées exactes est invariant sous une
translation commune légale : la première différence non nulle de coordonnées
est inchangée. Cela vaut aussi pour les centres rationnels. Un `S*` défini par
cardinalité minimale puis coordonnées lexicographiques et un Kruskal dont tous
les départages suivent cet ordre sont donc équivariants. Les identifiants de
travail et l'ordre de Morton restent internes. La numérotation des naissances
doit être celle du contrat v12, niveau puis centre exact, avant de calculer
« plus petite naissance » (`CST-0107`), et non CT:137/346 repris textuellement.

Le triangle `(0,1,1),(1,0,1),(1,1,0)` est testé sous cinq translations entières.
Les trois boules diamétrales sont vides et de niveau `1/2`. Le départage Morton
retient successivement trois arbres distincts, alors que le départage lex garde
toujours `{01,02}` en identifiants d'entrée. C'est un modèle exact du sélecteur,
pas une exécution de `spanning` natif. Changer ce départage corrige la sortie
squelette ; l'arbre FULL contracté reste la même fusion ternaire.

**Incompatibilité confirmée de formulation, rattachée à CST-0113.** Le
contrat numérique `e264de6f2:154` demande que toute translation préserve
l'empreinte sémantique, alors que son §4 renvoie au lecteur v11. Ce lecteur
absorbe les coordonnées absolues (`full_semantic.py:116`), les centres absolus
(`:152`) et exige le Morton absolu (`:110–113`). Il normalise fractions et
largeurs de profil ; il ne quotient pas les translations. Les cinq flux FULL
synthétiques du triangle sont tous acceptés et donnent cinq empreintes
différentes, conformément à son contrat.

Il faut distinguer : égalité de l'empreinte v11 pour **une même entrée** à
profils différents ; équivariance du résultat après retrait de la translation
et remappage des sites ; éventuelle nouvelle empreinte géométrique normalisée,
à spécifier et versionner. La correction du départage du squelette n'oblige pas
à changer les arbres FULL canoniques d'une même entrée. La promesse d'octets
identiques CPU/GPU concerne une même entrée et un même format ; elle est
compatible avec ces trois distinctions.

## Avis pour le port

Inscrire les hypothèses corrigées et les références de preuve avant T2 ; garder
les fixtures exactes dans les futures portes natives et les comparer à l'oracle
borné. Les points mathématiques examinés n'imposent pas une autre architecture.
Ils imposent les inclusions de T1, les dates de T3, les sommets courants et le
quotient de T4, l'historique séparé de T5, les branches étendues de T6, les
quotients et budgets de T7, et des départages publiés indépendants de Morton.

Cette contre-lecture ne ferme ni les budgets numériques complets, ni la
complétude/terminaison du générateur, ni les contrats temps/mémoire, ni la
stabilité des vues de points, ni la qualification native v12.
