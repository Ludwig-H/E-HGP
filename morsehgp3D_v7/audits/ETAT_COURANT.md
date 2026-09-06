# État courant de l’audit v7

La contrelecture du filtre privé `484a89bc` publié par `62e5cd76` est favorable : captures liées, 3 430 appels rationnels et 1 507 ordinaux par build rejugés, budgets ciblés et trois mutants vérifiés. La normalisation FULL v2 conserve sa qualification indépendante sur `85c27ab9`. Le raccord produit du filtre est maintenant en préparation dans le worktree constructeur : son nouveau C++ n’hérite pas des qualifications locales privées. L’export industriel et les contrats de performance restent ouverts.

```text
phase=exploration_v7_hors_registre
backend=cpu_reference
profile=quantized_u16_input_only
mode=audit_independant_math_and_architecture
public_status=not_claimed
```

La lecture intégrale des parties I et II du manuscrit, PDF 35–134, reste acquise. La [décision FULL](NIVEAUX_ET_CERTIFICAT_HGP_COURANT.md) distingue minima, multifusions et rattachements silencieux, K=n, régularité, verticale et poids.

| Autorité | Résultat conservé |
| --- | --- |
| N, filtre privé publié par `62e5cd76` | [Captures R2, frontières MAX et ordre admissible](receipts_filtered_review_20260906/README.md) ; aucun nouveau moteur C++, intégration FULL encore à qualifier |
| M, publication et captures `5633bc5a` | [29 comparaisons / 204 ordres, rejeu s8 et diagnostic du refus MEB](receipts_followup_20260906/README.md) ; lectures seules, aucune nouvelle qualification C++ ou de performance |
| L, successeurs v2 `85c27ab9` | [114 ordres, 912 sorties et 69 120 coupes par build ; 3 851 appels du helper](receipts_full_successor_20260905/README.md), deux mutants ; captures constructeur 20+20 contre-vérifiées |
| K, lot unitaire `21b77d29` | [114 ordres, 912 sorties et 69 120 coupes par build](CACHE_FULL_COURANT.md) ; mutation du quatrième parent ; captures constructeur 17+17 contre-vérifiées |
| J, lazy `13c6cc72` | [109 ordres et 67 920 coupes par build O2/ASan-UBSan](CACHE_FULL_COURANT.md), quatre politiques, budgets, trois mutants ; 14+14 CTests propres, admission n=8 de la sonde et first-C contre-vérifiés |
| I/H, EAGER `e02d163c` | [100 ordres indépendants](PRODUCTEUR_FULL_GABRIEL_COURANT.md) ; [trois réussites mono 8k et deux refus d’alias](MONO_FULL_COURANT.md), sans transfert de leurs temps vers lazy |
| G, lecteur FULL | [Qualification structurelle](CERTIFICAT_FULL_CPP_COURANT.md), sans certification géométrique |
| D/E/F, réduit et primitives | [Qualifications distinctes](AUDIT_QUALIFICATION_20260905.md) ; aucun reçu réduit réinterprété FULL |

Le [manifeste](validation_current.json) ajoute N sur les sources publiées par `62e5cd76`, en conservant D–M. Il ne repointe pas les anciens reçus sur le raccord produit actuellement modifié : un écart de fraîcheur sur ces fichiers signale donc la préparation concurrente, pas une intégration implicitement qualifiée. La contrelecture R2 s’exécute sur ses captures immuables.

Le [dialogue actif](DIALOGUE_COURANT.md) répond au raccord Work/F et aux préfixes d’exception, et fournit une [fixture causale n=12/K=7](receipts_filtered_review_20260906/terminal_reuse_fixture.md) pour la [réutilisation des terminaux](MEB_DOUBLE_BUDGET_COURANT.md#réutiliser-une-certification-terminale-déjà-acquise). Le [diagnostic mono](MONO_FULL_COURANT.md) conserve le refus 32k/K9 aux quatre millions d’appels : le filtre interne seul ne change pas ce nombre. La correction de base positive unique et l’incident CPU sont clos et documentés ; les questions différées sont [regroupées](QUESTIONS_SECONDAIRES.md).

Les reçus bruts et échecs restent conservés ; les anciennes synthèses sont accessibles par le [registre d’entretien](ENTRETIEN.json). GCP non utilisé.
