# Bascule A : deux correctifs intégrés et un raccord de commande proposé

8 octobre 2026, Codex. **86d7e39d834cae86768d49a23d83bf53454ec2a3** intègre exactement les deux propositions déjà publiées : [fenêtre des feuilles](../t2d_a_fenetre_patch/README.md) et [admission de la campagne A](../t2da_integration/README.md). Application indépendante des deux diffs sur les blobs parents 41d4 : les deux postimages sont identiques au livré, octet pour octet. Aucune exécution native, GPU ou GCP ; aucun nouveau temps.

La fenêtre réelle `[t0,t1]` remplace la fenêtre décalée de la pré-passe. Le seul appel avec LeafWindow est celui de run_g ; le noyau conserve sa charge englobante. La preuve antérieure couvre durée nulle, refus et absence de double charge ; elle n’est pas présentée comme une nouvelle qualification concurrente. `pipeline_run.cpp` livré porte SHA256 **f3d260ae51159c5c039082f8b1442434123bdb59114d4364c4d0a4e7ef8f09a6**.

Pour le pilote livré SHA256 **12c515c1b482088c1c4fb46ab4e1b8159a7d6e9246d2a84859fb1766f0ecee75**, le rejeu JSON reprend explicitement par AST les fabriques et la fonction exercise publiées, avec `corrected=True`. Le nominal est admis (`adopte`) ; les trois contre-exemples sont `refuse` : observation GPU inconnue, trame étrangère ajoutée, bras après réellement séquentiel. Les refus proviennent des gardes de campagne, pas d’une nouvelle règle statistique. Le présent reçu ferme ces résidus source/admission dans leur portée ; il ne réadmet pas une campagne réelle ni tous les lecteurs FULL.

La bascule rend toutefois une future prise `apres_sequentiel` inexécutable telle que demandée : `commande` n’ajoute un flag que pour le schéma recouvert. Sans flag, la sonde livrée choisit désormais build_tower, tandis que le pilote attend des lignes séquentielles et les refusera. C’est un **faux refus de futur rejeu**, pas une invalidation des prises historiques antérieures à la bascule.

`commande.patch` est une proposition non appliquée au produit. Elle ajoute `--sequentiel` uniquement pour **bras=apres, schema=sequentiel**, et corrige trois commentaires. Le bras avant reste sans flag : ses archives 902041f66 et 27eca166b ne reconnaissent pas cette option. Les appels actuels de prise renseignent déjà `attendu["bras"]`. Les modes CPU/appareil, empreinte et les autres arguments restent identiques.

Douze vérifications de commande (trois bras/schémas réellement utilisés × CPU/appareil × empreinte oui/non) passent sur la proposition appliquée en copie isolée. Le témoin conserve l’absence fautive des deux flags sur le pilote livré et constate le seul ajout demandé sur la proposition. Les anciens corps CLI sont épinglés et relus, sans lancer leurs binaires.

```sh
python check.py /chemin/depot
python -O check.py /chemin/depot
```

Sorties identiques à `results.json`. Le lecteur charge uniquement les fonctions du pilote ; son main et ses commandes de construction/campagne ne sont jamais appelés. Huit blobs principaux, trois préimages et deux sondes historiques sont hachés dans `capture.json`. Les anciens reçus sont inchangés. L’égalité des postimages clôt la reprise des propositions, pas les autres limites de diagnostics ou la qualification de performance A+B.
