# État courant de l’audit v7

Actualisé le 5 septembre 2026 depuis `35dda097f75a66f8264002c58b9ccc4888c46d2e`. Les écritures restent exclusivement dans ce dossier, sur `main` sans nouvelle branche.

```text
phase=exploration_v7_hors_registre
backend=cpu_reference
profile=quantized_u16_input_only
mode=audit_independant_math_and_architecture
public_status=not_claimed
```

La porte exploratoire est satisfaite : demande v7 explicite, lecture intégrale des parties I et II du manuscrit, sources et limites déclarées. Le [manifeste courant](validation_current.json) distingue deux snapshots entiers ; son code 0 affiche celui qui correspond et sa portée, sans promotion publique.

| Source reconnue | Qualification réellement acquise |
| --- | --- |
| D, produit du commit `e6d33698` | Construction indépendante neuve Release : 323/323 CTests CPU, zéro échec/skip, 115 sources et 37 binaires stables ; MEB/index/arrondi jugés séparément |
| E q2, delta constructeur épinglé | Preuve locale et même oracle rationnel : 431 appels identiques à D, mutant q2 détecté ; aucune suite complète E exécutée par cet audit |

**Le registre arithmétique des témoins est maintenant couvert** par les preuves des [fuseaux](ARITHMETIQUE_SPINDLE_COURANTE.md), [secteurs/cordes](ARITHMETIQUE_SECTEUR_CORDE_COURANTE.md) et [cellules](ARITHMETIQUE_CELLULES_COURANTE.md). La base des secteurs réussit dès A=B=1 dans le domaine appelé. Les nouveaux certificats sont des calculs Python légers ; les frontières proposées restent à raccorder directement aux helpers compilés.

La composition S1 dispose des preuves géométriques, du raccord index/front, des bornes de primitives et d’un [domaine CPU explicite](DOMAINE_CPU_COURANT.md). La qualification intégrée du prochain delta, le certificat horizontal réduit et son domaine de régularité, puis les plateaux, la verticale, les poids et les coûts gardent leurs obligations. Le payload par défaut reste `verified_events_only`, la complétion `normalized_horizontal_h0_candidate`, l’archive `vertical_maps=none` ; `--require-exact` refuse.

Les constats A1/C1 et les demandes d’intégration Cassini/U320 sont fermés. Douze notes transitoires sont [consolidées](receipts_front_20260905/documentation_retirement.json) ; leurs preuves brutes et fixtures sont conservées. Aucun chantier déjà fermé n’est rouvert par le nettoyage.

Consulter la [synthèse](AUDIT_INDEPENDANT_20260904.md) et le [dialogue actif](DIALOGUE_COURANT.md). Aucune nouvelle compilation ou charge lourde n’a été lancée pendant la fenêtre de mesure constructeur. GCP non utilisé ; aucun résultat GPU attribué à cet audit.
