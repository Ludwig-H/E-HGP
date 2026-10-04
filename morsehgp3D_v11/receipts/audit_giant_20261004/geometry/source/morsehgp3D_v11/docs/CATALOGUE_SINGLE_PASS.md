# Catalogue à une passe géométrique

L'option `CatalogueParams::single_pass`, désactivée par défaut, attend une
qualification G4 distincte. Elle exige l'overload avec Pool ; l'overload à trois
arguments la refuse avec `parameter_out_of_range` après la validation des
paramètres. Aucun résultat des campagnes antérieures ne qualifie cette voie.

La préparation du front fixe ou adaptatif reste inchangée. Chaque suffixe est
ensuite exécuté **une seule fois**. Son ordinal possède ses émissions et
incidences, indépendamment du worker. Les boîtes, G1/J2/G3/G4, supports obtus
prolongés en q4, census complets et règles de canonisation restent ceux du
générateur existant. Il n'y a ni deuxième préparation du front ni rejeu des
suffixes. Les quotas de nœuds restent globaux pour cette passe ; aucune limite
de sortie par worker ou nombre maximal de blocs n'est ajoutée.

## Blocs et propriété

Deux chaînes de blocs fixes appartiennent à chaque ordinal : 256 `Emission`
par bloc, 2048 `SiteIdx` par bloc. Une chaîne vide n'alloue rien. Il n'y a ni
doublement, ni recopie de croissance, ni tableau d'entrée en `std::vector`.
Le tableau des propriétaires d'ordinaux est borné par la frontière (256 ou
1024) à la compilation ; ses listes, chemins et scratchs conservent leurs
réservations existantes.

Chaque page possède un `Buffer<T>` de données et un `Buffer<std::byte>` pour
son enveloppe. Le nœud propriétaire est construit explicitement dans cette
enveloppe par `std::construct_at`. Son alignement est gardé par `static_assert`
contre l'alignement standard fourni par `operator new`. Tous les arguments sont
déjà acquis et le constructeur est `noexcept`. À la libération, l'enveloppe est
d'abord déplacée sur la pile, le nœud est détruit par `std::destroy_at`, puis
l'enveloppe est libérée. Il n'y a aucun accès au nœud après cette libération et
la destruction de la chaîne est itérative.

Chaque émission valide contient au plus `max_leaf<=1024` incidences, donc
nécessite au plus une nouvelle page de chaque chaîne. Les deux acquisitions
réussissent avant de relier la première page ou d'écrire la première valeur.
Un refus de la seconde rend la première et conserve exactement les valeurs,
tailles et capacités antérieures. Les populations I et U peuvent traverser la
frontière d'une page ; elles sont copiées directement dans cet ordre, sans
tampon de population intermédiaire.

## Mémoire et refus tardifs

Soient B et I les nombres finaux d'émissions et d'incidences, et J le nombre
d'ordinaux non vides. Les capacités sont au plus `B+255*J` émissions et
`I+2047*J` incidences. Chaque bloc ajoute aussi l'enveloppe de son propriétaire.
Les sommes, produits et offsets sont contrôlés avant allocation/compactage.
Pendant un append, les pages en préparation sont elles aussi réservées dans
le même budget atomique. Aucun morceau de mémoire n'est caché dans des
descripteurs alloués hors budget.

L'admission initiale couvre le front et les scratchs, **pas les sorties encore
inconnues**. `admit` ne réserve pas de crédit exclusif pour un worker. Une page
peut donc être refusée après un calcul géométrique déjà payé, même si les
autres workers ont réussi. Le Pool acquitte toutes les tâches avant le retour
du refus ; aucune sortie partielle ni diagnostic partiel n'est publié.

Après join, le pilote somme les tailles et contrôle aussi la borne globale
exclusive `ball_limit`. Il alloue alors les deux tableaux exacts de B émissions
et I incidences, pendant que toutes les arènes, le front et les scratchs sont
encore vivants. Le compactage copie les pages dans des plages disjointes,
attribuées par ordinal ; les offsets de population deviennent globaux. Ce
compactage peut lui aussi être refusé faute de mémoire. Toutes les pages et
leurs métadonnées sont rendues avant le tri et l'assemblage existants.

Les réservations Buffer retenues par un catalogue publié sont inchangées. Le pic supplémentaire
et le coût des allocations/copies sont mesurables ; cette option n'annonce
aucune admission globale complète avant workers et aucun gain de temps acquis.
Les objets de diagnostics précédents et leurs réservations coexistent avec
le brouillon jusqu'au succès complet ; un refus conserve leurs vues et leurs
valeurs, ainsi que les anciens catalogues.

## Compteurs et temps

Tous les champs de `CatalogueLedger` sont conservés : géométrie, q4, cache J2,
maxima compris. Ils décrivent le travail logique d'une passe. Le nouvel objet
possédé `CatalogueExecution`, accessible par `execution()`, donne :

- `geometry_passes` : deux pour la construction historique réussie, un pour
  cette option ; la préparation géométrique du front est incluse ;
- `arena_blocks`, `arena_capacity_bytes`, `arena_metadata_bytes` : blocs fixes
  alloués avec succès et leurs capacités exactes ; chaque bloc utilise deux
  allocations Buffer, données et enveloppe ;
- `compact_records`, `compact_population` : éléments réellement copiés au
  compactage, égaux à B et I. Les écritures initiales de B/I dans les pages se
  distinguent de cette copie ; aucune copie de croissance n'existe.

Un catalogue sans émission mais issu d'un Cloud non vide peut avoir un front
sans suffixe. La préparation compte encore comme passage géométrique ; le
compteur ne prétend pas qu'un suffixe vide a été parcouru. Un Cloud vide est
refusé avant le générateur et ne publie aucun objet d'exécution.

Les intervalles murs `single_pass_ns` et `compact_ns` ainsi que les sommes et
maxima de tâches correspondants s'ajoutent à `CatalogueTimings`. `count_ns`,
`fill_ns` et `replay_ns` restent nuls dans cette voie. Les nouvelles durées par
tâche sont séparées des anciennes ; aucune horloge n'est créée sans demande
de timings ou de diagnostics. La borne `sum_task <= min(W,J)*wall` s'applique
séparément aux deux phases. Le bound de rejeu publié est zéro puisqu'aucun
rejeu n'est demandé. Le format des anciens reçus est inchangé.

## Portes préparées

Les portes natives couvrent les franchissements de pages et la transaction
sur chacune des quatre allocations nécessaires lorsque les deux chaînes
doivent s'étendre ensemble. Elles comparent W1/W4/W48, les fronts fixe/adaptatif,
les coordonnées extrêmes des profils, les coquilles étendues et le q4 ; le tri
indirect, le cache J2 et l'assemblage parallèle restent composables.

Les refus portent sur la voie sans Pool, le Pool réentrant, la limite globale
de nœuds, `ball_limit=B/B+1`, toutes les positions d'allocation observées et un
budget public qui refuse après acquisition des pages. Le compteur des fautes
hors pilote est observé, sans assertion fragile sur le scheduling. Le juge
Gram/Fraction existant conserve la vérité géométrique ; le nouveau contrôleur
vérifie les coûts des pages depuis les sorties par ordinal. Son modèle passe
en Python normal et optimisé. Les mutations visent données compactées, offsets,
métadonnées, limite globale et nombre de passes. La qualification native reste
à exécuter.
