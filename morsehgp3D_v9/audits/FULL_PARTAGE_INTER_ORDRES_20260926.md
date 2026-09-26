# FULL : ce qui peut réellement être partagé entre les ordres

26 septembre 2026. Lecture de `92c709bc8`, sans modification du moteur,
compilation, nouveau benchmark ou GCP. Profil grille entière 1 mm/u18.
Cette note complète les plans existants ; elle ne revendique ni une nouvelle
qualification ni un gain mesuré.

## Conclusion pour le développeur

**Ne pas chercher K copies d'un même calcul géométrique : elles n'existent
pas sous cette forme.** La prochaine architecture utile est un seul espace
de *blocs actifs*, conservant K dans chaque identité, et un seul index
d'événements par forêt qui sert à A, aux verticales et à l'écriture FULL.
Le raccord à préciser maintenant est **A → tableaux finaux**, sans reconstruire
une seconde fois les mêmes événements dans l'encodeur séquentiel.

Les idées de cibles batchées, saut au centre, naissance descendante et graphe
temporel sont déjà documentées. Les nouveautés de cette lecture sont les
points d'intégration ci-dessous : index compact des blocs actifs, réutilisation
du même index temporel pour C, et formulation parallèle du contrôle `live`
de l'encodeur. Aucune de ces trois variantes n'a été exécutée ici.

## 1. Le budget réellement observé après le port q3

Sources brutes : `receipts/g4_q3_payload_20260926/vm/probe_{0,5,8,11,20,23}.stdout`.
Il s'agit des bras ON, sur entrée préparée en mémoire, chacun `frames=1`.
Les valeurs ci-dessous sont des passages individuels, pas des médianes.
00/01/02 sont trois trames de la **même séquence 08** ; b00 est 00 brut.

| entrée | K | chaîne FULL ms | tour ms | phase 0 ms | lots exposés ms | images exposées ms | encodage ms |
|---|---:|---:|---:|---:|---:|---:|---:|
| 00 sans sol | 5 | 918,275 | 289,648 | 83,690 | 103,492 | 16,353 | 39,455 |
| 01 sans sol | 5 | 749,028 | 235,098 | 68,453 | 83,345 | 13,339 | 27,730 |
| 02 sans sol | 5 | 969,243 | 303,407 | 78,833 | 116,768 | 17,534 | 39,938 |
| 00 sans sol | 10 | 2 950,635 | 1 209,185 | 732,542 | 163,456 | 47,468 | 109,253 |
| b00 brut | 5 | 1 942,923 | 635,588 | 163,522 | 251,356 | 48,958 | 72,799 |
| b00 brut | 10 | 5 938,304 | 2 402,830 | 1 255,036 | 395,672 | 94,059 | 203,589 |

Les temps de lots/images sont ceux de la fenêtre recouverte du produit,
pas la somme des temps propres par K. Un nouveau calendrier modifierait ce
recouvrement : les soustraire n'est pas une prédiction de gain.

À 00/K5, phase 0 = collecte 18,528 + regroupement 12,654 + champ historique
`static_sort` 3,779 + résolution 48,676 ms, à l'arrondi et à l'administration
près. Ce dernier champ inclut l'index de graines en mode haché. À K10, la
résolution seule atteint 557,589 ms, contre 10,074 ms pour `static_sort`.
Le nombre de MEB passe de 1 289 447 à 11 309 383 ; les visites d'intrus de
31 708 173 à 403 429 577. Le tri n'est pas le verrou principal.

À 00/K5, les 1 541 750 nœuds, 897 776 contributions et leurs liens doivent
rester explicites. La queue mesurée contient encore populations 11,269,
banque 0,872 et encodage 39,455 ms, en plus des images. Même annuler toute
la tour laisserait 628,627 ms de cette chaîne : cette proposition est
indépendante du front q3/q4, pas une solution isolée aux 100 ms.

## 2. Ce qui est déjà partagé — ne pas le réinventer

Dans `src/tower/forest/full_ball_tower.hpp` :

- `validate_catalogue()` (~1800–2044) construit une fois l'index de clés,
  le tri de niveaux, les rangs exacts et les programmes filtrés par K.
- `count_block_at()/visit_block_at()` (~2843–2905) : une boule régulière
  avec m = p + q ne travaille qu'en K=m−1 (q représentants, q≤4), puis
  K=m (contribution sans représentant), si ces ordres sont demandés.
- `prepare_static_order()` (~2541) groupe déjà les facettes identiques
  d'un ordre et utilise les populations complètes comme graines.
- `compute_population_offsets()/order_populations()` (~1453–1548)
  attribuent déjà une seule population partagée à chaque boule, dans
  l'ordre de sa première contribution. Une coquille étendue peut
  contribuer plusieurs fois, sans dupliquer cette population.

Deux facettes de cardinalités différentes ne sont pas la même requête.
L'identité de composante et la fenêtre terminale dépendent de K : conserver
`(K, facette)` et `(K, BallId)`, jamais fusionner les DSU de deux ordres.

Les [cibles batchées et le graphe](ARCHITECTURE_GPU_100MS_Q34_FULL_20260924.md)
et le [modèle événementiel A](b_full_phase_a_work_20260926/README.md) sont
des plans antérieurs. Le [sidecar D5](../docs/d5_conception_20260923/ANALYSE_SIDECAR_C.md)
contient déjà l'intrus/census lu dans le catalogue, le saut au centre et
les images depuis une naissance descendante. L'index des selles isolé a
été mesuré négatif : ne pas le ressortir comme gain nouveau. Le census
catalogué ne peut aider un état non terminal à K=Kmax : sa clé serait
hors de la fenêtre globale du catalogue.

## 3. Un espace de blocs actifs, pas K tableaux denses de catalogue

Aujourd'hui `OrderState::anchors` (~758), initialisé par `order_lots()`
(~1355), réserve `balls.size()` u32 pour **chaque K**, même si une boule
n'appartient pas à cet ordre. Le programme utile est beaucoup plus petit.

Pour une boule régulière b, poser `lo(b)=p+q−1`. Deux emplacements suffisent :
`slot(b,K)=2*b+(K−lo(b))`, uniquement après avoir certifié
`lo(b)≤K≤min(Kmax,lo(b)+1)`. Les coquilles étendues prennent des emplacements
CSR distincts, sur leur fenêtre entière, avec leur `ShellTable` actuelle.
Les cases régulières inutilisées restent absentes ; K1 garde ses sites
initiaux séparés. Cela fournit directement les sommets auxiliaires du
graphe, les ancres et la conversion BallId terminal → sommet du bon ordre.

Ordres conservés séparés, mais stockage/réservation partagés. Pour les
seules ancres denses, hors autres structures et coquilles étendues :

| catalogue 00 | K tableaux actuels | deux cases par boule |
|---|---:|---:|
| K5 : 1 306 696 boules | 26 133 920 octets | 10 453 568 octets |
| K10 : 5 512 670 boules | 220 506 800 octets | 44 101 360 octets |

Ce sont des capacités théoriques u32, **pas un gain de temps ni un pic RSS
mesuré**. Ne pas ajouter une table inverse dense K×R qui annulerait
l'économie. Le calcul direct fonctionne parce que la fenêtre régulière
est contiguë ; pour l'étendue, payer les offsets et rangs exacts. Les
indices arithmétiques intermédiaires et les offsets doivent être 64 bits.
Un rangement global par programme peut mieux servir les écritures GPU,
mais doit alors conserver cette bijection et son coût.

## 4. Réutiliser l'index événementiel A pour les images C

Le lemme de naissance descendante est celui de D5, pas une nouvelle preuve
de géométrie. Le raccord concret à `order_images()` (~1557) est :

1. Pour chaque nœud horizontal v, pointer vers son premier parent s'il en
   a ; une naissance pointe vers elle-même. Par doublement, retrouver
   une naissance descendante `birth(v)`. Les IDs des parents sont strictement
   antérieurs : pas de cycle. Cela remplace la propagation séquentielle
   de `o.draft.lower_nodes`.
2. Pour K≥2, prendre b=`birth_ball[birth(v)]`, puis demander au **même index
   temporel** que A la composante fermée du sommet `(K−1,b)` au rang de v.
   Son dernier événement à cette coupe, après résolution des continuations,
   fournit le node ID historique.
   K1 publie absent.
3. Pour chaque arête parent p→v, comparer aussi la requête issue de
   `birth(p)` à celle de v, à la coupe de v. Cela conserve le contrôle
   de naturalité actuel, en parallèle, sans une nouvelle DSU inférieure.

Justification : pour t≤u et une ancre née avant t,
`root_closed(root_closed(a,t),u)=root_closed(a,u)`.
Dérouler les images des parents jusqu'à une naissance donne donc la même
image. Une naissance à K vérifie K≥p+q_min, donc le bloc b existe à K−1 ;
la coquille étendue garde sa vraie fenêtre. La racine **finale** de K−1
n'est pas une image valide à un niveau ancien. La coupe doit être fermée,
et les événements silencieux/plateaux de A restent indispensables.

La seule nouveauté architecturale proposée ici est de **ne pas construire
un deuxième index d'ancêtres** : l'index ouvert/fermé nécessaire à A
répond aussi à ces requêtes. Les tables O(V log V) et leur construction
du plan A restent payées. Avec V nœuds et P parents, naissance par sauts
O(V log V), images/naturalité O((V+P) log V) dans cette variante ; pas
d'affirmation de borne globale sous-quadratique en n.

## 5. Éviter le deuxième parcours chronologique de construction FULL

`order_new_node()` (~1160) a déjà décidé IDs et successeurs ; `order_lot()`
a construit parents/contributions dans `FullCoverageFlatDraft`. Puis
`full_coverage_certificate.hpp::build_from()` (~285–397) recompte les
tailles, **rejoue les lots avec un tableau `live`**, reconstruit les mêmes
nœuds/successeurs et recopie parents/contributions. `finish()` (~723)
parallélise uniquement entre ordres, pas à l'intérieur du plus gros ordre.

Le graphe événementiel doit fournir directement un tableau d'actions
canonique `(K, rang, premier bloc)`, plus offsets parents/contributions.
Les actions sans exactement un parent créent un nœud ; préfixes donnent
leurs IDs, les tailles et les emplacements finaux. Les continuations
gardent le segment de leur parent et toutes leurs contributions datées.
Chaque parent de fusion reçoit son successeur unique. Écrire les tableaux
FULL finaux à partir de ces offsets évite la construction intermédiaire,
sans remplacer la sortie par un format compact à décoder plus tard.

**Le contrôle `live` ne doit pas être simplement supprimé.** Une formulation
batchée équivalente est possible, à qualifier :

- ordonner les incidences d'un parent par `(K,parent,lot,action)` ;
- sa création précède strictement chaque lot utilisateur ;
- au plus une incidence de ce parent dans un même lot ;
- toutes ses utilisations avant la dernière sont des continuations ;
  une utilisation dans une fusion, si elle existe, est nécessairement la
  dernière. Elle termine exactement sa vie ; les continuations la gardent.

Avec ces conditions, l'induction sur les lots redonne le test de racine
vivante du validateur actuel. Ajouter les contrôles locaux déjà présents :
parents strictement ordonnés, référence de population/mask valide,
naissance avec population entière, K1 et niveaux, CSR complets, etc.
Les erreurs sont réduites à leur première position canonique, pas au
premier thread fautif ; aucune lecture indirecte avant contrôle de domaine.
La sortie est transactionnelle. Les doublons au même plateau, continuation
après fusion et parent créé dans le même lot doivent être rejetés.

Un premier sidecar peut appliquer ce nouvel encodeur **au draft du produit**,
sans attendre un nouveau MSF. Comparer littéralement sa sortie et ses refus
à `build_from()` isole le problème. Une fois qualifié, A pourra alimenter
le même encodeur directement. Le tri/groupement des incidences et les
buffers coûtent O(P log P) / O(P+V+C), sauf réutilisation prouvée de classes
déjà présentes : cette dépense peut dépasser l'encodeur CPU actuel.
Ne pas extrapoler les 39,455 ms de K5 en gain net avant mesure.

## 6. Prochaine expérience et ordre de priorité

1. Export audit-only, mêmes catalogues et ordinals : blocs actifs, cibles,
   draft, ancres et résultats FULL exacts. Mesurer V/E/actions/parents,
   nombre de niveaux et tailles de plateaux, capacités et mémoire.
2. Sidecar encodeur batché sur le draft existant : mêmes nœuds, niveaux
   représentés à l'identique, parents, successeurs, contributions, banque
   et verticales. Inclure coquilles étendues, continuation contributive,
   bloc muet premier du groupe et multifusion à plus de 13 parents.
3. Raccorder le sidecar événementiel A existant puis les images C sur son
   index ; mesurer **A+C+écriture explicite**, copies comprises. Ne pas
   annoncer le seul temps MSF ou cacher une expansion après les 100 ms.
4. Phase 0 reste indépendante et lourde, surtout K10 : reprendre le plan
   batché exact existant et comparer séparément saut D5 / échanges, sans
   réintroduire l'index de selles négatif. Tout callback doit préserver le
   calendrier parallèle : la couture actuelle `FullBallBatchResolver`
   désactive `run_orders_parallel()`.

Conserver les synthétiques 8k/16k/32k, les coupes LiDAR passant par le
capteur (effectifs réels, jamais forcés), le brut, K5/K10, s8/10/12
et plusieurs trames/séquences. Mesurer le travail total et l'expansion
FULL, jamais la seule croissance de la table partagée. **Aucun test
nouveau, port produit ni gain G4 n'est acquis par cette note.**
