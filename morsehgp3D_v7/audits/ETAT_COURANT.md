# État courant de l’audit v7

Actualisé le 5 septembre 2026 depuis `4cc804e50c9effdc6fb65b157df0f8b5168bf60e` : reconstruction verticale prouvée depuis les tokens E, autorité de comparaison p3 et paliers F clos. Les écritures restent exclusivement dans ce dossier, sur `main` sans nouvelle branche.

```text
phase=exploration_v7_hors_registre
backend=cpu_reference
profile=quantized_u16_input_only
mode=audit_independant_math_and_architecture
public_status=not_claimed
```

La porte exploratoire est satisfaite : demande v7 explicite, lecture intégrale des parties I et II du manuscrit, sources et limites déclarées. Le [manifeste courant](validation_current.json) distingue les snapshots entiers et leur portée ; son code 0 affiche celui qui correspond et sa portée, sans promotion publique.

| Source reconnue | Qualification réellement acquise |
| --- | --- |
| D, produit du commit `e6d33698` | Construction indépendante neuve Release : 323/323 CTests CPU, zéro échec/skip, 115 sources et 37 binaires stables ; MEB/index/arrondi jugés séparément |
| E q2, publié et figé | Certificat horizontal réduit assemblé ; 840 coupes du vrai pipeline et 272 coupes du fold par build O2/UBSan ; reçus propres E324 et E33+33 contre-vérifiés |
| F, sources constructeur épinglées | Conservation statique LIFO/masques/comptes et campagnes propres 339/339 Release, 48/48 ciblées Release, 48/48 ASan/UBSan contre-vérifiées ; aucun transfert des tests E |

**Le registre arithmétique et son raccord compilé sont fermés** pour les [fuseaux](ARITHMETIQUE_SPINDLE_COURANTE.md), [secteurs/cordes](ARITHMETIQUE_SECTEUR_CORDE_COURANTE.md) et [cellules](ARITHMETIQUE_CELLULES_COURANTE.md). Trois sondes indépendantes passent chacune en O2 et O1 UBSan ; six vrais mutants produit sont détectés, avec une faute de paramètre d'audit séparée. La base des secteurs réussit dès A=B=1 ; les grilles sont confrontées à 38 400 cellules par build, avec des coordonnées atteignant 98 bits.

**Le [certificat horizontal réduit](CERTIFICAT_HORIZONTAL_COURANT.md) est maintenant fermé sur E**, sous son domaine CPU et sa régularité de fenêtre/descente explicités. Il conserve composantes, points et évolution entre coupes ; aucune régularité globale ni matérialisation de Gamma n'est exigée. Les [applications verticales](CONTRAT_VERTICAL_COURANT.md) sont désormais reconstructibles depuis `born` et `parents` : la totalité du scan de naissance est prouvée, sans resolver géométrique général. Le lecteur d’audit existe ; son port et son export produit restent à réaliser. Le [contrat de masses/vote](CONTRAT_MASSES_VOTE_COURANT.md) distingue les univers d’incidence. L’[autorité p3](AUTORITE_VOTE_P3_COURANTE.md) compare exactement les numérateurs sous budgets explicites ; les quotients de masses et la condensation restent distincts. Identités publiques, plateaux à étendre et coûts restent séparés. Le payload demeure `normalized_horizontal_h0_candidate`, l'archive `vertical_maps=none` et le statut `not_claimed` ; `--require-exact` refuse.

Les constats A1/C1 et les demandes d’intégration Cassini/U320 sont fermés. Douze notes transitoires sont [consolidées](receipts_front_20260905/documentation_retirement.json) ; leurs preuves brutes et fixtures sont conservées. Aucun chantier déjà fermé n’est rouvert par le nettoyage.

Consulter la [synthèse](AUDIT_INDEPENDANT_20260904.md) et le [dialogue actif](DIALOGUE_COURANT.md). Les dernières compilations de l’auditeur sont closes à 09:21:33 UTC. Les [nouveaux reçus](receipts_resolver_20260905/README.md) relisent les 16 sorties E scellées : 764 cartes, 720 carrés et 400 compositions par provenance. Le réindexage et la multifusion synthétique sont comptés séparément. Les 27 cas p3 passent normalement et sous Python optimisé. Aucun moteur ni build n’est relancé. Les trois paires E/F 8k ont des objets égaux ; F16k termine en 413,816 s et F32k refuse à K9 sur les occurrences temporaires, sans tour complète. Ces mesures uniques sur hôte partagé ne constituent ni un gain statistique ni un SLO. GCP non utilisé ; aucun résultat GPU attribué à cet audit.
