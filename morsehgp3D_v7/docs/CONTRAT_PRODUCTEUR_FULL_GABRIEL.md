# Producteur FULL horizontal depuis les catalogues Gabriel

5 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

Le nouveau [full_gabriel.hpp](../src/forest/full_gabriel.hpp) décide les
minima et les vrais parents FULL à partir de deux catalogues fournis. Il
produit le [certificat compact](CONTRAT_CERTIFICAT_FULL.md), sans appeler
le constructeur de cœur F, sans catalogue Gamma exhaustif et sans journal
des cofaces silencieuses. Il reste séparé du pipeline F et de sa CLI.
Ce document décrit le contrat du composant, pas une promotion industrielle.
Il décrit ci-dessous la politique historique eager, conservée par défaut.
La nouvelle API séparée et ses différences de calendrier, de cache et de
traitement J=1 sont spécifiées dans le
[contrat du cache facultatif](CONTRAT_CACHE_FULL_PARESSEUX.md) ; ses preuves
et mesures ne sont pas héritées des reçus historiques cités ici.

## 1. Autorité et domaine exacts

`build_full_gabriel_order` reçoit un `CloudIndex` authentique construit par
l'entrée vérifiée, immutable pendant l'appel, un ordre K entre 1 et 10,
les minima Gabriel de cardinal K et les connexions Gabriel de cardinal K+1.
L'entrée u16 possède des identités distinctes et des positions distinctes.
Les tableaux publics de l'index ne constituent pas un format non fiable
que ce composant revaliderait intégralement.

Les catalogues sont supposés **complets, géométriquement exacts et réguliers
sous leur propre autorité extérieure**. Pour chaque record, la coquille
globale doit être exactement le support essentiel, affinement indépendant,
de cardinal entre deux et quatre, avec tous ses retraits stricts actifs.
Les labels, niveaux et intérieurs doivent être ceux de la boule autorisée.
Le composant valide la grammaire, les identités, les doublons, les masques
et les niveaux positifs ; il ne refait pas le census de tous les records
et n'authentifie pas la complétude de leur producteur.

Un succès s'appelle donc `kCompleteRelative` et porte littéralement
`full_horizontal_relative_to_supplied_complete_exact_regular_gabriel_catalogues`.
Il ne devient pas une certification inconditionnelle de l'entrée ou un
statut public `exact`. Une fixture aiguë conserve ses trois minima mais
omet ABC : l'appel termine relativement, tandis que l'oracle Gamma réfute
la hiérarchie obtenue. Cette sentinelle interdit précisément cette promotion.

À K=1, le catalogue de minima doit être vide : tous les points naissent à
zéro. À K=n, le minimum terminal est conservé et aucune coface n+1 n'est
nécessaire. Pour une tour jusqu'à Kmax, les catalogues de cardinal 2 à
min(n,Kmax+1) peuvent être partagés entre ordres adjacents. Les ordres
supérieurs à n ne sont pas inventés.

La régularité des directes ne suffit pas à garantir celle des MEB visitées
par les portails. Chaque MEB locale est vérifiée ; chaque requête d'intrus
non terminale achève son contrôle de coquille, même après deux intrus
trouvés. Une coquille locale non essentielle ou un point extérieur sur le
bord entraîne `kUnsupportedDegeneracy`, sans forêt. Un terminal identifié
dans le catalogue réutilise son autorité globale après recalcul local de
la MEB et comparaison exacte du niveau. Aucun domaine dégénéré général
n'est qualifié par ce raccourci.

## 2. Calendrier atomique et deux économies prouvées

La [revue mathématique indépendante](../audits/receipts_full_cpp_20260905/portal_next_step_review.md)
valide le calendrier proposé sous ces prémisses ; elle précède le code et
ne constitue pas son audit d'exécution. La [preuve FULL](AUDIT_NIVEAUX_GABRIEL_20260905.md)
explique pourquoi les niveaux Gamma silencieux ne sont pas des niveaux
de sortie nécessaires.

Les catalogues sont indexés par label et ordonnés par comparaison rationnelle
exacte. Les niveaux sémantiquement égaux forment un seul lot. Pour chaque
directe du lot, seuls les retraits de ses essentiels demandent une ancienne
racine : au plus quatre facettes, quel que soit K. Retirer un intérieur
conserve la boule ; cette facette naît dans le lot et ne peut être un parent
strict. Deux directes régulières distinctes ne peuvent partager une telle
facette au même niveau.

Les anciennes racines restent figées pendant toute cette première passe.
Une union locale groupe les connexions simultanées. Les minima reçoivent
leurs identifiants avant les vraies multifusions, chacune ayant au moins
deux parents pré-lot. Après fermeture du lot entier, toutes les facettes
des directes deviennent des alias et chaque directe garde son ancre, même
si elle ne produit aucun nœud. Les alias historiques sont résolus par les
successeurs : aucune union n'est décidée sur la seule couverture ponctuelle.

Une facette stricte inconnue F entre au portail. Si sa MEB ne contient
aucun intrus, son minimum manque ; si elle en contient exactement un,
une incidence directe antérieure manque. Ces deux cas sont refusés. Avec
au moins deux intrus certifiés z,w, F+z a exactement la MEB et le support
de F, et w reste un intrus. Cette première coface ne demande donc **ni
nouvelle MEB ni nouvelle recherche spatiale**. On retire un essentiel et
on ajoute w ; le niveau décroît strictement. Les pas suivants vérifient
chaque nouvelle MEB et, hors terminal autorisé, sa coquille et ses intrus.

Le terminal doit être une directe de même label et même niveau exact,
déjà traitée à un niveau strictement antérieur. Son ancre, éventuellement
historique, est normalisée vers la racine active pré-lot. Sur E5, le portail
AC aboutit à CDE puis remonte son successeur après ADE ; il ne fabrique ni
minimum AC à 33/2 ni fausse fusion à 24. Les sept minima et trois
multifusions ternaires sont testés avec leurs niveaux et parents exacts.

## 3. État retenu, plafonds et échec

Pendant l'ordre, l'appel possède deux index de catalogues, leurs permutations
chronologiques, un index PointId–Morton, des alias de facettes vers les
nœuds historiques, les successeurs, les lots de sortie et une union locale
au lot. Les records de catalogue référencent les événements immuables
fournis, sans recopier leur payload. Les labels des facettes incidentes sont
conservés même lorsqu'ils n'ont aucun nœud de sortie.

La descente ne possède **ni cache de cofaces ni chemin enregistré** : un
label courant, une boule courante, les témoins et la pile du helper suffisent.
Seul le rattachement de la facette demandée est mémorisé. Les helpers MEB
et intrus F gardent leur état budgétaire à l'échelle de l'ordre ; leur
`run()` et leur matérialisation du cœur ne sont jamais appelés. Les tests
exigent `core_records=core_facets=added_cofaces=0` avec un portail non vide.

Les plafonds sont nuls par défaut et chargés prospectivement :

| Plafond | Travail ou stockage compté |
| --- | --- |
| `max_points` | Taille du domaine vérifiée avant les index locaux |
| `max_input_records` | Somme des deux catalogues fournis |
| `max_aliases` | Installations de nouvelles clés de facettes |
| `max_face_visits` | Retraits essentiels en première passe et tous les retraits en seconde passe |
| `max_portal_requests` | Facettes strictes inconnues à résoudre |
| `max_chain_steps` | Remplacements effectivement commencés, y compris le premier |
| `max_meb_calls` | Appels physiques au helper MEB, avant l'appel |
| `max_meb_supports` | Candidats de support facturés par le helper F |
| `max_query_nodes` | Nœuds des requêtes spatiales facturés par le helper F |
| `max_successor_steps` | Lectures logiques du tableau de successeurs et écritures de compression |
| `certificate.*` | Lots de sortie, nœuds et références de parents |

Pour une normalisation de profondeur d, le compteur de successeurs facture
3d+1 opérations : la lecture terminale est incluse, même pour une racine
déjà active. Ce n'est **pas** un budget de toutes les opérations DSU : les
unions locales du lot et les installations de nouveaux successeurs n'y
sont pas incluses. Leur domaine et leurs appels sont bornés par les visites
de faces et les nœuds. Aucun compteur n'est assimilé à un temps CPU maximal.

Tout refus rend un certificat invalide avec toutes ses arènes vides. Le
travail déjà effectué reste visible dans les statistiques. `bad_alloc` et
`length_error`, y compris pendant la validation finale structurelle,
deviennent les refus nommés `full_gabriel_allocation_failed` et
`full_gabriel_size_overflow`. Les données sources ne sont pas modifiées.

Les lots temporaires coexistent avec la forêt pendant sa validation finale.
Ce premier constructeur n'est ni streaming ni muni d'un budget RSS certifié.
Les plafonds de records ne bornent pas la capacité réservée des conteneurs,
les frais d'allocation ou toutes les copies temporaires. Le nombre de
minima n'est pas borné par n : aucune extrapolation aux dizaines de millions
de points ne découle de la compacité du certificat final.

## 4. Qualification et limites de représentation

Le [juge FULL](../tests/full_gabriel_gate.cpp) utilise
[full_gamma.hpp](../oracle/full_gamma.hpp), oracle OBig indépendant borné
à huit points : toutes les K-facettes sont activées, y compris les isolées,
puis toutes les cofaces de connexion. Les catalogues réellement générés
sont comparés exhaustivement à cet oracle avant de juger les forêts.
Les coupes ouvertes et fermées comparent la partition des minima étiquetés
et la couverture ponctuelle de **toutes** les facettes de chaque composante,
pas seulement les nombres de racines ou leurs ensembles de points.

Les fixtures couvrent triangles aigu/obtus, simultanéité, E5, domaine u16,
permutations physiques et des catalogues, identités non monotones et extrêmes,
K=1, K=n et s=8,10,12. Les plafonds à la valeur observée, à cette valeur
moins un et à zéro sont exercés non trivialement. Les refus causaux incluent
terminal absent, niveau terminal faux, minimum absent, faux parent du lot
et coquille réellement interrogée dans un catalogue explicitement invalide.
La [porte mémoire](../tests/full_gabriel_allocation_gate.cpp) balaie toutes
les allocations observées d'un appel E5, avec défaillance persistante et
retour sans préfixe, puis vérifie qu'une nouvelle tentative réussit.

Les [reçus ciblés](../receipts/full_gabriel_20260905/README.md) ferment
7/7 CTests Release et 7/7 ASan/UBSan sur des builds neufs, lecteur structurel
inclus. Ils conservent la première tentative instrumentée échouée sous
traçage, puis la reprise des mêmes binaires et options dans le contexte
ROOT non tracé, avec LeakSanitizer toujours actif. Le positif compare
67 exécutions d'ordres et 1 492 coupes ; les refus ciblés sont au nombre de
80, et les 102 allocations observées sont toutes balayées séparément.
Les 67 exécutions sont 40 couples nuage/ordre et 27 répétitions sous
permutation, pas 67 configurations géométriques indépendantes. L'oracle
n≤8 ne qualifie pas dynamiquement le producteur à K9 ou K10 ; le témoin
structurel K10 relève d'une autre porte.

Une [qualification externe distincte](../audits/PRODUCTEUR_FULL_GABRIEL_COURANT.md)
ferme ensuite 100 ordres en deux représentations et 16 506 coupes par build
O2/ASan-UBSan depuis des catalogues rationnels indépendants, n≤7. Elle
exerce une seconde itération nommée et réfute trois mutations privées.
Ce résultat ne transforme pas nos propres planchers en une couverture
qu'ils n'ont pas exécutée, ni ces petites fixtures en qualification massive.

Ces tests ne sont ni la suite F complète, ni une preuve exhaustive des
entrées de grande taille. Les coupes FULL compactes ne reconstruisent pas
le catalogue des facettes Gamma silencieuses ni le carrier marqué.
Les poids de l'Algorithme 1 restent un supplément distinct : ils ne se
réduisent pas aux seuls minima FULL. Les ancres inter-K, la verticale,
l'autorité terminale d'une tour, l'archive et la CLI FULL restent à raccorder.
Le contrat 50k porte toujours sur **toute** cette tour, pas sur ce composant
horizontal isolé. Aucun résultat 1 seconde, 100 ms ou G4 massif n'est acquis.

Les positifs actuels exigent la régularité de tous les sous-ensembles du
petit nuage oracle. Ils ne qualifient pas l'extension de fenêtre avec des
irrégularités hors fenêtre. Le plancher E5 exerce un portail et une descente,
mais n'impose pas encore une deuxième itération nommée ni un état
intermédiaire à exactement un intrus. Le code permet ce dernier cas après
l'entrée du portail ; l'exclusion de zéro ou un intrus ne concerne que
la facette stricte inconnue initiale. Ces limites de couverture dynamique
ne sont pas masquées par le nombre global de contrôles.
