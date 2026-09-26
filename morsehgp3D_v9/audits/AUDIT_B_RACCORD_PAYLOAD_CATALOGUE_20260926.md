# B — raccord des IDs intérieurs et priorités réelles vers 100 ms

26 septembre 2026, source moteur `52ff41802`. Audit/plan de port,
`quantized_u18_input_only`, `not_claimed`. Moteur inchangé.

## Ce qui est prêt à porter, et ce qui ne l'est pas

Le [producteur q3](b_q3_payload_producer_20260926/README.md) récupère les
IDs dans les votes intérieurs déjà calculés, sans nouveau prédicat ni
nouveau parcours global. Le [consommateur précédent](b_census_payload_20260926/README.md)
montre comment reconstruire une boule régulière à partir d'un paquet
complet, avec propriétaire, clé, positivité et contrôles exacts locaux.
Les deux extrémités sont maintenant éprouvées en CPU ; **la plomberie
CUDA entre les deux reste à construire et juger**.

Pour q4, le [nouvel audit](AUDIT_B_TRANSPORT_INTERIEURS_Q4_20260926.md)
établit qu'un second passage par émission n'est pas nécessaire : les
masques T1 et les intérieurs communs à l'intervalle suffisent. Ce point
remplace la recommandation conservatrice de collecte supplémentaire
de la première note. Il reste des votes, écritures, synchronisations
et transferts à payer, pas zéro coût.

À K5, au plus trois IDs q3 et deux IDs q4 par sortie ; à K10, huit et
sept. Ce sont des **bornes de profondeur des sorties admissibles**,
pas des plafonds de recherche.

## Les associations à ne pas perdre pendant le port

| Étape actuelle | Contrat du futur paquet |
| --- | --- |
| `q3_census_range` / `q4_seed_finish` | IDs originaux, pas rangs spatiaux ; liste complète seulement après acceptation ; scratch d'une graine rejetée jamais publié |
| tri q4 par représentant, `q4_lanes.hpp:646` | permuter record et paquet ensemble ; `edge` sert provisoirement de clé de tri, pas d'identifiant persistant du paquet |
| L15, `lanes_tasks.hpp::lanes_task_slab_index` | q3 est écrit depuis le haut en ordre inversé, q4 depuis le bas ; appliquer la même fonction de placement aux IDs |
| copie tâche→staging, `filter_runner.cu::lanes_task_kernel` | réservation commune aux records et paquets ; écritures achevées avant publication ; dépassement ou tâche différée n'expose aucun préfixe |
| replay/scan/gather de C | rejouer les mêmes statuts, destinations et offsets ; ne pas recompacter séparément les deux tableaux sans identité commune |
| sortie paginée ou bassin épinglé | le bail doit posséder aussi les IDs ; un pointeur dans une slab réutilisée devient invalide dès la prochaine tâche |
| conversion GPU→`Q34LanesBatch` | même ordinal que le record, propriétaire/lot conservé ; contrôler limites et cardinalité avant lecture |
| `lanes.sink`→`Presentation`→`gather_presentations` | stocker les petits IDs par valeur ou un handle vers une arène possédée ; les tris actuels déplacent/copient les présentations puis libèrent les slots |
| groupe canonique d'une clé | choisir un paquet compatible, jamais additionner les intérieurs de plusieurs supports de la même boule |
| import `BallData` | convertir ID original→rang Morton de **l'index de tour**, différent du rang du générateur ; reconstruire niveau, Euler et compteurs comme avant |

Le choix simple est un petit sidecar de stride dépendant de K, lié à la
position du record jusqu'au sink, puis un handle stable vers une arène
possédée par la chaîne pendant le rassemblement. Garder l'identité du
nuage et du lot au niveau du propriétaire, pas un `shared_ptr` par ID.
Une solution intégrée par valeur est également exacte ; son alignement
et le trafic des tris doivent être mesurés. Ce tableau est une revue
de raccord, **pas une qualification de ces déplacements non implémentés**.

Le contrôle de lots actuel juge arité, compte, support, arête et partition
des slices. Il ne juge pas encore ces IDs inexistants. Étendre ses tests
avec permutation sans sidecar, rang pris pour ID, mauvais bail après
réemploi, record d'un autre nuage, L15, différé partiel et saturation de
staging. Garder une voie à census global comme oracle différentiel.

## Sceau et refus : changer honnêtement la provenance

Le sceau actuel vient du census global de chaque clé. Un import local
ne peut pas affirmer avoir rejoué ce contrôle indépendant. La preuve
nouvelle est : compte complet produit par le cover certifié, puis d IDs
distincts de puissance strictement négative dans le même nuage et la
même boule. Si la coquille compte q et le support positif porte q sites
de puissance nulle, ce support est la coquille entière.

Une liste tronquée et un compte falsifié ensemble peuvent passer une
vérification locale. Ne pas créer de fabrique publique recevant librement
`depth` et `certified=true`. Le contrat interne, les portes du producteur
et le juge global activable restent nécessaires. Modifier explicitement
la raison/scellé publiée et le validateur de sonde si le mécanisme change.

Si la coquille dépasse l'arité, garder `census_key` et `ShellTable` :
q_min, Euler, contacts et multifusions ne se déduisent pas d'une seule
liste intérieure. Les émissions CPU de repli sans paquet gardent aussi
le census. Conserver la priorité d'erreur par plus petite clé, et ne pas
transférer le sceau d'une exécution interrompue.

## Taille de l'occasion, pas soustraction de chrono

Dans R24-B `vm/probe_0.stdout` (00 sans sol, K5/s8), sur 849 777 clés
tardives, 849 774 sont régulières ; le census tardif prend 100,273 ms.
Cela rend le port prometteur, **sans autoriser à soustraire 100 ms** :
imports, collecte, copies, allocations et replis remplacent ce coût.

Les 691 284 records q3 et 158 496 q4 du même passage demanderaient,
avec les strides maximaux propres à K5, au plus
`4·(3·691284 + 2·158496) = 9 563 376` octets d'IDs par tableau de
sortie. Le prototype q3 choisit volontairement huit slots fixes ; huit
pour toutes ces sorties ferait 27 192 960 octets. Ces nombres ne sont
ni le nombre réel d'IDs, ni le pic de mémoire : staging, arène finale,
bail hôte et scratch peuvent coexister, et les capacités excèdent les
tailles utilisées. Une largeur commune de trois IDs pour les deux
arités ferait 10 197 360 octets, option simple à mesurer.

## Le tri q4 n'est pas encore le bon poste à accélérer aveuglément

Lecture directe des quatre bras complets R24-B (indices0/7/14/21) :

| Entrée / K | Passages T1 | Chunks T1 / passage | Chunks du passage lentille |
| --- | ---: | ---: | ---: |
| sans sol 00 / 5 | 470 724 | 1,010 | 11 680 121 |
| sans sol 00 / 10 | 5 324 855 | 1,017 | 68 118 302 |
| brute b00 / 5 | 721 470 | 1,004 | 29 361 392 |
| brute b00 / 10 | 8 030 029 | 1,006 | 131 520 272 |

Depuis T1, `groups` compte des passages, pas nécessairement des classes
distinctes : une classe profonde peut être revisitée. Les moyennes ne
bornent donc ni les maxima ni le travail par vraie classe. Mais elles
ne montrent pas de longs parcours moyens à éliminer dans ces cas.
Un chunk lentille et un chunk de comparaison n'ont pas le même coût ;
ce tableau ne donne pas des pourcentages de temps.

La [voie triée native](b_q4_sorted_events_20260926/README.md) valide la
construction O(m log m) par famille, pas un gain GPU. Son cas synthétique
dense8k/16k/32k montre même le rejet précoce environ cinq fois plus rapide.
Conserver T1 sur les petites listes faciles ; instrumenter longueurs,
revisites et travail par segment avant une voie triée adaptative.

Ordre de travail recommandé vers les 100 ms :

1. Raccorder les IDs q3 au catalogue, avec oracle FULL et coût net G4 ;
   ajouter q4 par les masques T1/lentille, sans nouveau scan global.
2. Porter le [front compact par vagues](b_front_waves_20260926/README.md)
   sur GPU et mesurer l'ensemble front→sorties, pas seulement son noyau.
3. Éprouver le [graphe événementiel FULL tous ordres](b_full_phase_a_work_20260926/README.md)
   en parallèle, en matérialisant les nœuds/liens dans le chrono.
4. Utiliser la voie q4 triée pour les segments réellement coûteux si les
   mesures le justifient ; conserver les contre-régimes publiés.

Les prototypes de ce lot n'ont pas lancé GCP. Le cache S2 a déjà son
reçu G4 distinct ; répéter cette séance ne répondrait pas aux nouvelles
questions. La prochaine séance utile doit mesurer le **raccord CUDA et
la chaîne**, après ses portes, avec bras appariés. Ni 100 ms FULL ni
sous-quadratique LiDAR global nouvellement acquis.
