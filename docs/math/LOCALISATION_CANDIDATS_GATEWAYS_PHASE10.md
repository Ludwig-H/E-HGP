# Localisation sparse des candidats de gateways — Phase 10.9

## Porte d'entrée et statut

Phase 10, backend `reference_cpu`, profil `hgp_reduced`, mode `certified`, sémantique interne `partial_refinement`. La porte locale de 10.9 est satisfaite par le locator positif sparse 10.5a et le journal de candidats 10.7, tous deux implémentés et validés avec rejeu frais. 10.9 est lui-même implémenté et validé sur l'hôte avec suivi `validated_host_software`. Le différentiel Gamma 10.8 autorise la poursuite comme falsificateur borné seulement : son succès ne devient ni une prémisse de preuve, ni une autorité produit.

Le jalon localise les facettes de suppression des cofaces candidates 10.7 dans un locator positif gelé. Il ne publie aucune union, racine, composante singleton, naissance, forêt ou `GatewayAttach`. Le `public_status` reste `not_claimed`.

## Objet factorisé

Un candidat 10.7 est la paire factorisée $(F_c,x_c)$ représentant la coface logique $Q_c=F_c\cup\lbrace x_c\rbrace$, sans clé persistante pour $Q_c$. Si $C$ est le nombre de candidats et $q_c=\lvert F_c\rvert+1$, le jalon reconstruit transitoirement toutes les suppressions

$$G_{c,i}=Q_c\setminus\lbrace q_{c,i}\rbrace,\qquad 0\leq i<q_c.$$

À la frontière produit $K=10$, on a $q_c\leq11$ et chaque $G_{c,i}$ contient au plus dix `PointId`. Le nombre d'occurrences et le nombre de clés distinctes vérifient

$$P=\sum_{c=0}^{C-1}q_c\leq11C,\qquad D=\left\lvert\left\lbrace G_{c,i}\right\rbrace\right\rvert\leq P.$$

Les clés complètes des $G_{c,i}$ sont triées et dédupliquées globalement. Une empreinte éventuelle ne constitue jamais leur identité. Chaque clé distincte est sondée exactement une fois dans le chemin scientifique, puis partagée par toutes ses occurrences.

Le résultat complet contient seulement deux arènes scientifiques :

- une projection par occurrence $(c,i)$ vers un token de facette, ordonnée par candidat puis par position supprimée et conservant le `source_batch_index` 10.7 ;
- un token par clé distincte, contenant cette clé de largeur au plus dix et soit un hit positif relatif, soit un miss latent non résolu.

La suppression de $x_c$ redonne exactement $F_c$. Les autres suppressions remplacent un point de $F_c$ par $x_c$. Aucun tableau de onze `PointId`, aucune coface globale et aucun catalogue Gamma ne persistent dans le résultat.

## Sémantique du locator

10.7 ne possède aucune autorité de liaison positive. L'appel 10.9 reçoit donc explicitement un `locator_query_witness` non nul dont `external_authority_id` coïncide avec celui du locator, selon le même seam ABI que 10.5b et 10.5c.

Le locator vivant, construit par l'API 10.5a, est une entrée fiable. Il doit rester immuable sous exclusion externe pendant toute la construction et toute la vérification. 10.9 capture son `ExactDirectSparsePositiveFacetLocatorSnapshotStamp` avant le premier travail scientifique, le contrôle après chaque sonde et immédiatement avant publication. Ce stamp détecte les mutations séquentielles mais n'est ni un verrou, ni un snapshot mémoire, ni une protection contre une data race.

Le stamp n'engage aucun niveau exact et le même snapshot courant couvre potentiellement plusieurs lots `(cardinal, niveau)` de 10.7. Un hit 10.9 ne prouve donc pas que sa liaison existait dans l'état pré-lot de `source_batch_index`. Le résultat conserve cet index mais impose `locator_snapshot_batch_level_alignment_claimed=false`. Une intégration temporelle future devra fournir un locator pré-lot par lot ou un certificat séparé d'alignement ; 10.9 ne publie aucune temporalité de quotient.

Pour chaque token, les issues sont strictement séparées :

- `relative_positive` conserve le handle de composante résolu et le témoin de liaison source renvoyés par 10.5a ;
- `latent_unresolved` conserve l'absence complète de liaison comme dette de raffinement ;
- `budget_exhausted` est un échec atomique du journal entier et n'est jamais converti en miss.

Un miss signifie seulement que la clé n'est pas liée dans le domaine positif relatif du snapshot fourni. Il ne signifie ni facette absente, ni composante isolée, ni naissance. Le locator ne rejoue pas la sémantique de son autorité externe ; tout handle positif reste `conditional_on_caller_external_replay`.

## Budgets et atomicité

Les plafonds globaux portent séparément sur le nombre de candidats sources, les occurrences de suppression, les clés distinctes, les `PointId` des clés distinctes, les visites de slots cumulées, les sauts de parents cumulés et le stockage logique. Un budget de sonde 10.5a borne en plus chaque clé distincte. Avant chaque sonde, le budget effectif de slots et de parents est le minimum du plafond local et du reliquat global correspondant; aucun travail ne peut donc dépasser un cap agrégé avant que l'échec soit observé.

Toutes les additions d'un agrégat de sonde sont préparées dans une copie puis engagées ensemble seulement si elles sont toutes représentables. Le cap de candidats est contrôlé avant le rejeu coûteux 10.7; les caps d'occurrences sont contrôlés avant les allocations proportionnelles correspondantes. Après reconstruction, les caps de clés distinctes et de points de clés sont contrôlés avant les sondes. Un overflow, un plafond global, un épuisement local de sonde, une source 10.7 non certifiée ou une divergence séquentielle du stamp laisse les deux arènes scientifiques vides. Aucun préfixe de localisation n'est publiable.

Le journal source 10.7 est rejoué fraîchement depuis le nuage, le LBVH, la façade directe, les journaux 10.2 et 10.4 et tous leurs budgets fiables. Un booléen recopié depuis 10.7 ne suffit pas à autoriser les sondes.

## Coût et structures globales évitées

La reconstruction utilise un scratch de $P$ occurrences et un ordre de tri de $P$ indices. Les comparaisons portent sur des clés de largeur au plus dix. Le coût propre, hors sondes, est $O(P\log P+KD)$ avec $K\leq10$ ; le stockage persistant est $O(C+P+KD)$ et le scratch est $O(P+D)$. Les sondes ajoutent au plus les plafonds cumulés explicitement fournis pour les slots et les parents.

Le chemin ne matérialise ni les $\binom{n}{K}$ facettes possibles, ni les cofaces globales, ni les cellules Gamma, ni les incidences globales, ni la mosaïque de Delaunay d'ordre supérieur. Son coût dépend seulement du flux direct déjà produit par 10.7 et du locator positif sparse existant. Le rejeu frais amont conserve toutefois le pire cas $O(D_{10.7}n)$ de 10.6 ; 10.9 ne qualifie donc encore ni le SLO 50 k sous la seconde, ni le profil 10 M+.

## Vérificateur frais

Le vérificateur reçoit toutes les autorités et tous les budgets séparément du résultat observé. Il borne les deux tailles observées et leur stockage de clés avant de parcourir les records, exige que le stamp courant égale celui enregistré, rejoue intégralement 10.7, reconstruit chaque suppression via le reconstructeur 10.7, redéduplique les clés complètes, relance chaque sonde depuis le locator gelé et compare récursivement le résultat attendu au résultat observé.

Il vérifie notamment $P\leq11C$, $D\leq P$, l'unicité des tokens, la partition exacte des projections par candidat, les compteurs de hits et de latents, les handles et témoins positifs, l'absence de payload sur miss, l'absence d'état partiel sur budget et l'absence de mutation du locator. Il ne reconstruit ni Gamma ni une autorité positive externe.

## Validation courte

E5 conserve `AC` partagé entre `ACD` et `ACE`, exactement deux clés positives sous collisions forcées et tous les autres tokens latents sans payload. À $K=10$, onze candidats factorisant la même coface logique produisent 121 projections vers onze tokens distincts de cardinal dix, tous latents dans un locator vide. Les sept caps globaux et les deux caps locaux passent à leur valeur exacte puis échouent atomiquement à moins un, sans dépasser les reliquats globaux de travail; le cap de candidats échoue avant le rejeu 10.7. La suite couvre aussi $C=P=D=0$ avec exactement deux contrôles de stamp et aucune sonde, les témoins nuls, divergents ou de token nul, un locator non certifié, un LBVH étranger, un ordre invalide, le commit vide qui périme le stamp, la promotion ultérieure d'un miss latent, une source 10.7 falsifiée, les mutations de clés, projections, lots, handles, témoins, compteurs et portée, ainsi que l'audit statique et `nm` de l'archive isolée. Le garde dépouille commentaires et littéraux, impose un unique site de rejeu 10.7, de reconstruction et de sonde, puis exige le contrôle du stamp dans la boucle.

Les douze CTests GCC Release de la chaîne sparse passent en 18,36 secondes, dont les deux tests 10.9 rejoués seuls en 3,87 secondes. Le build et les tests dédiés Clang 18 passent en 4,03 secondes. L'installation, l'export CMake et un consumer externe passent également. Ces validations sont courtes et n'utilisent ni benchmark long ni GCP.

## Limites ouvertes

10.9 ne prouve pas que les candidats 10.7 sont complets hors du sous-univers direct. Il ne décide pas comment plusieurs composantes positives incidentes doivent être intégrées atomiquement au quotient, ne résout pas les facettes latentes, ne publie pas les attaches hiérarchiques et ne prouve pas M.1. Ces obligations restent nécessaires avant toute fermeture de Phase 10 ou promotion du statut public.
