# Balayage des préfixes du locator positif — Phase 10.10

## Porte d'entrée et statut

Phase 10, backend `reference_cpu`, profil `hgp_reduced`, mode `certified`, sémantique `partial_refinement`. La porte locale est satisfaite par le locator positif sparse 10.5a, son historique durable et la localisation 10.9, qui a isolé l'absence d'autorité temporelle. Le jalon 10.10 certifie uniquement l'horloge de commits propre à un locator vivant et gelé. Il ne prétend pas aligner cette horloge avec `source_batch_index` 10.7 et conserve `source_batch_alignment_claimed=false`.

Le but est de répondre à plusieurs requêtes historiques sans copier un locator complet par lot et sans rejouer le DSU pour chaque clé. Le chemin reste relatif à l'autorité externe affirmée par le locator; tout hit conserve le statut `conditional_on_caller_external_replay` et le `public_status` reste `not_claimed`.

## Préfixes durables

Soient les lots committés $b=0,\ldots,T-1$. Le record durable du lot $b$ conserve le nombre $i_b$ de nouvelles liaisons uniques et le nombre $u_b$ d'unions. Pour $0\leq p\leq T$, l'état après les $p$ premiers lots, donc l'état strictement pré-lot $p$ lorsque $p<T$, possède exactement les préfixes

$$I_p=\sum_{b=0}^{p-1}i_b,\qquad U_p=\sum_{b=0}^{p-1}u_b.$$

`apply_batch` attribue aux nouvelles liaisons les indices consécutifs à partir du nombre déjà committé et ajoute les unions dans le même ordre que les lots. Les clés actives au préfixe $p$ sont donc exactement celles dont `committed_binding_index<I_p`. Le DSU historique est exactement le rejeu des $U_p$ premiers records d'union depuis les handles denses identitaires.

Cette reconstruction ne requiert ni payload des requêtes historiques, ni doublons compatibles : ces opérations n'ajoutent ni clé unique, ni union, et leurs compteurs ne modifient pas l'état positif recherché.

## Lemme de recherche dans la table finale

La table 10.5a a une capacité fixe, n'est jamais réhachée et ne supprime aucune clé. Chaque liaison est insérée au premier slot libre de sa suite de sondage. Le vérificateur structurel rejoue désormais cet ordre physique par `committed_binding_index`; une simple retrouvabilité dans la table finale ne suffit pas.

Pour sonder le préfixe $p$ dans la table finale, un slot non occupé ou portant un indice de liaison supérieur ou égal à $I_p$ est traité comme le premier slot logiquement vide. Si une clé active se trouvait après un tel slot dans sa suite de sondage, ce slot aurait été libre lors de son insertion et la clé aurait dû s'y arrêter, contradiction. La première clé complète active égale est donc le hit historique exact; atteindre un vide logique certifie seulement un miss latent relatif au domaine positif de ce préfixe.

Les empreintes restent des accélérateurs. Toute empreinte égale déclenche une comparaison de la clé complète, y compris sous masque nul et collisions forcées.

## Balayage monotone

Les requêtes d'entrée sont canoniques et ordonnées par `committed_batch_prefix_count` croissant. Une première passe lit les $B$ records nécessaires pour préflight des sommes et des budgets. Le builder maintient ensuite un unique curseur de lots, un unique préfixe de liaisons et un DSU scratch dense. Avant une requête au préfixe $p$, il avance une seule fois des préfixes déjà rejoués jusqu'à $p$, puis sonde la clé dans l'état logique correspondant. Chaque transition est donc appliquée une fois, mais les compteurs auditent exactement $2B$ lectures de records de lots.

Le résultat complet conserve une seule arène plate, avec une résolution par requête dans l'ordre source :

- `relative_positive`, contenant la racine historique du handle original et le témoin de la première liaison;
- `latent_unresolved`, sans handle ni témoin positif.

Un budget épuisé, un record durable incohérent, un stamp divergent ou une entrée non canonique ne publie aucun préfixe partiel. Un miss ne signifie ni facette absente, ni isolation, naissance ou singleton. Aucun `apply_batch`, union durable, racine persistante, forêt, quotient ou `GatewayAttach` n'est produit.

## Budgets et coût

Les plafonds globaux portent séparément sur les requêtes, leurs `PointId`, les handles du scratch, les records de lots, les records d'union, les sauts de parents pendant le rejeu des unions, les visites de slots, les sauts de parents pendant les requêtes et le stockage logique de sortie. Un budget local de sonde borne en plus chaque requête; le budget effectif est le minimum du cap local et du reliquat global.

Pour $Q$ requêtes, $H$ handles, $B$ lots avancés, $U$ unions rejouées, $V$ slots visités, $J$ sauts de parents du rejeu, $R$ sauts de parents des requêtes et $C$ comparaisons de clés complètes, le travail propre du sweep est

$$O(H+B+U+V+J+R+K(Q+C)),$$

avec $C\leq V$, un scratch $O(H)$ et une sortie $O(Q)$ pour $K\leq10$. Aucun terme ne dépend de $\binom{n}{K}$ et aucun snapshot par lot n'est matérialisé.

La vérification fraîche possède un budget distinct, obligatoire avant toute allocation. Pour $M$ slots, $L$ liaisons, $P$ `PointId`, $H_L$ handles du locator, $U_L$ records d'union, $T$ records de lots, $F$ visites cumulées des recherches de fingerprint, $A$ visites cumulées du rejeu chronologique et $J_L$ sauts de parents du DSU structurel, son travail est $O(M+L+P+H_L+U_L+T+F+A+J_L)$ et son scratch $O(M+L+P+H_L)$. Les populations, les octets du scratch et les trois coûts variables sont plafonnés; atteindre un cap avant la fin échoue fermé sans reconstruire le sweep. Chaque record de lot invalide est rejeté avant sa première boucle interne, de sorte qu'un historique hostile ne peut pas faire rescanner le même préfixe. Le vérificateur 10.10 ne lance ensuite sa propre reconstruction bornée que si cette recertification structurelle a abouti.

Le prédicat auto-contenu du résultat ne remplace pas cette vérification fraîche, mais il recompte les dispositions et refuse déjà tout handle positif hors du scratch dense déclaré ou tout témoin positif lié à une autre autorité.

À titre d'ordre de grandeur, dix millions de handles denses exigent environ 80 Mo pour le scratch `size_t` du DSU, sans compter le locator déjà durable. Cette architecture reste compatible avec une voie linéaire, mais aucun SLO 50 k sous la seconde ni aucune qualification 10 M+ n'est revendiqué à ce stade.

## Deux horloges distinctes

Le préfixe $p$ est un nombre de commits 10.5a. Un lot 10.7 possède au contraire un `source_batch_index`, un cardinal et un niveau exact. L'état durable actuel du locator ne conserve ni cet index source, ni ce niveau, ni un digest du lot 10.7. L'égalité numérique de deux indices ne constitue donc jamais une preuve d'alignement.

Une intégration ultérieure devra fournir un certificat séparé, lié au journal 10.7 fraîchement rejoué et à chaque stamp préfixe du locator. Jusqu'à cette couture, 10.10 rend l'état historique du locator interrogeable, mais ne transforme pas les localisations 10.9 en décisions de quotient pré-lot.

## Validation courte

La campagne doit couvrir les préfixes initial, vide, post-insertion, post-union et final sous collisions forcées; une clé encore future doit servir de vide logique et la même liaison doit suivre ses racines historiques successives sans réécriture. Elle couvre aussi les requêtes vides, la frontière de clé à dix points, chaque cap du sweep exact puis moins-un, les plafonds distincts de tailles et de visites du vérificateur, les témoins et entrées invalides, le stamp périmé, les mutations hostiles du résultat, les recomptages auto-contenus, la chronologie physique falsifiée et l'isolation statique et symbolique du target.

Cette campagne est validée sur l'hôte. Les quatorze CTests GCC Release de la chaîne sparse passent en 8,15 secondes; les quatre contrôles dédiés Clang 18 passent en 0,18 seconde et les deux tests fonctionnels GCC ASan/UBSan en 0,04 seconde. Installation, export CMake et consumer externe du symbole 10.10 passent également. Aucun benchmark long ni GCP n'a été utilisé.
