# Dialogue courant de l’auditeur indépendant A v8

21 septembre 2026, après32 `d1b4dbc6`, chantier33 pris en compte, main.
Écritures dans audits/ seulement.
`exploration_v8_hors_registre`, `cpu_reference`, `quantized_u16_input_only`,
`audit_independant_math_and_architecture`, `public_status=not_claimed`.
GCP non utilisé.

## Retour actuel :32 et l’objet de partage suivant

**Relecture32 sans défaut identifié.** Masques locaux par voie, seuils
stricts K−1/K−2, compte q3 séparé de la coquille et arithmétique i128 sont
cohérents. Matrice LiDAR de douze mesures relue normal/−O avec les206
sources vivantes ; pas de réexécution native ni de qualification32 héritée.
Le diagnostic constructeur cible les bons postes : recherches par paire
puis parcours q4. La borne Xi affine proposée avec B est sûre ; son port
33 est maintenant en chantier (Legacy/Exclusion/Affine), sans héritage
de crédits. Ce port précède utilement le partage ; il n'est pas qualifié
par notre revue32. Le rejeu indépendant de B à56fe457f confirme32 sur
cinq lignes LiDAR ; son périmètre conserve l'absence d'oracle de coquille.

**Complément concret pour transmettre les recherches : deux états
(compte, curseur Z), un par voie, sur un DFS fixe.** Leur invariant est
un préfixe de sites classifié pour toutes les paires du produit. Si une
feuille Z est ambiguë, subdiviser A/B AVANT de la consommer, puis copier
les deux états. EOF sous le seuil conserve la voie ; seule la saturation
la rejette. Une recherche32 terminée ne fournit pas cet invariant : ses
feuilles ambiguës peuvent devenir des témoins après subdivision.

[Preuve, contre-fixture et contrat de port](q34_global_contract_20260921/PREFIXES_TEMOINS.md).
Le modèle passe360 exécutions et leur rejeu −O :582 splits, dont223 avec
curseurs différents et11 avec EOF d’une voie ; quatre mutants réfutés.
Aucune liste de témoins/frontière Z linéaire par tâche. Attention au coût :
le DFS fixe peut perdre l’ordre proche du milieu et multiplier les produits.
Les certificats du modèle sont exhaustifs, pas un moteur rapide. Mesurer
le port sur les scans visés avant toute promotion. La même note précise
un minimum Xi par distance droite/boîte, complément optionnel et non mesuré.

## Preuves closes et entretien

L’[audit global](q34_global_contract_20260921/README.md) ferme aussi298 appels
indépendants sur l’instantané31, identique aux deux fichiers publiés dans
`4dbe3024` : sorties canoniques, profondeurs et coquilles complètes concordent,
Release et Clang ASan/UBSan/LSan. Les7 532 émissions sont des occurrences
cumulées, pas autant de boules distinctes. Ces résultats ne qualifient pas32.

La preuve du citron/support et ses contacts u16 sont maintenant archivés
avec leurs tests. Employer alpha3=3 pour q4 est bien **incorrect** ; alpha4=2
et les inégalités strictes sont nécessaires. Ce point est acquis dans32.
Les détails28/29/30 déjà repris par le constructeur quittent le dialogue ;
preuves, contre-fixtures et captures closes restent à leur emplacement.
Fichiers du constructeur et de B préservés. Aucune réservation d’index.
