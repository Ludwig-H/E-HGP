# Premier composant FULL : certificat structurel compact

5 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

Le composant [full_certificate.hpp](../src/forest/full_certificate.hpp)
construit et relit une forêt **déjà décidée**. Son autorité est
`structural_only`, son schéma `full_minima_merge_forest_v1`. Il ne calcule
aucune boule, aucun portail, aucun catalogue Gabriel et aucune couture
verticale. Un succès ne prouve ni l'exactitude des niveaux fournis, ni la
complétude géométrique de la forêt. Il n'est pas raccordé au pipeline ou à
la CLI F et n'en change aucun format public.

Le [complément du 6 septembre sur les plateaux](PLATEAUX_FULL_ET_ANCRES.md)
borne explicitement cette représentation au modèle régulier : hors
régularité, les unions de feuilles ne suffisent plus. Il faut versionner
couvertures initiales et gains datés, même sans fusion. Ce supplément
n'est pas implémenté dans le schéma v1 décrit ici.

## 1. Information conservée et autorité extérieure

La [preuve de suffisance](AUDIT_NIVEAUX_GABRIEL_20260905.md) et sa
[contrelecture indépendante](../audits/NIVEAUX_ET_CERTIFICAT_HGP_COURANT.md)
motivent le modèle régulier : une feuille est un minimum Gabriel de cardinal
K, un nœud interne une vraie multifusion de parents préexistants. Leur
qualification géométrique reste au futur producteur.

Le certificat possède trois tableaux privés : nœuds avec niveau exact,
labels des minima (K PointId dans un `FacetKey` de capacité 10), et références
de parents en CSR. Un nœud indique soit l'indice de son minimum, soit
l'offset et le nombre de ses parents. Les identifiants de nœuds sont denses
et distincts même si les couvertures ponctuelles se recouvrent. Aucun
catalogue Gamma, aucune coface silencieuse, aucun delta de continuation,
aucune couverture copiée par nœud n'est retenu.

Avec L feuilles, I multifusions et R racines finales, le nombre de références
est $L+I-R$ et $I\leq L-R$. Le modèle mathématique porte O(KL) identifiants
de feuilles ; ce premier stockage à capacité fixe réserve dix cases par
minimum, même aux petits K. L n'est pas borné par n en général. Ces bornes
ne concernent ni la découverte des candidats ni le travail des portails.

L'identité/hachage de l'entrée, la métrique, l'unité des niveaux, les ordres
disponibles, l'horizon certifié, l'autorité terminale, la complétude et les
ancres inter-K appartiennent au manifeste du futur producteur, **absent de
ce composant**. Le domaine PointId reçu est validé mais non recopié dans le
résultat. Les niveaux rationnels sont comparés sémantiquement ; leurs
fractions ne sont pas normalisées pour un éventuel format binaire canonique.

## 2. Construction en une transaction

`build_full_certificate` reçoit un ordre entre 1 et 10, un domaine PointId
strictement croissant, des lots non vides et trois plafonds explicites :
nombre de lots, nombre de nœuds et nombre de références de parents. Les
plafonds valent zéro par défaut : aucune capacité implicite illimitée.

Les niveaux des lots sont strictement croissants par comparaison rationnelle
exacte. Deux écritures comme 9/1 et 18/2 ne peuvent former deux lots. Dans
chaque lot, les feuilles sont triées par clé et les groupes de parents par
liste lexicographique ; chaque liste de parents est strictement croissante.
Les clés ont K points distincts connus et un padding nul. Un minimum ne
peut naître deux fois, même à deux niveaux différents.

Les feuilles reçoivent leurs identifiants avant les multifusions de leur
lot. Une fusion a au moins deux parents et ne peut consommer que des racines
actives **avant** ce lot. Parent inconnu, parent déjà consommé, parent partagé
entre deux fusions simultanées ou créé dans le lot : refus. Les groupes
simultanés doivent donc avoir été quotientés par le producteur en vraies
multifusions avant cet appel.

À K=1, tous les points du domaine naissent à zéro et aucune feuille ne
naît ensuite. Pour K supérieur à un, les niveaux sont positifs. Le cas
K=n peut se réduire à un unique minimum sans parent ; aucune coface n+1
n'est requise. Le composant accepte une forêt non vide encodée, mais ne
décide pas si son horizon ou sa terminaison sont complets.

Les cardinalités sont contrôlées avant les réservations. Les références
sont ensuite validées sur un état privé. Tout refus, y compris tardif ou
par `bad_alloc`, rend un certificat invalide avec tableaux vides : aucun
préfixe n'est publiable. Le certificat est non copiable et transférable
sans allocation ; le déplacement invalide explicitement sa source. Ceci
évite qu'une affectation copiée partiellement par manque de mémoire laisse
des tableaux incompatibles accessibles aux lecteurs.

Ce premier constructeur est **en un seul appel, pas en streaming** : les
lots restent détenus par l'appelant et coexistent avec le résultat en
construction. Un tableau temporaire de racines actives et une copie triée
des minima servent à la validation. Les plafonds comptent des records,
pas la RAM totale, la capacité allouée des vectors ou les octets de l'entrée.
Il n'y a ni budget RSS certifié ni qualification massive à ce stade.

## 3. Lectures bornées

`full_certificate_roots_at` rejoue une coupe ouverte ou fermée en comparant
exactement son niveau à chaque lot. Son plafond de nœuds couvre la forêt
entière, y compris pour une coupe précoce. Il utilise un état temporaire
linéaire ; il ne prétend pas fournir une requête de coupe sublinéaire.

`full_certificate_coverage` visite les feuilles descendantes d'un nœud et
retourne l'union triée de leurs PointId. Le plafond de nœuds compte les
visites programmées, pas seulement la profondeur ; celui des points compte
les références **avant déduplication**. Deux feuilles de deux points qui
partagent un point consomment donc quatre références, même si leur union
n'a que trois points. Les refus et les pannes d'allocation vident le résultat
de lecture sans changer le certificat initial.

Les coupes, même après le dernier niveau encodé, ne portent que sur cette
forêt. Leur succès ne certifie aucun intervalle géométrique supplémentaire.
Les couvertures ponctuelles ne reconstruisent pas toutes les facettes Gamma,
leur incidence ou le carrier marqué. Les feuilles pondérées de l'Algorithme 1
du manuscrit ne sont pas remplacées par les minima FULL.

## 4. Qualification et suite

La [porte dédiée](../tests/full_certificate_gate.cpp) vérifie les triangles
aigu et obtus, les minima simultanés, K=1, K=n et K=10, les PointId extrêmes,
les grands numérateurs U192, les deux côtés des coupes, les unions avec
recouvrement, les plafonds exacts et insuffisants, les parents pré-lot,
les refus atomiques et les déplacements. Les pannes persistantes d'allocation
sont injectées sous l'API en balayant les allocations réellement observées,
sans macro de mutation dans le composant produit. Les compteurs et planchers
restent actifs en Release ; ce ne sont pas des `assert`.

Les [reçus du composant](../receipts/full_certificate_20260905/README.md)
distinguent les deux portes CTest Release et ASan/UBSan de la qualification
historique F. Les chemins défensifs `size_overflow` qui exigeraient des
conteneurs irréalisables sur cet hôte 64 bits ne sont pas déclarés exercés.
Les fixtures géométriques sont des entrées exactes épinglées, pas une
nouvelle qualification de générateur ou un oracle géométrique indépendant.

Le [producteur horizontal suivant](CONTRAT_PRODUCTEUR_FULL_GABRIEL.md)
produit désormais ces lots depuis deux catalogues Gabriel fournis, sous
prémisses de complétude, exactitude et régularité extérieures. Il possède
ses propres portails, budgets et comparaison bornée à Gamma ; ses résultats
ne sont pas hérités de ce certificat structurel. Les ancres verticales au
plateau inférieur fermé restent à raccorder. Aucun seuil de performance,
palier massif ou statut public exact ne découle de ce premier composant.
