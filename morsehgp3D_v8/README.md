# Morse HGP 3D v8 — reprise q3/q4 après audit du socle q2

Ouverture demandée le 13 septembre 2026, sur `main` uniquement.

```text
phase=exploration_v8_hors_registre
backend=cpu_reference
profile=quantized_u16_input_only
mode=implementation_v8_p0
public_status=not_claimed
```

L'audit d'ouverture est suivi, sur demande explicite du 13 septembre,
de l'implémentation P0 mono-thread. Aucun code moteur ni résultat de
performance n'est repris automatiquement. La cible reste toute la tour
HGP FULL K=1..10 à 50 000 points sous une seconde **sur G4**, repli sur
toute la tour 1..5, puis 100 ms sur cette cible. Les tests locaux mono
sont des étapes d'optimisation ; la grande échelle G4 est un contrat distinct.

La structure v7 est conservée pour organiser la refonte : `src/`, `cli/`,
`oracle/`, `tests/`, `bench/`, `cmake/`, `docs/`, `audits/`, `receipts/`.
Le module C++20 compare trois méthodes de crédits locaux sur un
rectangle séparé : pool directionnel, parcours conjoint de blocs et tubes
à suffixes certifiés. Il partage maintenant la préparation des tubes
entre q2/q3/q4 et ajoute un filtre q2 par colonnes exactes et index B.
Le mode additif cumule les témoins disjoints de ces colonnes ; une
intersection facultative conserve les rejets d'un plan local q2.
Les résidus sont conservés en sous-produits ou plages compacts. Un nouveau
census q2 interroge tous les sites, compte par paire ou par groupes, puis
émet les supports sous le seuil avec leurs IDs intérieurs et de coquille.
Un front WSPD réel couvre maintenant les paires ou les rejette par voie,
sans développer les produits. Son raccord q2 réutilise maintenant les
nœuds du même index et collecte les supports complets ; ce n'est ni un
catalogue q3/q4 ni une tour FULL.
Les mesures de l'audit d'ouverture restent v7 ; chaque tranche v8 porte
ses propres sources, tests et reçus, sans transfert implicite.

## État exécutable

Reprise du20 septembre : [audit du développement](docs/REPRISE_DEVELOPPEMENT_20260920.md)
après `3e94c868`. Les preuves de la tranche21 sont relues et leurs empreintes
vérifiées ; les gains q2 sont conservés sans nouvelle micro-variante.
Le [balayage exact d'une famille q4](docs/Q4_FAMILLE_ET_BALAYAGE_20260920.md)
est maintenant implémenté et qualifié : événements groupés et profondeur
partagée.82 CTests Release passent, ainsi que la nouvelle gate q4 sous
Clang ASan/UBSan ; mesures d'une famille à8k/16k/32k et lectures normal/−O
closes. Ce n'est pas encore un générateur complet q3/q4, un catalogue ou
une tour FULL. Le profil optimisé q2
`{2,16,true}` reste explicite ; aucun défaut historique n'est changé.
GCP non utilisé pendant cette reprise.

Suite du20 septembre : [candidats positifs q3/q4 par seed](docs/Q3_Q4_CANDIDATS_PAR_SEED_20260920.md)
qualifiés :84 CTests Release, deux nouvelles gates et six sondes sous
Clang ASan/UBSan, trente mesures dont dix-huit8k/16k/32k à K5/10.
Une clé primitive commune reconnaît la même boule
entre arités ; le raccord applique profondeur, positivité, propriété et
seed canonique, sans dépendre de l'acceptation q2/q3. La génération des
seeds, la déduplication globale, les intérieurs et FULL restent à raccorder.

La tranche24 introduit une [région de témoins partagée par arête](docs/Q3_Q4_COVERS_PARTAGES_20260920.md)
et trouve ses faces admissibles par blocs de l'index. Le nuage entier n'est
plus balayé par face lorsque cette région est petite. Le travail peut
cependant rester quadratique lorsque les faces et les témoins conservés
croissent ensemble ; ce n'est pas encore un générateur global sous-quadratique.
Qualification :85 CTests Release, trois portes/six sondes sous Clang
ASan/UBSan, trois mutants compilés et32 mesures closes. Voir les
[résultats et contre-régimes](receipts/q34_cover_20260920/README.md).

La tranche25 ajoute le [rejet universel d'une famille entière](docs/Q3_Q4_REJET_FAMILIAL_20260920.md)
avant ses événements : petit pool partagé par arête, décisions q3/q4
indépendantes, repli exact à compte zéro.86 CTests Release, quatre
portes et18 sondes Clang ASan/UBSan passent ;96 mesures et lectures
normal/−O sont closes. Sur l'adversaire256, pool64, les lectures tombent
de65 024 à15 616 (K5) ou33 536 (K10). Les fonds8k/16k/32k n'ont aucun
rejet supplémentaire : le filtre n'est pas un gain universel. Le terme
faces survivantes × taille du cover reste ouvert ; cette option ne
qualifie ni un générateur sous-quadratique global ni la tour G4.
Les [reçus](receipts/q34_pruning_20260920/README.md) comptent préparation,
propositions, racines entières, scans, tris et sorties, avec contre-régimes.

La tranche26 qualifie le [minimum collectif et la corde resserrée](docs/Q3_Q4_CERTIFICAT_COLLECTIF_20260920.md)
sur le même pool, quatre options explicites.87 CTests Release, cinq
portes/24 sondes Clang ASan/UBSan, trois mutants compilés et208 mesures
passent. L'adversaire256/C64 passe de15 616 à10 667 lectures K5 et
de33 536 à21 266 K10, sorties inchangées et tri du filtre payé.
Cela ne règle pas le grand dense : à32k/C64, au moins185,344M/358,240M
lectures de repli resteraient nécessaires pour K5/10. Ce repli n'a pas
été exécuté ; les48 filtres auxiliaires l'ont quantifié. Prochaine
priorité : partager les certificats entre faces dans le plan des centres.
Pas de sous-quadratique global, de FULL ou de contrat G4 acquis. Voir
les [preuves et limites](receipts/q34_collective_20260920/README.md).

La tranche27 ajoute une [carte q4 partagée entre faces](docs/Q4_CARTE_CENTRES_PARTAGEE_20260920.md),
raffinée à la demande, et prépare le domaine positif par blocs du même
index.88 CTests Release, six gates/24 sondes Clang ASan/UBSan, trois
mutants compilés,20 différentiels contre26 et128 mesures principales
passent ;48 filtres denses auxiliaires sont qualifiés séparément.
**Résultat de performance modeste, aucun gain stable** : avec le même
pool64 après26, le dense32k garde au moins185,344M/357,472M lectures
futures à K5/10, non exécutées. La carte ne trouve pas de nouveaux témoins.
Le défaut reste inchangé ; suite prioritaire, blocs témoins et traitement
exact local du résidu, sans recycler les listes de filtre comme census.
Pas de borne sous-quadratique globale, de FULL ou de contrat G4 acquis.
Voir les [résultats, limites et preuves](receipts/q4_center_map_20260920/README.md).

## Vingt-et-unième tranche publiée à3e94c868 — historique

La vingt-et-unième tranche transmet aux enfants les
[témoins certifiés du front](docs/P0_TEMOINS_HERITES_Q2.md) : un produit non
rejeté passe à ses deux enfants les **rangs** de ses témoins certifiés (au
plus Kmax − 1, portés par valeur dans la tâche) ; l'enfant part de ce compte,
saute sans test tout rang déjà reçu et reste rejeté par Kmax rangs distincts.
Option explicite `WspdFrontProposals::inherit_witnesses`, voie q2 seule,
transmise au front mono, aux jobs, au dispatch et aux cinq entrées q2 ; **le
moteur sans héritage reproduit la tranche 20 compteur pour compteur**, pour
les trois fenêtres (capture différentielle contre son build épinglé).

Qualification propre close le 17 septembre 2026 : 81 CTests Release et
Clang ASan/UBSan, portes héritage, dispatch et jobs sous Clang TSan, rejeu
indépendant du front q2 (1 125 exécutions), modèle Python indépendant, oracle
de sûreté par force brute, dix mutants causaux tués dont quatre non sûrs,
990 appels des cinq entrées contre l'oracle, 854 mesures et lectures
normal/−O identiques. Temps mur du pipeline q2 complet, exécution avec
héritage rapportée à sa jumelle de même fenêtre, n8k/16k/32k, K5/10,
s8/10/12, un et quatre workers : fenêtre 2K petits facteurs, uniforme ×0,86
à ×0,93, amas ×0,88 à ×0,96, terrain ×0,89 à ×0,96, **rangées ×0,99 à ×1,03,
aucun gain** ; fenêtre historique, uniforme ×0,50 à ×0,63, amas ×0,57 à
×0,72, terrain ×0,68 à ×0,75. Avec la fenêtre 2K, le temps q2 rapporté au
front historique devient ×0,35 à ×0,47 (uniforme), ×0,43 à ×0,56 (amas),
×0,58 à ×0,65 (terrain). Supports identiques. Une reprise exacte de la
descente du proposeur a été mesurée (×0,94 à ×0,98) et non portée. Lire les
[reçus](receipts/q2_front_inheritance_20260917/README.md). La croissance du
travail restant est inchangée : ni borne sous-quadratique, ni P0 global, ni
contrat FULL/G4. GCP non utilisé.

## Vingtième tranche publiée à 8190e7ab — historique

La vingtième tranche porte au front la [surproposition de témoins](docs/P0_SURPROPOSITION_TEMOINS_Q2.md) :
le seuil de rejet reste Kmax, mais un produit que la fenêtre historique de
Kmax rangs ne rejette pas peut proposer les deux intervalles qui complètent
une fenêtre de 2·Kmax ou 4·Kmax rangs autour du même pivot, crédits
conservés, sans nouvelle descente. Option explicite `WspdFrontProposals`,
réservée à la voie q2, transmise au front mono, aux jobs, au dispatch et aux
cinq entrées q2 ; **le défaut reproduit le moteur précédent à l'unité**
(capture différentielle contre le build épinglé de la tranche 19).

Qualification propre close le 17 septembre 2026 : 78 CTests Release et
Clang ASan/UBSan, portes proposals et dispatch sous Clang TSan, rejeu
indépendant du front q2 (1 575 exécutions, dix mutants causaux tués), oracle
force brute sur les cinq entrées (990 appels), 684 mesures et lectures
normal/−O identiques. Fenêtre 2K « petits facteurs », temps mur du pipeline
q2 complet rapporté à la fenêtre historique, n8k/16k/32k, K5/10, s8/10/12,
un et quatre workers : uniforme ×0,42 à ×0,54, amas ×0,51 à ×0,62, terrain
×0,64 à ×0,71 ; **rangées ×1,01 à ×1,18, régression conservée** (presque
aucun témoin universel). Candidats du census à 26 à 40 % de la référence
hors rangées, supports identiques. Lire les [reçus](receipts/q2_front_proposals_20260917/README.md).
La croissance du travail restant est inchangée : ni borne sous-quadratique,
ni P0 global, ni contrat FULL/G4. GCP non utilisé.

## Dix-neuvième tranche publiée à 8d615cfd — historique

La dix-neuvième tranche a testé les [lots de singletons q2](docs/P0_LOTS_SINGLETON_Q2.md) :
des recherches indépendantes partagent un moteur par worker et avancent en
alternance, sans changer leur ordre de témoins. **Résultat négatif qualifié
le 17 septembre 2026.** Le format est exact (75 CTests Release et Clang
ASan/UBSan, porte Clang TSan, 595 appels à lots contre oracle, tous les
comptes géométriques égaux à Coarse), mais il est plus lent que Coarse dans
les 54 comparaisons closes à n8k/16k/32k : rapport lots / Coarse de 1,005 à
1,225, médiane 1,112, et seize voies entrelacées ne font pas mieux qu'une.
L'entrée `run_wspd_q2_census_batched` reste explicite et hors défaut ;
Coarse reste le défaut. Lire les [reçus de clôture](receipts/q2_singleton_batch_20260915/README.md).
La piste « lots compacts » est inscrite aux [fausses pistes](docs/FAUSSES_PISTES.md).

## Dix-huitième tranche publiée à2741d614 — historique

La dix-huitième tranche ajoute le [partage de plages d'ancres](docs/P0_PLAGES_ANCRES_Q2.md)
non commencées. Le receveur réutilise son moteur privé, sans allocation
de continuation par ancre. Pool est préparé une fois par rectangle,
possédé avec son index et partagé entre les bandes. Son repli sans
filtrage reste Shared ; il peut maintenant partager ses ancres.

Le nouveau point d'entrée est explicite, sans changer le défaut Coarse.
Les [preuves propres](receipts/q2_anchor_ranges_20260915/README.md) sont
closes :72 CTests Release et Clang ASan/UBSan, gate Clang ThreadSanitizer,
415 appels ranges/135 Coarse contre l'oracle et tous les comptes
géométriques ; coquille30 et dons de bandes Pool filtrées exercés.
174 mesures n8k/16k/32k, K5/10, s8/10/12, lecteurs/analyseurs normal/−O
concordants. Les trois builds sont désormais épinglés.

La file porte des plages de72 octets, sans pile réservée par ancre.
Les mesures observent278 dons Shared/repli, dont52 après attribution des
seeds ; le partage Pool filtré est exercé par les gates, pas par ces grands
nuages au grain64. Aucun gain stable établi sous charge concurrente.
Les six postes principaux restent sous×3 aux doublements sur Pool64,
mais F des rangées fait×4 et certains ratios de scheduling dépassent×4.
Aucune borne générale sous-quadratique. La suite visait alors les millions
de petits census en lots compacts ; cette piste a depuis été mesurée et
fermée par la dix-neuvième tranche. FULL, q3/q4 produit, GPU et contrats G4
restent ouverts. GCP non utilisé.
Les [deux fixtures de tangence q3](docs/Q3_Q4_OBJETS_ET_STRATEGIE_20260914.md)
précisent le compte strict sur les rangées après dialogue avec B.

## Dix-septième tranche publiée à beee3341 — historique

La dix-septième tranche raccorde les [branches q2 à une équipe persistante
commune au front](docs/P0_EQUIPE_PERSISTANTE_Q2.md). Plus de création
d'équipe par ancre dans cette entrée : les workers peuvent reprendre
des frères B non visités sans refaire leurs préfixes. Les petits census
et les rectangles Pool restent synchrones ; Coarse reste le défaut existant.

Les [preuves propres](receipts/q2_cooperative_20260915/README.md) comprennent
69 CTests Release et Clang ASan/UBSan, la gate Clang TSan,235 appels coopératifs contre91
références et un oracle indépendant, et174 mesures closes à1/4workers,
n8k/16k/32k, K5/10, s8/10/12. La reprise ASan/UBSan est close après
interruption de sa première capture par redémarrage ; celle-ci reste incomplète
et conservée. Les trois builds de qualification sont désormais épinglés.

**Pas de gain de vitesse établi.** Les dons sont réellement exercés sur
les rangées sans Pool, mais les continuations restent trop coûteuses dans
ces observations. Les six postes principaux sont sous×3 aux doublements
sur les quatre séries Pool64 ; F des rangées fait néanmoins×4 et le résidu
Pairwise des amas jusqu'à×3,447. Aucune borne générale sous-quadratique.
La suite vise les plages d'ancres et plans Pool possédés, puis les millions
de petites requêtes en lots compacts. FULL, q3/q4 produit, GPU et contrats
G4 restent ouverts ; GCP non utilisé.

## Seizième tranche publiée à897085f8 — historique

La seizième tranche ajoute le [détachement des frères B et un répartiteur
intérieur](docs/P0_DETACHEMENT_CENSUS_Q2.md). Une même ancre peut maintenant
être traitée simultanément par plusieurs workers, sans refaire ses préfixes
ni perdre son compte acquis. **66 CTests Release et Clang ASan/UBSan passent**,
ainsi que la gate Clang TSan du répartiteur. Les
[preuves propres](receipts/q2_census_split_20260915/README.md) comprennent
302 scénarios de détachement,2 304 appels parallèles,144 mesures à8k/16k/32k,
K5/10, s8/10/12 et12 mesures du chemin q2 complet inchangé.

Les sorties et les36 compteurs géométriques restent identiques au mono.
Les principaux comptes du q2 complet restent sous×3 aux doublements sur
les quatre familles mesurées, sans borne générale. **Aucun gain de vitesse
général** : la plupart des ancres sélectionnées sont trop petites pour payer
une création d'équipe. La prochaine intégration réutilisera les workers
persistants du front et des plans Pool parentaux possédés ; elle reste à faire.
Les trois builds de qualification sont épinglés. FULL, GPU et contrats G4
restent ouverts ; GCP non utilisé.

La contrelecture B a aussi permis de préciser pourquoi une plage multiple
ne peut pas être admise dans ce raccord B/Z : la règle des diagonales force
sa division. Pour q3/q4, un nouvel oracle indépendant B5124095b est disponible ;
les moteurs produit ne sont pas encore implémentés.

## Quinzième tranche publiée à d09e2207 — historique

La quinzième tranche ajoute un [census q2 reprenable](docs/P0_CENSUS_REPRENABLE_Q2.md)
possédant son index et son état de parcours. Les 62 CTests Release et
Clang ASan/UBSan passent, ainsi que la gate Clang TSan. Les
[preuves propres](receipts/q2_census_resume_20260914/README.md) comprennent
768 reprises contre référence et oracle, 144 mesures d'ancres sélectionnées
à8k/16k/32k, K5/10, s8/10/12, et12 mesures de non-régression du q2 complet.
Lecteurs/analyseurs normal/−O concordent. Les trois builds sont épinglés.

La pile peut passer d'un thread à un autre sans refaire ses tests, mais ses
frères B ne sont pas encore répartis simultanément. Le pipeline/Pool existant
reste inchangé ; ses principaux comptes conservent une croissance sous×3
sur les quatre séries mesurées, sans borne générale. Aucun nouveau gain de
vitesse ni contrat de tour n'est revendiqué. Suite : détachement des frères
pendants et plans parentaux possédés, puis raccord au répartiteur.

La [stratégie q3/q4](docs/Q3_Q4_OBJETS_ET_STRATEGIE_20260914.md) distingue
les voies, les blocs de triangles et les événements q4. Six contre-fixtures
rationnelles montrent pourquoi les supports acceptés de la voie précédente
ne peuvent pas servir de filtre d'accès. Un comparateur q4 réduit en i128,
discuté avec B, est prouvé mathématiquement mais pas encore implémenté.
FULL, GPU/G4 et plusieurs dizaines de millions de points restent ouverts.

## Quatorzième tranche publiée à4e878754 — historique

La quatorzième tranche ajoute la [redistribution des produits pendants](docs/P0_REDISTRIBUTION_FRONT_Q2.md).
Le raccord `Donate` est implémenté :57 CTests Release/Clang ASan/UBSan,
deux gates Clang ThreadSanitizer et32 différentiels passent. Les
[192 mesures propres](receipts/q2_dynamic_front_20260914/README.md) sont
closes, lecteurs et analyseurs normal/−O identiques. La redistribution est
exacte sur les fixtures, mais son gain n'est pas général : modestes gains
LiDAR, régression mesurée sur amas8k, temps dispersés. `Coarse` reste donc
le défaut. Les principaux comptes géométriques des cinq séries8k/16k/32k
restent sous×3 à chaque doublement, sans borne générale ; certains comptes
variables de dons/attentes dépassent×4 et sont explicitement conservés.
Les quatre nouveaux builds sont épinglés. Prochaine priorité : continuations
possédées pour les gros census indivisibles, sans préparer les facteurs par
tranche. La tour FULL/G4, q3/q4 et le massif restent ouverts ; GCP non utilisé.

## Treizième tranche publiée à b268cf6f — historique

La treizième tranche répartit les [sous-arbres du front entre workers](docs/P0_FRONT_WORKERS_Q2.md).
Le nuage et l'index sont partagés en lecture seule ; chaque worker possède
son moteur de comptage et ses buffers de collecte. Les tests des parents
ne sont pas répétés. La sortie complète q2 et tous les compteurs
géométriques doivent être identiques au mono, indépendamment du nombre
de workers et de la granularité. Le nombre de jobs règle la distribution,
jamais la quantité de résultats conservés.

La [qualification propre](receipts/q2_front_workers_20260914/README.md)
est close : 53 CTests Release et53 Clang ASan/UBSan passent, ainsi que
la porte ThreadSanitizer. Les 32 différentiels contre l'ancien mono
conservent tous les champs hors chronos. Les 134 mesures sont closes,
avec lecteurs normal/−O identiques. À 8k/K10, sur quatre cœurs physiques,
les médianes de trois essais donnent ×3,91 uniforme, ×3,40 terrain,
×3,89 amas et ×1,93 rangées entre un et quatre workers. Les principaux
compteurs restent sous le quadruplement à chaque doublement8k/16k/32k,
sans borne générale. Les mesures sur deux cœurs/quatre SMT sont séparées.
Aucun nouveau contrat FULL/G4 n'est revendiqué. Les mesures détaillent
le temps mur, les sommes d'intervalles des workers, le préfixe séquentiel
et le déséquilibre ; ces différentes unités doivent rester distinctes.
Les builds Release, ASan/UBSan et ThreadSanitizer sont épinglés.
Suite : redistribuer les sous-arbres pendants, puis traiter les gros
rectangles encore indivisibles. q3/q4, FULL et GPU restent ouverts.
GCP non utilisé.

## Douzième tranche publiée à ba11e3ab — historique

La douzième tranche raccorde [Pool terminal au census global](docs/P0_POOL_TERMINAL_Q2.md).
Un plan par rectangle, sans nouvelle copie du nuage, sélectionne au plus
K bandes résiduelles. Le census de leurs paires repart de zéro sur tous
les sites. Si le filtre ne retire aucune paire, le parcours initial est
conservé ; sa préparation supplémentaire reste comptée. Le défaut demeure
Pool désactivé. Le seuil de taille choisit une méthode, jamais un plafond
de sortie ou de recherche.

Les [preuves et mesures propres](receipts/q2_terminal_pool_20260914/README.md)
sont closes : **49 CTests Release/Clang ASan/UBSan passent**, lecteurs
normal/−O identiques et **80 mesures** appariées K5/K10, s8/10/12 à8k
puis croissance à16k/32k à s8. Sur les amas/K10/s8, le total q2 mono
passe de 13,412/47,179/184,306 s sans Pool à **3,589/7,614/19,180 s**
avec Pool64. Les visites census font alors ×2,958/×2,701, contre
×4,106/×4,229 sans filtre. Ce régime mesuré progresse réellement,
sans preuve générale de croissance sous-quadratique. Uniforme et terrain
ne sélectionnent aucun plan : leurs variations de temps ne sont pas
des gains du filtre. Les résultats du prototype A ne sont pas hérités.
Prochaine priorité : paralléliser les sous-arbres du front et leurs petits
rectangles, avec moteur/collecteur privés et contextes possédés.
P0, q3/q4, FULL, multi-CPU, GPU et contrats de tour restent ouverts.
GCP non utilisé.

## Onzième tranche publiée à b2106c3c — historique

La onzième tranche implémente le [census conjoint A×B](docs/P0_CENSUS_CONJOINT_Q2.md)
et son bras A seul, avant le passage aux ancres individuelles. Compte,
curseur et phase sont conservés ; aucun groupe A n'est retiré des témoins.
Les [mesures r2](receipts/q2_joint_r2_20260914/README.md) distinguent
bornes conjointes, reprises et travail après relais. Le partage A/B
équilibré fragmente trop B ; A seul évite cette explosion, sans gain
net déjà acquis. Individual reste le défaut.

Le collecteur Python est aussi corrigé contre la perte de sorties à
l'interruption ; six contre-tests déterministes supplémentaires sont
conservés. La première capture et son échec restent historiques, sans
remplacement silencieux. **47 CTests Release/Clang ASan/UBSan passent**,
lecteurs normal/−O identiques, 44 mesures closes. À s8, le bras A seul
prend 12,121/45,934/182,155 s sur amas8k/16k/32k ; ses visites après
relais font ×4,107 puis ×4,231. La croissance reste donc non résolue
dans ce régime, malgré des supports proches du linéaire. s8/10/12
sont comparés à 8k ; la croissance nouvelle est mesurée à s8 seulement.

Prochaine priorité : [Pool terminal sur le même index global](docs/P0_POOL_TERMINAL_RACCORD.md),
puis census des seules survivantes. L'auditeur A a mesuré un gain q2
complet dans son prototype publié à fbbecc01 ; ce raccord n'est pas
encore intégré au produit. P0, q3/q4, FULL, parallélisation et contrats
G4 restent ouverts. GCP non utilisé.

## Dixième tranche publiée à e3af11a7 — historique

La dixième tranche expérimente un [ordre de témoins propre à la requête](docs/P0_ORDRE_TEMOINS_Q2.md) :
complément du B original sans l'ancre, puis B original. Le contexte et
la continuation restent compacts ; aucune liste de frontières ni
préparation de facteur n'est ajoutée. Les coquilles sont inchangées.
Le défaut reste l'ordre DFS global. Les [56 mesures propres](receipts/q2_witness_order_20260914/README.md)
sont closes ; **45 CTests Release et Clang ASan/UBSan passent**.
À s8, avec le frère, les temps q2 8k/16k/32k valent
0,201/0,437/0,942 s sur les rangées et 11,378/43,750/173,471 s sur
les amas. Ces derniers restent non résolus : visites géométriques
×4,106 puis ×4,229. Le nouvel ordre ne gagne pas en temps sur uniforme8k ;
son coût structurel est publié séparément. s8/10/12 comparés à 8k,
croissance à s8 seulement, sans nouveau temps baseline aux grandes tailles.
La prochaine tâche partagera encore A et B avant le passage aux ancres,
sans réinitialiser le compte ni sa continuation. Non implémentée ici.
P0, q3/q4, FULL, multi-CPU, GPU et contrats G4 restent ouverts.

## Neuvième tranche publiée à 39b58f37 — historique

La neuvième tranche ajoute un [certificat autonome du bloc frère](docs/P0_CERTIFICAT_FRERE_Q2.md)
au raccord q2, en option. À chaque division d'un groupe B, l'autre
enfant peut certifier à lui seul K intérieurs et permettre un rejet
immédiat. Aucun crédit n'est ajouté au compte hérité ; aucun curseur
Z n'est modifié. Le défaut Disabled et le chemin Pairwise sont conservés.
Les [32 mesures et leurs qualifications](receipts/q2_sibling_20260914/README.md)
sont closes : **44 CTests Release et Clang ASan/UBSan passent**.
Sur les rangées, le total 8k passe de 1,494 à 0,241 s ; avec le frère,
8k/16k/32k prennent 0,241/0,481/0,969 s et les évaluations géométriques
font ×2,20 puis ×2,13. Mais les amas font encore ×4,29 puis ×4,22 :
la chaîne n'est pas globalement sous-quadratique. Uniforme/terrain8k
ne gagnent aucun rejet. Les nouvelles comparaisons s8/10/12 portent sur
8k ; la montée à 16k/32k est à s8. Le mode reste expérimental.
Une course du runner à l'interruption est aussi corrigée et testée.
Prochaine piste compacte : reporter le B original dans l'ordre Z et
exclure a du seul comptage, sans retirer aucun point de la coquille.
Aucun résultat FULL, multi-CPU ou GPU n'en découle. Contrats G4 ouverts.

## Huitième tranche publiée à f7edd646 — historique

La huitième tranche [raccorde le front au census q2 global](docs/P0_FRONT_ET_CENSUS_Q2.md).
Elle ne reconstruit aucun plan local, tableau B ni arbre B par rectangle.
Le plus petit facteur fournit les ancres ; les groupes B transmettent
leur compte et leur curseur Z aux enfants. L'appel demande q2 seulement,
sans tests Xi ni travail q3/q4. Les supports distincts d'une même boule
gardent leurs incidences ; toute la coquille est réellement collectée.

Le nouveau juge indépendant compare 46 762 supports complets sur
1 255 appels intégrés, y compris les subdivisions après crédit. Les
43 CTests passent en Release et sous Clang ASan/UBSan. Les
[mesures et qualifications de cette tranche](receipts/wspd_q2_census_20260914/README.md)
restent distinctes du front trois voies historique. Le coût des ancres
et des visites Z demeure : partager l'index ne prouve pas la
sous-quadraticité. À s8, uniforme8k/16k/32k fait environ
4,68/11,16/26,13 s ; les amas 12,87/64,84/228,53 s. Sur les amas,
les visites font ×4,31 puis ×4,21 malgré des supports proches du
linéaire : ce régime échoue au critère de croissance demandé.
À 8k, Samples améliore néanmoins le total face à Pure, nettement sur
uniforme/terrain ; Shared n'est pas toujours meilleur que Pairwise.
Les continuations multi-CPU/GPU, q3/q4, les parents
FULL et les contrats de tour 50k/G4 restent ouverts. GCP non utilisé.

## Septième tranche publiée à da366f7f — historique

La septième tranche implémente le [front par nœuds partagés](docs/P0_FRONT_REEL.md).
Il n'alloue aucun plan local ni tableau de facteur par produit. Les
rejets certifiés q2/q3/q4 sont transmis aux enfants ; les propositions
coûtent au plus une descente d'index et Kmax tests de sites par produit.
`Pure` est la référence sans rejet, `MidpointSamples` le filtre proposé.
La séparation est explicitement `box_gap_diameter_v1`, pas le s v4.

Les [tests et mesures propres à ce front](receipts/wspd_front_20260914/README.md)
distinguent produits visités, propositions, rectangles et masses résiduelles.
La couverture est confrontée à un oracle indépendant sur petits nuages.
Les 40 CTests Release et Clang ASan/UBSan passent. Premier résultat
important : sur uniforme32k/s8, le filtre réduit les rectangles de
56,8 à 20,9 millions, mais coûte 37,4 s contre 4,87 s pour Pure.
Il paie 954 millions de pas d'index : **ce proposeur n'est pas encore
une optimisation de temps du front**, même s'il réduit les candidates.
Les 72 mesures closes couvrent quatre familles, 8k/16k/32k et s8/10/12.
Les tâches croissent moins vite que le carré sur ces tailles, mais le
résidu des amas est presque quadratique. Aucune garantie globale n'est acquise.
Le front conserve des produits compacts ; son temps ne comprend aucun
census ni construction de la tour. Sur les deux rangées parallèles, même
tous les témoins ponctuels laisseraient un résidu quadratique q3/q4.
La prochaine étape est le raccord direct au census global, puis les
certificats collectifs et la génération canonique q3/q4. Aucun contrat
50k/G4 ou massif n'est encore acquis. GCP non utilisé.

## Sixième tranche publiée à 85015a8c — historique

La sixième tranche sépare [nuage global et rectangles](docs/P0_NUAGE_ET_INDEX_PARTAGES.md).
Copie/unicité et index census Z sont payés une fois par nuage ; les boîtes
des facteurs sont interrogées sans rescanner leurs coordonnées. Les
**37 CTests passent en Release et sous Clang ASan/UBSan**. Les trois
identités nuage, rectangle/seuil et ordre d'index restent distinguées.
Les builds `v8_cloud_20260914` et `v8_cloud_sanitize_20260914` sont réservés
à cette qualification, pas aux travaux suivants.

Les [66 essais](receipts/cloud_reuse_20260914/README.md) confrontent R
préparations globales à une seule, avec tout le travail local et la collecte.
À 32k/R32/s8, le composant grille passe de 732–919 à 457–465 ms selon
l'ordre ; le cas déséquilibré de 409–424 à 87–95 ms. La nappe reste proche
de 16 s à cause du résidu accru par subdivision. Ce ne sont pas des tours.
Les temps sont bruités ; les compteurs, eux, restent identiques entre bras.

La préparation globale n'est plus répétée, mais **la chaîne n'est pas
encore garantie sous-quadratique** : Pool/Axis gardent un terme Ω(R|B|)
sur les rectangles A_i×B. Faire croître R avec n révèle ce coût ; le
découpage peut aussi affaiblir les filtres et augmenter le résidu. Le
prochain chantier doit traiter le vrai front, les petits facteurs et les
préparations des facteurs. Les boîtes par plages originales sont un pont,
pas le futur chemin WSPD par nœuds certifiés. Aucun GPU ni tour FULL,
contrats G4 toujours ouverts. GCP non utilisé.

Les paragraphes suivants sont l'historique des cinq tranches précédentes,
avec leurs versions et mesures propres, non transférées au moteur courant.

La cinquième tranche prépare les [bornes q2 par tâche](docs/P0_BORNES_PREPAREES_ET_PARALLELISATION.md)
dans 48 octets, sans changer parcours, compteurs ni sorties. Les **34 CTests
passent en Release et sous Clang ASan/UBSan**. Les
[124 mesures de comparaison](receipts/q2_prepared_bounds_20260914/README.md)
retrouvent le même travail sur 8k/16k/32k, s8/10/12. À 32k/K10/s8,
trois mesures par ordre indiquent une baisse du temps total Shared de
4,0–5,7 % sur grilles et 4,2–9,5 % sur nappes. Ce gain local exploratoire
ne modifie pas la complexité et ne rend pas Shared universellement gagnant.
Les doublements du temps total sont entre 1,58 et 2,41 dans ces familles,
pas une preuve globale sous-quadratique. Prochaine priorité : partager
nuage/index sur les rectangles et rendre les continuations distribuables,
sans confondre identités de nuage, rectangle, permutation et index.
Ni tour FULL ni GPU implémentés ; les contrats G4 restent ouverts.

La quatrième tranche, publiée à f4815cd4, implémente le [census q2 partagé](docs/P0_CENSUS_Q2_PARTAGE.md).
L'état de recherche tient dans un groupe B, un compte et un curseur de
l'index global ; aucune liste de continuation n'est recopiée ou allouée
par requête. Le premier gate géométrique passe 277 cas et 557 exécutions,
avec 8 376 paires confrontées à l'oracle indépendant et 10 contre-modèles.
Les **31 CTests passent en Release et sous Clang ASan/UBSan**. Le flux
conserve les supports et leurs clés exactes, sans dédupliquer encore les
boules entre supports.

Les [204 mesures appariées](receipts/q2_census_20260913/README.md) paient
maintenant génération, index, préfiltre, census, collecte et émission.
Elles couvrent 8k/16k/32k, Kmax5/10 et s8/10/12 sur rectangles fixes,
puis des essais de composant à 50k. Sur les grilles32k/Kmax10,
l'intersection ∩ Pool avec census individuel prend environ 131 ms,
contre 3,74–3,79 s après le seul filtre additif. Sur les nappes, l'addition
réduit le coût total malgré sa sélection plus chère. Le census partagé
gagne sur certains grands résidus mais perd après l'intersection : moins
de visites ne suffit pas si les tests de groupes sont plus coûteux.

À 50k/Kmax10, le composant individuel mesure 176–186 ms sur les grilles,
mais 4,05–4,09 s sur les nappes. **Ni le contrat de tour à 1 s ni celui à
100 ms n'est acquis.** La croissance mesurée est sous-quadratique dans
ces familles seulement ; WSPD complète, q3/q4, FULL et grande échelle
restent à qualifier. GCP non utilisé.

Les paragraphes suivants décrivent les trois tranches précédentes.

Vingt-six CTests locaux passent en Release GCC 13.3 et en Debug Clang 18.1
avec ASan/UBSan. Les juges géométriques indépendants utilisent des entiers
multiprécision ; le produit utilise des entiers 64/128 bits sur u16.
Les tests couvrent notamment les crédits, les frontières, les identités,
les résidus, la séparation et les contre-fixtures des auditeurs.
Le propriétaire copie les coordonnées dans un stockage privé avant
validation et interdit copie/déplacement/affectation ; le runner rejette
les reçus incohérents et conserve les sorties brutes des essais invalides.
Les affectations de plans/batches conservent intégralement leur cible
en cas de panne mémoire. Les résumés refusent les provenances déclarées
hétérogènes de builds/machines et séparent les deux ordres de mesure.

Les 594 mesures de la deuxième tranche comparent le partage et le filtre
indépendant à leurs références,
avec n=8 000/16 000/32 000, Kmax5/10, s8/10/12 et deux ordres d'exécution.
Pour trois voies actives, le partage divise par trois la préparation
tri/cellules Tubes, à plans
identiques. Le filtre axial q2 réduit le résidu des nappes alignées mais
pas celui de tous les nuages : il est notamment sensible aux rotations.
Les 729 mesures r3 restent historiques, épinglées à `3589a2c9`.
Sur ces rectangles fixes, s vérifie seulement la séparation ; **la vraie
comparaison des WSPD s8/10/12 reste ouverte**. P0 n'est pas close.

La troisième tranche ajoute [648 mesures additives et d'intersection](receipts/additive_q2_20260913/README.md).
À n32k/Kmax10, l'addition réduit le résidu de la nappe complète de
6,48 à 3,93 millions, mais ralentit sa sélection : environ 238 ms contre
57 ms. Sur la grille, l'intersection Pool donne 114 716 candidates en
34–35 ms ; Pool seul garde 378 840 candidates en 2,4–2,6 ms. Le coût aval
doit encore départager ces choix. La borne linéaire des grilles planes
alignées ne se généralise ni aux rotations ni à toute la tour.

- [Contrat et algorithmes P0](docs/P0_CREDITS_LOCAUX.md).
- [Partage et filtre axial expliqués](docs/P0_PARTAGE_ET_FILTRE_AXIAL.md).
- [Addition et intersection expliquées](docs/P0_ADDITION_ET_INTERSECTION.md).
- [Deuxième tranche publiée à 8e406f9b](receipts/shared_axis_20260913/README.md).
- [Première tranche historique](receipts/p0_local_credits_20260913/README.md).
- [Sonde reproductible](bench/P0_PROBE.md).

Construction dans un répertoire **neuf**, avec GCC/Clang, CMake et les
headers Boost pour les juges seulement :

```bash
cmake -S morsehgp3D_v8 -B build/v8_new -DCMAKE_BUILD_TYPE=Release
cmake --build build/v8_new --parallel 2
ctest --test-dir build/v8_new --output-on-failure
```

Si Boost n'est pas installé globalement, fournir `-DBOOST_ROOT=/chemin/boost`.
Cette session utilise les headers tiers Boost 1.83 déjà extraits sous
`build/v7_boost_gate/extracted/usr`, en lecture seule : aucun code moteur
ni résultat v7 n'en est repris. Le build produit seul est possible avec
`-DBUILD_TESTING=OFF`, mais ne remplace jamais les gates. Les répertoires
`build/v8_p0_r3_20260913/` et `build/v8_p0_sanitize_r3_20260913/` portent
la version corrigée ; les builds r2 et sans suffixe conservent les passes
antérieures aux corrections de propriété. Tous sont épinglés ; ne pas
les écraser pour poursuivre. La deuxième tranche est épinglée dans
`build/v8_shared_axis_20260913/` et `build/v8_shared_axis_sanitize_20260913/`.
La troisième utilise `build/v8_additive_20260913/` et
`build/v8_additive_sanitize_20260913/`, également épinglés.
La quatrième utilise `build/v8_census_20260913/` et
`build/v8_census_sanitize_20260913/`, désormais épinglés.
La cinquième utilise `build/v8_prepared_bounds_20260913/` et
`build/v8_prepared_bounds_sanitize_20260913/`, désormais épinglés ;
les suites datent du 13 et les nouvelles mesures du 14 septembre.

## Commencer ici

- **Priorité P0 confirmée :** [supprimer les histogrammes quadratiques systématiques](docs/PLAN_DE_REFONTE.md#priorité-p0--supprimer-la-préparation-quadratique-des-témoins-locaux).
  Comparer les architectures avant de choisir ; les petits ensembles de
  témoins certifiés sont une piste parmi d'autres. Coût des candidates
  restantes et travail aval inclus ; la première brique ne clôt pas cette priorité.
- [Audit général et décisions](docs/AUDIT_V7_SYNTHESE.md) : verdict, contrats,
  causes de lenteur, ce qui doit être conservé ou refait.
- [Tout l'algorithme expliqué simplement](docs/ALGORITHME_EXPLIQUE.md).
- [Plan de refonte priorisé](docs/PLAN_DE_REFONTE.md).
- [Verrous d'architecture — consignes au futur développeur](docs/VERROUS_ARCHITECTURE.md) :
  après P0, les cinq obstacles à ne pas reproduire, leurs sources,
  changements à comparer et critères de validation.
- [Fausses pistes à ne pas réintroduire](docs/FAUSSES_PISTES.md).

Pour approfondir : [fondements et objet FULL](audits/FONDEMENTS_ET_OBJET.md),
[WSPD, q2/q3/q4 et témoins](audits/WSPD_Q2_Q3_Q4.md),
[code et parallélisation](audits/IMPLEMENTATION_PARALLELISATION.md),
[mesures et contrats](audits/CONTRATS_ET_MESURES.md),
[périmètre et preuves de l'audit](audits/PERIMETRE_ET_PREUVES.md).

Verdict : dernières tours 50k publiées, environ 419 s pour 1..10 et
34 s pour 1..5 ; aucune chaîne GPU FULL industrielle ni qualification
multi-millions. Les optimisations privées ultérieures n'ont pas leur
nouvelle mesure 50k. La v8 démarre sur ces constats, sans statut hérité.

Entrées de suivi : [passation](PASSATION.md), [état de l'audit](audits/ETAT_COURANT.md).
GCP non utilisé pour l'audit d'ouverture et les douze tranches mono.
