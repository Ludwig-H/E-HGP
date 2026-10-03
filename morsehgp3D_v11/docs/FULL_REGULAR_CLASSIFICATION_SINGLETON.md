# Classification régulière directe et terminal singleton

Deux raccourcis exacts sans option nouvelle ni allocation. Qualification
native et gain FULL encore attendus ; modèles Python et relectures
statiques favorables. La capture G4 `reuse1`, source `ae817d09e`, ne
contient pas ces changements.

## Cellule régulière

Le catalogue certifie q=qmin entre 2 et 4, p intérieurs et m points de
coquille. Si m=q, le centre appartient strictement au simplexe support.
Posons h=p+q. Les seuls ordres actifs sont h−1 et h : les q faces propres
ont une MEB strictement plus petite, et la partie entière naît à h.
Cela reste vrai si p>0. Le constructeur de forêt peut donc écrire
directement le type de cellule sans rappeler le classificateur général.

Le travail logique reste exactement celui du classificateur : une cellule
classifiée et q combinaisons à h−1, une combinaison à h. Les neuf autres
champs n'ajoutent rien et ne doivent surtout pas être remis à zéro après
une cellule étendue. Les cellules m>q gardent leur chemin général.
La mémoire, les plateaux et l'ordre des naissances ne changent pas.

Les portes natives comparent les dix champs avec `classify_cell`, y
compris après deux appels du même builder, puis les tables FULL avec un
oracle Gamma/Fraction sur sept nuages et tous leurs ordres. Elles couvrent
les variantes W1/W4, mémo, census, lookup dense et réemploi vertical.
Le modèle Python donne 168 ordres, 672 cas réguliers, 156 cas étendus,
7728 contrôles et 1413 corruptions. Les planchers natifs 7000/10000/1400
ne sont pas encore des résultats. Les mutants inversent le type de
cellule ou le nombre de combinaisons d'une naissance.

## Descente d'un singleton

Cloud possède des sites distincts. Pour une partie de cardinal un, la
boule minimale a rayon zéro, aucun intérieur strict et pour seule
coquille le site lui-même. Après les mêmes validations de propriétaire
du workspace, d'ordre, de cardinal et de site, `descent_step` conserve
la MEB calculée et construit directement sa graine terminale.
La représentation du niveau et le compte MEB restent ceux du chemin
normal ; aucun census ni lookup catalogue n'est effectué.

Le compteur distinct `singleton_hits` évite d'inventer un census gratuit.
La partition exacte est `census_calls + catalogue_hits + singleton_hits = steps`. L'addition de ce compteur est contrôlée et transactionnelle.
Les références fermées de descente, les deux sondes de forêt et le banc
FULL doivent sérialiser ce nouveau champ. Les mémos ne rejouent jamais
le travail d'un appel antérieur lors d'un hit.

Les tests conservent notamment le refus d'un workspace étranger même
sur singleton, l'identité d'un site autre que zéro, l'absence d'allocation,
les vrais census à k>1 et les saturations. Le losange à quatre sommets
garde un miss global à k4 après suppression des census de singletons.
Le modèle de descente donne 64650 contrôles/135 corruptions ; celui du
census partagé 65808/45, dont 81 faits de saturation et 3720 de routage.
Quatre mutants jugent le compteur, l'identité, le workspace et l'overflow.

Les premières erreurs des nouveaux harnais sont conservées dans le reçu
de développement : fixture q4 vide absente du modèle de classification,
et macro REQUIRE à retour void dans un helper non void. Elles ont été
corrigées avant tout lancement natif ; aucune qualification ne repose sur
la seule confiance accordée à leur auteur.
