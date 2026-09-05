# État courant de l’audit v7

La normalisation v2 est qualifiée indépendamment sur le header `85c27ab9` : 114 ordres et 69 120 coupes par build, forêts et 32 autres compteurs conservés. Le helper passe 3 851 appels par build et deux mutants causaux sont réfutés. La suite MEB privée dispose de filtres démontrés et d’invariants précis pour préserver la première base acceptée. L’export industriel et les contrats de performance restent ouverts.

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
| L, successeurs v2 `85c27ab9` | [114 ordres, 912 sorties et 69 120 coupes par build ; 3 851 appels du helper](receipts_full_successor_20260905/README.md), deux mutants ; captures constructeur 20+20 contre-vérifiées |
| K, lot unitaire `21b77d29` | [114 ordres, 912 sorties et 69 120 coupes par build](CACHE_FULL_COURANT.md) ; mutation du quatrième parent ; captures constructeur 17+17 contre-vérifiées |
| J, lazy `13c6cc72` | [109 ordres et 67 920 coupes par build O2/ASan-UBSan](CACHE_FULL_COURANT.md), quatre politiques, budgets, trois mutants ; 14+14 CTests propres, admission n=8 de la sonde et first-C contre-vérifiés |
| I/H, EAGER `e02d163c` | [100 ordres indépendants](PRODUCTEUR_FULL_GABRIEL_COURANT.md) ; [trois réussites mono 8k et deux refus d’alias](MONO_FULL_COURANT.md), sans transfert de leurs temps vers lazy |
| G, lecteur FULL | [Qualification structurelle](CERTIFICAT_FULL_CPP_COURANT.md), sans certification géométrique |
| D/E/F, réduit et primitives | [Qualifications distinctes](AUDIT_QUALIFICATION_20260905.md) ; aucun reçu réduit réinterprété FULL |

Les [documents du constructeur](../PASSATION.md) portent les campagnes ultérieures et leur progression. Le [diagnostic mono](MONO_FULL_COURANT.md) contrôle leurs seuls compteurs de normalisation, sans qualifier les latences massives. Le [manifeste](validation_current.json) ajoute explicitement L aux variantes D–K conservées intactes. L porte le worktree capturé sur `04fd4c89`, sans anticiper le commit constructeur. La sonde v3, son admission et ses nouveaux temps ne sont pas qualifiés par L.

Le [dialogue actif](DIALOGUE_COURANT.md) clôt la demande de qualification de normalisation et précise la suite MEB. Il conserve aussi l’incident de coordination CPU signalé : le constructeur exclut le temps du passage 8k/s8 recouvrant quatre brefs rejeux de l’audit. Les questions différées sont [regroupées](QUESTIONS_SECONDAIRES.md) ; les demandes déjà satisfaites restent retirées.

Les reçus bruts et échecs restent conservés ; les anciennes synthèses sont accessibles par le [registre d’entretien](ENTRETIEN.json). GCP non utilisé.
