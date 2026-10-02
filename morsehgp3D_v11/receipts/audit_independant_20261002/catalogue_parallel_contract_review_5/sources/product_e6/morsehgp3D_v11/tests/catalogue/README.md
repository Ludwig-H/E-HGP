# Juge indépendant du catalogue

Le modèle énumère les supports affinement indépendants de 2 à 4 sites, résout
leur système de Gram avec `Fraction`, puis conserve les poids barycentriques
strictement positifs. Il groupe par centre et rayon exacts, recense globalement
l’intérieur et toute la coquille, déduit le cardinal minimal et son premier
support lexicographique, puis applique `p+q_min <= K+1`.
Il n’importe ni R2, ni `intgeom`, ni la référence constructive FULL. Son coût
exhaustif est réservé aux petits nuages de test, au plus 14 sites distincts.

`probe.cpp` traite un lot texte : `K leaf maxleaf maxnodes balllimit budget n`,
puis n lignes `x y z PointId`. Une ligne JSON suit chaque requête, y compris
un refus du produit. Le processus finit à zéro si tout le lot a été traité ;
une entrée mal formée ou une panne du harnais rend 2. `--profile` donne le
profil réellement compilé. Le plafond d’un million de lignes d’entrée est
celui de ce pilote de test, distinct du domaine produit et du pilote LiDAR.

Les tableaux d’entrée et le JSON du harnais ne sont pas dans `MemoryBudget`.
Le nuage et le catalogue partagent le même budget ; `used_after` est relevé
après leur destruction et doit revenir à zéro, succès et refus compris.
`ledger` publie les compteurs logiques d’une passe, dont `census_tests` ;
le produit paie deux passes. `catalogue_ns` est un diagnostic du pilote,
pas une qualification de performance ni une mesure isolée.

Le lot natif préparé contient **378 requêtes** : 358 succès et 20 refus
attendus, sur 28 familles. Chaque famille passe K1..10 puis K12 diagnostic,
avec ordres de Morton indépendants et PointId maximal valide. Les variantes
comprennent des entrées permutées, petites feuilles et faces extrêmes sur
les trois axes. Le juge compare niveaux, rangs, support, qmin, p, m, intérieur,
coquille, complétude et ordre. Il vérifie 43 égalités entre variantes et
84 restrictions de CatK supérieur vers CatK inférieur après renumérotation.

Faits ciblés : triangle droit qmin2, poids nul qmin3 avec coquille4,
tétraèdre de niveau25 à préfixe obtus, coquilles cube/octa et qmin3 avec
présentations q4, admission à l’égalité, coordonnées extrêmes. Trois sites
`(0,0,0),(2a,0,0),(a,a,1)` donnent deux niveaux distincts `a²` et
`a²+1/(4(a²+1))` se confondant en binaire64 aux trois profils. Une octaèdre
de rayon5 et son centre forcent le témoin commun d’une paire à compter
une seule fois dans une feuille fine.

La relecture après `catalogue1` a isolé une lacune : aucun des 27 nuages du
premier lot n’avait simultanément qmin4 et une coquille de plus de quatre
sites ; le retour positif du chercheur de support tétraédrique canonique
restait donc hors de ce juge. Le lot suivant ajoute `extended_q4` :
`(10,5,5),(9,8,5),(5,2,1),(1,5,8),(9,2,5)`. La boule de centre `(5,5,5)`
et niveau25 a p0, m5, qmin4 ; ses deux supports stricts en rangs Morton
sont `(0,1,3,4)` et `(0,2,3,4)`. Les deux premiers quadruplets lexicographiques
ne sont pas stricts. Cette extension ne modifie pas la source figée de
`catalogue2` ; le nouveau mutant doit être qualifié dans un lot distinct.

`model_test.py` grave 42 faits sur les profils18/21/24 et tue 17 corruptions
de réponse/JSON. Il a été joué en Python normal et `-O`, avec résultats
identiques, avant qualification native. Le manifeste catalogue contient
neuf mutations du produit, toutes adressées au vrai juge Fraction.
Leur présence n’est pas une preuve de détection : résultats natifs et
mutants sont à relever dans la session G4 du développeur.

L’égalité rationnelle est jugée indépendamment de l’écriture non réduite.
Entre deux entrées permutées ou deux tailles de feuille, les champs
canoniques, écritures des niveaux comprises, sont exigés identiques.
Cette batterie qualifie le catalogue borné ; elle ne qualifie pas FULL,
les performances LiDAR ou la projection sur les points.
