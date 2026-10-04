# MEB bornée et raccord au census global

Cadre : `exploration_v11_hors_registre`, `cpu_reference`,
`quantized_u21_input_only`, `implementation_v11_meb`, `not_claimed`.
La primitive construit une MEB exacte, avec support local canonique, pour
1 à 12 sites. La nouvelle sélection par diamètre, préparée après `12f49`,
reste **à qualifier sur G4** ; aucun gain de temps n'est acquis.
Les [reçus MEB historiques](../receipts/meb_20261002/README.md) et la
[baseline FULL](../receipts/full_20261002/README.md) conservent leurs propres
sources et stratégies. Ils ne qualifient pas cette nouvelle recherche.

## Contrat géométrique

`bounded_meb(cloud, part)` copie puis trie les SiteIdx et leurs coordonnées
sans allocation. Il refuse une partie vide, un cardinal supérieur à 12,
un Cloud vide, un indice hors domaine ou répété, dans cet ordre. Les
indices sont les rangs Morton du même Cloud, pas les PointId externes.
Les poids sont ignorés par cette primitive géométrique.

Pour un singleton, la réponse est le point, au niveau zéro. Sinon, la
recherche choisit la première paire lexicographique parmi celles à distance
maximale et teste sa boule diamétrale. Si elle contient la partie, elle est
sa MEB q2 canonique ; sinon aucune paire ne convient. La preuve complète,
les bornes et les compteurs sont dans [MEB_DIAMETRE](MEB_DIAMETRE.md).

Après cet échec, tous les q3 puis q4 sont visités en ordre lexicographique,
jusqu'au premier support strict contenant. Un triangle doit être strictement
aigu et un tétraèdre contenir strictement son centre. Un préfixe q3 obtus
ne supprime jamais son prolongement q4. Chaque candidat positif est testé
contre toute la partie, avec arrêt au premier extérieur ; les contacts
sont inclus. M1 garantit l'existence du support strict et l'unicité de la
MEB. L'ordre de visite donne le cardinal minimal, puis le premier tuple.

La Sphere conserve la représentation de son support strict gagnant. Le
support reste **local à la partie**, distinct du support global S* du
catalogue et d'une clé d'identité géométrique. Une même MEB peut avoir une
présentation locale q4 et un support global q2 ou q3 ; `locate_part` traite
ce cas par un census complet du propriétaire commun.

## Propriété, mémoire et travail

Le résultat possède la Sphere, quatre indices et le ledger ; aucun emprunt
ne survit à l'appel. Interpréter les indices exige le même Cloud. Les
états temporaires sont fixes : 12 sites, quatre positions de combinaison,
une récursion de profondeur quatre et un entier de distance. Aucun cache,
Buffer, flottant ou nouvelle arithmétique scalaire n'est introduit.

`diameter_pairs=C(n,2)` compte les puissances q1 auxiliaires du choix du
diamètre, au plus 66. Elles sont distinctes des `presentations` de candidats
MEB. Pour n>1, au plus 716 présentations entraînent au plus 8592 inclusions,
auxquelles il faut ajouter ces distances et leurs comparaisons. Au succès,
`containing=1` et `comparisons=0` ; le singleton fait une présentation, un
test et zéro distance auxiliaire. Les cumuls cells/classification/descent/
forest conservent ce septième compteur avec addition contrôlée. Aucun coût
local ne borne le nombre de requêtes de toute la tour.

La limite 12 ne prétend pas accepter une partie de 13 sites. Le constructeur
FULL actuel par sphères critiques peut néanmoins traiter K12 : il n'énumère
pas les cofaces de cardinal 13 pour en appeler la MEB. La fixture ligne de
13 sites à K12 distingue ces deux contrats. FULL refuse les multiplicités
qu'il ne prend pas en charge avant de construire ses cellules.

## Raccord synchrone au propriétaire global

`meb_census(index, part, threshold, budget)` refuse d'abord le seuil nul,
puis calcule la MEB dans `index.cloud()` et interroge ce même index. Le
census seul alloue ; un refus restitue ses réservations sans résultat
partiel. Les résultats coexistants paient chacun leurs listes.

À seuil k, une saturation fournit exactement k témoins strictement
intérieurs, sans coquille. Leur MEB a un niveau strictement inférieur
(T5). Sinon, tout I et toute U sont conservés, sans plafond 12 de coquille.
[FullDomain](FULL_DOMAIN.md) lie désormais index/catalogue/lookup possédés ;
[cells, locate et descente](CELLS_AND_LOCATE.md) utilisent ce raccord sans
confondre support local, identité globale et date initiale du terminal.

## Portes et mesures

Le juge Gram/Gauss/Fraction minimise toutes les circonsphères affines
englobantes sans filtre de diamètre ni de positivité pour décider le
minimum ; il choisit ensuite le support strict minimal. La lecture de
travail suit seulement après cette réponse indépendante. Les fixtures
couvrent maxima ex aequo, singleton, ligne12, q3 aigu, q4 à face obtuse,
extrêmes18/21/24, refus, propriété, concurrence et pannes du census.
Les mutants ciblent choix de paire, canonicité, inclusion et comptage.

Le banc MEB distingue les temps MEB/census/wrapper/scan. Son scan partage
num::side et ne constitue pas un oracle arithmétique indépendant. Les
événements courants portent `meb_search=exact_diameter_then_q34_v1` ; le
décodeur vérifie C(n,2) et le rang exact du support arrêtant. Le format des
objets géométriques canoniques reste inchangé. Les anciens reçus et leurs
lecteurs restent intacts ; toutes les nouvelles portes natives attendent
G4 gardée. Les comparaisons de durées doivent compter le travail auxiliaire,
jamais conclure d'après la seule baisse des présentations.
