# Mémo de descente datée avant MEB

Tranche qualifiée à `c2c3e0323` : [capture memo1](../receipts/full_memo_20261003/memo1/README.md),
2475/2475 portes et178/178 ASan18. Les six paires LiDAR K5 montrent un gain,
mais FULL reste entre10,069 et14,690s avec mémo, hors contrat200ms. `descend` demeure la référence exacte sans table. Le modèle
préliminaire collinéaire de projection valide un contrat de cache pour un
résolveur déterministe ; il ne juge ni le choix natif des témoins de census
ni les cas 3D. Les nouvelles portes les traitent séparément.

## Identité et preuve

`FullParams{memo_capacity}` complète `build_full`. Le défaut zéro désactive
la voie ; toute capacité active est une puissance de deux explicite. La
première ablation demandée vaut 65536 slots, choix empirique à mesurer.
`DescentMemo` est privé au module tower et à un seul appelant synchrone.
Il emprunte un FullDomain immobile ; aucun partage concurrent de table.

Une clé possède les k SiteIdx de toute la partie, triés, complétés par
kNone jusqu'à 12, et sa cardinalité. L'ordre k, le domaine, les indices et
les doublons sont validés avant tout lookup. Le hash modulo 2^64 choisit
une case ; seule l'égalité complète du tuple et de son cardinal donne un
hit. Toute collision est un miss, puis un remplacement possible au succès.
Une instance de capacité zéro refuse aussi un domaine étranger ; nullptr
appelle directement la référence, sans deuxième validation/tri.

La valeur possède βinitial, βterminal et BirthSeed. Les deux Level sont
copiés sans réduction ni reconstruction depuis le catalogue : une MEB
locale q4 peut porter la même boule qu'une présentation globale q2/q3,
sans employer les mêmes numérateur et dénominateur. Aucun NodeIdx, parent,
DSU, coupe courante ou ledger ancien ne figure dans la table.

Inductivement, toute entrée publiée provient d'une descente exacte achevée,
ou d'un préfixe exact terminé sur une telle entrée. Un suffix-hit vérifie
βsuffix < βprécédente avant de reprendre son semis et son niveau terminal.
L'origine garde sa propre βinitial. Le premier hit termine la requête ;
aucun chemin ni tableau de suffixes n'est matérialisé. Seule l'origine est
publiée après le succès de la résolution et de ses additions de compteurs.
Un refus de MEB/census/date/compteur ne détruit aucune entrée ancienne.

Par déterminisme de la descente de référence, ce raccourci conserve son
semis et ses deux représentations de niveaux. La validité géométrique du
semis commence à la coupe fermée βinitial, jamais à βterminal. Les gardes
de plateau βinitial<λ et de verticale βinitial≤λ restent dans les appelants,
qui relèvent ensuite le semis dans leurs composantes courantes.

Sur X=(0,2,4,6), k2, la partie extrême a βinitial=9 et descend au semis
(1,2), βterminal=1. Ce semis existe à la coupe4, mais pas la partie extrême.
Mémoriser le niveau1 comme date de validité serait donc faux.

## Mémoire et transaction

Un seul Buffer contient C slots triviaux, tous initialisés avant lecture
(y compris sous poison). L'admission vérifie C≤u64max/sizeof(Slot), puis
C·sizeof(Slot). Le coût exact dépend de l'ABI et des profils numériques ;
il est publié, pas remplacé par une estimation fixe. Refus de capacité,
de budget ou d'allocation : pas de repli silencieux vers la voie sans mémo.

La table vit durant toutes les forêts et verticales du même build_full.
Elle est détruite **avant** le transfert final de FullDomain ; ses octets
ne restent pas dans FullTower. Le pic inclut sa coexistence avec tous les
ordres retenus. Un refus ultérieur du FULL détruit table et brouillons,
préserve le domaine et ne publie pas de diagnostics partiels.
`FullTimings` publie atomiquement memo_capacity, memo_slot_bytes et
memo_reserved_bytes ; le dernier champ désigne la réserve pendant
construction, pas la mémoire finale. À capacité zéro : 0, sizeof(Slot), 0.

La recherche ne s'arrête jamais parce que la table est pleine : elle
remplace directement une case. Ce plafond borne uniquement la mémoire du
cache, pas les chemins de descente ni la complétude. Aucune borne globale
de coût ni hypothèse de taux de hit ne découle de cette table.

## Travail et portes

`DescentLedger::memo` publie queries, lookups, hits, misses, collisions,
insertions, evictions et suffix_hits. Les autres champs ne comptent que le
travail effectivement refait. Un hit initial a donc steps=MEB=census=0.
Pour des requêtes actives réussies, les égalités suivantes sont testées :

- lookups=misses+hits et misses=steps ;
- queries=insertions+hits−suffix_hits ;
- steps−interior_steps−trace_steps=queries−hits ;
- suffix_hits≤hits≤queries, collisions≤misses, evictions≤insertions.

La référence et la capacité zéro gardent les huit champs nuls. Tous les
cumuls utilisent une addition contrôlée transactionnelle. Les anciens
planchers steps≥appels de résolution ne sont pas transférés aux hits : les
appels restent comptés par queries, le travail évité n'est pas crédité.

Les portes préparées comparent toutes les petites parties de fixtures 3D
à la référence, y compris représentations des deux Level ; elles couvrent
hits initiaux/suffixes, collisions réelles de capacité1, permutations,
refus sans éviction, domaine étranger même à zéro, résultats coexistants,
budgets privés concurrents et faute à chaque allocation FULL observée.
L'oracle Definition indépendant rejoue les mêmes 34 requêtes FULL, coupes
et verticales avec et sans table, sans imposer le chemin natif. Les mutants
ciblent clé partielle, dates perdues, faux travail, éviction prématurée,
propriétaire et débordement de cumul. Leur compilation/exécution est close dans memo1. Le calendrier massif reste
incomplet :17 succès sur18, dernier synthétique32k mode7 omis par budget.
