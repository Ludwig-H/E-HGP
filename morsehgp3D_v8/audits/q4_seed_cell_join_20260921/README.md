# Audit A — joindre les blocs de graines et les cellules q4

21 septembre2026. Prototype indépendant CPU, entrée u16 exacte,
`public_status=not_claimed`. Écritures limitées à audits/. GCP non utilisé.

## Contrat et objets

La [piste constructeur](../../docs/Q4_BLOCS_SEEDS_PISTE_20260921.md)
est mathématiquement sûre. Pour une arête fixe, la primitive existante
`Q4LocalGeometry::node_bounds(X,C)` borne la puissance sur la boîte de
graines X et la cellule de centres C. Un intervalle de signe strict
exclut cette incidence. Un zéro reste actif, y compris aux frontières
fermées. Le domaine Positive reste préparé sur toutes les complétions
de la lentille ; filtrer les seules graines aiguës pour le construire
perdrait des supports positifs.

Le parcours ne doit pas refaire une recherche depuis la racine pour
chaque incidence singleton/feuille. Il appelle directement le balayage
du fragment trouvé, avec les mêmes contrôles de propriété, positivité,
canonicité, frontières, compte strict et coquille complète.

Une difficulté de stockage est réelle : une descente commençant par C
peut traiter x1/C1, x2/C1, puis x1/C2. Un unique emplacement de famille
oblige alors à reconstruire x1. Le cas géométrique
a=(15,20,20), b=(24,23,20), x1=(20,15,20), x2=(23,16,20)
le réalise : les deux graines sont aiguës, ab est leur arête maximale,
et leurs droites sont identiques, `80+240ξ=0`.

**Solution essayée : un cache paresseux par bloc disjoint de graines.**
Une antichaîne de l'index partitionne les graines en blocs de taille
au plus B. Un bloc possède son cache de familles et mène à terme sa
DFS bloc×atlas. Chaque graine appartient à un bloc unique ; sa famille
est construite au plus une fois par arête. Les refus sont également
mémorisés. Le cache est libéré après le bloc, sans liste de cellules
par graine. B limite la mémoire, jamais la recherche.

Ce choix paie une nouvelle entrée dans l'atlas par bloc et perd les
rejets collectifs au-dessus de ces blocs. Il n'est donc pas un gain
automatique. Confier un bloc entier à un worker suffit pour la propriété
du cache ; redistribuer ses incidences exigerait une construction
synchronisée des familles et des buffers privés.

La DFS scinde un seul facteur à la fois. Sa pile logique comporte au plus
`1+hX+3hC` descripteurs, soit181 pour les limites générales48/44.
C'est une borne de pile, pas du travail, du nombre de blocs ni du RSS.
La sonde réserve physiquement181 cadres (4 344octets), même lorsque la
profondeur maximale7 suffit à borner la pile logique par70.
Les quatre frères de l'atlas sont réservés avant leurs descendants :
ce vecteur n'est **pas** un préordre à `escape`. Le parcours suit
explicitement `children+q`. Un résumé des feuilles vivantes se calcule
en sens inverse des IDs, les enfants étant postérieurs au parent.

## Instrumentation indépendante

[probe.cpp](probe.cpp) inclut une copie explicite, inchangée, de
[q4_local.cpp](snapshot/q4_local.cpp), SHA256
`0d52f1446b1a4d4ab102008abb4da8f6556681cf2973f6ad2435a87019ab0b45`.
L'option de compilation `-fno-access-control` sert seulement à inspecter
l'atlas privé et appeler son balayage. Les définitions de classes et
leurs dispositions mémoire restent inchangées. Les autres symboles
proviennent des bibliothèques32 épinglées, lues sans reconstruction ;
cette sonde n'est pas une qualification du produit33.

Trois parcours consomment le **même atlas** : la référence par graine,
un contrôle supprimant seulement les sous-arbres sans feuille vivante,
puis le produit X×C avec cache borné. Le contrôle intermédiaire empêche
d'attribuer à la borne géométrique coûteuse un gain dû à un simple bit.
La préparation de l'atlas et de ce résumé est publiée séparément ; il
faut la payer dans toute comparaison de coût complet.

Les multiensembles de supports, clés exactes, profondeurs et coquilles
sont comparés sans déduplication de boules. Les petites entrées ont en
plus un oracle cartésien rationnel, explicitement repris de
[l'audit31](../q34_global_contract_20260921/oracle.py), indépendant de
l'atlas et de ses balayages. Les grandes entrées comparent les parcours
du même noyau ; elles ne constituent pas un nouvel oracle exhaustif.

## Preuves et mesures

Les quatre captures sont closes, sans modification des bibliothèques32 :

| Capture | Commandes / appels C++ | Portée |
| --- | ---: | --- |
| [qualification_r1](receipts/qualification_r1/MANIFEST.json) | 528 /522 | 14 fixtures, K3/5/10, grains1/8/64, Release et Clang ASan/UBSan/LSan |
| [refined_r2](receipts/refined_r2/MANIFEST.json) | 260 /252 | Neuf fixtures, atlas raffiné, grains8/64, mêmes deux compilations |
| [lidar_r1](receipts/lidar_r1/MANIFEST.json) | 48 /48 | Ancre0, rangs32/512, trois scans et trois tailles, K5/10 |
| [lidar_anchors_r1](receipts/lidar_anchors_r1/MANIFEST.json) | 162 /162 | Ancres1000/3000/6000, rangs4/16/64, mêmes scans/tailles/seuils |

Les774 appels sur petites entrées comparent l'oracle cartésien complet,
dont387 sous sanitizer. Ils émettent572 supports cumulés, avec une
coquille maximale30. Les522 premiers appels utilisent les options par
défaut ; ils ne raffinent pas les petits atlas. Ce constat a motivé une
capture **distincte** remplaçant uniquement les options par
`Positive, profondeur4, budget341, testsZ16, feuille2, clipping=true`.
La [copie instrumentée](receipts/refined_r2/source.cpp) et son delta
unique sont explicitement épinglés ; la sonde initiale reste inchangée.
Par binaire, ces nouveaux appels exercent4 098 subdivisions de cellules,
2 028 réutilisations de familles,1 844 bornes avec zéro et100 émissions.
Les fragments balayés, comptes, tris et coquilles restent identiques
entre les trois parcours ; les capacités des buffers sont distinctes.

La première capture raffinée `refined_r1` est conservée séparément.
Le constructeur avait alors commencé34 et changé deux en-têtes pendant
notre chantier : ces déclarations nouvelles étaient figées dans r1,
mais associées à la bibliothèque32. Leurs résultats exacts ne sont pas
promus comme qualification d'un ensemble de sources cohérent. La reprise
r2 emploie les16 en-têtes32 extraits de `d1b4dbc6`, chacun vérifié contre
son hash antérieur, et compilés en priorité depuis
[snapshot/include/](snapshot/include/).

La [première relecture finale](preflight_validation/VALIDATION.json) a
refusé à juste titre des dépendances produit qui avaient changé après
la capture initiale. Elle reste en échec. Le
[lecteur de captures closes](read_closed.py) vérifie maintenant chaque
hash dans son fichier courant ou sa copie explicite adressée par hash,
puis applique les mêmes contrôles d'entrées, sorties et oracles. Les
sources34 ne sont jamais prises pour celles32 ; les archives ne changent
aucun reçu ni verdict géométrique ancien.

Le modèle [math_checks.py](math_checks.py) complète ces appels :
1 296 configurations structurelles, plus deux cas dédiés ; trois mutants
de cache/parcours réfutés. Il vérifie10 368 identités de puissance sur
96boîtes. Deux contre-exemples géométriques explicites imposent de garder
les contacts et le minimum intérieur : les seuls coins de X peuvent
être tous positifs (324), avec un minimum de−108 et une graine sur la
sphère. Le [préflight du modèle](preflight_math/FAILURE.json), dont la
fixture de pile n'atteignait pas la borne annoncée, est conservé avec
sa source ; la fixture corrigée atteint23cadres. Aucun défaut produit
n'est inféré de cet échec du harnais.

## Résultat LiDAR : le contrôle simple suffit d'abord

Les trois scans LiDAR restent séparés, sans appariement ni hypothèse
d'alignement des points. Les arêtes sont choisies par rang de voisinage
dans le préfixe8k, puis conservées à16k/32k. Il s'agit d'arêtes choisies,
pas d'un estimateur pondéré du front global ni d'un contrat de tour.

Les48 premiers appels n'émettaient aucun support. Ils sont conservés,
puis le choix a été étendu par une recette explicite, indépendante des
réponses du moteur. Sur210 appels au total, un seul émet quatre supports.
Cette faible population de sorties interdit d'en déduire le coût du
front complet. Les sorties et tout le travail de balayage concordent ;
les grandes entrées n'ont pas un nouvel oracle exhaustif.

Pour éviter de compter deux fois les grains8/64 de la première série,
le tableau suivant porte sur les198 appels à grain64 :

| Travail cumulé sur les arêtes choisies | Référence | Sous-arbres vivants | Produit X×C |
| --- | ---: | ---: | ---: |
| Familles préparées | 4 550 | 314 | 249 |
| Visites de carte par graine | 36 622 | 1 294 | 0 |
| Bornes de droite par graine | 9 521 | 726 | 0 |
| Produits visités | 0 | 0 | 5 274 |
| Bornes boîte X×C / singleton×C | 0 /0 | 0 /0 | 2 163 /594 |
| Tests spatiaux de graines | 20 790 | 5 027 | 5 138 |
| Tests ponctuels de graines | 7 088 | 1 025 | 1 017 |
| Balayages de feuilles | 315 | 315 | 315 |
| Lectures actives / comparaisons de tri | 7 272 /5 115 | 7 272 /5 115 | 7 272 /5 115 |

Le produit initialise aussi8 704 entrées de cache réparties en190blocs ;
le pic de capacité du cache est16 384octets, la pile logique maximale13.
Ces capacités excluent index, atlas, temporaires, allocations de contrôle
et copies de sorties conservées par l'audit. Les atlas du résumé disposent
d'un tableau de compteurs u64, pas encore d'un bit compact par cellule.

**Priorité utile : sauter un atlas sans feuille utile avant de générer
ses graines.** Ici110 des198 atlas sont vides de feuilles. Le constructeur
dispose déjà de `atlas.work().leaf_cells==0` : ce premier test est O(1)
après préparation et ne demande pas de tableau. Le résumé des branches
vivantes peut ensuite être évalué séparément. Le partage X×C testé ne
réduit que peu les préparations au-delà de ce contrôle et ajoute de
nombreux tests ; ces cas ne justifient pas son port immédiat par défaut.

Cette recommandation ne supprime ni les partitions préparatoires ni
l'aval q4. La [synthèse intégrale](LIDAR_SUMMARY.json) garde les groupes
par taille et tous les coûts discrets. Les chronos bruts sont conservés,
avec préparation commune séparée et charge concurrente ; aucun gain de
temps global n'est revendiqué. Le contexte O(1), les comparaisons finales
et l'impression JSON ne sont pas inclus dans les durées des trois modes.

[read_closed.py](read_closed.py) relit les hashes, les identités commande/reçu, les
entrées, les comparaisons et les oracles ; [analyze.py](analyze.py) produit
le tableau. [VALIDATION.json](VALIDATION.json) ferme leurs lectures
normal/−O. Les reçus figés sont conservés comme preuves ; seul le
[dialogue actif](../DIALOGUE_COURANT.md) est condensé au fil des réponses.
