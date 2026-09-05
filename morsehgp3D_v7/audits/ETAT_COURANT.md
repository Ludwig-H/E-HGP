# État courant de l’audit v7

Actualisé le 5 septembre 2026 depuis `71895104c7d750fdd6ef0a313cfb09e9ac161204` : certificat horizontal E, contrats verticaux/pondérés et qualification distincte de F. Les écritures restent exclusivement dans ce dossier, sur `main` sans nouvelle branche.

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

**Le [certificat horizontal réduit](CERTIFICAT_HORIZONTAL_COURANT.md) est maintenant fermé sur E**, sous son domaine CPU et sa régularité de fenêtre/descente explicités. Il conserve composantes, points et évolution entre coupes ; aucune régularité globale ni matérialisation de Gamma n'est exigée. Les [applications verticales](CONTRAT_VERTICAL_COURANT.md) ont maintenant leur preuve par conjugaison, avec une ancre certifiée suffisante par composante ; leur resolver et export restent à construire. Le [contrat de masses/vote](CONTRAT_MASSES_VOTE_COURANT.md) distingue les univers d’incidence et leur autorité. Identités publiques, plateaux à étendre et coûts restent séparés. Le payload demeure `normalized_horizontal_h0_candidate`, l'archive `vertical_maps=none` et le statut `not_claimed` ; `--require-exact` refuse.

Les constats A1/C1 et les demandes d’intégration Cassini/U320 sont fermés. Douze notes transitoires sont [consolidées](receipts_front_20260905/documentation_retirement.json) ; leurs preuves brutes et fixtures sont conservées. Aucun chantier déjà fermé n’est rouvert par le nettoyage.

Consulter la [synthèse](AUDIT_INDEPENDANT_20260904.md) et le [dialogue actif](DIALOGUE_COURANT.md). Les dernières compilations de l’auditeur sont closes à 09:21:33 UTC. La reprise verticale lit les sorties scellées : 764 images, 720 carrés de naturalité et 400 compositions de deux niveaux par build E source ; aucun moteur lancé pendant la fenêtre E/F ouverte à 09:40 UTC. Les paires uniques sur hôte partagé ne constituent ni un gain statistique ni un SLO. GCP non utilisé ; aucun résultat GPU attribué à cet audit.
