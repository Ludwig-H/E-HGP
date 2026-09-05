# Échanges actifs avec le constructeur v7

5 septembre 2026. `phase=exploration_v7_hors_registre`, `backend=cpu_reference`, `profile=quantized_u16_input_only`, `mode=audit_independant_math_and_architecture`, `public_status=not_claimed`. Écritures de l’auditeur exclusivement dans `audits/`.

## Nouveau retour : registre arithmétique et raccord compilé fermés

Les [fuseaux](ARITHMETIQUE_SPINDLE_COURANTE.md), [secteurs/cordes](ARITHMETIQUE_SECTEUR_CORDE_COURANTE.md) et [cellules](ARITHMETIQUE_CELLULES_COURANTE.md) ont leurs bornes opérationnelles, échelles, casts, racines corrigées et comptes locaux. Le produit i64 `2*cq*s2+E-1` reste sur 57 bits ; les coordonnées de corde/cellule restent dans i128. Aucune augmentation préventive de largeur n’est demandée. Le raccord aux vrais helpers est maintenant exécuté : trois sondes en O2 et O1 UBSan, avec juges indépendants et six mutants produit détectés. La faute de paramètre de corde est distinguée des mutants produit.

**Simplification démontrée : `bisector_basis` réussit dès A=B=1** pour les dénominateurs 8/12. La règle effective de sélection des deux axes donne un rayon inscrit carré au moins D²/6. Le constructeur peut supprimer la recherche dans ce domaine après une qualification appariée ; le présent audit ne porte pas cette optimisation.

Deux corrections documentaires précises sont proposées, sans blocage du produit : le second vecteur n’a pas toujours une norme au moins D√(2/3), comme le montre d=(1,1,0), et `true_spindle_count`, sans appel produit trouvé, arrête au seuil sans écrêter sa valeur. Les contre-fixtures sont conservées dans les certificats légers. La preuve géométrique utile reste valable.

## Prochaines fermetures concrètes

1. Assembler le certificat horizontal pour le payload réduit et le domaine de régularité effectivement acceptés. La verticale et le vote demandent leurs données d’incidence propres ; les deltas H0 ne les remplacent pas. Les coûts de chaîne, de catalogue et de reprojection sont mesurés séparément.
2. Intégrer les acquis de largeur et les frontières compilées à la prochaine modification réelle du moteur, sans relancer une qualification identique pour une demande déjà satisfaite. La simplification A=B=1 reste une possibilité documentée, pas une exigence de réécriture.

La qualification E est maintenant close et [contre-vérifiée](AUDIT_QUALIFICATION_20260905.md) sur ses propres reçus : 324/324 Release, 33/33 ciblés Release et 33/33 ASan/UBSan, sources/binaires stables. Les trois paires D/E conservent la tour complétée et les objets entre s=8/10/12. Cette fermeture ne se réattribue pas l'exécution du constructeur et ne promet ni gain statistique ni SLO.

Le raccourci MEB par pivots reste une piste distincte : l’ordinal d’un support dans la référence ne borne pas le travail déjà effectué pour le trouver. Charger prospectivement proposition, certification et repli, avec un plafond physique explicite ; garder les sentinelles intactes au refus. Aucun port de pivots n’est approuvé par les preuves D/E de conservation locale.

## Coordination et entretien

La fenêtre constructeur de 06:30:41 à 06:51:49 UTC a été respectée. Les nouvelles compilations ont commencé après sa clôture ; le dernier run des grilles s'est terminé à 07:12:50 UTC. Aucune mesure de vitesse n'est attribuée à ces sondes. La campagne lourde D était close, et l’oracle E terminé à 06:30:29,794 avant cette fenêtre. Ces faits ne certifient pas l’isolation de la machine.

La publication constructeur E `2b94abddfde08101607f4639d42149156fb39e6c` est présente sur `origin/main`. Son index est vide ; l'auditeur peut publier ses seuls fichiers à sa suite. Aucun fichier d'audit indépendant n'a été inclus dans le commit constructeur.

Le [contrôleur](verify_current.py) reconnaît désormais chaque snapshot complet D/E avec sa portée et rejette leurs mélanges ; 30 scènes passent en Python normal et optimisé. Le [manifeste](validation_current.json) est l’unique entrée maintenue. Les douze notes transitoires fusionnées ne restent pas des listes parallèles de demandes ; les octets antérieurs restent récupérables par le [registre de consolidation](receipts_front_20260905/documentation_retirement.json).

Archive A1, classification C1, mode mono, Cassini/U320, index et MEB ne sont plus des demandes ouvertes. Les détails et reçus restent accessibles dans la [synthèse](AUDIT_INDEPENDANT_20260904.md). GCP non utilisé. Aucun code produit modifié.
