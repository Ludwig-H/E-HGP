# Pilote apparié : fermer la cohorte avant le jugement

8 octobre 2026. Source publiée `0e62a7232`, pilote SHA `fe22a681…`, identique à la capture de travail
observée au HEAD67b6. **Quatre fausses admissions reproduites avec les journaux de la sonde Python
simulée officielle** ; aucune mesure réelle mise en cause. Le rejeu extrait cinq sources épinglées
par Git. Aucun moteur, compilation, donnée réelle ou appel cloud.

| Altération isolée | Juge livré | Proposition |
| --- | --- | --- |
| Cohorte configurée vide, journaux intacts | adopte même le bras séquentiel simulé 10 % plus lent | refuse |
| Cohorte réduite à ng00, identité/campagne toujours à trois trames | juge en ignorant deux trames décisives | refuse |
| Onzième tour ajouté avec copies des journaux et hashes exacts, configuration à dix | juge les onze tours | refuse |
| Options A/A différentes de celles de la référence, même schéma | conserve le contrôle A/A favorable | refuse |

Le premier témoin vient de `all(...)` sur une collection vide. Une relecture correcte de chaque
journal ne suffit donc pas : la population jugée doit elle-même être fermée. La CLI vérifie certains
points à l'ouverture, mais le juge public accepte un rapport relu ; il doit les vérifier à nouveau.
Deux témoins supplémentaires, sur les résumés synthétiques du juge seulement, montrent que ses minima
six passes/dix tours peuvent aussi être abaissés après production : le code livré juge, le correctif refuse.
Ces deux témoins n'utilisent pas de rejeu brut et sont distingués dans `results.json`.

`proposition.patch` ajoute la vérification de configuration avant les statistiques et la relecture :
bras conformes à la liste fermée, référence/A/A cohérents, types/régime/minima, cohorte non vide et unique,
égalité des ensembles configuration/identité/campagne/métadonnées, nombre de tours **exactement** annoncé.
Le mode essai conserve ses minima relâchés ; les auto-tests synthétiques restent sans métadonnées de trame.
Le patch ne change ni bootstrap, seuil, temps chaud, ordre des bras, ni règle d'adoption.

La campagne positive simulée (trois trames, quatre bras, dix tours, six passes) demeure identique :
cache adopté, séquentiel rejeté, A/A contrôle. Les huit auto-tests officiels passent avant/après.
Normal et −O donnent le même reçu. Application isolée et postimage hachée vérifiées.
Les sources fictives d'horloges de ce pin sont celles de la porte publiée ; leur ajustement pour les
nouvelles gardes LF est distinct. Aucun résultat réel n'est extrapolé depuis ces fixtures.

```sh
python3 -B check.py --repo DEPOT
python3 -B -O check.py --repo DEPOT
```

Cette proposition complète la [relecture de l'identité et des résumés](../apparie_identite/README.md).
Elle ne remplace pas la fermeture externe de la provenance, l'inventaire complet des fichiers produits,
ni une qualification générale de toutes les entrées JSON malformées. `CST-0018` reste ouvert.
