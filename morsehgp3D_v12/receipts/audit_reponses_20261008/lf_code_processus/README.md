# Code processus : fermer le résidu du lecteur FULL

8 octobre 2026, source `85db49890`. Le résidu signalé dans la
[contrelecture de livraison](../lecteurs_livraison_85db/README.md) est reproduit :
`False`, `0.0` et `-0.0` admettent un flux nominal comme code zéro ; `2.0` admet
un refus de ressources comme un résultat. Ces valeurs ne sont pas des codes
processus entiers. Aucun journal réel erroné ni temps faux n'est démontré.

Le patch ajoute une garde de type après la sentinelle `expire`, avant toute
lecture des lignes. Un code non entier devient `illisible`, donc contrôle
manquant, plutôt qu'un résultat adopté/refusé. Les vrais entiers zéro/deux,
le signal négatif et la sentinelle gardent exactement leur comportement.
Le juge apparié contrôle déjà ses codes strictement ; le changement vise le
lecteur partagé et les autres appelants.

`check.py` applique le patch à une copie Git, confronte les quatre contre-flux
à la version livrée et à la proposition, puis retire cette seule garde : les
quatre mauvaises admissions reviennent avec le positif conservé. La porte
Python officielle entière reste verte avant/après, sans modification de sa
fixture. Normal et `-O` rendent exactement `results.json`.

```sh
python3 -B check.py --repo DEPOT
python3 -B -O check.py --repo DEPOT
```

Proposition rattachée à CST-0018, aucun produit modifié ni moteur exécuté.
Le raccord dans le lecteur et l'ajout de ces cas à sa porte permanente restent
à faire ; aucune clôture globale du constat n'en découle.
