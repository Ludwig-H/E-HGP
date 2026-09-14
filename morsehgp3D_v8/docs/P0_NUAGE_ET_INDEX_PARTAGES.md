# Un nuage, un index global, plusieurs rectangles

14 septembre 2026. Sixième tranche P0, `cpu_reference`,
`quantized_u16_input_only`, `implementation_v8_p0`, `not_claimed`.
Cette note décrit le raccord implémenté, pas une WSPD ni la tour FULL.

## Ce qui change concrètement

Auparavant, le propriétaire d'un rectangle copiait et vérifiait tous les
points ; l'index du census était attaché à ce rectangle. Répéter cette
préparation pour R rectangles aurait payé R fois le nuage entier.
Le nouveau propriétaire `PreparedCloud` copie et certifie les coordonnées
une seule fois. Les rectangles et l'index de tous les témoins Z partagent
ce stockage immuable. Les plans de crédits restent propres au rectangle.

```text
PreparedCloud (coordonnées privées, IDs originaux, boîtes de plages)
├── Q2CensusIndex (tous les sites Z, ordre et échappements immuables)
├── PreparedRectangle A1×B1, seuil K1 → crédits → plan axial → census
└── PreparedRectangle A2×B2, seuil K2 → crédits → plan axial → census
```

L'appelant ne doit plus construire un `RectangleInput` avec n coordonnées
par rectangle. Il utilise `prepare_cloud`, puis
`prepare_rectangle(cloud, RectangleSpec, Kmax, s)` et un seul
`make_q2_cloud_index(cloud)`. L'ancienne factory reste un adaptateur de
compatibilité qui paie explicitement un nuage neuf à chaque appel.

Le partage n'est pas seulement une promesse de l'interface : la factory
du rectangle partagé ne parcourt aucun tableau de n sites. Elle interroge
deux boîtes, vérifie plages/séparation/seuil et certifie les propositions
du cœur. Elle ne copie pas les coordonnées. Le tri, la validation et la
copie des propositions de cœur restent payés selon leur nombre.

## Les boîtes ne doivent pas devenir le nouveau scan caché

Un arbre de segments sur **l'ordre original** conserve les boîtes de
plages. Il utilise 2n emplacements, dont un inutilisé : n feuilles et
n−1 unions. Une plage non vide est décomposée en O(log n) nœuds ; l'union
de leurs boîtes est exactement celle de ses points. L'opération d'union
étant associative et commutative, le rangement itératif reste valable
quand n n'est pas une puissance de deux.

Les compteurs distinguent les nœuds consommés (`factor_box_visits`) des
niveaux parcourus (`factor_box_steps`), y compris un niveau sans nœud
consommé. Aucun compteur partagé n'est modifié pendant une requête.
Le nuage paie sa copie O(n), l'unicité O(n log n) et cet arbre O(n).
L'index Z conserve sa construction et ses compteurs antérieurs, une seule
fois par nuage ; sa profondeur est bornée par la représentation u16.

Ce rangement est un **raccord provisoire**, pas le futur index WSPD.
Les facteurs WSPD seront contigus dans une permutation spatiale, pas
nécessairement dans l'ordre original. Il faudra déclarer cette permutation
et ses IDs, réutiliser les boîtes des nœuds certifiés, puis remplacer les
requêtes de plages devenues inutiles. Recopier les coordonnées par facteur
pour simuler cette contiguïté annulerait le gain recherché.

## Trois identités différentes à ne pas confondre

1. L'index Z et le plan de census doivent partager **le même nuage**.
   Un autre propriétaire ayant les mêmes coordonnées est refusé.
2. Les restrictions de crédits et le plan axial doivent partager
   **le même rectangle certifié**, seuil compris. Un nuage commun ne
   rend pas les crédits transférables à un autre facteur opposé.
3. Les plages B et les continuations Z dépendent de **leur ordre précis**.
   Le partage du nuage ne permet pas de transférer une plage vers une autre
   permutation, ni un curseur vers un autre index construit sur ce nuage.

Le seuil du census vient maintenant du rectangle du plan, jamais de
l'index global. Celui-ci peut donc servir des requêtes Kmax différents.
Les contre-fixtures de l'auditeur sont portées : crédits du mauvais
rectangle refusés, census partagé entre plusieurs seuils confronté à un
juge exhaustif, copie privée conservée malgré mutation du tampon source,
survie du nuage par les objets qui le retiennent. Les propriétaires ne
sont ni copiables ni déplaçables ; aucun certificat n'est modifiable.

Les appels restent synchrones : plan, index et callback sont empruntés
pendant tout l'appel. Leurs données immuables permettent le partage de
lecture ; les buffers de recherche et de collecte restent privés à chaque
appel. Cela ne constitue pas encore une exécution multi-CPU ou GPU testée.
Une exception du callback n'annule pas les émissions déjà effectuées.

## Ce qui reste coûteux — et peut encore être quadratique

Le retrait de la préparation globale répétée ne borne pas tout le travail.
Pour les crédits Pool, le filtre axial et son index local, on paie encore
des parcours des facteurs par rectangle. Sur une partition A_i×B, le même
B peut ainsi être traité R fois : un terme Ω(R|B|) subsiste. Si R croît
comme n et B comme n, ce terme est quadratique. Il ne faut ni le cacher
dans les candidats ni appeler toute la chaîne sous-quadratique parce que
la copie du nuage a disparu. La comparaison mesure les deux régimes :
R fixe, puis R doublé avec n.

Le découpage lui-même peut affaiblir les minorants : un point de A situé
hors de A_i ne participe plus au pool local de A_i. La sûreté des rejets
est conservée, mais le nombre de paires indécises peut augmenter fortement.
Le prochain front devra examiner la transmission de certificats parents
encore valables, sans contourner les identités ni additionner des témoins
dont la disjonction n'est pas prouvée. Recalculer de petits crédits enfants
sans payer la dégradation du résidu n'est pas une solution à P0.

L'arbre de requêtes B du census partagé reste propre à l'appel et à
l'ordre B du plan. L'auditeur a montré qu'un préfixe compact dans un ordre
peut être très fragmenté dans un autre. Partager un arbre spatial ne
transfère donc pas une borne de couverture O(log |B|) : voir
[son analyse des ordres](../audits/P0_SOUS_RECTANGLES_ET_GROUPES.md#94-partager-les-arbres-b-sans-transférer-leur-borne-de-couverture).
La sonde de cette tranche emploie le census individuel pour isoler le
partage global ; elle n'évalue pas ce futur raccord B.

La [mesure indépendante du front pur](../audits/REGIME_WSPD_20260914.md)
montre aussi des millions de petits rectangles. C'est un résultat
exploratoire v4, pas une mesure v8. Il interdit néanmoins de considérer
l'allocation d'un objet lourd et plusieurs tris comme un coût anodin par
rectangle. Le front fusionné avec rejets précoces, les petits facteurs
sans préparation et la réutilisation des index des facteurs sont les
prochains objets à traiter. Examiner les témoins sur les produits ancêtres
avant séparation complète, avec preuve de couverture résiduelle.

**Attention à s :** le prédicat de la factory reste
`gap(boîtes) ≥ s × max(diagonales)`. Ce n'est pas la convention du front
v4/v7. Les séries s8/10/12 présentes vérifient cette précondition sur des
rectangles fixes, elles ne comparent pas trois WSPD. Le pilote futur doit
fixer sa convention explicitement avant toute comparaison.

## Mesure, mémoire et suite

`mhgp8_cloud_reuse_probe` partitionne un seul A×B en R bandes A_i×B,
sans chevauchement ni perte. Le bras `fresh` prépare nuage/index par bande ;
`shared` le fait une fois. Les deux emploient les mêmes sources et le même
chemin local Pool ∩ Additive, puis census individuel et collecte réelle.
Sorties et travail local doivent coïncider ; le travail global du premier
bras doit être R fois celui du second, hors maxima de profondeur.
Un seul contexte de rectangle est conservé à la fois dans chaque bras.

Le temps englobant paie copies, index, filtres, census, collecte, checksum
du flux et destructions. La génération et destruction du tampon d'entrée
sont ajoutées à chaque total ; la comparaison des bras et l'impression JSON
restent séparées. Cela ne mesure ni la CLI complète ni la tour FULL.
Les capacités conservées du nuage et de Z sont rapportées séparément :
ce ne sont **ni un pic RSS, ni toute la mémoire locale, ni une mesure VRAM**.
Le tableau temporaire d'unicité est libéré avant la construction des boîtes.

La suite prioritaire est un pilote de front réel, avec permutation globale,
rejet précoce, compteurs de rectangles et somme des tailles de facteurs.
Raccorder ensuite le résidu Pool sans forcer les colonnes axiales, les
continuations bornées de l'auditeur, puis workers CPU et GPU. q3/q4,
déduplication des boules et parents FULL ne doivent pas être repoussés
derrière une série indéfinie de micro-optimisations q2. Les contrats de
tour 50k et multi-millions sur G4 restent ouverts. GCP non utilisé.

Qualification de cette tranche : [66 essais et 37 CTests par build](../receipts/cloud_reuse_20260914/README.md).
Les deux builds `v8_cloud_20260914` et `v8_cloud_sanitize_20260914` sont
épinglés ; poursuivre dans de nouveaux répertoires. Les essais R croissant
confirment le coût local quadratique ci-dessus, malgré la préparation
globale partagée. Aucun contrat de tour n'est promu.
