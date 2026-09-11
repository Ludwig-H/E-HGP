# Port GPU des résolutions : frontières et ordre de qualification

11 septembre 2026. `phase=exploration_v7_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.
Plan de port mis à jour après une première exécution de primitives sur G4.
Le résolveur complet et son raccord à la tour GPU restent à implémenter.

Après la session G4, la [couture MEB + clé](../receipts/gpu_meb_key_route_20260911/README.md)
est qualifiée O2/SAN ROOT et NVCC strict : le backend forme désormais la
clé primitive, sans matérialisation ni revalidation de puissance sur CPU.
Elle ne rend pas encore de niveau. Le [helper du premier intrus](../receipts/gpu_intruder_primitive_20260911/README.md)
est également clos localement : minimiseurs entiers, exclusions, parcours,
propriétaire d'index hôte et gate portable. Aucun de ces nouveaux raccords
n'a tourné sur device ; leurs prédécesseurs G4 gardent leurs preuves séparées.

Le terminal entier est maintenant assemblé dans un arbre privé, sur la
baseline `33e7d05e…` sans le nouveau raccourci CPU après échange. Il passe
ses premiers raccords FULL et T2 en stub O2/SAN, mais une contre-lecture a
trouvé une collision entre les compteurs de génération de deux types de
propriétaires : une vue mêlant l'index d'un nuage et le catalogue d'un autre
pouvait porter des tokens égaux. Une révision séparée réutilise le token du
nouvel index possédé et prépare sa fixture croisée. Les captures initiales
restent conservées avec ce manque ; ni ownership général, ni CUDA, ni device
ne sont qualifiés pour ce terminal. Il n'est pas intégré au moteur actif.

La [voie statique CPU](RESOLUTION_STATIQUE_CPU_20260911.md) sépare désormais
les résolutions géométriques des composantes temporelles. La première
réduction de travail est qualifiée ; le GPU doit ensuite traiter les clés
uniques non semées, sans recevoir toutes les occurrences répétées.
Les [tours G4 du 10 septembre](RESULTATS_TOUR_CACHE_G4_20260910.md) ne faisaient
que prefilter/census sur device : leur coût FULL restait CPU.

## Première primitive : sélection du support MEB

Le [wrapper propriétaire et sa gate](../receipts/gpu_meb_device_route_20260911/README.md)
sont maintenant qualifiés O2/SAN hôte, puis **sur la vraie G4** dans le
[nouveau reçu de session](RESULTATS_PRIMITIVES_GPU_20260911.md) : 605 cas,
21 432 contrôles, 44 rejets, identité de snapshot, ABI et transaction de lot.
Le contexte garde les positions résidentes et réutilise ses buffers ; il
matérialise encore les clés et niveaux sur CPU. Le moteur FULL actif ne
consomme pas encore cette route privée.

Les paragraphes suivants décrivent la première étape historique, conservée
dans son reçu sans lui réattribuer le nouveau succès :

Un [prototype privé est maintenant conservé avec ses preuves](../receipts/gpu_meb_selection_prototype_20260911/README.md) :
O2 passe 605 cas CPU/Gram (16 592 contrôles, 197 extra-shells) et 253 contrôles
de transport, dont 33 rejets. Le véritable kernel compile et se lie en SM120,
mais **n'a jamais été exécuté sur device**. SAN n'est pas qualifié : son
exécution s'arrête sur LSan/ptrace, sans replay hors sandbox. Le CPU nominal
n'est pas modifié par ce paquet et ses gates SAN restent distinctes.

Ce premier kernel traite un lot avec **un thread par facette**, au plus
dix indices géométriques par requête. Il conserve l'ordre q2, q3, q4 puis
l'ordre lexicographique des supports, et choisit le premier support positif
contenant les sites. Les boucles bornent les supports locaux, pas le nombre
de points du nuage ou la longueur future d'une descente.

L'interface est POD : indices, cardinal, statut, arité, slots du support,
nombre de sites sélectionnés sur la coquille et compteurs réellement payés.
Pour ce premier jalon, l'hôte matérialise encore la clé et le niveau exacts
à partir du support accepté. Une réussite de cette primitive **ne livre
ni un terminal GPU ni une tour FULL GPU**. Une variante warp par facette
ou lanes par support nécessiterait des compteurs différents : elle peut
calculer des supports que le scalaire n'aurait pas visités.

Les formes et puissances q2/q3/q4 sont déjà partiellement marquées
`MHGP7_HD` dans les lanes. Le bien-centrage q4, l'interface `std::span`
et certaines réductions restent hôte. Le test de compilation doit instancier
le kernel réel ; inclure seulement un header dans une unité NVCC ne prouve
pas que son code est compilable sur device. Le prototype a franchi cette
compilation/lien et ses juges O2 hôte ; son wrapper et sa vraie gate G4
n'étaient alors pas construits. Le nouveau wrapper est qualifié séparément
ci-dessus ; il reste hors du moteur actif.

Le validateur de matérialisation certifie le support positif et sa MEB, pas
l'exécution de tout le préfixe lexicographique : un support ultérieur peut
donner la même boule. Le carré teste cette limite, tandis que le juge compare
exactement slots et compteurs. Les puissances de revalidation hôte sont
comptées à part ; en échec, le POD brut et non `selection_work` conserve le
travail déclaré. Aucune transaction de lot FULL ou ABI device n'est déduite
de cette interface locale.

## Deuxième primitive : terminal entièrement géométrique

| Travail nécessaire | Réutilisation et obligation distincte |
| --- | --- |
| Clé de boule primitive | Raccord MEB + clé maintenant qualifié localement sur 605 cas, 6 050 mots, sans matérialisation hôte ; exécution device de ce raccord encore à faire |
| Recherche de catalogue | Comparer la clé entière, contrôler l'intervalle de K et l'antériorité stricte ; ni hash seul ni présence globale sans admission |
| Recherche de l'intrus | Helper local qualifié contre Boost et le parcours CPU sur 596 requêtes ; même premier indice et mêmes compteurs, six mutants causaux |
| Élagage de l'index | Minimiseurs entiers produits dans le helper HD ; 175 cas d'axes et 700 boîtes exhaustives ; ceux du census historique restent calculés par l'hôte |
| Descente | Conserver la décroissance exacte, y compris à rayon égal par baisse de coquille sélectionnée ; aucun plafond d'itérations transformé en terminal |
| Remise au calendrier | Rendre une BallId, puis normaliser son ancre fermée à la coupe pré-lot CPU ; ne transmettre aucun jeton union-find historique aux workers |

Le lookup et la MEB ne doivent pas consulter les ancres courantes. L'auditeur
a [justifié cette indépendance sous census exact complet](../audits/receipts_raccord_ancres_20260910/suite_cache_20260910/NOTE_PHASE_STATIQUE_MEB.md).
Les lots simultanés, naissances sans représentant, contributions unaires et
verticales historiques gardent leur calendrier nominal.

Le [plan détaillé du terminal suivant](../receipts/gpu_meb_key_route_20260911/PLAN_TERMINAL.md)
réunit ces helpers sans aller-retour hôte par étape. Les niveaux internes
peuvent rester bruts : q2 distance carrée/4, q3/q4 depuis leurs formes,
comparaison sémantique U320. Inutile de canoniser chaque niveau ; ne pas
le reconstruire naïvement depuis BallKey, dont `4A²` peut dépasser le
dénominateur i128 admis. K1 garde son chemin vers les feuilles sur CPU ;
le premier terminal géométrique cible K2..10 et rend une BallId, pas un
token DSU. Le propriétaire final devra lier index **et** catalogue à une
génération commune. Les premières gates FULL puis le
[juge T2](QUALIFICATION_TOUR_CENSUS_K10_20260911.md) seront rejoués sur ce
raccord ; aucun résultat de leurs sources antérieures n'est hérité.

## Résidence et mesures

Pour le terminal cible, l'index et le catalogue restent résidents pendant
les requêtes. Lier explicitement le contexte device à leur snapshot et à
leur durée de vie. Les buffers privés du contexte census ne sont pas une
API de prêt implicite ; une plage d'identifiants identique ne certifie pas
l'identité de l'index. Une requête entre une fois, sa cible/statut revient
une fois, hors compteurs nécessaires. Des allers-retours à chaque support
ou intrus ne constituent pas l'architecture massive visée.

Séparer wire, allocations, H2D, kernels, D2H, matérialisation éventuelle,
scatter et assemblage CPU, puis mesurer **toute la tour retenue**. Les
brouillons du journal restent une autre source de coût, traitée par un
[prototype indépendant](PLAN_JOURNAL_INCREMENTAL.md). Aucun débit de MEB
seul ne satisfait le contrat 50k/1s ou les dizaines de millions.

## Portes avant G4

Même kernel en stub hôte contre `anchor_meb` et l'oracle Gram : q2/q3/q4,
extra-shells sélectionnés, dégénérescences, permutations, bords u16, slots
déterministes et compteurs complets. Refuser indices/positions/doublons,
sorties omises et statuts/slots/coquilles mal formés, sans publier de préfixe.
Mutants de positivité, inclusion et choix du premier support ; O2/SAN,
puis compilation **et lien NVCC stricts**, puis seulement exécution device.
Le terminal et le raccord de tour reçoivent ensuite leurs propres gates.
Toute session G4 reste SPOT, bornée et doublement gardée, avec arrêt ciblé
certifié avant relais. Aucune durée ni accélération n'est extrapolée ici.
Le worker de tours G4 conserve la configuration nominale sans `--static-threads`.
Son adaptation et ses validations de configuration sont nécessaires avant
une campagne statique CPU/GPU ; recompiler le nouveau header ne suffit pas
à activer la voie optionnelle.
Le nouveau worker `anchor_meb_worker_v7.py` ne lance que les deux gates
de primitives ; il ne remplace pas ce worker de tours.

Base de cette analyse : `full_ball_tower.hpp` `33e7d05e…`,
`anchor_meb.hpp` `386072c8…`, `q3.hpp` `4155a1c3…`, `q4.hpp` `58aac9bd…`,
`census_kernels.cuh` `90c13d51…`, `census_route.cuh` `eb496ff1…`.
