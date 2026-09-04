# Exécution CPU réellement mono-thread

4 septembre 2026. Cadre v7 : `phase=exploration_v7_hors_registre`,
`backend=cpu_reference`, `profile=quantized_u16_input_only`,
`mode=audit_independant_math_and_architecture`, `public_status=not_claimed`.

## Contrat d'appel

Le protocole mono fixe les trois options suivantes :

```cpp
RunOptions options;
options.threads = 1;
options.fold_inflight = 1;
options.fold_join_before_next_k = true;
```

Dans le CLI, elles s'écrivent
`--threads=1 --fold-inflight=1 --fold-join=1`. Une tour 1..10 utilise
`--smax=11` ; une tour 1..5 utilise `--smax=6`. Le choix de la sémantique
d'incidence reste indépendant de l'ordonnancement et doit être déclaré.
Voir le [contrat courant de performance](CONTRAT_PERFORMANCE.md).

Lorsque `threads == 1` et `fold_join_before_next_k == true`, B(K) est appelé
directement : le pipeline ne crée pas un thread par ordre pour ce fold.
Les callbacks `on_forest` et `on_fold_phase` s'exécutent alors sur le thread
appelant, avant le retour du pipeline. Un consommateur ne doit pas supposer
qu'ils arrivent sur un worker. Leur ordre, leurs arguments et leur autorité
provisoire sont conservés : aucun préfixe reçu ne devient un résultat final
si un ordre ultérieur refuse ou lève une exception.

`threads=1` sans jointure conserve le fold asynchrone ; `fold_inflight=1`
seul ne sérialise pas A(K+1) avec B(K). Avec `threads>1`, la jointure conserve
également le thread de B. Les valeurs par défaut ne changent pas. Toute
mesure doit déclarer ces options ; le temps de la route mono n'est pas
interchangeable avec celui de la route à recouvrement.

## Invariant et qualification bornée

Le corps de B, ses captures et sa possession du Stage restent identiques.
Avec jointure avant l'ordre suivant, le slot précédent est terminé puis
retiré avant B(K) ; `next_publish == K` sur le chemin de succès. L'attente
de publication ne dépend donc pas d'un prédécesseur que seul le thread
appelant pourrait terminer. Le slot inline possède un `std::thread` non
joignable ; les chemins existants de reap, drain, annulation et propagation
d'exception restent employés.

La porte `mhgp7_mono_inline` interpose le vrai symbole `pthread_create`
avec `dlsym(RTLD_NEXT)`. Elle exige un contrôle positif isolé et 30 créations
sur les quatre tours asynchrones, contre zéro sur les quatre tours mono :
1..5 et 1..10 dans les deux sémantiques. La fixture régulière comporte onze
points et des événements au dernier ordre ; les digests, cartes et callbacks
des tours sont comparés au chemin asynchrone inchangé. Huit pannes de
callback vérifient les exceptions et l'invalidation transactionnelle.

Les trois portes `mhgp7_mono_inline_late_a`, `mhgp7_mono_inline_late_b` et
`mhgp7_mono_inline_early_alloc` exercent chacune deux sémantiques dans un
processus distinct : échec A(K2), exception B(K3), puis allocation refusée
à B(K1). Les codes de sortie et les planchers stdout sont enregistrés dans
CMake. Ce sont des tests fonctionnels, pas des mesures de performance.

L'[archive de l'overlay](../receipts/mono_inline_overlay_20260904/README.md)
conserve le défaut original, les comparaisons exactes des sorties et les
reçus sanitizer antérieurs à l'intégration. Ces reçus ne sont ni une
qualification Release du produit intégré, ni une promotion de son statut.
Les campagnes mono à 8k, 16k, 32k et 50k restent des preuves distinctes.

