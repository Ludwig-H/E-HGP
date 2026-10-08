# T2d-C — admission réécrite, contre-rejeu du prototype

Le prototype capturé (`lecteur 0b68756f`, `pilote 88ca8e3f`, `juge a00c2c0b`, [pins](capture.json)) corrige les défauts principaux du [reçu historique](../t2d_c_admission/README.md). Sources figées hors Git, relues inchangées après rejeu ; **aucune livraison produit, compilation, sonde native ou campagne G4 qualifiée ici**.

**39 cas du développeur passent en Python normal et −O**, dont 25 marqués admission. La fixture native historique CPU/W1 `3b6c4261`, inchangée, injectée dans le vrai `probe_run` à la place de sa seule commande externe, est désormais refusée pour la commande GPU/W48 (ligne open absente). Le moteur n’est pas exécuté. Les rapports complets des autres cas sont synthétiques : ils éprouvent le juge, sans certifier des compteurs géométriques ni une provenance de binaire.

| Obligation historique | Résultat à ce pin |
| --- | --- |
| Succès CPU/u18/K10/W1/feuille16, raison sous statut ok, indice répété, champs/étapes absents | Refusés ; commande, séquence, types et diagnostics relus dans les lignes natives. |
| Prise manquante / comptes d’une autre entrée | Refusés ; les comptes des prises sont liés à ceux de l’identité. |
| Mutant code0 sans comparaison | Refusé ; deux empreintes exigées. Le survivant est rejeté. |
| A/A à 1,02 ou 1,01 | Respectivement refusé/adopté selon la fenêtre explicite ±1,5 %. |
| Borne haute 0,99996 | Adoptée sur la valeur non arrondie ; affichage séparé. |
| Noms des bras | `repli_cles_entieres` conserve le repli sélectif ; `flux_et_repli_selectif` retire anticipation, second tampon et petites fenêtres. Le flux seul est décisif sur ng00/ng01 ; l’ancien repli n’a pas de bras. |

**Le chemin d’échec du mutant reste permissif.** À partir du nominal adopté, les trois mutations suivantes laissent chacune le rapport `adopte`, avec mutant déclaré tué :

- code3, statut `invariant_violated`, raison inconnue `not_a_reason` ;
- code3, statut `invariant_violated`, raison `memory_budget`, alors que `reasons.def:28` la lie à `resource_exhausted` ;
- ligne catalogue échouée CPU/u18/K10/W1/feuille16, indice `False`, mur `False`, malgré une commande appareil/u21/K5/W48.

Cause précise : `read_passes` vérifie les clés de l’échec, pas ses champs numériques/configuration ; `verdict_of_exit` compare les deux chaînes de cause sans les rattacher au catalogue fermé des raisons. Le producteur publie pourtant ces paramètres aussi en échec (`catalogue_probe.cpp:237–251`). Correction proposée : appliquer les mêmes gardes des champs communs aux succès et échecs, et vérifier le couple statut/raison contre `reasons.def` avant de compter le mutant tué. L’arrêt par signal est un cas séparé explicitement annoncé ; ce reçu ne le transforme pas en preuve d’une cause géométrique.

Deux résidus secondaires sont également reproduits. Un bras supplémentaire inconnu ajouté à une cohorte complète est ignoré (`campaign_table`), sans refus ; remplacer un bras requis reste bien refusé. Le lecteur FULL accepte `code=False` et `liberation.pass=False` pour la passe0, faute de garde `type(x) is int`. Fermer les ensembles de clés attendues des cohortes et typer ces champs complète l’admission annoncée. Aucun de ces témoins n’établit un défaut du moteur ou l’adoption passée d’une vraie campagne.

[check.py](check.py) réutilise les sources figées et leur auto-test, sans copier sources ni fixtures dans Git ; [results.json](results.json) porte les résultats. Les deux modes donnent les mêmes résultats. Rejeu, avec les deux chemins hors dépôt fournis explicitement :

```sh
python check.py --sources "$T2DC_SOURCE_SNAPSHOT" --fixture "$CPU_FORMAT_FIXTURE" --check
python -O check.py --sources "$T2DC_SOURCE_SNAPSHOT" --fixture "$CPU_FORMAT_FIXTURE" --check
```

Le lecteur refuse la dérive des huit sources importées et de la fixture. Pas de correction appliquée au prototype. Portée rattachée à CST-0018, sans clôture globale ; aucun nouveau temps ou gain annoncé.
