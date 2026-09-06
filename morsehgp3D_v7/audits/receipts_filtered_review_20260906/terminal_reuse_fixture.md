# Terminal répété après fusion de son ancienne racine

6 septembre 2026. Fixture rationnelle indépendante, sans exécution C++ ni mesure de performance. `public_status=not_claimed`. GCP non utilisé.

Le [script borné](terminal_reuse_fixture.py) fixe douze points u16 distincts, K=7 et la politique lazy C=0. Leurs PointId sont leurs positions 0..11. L’énumération rationnelle des 224 supports positifs de cardinal 2..4 vérifie partout que la coquille globale est exactement le support. Leurs ensembles fermés donnent le catalogue complet : 24 minima de cardinal 7 et 19 directes de cardinal 8. Les recherches initiales ayant trouvé ces coordonnées ne sont pas conservées comme un résultat scientifique.

Le modèle suit l’ordre des niveaux exacts, labels et essentiels ; ses intrus sont ordonnés par Morton48, avec x au bit 0, y au bit 1 et z au bit 2. Il conserve les lots atomiques et les ancres de directes fermées. Les numéros de nœuds ci-dessous appartiennent **au modèle** ; aucune correspondance avec un binaire C++ n’est revendiquée. La MEB est encore calculée à chaque étape pour établir le témoin : ce script n’est pas une implémentation du futur raccourci.

Le terminal `{2,3,4,5,6,8,9,10}` est atteint deux fois par une chaîne, depuis des facettes différentes :

| Visite | Facette demandée | Longueur de chaîne | Racine du modèle | Minima dans la composante |
| --- | --- | ---: | ---: | ---: |
| Première | `{1,2,4,5,6,8,9}` | 1 | 25 | 18 |
| Répétée | `{0,2,4,5,6,9,10}` | 2 | 34 | 24 |

Cette composante s’est donc agrandie entre les deux visites. Le juge exige exactement onze demandes de portail P, trois pas de chaîne C, neuf résolutions J=1, deux terminaisons de chaîne T et un label terminal distinct U : une réutilisation serait admissible par la politique proposée. J=1 n’amorce aucun mémo dans ce compte.

L’autorité des composantes est vérifiée séparément : les **792 facettes et 495 cofaces** possibles sont évaluées rationnellement, y compris les cellules non Gabriel. À chacune des **43 coupes strictes précédant un lot**, une exploration DFS du graphe Gamma exhaustif donne les composantes ; leurs minima coïncident avec toutes les composantes du modèle. Aux deux visites, la facette demandée et toutes les facettes du terminal appartiennent à la même composante Gamma. Cette vérification ne reprend ni la descente ni le calendrier d’unions du producteur pour établir H0.

Le mutant « rendre l’ancienne racine mémorisée sans normaliser le token courant » passe à tort 25 au lieu de 34 à la seconde visite. Le lot nominal ne publie aucune fusion ; le mutant ajoute une **fausse fusion de parents `[25,34]`**, alors que 25 appartient déjà à la composante 34. Le tableau de successeurs passe de 35 à 36 cases et change aussi. Le juge exige ces différences de parents et de tableau, et non seulement que les deux valeurs de racine diffèrent. Il est réfuté sur ce témoin non vide ; les états avant/après sont conservés dans les reçus.

Commandes depuis la racine, `PYTHONDONTWRITEBYTECODE=1` recommandé :

```bash
python3 morsehgp3D_v7/audits/receipts_filtered_review_20260906/terminal_reuse_fixture.py
python3 -O morsehgp3D_v7/audits/receipts_filtered_review_20260906/terminal_reuse_fixture.py
```

Les sorties [normale](terminal_reuse_normal.json) et [optimisée](terminal_reuse_optimized.json) sont identiques, SHA-256 `4a72653b26a5e5055fa4cd5d1283682fa8095ededc57858327e3ee2c1540cbe9`. Les commandes exécutées ont été bornées à 60 secondes et épinglées sur CPU1 ; aucun moteur ni compilation C++ n’a été lancé. Elles épinglent le script et l’oracle rationnel consommé `ad6c0d6c…`, dont le hash est vérifié avant import. Le header `85c27ab9…` est une provenance constante rattachée à la capture immuable de la qualification L, consultée mais ni importée ni exécutée ; le rejeu ne dépend pas du header produit vivant. Les budgets physiques, exceptions, allocations et gains natifs restent hors de ce témoin ; les sentinelles de transitions abstraites sont une preuve distincte.
