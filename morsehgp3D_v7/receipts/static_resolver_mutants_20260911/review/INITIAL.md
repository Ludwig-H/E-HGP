# Relecture de la variante privée initiale

11 septembre 2026. Source relue sans modification :
`build/v7_static_resolver_20260911/source/morsehgp3D_v7/src/forest/full_ball_tower.hpp`,
SHA-256 `c9be538c83ea3eb373fabbf286e375cc5dcb7e1f37401fed2a67b63dc55add22`.
Comparaison ligne à ligne avec le header actif `910f45ba…`. Aucune compilation,
exécution de moteur, modification de source active/privée, Git ou GCP par cette
contrelecture. Les verdicts ci-dessous portent sur ces octets précis.

## Verdict

**Favorable pour qualification privée. Aucun défaut bloquant de géométrie,
de raccord ou de concurrence trouvé dans cette version.** Cette lecture de code
ne remplace ni les tests indépendants ni les mutants. Trois écarts de mesure
méritent une correction ou une définition explicite avant publication.

## Accès concurrents

`static_terminal()` et `intruder_work()` ne lisent effectivement ni `anchors`,
ni `current`, ni `compressed`, ni `static_cursor`. Leurs dépendances partagées
sont l'index, le census, `by_key` et `current_k`, tous constants pendant
`parallel_ranges()`. Les valeurs locales de MEB et clés sont propres à l'appel.
Le visiteur et les tris ont terminé avant lancement des workers.

Les ranges de groupes distribuées par le pool sont disjointes. Chaque groupe
parcourt sa plage du vecteur `requests`, et chaque `ordinal` a été attribué une
seule fois avant tri. Les écritures du scatter ciblent donc des éléments
distincts de `static_targets`, dont taille et capacité sont fixes pendant le
pool. `std::vector<BallId>` n'est pas le cas spécial `vector<bool>` : aucune
écriture de mot partagé induite par bit-packing. Les éléments workers possèdent
leurs stats, pile et compteur de seed ; `merge_static_work()` tourne seulement
après jointure. Aucune course de données trouvée.

La capture `[&]` n'est pas une preuve d'immutabilité à elle seule, mais les
accès actuels ne modifient pas les structures partagées hors scatter disjoint.
Le qualificatif `const` de `static_terminal()` ne rend pas impossible tout
accès futur aux ancres du Builder : la séparation est actuellement vraie par
inspection, pas encore imposée par un type incapable de voir le calendrier.

## Géométrie, semis et consommation

- L'admission statique utilise `p+arity-1 <= current_k <= p+u` après validation
  du census ; `arity` est donc bien le q_min certifié.
- Une seed est créée uniquement pour un bloc programmé avec `K=p+u` : la
  facette est exactement le fermé I union U et sa MEB est la boule validée.
  Deux seeds avec la même facette mais des BallIds différents seraient deux
  MEB d'une même facette ; le rejet de doublon est donc correct.
- Le tri `(key, ordinal)` fait du premier élément de chaque groupe son
  consommateur chronologiquement le plus ancien, puisque les requests ont été
  émises dans l'ordre des programmes. Le contrôle explicite
  `before <= consumer.level` protège cette propriété au scatter.
- Le terminal strictement antérieur au premier consommateur est donc
  strictement antérieur à tous les autres. La comparaison est encore refaite
  par `resolve()` pour chaque occurrence avant accès à l'ancre ; c'est une
  défense correcte, y compris pour la route seed qui évite toute MEB.
- La descente conserve les niveaux non croissants et la diminution de coquille
  à rayon égal ; son test de terminal précède la requête d'intrus.
- Les deux passes utilisent exactement `visit_block()`, dans le même ordre des
  programmes et des composantes strictes. Aucune décision du visiteur ne dépend
  du calendrier ; `ShellTable::rank()` est `const`. Le curseur de consommation
  reproduit donc l'ordre d'émission des demandes, sans consulter l'ordre trié.
- Les requests temporaires sont détruites au retour de préparation, alors que
  les targets sont conservés jusqu'à la fin du K puis libérés. Chaque lot
  normalise tous ses parents avant `close_lot()` ; le calendrier lui-même n'a
  pas changé. La vérification du curseur final exclut un suffixe non consommé.
- K1 reste nominal. Une préparation K sans représentants possède seulement
  le sentinelle `groups[0]=0`, ne lance aucun job et conserve tous les blocs
  sans arête dans le calendrier. Le terminal K=n n'est donc pas supprimé.

Le scatter ordinal n'est pas auto-certifiant géométriquement : un mauvais
permutateur peut écrire un autre terminal valide, plus ancien, dont l'ancre
existe, et passer ces contrôles structurels. C'est normal pour une API interne,
mais rend indispensable le mutant de permutation et le jugement indépendant.

## Échec et ressources

Le pool existant annule l'admission si le lancement des threads échoue, joint
tous ceux déjà lancés, puis relance `std::system_error`. Le nouveau catch
extérieur produit un échec ressource. Pour une exception dans un job, le pool
joint également tous les workers avant le catch local et la fusion des stats.
Les sorties restent locales au Builder jusqu'au succès complet de la tour ;
aucun préfixe d'orders n'est publié.

Le cache direct nominal n'est pas configuré pour la route statique, et
`seed_closed_anchor()` retourne immédiatement. Il n'existe pas de cache O(n)
répliqué par worker. Les tableaux du plan peuvent encore lever `bad_alloc` ou
`length_error`, traités par le wrapper. Le dépassement des compteurs reste un
échec ressource contrôlé via `add()`.

Petite limite de diagnostic : le catch `std::system_error` enveloppe toute
construction et nomme toujours `thread_launch_failed`. Aujourd'hui les appels
susceptibles sont principalement le pool ; si d'autres primitives système sont
ajoutées, ce libellé devra rester borné à l'appel de lancement ou devenir plus
générique. Cela ne crée pas de faux succès dans la version relue.

## Comptabilité : corrections proposées à ROOT

1. **Workers créés versus utilisés.** `parallel_ranges(n,1,...)` retourne 1
   pour un travail exécuté sur le fil appelant, sans créer de thread. Avec zéro
   groupe il retourne 0. `static_workers_created` décrit donc le nombre cumulé
   de lanes utilisées, pas littéralement les threads créés. Renommer en
   `static_worker_lanes_used`, ou publier séparément les lanes retournées et
   `launched_threads = lanes > 1 ? lanes : 0`. Ne pas inférer une activité CPU
   concurrente d'une valeur positive égale à 1.
2. **Mémoire du plan.** `static_peak_request_bytes` conserve le maximum par K
   de la capacité finale du vecteur requests, et `static_peak_target_bytes`
   celui du scatter. Ces champs omettent `seeds`, `groups`, les workers et leurs
   piles. Ils omettent aussi la coexistence de l'ancien et du nouveau buffer
   pendant une croissance de capacité. Les renommer/qualifier comme capacités
   persistantes, ajouter les autres capacités et un maximum de leur somme au
   même instant. Ne pas sommer des maxima de K différents comme un pic observé.
   Une mesure d'allocateur ou RSS indépendante reste nécessaire pour le pic réel.
3. **Seed hits en échec.** Le catch local fusionne `w.work`, mais pas
   `w.seeded`. Si un autre groupe échoue après des hits déjà payés, le compteur
   publié de seed hits est sous-compté. Ajouter dans le catch la même réduction
   de `w.seeded` que dans la voie succès, ou factoriser les deux réductions dans
   une fonction commune exécutée exactement une fois après jointure. Ne pas
   rejouer une seconde fusion lorsque cette réduction elle-même lève.

Les trois points ci-dessus ne changent pas la forêt nominale. Ils importent
pour qualifier les coûts et conserver les reçus d'échec fidèles.

## Coût intermédiaire et suites de test

Le schéma matérialise O(R+A) données avec un tri O(R log R) des requests et un
tri des seeds. La génération de représentants et le quotient extra-shell sont
payés deux fois, et `resolve()` retrie encore les sites qui ne servent plus à
la géométrie dans cette route. Ce sont des coûts réels à mesurer, pas des bugs
de correction. Aucun gain mono-thread ni sous-quadratique universel n'en découle.

La capacité du vecteur requests peut dépasser nettement sa taille à cause de
la croissance géométrique. Un comptage préalable pourrait réserver exactement,
mais ajouterait un troisième parcours des représentants si l'on conserve ce
visiteur tel quel. Ne pas transformer la correction de mesure en refonte
coûteuse avant d'avoir le profil du prototype.

Avant toute intégration : gate nominal/statique 1/statique 4, fixture Kmax6
de présence globale inadmissible, singleton/K=n, corpus et mutations de
couvertures/verticale existants. Mutants nouveaux à rendre non vacus :

- écrire `static_targets[r]` au lieu de `static_targets[request.ordinal]` ;
- ne scatter que la première occurrence d'un groupe répété ;
- arrêter dès qu'une clé globale est trouvée sans contrôler K ;
- court-circuiter le contrôle strict de seed, avec corruption ciblée de sa date ;
- lire le BallId comme token, ou ne pas normaliser l'ancre pré-lot ;
- ignorer le sentinelle zéro-représentant ou supprimer ces blocs ;
- provoquer une erreur au lancement puis au milieu d'un job et contrôler
  `orders.empty()`, aucun worker survivant et stats du travail déjà payé.

Les mutations géométriques ne sont pas toutes garanties tuées par le seul
digest nominal de la porte actuelle : conserver un contrôle nommé de la clé
4225/K4 et des hits de seed, ainsi que des demandes réellement répétées dont
le tri change les positions d'écriture.
