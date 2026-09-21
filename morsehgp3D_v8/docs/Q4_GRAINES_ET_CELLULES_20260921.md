# q4 : parcourir ensemble les graines et les cellules

21 septembre 2026, tranche34. Qualification corrigée close, CPU u16,
`public_status=not_claimed`. Aucun contrat FULL/G4 revendiqué.

## Pourquoi changer le parcours

Le raccord global prépare un atlas par arête : ses cellules représentent
des régions où peuvent se trouver les centres des boules. Le parcours
Local28 énumère ensuite chaque graine admissible et redescend cet atlas.
Sur les préfixes LiDAR mesurés dans33, ce poste dépasse le quadruplement
au doublement de taille : jusqu'à×6,540 sur le scan200 entre8k et16k.

Une graine définit une droite de centres possibles. Plusieurs graines
voisines peuvent manquer ensemble une région de l'atlas. Tester un bloc
de graines contre une cellule permet de partager cette décision, avant
de construire leurs familles et sans supprimer les graines des autres
comptes ou coquilles.

La [note mathématique préalable](Q4_BLOCS_SEEDS_PISTE_20260921.md) justifie
les bornes. Pour la puissance multipliée par4, une borne inférieure
strictement positive ou une borne supérieure strictement négative exclut
le produit. **Zéro reste toujours actif** : les contacts aux côtés et
coins peuvent porter de vrais supports. Le minimum en coordonnées de
graines demande le sommet d'une quadratique, pas seulement les coins.
Le noyau de bornes de `Q4LocalGeometry` est partagé, pas recopié.

## Trois variantes distinctes

- `Individual` : entrée historique et défaut inchangés ; aucun résumé
  ni cache supplémentaire. Le registre additionnel reste nul.
- `LiveOnly` : d'abord le test O(1) `leaf_cells==0`, puis, seulement si
  l'atlas reste utile, un résumé immuable signale les branches sans feuille.
  Le parcours des graines reste individuel.
- `Joined` : partition spatiale en blocs disjoints, puis parcours des
  produits bloc×cellule. Un seul facteur est divisé à la fois. À une
  incidence graine×feuille, le balayage du fragment est appelé directement,
  sans repartir à la racine de l'atlas.

Le contrôle LiveOnly est important : il sépare l'effet d'un résumé bon
marché de celui des bornes géométriques collectives plus coûteuses.
La construction de l'atlas n'est pas réduite par ce premier port.
Joined utilise le même court-circuit initial. Le nombre d'appels actifs
est la somme des résumés préparés et des atlas entièrement écartés ; les
cellules des atlas écartés ne sont pas reparcourues pour le résumé.

## Possession et mémoire

Atlas, géométrie, cover et index original restent possédés et immuables
pendant l'appel. Résumé, pile, cache et buffers d'événements sont privés.
Le domaine Positive est construit avec toutes les complétions de la
lentille, y compris obtuses, jamais avec le seul bloc de graines courant.

Le cache est paresseux, par bloc spatial disjoint, indexé par rang dans
ce bloc. Il conserve aussi les refus. Une graine appartient à un seul
bloc et sa famille n'est préparée qu'une fois au plus par arête, même
si ses incidences avec plusieurs cellules sont entrelacées. Le grain
`block_sites=64` borne ce cache, **pas la recherche ni les sorties**.
La préparation du cache et les réentrées dans l'atlas par bloc sont payées.

L'équipe CPU existante continue d'attribuer des arêtes entières : aucune
arête n'est encore redistribuée. Un futur job de bloc pourra partager le
parent immuable avec un cache privé ; cette étape n'est pas implémentée.
Ne pas distribuer les incidences d'un même cache sans protocole de partage.

## Registres et interface

`Q4SeedCellOptions` s'ajoute explicitement au raccord global et à une
surcharge de l'entrée locale. Window30 avec un mode non Individual est
refusé, même si q4 est inactif ; mode et grain sont validés avant émission.
Les anciennes signatures et les schémas1/2/3 de la sonde restent présents.
Les arguments additionnels `individual|live|joined` et le grain facultatif
produisent le schéma4 et son registre séparé `q4_seed_cells`.

Les37 nouveaux compteurs comportent30 sommes et7 maxima. Ils paient le
résumé, les blocs, initialisations et consultations du cache, préparations
de familles/formes, produits, bornes, subdivisions et incidences terminales.
Les anciens compteurs décrivent toujours les opérations effectivement
exécutées : Joined ne fabrique pas de visites individuelles historiques.
Ses graines testées peuvent être plus nombreuses que ses familles préparées.

Les pics incluent le stockage des nouvelles structures, et le pic global
couple atlas, cache et buffers de balayage ; ce n'est pas le RSS. Les
métadonnées fixes de l'objet, allocations du consommateur et index partagé
restent explicitement hors de ce pic. L'accroissement des registres privés
par worker est réel, même au défaut, et doit rester visible.

L'identité du parcours conjoint est : produits visités = branches sans
feuille + rejets de graines + signes positifs + signes négatifs + produits
incertains. Ces derniers se partagent en divisions spatiales, divisions
de cellules et incidences terminales. Chaque incidence terminale réutilise
ou prépare une famille et déclenche un balayage de feuille.

## Preuves et limites

La qualification doit comparer les multiensembles complets de supports,
clés, profondeurs et coquilles contre un oracle rationnel indépendant,
puis comparer les trois parcours sur les mêmes grands nuages. Contacts,
complétions obtuses, extrema intérieurs, réutilisation du cache, exceptions,
possession et appels concurrents ont leurs fixtures propres. Les preuves
du prototype indépendant A ne deviennent pas celles de ce port.

Les mesures doivent inclure préparation, navigation, balayages, tris,
sorties et mémoire, à8k/16k/32k, avec scans séparés et s8/10/12. La borne
de pile (181 descripteurs pour les hauteurs actuelles) ne borne pas le
travail. Un gain sur les visites seules ne prouve ni une croissance
sous-quadratique complète, ni le contrat de tour50k. q3 est inchangé ici ;
son census collectif sur des blocs de graines est le chantier suivant.

## Résultat de la qualification34

Les [reçus](../receipts/q4_seed_cells_20260921/README.md) ferment96CTest
Release, quatre portes/24sondes instrumentées avec contrôle explicite des
fuites, trois mutants géométriques détectés et les lecteurs normal/−O.
Le premier mutant de contact survivant et le défaut d'autotest du lecteur
restent archivés ; R2 ne modifie que la fixture et le lecteur, pas le moteur.
Les quatre builds R1/R2 sont épinglés. Huit paires d'anciennes CLI gardent
les records complets ; la sonde Release R1/R2 est bit-identique.

Trente grandes observations34 couvrent les trois tailles, avec les
périmètres K5/10 et s8/10/12 précisés dans les reçus. La baisse de
navigation vient surtout de LiveOnly. Surscan0/K5, ses visites d'atlas
font×2,687 puis×2,441 au doublement, contre×4,018 puis×5,603 avant.
Joined prépare moins de familles, mais ses bornes de produits et ses
initialisations de cache empêchent d'établir un avantage supplémentaire
stable. À32k/K5, il initialise57,242M entrées et effectue30,322M bornes
graines×cellules, contre16,399M bornes de droite pour LiveOnly.

La préparation d'atlas garde des sous-postes>×4, et le scan200 conserve
le census q3>×4 au premier doublement. Tous les tris et balayages terminaux
sont inchangés. Aucune preuve de coût global sous-quadratique, ni contrat
de tour, ne résulte donc de cette amélioration locale importante.
Les temps sous charge partagée ne déterminent pas un nouveau défaut.
