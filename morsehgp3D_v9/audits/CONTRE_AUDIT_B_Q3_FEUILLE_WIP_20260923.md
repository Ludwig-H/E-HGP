# Contre-audit B — census q3 sur feuille d'atlas (WIP)

23 septembre 2026. Lecture **en cours de développement**, non publiée
comme moteur qualifié : `q4_local.cpp` SHA-256 `5624a774…`,
`wspd_q34.cpp` `13cb3b72…`, `tower_chain.hpp` `fd1421a6…`, sonde
`6eed4900…` dans `build/v9-open-worktree`. Les portes portées de v8 sont
fidèles à leur source, mais ne mettent pas `q3_leaf_census=true` ; deux
mutants ont échoué parce que leur site textuel a changé pendant ce WIP.
Les succès antérieurs ne qualifient donc pas la branche nouvelle.

## Invariant géométrique à préserver

Sur une `ExactLeaf` d'atlas **non saturée**, les nœuds uniformément
intérieurs ont une puissance strictement négative pour tous les centres
de la cellule fermée ; les nœuds uniformément extérieurs ont une
puissance strictement positive. Les nœuds actifs restants sont disjoints
et les sites du cover s'y répartissent exactement. Le code prend le
compte intérieur certifié puis recalcule le signe **exact** de tous les
sites actifs : profondeur et coquille entière en résultent. Un certificat
`Deep` sans fragment complet n'emprunte jamais cette voie ; si son
minorant est insuffisant, il retombe sur le census global. Le cover de
rayon `|ab|` contient chaque miniballe q3 aiguë possédée par sa plus
longue arête `ab` : son rayon est au plus `|ab|/√3`, le décalage de son
centre au milieu de `ab` au plus `|ab|/(2√3)`, somme `<|ab|`.
Aucun contre-exemple d'exactitude n'a été trouvé sous ces préconditions.

Une fixture native indispensable : `a=(14,20,20)`, `b=(26,20,20)`,
`x=(20,29,20)`, `y=(20,16,20)`, `m=(20,20,20)`, K3. Le centre q3
de `abx` vaut `(20,45/2,20)` et son rayon carré `169/4` ;
`a,b,x,y` sont contacts, `m` est strictement intérieur, `ab` est la
plus longue arête et la profondeur vaut 1. `m` est uniformément
intérieur dans tout l'atlas des centres sur `ab`, donc la racine est
`Deep` **exacte** au compte K−2=1 ; avec conservation des fragments,
la feuille doit rendre cette q3 et sa coquille de quatre IDs. Comparer
`q3_leaf_census` off/on et saturation off/on, plus un centre sur frontière
dyadique, au census global et à la sortie FULL. Une égalité de digest
sur une trame ne remplace pas cette porte causale.

## Mesure exploratoire, pas un gain acquis

Sur 08/000100 sans sol, 35 551 sites, K5/s8/W8, les fichiers locaux
**non versionnés** `s01_k5_sat.json` (SHA-256 `43840f86…`, voie sans
feuille, FULL statique0) et `s01_k5_leaf2.json` (`2f2eba6a…`, voie
feuille, FULL statique8) ont même entrée, catalogue, ordres et digest
`dbf799c8ed83f53f`. q3/q4 mesuré : 72,08 contre 69,48 s ; chaîne
82,89 contre 78,52 s. Ce sont des essais uniques sur hôte partagé, avec
**une option FULL différente** et des sources mouvantes ; même le travail
FULL diffère (2 249 645 hits de cache de résolution contre zéro, et
nombre d'appels MEB différent). Aucune vitesse relative n'est qualifiée.
Le premier essai feuille local était à 109,13 s
q3/q4 avec un nombre différent de feuilles, signe supplémentaire qu'une
comparaison non appariée serait trompeuse.

Les boîtes de census q3 passent de **807,38 M à 368,30 M** ; les tests
ponctuels du census classique de **43,99 M à 15,43 M**, mais la nouvelle
voie paie **610,29 M tests ponctuels** sur 10,70 M feuilles. La somme
des **deux colonnes exposées** de tests ponctuels q3 est donc multipliée
par **14,23**, malgré la baisse du temps observé. Ce n'est pas le total
physique q3 : `q3_census_point_tests` n'inclut pas la passe globale de
coquille, dont les compteurs existent dans le moteur mais ne sont pas
exportés par la sonde. Il faut payer les deux passes dans les diagnostics de
croissance 8k/16k/32k, y compris les feuilles répétées, les scans par
graine, les octets de frontière et l'aval. Une réduction d'une catégorie
de bornes ne suffit pas à prouver le sous-quadratique.

Le même soin vaut pour l'atlas : `atlas_ids_copied` compte les append de
**nœuds** frontaliers, non une population de sites ou des octets DRAM
mesurés ; `q4_sweep_events` omet les visites des sites actifs et le tri.
Publier séparément ces postes physiques et le pire coût par arête avant
d'affirmer une pente de croissance.

`ChainOptions::q3_leaf_census` vaut actuellement `true` par défaut,
mais la sonde v3 ne l'expose ni en argument CLI ni dans `options` JSON.
Deux voies géométriques différentes sont donc indiscernables dans le
profil publié. Épingler explicitement ce choix dans le plan et la sonde
avant de construire un reçu G4. Le paquet G4 R2 `0b29b6c3` est antérieur
à ce WIP ; son échec de schéma a une autre cause.

Enfin, l'en-tête `WspdQ34Options` dit que `q3_leaf_census` exige
`q3_atlas_consultation`, mais la validation ne rejette pas la combinaison
`true/false` : l'option devient alors silencieusement inerte. C'est un
défaut de contrat et de provenance, pas un contre-exemple géométrique.
Refuser la combinaison ou annoncer explicitement cette inertie.

## Mise à jour après publication `e54f727c`

Les deux défauts de provenance du WIP ci-dessus sont **corrigés dans le
code publié** : la sonde accepte `--q3-leaf`/`--no-q3-leaf` et publie le
choix dans `options`, et `WspdQ34Options` refuse la feuille si la
consultation de l'atlas n'est pas active sous `Local28`. Le constat WIP
reste conservé pour dater précisément ce qui a été inspecté ; il ne doit
pas être lu comme un défaut encore ouvert. Les portes natives portées et
les deux mutants q3 propres à cette branche sont déclarés dans le
commit ; aucune exécution G4 postérieure au commit ni qualification de
tour sous une seconde n'en découle.

L'analyse du même fichier exploratoire `s01_k5_leaf2.json` affine la
priorité : `10 146 767` des `10 699 704` census de fragment, soit
`94,83 %`, se terminent en rejet ; les `610 289 100` tests représentent
`57,04` tests par census en moyenne. Le travail à supprimer en premier
est donc le rejet tardif des graines, sans perdre la recollecte de la
coquille pour les `552 937` census non rejetés. Ce sont des compteurs
d'un seul essai local non apparié, pas une courbe de croissance.

Verrou structurel vérifié dans la construction de l'atlas : une vraie
cellule terminale `State::Leaf` a une frontière active **de singletons**.
Quand un budget laisse des nœuds internes, la construction les raffine
avant publication ; les `ExactLeaf` issues d'une cellule `State::Deep`
retenue peuvent, elles, conserver des blocs internes. Poser seulement
une borne spatiale sur chaque nœud actif ne peut donc réduire aucun test
ponctuel des vraies `Leaf`. Avant d'implémenter un nouveau parcours par
blocs, exporter les histogrammes du nombre de graines par cellule, de
la taille des frontières et des parts `Leaf`/`Deep` ; mesurer un hybride
singleton→prédicat ponctuel, puis le partage de préfixe **entre graines
d'une même cellule**. Un groupe traversant deux cellules ne peut pas
hériter d'un compte intérieur commun.

Pour un raccord exact de la voie `Deep`, l'invariant est
`cover = I ⊎ O ⊎ F` : démarrer du compte certifié de `I`, parcourir
uniquement les racines disjointes de `F` pour le compte, puis **toutes**
ces racines pour la coquille si la graine est acceptée. Ne jamais partir
de la racine globale avec ce compte, au risque de compter `I` deux
fois. Le ticket doit lier propriétaire, index, boule positive, cellule
et fragment ; un couple public libre `(compte, fragment)` serait
forgeable. Si la règle de compte saute un bloc à `min≥0`, une graine
cosphérique peut parvenir à EOF tout en restant valide : reprendre de
v8 l'assertion « EOF impossible » serait incorrect. Il faut une fixture
cosphérique et une comparaison de la coquille complète contre l'oracle.

L'[audit A de la descente rationnelle](Q3_STRUCTURE_ET_BORNES.md)
repère séparément la conversion complète sur 20 bits par axe avant
chaque localisation d'atlas : 466,02 M localisations dans le brut R2
000000/K10 impliquent des milliards d'itérations de quotient/reste.
Comparer directement les rationnels aux coupures dyadiques est exact
jusqu'à la profondeur 10 sous les bornes i128 établies, avec repli au-delà ;
son oracle autonome passe. Il faut une ablation native appariée, et ne
pas additionner ses éventuels gains aux `610,29 M` tests ponctuels
répétés de la voie feuille comme s'il s'agissait du même poste.
