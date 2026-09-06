# État courant de l’audit v7

Le raccord FULL du proposeur MEB filtré publié par `20b28b1d` passe une **nouvelle qualification indépendante O2 et ASan/UBSan** : 2 784 sorties et 214 704 coupes par build, avec budgets et Work persistant. Deux ordres n=14/K9/K10 étendent le corpus. Les réserves d’intégration sont closes ; l’export industriel et les contrats de performance restent ouverts.

Le [suivi du 6 septembre](receipts_probe_meb_review_20260906/README.md) ajoute une preuve quadratique sur les **minima FULL**, pour chaque K fixé≥2 à précision croissante, et des paires strictes sur les littéraux u16. Il apporte aussi deux gardes contre les faux diagnostics d’économie F et ferme les anciens défauts de format après contrelecture de 48 vraies paires / 312 ordres du comparateur réparé. Aucun nouveau moteur ni temps de tour n’est ajouté.

La [question sur les seuls sommets Gabriel](receipts_gabriel_vertices_20260906/README.md) est résolue mathématiquement : la restriction induite retarde une vraie fusion K-NN sur quatre points réguliers ; un quotient sur les minima avec les bons liens préserve exactement la hiérarchie. Une descente de facettes de cardinal constant permet un resolver sans ancres directes horizontales. Le prototype rationnel compose toute la tour K1..4, avec 76 comparaisons Γ et les cartes verticales. C’est une piste constructive nouvelle, sans qualification C++ ni gain de temps acquis ; le partage géométrique entre ordres existe déjà dans la v7.

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
| O, raccord FULL `20b28b1d` | [116 ordres, budgets, K9/K10 et deux mutants](receipts_full_meb_20260906/README.md) ; nouveaux builds indépendants, captures constructeur 30+30 contre-vérifiées |
| N, filtre privé publié par `62e5cd76` | [Captures R2, frontières MAX et ordre admissible](receipts_filtered_review_20260906/README.md) ; qualification locale historique distincte |
| M, publication et captures `5633bc5a` | [29 comparaisons / 204 ordres, rejeu s8 et diagnostic du refus MEB](receipts_followup_20260906/README.md) ; lectures seules, aucune nouvelle qualification C++ ou de performance |
| L, successeurs v2 `85c27ab9` | [114 ordres, 912 sorties et 69 120 coupes par build ; 3 851 appels du helper](receipts_full_successor_20260905/README.md), deux mutants ; captures constructeur 20+20 contre-vérifiées |
| K, lot unitaire `21b77d29` | [114 ordres, 912 sorties et 69 120 coupes par build](CACHE_FULL_COURANT.md) ; mutation du quatrième parent ; captures constructeur 17+17 contre-vérifiées |
| J, lazy `13c6cc72` | [109 ordres et 67 920 coupes par build O2/ASan-UBSan](CACHE_FULL_COURANT.md), quatre politiques, budgets, trois mutants ; 14+14 CTests propres, admission n=8 de la sonde et first-C contre-vérifiés |
| I/H, EAGER `e02d163c` | [100 ordres indépendants](PRODUCTEUR_FULL_GABRIEL_COURANT.md) ; [trois réussites mono 8k et deux refus d’alias](MONO_FULL_COURANT.md), sans transfert de leurs temps vers lazy |
| G, lecteur FULL | [Qualification structurelle](CERTIFICAT_FULL_CPP_COURANT.md), sans certification géométrique |
| D/E/F, réduit et primitives | [Qualifications distinctes](AUDIT_QUALIFICATION_20260905.md) ; aucun reçu réduit réinterprété FULL |

Le [manifeste](validation_current.json) ajoute O sur le publié `20b28b1d`, en conservant D–N. Les deux headers compilés sont des copies capturées ; les autres dépendances L sont réutilisées comme octets inchangés. CMake, deux documents et la porte locale en préparation gardent leurs pins publiés et expliquent l’écart de fraîcheur courant. La sonde v4 n’est ni épinglée ni qualifiée par O. Le juge se rejoue sur les captures sans moteur.

Le [dialogue actif](DIALOGUE_COURANT.md) situe la descente vers les minima et la recherche d’une tour plus efficace. Le [diagnostic mono](MONO_FULL_COURANT.md) conserve le refus historique 32k/K9 aux quatre millions d’appels ; sa suppression prospective dans la sonde ne change pas ce reçu. La [réutilisation terminale](MEB_DOUBLE_BUDGET_COURANT.md#réutiliser-une-certification-terminale-déjà-acquise) reste une proposition séparée, avec sa [fixture causale n=12/K7](receipts_filtered_review_20260906/terminal_reuse_fixture.md). Les questions différées sont [regroupées](QUESTIONS_SECONDAIRES.md).

Les reçus bruts et échecs restent conservés ; les anciennes synthèses sont accessibles par le [registre d’entretien](ENTRETIEN.json). GCP non utilisé.
