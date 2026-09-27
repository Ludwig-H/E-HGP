# Contrat du raccord rectangles GPU → arène des seuls survivants

27 septembre 2026, base `a7e80d7f9`. Proposition et contre-audit de
conception, sans code moteur, nouveau benchmark ni appel GCP. Profil
entier u18, grille 1 mm, K1..10, `not_claimed`. Les essais précédents ne
qualifient pas automatiquement ce nouveau raccord.

## Décision minimale

Extraire une **passe GPU limitée aux rectangles**, puis préparer l'arène
uniquement pour ceux dont au moins une voie subsiste. La préparation ne
doit jamais refaire `filter_q34_witnesses` en CPU. Elle doit accepter un
propriétaire scellé produit par cette passe, **pas** un tableau de masques
fourni par l'appelant avec un drapeau « trusted ».

Cette coupe vise le poste géométrique sériel contenu dans les 9,898 s de
préparation de la sonde G4. Elle ne promet pas d'économiser ces 9,898 s
entièrement : ce chrono contient aussi l'arène et son organisation. Ces
sous-coûts n'étaient pas séparés. L'arène CPU, les transferts, les sorties
et l'aval q3/q4 restent à payer.

Sources relues : [Prepared/Cursor](../b_q34_arena_waves_20260927/waves.hpp),
[arène collective](../b_q34_collective_arena_20260927/arena.hpp),
[snapshot CUDA](../b_q34_cuda_waves_20260927/snapshot.hpp),
[filtre CUDA natif](../../src/gpu/filter_runner.cu),
[types et validation CUDA](../../src/gpu/filter_runner.hpp),
[contrat Q34FilterBatch](../../src/gen/pipeline/wspd_q34.hpp) et
[contrôle du batch](../../src/gen/pipeline/wspd_q34.cpp).

## L'autorité des masques n'est pas un test de forme

Le contrôle actuel de `run_wspd_q34_batched` vérifie les tailles,
sous-masques, masses, domaines et ordre des survivantes. Il indique
explicitement qu'une paire supprimée n'est pas visible dans ce contrôle.
Un faux masque nul peut donc satisfaire toutes ces vérifications de
forme tout en perdant une partie de la hiérarchie.

La couture recommandée comporte une identité de propriétaire privée :

1. La fabrique copie les requêtes dans un stockage privé **avant** leur
   validation, retient l'index immuable et fixe K. Un déplacement du
   vecteur appelant ne suffit pas à exclure ses alias mutables.
2. La session résidente dérive ses nœuds/coordonnées de ce même index.
   Le filtre qualifié est le seul chemin normal pouvant construire la
   décision. Ses buffers de sortie ne sont pas exposés mutables.
3. Le propriétaire de décision fixe index, K, identité d'origine, nombre
   R, masques et compacts. Construction privée, copie/déplacement de
   l'objet interdits ; vues seulement constantes. La préparation et la
   consommation dérivent index/K de lui, sans second index/K à substituer.
4. La session de consommation vérifie l'identité de l'index résident ;
   le propriétaire et les allocations device restent vivants jusqu'à la
   fin de tous les kernels et copies, y compris lors d'une exception.

Si la session réutilise uniquement l'index device immuable, le reste des
données peut provenir intégralement du propriétaire préparé. Si elle
conserve aussi « les derniers rectangles filtrés », index/K ne suffisent
plus : filtrer B puis consommer la préparation A risquerait de lire les
buffers B. Il faut des buffers possédés par décision, ou une génération
privée vérifiée et une politique d'usage exclusif, avec événements/jointures
avant toute réutilisation. Ne pas annoncer une session concurrente sans
cette séparation.

Un hash n'est pas la preuve d'égalité d'une population ; un `const span`
sur un vecteur extérieur ne constitue pas davantage son propriétaire.
La filiation est établie par la construction interne et les objets
possédés. L'opacité empêche d'injecter accidentellement des masques
externes ; elle ne remplace ni la preuve du prédicat, ni sa qualification,
ni la détection des fautes remontées par CUDA.

Une fabrique CPU de référence peut produire la même interface, en
calculant réellement chaque masque. Une éventuelle injection synthétique
de test doit rester dans le test, et ne pas créer une fabrique publique
`from_masks` ou un callback arbitraire capable de sceller ses résultats.

## Quoi conserver, et quoi libérer

La copie complète des requêtes est nécessaire pendant leur validation et
leur filtrage. **Elle n'est pas nécessaire après certification, jointure
et compaction achevées.** Les rectangles fermés ne participent ensuite ni
aux facteurs, ni aux paires. Le préfixe complet `raw_begin[R+1]` est lui
aussi libérable une fois ses valeurs utiles copiées et les masses closes.

| Donnée persistante | Rôle |
| --- | --- |
| Index immuable, K, identité de filiation | Géométrie et paramètres non substituables |
| R et `rectangle_masks[R]` en u8 | Compatibilité avec le batch natif, y compris chaque zéro |
| Compacts vivants : ordinal source u64, raw_base u64, deux nœuds u32, masque u8 | Reconstitution de la paire originale et préparation des facteurs |
| Masses brutes et logiques par voie, travail rectangle | Bilan complet, sans relire les requêtes fermées |
| Arène, fallbacks, segments et masses E | Parcours exhaustif du résidu, sans tableaux de taille P/E |

Le vecteur de R masques est encore imposé par `Q34FilterBatch`. Le
supprimer nécessiterait un autre changement d'interface ; le remplacer
par un vecteur de seulement R_live masques n'est pas compatible.
En revanche, il est inutile de conserver la description lourde de tous
les fermés. Si décision compacte, description préparée et snapshot
recopient tous les mêmes vivants, **leurs capacités s'additionnent** :
ne pas annoncer le seul gain sur les fermés en oubliant ces nouvelles
copies. Une représentation finale commune peut être préparée avant sa
publication ; on ne déplace pas les tableaux d'un propriétaire déjà
publié comme immuable.

L'adaptateur `Q34BatchFilter` construit et consomme sa décision dans le
même appel depuis **son** span de rectangles. Il ne propose pas
`as_batch(decision, autres_rectangles)`. Le moteur garde son vecteur
original pour son contrôle de forme/ordre ; le propriétaire peut donc
libérer sa copie privée sans supprimer ce contrôle. Une simple identité
d'origine ne certifierait pas l'égalité d'un nouveau span extérieur.

## Ordre et masses : invariants exacts

Pour le rectangle source r, noter `m_r=|A_r||B_r|`, I_r son masque
d'entrée, D_r son masque après filtre rectangle, et
`raw_base[r]=sum_{j<r} m_j`. Ce préfixe inclut **tous** les rectangles,
même ceux finalement fermés. Il est calculé avec additions contrôlées.

Les compacts sont dans l'ordre source croissant. Leur indice compact
n'est ni l'ordinal source, ni l'indice d'arène, ni un ID de point. Pour
une paire de rangs spatiaux a,b :

```text
p = raw_base[r] + (a - A_r.first) * |B_r| + (b - B_r.first)
```

Cet ordinal u64 accompagne la survivante et son masque jusqu'au tri S.
Il ne vient jamais de l'ordre des classes/bandes ni de la position dans
une vague. Les segments peuvent indexer le vecteur compact ; les requêtes
de l'arène gardent leur `source_ordinal` original strictement croissant.
Des ordinals croissants ne prouvent pas la couverture/disjonction WSPD :
celle-ci reste l'autorité du vrai front, pas de la compaction.

Les masses distinctes sont :

- Praw : somme de tous les m_r ; conserver aussi les masses d'entrée par
  voie si l'on veut publier les rejets rectangle sans le vecteur source.
- P : somme de m_r pour D_r non nul. P3/P4 utilisent les bits de D_r.
- E : nombre de requêtes ponctuelles après Pool ou fallback, avec E3/E4.
- S : union des survivantes ponctuelles, et S3/S4 selon leurs masques.

Toujours `E≤P`, `E3≤P3`, `E4≤P4`. Les voies se recouvrent : ne jamais
additionner P3+P4 pour obtenir P, ni E3+E4 pour obtenir E.
La restitution native impose :

```text
expanded_pairs = P
pair_q3_rejected = (P3 - E3) + rejets_ponctuels3 = P3 - S3
pair_q4_rejected = (P4 - E4) + rejets_ponctuels4 = P4 - S4
```

Les masques de rectangle restent D_r, **avant Pool**. Même si aucune
paire ne survit à la fin, ne pas transformer après coup ce rectangle en
un rectangle fermé : cela déplacerait ses rejets entre les étages.
`rectangle_visits` compte la vraie passe rectangle ; `pair_visits` les
visites réellement payées sur E, pas une reconstitution du natif sur P.

Piège d'intégration actuel : `run_wspd_q34_batched` fixe encore
`witness.pairs.queries=expanded`, et `Q34FilterBatch` n'a aucun champ E.
Il faut garder P pour le contrat logique et publier E séparément dans
la sonde. Le port moteur devra ajouter/distinguer explicitement le
compteur physique ; remplacer `expanded_pairs` par E serait incorrect,
laisser P décrit comme travail effectivement exécuté serait trompeur.

## Lemme de complétude de la composition

Hypothèse 1 : D_r est exactement le masque du filtre rectangle natif
Affine sur le même index, K et I_r. Hypothèse 2 : le Pool utilise les
facteurs originaux disjoints et ses crédits stricts qualifiés. Une voie
retirée par Pool possède au moins son seuil de témoins distincts ; le
filtre ponctuel natif retirerait donc cette même voie.

Pour chaque paire, le masque ponctuel final calculé depuis son masque
Pool est alors celui obtenu depuis D_r. Une paire omise par Pool aurait
un masque final nul. Les fallbacks énumèrent D_r intégralement. Les
bandes énumèrent exactement une fois les autres paires et le tri de p
rétablit leur ordre natif. Donc le batch final conserve exactement S,
les endpoints, les masques, l'ordre et les rejets par voie.

Ce lemme autorise de supprimer du travail, pas de reprendre des crédits
comme un compte initial du census. Les comptes ponctuels repartent de
zéro. K1 n'a aucune voie ; K2 n'a que q3. Fermer les voies indisponibles
avant tout calcul `K-2` non signé. Les petits facteurs et le cas sans
assez de témoins potentiels restent des fallbacks complets, sans plan.

## Passe GPU minimale et coûts à publier

Ne pas appeler `run_filter_batch` pour ne garder que ses masques de
rectangle : cette fonction lance aussi les paires et alloue les anciens
tableaux P. Extraire le noyau rectangle et son comptage, en conservant
`gpu::filter_boxes` : il choisit le prédicat ponctuel pour deux boîtes
singleton, général sinon. Forcer la spécialisation ponctuelle sur de
vrais rectangles serait géométriquement faux.

Le premier raccord peut télécharger seulement R masques puis réaliser
**un scan hôte O(R)** pour masses/préfixes/compaction, sans géométrie.
Cela évite d'introduire d'emblée une seconde grande mécanique CUB.
Si la compaction est GPU, ses flags/préfixes restent bornés par une vague
Qr, et chaque compact porte sa position source globale u64. Le choix
est une mesure de coût, pas une différence de validité.

La session résidente peut partager les nœuds device entre cette passe
et les vagues ponctuelles ; les points en ordre spatial ne servent
qu'à ces dernières. Ne pas reconstruire/recharger l'index à chaque
rectangle ou vague. Le validateur public d'entrées CUDA brutes conserve
ses contrôles, dont l'inclusion de chaque point dans les boîtes en
O(n·profondeur). Un chemin privé dérivé directement de l'index immuable
peut éviter cette revalidation répétée, mais pas accepter des boîtes
extérieures au moyen d'un booléen « déjà vérifié ».

R, masses, préfixes, source_ordinal et positions globales sont u64 ;
nœuds/rangs locaux restent u32 dans le domaine qualifié, en excluant la
sentinelle `absent32`. Les scans CUB éventuels portent seulement sur une
vague admissible en `int`, sans plafonner R/P/E. Produits, additions,
tailles d'allocation et somme des compteurs doivent être contrôlés.
Un `stack_failure`, échec de kernel/copie ou allocation interdit de
publier la décision, même si des masques partiels sont disponibles.

Le travail total est au moins la somme du scan O(R), des visites V_R du
filtre rectangle, de la sélection/crédits O(KF), du regroupement/bandes,
des visites ponctuelles V_E et du tri/conversion S. Le regroupement
qualifié conserve son terme `O(F + 100L + C_B + C_A K log K + D)`, où L
compte les rectangles planifiés, C_A/C_B les classes et D les bandes.
Le décodage GPU actuel ajoute une recherche de segment par requête.
Le parallélisme réduit la durée, pas ces volumes ; ni R, ni V_R, F ou S
ne reçoivent ici une nouvelle borne sous-quadratique générale.

Mémoire persistante visée : O(n + R + R_live + F + classes + bandes + S),
plus scratch Qr/Q et mémoire des workers. Le front original du moteur
reste présent. Publier aussi les pics transitoires : copie privée des
requêtes et préfixe, buffers upload/compaction, coexistence décision /
préparation / snapshot, ancien et nouveau buffer lors d'une croissance S.
Aucun tableau temporaire global P/E ne doit réapparaître indirectement.

Mesurer un vrai intervalle depuis entrée de l'adaptateur jusqu'à sortie
S complète : validation/copie, filtre rectangle, scans/compaction, arène,
uploads additionnels, vagues, tri/conversion et libération effective des
temporaires. Garder séparément contexte CUDA initial, référence et
destruction de la sortie retenue. Un intervalle non attribué ne devient
pas un gain estimé. Le seul temps des kernels ne représente pas le port.

## Portes prioritaires avant une nouvelle mesure G4

1. Comparer tous les D_r puis chaque survivante finale au natif, pas
   seulement un digest. Variantes K1/2/5/10, s8/10/12, permutations d'IDs,
   boîtes singleton et non singleton, voies 2/4/6, vraies réductions Pool.
2. Mélanger rectangles fermés au début/milieu/fin, plans et fallbacks.
   Forcer `source_ordinal != compact_index != arena_index` et des bases
   non nulles après un rectangle fermé ; comparer les ordinals exacts.
3. Lots vides, toutes voies fermées, E>0/S=0, frontière exacte et contacts
   stricts. Ne pas inventer un plan local vide si ce cas est interdit par
   son invariant de paire croisée minimale.
4. Copier avant validation ; modifier le vecteur appelant et libérer son
   index après construction. Refuser session/index/K incompatibles et
   vérifier l'absence d'API publique scellant des masques arbitraires.
5. Vagues Qr/Q de tailles 1, petites, non multiples de warp et dépassant
   la taille utile ; frontières de rectangles/bandes entre vagues.
   Tester les formules u64 au-delà de 2^32 sans prétendre parcourir des
   milliards de paires, puis distinguer ce test du passage à l'échelle.
6. Fautes de device et allocations aux différentes transitions : jamais
   de succès partiel, propriétaires joints/libérés ; aucune course sur
   les décisions exposées. Mutants : masque fermé forgé, mauvais raw_base,
   ouverture d'une voie Pool, saut d'un fallback et substitution d'owner.

## Première confrontation à la couture hôte en cours

La première version non gelée de
[`host.hpp`](../b_q34_filtered_resident_20260927/host.hpp) réalise bien
une session liée à **une seule population** dès son ouverture, avec
constructeur privé de décision et contrôle d'une identité `Origin`.
Elle libère les requêtes/préfixes après compaction et ne propose aucun
re-filtrage public dans la session. Aucune nouvelle erreur géométrique
n'a été trouvée dans cette première lecture hôte.

Deux coûts ont été signalés au développeur avant qualification : les
copies POD de l'arène ne justifient pas d'en conserver en plus l'original
une fois les segments construits ; et `Session::close` ne libère pas
à lui seul la décision encore possédée par la préparation/session.
Les compteurs et chronos doivent refléter leurs propriétaires réels.
La validation explicite des voies disponibles à K et les masses brutes
par voie ont aussi été recommandées, sans accepter de masque extérieur.

Cette observation préliminaire n'est pas une qualification de source
figée : la relecture device, les gates et leurs reçus restent distincts.
Les anciennes captures restent figées. Cette note définit la prochaine
couture testable ; elle ne qualifie encore ni son implémentation, ni un
gain G4, ni la tour explicite 100 ms.
