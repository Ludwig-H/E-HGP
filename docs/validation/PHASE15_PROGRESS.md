# Progression Phase 15 — streaming transactionnel budgeté

## État

Phase `15`, incréments `15A` et `15B` implémentés, backend `reference_cpu`, profil `hgp_reduced`, mode `budgeted`, déploiement `architecture_only`, `public_status=not_claimed`.

La porte d'entrée est satisfaite par les revues de sortie des Phases 9, 10 et 11. `ExactDirectMorseBudgetTracker` livre la comptabilité interne des cinq budgets, `AtomicLinearRunStore` livre la publication atomique sur filesystem Unix local et `ExactDirectMorseChunkRunContext` raccorde désormais ce store à de vrais chunks 14A et à leurs deltas 14D fraîchement rejoués. Cette tranche ne réduit pas encore ces deltas vers le locator, le quotient ou la forêt; elle ne qualifie donc ni le volume 10 M+, ni un SLO, ni un résultat scientifique public.

## Raccord scientifique 15B

15B reconstruit et compare le plan 14C exactement une fois à l'ouverture du contexte, puis indexe en un passage un curseur immuable pour chaque lot ordre--niveau. Le rejeu d'un lot arbitraire appelle directement le même cœur 14D avec ce curseur; il ne parcourt pas les lots précédents, n'avance aucun curseur vivant et ne frappe aucun ticket 14H. Pour $B$ lots, le coût de contrôle est une construction 14C et une table $O(B)$, puis la somme des seuls rejeux demandés, jamais une reprise quadratique par préfixes.

Chaque autorité de lot engage son témoin, ses budgets 14D, son budget 10.5c, une enveloppe de ressources et surtout le stamp du locator strictement pré-lot. Une vue non possédante sur un résolveur externe fournit ce préfixe à la demande sous propriété partagée pour la durée du seul appel. Le contexte exige avant et après le rejeu le stamp attendu et impose son égalité avec le stamp du delta. Il ne possède ainsi aucun locator historique entre deux appels et ne peut pas rendre visibles trop tôt les naissances du niveau courant. La résidence et les caches internes du résolveur externe ne sont pas introspectables : l'audit les marque explicitement non audités et aucune qualification 10 M+ n'en est déduite.

Le wire 15B est big-endian, borné et canonique. Il contient l'identité et l'intervalle du chunk 14A, ses huit compteurs, ses frontières, le snapshot complet des cinq budgets, puis une entrée pour chaque lot, y compris un lot sans bras. Avant de réserver le tableau des lots, la préparation encode sous cap le préfixe fixe et comptabilise par additions contrôlées les huit octets de longueur de chaque entrée. Chaque projection scientifique est ensuite bornée par le reliquat cumulatif avant d'être retenue; le Writer final ne constitue plus la première vérification du cap global. La projection conserve budgets, niveau exact canonique, témoin, stamp strict pré-lot, intervalles, clés résolues, handles, témoins de liaison, joins de bras, décision et portée. Le décodeur refuse tailles, booléens, enums, clés, rationnels, comptes, troncatures et octets terminaux invalides; le réencodage doit reproduire exactement le payload.

Le digest d'application engage le nuage canonique, les identités sources, les lots et familles, le plan 14C, les configurations, tous les caps et toutes les autorités de lot. Le digest du checkpoint engage aussi le checkpoint source et le payload complet. Ces digests protègent le protocole; la seule autorité scientifique reste la comparaison octet par octet avec le delta 14D fraîchement reconstruit.

## Unité durable

L'unité durable est exactement un chunk 14A complet de `ExactDirectMorseIndustrialPlanResult`. Le raccord 15B lie son indice, l'intervalle contigu de lots ordre--niveau source, les compteurs 14A fraîchement reconstruits et la suite canonique de tous les deltas compacts produits pour ces lots. Un lot égal et un chunk 14A ne sont jamais coupés.

`AtomicLinearRunStore` reste volontairement générique : il ferme la contiguïté des indices et son wire, mais seul le recertificateur 15B peut établir qu'une transition correspond bien à cette unité 14A complète. Son digest de contrat lie aussi les cinq limites du store, de sorte qu'une réouverture sous des caps différents est refusée. Un `CanonicalId` tout-zéro n'est pas détourné en sentinelle : sa portée reste déterminée par le recertificateur.

Les chunks de requêtes utilisés par le callback 14L sont seulement des unités de transport propositionnel. Ils ne deviennent ni des runs, ni des checkpoints, ni des frontières de commit durable. Une interruption au milieu d'un chunk 14A abandonne ce chunk entier; la reprise repart du dernier chunk 14A complètement publié.

15A ne sérialise aucun ticket 14H, locator, tableau de parents DSU, quotient vivant ou forêt. Les digests source et successeur du store ne sont pas des checkpoints scientifiques matérialisés. Les états transitoires nécessaires à un rejeu ultérieur devront être reconstruits depuis les autorités amont et les deltas recertifiés. Cette tranche ne promet donc pas encore une reprise en place du calcul scientifique.

## Cinq budgets internes

Toute tentative porte cinq budgets explicites et indépendants :

| budget | unité interne | charge principale |
|---|---:|---|
| device | octet | allocations et scratch d'un éventuel producteur; consommation nulle sur la voie `reference_cpu` pure |
| hôte | octet | chunk complet, encodage, décodage et recertification |
| scratch | octet | ancien état, temporaire, manifeste, vérification et marge transactionnelle |
| sortie | octet | run final et métadonnées durables publiées |
| temps | nanoseconde monotone | préparation, recertification et écritures réversibles jusqu'avant remplacement de `HEAD`; synchronisation finale post-`HEAD` non mesurée |

Chaque contexte 15B possède exclusivement un tracker de session et sérialise ses évaluations. La politique effective entre dans le digest d'application. L'occupation courante device, hôte, scratch et sortie est fournie séparément par la session et charge `used_bytes` sans modifier le contrat durable. Pour chaque lot, l'autorité fournit une enveloppe fiable couvrant reconstruction du locator et rejeu 14D. Les octets device, hôte et scratch de ces enveloppes sont maximisés sur les lots séquentiels; leurs réserves temporelles sont sommées. Si $P$ est le cap de payload, $S$ le cap scientifique par lot et $B_c$ le nombre de lots du chunk, la réserve hôte logique de l'application est $2P+S+B_c\,\mathrm{sizeof}(\mathrm{ParsedBatch})+\max_b H_b$ en plus de l'occupation sessionnelle; la réserve scratch temporaire est $S+\max_b R_b$. Au rang durable $j$, l'axe sortie charge conservativement l'occupation externe puis $j(264+P)+192$ octets déjà présents, et réserve la nouvelle transition maximale ainsi que le nouveau `HEAD`; le scratch de checkpoint réserve la même nouvelle image et une marge explicite.

Une évaluation fraîche `before_run` précède tout appel au résolveur, tout rejeu et toute allocation scientifique de chunk. Une seconde évaluation `checkpoint` charge le temps monotone réellement écoulé; la préparation est refusée si cet écart dépasse sa réserve d'opération. Pour le store, ces deux appels appartiennent à un `AtomicLinearRunResourceGate` obligatoire et stateful distinct. Le store l'appelle avant le `AtomicLinearRunRecertifier`, garde la lease pendant recertification et écritures réversibles, puis la ferme juste avant le remplacement de `HEAD`; le temps observé peut utiliser les réserves d'opération et de checkpoint. La décision du recertificateur reste pure et scientifiquement idempotente sous autorités immuables : deux appels sur la même transition rendent le même verdict et ne lisent ni n'avancent l'horloge, même si les compteurs d'observation et caches non autoritatifs évoluent. Les factories 15B lient obligatoirement les deux callbacks du même contexte et la création préflighte les 192 octets du `HEAD` initial avant même le verrou. Publication et reprise utilisent l'occupation et le tracker de session courants. Le snapshot durable est vérifié contre la politique et la demande déterministe de la tentative d'origine, mais il n'autorise jamais les ressources d'une nouvelle session. Une reprise dont l'occupation hôte ne laisse qu'un octet est ainsi refusée avant le premier locator même lorsque le snapshot historique avait été accepté.

Le schéma public v2 exprime le temps en secondes et son `BudgetSnapshot` ne possède aucun champ de réserve temporelle. Même lorsqu'une conversion numérique en secondes serait exacte, il perdrait la distinction entre temps disponible et temps réservé. 15A ne projette donc pas son snapshot interne vers `BudgetPolicy` ou `BudgetSnapshot` v2, ne l'arrondit pas et ne sérialise aucun `MorseHGP3DResult` sous ce prétexte. Une migration de schéma explicite sera nécessaire.

## Recertification obligatoire

La publication et la réouverture exigent un callback de recertification et un gate de ressources tous deux non vides; il n'existe aucune voie d'acceptation scientifique ou opérationnelle par défaut. Le gate est une autorité séparée et ne peut jamais accepter un payload. Pour chaque chunk complet, le recertificateur à décision pure vérifie l'identité et l'intervalle contre le plan 14A déjà fraîchement reconstruit par le contexte, rejoue les deltas compacts dans leur ordre canonique et atteste que le payload est canonique et dépourvu de capacité locale au processus. Il ne fait confiance ni au checksum, ni au manifeste, ni aux audits, ni aux budgets persistés. En reprise, une erreur structurée distingue refus de gate avant rejeu, dépassement après rejeu et rejet scientifique.

Une absence, un refus, une exception ou une discordance du callback échoue fermé : aucun nouveau `HEAD` n'est publié et aucun curseur scientifique n'avance. Après le retour du callback, l'image canonique déjà formée est écrite puis relue et comparée octet par octet avant publication; les contrôles d'inode et de nombre de liens interdisent ensuite qu'un nom final désigne une autre image.

## Protocole Unix local

Le premier protocole pris en charge est un namespace coopérativement verrouillé sur un filesystem Unix local offrant `flock`, `fdatasync`, création de hard-link atomique, renommage atomique dans un même répertoire et `fsync` du répertoire.

1. réserver simultanément les cinq budgets pour le chunk complet et garder l'ancien `HEAD` autoritatif;
2. former l'image canonique complète et appeler le callback de recertification avant toute écriture;
3. écrire le run sous un nom temporaire exclusif dans le même répertoire, exécuter `fdatasync`, puis relire et comparer ses octets;
4. créer par hard-link son nom final immuable, contrôler l'inode et le nombre de liens, retirer le nom temporaire et synchroniser le répertoire;
5. écrire un manifeste temporaire qui chaîne le précédent digest, le synchroniser, le renommer atomiquement vers `HEAD`, puis synchroniser encore le répertoire;
6. acquitter seulement après cette dernière synchronisation.

Le remplacement de `HEAD` est le point de linéarisation. Un temporaire ou un run final non référencé après crash reste non committé et ne peut jamais être choisi comme « plus long préfixe » implicite. La reprise lit uniquement la chaîne annoncée par `HEAD`, vérifie sa contiguïté et recertifie chaque unité avant usage.

Toute erreur observée après le remplacement de `HEAD` rend l'issue indéterminée dans l'instance courante : aucun retry local n'est autorisé avant fermeture et réouverture du store.

Ce contrat ne couvre pas les filesystems réseau, l'anti-rollback sans ancre monotone externe, ni la durabilité d'un support dont les garanties sont plus faibles que les appels Unix supposés.

## Limites et prochaine porte

15A et 15B n'ajoutent aucune cellule, coface ou incidence globale, aucun Gamma et aucune mosaïque de Delaunay d'ordre supérieur. Ils ne checkpointent ni locator, ni DSU, ni forêt; ils n'établissent ni fonctionnement à 10 000 000 de points, ni latence 50 k, ni SLO, ni `public_status=exact`. La borne hôte 15B porte sur les octets logiques transitoires de l'application, l'occupation sessionnelle fournie et l'enveloppe déclarée du résolveur. L'appelant doit inclure dans cette occupation la résidence antérieure du nuage, du LBVH, du plan 14C, de la table de curseurs et de la copie des autorités; elle n'est pas mesurée automatiquement. La borne ne couvre pas le slack de l'allocateur, plusieurs propositions basses retenues simultanément, les copies internes du store, l'espace libre réel et la synchronisation finale postérieure au remplacement de `HEAD`. Un dépassement physique après ce point de linéarisation ne peut pas être annulé. De plus, `committed_elapsed_ns` n'est ni extrait de `HEAD`, ni authentifié par lui : une reprise doit recevoir cette ancre temporelle d'une autorité monotone externe, faute de quoi le temps total inter-processus peut repartir à zéro. La qualification industrielle reste donc ouverte.

La prochaine tranche doit replier les chunks recertifiés vers un reducer hiérarchique séquentiel qui traite tout lot égal avant mutation, puis reconstruire ce reducer depuis le préfixe durable. Elle devra partager son noyau avec le builder résident afin de comparer les sorties octet par octet. Les snapshots DSU optimisés, merges externes, mmap, jalon un million et campagne 10 M+ restent ultérieurs.

## Validation de cette ouverture

Le build GCC Release strict des deux targets passe. Le CTest `morsehgp3d.hierarchy_direct_morse_budget` passe 1/1 en 0,00 seconde; il couvre les six frontières, chaque axe juste sous, sur et au-dessus de sa limite, l'ordre déterministe des refus, tous les overflows d'addition, la régression d'horloge, le digest canonique et l'absence revendiquée de projection v2.

Le CTest `morsehgp3d.hierarchy_atomic_linear_run_store` passe 1/1 en 0,02 seconde. Il couvre deux transitions synthétiques puis la réouverture, le wire borné, la recertification aux deux phases, le gate obligatoire, l'ordre strict avant--rejeu--après, une exception du recertificateur, une exception du gate après les écritures réversibles et leur nettoyage, les limites et formes hostiles, la corruption, les contrats, caps et ancres divergents, les orphelins non committés, la disparition de l'ancien `HEAD` avant renommage et une faute postérieure au remplacement de `HEAD`. Les deux dernières fenêtres imposent la réouverture au lieu d'autoriser un retry sur une autorité incertaine. Ce test générique ne remplace pas la fixture 15B au vrai chunk 14A décrite plus bas. Les deux tests 15A passent donc 2/2 en 0,02 seconde; CMake, installation et export des deux bibliothèques sont câblés.

Aucun benchmark, test massif ou GCP n'est exécuté par 15A.

## Validation courte de 15B

Le target 15B et son test compilent sous GCC 13 Release avec les avertissements stricts. Le CTest `morsehgp3d.hierarchy_direct_morse_chunk_run` passe 1/1 en 0,03 seconde sur le rejeu final.

La fixture est un vrai tétraèdre à $K=1$ sous politique `massive_external_streaming` et cap d'un lot par chunk. Elle produit deux chunks 14A : le premier contient l'entrée durable du lot sans bras, le second conserve quatre clés résolues et douze joins de bras. Le premier contexte publie le chunk zéro puis forme sans publier le chunk un; après destruction complète, un nouveau contexte dont l'occupation hôte sessionnelle diffère d'un octet conserve le même digest, reconstruit 14C une fois, recertifie le seul préfixe committé et publie le second chunk. Chaque contexte indexe deux curseurs et compte trois rejeux arbitraires, avec zéro progression de curseur scientifique et zéro ticket, locator historique, parent DSU, forêt, facette globale, coface, cellule, Gamma ou Delaunay possédé par le cœur entre deux appels. Un locator sparse externe au plus est vivant pendant un rejeu; sa résidence amont reste hors audit.

Une reprise sous le même contrat mais avec une occupation hôte sessionnelle presque saturée échoue par `AtomicLinearRunRecoveryError` avant le résolveur. Une limite sortie de 191 octets refuse le `HEAD` initial avant toute mutation du répertoire. Un second scénario place les deux lots dans un seul chunk : le cap cumulatif exact du payload passe, tandis que le même cap moins un octet échoue après un seul lot retenu et avant la rétention du second segment. Deux appels directs au recertificateur rendent la même décision sans évaluation budgétaire; le gate séparé refuse ensuite un delta temporel observé supérieur à la somme réservée, après recertification et écritures réversibles mais avant remplacement de `HEAD`, puis l'inventaire confirme l'absence du run final, de son temporaire et de `.HEAD.tmp`. La politique effective modifie le digest, et l'axe sortie accepte exactement le rang zéro puis refuse le rang un après charge conservative du préfixe durable. Une mutation interne, un octet terminal et la substitution d'un stamp locator non fourni par le résolveur échouent fermés sans avancer `HEAD`. Cette validation établit le raccord durable, son budget applicatif frais et la reprise du préfixe de deltas; elle ne constitue ni un reducer hiérarchique, ni une identité résident--streaming, ni une preuve à un million ou dix millions de points, ni une mesure du SLO 50 k.
