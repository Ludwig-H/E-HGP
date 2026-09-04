# Lecture du manuscrit et décision d'architecture v7

Lecture intégrale effectuée le 4 septembre 2026 : parties I et II,
pages PDF 35–134 du [manuscrit](../../docs/references/MANUSCRIT_THESE_HAUSEUX.pdf).
L'extraction textuelle a été lue par plages contiguës 35–48, 49–62, 63–76,
77–90, 91–104, 105–118 et 119–134. Aucun résultat empirique de la thèse
n'est une mesure du code v7.

## Fondements retenus

Le chapitre 2 relie le single-linkage, les composantes des graphes seuils,
le MST et les amas discrets de densité 1-NN. Le fait 2 donne l'invariant
utile : un arbre minimum couvrant conserve les composantes à chaque seuil.
L'inclusion MST–RNG–Gabriel–Delaunay explique la recherche de certificats
géométriques plus petits que le graphe complet.

Le chapitre 3 distingue partition, dendrogramme et ultramétrique. Les
chapitres 4–5 expliquent la différence entre la construction d'une hiérarchie
et son exploitation : condensation, excès de masse, sélection guidée et vote
sont des opérations aval. Une meilleure fonction de sélection ne répare
pas une hiérarchie incorrecte.

Le chapitre 6 définit HGP par les composantes des facettes de Čech et
établit leur lien avec les régions témoins de densité K-NN. Pour K supérieur
à un, la hiérarchie est laminaire sur les facettes ; sa projection en points
peut se recouvrir. Le chapitre 7 analyse la percolation : il ne fournit pas
un théorème de complexité pour un constructeur WSPD sur des scans réels.
La limite gaussienne admise et les intuitions à haut rappel gardent leurs
statuts conditionnels.

Les chapitres 8–9 motivent les supports de miniball et les événements
Gabriel, puis le vote pondéré. La proposition 5 conserve les composantes
après restriction aux cofaces élémentaires ; elle n'affirme pas l'égalité
des adjacences. La suppression de toutes les cofaces non-Gabriel de la
proposition 6 et du théorème 5 n'est plus une autorité autorisée : la
[contre-fixture à cinq points](../../docs/math/INCIDENCES_SILENCIEUSES_GAMMA.md)
montre une incidence silencieuse nécessaire à une fusion ultérieure.

## Ce que la v6 optimise

La v6 utilise un seul index radix sur Morton et une descente WSPD fusionnée
avec masques pour trois arités. Une paire de facteurs est terminale dès
qu'elle est géométriquement séparée ; aucun cap de population ne remplace
ce critère. Les témoins de fuseaux et les bornes de boîtes éliminent des
familles entières seulement sur une preuve de profondeur excessive.

En dimension trois, les miniballs sont déterminées par au plus quatre
points. Les lanes q2/q3/q4 énumèrent ces supports, avec une ancre maximale
canonique. La v6 remplace les rescans de profondeur par complétion q4 par
un balayage des racines exactes sur la corde d'un seed aigu. Le census
strict et le traitement atomique des niveaux égaux restent les autorités.
Le cover q4 utilise le coefficient quatre : le coefficient trois perdait
des témoins intérieurs. Aucun de ces mécanismes ne construit la mosaïque
de Delaunay d'ordre supérieur.

Ces réductions diminuent le travail intermédiaire. Elles ne prouvent pas
une borne linéaire universelle : les arcs liés constituent déjà une famille
à sortie quadratique. Les compteurs du grand-livre v6 sont donc repris et
les coûts des nouvelles descentes doivent s'y ajouter explicitement.

## Ordre des améliorations v7

Le port est explicite et épinglé dans [V6_SOURCE_SNAPSHOT.json](V6_SOURCE_SNAPSHOT.json).
Les domaines de digest `mhgp6-digest-v1:postprefilter-candidates` et
`mhgp6-diagnostic-v1:raw-candidates` sont figés volontairement ; les digests
d'objet restent au format historique v4. Le domaine de graine du plan CSR
reste également celui de la fixture historique v6, sans changer les entrées.
Les anciennes références v5 copiées sous `receipts/conformite_v5` sont
uniquement des données contractuelles différentielles, pas des reçus v7.

Les premières corrections de coût éliminent la copie des `BallData` du
census, les équipes de threads lancées seulement pour calculer un plan
de tranches et l'allocation temporaire de chaque événement régulier.
Les créations partielles de threads doivent toujours être jointes avant
propagation de l'échec. Ces corrections conservent l'objet v6.

La complétion silencieuse change au contraire l'objet. Elle utilise les
seules facettes du cœur direct et une chaîne de remplacements par intrus
avec niveau strictement décroissant. Chaque étape est une vraie coface
Gamma, le terminal doit être présent dans le catalogue Gabriel et tout
épuisement est transactionnel. La [preuve horizontale conditionnelle](../../docs/math/INCIDENCES_SILENCIEUSES_GAMMA.md)
doit être requalifiée par une comparaison indépendante à toutes les coupes.
Cette voie ne promet ni le catalogue Gamma exhaustif ni la verticale.

L'option `--complete-incidences` sélectionne aussi un fold réduit : pour
K supérieur à un, une facette active mais jamais incidente est latente et
ne compte pas comme parent. Seules les composantes ayant déjà traité une
coface portent une racine antérieure. À l'ordre un, les singletons restent
des parents normatifs. Le champ `born` devient explicitement la première
matérialisation d'une facette dans le sous-flot retenu ; il ne prétend pas
donner sa naissance géométrique exhaustive dans Gamma. Cette sémantique
est nommée `normalized_horizontal_h0_candidate` dans la sortie et l'archive,
distincte de `verified_events_only`. `--require-exact` refuse encore.

## Contrats d'échelle conservés

Le [plan transverse](../../docs/TEST_PLAN_MORSEHGP3D.md) exige un véritable
payload complet à 50 000 points : deux échauffements, dix nuages frais par
famille, incidences silencieuses et verticales. **La consigne utilisateur
du 4 septembre 2026 remplace la priorité temporelle pour la v7 : moins
d'une seconde pour toute la tour K=1 à 10 ; repli sur toute la tour K=1
à 5 si le délai n'est pas satisfait à 10.** L'ancienne cible primaire
100 ms devient, sur précision utilisateur, le jalon suivant dès que la
seconde est validée pour le même périmètre. Le périmètre de tour 1..K
est confirmé par l'utilisateur ; il ne s'agit pas d'un ordre isolé.
Les optimisations se qualifient d'abord en mono-thread, puis en multi-CPU,
enfin sur GPU ; voir le [contrat courant](CONTRAT_PERFORMANCE.md).
Trois runs de diagnostic ne constituent pas cette porte.

La qualification massive commence à 10 000 001 points, puis 30, 50 et
100 millions conditionnellement aux paliers précédents, avec interruption,
reprise et budgets mesurés. La résidence globale v6, les incidences i32,
les index et formats u32 et l'absence de reprise moteur restent des verrous.
Une archive atomique de sortie ne résout pas la reprise du calcul.

Les résultats CPU, CUDA et les extrapolations doivent rester séparés.
`public_status=not_claimed` demeure tant que les portes requises manquent.
