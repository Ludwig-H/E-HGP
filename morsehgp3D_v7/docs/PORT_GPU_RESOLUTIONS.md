# Port GPU des résolutions : frontières et ordre de qualification

11 septembre 2026. `phase=exploration_v7_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.
Plan de port, pas un résultat device. GCP non utilisé pour cette analyse.

La [voie statique CPU](RESOLUTION_STATIQUE_CPU_20260911.md) sépare désormais
les résolutions géométriques des composantes temporelles. La première
réduction de travail est qualifiée ; le GPU doit ensuite traiter les clés
uniques non semées, sans recevoir toutes les occurrences répétées.
Les [tours G4 du 10 septembre](RESULTATS_TOUR_CACHE_G4_20260910.md) ne faisaient
que prefilter/census sur device : leur coût FULL restait CPU.

## Première primitive : sélection du support MEB

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
compilation/lien et ses juges O2 hôte ; son wrapper et sa vraie gate G4 restent
à construire. Il reste hors du moteur actif.

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
| Clé de boule primitive | Qualifier le GCD128 et sa réduction ; les divisions bornées du témoin device historique ne prouvent pas une division générale i128/i128 |
| Recherche de catalogue | Comparer la clé entière, contrôler l'intervalle de K et l'antériorité stricte ; ni hash seul ni présence globale sans admission |
| Recherche de l'intrus | Reprendre exclusions des sites sélectionnés et ordre gauche d'abord/ranges croissants du resolver ; le census device existant visite droite d'abord |
| Élagage de l'index | Produire les minimiseurs entiers pour chaque nouvelle boule ; ceux du census courant arrivent déjà calculés par l'hôte |
| Descente | Conserver la décroissance exacte, y compris à rayon égal par baisse de coquille sélectionnée ; aucun plafond d'itérations transformé en terminal |
| Remise au calendrier | Rendre une BallId, puis normaliser son ancre fermée à la coupe pré-lot CPU ; ne transmettre aucun jeton union-find historique aux workers |

Le lookup et la MEB ne doivent pas consulter les ancres courantes. L'auditeur
a [justifié cette indépendance sous census exact complet](../audits/receipts_raccord_ancres_20260910/suite_cache_20260910/NOTE_PHASE_STATIQUE_MEB.md).
Les lots simultanés, naissances sans représentant, contributions unaires et
verticales historiques gardent leur calendrier nominal.

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
Le worker G4 publié conserve la configuration nominale sans `--static-threads`.
Son adaptation et ses validations de configuration sont nécessaires avant
une campagne statique CPU/GPU ; recompiler le nouveau header ne suffit pas
à activer la voie optionnelle.

Base de cette analyse : `full_ball_tower.hpp` `33e7d05e…`,
`anchor_meb.hpp` `386072c8…`, `q3.hpp` `4155a1c3…`, `q4.hpp` `58aac9bd…`,
`census_kernels.cuh` `90c13d51…`, `census_route.cuh` `eb496ff1…`.
