# Registre R — branches ouvertes, prélecture du 7 octobre 2026

**Le prototype répond à la perte d'information signalée par l'audit : les branches `ant(b)` sont maintenant
matérialisées. Lecture mathématique et de concurrence favorable ; une formule d'admission mémoire reste à compléter.**
Capture non publiée, stable par double lecture, `registry_branches.cpp` SHA256
`9cc388d6afbed55f98e58ca84eb7948ee30762dbf4aca918a63a3814c2a6aa0b` ; autres épingles dans `capture.json`.
Main observé : `b24c256541ffdbe8327f894cd9efebdd7476b51c`. Base déclarée du prototype : `9c5809919`.
Cadre : exploration v12 hors registre, CPU de référence, FULL pi0, u21 ; `public_status=not_claimed`.

## Mathématiques et concurrence

Les blocs consécutifs de `event_cell` identifient exactement les cellules ayant produit au moins une union. Pour
chacune, R relit ses représentants, retrouve une naissance témoin, puis appelle `component_at(witness, r-1)`.
Les rangs sont entiers : les attaches et événements retenus ont donc un rang strictement inférieur à `r`.

Une cible naissance fournit `birth_node[index]`. Une cible cellule fournit `minleaf[cell_node[index]]` après M.
La prépasse T a déjà imposé que le rang de la cible soit strictement inférieur à `r`. Son sommet, même contracté avec
d'autres événements de son propre plateau, appartient donc à une composante née avant `r` ; sa plus petite naissance
est un témoin valide de la même composante à la coupe ouverte de `r`. Il ne s'agit pas de la racine courante après
les unions du plateau `r`. L'hypothèse de domaine de `component_at` est satisfaite ; le contrôle explicite `r != 0`
évite la soustraction sous zéro. Le tri et la déduplication donnent la ligne canonique de `ant(b)`.

Chaque tâche écrit des lignes distinctes du tampon temporaire et possède ses compteurs. Après la fin du premier
`parallel_for`, le pilote construit les préfixes et alloue la sortie. La seconde phase copie des plages CSR disjointes.
La forêt et ses historiques restent en lecture seule. Aucune nouvelle barrière par plateau ni descente G.

## Admission à compléter

`place_rows` alloue `branches.off`, soit `8*(R_k+1)` octets par ordre, puis les valeurs. La première admission couvre
les métadonnées, **le tampon temporaire** `branch_off`, `branch_nodes` et `branch_count`, ainsi que les tâches ; elle
ne couvre pas les décalages de la CSR finale. La seconde admission annonce seulement `4*A`. Il manque donc
`8*sum_k(R_k+1)` avant `place_rows`, y compris huit octets pour un ordre sans cellule retenue.

Cela peut produire un refus mémoire tardif après une admission acceptée. Chaque allocation reste individuellement
soumise au `MemoryBudget` : aucun dépassement du plafond physique ni corruption n'est déduit de cette omission.
Le contrat de `buffer.hpp` distingue explicitement admission préalable et allocation individuelle.

`budget_proposed.patch` ajoute ces seuls décalages à la seconde formule. Application textuelle réussie dans un
dossier temporaire, corps résultant `0d616e69a3878b238bcfd0f0beb90ed3b161c77ebbfa740abd663b8b650009d5`.
**Patch non appliqué au prototype, non compilé et non qualifié nativement.** Les branches et compteurs logiques ne
changent pas ; un refus jusque-là tardif peut désormais intervenir à cette admission. Cette petite correction ne
prétend pas qualifier toutes les formules mémoire de la tour.

## Travail et mémoire réellement prévus

Soient `P_R` les représentants des cellules retenues, `A=sum |ant(b)|` et `R` leur nombre. Le prototype relit les
`P_R` représentants une fois, fait autant de requêtes de composante et trie chaque ligne, puis copie `A` valeurs.
`branch_reads=P_R`, `branches=A` : la seconde passe n'est pas une seconde lecture des représentants. Le coût comporte
les requêtes de l'historique et `sum_b O(d_b log d_b)` pour les tris, où `d_b` est le nombre de représentants relus.

Le tampon temporaire `4*P_R` coexiste avec les `4*A` octets de sortie, leurs décalages CSR et la forêt déjà construite.
Les tableaux propres aux lignes ajoutent `12*R + 4*R + 8*(R+K)` octets temporaires/métadonnées ; la CSR finale ajoute
`8*(R+K)`. Les tâches, alignements et classes du cache s'ajoutent à ces tailles brutes. Le tampon est libéré après
remplissage ; un cache résident peut conserver les blocs physiques.

Le choix diffère de la variante du précédent reçu qui relisait les représentants pour remplir une sortie exacte
en réduisant le workspace : ici une seule série de requêtes est payée, au prix du tampon `4*P_R`. `A <= P_R`, mais
aucune borne `A <= naissances-1` n'est utilisée. `registry_ns` rend visible le coût de cette nouvelle étape.
Aucun gain temporel ni respect des 100 ms n'est déduit de la lecture.

## Portes présentes et intégration restante

Le nouveau groupe officiel `branches` contient les deux hypergraphes de notre témoin, la famille sept naissances /
six événements / 27 branches et 300 hypergraphes aléatoires. Son attendu utilise un Kruskal par lots, un union-find
indépendant pour décider les cellules retenues, puis une remontée parent par parent de la forêt de référence pour
les coupes ouvertes. Le mutant `branches_coupe_fermee` remplace `r-1` par `r`. Sources lues, **aucune de ces portes
n'a été exécutée par ce contre-audit**.

Le rapport développeur revendique 3 112 lignes aléatoires et un essai ng00 K5 inchangé en octets, puis indique la
batterie finale relancée après modification de R. Ce sont ses résultats, pas nos rejeux. Les 651 tests antérieurs
à R ne qualifient pas ce nouveau corps. Les chiffres locaux du rapport, obtenus sur une machine chargée, ne sont
pas des mesures contractuelles G4.

Le cache du prototype est bien celui de main (`buffer.cpp` `5c885dbf…`, en-tête `3c7fffc6…`). En revanche, son juge
G `g_determinism.py` reste `beb8c3a1…`, antérieur au durcissement publié `650c63a1…` ; lors de la fusion, conserver le
juge actuel et ses nouvelles portes dans `tests/tower/tests.cmake`. La présence de R et la lecture favorable ne
valent pas qualification du produit intégré ni clôture automatique de la livraison T/M/V/R.

Reçu de lecture statique : aucun nouveau modèle, test natif, build, benchmark, GCP, coordonnée ou source complète.
Les empreintes locales se contrôlent avec `sha256sum -c SHA256SUMS`.
