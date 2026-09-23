# Phase A de FULL : la racine vivante est un maximum d'événements

23 septembre 2026. Contrelecture indépendante de
[`PHASE_A_GRAPHE_TEMPOREL_20260923.md`](PHASE_A_GRAPHE_TEMPOREL_20260923.md)
et de `src/tower/forest/full_ball_tower.hpp` (SHA-256
`77588b5d1880c60aa881fdc6ffb1e847494f9e8a4a194b12a0885ec311ebf865`,
produit `ec6d1b74`). **Lemme et schéma d'algorithme, pas port du moteur ni
qualification GPU.** Le résultat utile est plus fort que « le graphe donne
les mêmes composantes » : sous les gardes du produit, il donne aussi les
*IDs historiques canoniques* sans rejouer les fusions niveau par niveau.

## Modèle et deux seuils distincts

Fixer un ordre K. Chaque bloc de `programs[K]` est un sommet `(K, BallId)` ;
K1 possède en outre les sommets des sites initiaux, dans l'ordre du domaine.
Pour chaque facette effectivement émise par `visit_block_at`, placer une
arête du bloc source vers le **bloc terminal résolu**, ou vers un site pour
K1. Son poids est le rang du niveau exact du bloc source, pas un flottant.
Le tableau de BallId terminaux existe dans la **voie statique** de phase 0 ;
la voie séquentielle avec cache de jetons temporels ne livre pas directement
ces extrémités et n'est pas couverte par cette construction sans adaptation.
La cible a un niveau strictement inférieur ; l'ordre du programme est
croissant en niveau exact puis en clé. Deux ordres K ne partagent aucun
sommet, même quand ils emploient le même BallId. Les arêtes multiples sont
permises ; les contributions ne sont pas des arêtes.

Écrire `C<(v, λ)` pour la composante de v utilisant seulement les arêtes de
poids **strictement inférieur** à λ, et `C≤(v, λ)` pour celle après toutes
les arêtes du plateau λ. Les cibles d'un bloc à λ se lisent dans `C<` :
utiliser `C≤` effacerait les parents d'une multifusion. Deux blocs à λ
appartiennent au même groupe de lot exactement si leurs `C≤` sont égales.
Pour ce groupe, les parents sont les `C<` distinctes de toutes ses cibles.
Un bloc sans facette est isolé à sa naissance. Une naissance sans parent
reste soumise à la garde produit : groupe singleton et une contribution.

Une forêt couvrante **minimale** en rang exact conserve les composantes de
tous les préfixes ouverts et fermés. En effet, si une arête du préfixe
reliait deux composantes de la forêt restreinte au même préfixe, elle
remplacerait sur leur chemin une arête plus lourde, contradiction. Un
départage déterministe par ordinal d'arête peut rendre la forêt unique ; il
ne doit pas transformer un plateau en actions intermédiaires publiées.
Une forêt couvrante quelconque ne possède pas cette propriété.

## Lemme du maximum d'ID

Marquer le sommet de chaque site initial K1 par l'ID de son nœud initial.
À chaque groupe de plateau, le produit crée un nœud si et seulement si
son nombre de parents est **zéro ou au moins deux**. Marquer alors le
premier bloc du groupe par l'ID de ce nouveau nœud. Une continuation à un
parent, même avec contribution, n'ajoute pas de marque.

> Pour toute composante vivante au seuil ouvert λ, sa racine historique
> canonique est le **maximum des IDs marqués** dans cette composante.

Preuve par induction sur les plateaux. Initialement chaque site est seul
et porte son ID ; pour K>1, aucune composante n'existe avant la première
naissance. Un groupe sans parent crée un nœud, dont l'ID est l'unique marque
nouvelle de sa composante. Avec un parent, le produit garde le nœud de ce
parent : aucune marque nouvelle, donc le maximum reste inchangé. Avec au
moins deux parents, il crée un nœud après tous leurs nœuds ; son ID est
strictement supérieur à chaque marque des composantes réunies. Les groupes
distincts d'un même plateau sont disjoints et ne lisent que l'état ouvert.
L'induction est donc valable aussi pour un plateau à plusieurs blocs. Le
lemme concerne les **nœuds vivants de phase A**, pas l'image verticale à
un ancien seuil : `anchors[ball]` doit rester son jeton historique, et
`current.next` doit conserver les successeurs.

## Reconstruction hors des barrières de niveau

1. Préparer des offsets de facettes par bloc et conserver les cibles
   terminales par ordinal. La liste complète reste nécessaire aux gardes,
   aux compteurs et aux contributions ; la forêt minimale ne remplace que
   les requêtes de connectivité. Une construction déterministe possible de
   cette forêt est Borůvka avec clé `(rang, ordinal)` : au plus
   `⌈log₂ V⌉` tours et `O(E log V)` inspections d'arêtes si chaque tour
   balaie E arêtes. Ce n'est pas encore un résultat de performance v9.
2. Construire une hiérarchie de composantes pondérée qui répond à `C<`,
   `C≤` et à l'agrégat maximum des marques, **sans rescanner E arêtes pour
   chaque niveau**. Le coût, la mémoire et la profondeur parallèle de
   cette construction doivent être publiés séparément ; la seule forêt
   minimale ne livre pas automatiquement ces requêtes.
3. Pour chaque bloc à λ, calculer son label `C≤`, puis regrouper par
   `(λ, label)`. Les cibles fournissent les labels parentaux `C<`. Trier
   les groupes par `(λ, premier ordinal de bloc)`, les parents distincts,
   et les contributions dans l'ordre des blocs du programme. Cette règle
   reproduit le `find` à représentant minimal de `order_lot`.
4. Préfixer les groupes qui créent un nœud pour leur attribuer exactement
   les IDs du produit : les sites K1 précèdent les blocs, chaque naissance
   ou multifusion ajoute un ID, aucune continuation n'en ajoute. Après
   ce préfixe, placer les marques et demander le maximum de marques dans
   chaque composante parentale **ouverte** : ce maximum est l'ID du parent
   canonique. On peut alors renseigner `parents`, `next`, `birth_ball`,
   `anchors`, les niveaux et les contributions à leurs emplacements
   déterministes. Les groupes avec un parent et une contribution émettent
   bien une action ; les groupes sans action restent comptés comme inertes.
   Une *batch* est celle du plateau original, non une batch par composante.

Il n'y a pas de circularité dans l'étape 4 : le nombre de parents d'un
groupe dépend seulement des labels de composantes, non de leurs IDs de
nœuds. Les maxima sont calculés **après** attribution des IDs, mais ne
changent ni les groupes ni les choix naissance/continuation/fusion.
Lorsque tous les groupes d'un plateau sont traités ensemble, deux groupes
ne peuvent écrire `next` pour la même racine antérieure : ils partageraient
alors une composante `C<` et seraient le même groupe.

## Gardes et porte d'industrialisation

Ne pas déduire de la forêt la validité d'une cible : vérifier l'ordinal,
le BallId, la fenêtre de rang, l'ancre déjà née et le niveau strict pour
**toutes** les facettes avant de contracter. Préserver l'ordre de priorité
des échecs entre K, les échecs de représentation u32, le nombre exact de
lots singleton/groupés, les actions inertes et la fermeture d'un plateau
avant publication. Le graphe d'un K ne peut pas absorber celui d'un autre ;
les IDs de populations (phase B) et les images verticales (phase C)
requièrent toujours leur ordre canonique et leur propre coût. Une méthode
de requêtes de composantes à `O(E × nombre_de_niveaux)` annulerait tout
l'intérêt de la refonte. À K10 sur le seul nuage mesuré par le sidecar,
2 081 320 lots sont singletons : une barrière GPU par niveau serait
particulièrement coûteuse, même si la forêt était construite vite.

La porte minimale est un sidecar sur les **mêmes catalogues exacts** qui
compare octet pour octet tous les nœuds, niveaux, parents, `next`, ancres,
contributions, batches, populations et images verticales, ainsi que les
statuts et compteurs, à la voie produit. Inclure K1 sans bloc, K>1 avec
naissance isolée, plateau de multifusion à trois sites, mêmes BallId dans
deux K, facettes doublées, continuation contributive, bloc inerte et cible
géométrique échangée pour son terminal. Publier travail total et mémoire
(construction des arêtes, forêt, hiérarchie, requêtes, tri, émission), puis
la croissance appariée 8k/16k/32k, les trames entières sans sol et brutes
de plusieurs séquences, K5/K10. Ni ce lemme ni l'éventuel parallélisme
de phase A ne résolvent q3/q4, la complétude des clés, la phase C, ni le
contrat G4 sous une seconde.

[`check_phase_a_temporal_max_id_20260923.py`](check_phase_a_temporal_max_id_20260923.py)
compare, sur **3 000 historiques abstraits déterministes**, une DSU
chronologique, les groupes/parents/IDs reconstruits par seuils et les
partitions préfixes d'une forêt de Kruskal. Il ne lit ni le catalogue ni
le moteur ; succès de ce test = contrôle combinatoire du lemme, **pas**
qualification de FULL.
