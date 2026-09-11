# Prochain raccord privé : terminal géométrique complet

Plan seulement, sans modification des sources MEB/clé gelées et sans nouvelle session G4. Socle lu : `c03f6be8488453486b112811071827a96303ec86`, `source/morsehgp3D_v7/src/forest/full_ball_tower.hpp`, `anchor_meb.hpp`, `lanes/{level,q2,q3,q4}.hpp`, et prototype pair `build/v7_gpu_intruder_20260911/{intruder.cuh,PROOF.md}`. La prochaine qualification utile est celle du raccord complet, pas une session facturable distincte pour chaque brique.

## Autorité et résultat

Reproduire `static_terminal` (`full_ball_tower.hpp:525`) : il dépend de l'index immuable, du catalogue, de K et du niveau strict du premier consommateur ; jamais des ancres temporelles ni du DSU. La sortie est un **BallId du catalogue**, lié à son propriétaire/snapshot, avec statut, ordinal et compteurs. Le raccord hôte attend ensuite que cette ancre soit effectivement fermée et appelle `root(anchors[target], prior_count)` au point chronologique actuel (`:679–687`). Une clé, une population ou des points communs ne remplacent pas cette identité de composante.

K=1 est distinct : `resolve` retourne directement la feuille associée au PointId (`:675–677`). Le catalogue validé courant impose une arité 2 à 4, donc le terminal géométrique GPU initial cible K=2..10 ; la primitive q1 qualifiée n'invente pas une entrée singleton dans le catalogue. Le cas terminal K=n, les plateaux extra-shell et le raccord vertical gardent les règles du producteur FULL.

## Niveaux bruts, sans canonisation inutile

Calculer le niveau à partir du support accepté, jamais à partir d'une formule de rayon appliquée à la clé primitive : le dénominateur $4A^{2}$ n'est pas garanti représentable par le i128 positif de `ExactLevel`. Aucune nouvelle arithmétique générale n'est nécessaire :

- q1 : numérateur nul, dénominateur 1 (utile à la gate locale, pas une cible de catalogue K=1) ;
- q2 : distance carrée / 4, sans appel hôte à `q2_exact_level` qui réduit ;
- q3 : `q3_level_raw` (`q3.hpp:75`) puis promotion POD explicite du numérateur i128 positif vers trois mots u64 ;
- q4 : `q4_level_raw` (`q4.hpp:149`), déjà HD.

`compare_exact_level` (`level.hpp:55`) et `same_exact_level` sont HD et comparent par produits croisés U320. Ils suffisent à tous les tests de descente et de catalogue. Ne pas utiliser `ExactLevel::operator==`, ordre de représentation, hash ou rayon flottant. `promote_level` étant encore hôte, une petite promotion HD locale sans PGCD suffit ; les entrées négatives ou dénominateurs non positifs sont refusées. Si le niveau traverse le transport pour les diagnostics, employer cinq mots u64 (numérateur 3, dénominateur 2), pas un i128 natif. Les attentes CPU q2/q3 réduites seront comparées **sémantiquement**, pas mot à mot, aux niveaux bruts.

## Boucle initiale : un thread par facette

Conserver l'ordre exact payé par le CPU et par les deux helpers ; aucune variante warp/support à ce premier raccord.

1. Charger la sélection courante K indices géométriques strictement croissants ; calculer la MEB, sa clé primitive, son niveau brut, le premier support et la coquille complète.
2. Exiger `local.level < before` **à chaque itération**, avant toute résolution. Faire le lookup exact par clé dans le catalogue trié ; l'ordre des coefficients est signé, donc un memcmp des dix mots low/high est incorrect. Un comparateur HD explicite doit reproduire `BallKey::<`.
3. Si la clé existe, exiger l'égalité sémantique des niveaux. Accepter ce BallId seulement si `n_interior + arity - 1 <= K <= n_interior + n_shell`. Le lookup précède obligatoirement l'intrus ; les compteurs doivent le montrer (`static_terminal:531–543`).
4. Si la clé est absente **ou présente mais hors de la fenêtre K**, trouver le premier rang Morton strictement intérieur hors sélection, en conservant exactement droite-puis-gauche dans la pile donc gauche d'abord en visite, comme `intruder_work:494–520`. Un hit hors fenêtre ne provoque donc pas un refus prématuré. L'absence d'intrus sans terminal est un refus d'invariant, pas une résolution partielle.
5. Remplacer **le premier slot du support** par cet intrus, trier les K indices, recalculer la MEB. Exiger niveau non croissant. À niveau égal, exiger **même clé primitive et cardinalité de coquille diminuée exactement de 1** ; sinon compter une vraie descente. Puis recommencer.

Ne pas ajouter de quota empirique de chaîne : la descente lexicographique niveau/coquille et l'espace fini de sélections portent le raisonnement de terminaison. Les compteurs longs sont additionnés avec contrôle de débordement et les fautes rendent le lot non publiable. Le plafond de pile 49 de l'intrus dérive des 48 bits Morton de l'index Patricia validé ; ce n'est ni un plafond de points ni une troncature du parcours.

## Résidence et propriétaire communs

Le propriétaire futur doit posséder ensemble les tableaux d'index, le catalogue, son tri par clé et ses données `arity/n_interior/n_shell/level`, puis produire un seul token immuable à génération non réutilisable dans le processus, pas recopier les tokens de deux propriétaires séparés. Refuser les mélanges index/catalogue anciens ou étrangers et certifier la libération persistante de toutes les allocations. `BallId` garde exactement l'ordinal du catalogue source ; la permutation de recherche est une table séparée.

La vue intrus actuelle emploie des positions u16 interlacées, des liens/ranges i32 et des boîtes u16. Le MEB utilise des P3 i64. Pour éviter deux copies complètes des positions GPU, le premier raccord peut charger les K positions u16 en P3 locaux puis appeler le helper avec une requête locale `[0..K-1]`, en préservant l'ordre original et les slots. Cette conversion ne change pas les supports ni les puissances ; elle devra être qualifiée. Les indices géométriques d'origine restent ceux de l'exclusion, du remplacement et du résultat. Les bornes du helper intrus ($0<A<2^{68}$, $|B_i|<2^{87}$, $|C|<2^{105}$) doivent rester des assertions d'admission prouvées pour les clés produites, jamais une normalisation ou un clipping silencieux.

Le lot transporte seulement les facettes/ordinaux/niveaux `before` et rend BallId/statut/compteurs. MEB, clé, niveau, lookup et intrus restent sur device pendant toute la chaîne. Éviter absolument des allers-retours hôte pour chacune des étapes. Le premier raccord peut garder le tri/dédoublonnage des demandes et les seeds sur CPU, selon `prepare_static_targets` (`:576–665`), mais doit compter explicitement ces coûts et leurs transferts. Une seed renvoie un BallId, jamais un token DSU. Chaque consommateur de la classe est au moins au niveau du premier consommateur ; la chronologie est vérifiée avant publication.

## Porte de qualification avant G4

Créer une nouvelle source privée, comparer au CPU `static_terminal` puis aux sorties FULL physiques, sans modifier la couture MEB/clé ni les preuves fermées. Fixtures minimales : q2/q3/q4, extra-shell, lookup hit avant intrus, clé présente hors fenêtre K, descente stricte, descente à rayon égal, absence d'intrus, doublons de demandes, seeds, K=1 séparé et K=n. Inclure des niveaux bruts équivalents mais non identiques en représentation, et des clés de rayon égal distinctes.

Comparer BallId, traces bornées de sélection et compteurs exacts (MEB appels/supports/puissances/matérialisations ; lookup/hits ; intrus nœuds/ranges/puissances ; descentes et longueur maximale). Mutations causales : `<= before` au lieu de `<`, mauvaise fenêtre K, lookup après intrus, mauvais sens de parcours, intrus frontière au lieu d'intérieur, mauvais slot remplacé, oubli du tri, même rayon sans même clé, coquille non décrémentée, mauvais BallId/snapshot, omission de dernière sortie. Les traces détaillées sont seulement des diagnostics de gate, pas un payload produit massif.

Exiger O2 et SAN hôte sur mêmes snapshots, puis compilation/lien NVCC strict sm120 sans nouveau vendor. Après revue, une seule session G4 SPOT gardée pour le terminal complet peut confronter le device au juge autonome et fermer sa cible précise. Aucun contrat temporel de la tour FULL ne découle d'un chronométrage de ce composant ; le contrat reste toute la tour K=1..10 puis K=1..5 sur 50k, et le régime multi-millions reste à qualifier séparément.
