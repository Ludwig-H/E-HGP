# Sous-arbres du front et census q2 par workers privés

14 septembre 2026. Treizième tranche implémentée :
`exploration_v8_hors_registre / cpu_reference / quantized_u16_input_only /
implementation_v8_p0 / not_claimed`. Les [preuves locales](../receipts/q2_front_workers_20260914/README.md)
sont propres à cette tranche : 53 CTests Release/Clang ASan/UBSan,
porte ThreadSanitizer et32 différentiels ancien/nouveau mono passent.
Les mesures de performance sont séparées des preuves fonctionnelles.

## Objet et preuve de découpage

La [tranche Pool terminal](P0_POOL_TERMINAL_Q2.md) retire le quasi-carré
mesuré sur les amas8k/16k/32k, mais laisse le front et ses nombreux petits
rectangles. Cette tranche partage ce travail entre CPU, sans changer
les décisions géométriques ni les continuations de census.

Préparer des jobs à partir du véritable parcours WSPD. Chaque produit
parent subit ses tests une seule fois. S'il est indécis, ses enfants
héritent du masque et de la profondeur. LL/LR/RR partitionnent les paires
non ordonnées d'une diagonale ; les deux enfants d'un produit disjoint
partitionnent son produit cartésien. Les rejets restent ceux du parent.

Deux sortes de jobs sont distinguées : produit non visité à reprendre,
et rectangle terminal dont les compteurs du front sont déjà acquis.
Le second appelle seulement son consommateur ; il ne refait aucun test
ni aucune émission comptable. Le plan possède le même index immuable.
Les états restent privés, accessibles par ordinal dans ce plan précis.

La granularité vise quelques jobs par worker, pas un catalogue de la WSPD.
Le critère doit compter les produits en attente **et** les terminaux,
sinon une simple pile DFS trop petite ferait exécuter tout le front en
série avant de lancer les workers. Aucun plafond ne tronque le calcul.

## Propriété et sortie

Chaque worker a un moteur census réutilisé, ses vues B et ses buffers.
Le census d'un rectangle reste intégralement dans ce worker : même
ordre des témoins, mêmes crédits/curseurs, même préparation Pool unique.
Aucun partage du plan sur des tranches A_i×B n'est introduit ici.
Les gros jobs peuvent donc encore déséquilibrer le travail ; ils sont
à mesurer, pas à masquer par un gain sur la moyenne.

Un callback par slot worker, copié avant le lancement. Les captures
mutables du consommateur doivent être disjointes ou synchronisées par
l'appelant. Une vue de support dure seulement pendant son callback.
L'ordre inter-workers n'est pas déterministe ; le multiensemble des
supports complets l'est. Il ne s'agit ni d'un catalogue canonique ni de FULL.

Sur exception, chaque worker cesse de prendre des jobs dès qu'il observe
l'annulation, puis tous sont joints avant de propager l'erreur. La
vérification est coopérative : un job peut encore être engagé par un
worker ayant lu le drapeau juste avant l'erreur. Ces jobs peuvent finir ; les
émissions partielles ne sont pas annulées. Aucun worker détaché, aucun
contexte emprunté à un callback terminé. Une erreur de lancement suit
la même fermeture. Les tests doivent couvrir ces chemins réellement.

## Mesures exigées

Confronter les supports complets au mono et à un petit oracle indépendant,
avec ordre de jobs modifié, propriétaire externe relâché, coquilles,
exceptions et options invalides. Tous les compteurs de travail doivent
être conservés par somme, les profondeurs par maximum. Le maximum de
pile DFS logique peut être préservé par les frères virtuels en attente ;
ce n'est pas la mémoire physique de tous les workers.

Le temps mur englobant et les sommes d'intervalles workers/payload sont
distincts : ne jamais soustraire une somme CPU au temps mur. Publier
coût du préfixe, jobs, travail/temps par worker, stockage de jobs et
somme des maxima de plans, cette dernière seulement comme majorant
des capacités de stockage retenues simultanées, hors objets/transitoires,
pas comme pic RSS. Nuage et index sont payés
une fois ; copies/validation/hash et collecte des callbacks restent payés.

Comparer le mono historique, le nouveau chemin à un worker et plusieurs
workers sur les mêmes entrées, puis croissance8k/16k/32k et s8/10/12.
Une accélération murale à travail identique ne change aucune borne de
complexité. La tour FULL50k/G4 et les dizaines de millions restent ouverts.
Les nouveaux builds sont `build/v8_front_workers_20260914/` et
`build/v8_front_workers_sanitize_20260914/` ; ne pas modifier les builds
Pool terminal épinglés. Ces nouveaux builds, ainsi que le témoin
ThreadSanitizer `build/v8_front_workers_tsan_20260914/`, sont désormais
épinglés après les qualifications et134 mesures. Les lecteurs normal/−O
passent séparément pour les deux affinités (deux cœurs/quatre SMT,
quatre cœurs physiques). GCP non utilisé.

La [contre-vérification indépendante B publiée à375c5288](../audits/DIALOGUE_AUDITEUR_B.md)
confirme les sources Pool ba11e3ab sur ses fixtures et mesure le coût
des seuils1/2/64. Sur son corpus Samples,64 conserve99,94 % des rejets
locaux pour57 fois moins de sites relus que1 ; cela ne prouve pas un
seuil optimal universel. Son avertissement sur Pure et résidus partiels
reste ouvert. Ne pas changer cette politique simultanément avec le
partage CPU : isoler l'effet de l'ordonnancement à travail géométrique égal.

## Suite : redistribuer les produits encore pendants

L'[audit A329e5b86](../audits/front_tasks_20260914/README.md) motive une
politique dynamique distincte, pas encore implémentée. Garder `expand`
inchangé ; donner un enfant non visité ou un produit retiré de la pile
du donneur. Transmettre aussi `dfs_pending` pour conserver le maximum
logique actuel, en plus de a/b/masque/profondeur et du même contexte.
Ne pas transférer un callback commencé ni ses vues Pool/Z. Un frère
nouvellement créé peut être donné en O(1), sans déplacer toute une pile.

Une file bornée préallouée limite la mémoire d'ordonnancement, jamais
la recherche : si elle est pleine, le donneur continue localement.
Une file vide ne signifie pas la fin : il faut aussi que les seeds
initiaux soient épuisés et qu'aucun worker n'ait une pile ou un callback
actif. Le comptage doit distinguer seeds, dons et fragments achevés.

Attention au raccord des exceptions : si des receveurs attendent sur
une condition, le helper actuel, qui pose seulement un drapeau, ne
suffit plus. Un réveil non levant doit accompagner l'annulation après
erreur de callback **et** erreur de lancement partiel, avant les joins.
Tester file de taille1 pleine, demande tardive, dernier worker actif,
erreur avec receveurs endormis et égalité des compteurs/masques/maxima.
La mémoire comprendra aussi le plan initial conservé, les piles et les
objets actifs ; ne pas présenter Q+97W comme le pic de toute la chaîne.
Le coût d'un gros rectangle et de sa coquille reste une limite séparée.
