# Reprise du développement v8 — audit du 20 septembre 2026

Point de départ : `3e94c868`, sur `main`. Cadre inchangé :
`exploration_v8_hors_registre`, `cpu_reference`,
`quantized_u16_input_only`, `implementation_v8_p0`, `not_claimed`.
GCP non utilisé pour cet audit.

## Verdict en bref

Le développeur précédent a apporté deux améliorations substantielles au
front q2 et a fermé honnêtement une piste négative. Le code et les reçus
correspondent : la reprise n'a trouvé aucun défaut d'exactitude avéré dans
ces trois changements. **La v8 n'est toutefois pas encore un moteur de
tour HGP** : elle produit actuellement les supports q2 et leur census.
Ni q3/q4 complets, ni catalogue commun, ni parents FULL, ni GPU ne sont
implémentés à ce point de départ.

Nous conservons ce socle et changeons de priorité : construire les pièces
manquantes, sans prolonger la recherche de quelques pourcents sur q2.

## Ce qui a changé depuis la précédente reprise

| Commit | Changement | Conclusion |
| --- | --- | --- |
| `8d615cfd` | Petites recherches q2 entrelacées, état de72 octets | Exact sur les portes, mais plus lent ; hors défaut, piste fermée |
| `8190e7ab` | Proposer2K ou4K sites au lieu deK, sans changer le seuil de rejetK | Réduit fortement le résidu et le temps hors rangées |
| `3e94c868` | Transmettre les identifiants des témoins certifiés aux enfants | Réduit encore recherches et résidu ; aucun crédit ajouté au census |

Les témoins hérités restent universels sur les boîtes enfants et sont
dédupliqués. Les tâches, jobs et dons portent leurs propres listes ; le
résultat ne dépend pas du worker qui les exécute. Le défaut garde sa
sémantique et ses compteurs, **pas exactement son coût mémoire** : les
tâches front pèsent maintenant72 octets au lieu de32, même option inactive.

Les lots compacts restent disponibles comme témoin expérimental ; ils
ne doivent pas être présentés comme l'optimisation retenue. Le chemin
singleton historique était déjà une boucle sans allocation par ancre.

## Preuves effectivement relues

La reprise a vérifié les130 empreintes de sources de la tranche21,
ses98 artefacts et les887 entrées de sa fermeture d'analyse. Les onze
captures ont été relues et analysées en Python normal et `-O` : synthèse
identique au reçu,854 mesures et372 comparaisons. Les journaux authentifiés
portent81 CTests Release,81 Clang ASan/UBSan et trois portes Clang TSan.
Ce sont des **vérifications des preuves existantes**, pas de nouvelles
exécutions des81 tests sous chaque compilateur. Voir le
[rapport de contrôle](../receipts/reprise_20260920/AUDIT_PREUVES.md) et les
[reçus d'origine](../receipts/q2_front_inheritance_20260917/README.md).

Les petits oracles comparent les objets complets ; les grandes mesures
apparient notamment leurs empreintes. La campagne70k exerce des rangs
supérieurs à16 bits. Le refus au-delà de2³² sites n'est testé que comme
prédicat, pas par la construction d'un tel nuage.

## Résultats utiles à conserver

Pipeline q2, front+census+collecte/callback, hors préparation du propriétaire
et de l'index, Kmax10, s8, Pool64,
options explicites `{2,16,true}`. Secondes locales ; mono = minimum des
trois captures historiques. Le quatre-workers est une observation à32k,
pas une campagne à8k/16k. Ce ne sont pas des temps de tour ni de G4.

| Famille | Mono8k | Mono16k | Mono32k | Quatre workers32k |
| --- | ---: | ---: | ---: | ---: |
| Uniforme | 2,110 | 4,842 | 10,506 | 2,743 |
| Terrain | 0,572 | 1,184 | 2,556 | 0,666 |
| Amas | 1,549 | 3,788 | 8,715 | 2,262 |
| Rangées | 0,228 | 0,472 | 0,968 | 0,245 |

À uniforme32k, le q2 mono historique prenait29,03 s. Le gain est donc
substantiel, mais le contrat50k exige **toute la tour1..10 sur G4 sous
une seconde**, avec repli sur toute la tour1..5 : il demeure ouvert.

Les visites du census font×2,03 à×2,58 aux doublements de ces séries
optimisées. C'est une croissance mesurée favorable, pas un théorème global.
La somme des facteurs Pool des rangées peut encore faire×4 ; leur masse
candidate demeure quadratique malgré le traitement partagé. Le volume
de sortie FULL peut lui-même être quadratique dans certaines familles.
Il faut distinguer ce coût nécessaire des répétitions évitables.

## Inventaire au point de départ de la reprise

| Pièce | Acquis | Reste à construire |
| --- | --- | --- |
| Géométrie q2 | Chaîne CPU, collecte exacte, options qualifiées | Garder la référence et ses contre-régimes |
| Front q3/q4 | Masques et certificats géométriques de base | Accès complet aux supports, filtres collectifs et héritage multivoie jugé |
| q3 | Stratégie et contre-exemples exécutables | Génération arête×bloc, forme et census, ownership exact |
| q4 | Identités de famille et oracle indépendant borné | Événements, tri/groupes, positivité, ownership, génération complète |
| Catalogue | Contrat mathématique de boule et seuil par voie | Clé commune, déduplication, collecte unique, niveau exact |
| Tour FULL | Fondements et prototypes v7 audités | Parents, portails silencieux, multifusions, plateaux, verticales |
| Déploiement | Scripts GCP gardés, autorisation SPOT | Backend v8 GPU, résidence et preuves CPU/GPU, CLI/export |

Les graphes k-NN ordinaires ne remplacent pas les hiérarchies HGP : les
composantes portent les facettes et peuvent se recouvrir sur les points.
Ne conserver que les minima Gabriel sans leurs rattachements silencieux
est faux ; les contre-exemples existants restent des portes obligatoires.

## Décisions de reprise

1. Garder le défaut q2 historique pour la compatibilité ; employer le
   profil explicite `{2,16,true}` comme bras optimisé des comparaisons
   pertinentes, avec sa jumelle historique et les rangées défavorables.
2. Livrer la [famille q4 exacte](Q4_FAMILLE_ET_BALAYAGE_20260920.md) :
   une seed aiguë, événements sur les sites, tri exact et profondeurs par
   groupes. Elle retire le census répété par quatrième sommet, sans
   prétendre résoudre la génération de toutes les seeds.
3. Raccorder ensuite accès arête×bloc q3/q4, positivité et ownership,
   puis les clés communes de boules et leur collecte. Ne pas accéder à
   q3 via les seuls q2 acceptés, ni à q4 via les seuls q3 acceptés.
4. Construire une tranche FULL minimale avec oracle indépendant des
   parents et plateaux. Mesurer les volumes avant de généraliser la
   reconstruction parallèle ; le tri seul ne livre pas une hiérarchie.
5. Porter les objets retenus sur GPU et tester G4 SPOT dès qu'une
   expérience GPU utile est prête. Les primitives indépendantes n'exigent
   pas d'attendre tout FULL, mais leur vitesse ne sera jamais un contrat
   de tour. Mesurer ensuite séparément50k et les dizaines de millions.

Le premier noyau q4 ne construit ni catalogue de tétraèdres ni liste
des intérieurs à chaque événement. Les états de seed et les segments
d'événements préparent une production parallèle ; le tri et les sommes
segmentées sont des consommateurs identifiés, pas une promesse GPU.

## Livraison de cette reprise

La famille q4 est implémentée dans `src/lanes/q4_family.*`.82 CTests Release
passent ; la nouvelle gate et trois sondes passent aussi sous Clang
ASan/UBSan (pas une réexécution de toute la suite sanitizer historique).
L'oracle rationnel indépendant compare206 appels sur57 nuages,67 354
contrôles ; les cas de frontière et de profondeur décroissante sont exercés.
Les trois captures, quinze mesures et dix lectures/contrôles normal/−O sont
[clos et épinglés](../receipts/reprise_20260920/README.md).

Une famille uniforme de8k/16k/32k sites prend3,942/8,153/17,464 ms,
callback de validation compris, sur un CPU local. Le tri croît de×2,083
puis×2,161 ; les buffers doublent. Le producteur de vues est O(n+e log(1+e)),
e≤n, par seed ; tout travail supplémentaire du consommateur s'ajoute.
Ces résultats ne prouvent pas une borne globale pour toutes les seeds,
ni la complétude d'un générateur q4, ni un contrat de tour.

Le q2 n'a pas été modifié. Aucun backend GPU n'étant prêt pour cette
nouvelle brique, aucune ressource GCP n'a été engagée dans cette reprise.

## Retours indépendants arrivés pendant la reprise

La [contrelecture A](../audits/front_options_lidar_20260920/README.md),
publiée à94960c5e sur le socle3e94c868, conserve40 mesures sur trois scans
KITTI08 séparés. Pour le premier scan50k/K10/s8/Pool64, médianes mono :
défaut9,604 s, `{2,16,true}`5,307 s, `{4,all,true}`4,774 s.
Ce sont des mesures indépendantes du pipeline q2 local, pas de la tour
ou de G4. La référence2K est maintenue ;4K reste un candidat à comparer
sur LiDAR, pas un nouveau défaut. Trois fixtures de cet audit précisent
les promotions de voie et le transport des listes pour le futur q3/q4.

L'[audit des facettes silencieuses](../audits/FACETTES_SILENCIEUSES_REPRISE_V7_20260919.md),
à c92aad13, ajoute un contrat de port essentiel pour FULL : garder les MEB
intermédiaires même hors catalogue, résoudre avant le niveau consommateur,
clore les plateaux atomiquement et publier aussi les ancres inertes.
La fixture A(0,1,0), B(2,5,0), C(4,1,0), D(1,0,0), E(3,0,0), Kmax2
doit être portée : remplacer le MEB de la facette silencieuse AC par
une autre boule au bon rang/date donne tout de même de mauvais parents.

Son optimisation par petits pivots MEB, suivie d'une canonisation sur
la coquille **sélectionnée dans F**, est prometteuse mais reste un prototype
indépendant. Cette coquille de taille≤K n'est pas la coquille globale du
census ou de la famille q4. Ne pas importer le plafond12 v7 ; le coût
des grandes coquilles et de leur quotient reste à traiter explicitement.

## Points laissés à l'auditeur

La demande de contrelecture a été inscrite dans le
[journal de coordination](../../audits/COORDINATION_MORSEHGP3D_V8.md).
Elle porte sur le signe du comparateur réduit, les groupes mêlant
entrées/sorties, les témoins coplanaires et le périmètre des futurs covers.
Les retours antérieurs de B restent attachés à leurs sources : son passage
au rôle de constructeur ne crée aucune validation indépendante de son code.
Le retour A du20 septembre juge cohérent ce contrat limité de famille q4,
sans le convertir en qualification du moteur ni de la génération complète.
