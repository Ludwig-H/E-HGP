# État courant de l’audit v7

La qualification indépendante FULL lazy reste attachée à `13c6cc72`. Le constructeur prépare la spécialisation du lot unitaire : ce nouveau code n’est pas encore qualifié ici. Le nouvel avis démontre comment supprimer deux opérations redondantes par normalisation non triviale ; ses effets sur le compteur sont recalculés sur 48 ordres clos, sans promettre une accélération ni l’admission du K9 refusé. L’export industriel et les contrats de performance restent ouverts.

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
| J, lazy `13c6cc72` | [109 ordres et 67 920 coupes par build O2/ASan-UBSan](CACHE_FULL_COURANT.md), quatre politiques, budgets, trois mutants ; 14+14 CTests propres, admission n=8 de la sonde et first-C contre-vérifiés |
| I/H, EAGER `e02d163c` | [100 ordres indépendants](PRODUCTEUR_FULL_GABRIEL_COURANT.md) ; [trois réussites mono 8k et deux refus d’alias](MONO_FULL_COURANT.md), sans transfert de leurs temps vers lazy |
| G, lecteur FULL | [Qualification structurelle](CERTIFICAT_FULL_CPP_COURANT.md), sans certification géométrique |
| D/E/F, réduit et primitives | [Qualifications distinctes](AUDIT_QUALIFICATION_20260905.md) ; aucun reçu réduit réinterprété FULL |

Les [documents du constructeur](../PASSATION.md) portent les campagnes ultérieures et leur progression. Le [diagnostic mono](MONO_FULL_COURANT.md) contrôle leurs seuls compteurs de normalisation, sans qualifier les latences massives. Le [manifeste](validation_current.json) conserve les sources qualifiées : il signale normalement une dérive sur le nouveau code en préparation, sans le réépingler silencieusement.

Le [dialogue actif](DIALOGUE_COURANT.md) donne la preuve du raccourci de normalisation, les avis antérieurs retenus et les contrôles ciblés des prochains deltas. Les questions différées sont [regroupées](QUESTIONS_SECONDAIRES.md) ; les demandes déjà satisfaites restent retirées.

Les reçus bruts et échecs restent conservés ; les anciennes synthèses sont accessibles par le [registre d’entretien](ENTRETIEN.json). GCP non utilisé.
