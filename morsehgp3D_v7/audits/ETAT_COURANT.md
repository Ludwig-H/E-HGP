# État courant de l’audit v7

Actualisé le 5 septembre 2026 : reprise depuis `a32dc78f`, puis publication constructeur E `2b94abddfde08101607f4639d42149156fb39e6c` vérifiée sur `origin/main`. Les écritures restent exclusivement dans ce dossier, sur `main` sans nouvelle branche.

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
| E q2, delta constructeur épinglé | Oracle indépendant : 431 appels identiques à D ; reçus constructeur propres contre-vérifiés : 324/324 Release, 33/33 ciblés Release et 33/33 ASan/UBSan ; cardinalités et digests égaux entre D/E et entre séparations |

**Le registre arithmétique et son raccord compilé sont fermés** pour les [fuseaux](ARITHMETIQUE_SPINDLE_COURANTE.md), [secteurs/cordes](ARITHMETIQUE_SECTEUR_CORDE_COURANTE.md) et [cellules](ARITHMETIQUE_CELLULES_COURANTE.md). Trois sondes indépendantes passent chacune en O2 et O1 UBSan ; six vrais mutants produit sont détectés, avec une faute de paramètre d'audit séparée. La base des secteurs réussit dès A=B=1 ; les grilles sont confrontées à 38 400 cellules par build, avec des coordonnées atteignant 98 bits.

La composition S1 dispose des preuves géométriques, du raccord index/front, des bornes de primitives et d’un [domaine CPU explicite](DOMAINE_CPU_COURANT.md). La qualification E dispose désormais de ses propres preuves terminales ; l'auditeur les inspecte sans se réattribuer leur exécution. Restent le certificat horizontal réduit et son domaine de régularité, puis les plateaux, la verticale, les poids et les coûts. Le payload par défaut reste `verified_events_only`, la complétion `normalized_horizontal_h0_candidate`, l’archive `vertical_maps=none` ; `--require-exact` refuse.

Les constats A1/C1 et les demandes d’intégration Cassini/U320 sont fermés. Douze notes transitoires sont [consolidées](receipts_front_20260905/documentation_retirement.json) ; leurs preuves brutes et fixtures sont conservées. Aucun chantier déjà fermé n’est rouvert par le nettoyage.

Consulter la [synthèse](AUDIT_INDEPENDANT_20260904.md) et le [dialogue actif](DIALOGUE_COURANT.md). Les compilations nouvelles sont postérieures à la fenêtre de mesure constructeur. Les paires uniques sur hôte partagé ne constituent ni un gain statistique ni un SLO. GCP non utilisé ; aucun résultat GPU attribué à cet audit.
