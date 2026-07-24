# Progression Phase 15 — streaming transactionnel budgeté

## État

Phase `15`, incrément `15A` implémenté, backend `reference_cpu`, profil `hgp_reduced`, mode `budgeted`, déploiement `architecture_only`, `public_status=not_claimed`.

La porte d'entrée est satisfaite par les revues de sortie des Phases 9, 10 et 11. `ExactDirectMorseBudgetTracker` livre la comptabilité interne des cinq budgets et `AtomicLinearRunStore` livre le codec borné, la publication atomique et le rejeu de récupération sur filesystem Unix local. Cette infrastructure générique n'est pas encore raccordée au curseur scientifique 14D; elle ne qualifie ni le volume 10 M+, ni un SLO, ni un résultat scientifique public.

## Unité durable

L'unité durable autorisée par le contrat 15A est exactement un chunk 14A complet de `ExactDirectMorseIndustrialPlanResult`. Le futur raccord applicatif devra lier son indice, l'intervalle contigu de lots ordre--niveau source, les compteurs 14A fraîchement reconstruits et la suite canonique de tous les deltas compacts produits pour ces lots. Un lot égal et un chunk 14A ne sont jamais coupés.

`AtomicLinearRunStore` est volontairement générique : il ferme la contiguïté des indices et le wire, mais seul le recertificateur applicatif obligatoire peut établir qu'une transition correspond bien à cette unité 14A complète. Son digest de contrat lie aussi les cinq limites du store, de sorte qu'une réouverture sous des caps différents est refusée. Un `CanonicalId` tout-zéro n'est pas détourné en sentinelle : sa portée reste déterminée par le recertificateur. Le raccord au vrai exécuteur reste la prochaine tranche.

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
| temps | nanoseconde monotone | calcul, recertification, synchronisation et publication |

Chaque budget conserve une limite, une consommation, une réserve non prêtable et un reliquat, avec arithmétique entière contrôlée avant mutation. Le temps conserve notamment `reserved_ns`; commencer un chunk est interdit si le reliquat ne couvre pas la réserve de recertification et de fermeture durable.

Le schéma public v2 exprime le temps en secondes et son `BudgetSnapshot` ne possède aucun champ de réserve temporelle. Même lorsqu'une conversion numérique en secondes serait exacte, il perdrait la distinction entre temps disponible et temps réservé. 15A ne projette donc pas son snapshot interne vers `BudgetPolicy` ou `BudgetSnapshot` v2, ne l'arrondit pas et ne sérialise aucun `MorseHGP3DResult` sous ce prétexte. Une migration de schéma explicite sera nécessaire.

## Recertification obligatoire

La publication et la réouverture exigent un callback de recertification fourni explicitement; il n'existe aucune voie d'acceptation par défaut. Pour chaque chunk complet, ce callback reconstruit le plan 14A depuis les autorités amont, vérifie l'identité et l'intervalle du chunk, rejoue les deltas compacts dans leur ordre canonique et atteste que le payload est canonique et dépourvu de capacité locale au processus. Il ne fait confiance ni au checksum, ni au manifeste, ni aux audits, ni aux budgets persistés.

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

15A n'ajoute aucune cellule, coface ou incidence globale, aucun Gamma et aucune mosaïque de Delaunay d'ordre supérieur. Il ne checkpoint ni locator, ni DSU, ni forêt; il n'établit ni fonctionnement à 10 000 000 de points, ni latence 50 k, ni SLO, ni `public_status=exact`.

La prochaine tranche devra raccorder les payloads compacts du chunk 14A, le tracker et le store à l'exécuteur ancré, sans sérialiser ses capacités locales. Les campagnes massives, benchmarks longs et tests GCP ne sont pas une porte de 15A.

## Validation de cette ouverture

Le build GCC Release strict des deux targets passe. Le CTest `morsehgp3d.hierarchy_direct_morse_budget` passe 1/1 en 0,00 seconde; il couvre les six frontières, chaque axe juste sous, sur et au-dessus de sa limite, l'ordre déterministe des refus, tous les overflows d'addition, la régression d'horloge, le digest canonique et l'absence revendiquée de projection v2.

Le CTest `morsehgp3d.hierarchy_atomic_linear_run_store` passe 1/1 en 0,02 seconde. Il couvre deux transitions synthétiques puis la réouverture, le wire borné, la recertification aux deux phases, les limites et formes hostiles, la corruption, les contrats, caps et ancres divergents, les orphelins non committés, la disparition de l'ancien `HEAD` avant renommage et une faute postérieure au remplacement de `HEAD`. Les deux dernières fenêtres imposent la réouverture au lieu d'autoriser un retry sur une autorité incertaine. Ce test ne remplace pas la future fixture d'intégration au vrai chunk 14A. Les deux tests passent donc 2/2 en 0,02 seconde; CMake, installation et export des deux bibliothèques sont câblés.

Aucun benchmark, test massif ou GCP n'est exécuté par cet incrément.
