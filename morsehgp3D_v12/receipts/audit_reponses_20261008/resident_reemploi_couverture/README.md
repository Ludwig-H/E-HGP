# Réemploi après succès : couverture et porte manquante

8 octobre 2026, audit Codex. Pin `95d927da0`, après livraison A6b `f2c106d93`.
Complément ciblé de CST-0243 et du [diagnostic TU Wien](../b1t_tuwien_memoire/README.md), sans reprendre
ses observations historiques. **Aucun correctif de cette politique mémoire n'est présent dans les sources
examinées.** Les onze sources produit/tests/sonde capturées sont identiques à `caf9585e4` ; seul
`tests/catalogue/tests.cmake` diffère parmi les douze fichiers, par les juges Python T1-d. Aucun moteur,
compilation, GPU, contrôleur ni contenu de jeu n'a été exécuté ou lu par l'auditeur.

## Ce que les portes existantes couvrent

| Porte | Contexte et succession | Limite de la preuve pour CST-0243 |
| --- | --- | --- |
| `device_open` | Vrai `CatalogueDevice` conservé ; deux succès consécutifs par témoin, comparaison CPU et entre passes | Budget illimité ; la première sortie reste vivante lors du second appel. Ne force pas une finition complète ayant rendu le front. |
| `device_open_budget` | Deux fixtures, sept appels chacune, soit quatorze ; `device_trial` ouvre un contexte neuf par appel | Refus propres et adaptation sous limite ; aucune deuxième passe sur cet état. |
| `pipeline_resident` | État Pool conservé, gros/petit/triangle/gros ; sorties de chaque itération détruites | Budget illimité ; `PoolExecutor::adopt` peut en plus déplacer un tableau de finition vers la sortie. Ce n'est pas le stockage résident CUDA. |
| `pipeline_budget_reuse` | Même `StagedExecutor` : refus hôte au pic moins un octet, puis petit carré réussi | Reprise après refus, pas réemploi du même gros nuage après succès. |
| `slices_device_reuse` | Même `StagedExecutor`, budget appareil pic/3 : gros, carré, gros ; identités vérifiées | Les deux gros succès exigent `finish_slices >= 1`. Cette voie purge la finition avant le retour. Le petit succès complet intercalé n'a pas la même pression mémoire. |

Il serait donc faux de conclure « aucun succès répété n'est testé ». **La combinaison manquante est : même
état, budget fini, succès complet après restitution du front, puis immédiatement le même gros calcul,
sortie précédente détruite.** `finish_slices == 0` seul ne prouve pas que le front a été rendu : publier
un témoin de cette restitution. Un pic SMI n'est pas ce témoin.

Les traces de portes déjà admises gardent leur portée. Cette lecture de sources n'ajoute aucun test
natif exécuté aux 753 réussites locales et à la sentinelle sautée de la clôture A6b.

## Le seuil Pool ne se transfère pas au GPU

Le transit de `StagedExecutor` suit la politique CUDA et son `adopt` renvoie toujours faux. Ses tableaux
restent cependant des `FrontArray` hérités de `PoolExecutor` : croissance `max(n, 2*cap)`. CUDA utilise
`max(n, cap + floor(cap/2) + 1024)`. L'ancien tableau est rendu avant croissance sans contenu conservé ;
avec contenu conservé, ancien et nouveau coexistent. Les capacités, pics et points de bascule peuvent
donc différer. L'exactitude d'une sortie Pool, une limite favorable Pool et un succès réel CUDA sont
trois observations distinctes.

Autre précaution : `full_finish_bytes` est documenté comme **estimation de décision**, non garantie des
réservations. Une correction de réemploi ne doit pas être qualifiée en soustrayant aveuglément tous les
octets résidents de cette estimation : les familles inutilisées, les capacités et les pics transitoires
de croissance doivent rester comptés. La réserve finale est toujours contrôlée par le budget.

## Protocole minimal proposé

Le [plan déclaratif](protocole.json) n'est ni un lanceur ni un résultat. Une courte recherche sur les
deux nuages publics déjà utilisés par les portes est séparée de la qualification : mesurer un pic froid
neuf, essayer les huit fractions prédéclarées, conserver chaque issue et la route effectivement prise.
Un candidat exige, à la première passe, succès complet, arène non diffusée, front rendu et finition
encore résidente. **Si aucun candidat n'atteint ces conditions, le diagnostic est non couvrant**, même
si toutes ses sorties sont correctes. La recherche ne présume aucun seuil ni aucun défaut reproduit.

Une fois ce témoin observé, figer sa génération, ses paramètres et son budget avant de qualifier le
correctif. Jouer trois fois le même nuage dans le même état, avec trois succès exigés, sans petit nuage,
purge de l'appelant ni réouverture intercalés ; détruire chaque résultat avant l'appel suivant. À chaque
succès : empreinte catalogue, grand livre, mots des niveaux et recherche de chaque support identiques
à la référence CPU. Publier chaque route et contrôler séparément budgets hôte/appareil ; vérifier leur
restitution seulement après destruction de toutes les sorties, de l'état et de l'exécuteur.

Les passes suivantes peuvent emprunter une adaptation autorisée par le produit ; si le correctif promet
expressément une route complète stable, cette route devient aussi une assertion. **Ne pas recalibrer le
seuil au code corrigé**, sinon une régression peut être masquée. Le témoin CUDA a sa propre limite figée,
à cause des capacités différentes. La porte Pool ne dispense pas du vrai réemploi GPU.

La reprise TU Wien est ensuite distincte : même Session FULL, budgets 160/88 Gio et cache 8 Gio, trois
passes demandées, sorties détruites entre passes. Conserver étapes et refus, avant/après des usages et
capacités, fermeture des sources/ELF et arrêt certifié. Les purges éventuelles restent dans C/FULL.
Le seuil historique qui désactive FUL1 au-delà de 1,6 million ne constitue pas une identité massive :
une nouvelle comparaison complète doit être activée et budgétée explicitement si elle est recherchée.

```sh
python -B check.py DEPOT_GIT
python -B -O check.py DEPOT_GIT
```

Le lecteur ferme les douze sources Git, vérifie les marqueurs de couverture et la cohérence du plan.
Ses sorties normal et optimisé sont identiques à `results.json`. Il ne joue aucun test du moteur et
ne qualifie aucune politique de mémoire ; CST-0243 reste ouvert.
